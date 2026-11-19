#ifndef _EOS_TRANSPORT_H_
#define _EOS_TRANSPORT_H_

#include "itransport.h"
#include "poll.h"

#include <stdint.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <queue>
#include <vector>

class EosTransport : public ITransport, public IPollSink
{
public:

    typedef std::function<bool(
        const char* remote_product_user_id,
        const char* buffer,
        int len
    )> SendCallback;

    typedef std::function<void(
        const TransportAddress& from,
        const char* buffer,
        int len
    )> ReceiveCallback;

    void SetReceiveCallback(ReceiveCallback callback);

    EosTransport();
    virtual ~EosTransport();

    // Initialize the EOS transport.
    void Init(Poll* poll);

    // Register callback used to send packets through Unity/EOS.
    void SetSendCallback(SendCallback callback);

    // Set local EOS identity.
    void SetLocalProductUserId(const char* product_user_id);

    // ITransport implementation.
    bool SendTo(
        const char* buffer,
        int len,
        const TransportAddress& destination
    ) override;

    void Close() override;

    // IPollSink implementation.
    bool OnLoopPoll(void* cookie) override;

    // Called by the Unity/C# bridge when an EOS packet arrives.
    void ReceivePacket(
        const char* remote_product_user_id,
        const uint8_t* data,
        int len
    );

private:

    struct ReceivedPacket
    {
        TransportAddress from;
        std::vector<uint8_t> data;
    };

    SendCallback _send_callback;

    std::string _local_product_user_id;

    std::queue<ReceivedPacket> _receive_queue;

    ReceiveCallback _receive_callback;

    bool _initialized;
};

#endif
