#pragma once

#include "../network/eos_transport.h"

#ifdef _WIN32
#define GGPO_EOS_EXPORT extern "C" __declspec(dllexport)
#else
#define GGPO_EOS_EXPORT extern "C"
#endif

GGPO_EOS_EXPORT void GGPO_SetEosTransport(EosTransport* transport);