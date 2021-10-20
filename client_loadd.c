/**
 * @file client_loadd.c
 * @author crackcat
 * @date 20 Oct 2021
 * @brief A simple load deamon client.
 *
 * This client requests the current load status of a given server IP
 * via UDP and prints both the average load and amount of currently
 * logged in users to the console.
 *
 * DISCLAIMER: Do not use in production. This is purely for educational purposes.
 *
 * COMPILING: gcc -o client -Wall -Wpedantic -Wextra client_loadd.c
 */


#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* Server UDP port */
#define SERVER_PORT 4701

/* Structure for the load statistics */
struct rload_stat {
    double load; // Average CPU load over the last minute
    int users;   // Currently logged in users (via ssh)
};

static char *banner = "\n\
  _              _      _ \n\
 | |___  __ _ __| |  __| |__ _ ___ _ __  ___ _ _ \n\
 | / _ \\/ _` / _` | / _` / _` / -_) '  \\/ _ \\ ' \\ \n\
 |_\\___/\\__,_\\__,_| \\__,_\\__,_\\___|_|_|_\\___/_||_|\n\
              _           _  \n\
           __| (_)___ _ _| |_ \n\
          / _| | / -_) ' \\  _|\n\
          \\__|_|_\\___|_||_\\__|       @crackcat\n\n\
";

int main(int argc, char *argv[]) {

	fprintf(stdout, "%s", banner);

	if (argc!=2) {
		printf("Usage: %s <IP of the loaddaemon server>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Necessary variables for socket communication */
	struct sockaddr_in serveraddr; // address structure for target
	struct sockaddr_in remoteaddr; // address structure for responses
	unsigned int remoteaddrLen;
	char* serverIP = argv[1];      // ascii address of the server
	int sendLen;
	int recvLen;

	/* Zero out and prepare the target structure */
	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET; // set INET address family

	/* Convert ascii IP address into binary form (in network byte order) */
	if (!inet_aton(serverIP, &serveraddr.sin_addr)) {
		printf("Invalid IP address\n");
		exit(EXIT_FAILURE);
	}

	serveraddr.sin_port = htons(SERVER_PORT); // convert and set the PORT value in network byte order


	/* Create and zero out a load stat sructure */
	struct rload_stat stats;
	memset(&stats, 0, sizeof(stats));

	/* Create a UDP socket for the client */
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
		perror("socket() failed");
		exit(EXIT_FAILURE);
	}

	/* Set a receive timeout for the UDP socket (we don't want to wait forever) */
	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 00000;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    		perror("setsockopt() failed to set RECV timeout");
		exit(EXIT_FAILURE);
	}

	/* Send the empty load stat structure to the server */
	sendLen = sendto(sockfd, (struct rload_stat*)&stats, sizeof(stats), 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
	if (sendLen != sizeof(stats)) {
		perror("sendto() failed to sent everything correctly");
		exit(EXIT_FAILURE);
	}

	/* Wait for the server to respond */
	remoteaddrLen = sizeof(remoteaddr);
	if ( (recvLen = recvfrom(sockfd, &stats, sizeof(stats), 0, (struct sockaddr *) &remoteaddr, &remoteaddrLen)) < 0) {
		perror("rcvfrom() failed");
		exit(EXIT_FAILURE);
	/* Drop and report response if the size does not match the load structure */
        } else if (recvLen != sendLen) {
		printf("Expected %d bytes but received %d from: %s\n", sendLen, recvLen, inet_ntoa(remoteaddr.sin_addr));
		exit(EXIT_FAILURE);
	}

	/* Print the received values */
	printf("[+] Load stats (%s)\n", inet_ntoa(remoteaddr.sin_addr));
	printf("\tAvg. CPU load (1min): %lf\n", stats.load);
	printf("\tLogged in users: %d\n", stats.users);

	/* Close the socket */
	close(sockfd);
	return EXIT_SUCCESS;
}
