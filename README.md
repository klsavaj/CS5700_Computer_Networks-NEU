# Half-Duplex Chat System (Client-Server)

This project implements a **half-duplex chat** system using TCP sockets in C. The communication follows a turn-based model where only one side writes at a time while the other reads.

## Tasks Implemented

1. **Server waits for a client, client writes first.**  
   - The server listens on **port 9000** and waits for a client connection.  
   - Once the client connects, the client starts sending messages.

2. **Half-Duplex Chat Model (`x` to yield, `xx` to terminate).**  
   - A single **`x`** (on a line by itself) **yields writing control** to the other side.  
   - A single **`xx`** (on a line by itself) **closes the connection** and terminates the chat.

3. **Reorganized Loop Design to Implement Half-Duplex.**  
   - Each side alternates between **write phase** and **read phase** based on the chat protocol.

4. **Server Must Always Send a Response.**  
   - The server sends at least **one automatic reply** to every message received from the client.

5. **Error Handling.**  
   - If the server detects a **system call error** (`recv()`, `send()`, etc.), it prints the error locally and **sends an error message back** to the client.

6. **Error Message Format.**  
   - The server's error message includes the **source file name** (e.g., `unixserver.c`) and the **text from `strerror(errno)`**.

---

## How to Run the Code

This application runs on **port 9000**, as defined in `unixserver.c`. Below are the steps to **compile and execute** the programs.

### **Step 1: Open Terminal 1 (Start the Server)**

1. **Compile the server:**
   ```bash
   gcc -o server unixserver.c server.c
   ./server

### **Step 2: Open Terminal 2 (Start the Client)**

1. **Compile the client:**
   ```bash
   gcc -o client unixclient.c client.c
   ./client 9000

