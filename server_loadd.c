/**
 * @file server_loadd.c
 * @author crackcat
 * @date 20 Oct 2021
 * @brief A simple load deamon server.
 *
 * This server accepts UDP requests with a struct from load deamon
 * clients, fills them with current information about the CPU load
 * average and the number of logged in users and sends the structure
 * back to the client.
 *
 * DISCLAIMER: Do not use in production. This is purely for educational purposes.
 *
 * COMPILING: gcc -o server -Wall -Wpedantic -Wextra server_loadd.c
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
#define PORT 4701

/* Just for lulz */
#define EVER ;;


/* Structure for the load statistics */
struct rload_stat {
	double load; // the average CPU load over the last minute
	int users; // the amount of currently logged in users
};


/**
 * Get the CPU load
 *
 * @return The average CPU load over the last minute as double
 */
double cpu_load(void) {
        FILE *fptr;
        fptr = fopen("/proc/loadavg", "r");

        if (fptr == NULL) {
                perror("Read-operation failed");
                return -1;
        }

	double load;
        fscanf(fptr, "%lf", &load);
        fclose(fptr);

        return load;
}

/**
 * Get the amount of currently logged in users
 *
 * @return The amount of users as integer
 */
int logged_in_users(void) {

	FILE *fptr;
	/* Execute "who" and count unique usernames */
	fptr = popen("/usr/bin/who | /usr/bin/tr -s ' ' | /usr/bin/cut -d ' ' -f 1 | /usr/bin/sort -u | /usr/bin/wc -l", "r");

	if (fptr == NULL) {
		perror("Failed to execute user count");
		return -1;
	}

	int n_users;
	fscanf(fptr, "%d", &n_users);

	fclose(fptr);
	return n_users;
}


/**
 * Fill a load stats structure with updated values
 *
 * @param stats Pointer to an rload_stat structure
 */
void updateLoadStats(struct rload_stat *stats) {
	stats->users = logged_in_users();
	stats->load = cpu_load();
}


static char* banner = "\n\
  _              _      _\n\
 | |___  __ _ __| |  __| |__ _ ___ _ __  ___ _ _\n\
 | / _ \\/ _` / _` | / _` / _` / -_) '  \\/ _ \\ ' \\\n\
 |_\\___/\\__,_\\__,_| \\__,_\\__,_\\___|_|_|_\\___/_||_|\n\
             ___ ___ _ ___ _____ _ _\n\
            (_-</ -_) '_\\ V / -_) '_|\n\
            /__/\\___|_|  \\_/\\___|_|     @crackcat\n\n\
";

int main(void) {

	printf(banner);

	/* Variables for socket communication */
	struct sockaddr_in servaddr; // server address structure
	struct sockaddr_in clientaddr; // client address structure
	unsigned int clientAddrLen;
	int recvMsgSize;

	/* Create a UDP socket */
	int sockfd;
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 ) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* Zero out and prepare the address structures */
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&clientaddr, 0, sizeof(clientaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY; // no need for htonl becuz its 0 anyway (0.0.0.0)
	servaddr.sin_port = htons(PORT);
	clientAddrLen = sizeof(clientaddr);

	/* Bind the socket to 0.0.0.0 (all interfaces) */
	if ( bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ) {
		perror("Binding to socket failed");
		exit(EXIT_FAILURE);
	}

	/* Allocate space for a load stats structure */
	struct rload_stat *pStats = malloc(sizeof(struct rload_stat));

	/* Fill all incoming structures with new values and send them back */
	for(EVER) {

		/* Receive incoming values into a load stats structure */
		/* For this app it isn't even necessary, but I like it */
		if ( (recvMsgSize = recvfrom(sockfd, pStats, sizeof(*pStats), 0, (struct sockaddr *) &clientaddr, &clientAddrLen)) < 0) {
			perror("rcvfrom() failed");
			exit(EXIT_FAILURE);
		}

		/* Some pretty debug information */
		printf("[+] Handling incoming request (%s)\n", inet_ntoa(clientaddr.sin_addr));
		printf("\tReceived bytes: %d\n", recvMsgSize);

		/* Fill the struct with current values */
		updateLoadStats(pStats);

		/* More pretty output */
		printf("\tSending (load): %lf\n", pStats->load);
		printf("\tSending (users): %d\n", pStats->users);

		/* Send back the updated structure */
		int sent;
		if ( (sent = sendto(sockfd, (struct rload_stat *)pStats, sizeof(*pStats), 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr))) < 0) {
			perror("sento() failed");
			exit(EXIT_FAILURE);
		}

		/* Even more output */
		printf("\tSent bytes: %d\n", sent);
	}

	/* Close the socket - this will never be reached tough. */
	close(sockfd);
	return EXIT_SUCCESS;
}
