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

#include "half.hpp"

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

// Listen to the broadcast and update the specified array
// with the signal contents. Optionally report signal rate.
void UdpBroadcastClient::listen(char* signal, double* rate)
{
	vector<char> vpacket;
	vpacket.resize(UdpBroadcastServer::PacketSize + sizeof(unsigned int));

	for (int measure = 1; ; measure++, measure %= 8192)
	{
		struct timespec start;
		if (rate && (measure == 1))
		{
			clock_gettime(CLOCK_REALTIME, &start);
			measure++;
		}
	
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
		half_float::half* src = (half_float::half*)packet;
		float* dst = (float*)(signal + 2 * (ipacket - 1) * UdpBroadcastServer::PacketSize);
		for (int i = 0; i < UdpBroadcastServer::PacketSize / sizeof(half_float::half); i++)
			dst[i] = half_float::half_cast<float, std::round_to_nearest>(src[i]);
		
		if (rate && (measure == 0))
		{
			struct timespec finish;
			clock_gettime(CLOCK_REALTIME, &finish);
			double time = finish.tv_nsec / 1e9 + finish.tv_sec -
				start.tv_nsec / 1e9 - start.tv_sec;
			*rate = vpacket.size() * (8192 - 2) / 1024.0 / 1024.0 / time;
		}
	}
}

