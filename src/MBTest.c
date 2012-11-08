/*
 ============================================================================
 Name        : MBTest.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>	//for printf
#include <string.h> //memset
#include <sys/socket.h>	//for socket ofcourse
#include <stdlib.h> //for exit(0);
#include <errno.h> //For errno - the error number
#include <netinet/tcp.h>	//Provides declarations for tcp header
#include <netinet/ip.h>	//Provides declarations for ip header
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <arpa/inet.h>

// constant and variables
#define MAX_TTL 30
int src_port = 10100;
int dst_port = 10100;
int max_win  = 50000; // tcp maximum window size
char *dst_IP = "141.212.108.135"; // set the destination IP address (owl.eecs.umich.edu)

/* reverse:  reverse string s in place */
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* record sign */
        n = -n;          /* make n positive */
    i = 0;
    do {       /* generate digits in reverse order */
        s[i++] = n % 10 + '0';   /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
}

/*
	96 bit (12 bytes) pseudo header needed for tcp header checksum calculation
*/
struct pseudo_header
{
	u_int32_t source_address;
	u_int32_t dest_address;
	u_int8_t placeholder;
	u_int8_t protocol;
	u_int16_t tcp_length;
};

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

/*
 * Fetch the current IP address
 */
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

int main (void)
{
	//Create a raw socket
	int s = socket (PF_INET, SOCK_RAW, IPPROTO_TCP);

	if(s == -1)
	{
		//socket creation failed, may be because of non-root privileges
		perror("Failed to create socket");
		exit(1);
	}

	//Datagram to represent the packet
	char datagram[4096] , source_ip[32] , *data , *pseudogram;

	//zero out the packet buffer
	memset (datagram, 0, 4096);

	//IP header
	struct iphdr *iph = (struct iphdr *) datagram;

	//TCP header
	struct tcphdr *tcph = (struct tcphdr *) (datagram + sizeof (struct ip));
	struct sockaddr_in sin;
	struct pseudo_header psh;

	//Data part
	data = datagram + sizeof(struct iphdr) + sizeof(struct tcphdr);

	//some address resolution
	strcpy(source_ip , getHostIPAddress());
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);
	sin.sin_addr.s_addr = inet_addr(dst_IP);

	//Fill in the IP Header
	iph->ihl = 5;
	iph->version = 4;
	iph->tos = 0;
	iph->tot_len = sizeof (struct iphdr) + sizeof (struct tcphdr) + strlen(data);
	iph->id = htonl (54321);	//Id of this packet
	iph->frag_off = 0;
	iph->ttl = 1;
	iph->protocol = IPPROTO_TCP;
	iph->check = 0;		//Set to 0 before calculating checksum
	iph->saddr = inet_addr ( source_ip );	// Spoof the source ip address
	iph->daddr = sin.sin_addr.s_addr;

	//TCP Header
	tcph->source = htons (src_port);
	tcph->dest = htons (dst_port);
	tcph->seq = 0;
	tcph->ack_seq = 0;
	tcph->doff = 5;	//tcp header size
	tcph->fin=0;
	tcph->syn=1;
	tcph->rst=0;
	tcph->psh=1;	// send no delay
	tcph->ack=0;
	tcph->urg=0;
	tcph->window = htons (max_win);	/* maximum allowed window size */
	tcph->check = 0;	//leave checksum 0 now, filled later by pseudo header
	tcph->urg_ptr = 0;

	//Now the TCP checksum
	psh.source_address = inet_addr( source_ip );
	psh.dest_address = sin.sin_addr.s_addr;
	psh.placeholder = 0;
	psh.protocol = IPPROTO_TCP;

	//IP_HDRINCL to tell the kernel that headers are included in the packet
	int one = 1;
	const int *val = &one;

	if (setsockopt (s, IPPROTO_IP, IP_HDRINCL, val, sizeof (one)) < 0)
	{
		perror("Error setting IP_HDRINCL");
		exit(0);
	}

	// send TTL from 1 to 64
	int i;
	char payload[2];
	for (i = 1; i <= MAX_TTL; i++) {
		// reset the TTL
		iph->ttl = i;
		// store the data the same as TTL value

		itoa(i, payload);
		strcpy(data , payload);

		// reassign the payload length
		iph->tot_len = sizeof (struct iphdr) + sizeof (struct tcphdr) + strlen(data);

		//Ip checksum
	    iph->check = csum ((unsigned short *) datagram, iph->tot_len);

		// TCP checksum
		psh.tcp_length = htons(sizeof(struct tcphdr) + strlen(data) );

		int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + strlen(data);
		pseudogram = malloc(psize);

		memcpy(pseudogram , (char*) &psh , sizeof (struct pseudo_header));
		memcpy(pseudogram + sizeof(struct pseudo_header) , tcph , sizeof(struct tcphdr) + strlen(data));

		tcph->check = csum( (unsigned short*) pseudogram , psize);

		//Send the packet
		if (sendto (s, datagram, iph->tot_len ,	0, (struct sockaddr *) &sin, sizeof (sin)) < 0)
		{
			perror("sendto failed");
		}
		//Data send successfully
		else
		{
			printf ("Packet %d Send. Length : %d ; TTL: %d\n" , i, iph->tot_len, iph->ttl);
		}
	}

	return 0;
}


