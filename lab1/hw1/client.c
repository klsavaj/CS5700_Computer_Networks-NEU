#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "client.h"
#include <time.h>

extern char *inet_ntoa(struct in_addr);

#define NAMESIZE 255
#define BUFSIZE 81

void client(int server_number, char *server_node)
{
    int len;
    short fd;
    struct sockaddr_in address;
    struct hostent *node_ptr;
    char local_node[NAMESIZE];
    char buffer[BUFSIZE];

    // Get local host name
    if (gethostname(local_node, NAMESIZE) < 0) {
        perror("client gethostname");
        exit(1);
    }
    fprintf(stderr, "client running on node %s\n", local_node);

    // Set server_node to local node if not specified
    if (server_node == NULL)
        server_node = local_node;
    fprintf(stderr, "client connecting to server at port %d on node %s\n", server_number, server_node);

    // Resolve server address
    if ((node_ptr = gethostbyname(server_node)) == NULL) {
        perror("client gethostbyname");
        exit(1);
    }

    // Fill server address structure
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    memcpy(&address.sin_addr, node_ptr->h_addr, node_ptr->h_length);
    address.sin_port = htons(server_number);

    // Create TCP socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client socket");
        exit(1);
    }

    // Connect to the server
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("client connect");
        close(fd);
        exit(1);
    }

    // Display ASCII welcome banner
    printf("\n");
    printf("**********************************\n");
    printf("*    Welcome SK's Server         *\n");
    printf("**********************************\n");
    printf("\n");

    // Chat loop: Half-duplex communication
    while (1) {
        // Write phase
        while (1) {
            fprintf(stdout, "You> ");
            fflush(stdout);
            if (fgets(buffer, BUFSIZE, stdin) == NULL)
                strcpy(buffer, "xx\n");
            int len_to_send = strlen(buffer);
            if (send(fd, buffer, len_to_send, 0) < 0) {
                perror("client send");
                close(fd);
                exit(1);
            }
            if (strcmp(buffer, "xx\n") == 0) {
                fprintf(stderr, "You typed 'xx'. Closing connection.\n");
                close(fd);
                return;
            } else if (strcmp(buffer, "x\n") == 0) {
                break;
            }
        }
        // Read phase
        fprintf(stderr, "Waiting for server response...\n");
        while (1) {
            int n = recv(fd, buffer, BUFSIZE - 1, 0);
            if (n < 0) {
                perror("client recv");
                close(fd);
                exit(1);
            }
            if (n == 0) {
                fprintf(stderr, "Server disconnected.\n");
                close(fd);
                return;
            }
            buffer[n] = '\0';
            fprintf(stdout, "Server> %s", buffer);
            if (strcmp(buffer, "xx\n") == 0) {
                fprintf(stderr, "Server sent 'xx'. Closing connection.\n");
                close(fd);
                return;
            } else if (strcmp(buffer, "x\n") == 0) {
                break;
            }
        }
    }
    close(fd);
}
