#include "UdpBroadcastServer.h"

#include <algorithm>
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
	// Form the server address
	int len_srvr = sizeof(adr_srvr);
	int z = mkaddr(&adr_srvr,  /* Returned address */
		       &len_srvr,      /* Returned length */
		       sv_addr,        /* Input string addr */
		       "udp");         /* UDP protocol */

	if (z == -1)
		displayError("Bad server address");

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
	static int so_broadcast = TRUE;
	z = setsockopt(s, SOL_SOCKET, SO_BROADCAST, &so_broadcast, sizeof(so_broadcast));
	 
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

	initialize("127.0.0:9097", "127.255.255.2 9097");
}

using namespace std;

vector<int> packets_indexes;

void UdpBroadcastServer::feedback(FeedbackConfig& config)
{
	// Wait for a broadcast message
	socklen_t x = 0;
	int z = recvfrom(s,                       /* Socket */
		         &config,                     /* Receiving buffer */
		         sizeof(config),              /* Max rcv buf size */
		         0,                           /* Flags: no options */
		         (struct sockaddr *)&adr_bc,  /* Addr */
		         &x);                         /* Addr len, in & out */
}

void UdpBroadcastServer::broadcast(
	char* packets, int width, int height, int szpoint, int wstep, int hstep)
{
	// Send the zero packet, which shall contain the screen dimensions.
	{
		vector<char> vpacket;
		vpacket.resize(UdpBroadcastServer::PacketSize + sizeof(unsigned int));
		char* packet = (char*)&vpacket[0];
		unsigned int* index = (unsigned int*)packet;
		*index = 0;
		DisplayConfig* config = (DisplayConfig*)(packet + sizeof(unsigned int));
		config->width = width / wstep;
		config->height = height / hstep;
		config->szpoint = szpoint;
		config->adr_srvr = adr_srvr;

		int z = sendto(s, packet,
			UdpBroadcastServer::PacketSize + sizeof(unsigned int),
			0, (struct sockaddr*)&adr_bc, len_bc);

		if (z == -1)
			displayError("sendto()");
	}

	int npackets = sizeof(float) * (width / wstep) * (height / hstep) / UdpBroadcastServer::PacketSize;
	if (sizeof(float) * (width / wstep) * (height / hstep) % UdpBroadcastServer::PacketSize)
		npackets++;

	if (packets_indexes.size() != npackets)
	{
		packets_indexes.resize(npackets);
		for (int i = 0; i < npackets; i++)
			packets_indexes[i] = i;
	}

	random_shuffle(packets_indexes.begin(), packets_indexes.end());

	for (int ii = 0; ii < npackets; ii++)
	{
		int i = packets_indexes[ii];

		char* packet = packets + i * (UdpBroadcastServer::PacketSize + sizeof(unsigned int));
		
		int z = sendto(s, packet,
			UdpBroadcastServer::PacketSize + sizeof(unsigned int),
			0, (struct sockaddr*)&adr_bc, len_bc);

		if (z == -1)
			displayError("sendto()");

		//printf("ipacket = %d\n", i + 1);
	}
}

