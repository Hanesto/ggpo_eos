#ifndef _EOS_TRANSPORT_H_
#define _EOS_TRANSPORT_H_

#include "itransport.h"
#include "poll.h"

#include <stdint.h>
#include <string>
#include <functional>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>

// EOS callback shared with the GGPO backend and bridge.
typedef bool (*EosSendCallback)(
    const char* remote_product_user_id,
    const char* packet,
    int packet_size
);

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

    EosTransport();
    virtual ~EosTransport();

    // Initialize and register with GGPO's poll system.
    void Init(Poll* poll);

    // Register callbacks.
    void SetSendCallback(SendCallback callback);
    void SetReceiveCallback(ReceiveCallback callback);

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

    static const size_t MAX_PACKET_SIZE = 64 * 1024;
    static const size_t MAX_QUEUE_PACKETS = 1024;
    static const size_t MAX_QUEUE_BYTES = 4 * 1024 * 1024;

    void FinishCallback();

    SendCallback _send_callback;
    ReceiveCallback _receive_callback;

    std::string _local_product_user_id;

    std::queue<ReceivedPacket> _receive_queue;

    size_t _queued_bytes;
    size_t _active_callbacks;

    Poll* _poll;

    std::mutex _mutex;
    std::condition_variable _callbacks_finished;

    bool _initialized;
};

#endif
