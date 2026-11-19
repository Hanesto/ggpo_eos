/* -----------------------------------------------------------------------
 * GGPO.net (http://ggpo.net)  -  Copyright 2009 GroundStorm Studios, LLC.
 *
 * Use of this software is governed by the MIT license that can be found
 * in the LICENSE file.
 */

#ifndef _UDP_H
#define _UDP_H

#include "poll.h"
#include "udp_msg.h"
#include "ggponet.h"
#include "ring_buffer.h"
#include "itransport.h"

#define MAX_UDP_ENDPOINTS     16

static const int MAX_UDP_PACKET_SIZE = 4096;

class Udp : public ITransport, public IPollSink
{
public:
   struct Stats {
      int      bytes_sent;
      int      packets_sent;
      float    kbps_sent;
   };

   struct Callbacks {
      virtual ~Callbacks() { }
      virtual void OnMsg(const TransportAddress& from, UdpMsg *msg, int len) = 0;
   };

protected:
   void Log(const char *fmt, ...);

public:
   Udp();

   void Init(uint16 port, Poll *p, Callbacks *callbacks);
   
   bool SendTo(const char* buffer, int len, const TransportAddress& destination) override;

   void Close() override;

   virtual bool OnLoopPoll(void *cookie);

public:
   ~Udp(void);

protected:
   // Network transmission information
   SOCKET         _socket;

   // state management
   Callbacks      *_callbacks;
   Poll           *_poll;
};

#endif
