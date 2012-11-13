/****************setOptionTTL.c*****************/
/**
 * @author: Haokun Luo
 *
 * This is a middle box probing UDP program, which send
 * series of UDP packets to a server with increasing TTL.
 * Based on tcpdump result on the server side,
 * you can observe the expected dropped TTL value from
 * the IP header. If the TTL increase or not constantly
 * decrease, then it is highly likely that there is a middle
 * box in between the client and server.
 */
/***********************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Host name of my system, change accordingly */
/* Put the server hostname that run the UDP server program */
/* This will be used as default UDP server for client connection */
#define SERVER "141.212.108.135" 	// owl.eecs.umich.edu
/* Server's port number */
#define SERVPORT 10100
/* Pass in 1 argument (argv[1]) which is either the */
/* address or host name of the server, or */
/* set the server name in the #define SERVER above. */
#define TTL_MAX 30

// helper functions
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

int main(int argc, char *argv[])
{
	/* Variable and structure definitions. */
	int sd, rc;
	struct sockaddr_in serveraddr;
	// int serveraddrlen = sizeof(serveraddr);
	char server[255];
	char buffer[100];
	int ttl_value = TTL_MAX;
	char payload[2];
	// struct hostent *hostp;
	// memset(buffer, 0x00, sizeof(buffer));
	/* 36 characters + terminating NULL */
	// memcpy(buffer, "Hello! A client request message lol!", 37);

	/* The socket() function returns a socket */
	/* descriptor representing an endpoint. */
	/* The statement also identifies that the */
	/* INET (Internet Protocol) address family */
	/* with the UDP transport (SOCK_DGRAM) will */
	/* be used for this socket. */
	/******************************************/
	/* get a socket descriptor */
	if((sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("UDP Client - socket() error");
		/* Just exit lol! */
		exit(-1);
	}
	else
		printf("UDP Client - socket() is OK!\n");

	/* If the hostname/IP of the server is supplied */
	/* Or if(argc = 2) */
	if(argc > 1) {
		strcpy(server, argv[1]);
		ttl_value = atoi(argv[2]);
		printf("MAX TTL is reset as %d\n", ttl_value);
	}
	else
	{
		/*Use default hostname or IP*/
		printf("UDP Client - Usage %s <Server IP>\n", argv[0]);
		printf("UDP Client - Using default hostname/IP!\n");
		strcpy(server, SERVER);
	}

	memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERVPORT);
	serveraddr.sin_addr.s_addr = inet_addr(server);

	// increase the TTL value and send it
	while (ttl_value > 0) {
		if (setsockopt(sd, IPPROTO_IP, IP_TTL, &ttl_value, sizeof(ttl_value) ) < 0)
		{
			perror("setsockopt() error");
			exit(-1);
		}
		else
			printf("setsockopt() is OK with TTL %d.\n", ttl_value);

		// set the TTL into the payload
		itoa(ttl_value, payload);

		/* Use the sendto() function to send the data */
		/* to the server. */
		/************************************************/
		/* This example does not use flags that control */
		/* the transmission of the data. */
		/************************************************/
		/* send request to server */
		rc = sendto(sd, payload, sizeof(payload), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if(rc < 0)
		{
			perror("UDP Client - sendto() error");
			close(sd);
			exit(-1);
		}
		else
			printf("UDP Client - sendto() is OK!\n");

		ttl_value--;
		sleep(2);
	}

	// printf("Waiting a reply from UDP server...\n");

	/* Use the recvfrom() function to receive the */
	/* data back from the server. */
	/************************************************/
	/* This example does not use flags that control */
	/* the reception of the data. */
	/************************************************/
	/* Read server reply. */
	/* Note: serveraddr is reset on the recvfrom() function. */
	/*rc = recvfrom(sd, bufptr, buflen, 0, (struct sockaddr *)&serveraddr, &serveraddrlen);

	if(rc < 0)
	{
	perror("UDP Client - recvfrom() error");
	close(sd);
	exit(-1);
	}
	else
	{
	printf("UDP client received the following: \"%s\" message\n", bufptr);
	printf(" from port %d, address %s\n", ntohs(serveraddr.sin_port), inet_ntoa(serveraddr.sin_addr));
	}*/

	/* When the data has been received, close() */
	/* the socket descriptor. */
	/********************************************/
	/* close() the socket descriptor. */
	close(sd);
	exit(0);
}
