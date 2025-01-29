#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include "client.h"

extern char *inet_ntoa( struct in_addr );

#define NAMESIZE		255
#define BUFSIZE			81

void client(int server_number, char *server_node)
{
    int                   len;
    short                 fd;
    struct sockaddr_in    address;
    struct hostent       *node_ptr;
    char                  local_node[NAMESIZE];
    char                  buffer[BUFSIZE];

    // (A) Get local host name
    if (gethostname(local_node, NAMESIZE) < 0) {
        perror("client gethostname");
        exit(1);
    }
    fprintf(stderr, "client running on node %s\n", local_node);

    // (B) If no server node specified, default to local node
    if (server_node == NULL)
        server_node = local_node;
    fprintf(stderr, "client about to connect to server at port number %d on node %s\n",
            server_number, server_node);

    // (C) Resolve server_node to an IP address
    if ((node_ptr = gethostbyname(server_node)) == NULL) {
        perror("client gethostbyname");
        exit(1);
    }

    // (D) Fill 'address' with server info: IP and port
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    memcpy(&address.sin_addr, node_ptr->h_addr, node_ptr->h_length);
    address.sin_port = htons(server_number);

    // (E) Create a TCP socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("client socket");
        exit(1);
    }

    // (F) Connect the socket to the server's address
    if (connect(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("client connect");
        close(fd);
        exit(1);
    }

    // ---- (G) Now the chat loop: half-duplex ----
    while (1) {
        // ========== (1) CLIENT WRITES PHASE ==========
        while (1) {
            fprintf(stdout, "You> ");
            fflush(stdout);

            // Read from stdin
            if (fgets(buffer, BUFSIZE, stdin) == NULL) {
                // EOF => treat like "xx"
                strcpy(buffer, "xx\n");
            }

            // Send to server
            int len_to_send = strlen(buffer);
            if (send(fd, buffer, len_to_send, 0) < 0) {
                perror("client send");
                close(fd);
                exit(1);
            }

            // Check special commands
            if (strcmp(buffer, "xx\n") == 0) {
                fprintf(stderr, "You typed 'xx'. Closing connection.\n");
                close(fd);
                return;
            }
            else if (strcmp(buffer, "x\n") == 0) {
                // yield
                break;
            }
        }

        // ========== (2) CLIENT READS PHASE ==========
        fprintf(stderr, "Waiting for server response...\n");
        while (1) {
            int n = recv(fd, buffer, BUFSIZE-1, 0);
            if (n < 0) {
                perror("client recv");
                close(fd);
                exit(1);
            }
            if (n == 0) {
                // server closed
                fprintf(stderr, "Server disconnected.\n");
                close(fd);
                return;
            }

            buffer[n] = '\0';  // make it a string
            fprintf(stdout, "Server> %s", buffer);

            if (strcmp(buffer, "xx\n") == 0) {
                fprintf(stderr, "Server sent 'xx'. Closing connection.\n");
                close(fd);
                return;
            }
            else if (strcmp(buffer, "x\n") == 0) {
                // server yielded
                break;
            }
        }
    }

    // (H) We normally never get here, but just in case:
    close(fd);
}
