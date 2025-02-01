#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>        // For strerror() and errno
#include "server.h"
#include <time.h>         /* ADDED FOR CHAT HISTORY LOGGING FEATURE */

extern char *inet_ntoa( struct in_addr );

#define NAMESIZE        255
#define BUFSIZE         81
#define LISTENING_DEPTH 2

/* ADDED FOR CHAT HISTORY LOGGING FEATURE */
static void log_message(const char *prefix, const char *msg)
{
    FILE *fp = fopen("chat_log.txt", "a");
    if (!fp)
        return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);
    fprintf(fp, "[%s] %s %s", timestr, prefix, msg);
    if (msg[strlen(msg)-1] != '\n')
        fprintf(fp, "\n");
    fclose(fp);
}

// ADDED FOR TASK #3 and #4: helper function to send error to client
static void send_server_error(int client_fd, const char *file_context)
{
    if (client_fd < 0) {
        // If we have no valid client yet, we can't send anything
        return;
    }
    // Build the error message with file name + strerror()
    char error_msg[BUFSIZE*2];
    snprintf(error_msg, sizeof(error_msg),
             "SERVER ERROR in %s: %s\n", file_context, strerror(errno));

    // Print locally on server syserr
    fprintf(stderr, "%s", error_msg);

    // Send back to client so they see it too
    // (client prints "Server> ..." anyway)
    send(client_fd, error_msg, strlen(error_msg), 0);
}

void server(int server_number)
{
    int                   fd, client_fd;
    struct sockaddr_in    address, client;
    struct hostent       *node_ptr;
    char                  local_node[NAMESIZE];
    char                  buffer[BUFSIZE+1];
    socklen_t             len;

    // (A) Get local host name
    if (gethostname(local_node, NAMESIZE) < 0) {
        perror("server gethostname");
        exit(1);
    }

    // (B) gethostbyname(local_node) => IP of this machine
    if ((node_ptr = gethostbyname(local_node)) == NULL) {
        perror("server gethostbyname");
        exit(1);
    }

    // (C) Fill server address
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    memcpy(&address.sin_addr, node_ptr->h_addr, node_ptr->h_length);
    address.sin_port = htons(server_number);

    // (D) Create socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server socket");
        exit(1);
    }

    // (E) Bind to (IP, port)
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("server bind");
        close(fd);
        exit(1);
    }

    // (F) Listen for incoming connections
    if (listen(fd, LISTENING_DEPTH) < 0) {
        perror("server listen");
        close(fd);
        exit(1);
    }

    // (G) Accept a client connection
    len = sizeof(client);
    if ((client_fd = accept(fd, (struct sockaddr *)&client, &len)) < 0) {
        perror("server accept");
        close(fd);
        exit(1);
    }

    // --- Chat Loop: Client writes first, so SERVER READS first ---
    while (1) {
        // ========== (1) SERVER READ PHASE ==========
        while (1) {
            int n = recv(client_fd, buffer, BUFSIZE, 0);
            if (n < 0) {
                send_server_error(client_fd, "server.c:recv()");
                perror("server recv");
                close(client_fd);
                close(fd);
                exit(1);
            }
            if (n == 0) {
                // Client closed the connection.
                fprintf(stderr, "Client disconnected.\n");
                close(client_fd);
                close(fd);
                return;
            }

            buffer[n] = '\0';
            fprintf(stdout, "Client> %s", buffer);
            log_message("Client>", buffer);

            if (strcmp(buffer, "xx\n") == 0) {
                // Client requests termination.
                close(client_fd);
                close(fd);
                return;
            }
            else if (strcmp(buffer, "x\n") == 0) {
                // Yield.
                break;
            }
            else {
                // Auto-reply to every normal message.
                char ack[BUFSIZE+1];
                snprintf(ack, sizeof(ack), "I got -> %s", buffer);

                int len_auto = strlen(ack);
                if (send(client_fd, ack, len_auto, 0) < 0) {
                    send_server_error(client_fd, "server.c:auto-reply send()");
                    perror("server send (auto-reply)");
                    close(client_fd);
                    close(fd);
                    exit(1);
                }
                log_message("Server auto-reply:", ack);
            }
        }

        // ========== (2) SERVER WRITE PHASE ==========
        while (1) {
            fprintf(stdout, "Server> ");
            fflush(stdout);
            if (fgets(buffer, BUFSIZE, stdin) == NULL) {
                strcpy(buffer, "xx\n");
            }
            log_message("Server>", buffer);

            int len_to_send = strlen(buffer);
            if (send(client_fd, buffer, len_to_send, 0) < 0) {
                send_server_error(client_fd, "server.c:send() in write phase");
                perror("server send");
                close(client_fd);
                close(fd);
                exit(1);
            }

            if (strcmp(buffer, "xx\n") == 0) {
                // Server requests termination.
                close(client_fd);
                close(fd);
                return;
            }
            else if (strcmp(buffer, "x\n") == 0) {
                // Yield.
                break;
            }
        }
    }

    // Close sockets if the loop ever exits.
    close(client_fd);
    close(fd);
}
