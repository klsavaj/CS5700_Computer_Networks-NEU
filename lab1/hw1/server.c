#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include "server.h"
#include <time.h>

extern char *inet_ntoa(struct in_addr);

#define NAMESIZE 255
#define BUFSIZE 81
#define LISTENING_DEPTH 2

// Log a message with a timestamp to chat_log.txt
static void log_message(const char *prefix, const char *msg) {
    FILE *fp = fopen("chat_log.txt", "a");
    if (!fp) return;
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestr[64];
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S", t);
    fprintf(fp, "[%s] %s %s", timestr, prefix, msg);
    if (msg[strlen(msg)-1] != '\n')
        fprintf(fp, "\n");
    fclose(fp);
}

// Send an error message to the client and print it locally
static void send_server_error(int client_fd, const char *file_context) {
    if (client_fd < 0) return;
    char error_msg[BUFSIZE*2];
    snprintf(error_msg, sizeof(error_msg), "SERVER ERROR in %s: %s\n", file_context, strerror(errno));
    fprintf(stderr, "%s", error_msg);
    send(client_fd, error_msg, strlen(error_msg), 0);
}

void server(int server_number) {
    int fd, client_fd;
    struct sockaddr_in address, client;
    struct hostent *node_ptr;
    char local_node[NAMESIZE];
    char buffer[BUFSIZE+1];
    socklen_t len;

    // Get local host name
    if (gethostname(local_node, NAMESIZE) < 0) {
        perror("server gethostname");
        exit(1);
    }
    // Get local IP address
    if ((node_ptr = gethostbyname(local_node)) == NULL) {
        perror("server gethostbyname");
        exit(1);
    }
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    memcpy(&address.sin_addr, node_ptr->h_addr, node_ptr->h_length);
    address.sin_port = htons(server_number);
    // Create socket
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("server socket");
        exit(1);
    }
    // Bind socket
    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("server bind");
        close(fd);
        exit(1);
    }
    // Listen for connections
    if (listen(fd, LISTENING_DEPTH) < 0) {
        perror("server listen");
        close(fd);
        exit(1);
    }
    // Accept a connection
    len = sizeof(client);
    if ((client_fd = accept(fd, (struct sockaddr *)&client, &len)) < 0) {
        perror("server accept");
        close(fd);
        exit(1);
    }

    srand(time(NULL)); // Seed random generator for /roll

    // Chat loop: Server reads, then writes
    while (1) {
        // Read phase
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
                fprintf(stderr, "Client disconnected.\n");
                close(client_fd);
                close(fd);
                return;
            }
            buffer[n] = '\0';
            fprintf(stdout, "Client> %s", buffer);
            log_message("Client>", buffer);
            if (strcmp(buffer, "xx\n") == 0) {
                close(client_fd);
                close(fd);
                return;
            } else if (strcmp(buffer, "x\n") == 0) {
                break;
            } else {
                char ack[BUFSIZE+1];
                if (buffer[0] == '/') {
                    if (strncmp(buffer, "/time", 5) == 0) {
                        time_t now = time(NULL);
                        struct tm *local = localtime(&now);
                        char timeStr[64];
                        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", local);
                        snprintf(ack, sizeof(ack), "Current time: %s\n", timeStr);
                    } else if (strncmp(buffer, "/roll", 5) == 0) {
                        int roll = (rand() % 6) + 1;
                        snprintf(ack, sizeof(ack), "You rolled: %d\n", roll);
                    } else if (strncmp(buffer, "/quote", 6) == 0) {
                        const char *quotes[] = {
                            "\"The only way to do great work is to love what you do.\" – Steve Jobs",
                            "\"Life is what happens when you're busy making other plans.\" – John Lennon",
                            "\"The journey of a thousand miles begins with one step.\" – Lao Tzu",
                            "\"You miss 100%% of the shots you don't take.\" – Wayne Gretzky"
                        };
                        int numQuotes = sizeof(quotes) / sizeof(quotes[0]);
                        int index = rand() % numQuotes;
                        snprintf(ack, sizeof(ack), "Quote: %s\n", quotes[index]);
                    } else {
                        snprintf(ack, sizeof(ack), "Unknown command. Available commands: /time, /roll, /quote\n");
                    }
                } else {
                    snprintf(ack, sizeof(ack), "I got -> %s", buffer);
                }
                int len_auto = strlen(ack);
                if (send(client_fd, ack, len_auto, 0) < 0) {
                    send_server_error(client_fd, "server.c:send() (command/auto-reply)");
                    perror("server send (command/auto-reply)");
                    close(client_fd);
                    close(fd);
                    exit(1);
                }
                log_message("Server auto-reply:", ack);
            }
        }
        // Write phase
        while (1) {
            fprintf(stdout, "Server> ");
            fflush(stdout);
            if (fgets(buffer, BUFSIZE, stdin) == NULL)
                strcpy(buffer, "xx\n");
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
                close(client_fd);
                close(fd);
                return;
            } else if (strcmp(buffer, "x\n") == 0) {
                break;
            }
        }
    }
    close(client_fd);
    close(fd);
}
