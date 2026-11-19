#include "../network/eos_transport.h"

typedef bool (*EosSendCallback)(
    const char* remote_product_user_id,
    const char* packet,
    int packet_size
);

static EosTransport* g_eos_transport = nullptr;

extern "C" __declspec(dllexport)
void GGPO_SetEosTransport(EosTransport* transport)
{
    g_eos_transport = transport;
}

extern "C" __declspec(dllexport)
void GGPO_RegisterEosSendCallback(EosSendCallback callback)
{
    if (!g_eos_transport)
        return;

    g_eos_transport->SetSendCallback(
        [callback](
            const char* remote_id,
            const char* packet,
            int len
        ) -> bool
        {
            if (!callback)
                return false;

            return callback(remote_id, packet, len);
        }
    );
}