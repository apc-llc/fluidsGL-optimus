#include "UdpBroadcastClient.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
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
	fputs(strerror(errno),stderr);
	fputs(": ",stderr);
	fputs(on_what,stderr);
	fputc('\n',stderr);
	exit(1);
}

void UdpBroadcastClient::initialize(const char *bc_addr)
{
	static int so_reuseaddr = TRUE;

	// Create a UDP socket to use
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
		displayError("socket()");

	// Form the broadcast address
	len_inet = sizeof(adr);

	int z = mkaddr(&adr, &len_inet, bc_addr, "udp");

	if (z == -1)
		displayError("Bad broadcast address");

	// Allow multiple listeners on the broadcast address
	z = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));

	if (z == -1)
		displayError("setsockopt(SO_REUSEADDR)");

	// Bind our socket to the broadcast address
	z = bind(s, (struct sockaddr *)&adr, len_inet);

	if (z == -1)
		displayError("bind(2)");
}

UdpBroadcastClient::UdpBroadcastClient(const char *bc_addr)
{
	initialize(bc_addr);
}

UdpBroadcastClient::UdpBroadcastClient()
{
	initialize("127.255.255.2:9097");
}

using namespace std;

// Get display configuration from the broadcast.
void UdpBroadcastClient::configure(DisplayConfig& config)
{
	vector<char> vpacket;
	vpacket.resize(UdpBroadcastServer::PacketSize + sizeof(unsigned int));

	for (;;)
	{
		char* packet = &vpacket[0];

		// Wait for a broadcast message
		socklen_t x = 0;
		int z = recvfrom(s,                    /* Socket */
			         packet,                   /* Receiving buffer */
			         vpacket.size(),           /* Max rcv buf size */
			         0,                        /* Flags: no options */
			         (struct sockaddr *)&adr,  /* Addr */
			         &x);                      /* Addr len, in & out */

		if (z < 0)
			displayError("recvfrom(2)");

		int ipacket = *(unsigned int*)packet;
		if (ipacket != 0) continue;

		packet += sizeof(unsigned int);
		config = *(DisplayConfig*)(packet);
		return;
	}
	
	printf("One\n");
}

// Listen to the broadcast the specified amount of milliseconds.
void UdpBroadcastClient::listen(char* signal)
{
	vector<char> vpacket;
	vpacket.resize(UdpBroadcastServer::PacketSize + sizeof(unsigned int));

	for (;;)
	{
		char* packet = &vpacket[0];

		// Wait for a broadcast message
		socklen_t x = 0;
		int z = recvfrom(s,                    /* Socket */
			         packet,                   /* Receiving buffer */
			         vpacket.size(),           /* Max rcv buf size */
			         0,                        /* Flags: no options */
			         (struct sockaddr *)&adr,  /* Addr */
			         &x);                      /* Addr len, in & out */

		if (z != vpacket.size())
			displayError("recvfrom(2)");

		int ipacket = *(unsigned int*)packet;
		if (ipacket == 0) continue;

		//printf("ipacket = %d\n", ipacket);

		packet += sizeof(unsigned int);
		memcpy(signal + (ipacket - 1) * UdpBroadcastServer::PacketSize, packet,
			UdpBroadcastServer::PacketSize);
	}
}

