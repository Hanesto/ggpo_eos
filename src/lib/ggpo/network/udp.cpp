/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#include "types.h"
#include "udp.h"

#include <utility>
#include <string.h>

static Udp::EosSendCallback g_eos_send_callback = NULL;
std::queue<Udp::EosPacket> Udp::_eos_receive_queue;
std::mutex Udp::_eos_queue_mutex;

SOCKET
CreateSocket(uint16 bind_port, int retries)
{
   SOCKET s;
   sockaddr_in sin;
   uint16 port;
   int optval = 1;

   s = socket(AF_INET, SOCK_DGRAM, 0);
   setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof optval);
   setsockopt(s, SOL_SOCKET, SO_DONTLINGER, (const char *)&optval, sizeof optval);

   // non-blocking...
   u_long iMode = 1;
   ioctlsocket(s, FIONBIO, &iMode);

   sin.sin_family = AF_INET;
   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   for (port = bind_port; port <= bind_port + retries; port++) {
      sin.sin_port = htons(port);
      if (bind(s, (sockaddr *)&sin, sizeof sin) != SOCKET_ERROR) {
         Log("Udp bound to port: %d.\n", port);
         return s;
      }
   }
   closesocket(s);
   return INVALID_SOCKET;
}

Udp::Udp() :
   _socket(INVALID_SOCKET),
   _callbacks(NULL),
   _poll(NULL)
{
}

Udp::~Udp(void)
{
   Close();
}

void
Udp::Init(uint16 port, Poll *poll, Callbacks *callbacks)
{
   _callbacks = callbacks;

   _poll = poll;
   _poll->RegisterLoop(this);

   Log("binding udp socket to port %d.\n", port);
   _socket = CreateSocket(port, 0);

   if (_socket == INVALID_SOCKET)
   {
      Log("Failed to create UDP socket.\n");
      return;
   }
}


bool
Udp::SendTo(const char* buffer, int len, const TransportAddress& destination)
{
   if (buffer == NULL || len <= 0){ return false; }

   // Existing UDP transport.
   if (destination.type == TransportAddress::UDP)
   {
      if (_socket == INVALID_SOCKET){ return false; }

      if (destination.udp.sin_family != AF_INET ||
         destination.udp.sin_addr.s_addr == 0 ||
         destination.udp.sin_port == 0)
      {
         return false;
      }

      int res = sendto(_socket, buffer, len, 0, (const sockaddr*)&destination.udp, sizeof(destination.udp));

      if (res == SOCKET_ERROR)
      {
         DWORD err = WSAGetLastError();
         Log("sendto failed (error: %lu).\n", err);
         return false;
      }

      return res == len;
   }

   if (destination.type == TransportAddress::EOS)
   {
      if (g_eos_send_callback == NULL)
      {
         Log("EOS send callback is not registered.\n");
         return false;
      }

      return g_eos_send_callback(destination.eos_product_user_id, buffer, len);
   }

   return false;
}

bool
Udp::OnLoopPoll(void *cookie)
{
   // Process existing UDP packets.
   if (_socket != INVALID_SOCKET)
   {
      uint8 recv_buf[MAX_UDP_PACKET_SIZE];
      sockaddr_in recv_addr;
      int recv_addr_len;

      for (;;)
      {
         recv_addr_len = sizeof(recv_addr);

         int len = recvfrom(_socket, (char *)recv_buf, MAX_UDP_PACKET_SIZE, 0, (struct sockaddr *)&recv_addr, &recv_addr_len);

         if (len == SOCKET_ERROR)
         {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK){ Log("recvfrom error: %d\n", error); }
            break;
         }

         if (len > 0 && _callbacks)
         {
            TransportAddress from;
            from.type = TransportAddress::UDP;
            from.udp = recv_addr;

            UdpMsg *msg = (UdpMsg *)recv_buf;

            _callbacks->OnMsg(from, msg, len);
         }
      }
   }

   // Process incoming EOS packets.
   std::queue<EosPacket> pending_packets;

   {
      std::lock_guard<std::mutex> lock(_eos_queue_mutex);
      std::swap(pending_packets, _eos_receive_queue);
   }

   while (!pending_packets.empty())
   {
      EosPacket packet = std::move(pending_packets.front());
      pending_packets.pop();

      if (!_callbacks || packet.data.size() < sizeof(UdpMsg::hdr))
      {
         continue;
      }

      if (packet.remote_product_user_id.empty() ||
          packet.remote_product_user_id.size() >= sizeof(TransportAddress::eos_product_user_id))
      {
         continue;
      }

      TransportAddress from;
      from.type = TransportAddress::EOS;

      memcpy(
         from.eos_product_user_id,
         packet.remote_product_user_id.c_str(),
         packet.remote_product_user_id.size() + 1
      );

      UdpMsg *msg = (UdpMsg *)packet.data.data();

      _callbacks->OnMsg(
         from,
         msg,
         (int)packet.data.size()
      );
   }

   return true;
}


void
Udp::Close()
{
   if (_socket != INVALID_SOCKET) {
      closesocket(_socket);
      _socket = INVALID_SOCKET;
   }
}

void
Udp::SetEosSendCallback(EosSendCallback callback)
{
   g_eos_send_callback = callback;
}

void
Udp::ReceiveEosPacket(const char* remote_product_user_id, const char* packet, int packet_size)
{
   if (!remote_product_user_id || !packet || packet_size <= 0){ return; }

   EosPacket incoming;

   incoming.remote_product_user_id = remote_product_user_id;
   incoming.data.assign(packet, packet + packet_size);

   std::lock_guard<std::mutex> lock(_eos_queue_mutex);

   _eos_receive_queue.push(std::move(incoming));
}

void
Udp::Log(const char *fmt, ...)
{
   char buf[1024];
   size_t offset;
   va_list args;

   strcpy_s(buf, "udp | ");
   offset = strlen(buf);
   va_start(args, fmt);
   vsnprintf(buf + offset, ARRAY_SIZE(buf) - offset - 1, fmt, args);
   buf[ARRAY_SIZE(buf)-1] = '\0';
   ::Log(buf);
   va_end(args);
}
