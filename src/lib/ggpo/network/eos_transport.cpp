#include "eos_transport.h"

#include <string.h>
#include <utility>

EosTransport::EosTransport()
    : _initialized(false)
{
}

EosTransport::~EosTransport()
{
    Close();
}

void
EosTransport::Init(Poll* poll)
{
    if (!poll){ return; }

    poll->RegisterLoop(this);
    _initialized = true;
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
    if (!_initialized){ return false; }
    if (!buffer || len <= 0){ return false; }
    if (destination.type != TransportAddress::EOS){ return false; }
    if (!_send_callback){ return false; }

    const char* remote_id = destination.eos_product_user_id;

    if (remote_id[0] == '\0'){ return false; }

    return _send_callback(remote_id, buffer, len);
}

void
EosTransport::ReceivePacket(const char* remote_product_user_id, const uint8_t* data, int len)
{
    if (!_initialized || !remote_product_user_id || !data || len <= 0 ||
        strlen(remote_product_user_id) >= sizeof(TransportAddress::eos_product_user_id))
    {
        return;
    }

    ReceivedPacket packet;

    packet.from.type = TransportAddress::EOS;

    strncpy_s(packet.from.eos_product_user_id, remote_product_user_id, sizeof(packet.from.eos_product_user_id) - 1);
    packet.from.eos_product_user_id[sizeof(packet.from.eos_product_user_id) - 1] = '\0';

    packet.data.assign(data, data + len);

    _receive_queue.push(packet);
}

bool
EosTransport::OnLoopPoll(void* cookie)
{
    while (!_receive_queue.empty())
    {
        ReceivedPacket packet = std::move(_receive_queue.front());
        _receive_queue.pop();

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

    while (!_receive_queue.empty()){ _receive_queue.pop(); }

    _send_callback = nullptr;
    _receive_callback = nullptr;
}
