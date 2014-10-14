#ifndef UDP_BROADCAST_SERVER_H
#define UDP_BROADCAST_SERVER_H

#include <netinet/in.h>

struct DisplayConfig
{
	int width;
	int height;
	int szpoint;
};

class UdpBroadcastServer
{
	int s;                       /* Socket */
	int len_bc;
	struct sockaddr_in adr_bc;   /* AF_INET */

	void initialize(const char *sv_addr, const char *bc_addr);

public :

	static const unsigned int PacketSize = 32768;

	UdpBroadcastServer(const char *sv_addr, const char *bc_addr);

	UdpBroadcastServer();

	void broadcast(char* msg, int width, int height, int szpoint);
};

#endif // UDP_BROADCAST_SERVER_H

