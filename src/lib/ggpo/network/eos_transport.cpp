#include "eos_transport.h"

#include <string.h>
#include <utility>

EosTransport::EosTransport() : _queued_bytes(0), _active_callbacks(0), _poll(nullptr), _initialized(false){ }

EosTransport::~EosTransport()
{
    Close();
}

void
EosTransport::Init(Poll* poll)
{
    if (!poll){ return; }
    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (_initialized){ return; }

        _poll = poll;
        _initialized = true;
    }

    // Register on the GGPO/game thread.
    poll->RegisterLoop(this);
}

void
EosTransport::SetSendCallback(SendCallback callback)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _send_callback = std::move(callback);
}

void
EosTransport::SetReceiveCallback(ReceiveCallback callback)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _receive_callback = std::move(callback);
}

void
EosTransport::SetLocalProductUserId(const char* product_user_id)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!product_user_id)
    {
        _local_product_user_id.clear();
        return;
    }

    _local_product_user_id = product_user_id;
}

bool
EosTransport::SendTo(const char* buffer, int len, const TransportAddress& destination)
{
    if (!buffer || len <= 0){ return false; }

    if (destination.type != TransportAddress::EOS){ return false; }

    const char* remote_id = destination.eos_product_user_id;

    if (!remote_id){ return false; }

    const size_t id_capacity = sizeof(destination.eos_product_user_id);

    if (!memchr(remote_id, '\0', id_capacity)){ return false; }
    if (remote_id[0] == '\0'){ return false; }

    SendCallback callback;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_initialized || !_send_callback)
            return false;

        callback = _send_callback;
        ++_active_callbacks;
    }

    bool result = false;

    try
    {
        result = callback(remote_id, buffer, len);
    }
    catch (...)
    {
        result = false;
    }

    FinishCallback();

    return result;
}

void
EosTransport::ReceivePacket(const char* remote_product_user_id, const uint8_t* data, int len)
{
    if (!remote_product_user_id || !data || len <= 0){ return; }
    if (static_cast<size_t>(len) > MAX_PACKET_SIZE){ return; }

    ReceivedPacket packet;

    packet.from.type = TransportAddress::EOS;

    const size_t id_capacity = sizeof(packet.from.eos_product_user_id);

    // Avoid reading beyond the fixed-size destination ID.
    size_t id_length = 0;

    while (id_length < id_capacity && remote_product_user_id[id_length] != '\0')
    {
        ++id_length;
    }

    if (id_length == 0 || id_length >= id_capacity){ return; }

    memcpy(packet.from.eos_product_user_id, remote_product_user_id, id_length + 1);

    packet.data.assign(data, data + len);

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_initialized){ return; }
        if (_receive_queue.size() >= MAX_QUEUE_PACKETS){ return; }
        if (_queued_bytes + packet.data.size() > MAX_QUEUE_BYTES){ return; }

        _queued_bytes += packet.data.size();

        _receive_queue.push(std::move(packet));
    }
}

bool
EosTransport::OnLoopPoll(void* cookie)
{
    std::queue<ReceivedPacket> pending_packets;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_initialized){ return true; }

        std::swap(pending_packets, _receive_queue);

        _queued_bytes = 0;
    }

    while (!pending_packets.empty())
    {
        ReceivedPacket packet = std::move(pending_packets.front());

        pending_packets.pop();

        ReceiveCallback callback;

        {
            std::lock_guard<std::mutex> lock(_mutex);

            if (!_initialized){ break; }
            if (!_receive_callback){ continue; }

            callback = _receive_callback;

            ++_active_callbacks;
        }

        try
        {
            callback(
                packet.from,
                reinterpret_cast<const char*>(packet.data.data()),
                static_cast<int>(packet.data.size())
            );
        }
        catch (...)
        {
            // Do not allow exceptions to escape into GGPO.
        }

        FinishCallback();
    }

    return true;
}

void
EosTransport::FinishCallback()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_active_callbacks > 0){ --_active_callbacks; }
    }

    _callbacks_finished.notify_all();
}

void
EosTransport::Close()
{
    Poll* poll = nullptr;

    {
        std::lock_guard<std::mutex> lock(_mutex);

        if (!_initialized && !_poll){ return; }

        // Stop accepting packets and new operations.
        _initialized = false;

        poll = _poll;
        _poll = nullptr;

        // Clear callbacks.
        _send_callback = nullptr;
        _receive_callback = nullptr;

        // Clear queued packets.
        std::queue<ReceivedPacket> empty_queue;

        std::swap(_receive_queue, empty_queue);

        _queued_bytes = 0;

        _local_product_user_id.clear();
    }

    // IMPORTANT:
    // This must execute on the same thread that owns Poll,
    // outside an active Poll::Pump() call.
    if (poll)  poll->RemoveLoop(this);

    // Wait for callbacks that were already executing.
    std::unique_lock<std::mutex> lock(_mutex);

    _callbacks_finished.wait(
        lock,
        [this]()
        {
            return _active_callbacks == 0;
        }
    );
}
