#pragma once

#include "../network/eos_transport.h"

#include <stdint.h>

#ifdef _WIN32
#define GGPO_EOS_EXPORT extern "C" __declspec(dllexport)
#else
#define GGPO_EOS_EXPORT extern "C"
#endif

void RegisterEosTransportCallbacks(EosTransport* transport);

typedef bool (*EosSendCallback)(
    const char* remote_product_user_id,
    const char* packet,
    int packet_size
);

GGPO_EOS_EXPORT void GGPO_RegisterEosSendCallback(
    EosSendCallback callback
);

GGPO_EOS_EXPORT void GGPO_UnregisterEosSendCallback();

GGPO_EOS_EXPORT void GGPO_ReceiveEosPacket(
    EosTransport* transport,
    const char* remote_product_user_id,
    const uint8_t* packet,
    int packet_size
);