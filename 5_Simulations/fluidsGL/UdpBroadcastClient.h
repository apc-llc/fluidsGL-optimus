#ifndef UDP_BROADCAST_CLIENT_H
#define UDP_BROADCAST_CLIENT_H

#include "UdpBroadcastServer.h"

#include <netinet/in.h>

class UdpBroadcastClient
{
	int s;                   /* Socket */
	int len_inet;            /* length */
	struct sockaddr_in adr;  /* AF_INET */

	void initialize(const char *bc_addr);

public :

	UdpBroadcastClient(const char *bc_addr);

	UdpBroadcastClient();
	
	// Get display configuration from the broadcast.
	void configure(DisplayConfig& config);

	// Listen to the broadcast the specified amount of milliseconds.
	void listen(char* signal);
};

#endif // UDP_BROADCAST_CLIENT_H
