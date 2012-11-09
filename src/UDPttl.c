// ----rawudp.c------
// Must be run by root lol! Just datagram, no payload/data
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

// The packet length
#define PCKT_LEN 8192
#define TTLMAXITER 20
#define RANDMAX 99999

int src_port = 10100;
int dst_port = 10100;
char *dst_IP = "141.212.108.135"; // set the destination IP address (owl.eecs.umich.edu)
char *src_IP;

// Can create separate header file (.h) for all headers' structure
// The IP header's structure
struct ipheader {
	 unsigned char      iph_ihl:5, iph_ver:4;
	 unsigned char      iph_tos;
	 unsigned short int iph_len;
	 unsigned short int iph_ident;
	 unsigned char      iph_flag;
	 unsigned short int iph_offset;
	 unsigned char      iph_ttl;
	 unsigned char      iph_protocol;
	 unsigned short int iph_chksum;
	 unsigned int       iph_sourceip;
	 unsigned int       iph_destip;
};

// UDP header's structure
struct udpheader {
	 unsigned short int udph_srcport;
	 unsigned short int udph_destport;
	 unsigned short int udph_len;
	 unsigned short int udph_chksum;
};
// total udp header length: 8 bytes (=64 bits)

// Function for checksum calculation. From the RFC,
// the checksum algorithm is:
//  "The checksum field is the 16 bit one's complement of the one's
//  complement sum of all 16 bit words in the header.  For purposes of
//  computing the checksum, the value of the checksum field is zero."
/*unsigned short csum(unsigned short *buf, int nwords)
{       //
        unsigned long sum;
        for(sum=0; nwords>0; nwords--)
                sum += *buf++;
        sum = (sum >> 16) + (sum &0xffff);
        sum += (sum >> 16);
        return (unsigned short)(~sum);
}*/
/*
	Generic checksum calculation function
*/
unsigned short csum(unsigned short *ptr,int nbytes)
{
	register long sum;
	unsigned short oddbyte;
	register short answer;

	sum=0;
	while(nbytes>1) {
		sum+=*ptr++;
		nbytes-=2;
	}
	if(nbytes==1) {
		oddbyte=0;
		*((u_char*)&oddbyte)=*(u_char*)ptr;
		sum+=oddbyte;
	}

	sum = (sum>>16)+(sum & 0xffff);
	sum = sum + (sum>>16);
	answer=(short)~sum;

	return(answer);
}

// fetch local IP
char *getHostIPAddress() {
    int sock, index;
    struct ifreq ifreqs[20];
    struct ifconf ic;

    ic.ifc_len = sizeof ifreqs;
    ic.ifc_req = ifreqs;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Fail to open socket diagram");
        exit(1);
    }

    if (ioctl(sock, SIOCGIFCONF, &ic) < 0) {
        perror("SIOCGIFCONF");
        exit(1);
    }

    // get the last IP address from ifconfig
    index = ic.ifc_len/sizeof(struct ifreq) - 1;
    char * ip_address = inet_ntoa(((struct sockaddr_in*)&ifreqs[index].ifr_addr)->sin_addr);
    printf("Interface is %s\n", ifreqs[index].ifr_name);
    printf("Fetched ip is %s\n", ip_address);
    return ip_address;
}

// Source IP, source port, target IP, target port from the command line arguments
int main(int argc, char *argv[])
{
	int sd;
	// No data/payload just datagram
	char buffer[PCKT_LEN];
	// Our own headers' structures
	struct iphdr *ip = (struct iphdr *) buffer;
	struct udphdr *udp = (struct udphdr *) (buffer + sizeof(struct iphdr));

	// Source and destination addresses: IP and port
	struct sockaddr_in sin, din;
	int one = 1;
	const int *val = &one;

	memset(buffer, 0, PCKT_LEN);

	/*if(argc != 5)
	{
		printf("- Invalid parameters!!!\n");
		printf("- Usage %s <source hostname/IP> <source port> <target hostname/IP> <target port>\n", argv[0]);
		exit(-1);
	}*/

	// Create a raw socket with UDP protocol
	sd = socket(PF_INET, SOCK_RAW, IPPROTO_UDP);
	if(sd < 0)
	{
		perror("socket() error");
		// If something wrong just exit
		exit(-1);
	}
	else
		printf("socket() - Using SOCK_RAW socket and UDP protocol is OK.\n");

	// The source is redundant, may be used later if needed
	// The address family
	sin.sin_family = AF_INET;
	din.sin_family = AF_INET;
	// Port numbers
	sin.sin_port = htons(src_port);
	din.sin_port = htons(dst_port);
	// IP addresses
	src_IP = getHostIPAddress();
	sin.sin_addr.s_addr = inet_addr(src_IP);
	din.sin_addr.s_addr = inet_addr(dst_IP);

	// Fabricate the IP header or we can use the
	// standard header structures but assign our own values.
	ip->ihl = 5;
	ip->version = 4;
	ip->tos = 16; // Low delay
	ip->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr);
	ip->id = htons(rand()%RANDMAX);
	ip->ttl = 64; // hops
	ip->protocol = 17; // UDP
	// Source IP address, can use spoofed address here!!!
	ip->saddr = inet_addr(src_IP);
	// The destination IP address
	ip->daddr = inet_addr(dst_IP);
	ip->frag_off = 0;

	// Fabricate the UDP header. Source port number, redundant
	udp->source = htons(src_port);
	// Destination port number
	udp->dest = htons(dst_port);
	udp->len = htons(sizeof(struct udphdr));
	udp->check = csum((unsigned short *)(buffer + (sizeof(struct iphdr))), sizeof(struct udphdr));
	// Calculate the checksum for integrity
	ip->check = csum((unsigned short *)buffer, sizeof(struct iphdr) + sizeof(struct udphdr));
	// Inform the kernel do not fill up the packet structure. we will build our own...
	if(setsockopt(sd, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0)
	{
		perror("setsockopt() error");
		exit(-1);
	}
	else
		printf("setsockopt() is OK.\n");

	// Send loop, send for every 2 second for 100 count
	printf("Trying...\n");
	printf("Using raw socket and UDP protocol\n");
	printf("Using Source IP: %s port: %d, Target IP: %s port: %d.\n", src_IP, src_port, dst_IP, dst_port);

	int count;
	for(count = 1; count <=TTLMAXITER; count++)
	{
		if(sendto(sd, buffer, ip->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		// Verify
		{
			perror("sendto() error");
			exit(-1);
		}
		else
		{
			printf("Count #%u - sendto() is OK.\n", count);
			//sleep(1);
		}
	}
	close(sd);
	return 0;
}
