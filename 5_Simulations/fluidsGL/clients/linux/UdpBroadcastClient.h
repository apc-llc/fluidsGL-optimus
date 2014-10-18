#ifndef UDP_BROADCAST_CLIENT_H
#define UDP_BROADCAST_CLIENT_H

#include "UdpBroadcastServer.h"

#include <cstdio>
#include <netinet/in.h>

class UdpBroadcastClient
{
	int s;                   /* Socket */
	int len_inet;            /* length */
	struct sockaddr_in adr;  /* AF_INET */
	struct sockaddr_in adr_srvr;
	
	void initialize(const char *bc_addr);

public :

	UdpBroadcastClient(const char *bc_addr);

	UdpBroadcastClient();
	
	// Get display configuration from the broadcast.
	void configure(DisplayConfig& config);

	// Listen to the broadcast and update the specified array
	// with the signal contents. Optionally report signal rate.
	void listen(char* signal, double* rate = NULL);
	
	// Send feedback commands to server.
	void feedback(FeedbackConfig& config);
};

#endif // UDP_BROADCAST_CLIENT_H

