#include "UdpBroadcastServer.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
 
#ifndef TRUE
#define TRUE 1
#endif
 
extern "C" int mkaddr(void *addr, int *addrlen, const char *str_addr, const char *protocol);
 
// This function reports the error and exits back to the shell
static void displayError(const char *on_what)
{
	fputs(strerror(errno), stderr);
	fputs(": ", stderr);
	fputs(on_what, stderr);
	fputc('\n', stderr);
	exit(1);
}

void UdpBroadcastServer::initialize(const char *sv_addr, const char *bc_addr)
{
	struct sockaddr_in adr_srvr; /* AF_INET */
	static int so_broadcast = TRUE;
 
	// Form the server address
	int len_srvr = sizeof(adr_srvr);
#if 1
	int z = mkaddr(&adr_srvr,  /* Returned address */
		       &len_srvr,      /* Returned length */
		       sv_addr,        /* Input string addr */
		       "udp");         /* UDP protocol */

	if (z == -1)
		displayError("Bad server address");
#endif
#if 0
	adr_srvr.sin_family = AF_INET;
    adr_srvr.sin_port = htons(9097);
    adr_srvr.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
	// Form the broadcast address
	len_bc = sizeof(adr_bc); 
	z = mkaddr(&adr_bc,  /* Returned address */
		       &len_bc,  /* Returned length */
		       bc_addr,  /* Input string addr */
		       "udp");   /* UDP protocol */

	if (z == -1)
		displayError("Bad broadcast address");
	 
	// Create a UDP socket to use
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		displayError("socket()");
	 
	// Allow broadcasts
	z = setsockopt(s, SOL_SOCKET,
		           SO_BROADCAST,
		           &so_broadcast,
		           sizeof(so_broadcast));
	 
	if (z == -1)
		displayError("setsockopt(SO_BROADCAST)");

	// Bind an address to our socket, so that
	// client programs can listen to this server:
	z = bind(s, (struct sockaddr *)&adr_srvr, len_srvr);

	if ( z == -1 )
		displayError("bind()");	 
}

UdpBroadcastServer::UdpBroadcastServer(const char *sv_addr, const char *bc_addr)
{
	if (PacketSize % sizeof(float))
	{
		fprintf(stderr, "PacketSize must be a multiply of sizeof(float)\n");
		exit(1);
	}

	initialize(sv_addr, bc_addr);
}

UdpBroadcastServer::UdpBroadcastServer()
{
	if (PacketSize % sizeof(float))
	{
		fprintf(stderr, "PacketSize must be a multiply of sizeof(float)\n");
		exit(1);
	}

	initialize("127.0.0:*", "127.255.255.2 9097");
}

using namespace std;

void UdpBroadcastServer::broadcast(char* packets, int width, int height, int szpoint)
{
	// Send the zero packet, which shall contain the screen dimensions.
	{
		vector<char> vpacket;
		vpacket.resize(UdpBroadcastServer::PacketSize + sizeof(unsigned int));
		char* packet = (char*)&vpacket[0];
		unsigned int* index = (unsigned int*)packet;
		*index = 0;
		DisplayConfig* config = (DisplayConfig*)(packet + sizeof(unsigned int));
		config->width = width;
		config->height = height;
		config->szpoint = szpoint;

		int z = sendto(s, packet,
			UdpBroadcastServer::PacketSize + sizeof(unsigned int),
			0, (struct sockaddr*)&adr_bc, len_bc);

		if (z == -1)
			displayError("sendto()");
	}

	int npackets = sizeof(float) * width * height / UdpBroadcastServer::PacketSize;
	if (sizeof(float) * width * height % UdpBroadcastServer::PacketSize)
		npackets++;

	for (int i = 0; i < npackets; i++)
	{
		char* packet = packets + i * (UdpBroadcastServer::PacketSize + sizeof(unsigned int));
		
		int z = sendto(s, packet,
			UdpBroadcastServer::PacketSize + sizeof(unsigned int),
			0, (struct sockaddr*)&adr_bc, len_bc);

		if (z == -1)
			displayError("sendto()");

		//printf("ipacket = %d\n", i + 1);
	}
}

