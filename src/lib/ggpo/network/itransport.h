#ifndef _I_TRANSPORT_H_
#define _I_TRANSPORT_H_

#include <stdint.h>
#include <string.h>
#include <winsock2.h>

struct TransportAddress
{
    enum Type
    {
        UDP,
        EOS
    };

    Type type;

    union
    {
        sockaddr_in udp;
        char eos_product_user_id[128];
    };

    TransportAddress()
    {
        memset(this, 0, sizeof(*this));
    }
};

class ITransport
{
public:
    virtual ~ITransport() = default;

    virtual bool SendTo(
        const char* buffer,
        int len,
        const TransportAddress& destination
    ) = 0;

    virtual void Close() = 0;
};

#endif
