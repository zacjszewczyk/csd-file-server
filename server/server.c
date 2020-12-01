// Includes
#include <netinet/in.h> // Build socket
#include <stdio.h> // Write to console
#include <unistd.h> // Read data from socket
#include <stdlib.h> // Exit on error
#include <string.h> // Compare strings
#include <sys/stat.h> // Validate files and directories

// Main function
int main(int argc, char const *argv[]) 
{ 
	char help[] = "usage: server -p SERVER_PORT -d TARGET_DIRECTORY\n  -p  The port on which to make the file server available.\n  -d  The directory in which to store files, and from which to serve them.\n  -h  Display this help menu.\nNote: The order of the parameters is unimportant.";

	// Perform basic input validation.
	// Validate that the user entered two command-line arguments
	if (argc == 1 || (strstr(argv[1], "-h") || strstr(argv[1], "--help"))) {
		printf("%s\n",help);
		exit(0);
	} else if (argc != 5) {
		printf("Error: Server port and target directory not specified. Please try again.\n\n");
		printf("%s\n",help);
		exit(1);
	}

	// Extract and validate server port
	int port_location = 0;
	if (strstr(argv[1],"-p")) {
		port_location = 2;
	} else if (strstr(argv[3],"-p")) {
		port_location = 4;
	} else {
		printf("Error: Server port not specified. Please try again.\n");
		exit(1);
	}
	int const server_port = atoi(argv[port_location]);
	if (server_port < 0 || server_port > 65535) {
		printf("Error: Invalid server port. Please try again.\n");
		exit(1);	
	}

	// Extract and validate target directory
	int dir_location = 0;
	if (strstr(argv[1],"-d")) {
		dir_location = 2;
		printf("Directory first\n");
	} else if (strstr(argv[3],"-d")) {
		dir_location = 4;
	} else {
		printf("Error: Target directory not specified. Please try again.\n");
		exit(1);
	}
	const char* dir = argv[dir_location];
	struct stat st;
	if (stat(dir, &st) != 0 || S_ISDIR(st.st_mode) == 0) {
        printf("Error: Target directory does not exist. Please try again.\n");
		exit(1);
    }

	// Initialize the sockets
	int server_sock; // Server socket
	int client_sock; // Client socket
	// Attempt to create the server socket. Print an error on failure and halt
	// execution.
	if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
		printf("Error: Socket creation failed."); 
		exit(1); 
	}
	
	// Construct the server address object to be bound to the server socket.
	struct sockaddr_in address; // 
	address.sin_addr.s_addr = INADDR_ANY; // Accept any source IP addresses.
	address.sin_family = AF_INET; // Required.
	address.sin_port = htons(server_port); // Set server port.
	int addrlen = sizeof(address); // Size of the address object for binding.
	int opt = 1; // setsockopt() optval field.
	
	// Configure the socket options. Print an error on failure and halt
	// execution.
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { 
		printf("Error: Socket configuration failed."); 
		exit(1); 
	} 

	// Bind the socket. Print an error on failure and halt execution.
	if (bind(server_sock, (struct sockaddr *)&address, sizeof(address)) < 0) { 
		printf("Error: Socket binding failed."); 
		exit(1);
	} 

	// Start listening for connections on the server socket.
	if (listen(server_sock, 3) < 0) {
		printf("Error: Failed to listen on server port %d\n",server_port);
		exit(1); 
	}

	// Print a status message to the user, so that they know that the server
	// is now listening for connections.
	printf("Listening on port %d.\n",server_port);

	// Initialize a 1kb "buff" variable to store chunks of data during
	// transmission, a "bytes_transferred" variable to track those transfers,
	// and an "fd" file descriptor variable for sending and receiving files.
	char buff[1024] = {0}; 
	int bytes_transferred = 0;
	FILE *fd;

	// Accept new connections indefinitely with an infinite for loop.
	for (;;) {
		// Accept a new client connection; assign these sockets to "client_socket".
		client_sock = accept(server_sock, (struct sockaddr *)&address, (socklen_t*)&addrlen);
		// Proceed once a client has connected.

		// Read the client's command into "buff"
		bytes_transferred = read(client_sock, buff, 1024);

		// Handle a file upload.
		if (strstr(buff,"UPLOAD") != NULL) {
			memset(buff, 0, sizeof buff); // Clear "buff".
			
			// Notify the client that the server is ready to receive the file.
			send(client_sock , "READY", 5, 0);

			// Read the file's name from the "SENDING X" response, where "X"
			// is the file's name.
			bytes_transferred = read(client_sock, buff, 1024);

			// Extract the filename from the message and store it in the
			// variable "fn." Concatenate this with the path provided by the
			// user to produce "filename".
			bytes_transferred = read(client_sock, buff, 1024);
			char *fn = strtok(buff, " ");
			fn = strtok(NULL, " ");
			char filename[128];
			strcpy(filename, dir);
			strcat(filename, fn);

			// Print a status message to the user.
			printf("Receiving file %s ... ",filename);
			fflush(stdout);

			// Clear the destination file, then open it in "append" mode so
			// that its contents from the client can be written in chunks.
			fd = fopen(filename,"w");
			fclose(fd);
			fd = fopen(filename,"a");

			// Continue reading data from the client socket in 1024 byte chunks
			// until the client stops sending data.
			while ((bytes_transferred = read(client_sock, buff, 1024)) != 0) {
				// Write the contents of "buff" to the output file "fd".
				fwrite(buff, sizeof(buff[0]), sizeof(buff)/sizeof(buff[0]), fd);
				// Clear buff in preparation for the next chunk.
				memset(buff, 0, sizeof buff);
			}

			// Close the output file, then update the status message to the
			// user that the file has been transferred.
			fclose(fd);
			printf("done.\n");

		} 
		// Handle file download.
		else if (strstr(buff,"DOWNLOAD") != NULL) {
			memset(buff, 0, sizeof buff); // Clear "buff".
			
			// Notify the client that the server is ready to send the file.
			send(client_sock , "READY", 5, 0);

			// Read the desired file's name into "fn" Concatenate this with
			// the path provided by the user to produce "filename".
			bytes_transferred = read(client_sock, buff, 1024);
			char *fn = strtok(buff, " ");
			fn = strtok(NULL, " ");
			char filename[128];
			strcpy(filename, dir);
			strcat(filename, fn);

			// Verify that the desired file. If it does not, send an error
			// message "FILE_NOT_FOUND" to the client.
			if (access(filename, F_OK) != -1) {
				// Open the target file.
				fd = fopen(filename,"r");
				if (!fd) {
					printf("Error: Failed to open file.\n");
					exit(1);
				}

				// If the file does exist, calculate it's size, then store it
				// in a temporary "temp" string.
				char temp[16];
				fseek(fd, 0L, SEEK_END);
				int filesize = ftell(fd);
				sprintf(temp, "%d", filesize);
				rewind(fd);

				// Prepare the client to receive the file with the message
				// "SENDING".
				char msg[64];
				strcpy(msg, "SENDING ");
				strcat(msg, temp);
				send(client_sock, msg, strlen(msg), 0);

				// Print a status message to the user.
				printf("Sending file %s ... ",filename);
				fflush(stdout);

				// Read 1k chunks from the file at a time, then send them to
				// the client until no more data remains.
				memset(buff, 0, sizeof buff); // Clear "buff".
				while ((bytes_transferred = fread(buff, 1, 1024, fd)) > 0)
				{
					send(client_sock, buff, bytes_transferred, 0);
					memset(buff, 0, sizeof buff); // Clear "buff".
				}

				// Close the input file, then update the status message to the
				// user that the file has been transferred.
				fclose(fd);
				printf("done.\n");
			}
			// If the requested file does not exist, send an error message
			// "FILE_NOT_FOUND" to the client.
			else {
				send(client_sock , "FILE_NOT_FOUND", 14, 0);
			}
		} 
		// Handle invalid commands.
		else {
			// If an invalid command was sent by the client, respond with
			// "ERROR" and print a message to the server console.
			send(client_sock, "ERROR", 5, 0);
			printf("Client sent invalid command '%s'\n",buff);
		}

		// After each operation, close the connection.
		close(client_sock);
	}
	return 0; // Return success
} 
