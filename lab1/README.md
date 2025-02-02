# Half-Duplex Chat System (Client-Server)

This project implements a **half-duplex chat** system using TCP sockets in C. The communication follows a turn-based model where only one side writes at a time while the other reads.

---

## Tasks Implemented

1. **Server waits for a client, client writes first.**  
   - The server listens on **port 9000** and waits for a client connection.  
   - Once the client connects, the client starts sending messages.
     
2. **Half-Duplex Chat Model (`x` to yield, `xx` to terminate).**  
   - A single **`x`** (on a line by itself) **yields writing control** to the other side.  
   - A single **`xx`** (on a line by itself) **closes the connection** and terminates the chat.
   - Code snippet Server side:
     ```c
     buffer[n] = '\0';
            fprintf(stdout, "Client> %s", buffer);
            log_message("Client>", buffer);
            if (strcmp(buffer, "xx\n") == 0) {
                close(client_fd);
                close(fd);
                return;
            } else if (strcmp(buffer, "x\n") == 0) {
                break;
     ```
   - Code snippet Client side:
     ```c
     if (strcmp(buffer, "xx\n") == 0) {
                fprintf(stderr, "You typed 'xx'. Closing connection.\n");
                close(fd);
                return;
            } else if (strcmp(buffer, "x\n") == 0) {
                break;
            }
     ```
     
     

3. **Reorganized Loop Design to Implement Half-Duplex.**  
   - Each side alternates between **write phase** and **read phase** based on the chat protocol.

4. **Server Must Always Send a Response.**  
   - The server sends at least **one automatic reply** to every message received from the client.
   - Code snippet
     ```c
     else {
       snprintf(ack, sizeof(ack), "I got -> %s", buffer);
       if (send(client_fd, ack, strlen(ack), 0) < 0) {
        perror("server send (auto-reply)");
       }
      }
     ```

5. **Error Handling.**  
   - If the server detects a **system call error** (`recv()`, `send()`, etc.), it prints the error locally and **sends an error message back** to the client.
   - Code snippet
     ```c
        static void send_server_error(int client_fd, const char *file_context) {
           if (client_fd < 0) return;
           char error_msg[BUFSIZE*2];
           snprintf(error_msg, sizeof(error_msg), "SERVER ERROR in %s: %s\n", file_context, strerror(errno));
           fprintf(stderr, "%s", error_msg);
           send(client_fd, error_msg, strlen(error_msg), 0);
         }
     ```

6. **Error Message Format.**  
   - The server's error message includes the **source file name** (e.g., `unixserver.c`) and the **text from `strerror(errno)`**.

---

## Additional Task Implemented 

1. **Server Commands (/time, /roll, /quote)**
   - Goal:  When a client sends a message starting with /, treat it as a command. The server checks if it’s /time, /roll, or /quote, and responds accordingly.
   - How It Works:
      - /time: Server sends back the current date and time.
      - /roll: Server generates a random number 1–6 and returns it.
      - /quote: Server chooses a random quote from the array and returns it.
If the command is unrecognized, the server replies with available commands.
   - Code snippet
   
   ```c
   if (buffer[0] == '/') {
    if (strncmp(buffer, "/time", 5) == 0) {
        // Return current date/time
        time_t now = time(NULL);
        struct tm *local = localtime(&now);
        char timeStr[64];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", local);
        snprintf(ack, sizeof(ack), "Current time: %s\n", timeStr);
    } else if (strncmp(buffer, "/roll", 5) == 0) {
        // Roll a six-sided die
        int roll = (rand() % 6) + 1;
        snprintf(ack, sizeof(ack), "You rolled: %d\n", roll);
    } else if (strncmp(buffer, "/quote", 6) == 0) {
        // Return random quote
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
   }

   ```

2. **ASCII Welcome Banner (Client Side)**
   - Goal:  As soon as the client connects to the server, display a welcome message or ASCII art in the client’s terminal.
   - What Happens: When the client successfully connects to the server,the console prints:
      - '**********************************'
      - '*     Welcome SK's Server        *'
      - '**********************************'

   - Code Snippet:
      ```c
     // Display ASCII welcome banner
       printf("\n");
       printf("**********************************\n");
       printf("*    Welcome SK's Server         *\n");
       printf("**********************************\n");
       printf("\n");
      ```

4. **Chat History Logging**
   - Goal: Keep a persistent record (chat_log.txt) of each message (both from the client and from the server). The server handles all logging in append mode, so the conversation is saved even across multiple sessions.
   - How It Works:
      - Every time the server receives or sends a message, it calls log_message("Client>", msg) or log_message("Server>", msg).
      - The function opens chat_log.txt in append mode and writes a timestamp, a prefix (e.g., Client>), and the actual message.

   - Code Snippet:
     ```c
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
     ```

---

## How to Run the Code

This application runs on **port 9000**, as defined in `unixserver.c`. Below are the steps to **compile and execute** the programs.

### **Step 1: Open Terminal 1 (Start the Server)**

1. **server:**
   ```bash
   cd /(GO TO DIRECTORY WHERE hw1 IS PRESENT)/hw1
   //compile server file
   gcc -o server unixserver.c server.c
   
   //run file
   ./server

### **Step 2: Open Terminal 2 (Start the Client)**

1. **client:**
   ```bash
   cd /(GO TO DIRECTORY WHERE hw1 IS PRESENT)/hw1
   gcc -o client unixclient.c client.c
   
   //Mentioned the port number and run file 
   ./client 9000

---
## Example Output

Here’s a simple illustration of how the chat might look. Let’s assume the client types some messages and commands:

1. **Client Side:**
   ```bash
   **********************************
   *    Welcome SK's Server         *
   **********************************
   You> /time
   Waiting for server response...
   Server> Current time: 2025-02-01 13:56:07
   You> Hello server
   Waiting for server response...
   Server> I got -> Hello server
   You> /roll
   Waiting for server response...
   Server> You rolled: 3
   You> x
   Waiting for server response...
   Server> <The server now starts writing...>
   ...
   ```
3. **Server Side:**
   ```bash
   Server> 
   Client> /time
   Server auto-reply: Current time: 2025-02-01 13:56:07
   Client> Hello server
   Server auto-reply: I got -> Hello server
   Client> /roll
   Server auto-reply: You rolled: 3
   Client> x
   Server> ...
   ```
5. **And in the file chat_log.txt on the server side, you might see entries like:**
   ```txt
   [2025-02-01 13:56:07] Client> /time
   [2025-02-01 13:56:07] Server auto-reply: Current time: 2025-02-01 13:56:07
   [2025-02-01 13:56:10] Client> Hello server
   [2025-02-01 13:56:10] Server auto-reply: I got -> Hello server
   [2025-02-01 13:56:13] Client> /roll
   [2025-02-01 13:56:13] Server auto-reply: You rolled: 3
   [2025-02-01 13:56:16] Client> x
   [2025-02-01 13:56:16] Server> ...
   ```
