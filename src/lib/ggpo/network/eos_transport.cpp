#include "eos_transport.h"

#include <string.h>
#include <utility>
#include <mutex>

EosTransport::EosTransport()
    : _initialized(false),
    _send_callback(nullptr),
    _receive_callback(nullptr)
{
}

EosTransport::~EosTransport()
{
    Close();
}

void
EosTransport::Init(Poll* poll)
{
    if (!poll || _initialized){ return; }

    _initialized = true;

    poll->RegisterLoop(this);
}

void
EosTransport::SetSendCallback(SendCallback callback)
{
    _send_callback = callback;
}

void
EosTransport::SetLocalProductUserId(const char* product_user_id)
{
    if (!product_user_id)
    {
        _local_product_user_id.clear();
        return;
    }

    _local_product_user_id = product_user_id;
}

void
EosTransport::SetReceiveCallback(ReceiveCallback callback)
{
    _receive_callback = callback;
}

bool
EosTransport::SendTo(const char* buffer, int len, const TransportAddress& destination)
{
    if (!_initialized || !buffer || len <= 0){ return false; }
    if (destination.type != TransportAddress::EOS){ return false; }

    const char* remote_id = destination.eos_product_user_id;

    if (remote_id[0] == '\0'){ return false; }
    if (!_send_callback){ return false; }

    return _send_callback(remote_id, buffer, len);
}

void
EosTransport::ReceivePacket(const char* remote_product_user_id, const uint8_t* data, int len)
{
    if (!remote_product_user_id || !data || len <= 0){ return; }
    if (strlen(remote_product_user_id) >= sizeof(TransportAddress::eos_product_user_id)){ return;}

    ReceivedPacket packet;

    packet.from.type = TransportAddress::EOS;

    strcpy_s(
        packet.from.eos_product_user_id,
        sizeof(packet.from.eos_product_user_id),
        remote_product_user_id
    );

    packet.data.assign(data, data + len);

    {
        std::lock_guard<std::mutex> lock(_receive_mutex);

        if (!_initialized){ return; }

        _receive_queue.push(std::move(packet));
    }
}

bool
EosTransport::OnLoopPoll(void* cookie)
{
    // Move incoming packets into a local queue.
    std::queue<ReceivedPacket> pending_packets;

    {
        std::lock_guard<std::mutex> lock(_receive_mutex);
        std::swap(pending_packets, _receive_queue);
    }

    // Process packets without holding the receive mutex.
    while (!pending_packets.empty())
    {
        ReceivedPacket packet = std::move(pending_packets.front());
        pending_packets.pop();

        if (!_receive_callback){ continue; }
        if (packet.data.empty()){ continue; }

        _receive_callback(packet.from, reinterpret_cast<const char*>(packet.data.data()), static_cast<int>(packet.data.size()));
    }

    return true;
}

void
EosTransport::Close()
{
    _initialized = false;

    {
        std::lock_guard<std::mutex> lock(_receive_mutex);

        std::queue<ReceivedPacket> empty_queue;
        std::swap(_receive_queue, empty_queue);
    }

    _send_callback = nullptr;
    _receive_callback = nullptr;

    _local_product_user_id.clear();
}
