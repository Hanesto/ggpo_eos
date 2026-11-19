#include "../network/eos_transport.h"

typedef bool (*EosSendCallback)(
    const char* remote_product_user_id,
    const char* packet,
    int packet_size
);

static EosSendCallback g_eos_send_callback = nullptr;

void RegisterEosTransportCallbacks(EosTransport* transport)
{
    if (!transport)
        return;

    transport->SetSendCallback(
        [](const char* remote_id, const char* packet, int len) -> bool
        {
            if (!g_eos_send_callback)
                return false;

            return g_eos_send_callback(remote_id, packet, len);
        }
    );
}

extern "C" __declspec(dllexport)
void GGPO_RegisterEosSendCallback(EosSendCallback callback)
{
    g_eos_send_callback = callback;
}

extern "C" __declspec(dllexport)
void GGPO_UnregisterEosSendCallback()
{
    g_eos_send_callback = nullptr;
}

extern "C" __declspec(dllexport)
void GGPO_ReceiveEosPacket(EosTransport* transport, const char* remote_product_user_id, const uint8_t* packet, int packet_size)
{
    if (!transport || !remote_product_user_id || !packet){ return; }
    if (packet_size <= 0){ return; }

    transport->ReceivePacket(remote_product_user_id, packet, packet_size);
}