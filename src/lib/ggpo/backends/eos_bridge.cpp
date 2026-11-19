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