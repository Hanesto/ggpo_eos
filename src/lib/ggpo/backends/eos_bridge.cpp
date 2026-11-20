#include "../network/eos_transport.h"
#include "backend.h"
#include "ggponet.h"
#include "backends/p2p.h"

#include <stdint.h>
#include <mutex>

typedef bool (*EosSendCallback)(
    const char* remote_product_user_id,
    const char* packet,
    int packet_size
);

static EosSendCallback g_eos_send_callback = nullptr;
static std::mutex g_eos_callback_mutex;

// Safely retrieve the currently registered callback.
static EosSendCallback GetEosSendCallback()
{
    std::lock_guard<std::mutex> lock(g_eos_callback_mutex);
    return g_eos_send_callback;
}

void RegisterEosTransportCallbacks(EosTransport* transport)
{
    if (!transport){ return; }

    transport->SetSendCallback(
        [](const char* remote_id, const char* packet, int len) -> bool
        {
            if (!remote_id || !packet || len <= 0){ return false; }

            EosSendCallback callback = GetEosSendCallback();

            if (!callback){ return false; }

            return callback(remote_id, packet, len);
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
    g_eos_send_callback = callback;
}

extern "C" __declspec(dllexport)
void GGPO_UnregisterEosSendCallback()
{
    std::lock_guard<std::mutex> lock(g_eos_callback_mutex);
    g_eos_send_callback = nullptr;
}

extern "C" __declspec(dllexport)
void GGPO_ReceiveEosPacket(GGPOSession* session, const char* remote_product_user_id, const uint8_t* packet, int packet_size)
{
    if (!session || !remote_product_user_id || !packet || packet_size <= 0){ return; }

    Peer2PeerBackend* backend = dynamic_cast<Peer2PeerBackend*>(session);

    if (!backend){ return; }

    backend->ReceiveEosPacket(remote_product_user_id, packet, packet_size);
}
