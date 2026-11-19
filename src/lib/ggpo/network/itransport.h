#ifndef _I_TRANSPORT_H_
#define _I_TRANSPORT_H_

class ITransport
{
public:
    virtual ~ITransport() {}

    virtual void SendTo(
        char* buffer,
        int len,
        int flags,
        struct sockaddr* dst,
        int destlen
    ) = 0;
};

#endif