#include "../network/eos_transport.h"
#include "backend.h"
#include "ggponet.h"
#include "backends/p2p.h"

#include <stdint.h>
#include <mutex>
#include <condition_variable>

using EosSendCallback = bool (__cdecl *)(
    const char* remote_product_user_id,
    const char* packet,
    int packet_size
);

static EosSendCallback g_eos_send_callback = nullptr;

static std::mutex g_eos_callback_mutex;
static std::condition_variable g_eos_callback_cv;

static int g_active_eos_callbacks = 0;
static bool g_eos_callback_shutting_down = false;

static bool InvokeEosSendCallback(const char* remote_id, const char* packet, int len)
{
    if (!remote_id || !packet || len <= 0){ return false; }

    EosSendCallback callback = nullptr;

    {
        std::lock_guard<std::mutex> lock(g_eos_callback_mutex);

        if (g_eos_callback_shutting_down || g_eos_send_callback == nullptr)
        {
            return false;
        }

        callback = g_eos_send_callback;
        ++g_active_eos_callbacks;
    }

    bool result = false;

    try
    {
        result = callback(remote_id, packet, len);
    }
    catch (...)
    {
        result = false;
    }

    {
        std::lock_guard<std::mutex> lock(g_eos_callback_mutex);

        --g_active_eos_callbacks;

        if (g_active_eos_callbacks == 0){ g_eos_callback_cv.notify_all(); }
    }

    return result;
}

void RegisterEosTransportCallbacks(EosTransport* transport)
{
    if (!transport){ return; }

    transport->SetSendCallback(
        [](const char* remote_id, const char* packet, int len) -> bool
        {
            return InvokeEosSendCallback(remote_id, packet, len);
        }
    );
}

extern "C" __declspec(dllexport)
EosTransport* GGPO_CreateEosTransport()
{
    EosTransport* transport = new EosTransport();
    RegisterEosTransportCallbacks(transport);
    return transport;
}

extern "C" __declspec(dllexport)
void GGPO_DestroyEosTransport(EosTransport* transport)
{
    if (!transport){ return; }

    transport->Close();
    delete transport;
}

extern "C" __declspec(dllexport)
void GGPO_RegisterEosSendCallback(EosSendCallback callback)
{
    std::lock_guard<std::mutex> lock(g_eos_callback_mutex);

    g_eos_callback_shutting_down = false;
    g_eos_send_callback = callback;
}

extern "C" __declspec(dllexport)
void GGPO_UnregisterEosSendCallback()
{
    std::unique_lock<std::mutex> lock(g_eos_callback_mutex);

    g_eos_callback_shutting_down = true;

    g_eos_send_callback = nullptr;

    g_eos_callback_cv.wait(
        lock,
        []()
        {
            return g_active_eos_callbacks == 0;
        }
    );
}

extern "C" __declspec(dllexport)
void GGPO_ReceiveEosPacket(GGPOSession* session, const char* remote_product_user_id, const uint8_t* packet, int packet_size)
{
    if (!session || !remote_product_user_id || !packet || packet_size <= 0){ return; }

    Peer2PeerBackend* backend = dynamic_cast<Peer2PeerBackend*>(session);

    if (!backend){ return; }

    backend->ReceiveEosPacket(remote_product_user_id, packet, packet_size);
}
