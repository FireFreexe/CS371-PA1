/*
# Copyright 2025 University of Kentucky
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
*/

/* 
Please specify the group members here

# Student #1: Claire Caldwell 
# Student #2: Casey Howard
# Student #3: 

*/

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <pthread.h>

#define MAX_EVENTS 64
#define MESSAGE_SIZE 16
#define DEFAULT_CLIENT_THREADS 4

char *server_ip = "127.0.0.1";
int server_port = 12345;
int num_client_threads = DEFAULT_CLIENT_THREADS;
int num_requests = 1000000;

/*
 * This structure is used to store per-thread data in the client
 */
typedef struct {
    int epoll_fd;        /* File descriptor for the epoll instance, used for monitoring events on the socket. */
    int socket_fd;       /* File descriptor for the client socket connected to the server. */
    long long total_rtt; /* Accumulated Round-Trip Time (RTT) for all messages sent and received (in microseconds). */
    long total_messages; /* Total number of messages sent and received. */
    float request_rate;  /* Computed request rate (requests per second) based on RTT and total messages. */
} client_thread_data_t;

/*
 * This function runs in a separate client thread to handle communication with the server
 */
void *client_thread_func(void *arg) {
    client_thread_data_t *data = (client_thread_data_t *)arg;
    struct epoll_event events[MAX_EVENTS];
    char send_buf[MESSAGE_SIZE] = "ABCDEFGHIJKMLNOP"; /* Send 16-Bytes message every time */
    char recv_buf[MESSAGE_SIZE];
    struct timeval start, end;
    int messages_sent = 0;
    
    // Hint 1: register the "connected" client_thread's socket in the its epoll instance
    // Hint 2: use gettimeofday() and "struct timeval start, end" to record timestamp, which can be used to calculated RTT.

    /* TODO:
     * It sends messages to the server, waits for a response using epoll,
     * and measures the round-trip time (RTT) of this request-response.
     */

    for (int i = 0; i < num_requests; i++) {
		
        int term_outer_loop = 0; // Flag to terminate the outer loop
        gettimeofday(&start, NULL); // Record the start time
        ssize_t bytes_sent = write(data->socket_fd, send_buf, MESSAGE_SIZE); // Send message to server
        if (bytes_sent == -1) {
                
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				
				usleep(1000); // wait 1ms before trying again
				i--;
				continue;
			}
			perror("Send message failed");
			// Exit the loop on failure
			break; 
		}
        int term_inner_loop = 0; // Flag to terminate the inner loop
        while (!term_inner_loop) {
			
            int num_events = epoll_wait(data->epoll_fd, events, MAX_EVENTS, 5000); // Wait for events
            if (num_events == -1) {
				
                    perror("Epoll wait failed");
                    term_inner_loop = 1;
                    term_outer_loop = 1;
                    break; //Exit inner loop
            }
            if (num_events == 0) continue;  // Timeout, retry
            for (int i = 0; i < num_events; i++) {
                    
                if (events[i].data.fd == data->socket_fd) {

                    ssize_t num_bytes_received = read(data->socket_fd, recv_buf, MESSAGE_SIZE);
                    if (num_bytes_received == -1) {
						
                        if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
                        perror("No response");
                        term_inner_loop = 1;
                        term_outer_loop = 1;
                        break;
                    }
					
					// Connection closed by server
                    if (num_bytes_received == 0) { // Connection closed
					
                        term_inner_loop = 1;
                        term_outer_loop = 1;
                        break;
                    }
                    if (num_bytes_received == MESSAGE_SIZE) {
						
                        gettimeofday(&end, NULL);
						
						/// RTT stuff
                        long long rtt = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec); 
                        data->total_rtt += rtt;
                        data->total_messages++;
                                
                        term_inner_loop = 1;
                        break;
                    }
                }
            }
        }

        if (term_outer_loop) {
            break; // exit Outer Loop
        }
    }
    
	// Check if any messages were sent
    if (data->total_messages > 0) {
        data->request_rate = (float)data->total_messages / ((float)data->total_rtt / 1000000.0);
    }
	
	// END TODO
	
    return NULL;

}

/*
 * This function orchestrates multiple client threads to send requests to a server,
 * collect performance data of each threads, and compute aggregated metrics of all threads.
 */
void run_client() {
	
    pthread_t threads[num_client_threads];
    client_thread_data_t thread_data[num_client_threads];
    struct sockaddr_in server_address;
    
	/* TODO:
     * Create sockets and epoll instances for client threads
     * and connect these sockets of client threads to the server
     */
	
	// Initialize Server Address Structure
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    for (int i = 0; i < num_client_threads; i++) {
		
		//create socket for client
        thread_data[i].socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (thread_data[i].socket_fd == -1) {
			
			// error handleing
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
		
        // Set socket to non-blocking mode
        int flag = fcntl(thread_data[i].socket_fd, F_GETFL, 0);
        fcntl(thread_data[i].socket_fd, F_SETFL, flag | O_NONBLOCK);

        if (connect(thread_data[i].socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
			//check if connection is in progress
            if (errno != EINPROGRESS) {
				
				// error handleing for connection failure
                perror("Connecton failed");
                exit(EXIT_FAILURE);
            }
        }
		// create epoll instance for the thread
        thread_data[i].epoll_fd = epoll_create1(0);
        if (thread_data[i].epoll_fd == -1) {
			
            perror("Epoll creation failed");
            exit(EXIT_FAILURE);
        }
		
		// initialize Epoll_eVent
        struct epoll_event e_v;
        e_v.events = EPOLLIN | EPOLLOUT;
        e_v.data.fd = thread_data[i].socket_fd;
        if (epoll_ctl(thread_data[i].epoll_fd, EPOLL_CTL_ADD, thread_data[i].socket_fd, &e_v) == -1) {
			// Error handleing
            perror("Epoll_ctl failed");
            exit(EXIT_FAILURE);
        }

		// RTT initialization for this thread
        thread_data[i].total_rtt = 0;
        thread_data[i].total_messages = 0;
        thread_data[i].request_rate = 0;
    }

    // END TODO

    // Hint: use thread_data to save the created socket and epoll instance for each thread
    // You will pass the thread_data to pthread_create() as below
	
    for (int i = 0; i < num_client_threads; i++) {
		
        if (pthread_create(&threads[i], NULL, client_thread_func, &thread_data[i]) != 0);
    }

    /* TODO:
     * Wait for client threads to complete and aggregate metrics of all client threads
     */
	 
	// Initialization
    long long total_rtt = 0;
    long total_messages = 0;
    float total_request_rate = 0.0;

    // Find all metrics
    for (int i = 0; i < num_client_threads; i++) {
		//Wait for threads to finish
        pthread_join(threads[i], NULL);
		// close sockets and epoll insatnces
        close(thread_data[i].socket_fd);
        close(thread_data[i].epoll_fd);
		// RTT calculations
        total_rtt += thread_data[i].total_rtt;
        total_messages += thread_data[i].total_messages;
        total_request_rate += thread_data[i].request_rate;
    }

    // END TODO

    // Print Metrics
    printf("Average RTT: %lld us\n", total_rtt / total_messages);
    printf("Total Request Rate: %.2f messages/s\n", total_request_rate);
}

void run_server() {
    
    /* TODO:
     * Server creates listening socket and epoll instance.
     * Server registers the listening socket to epoll
     */
	
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
		
        perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    int option = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) == -1) {
		
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    fcntl(server_socket, F_SETFL, O_NONBLOCK);

	// server address initializations
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr(server_ip);
    server_address.sin_port = htons(server_port);

	//Error handleings
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
		
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) == -1) {
		
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
		
        perror("Epoll creation failed");
        exit(EXIT_FAILURE);
    }

	// Epoll_eVent initialization
    struct epoll_event e_v;
    e_v.events = EPOLLIN;
    e_v.data.fd = server_socket;
	
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &e_v) == -1) {
		
        perror("Epoll_ctl failed");
        exit(EXIT_FAILURE);
    }

	// Hold events from epoll
    struct epoll_event events[MAX_EVENTS];
	
	// END TODO
	
	/* Server's run-to-completion event loop */
    while (1) {
		/* TODO:
         * Server uses epoll to handle connection establishment with clients
         * or receive the message from clients and echo the message back
         */
		 
		// Wait for events on the epoll instance
        int num_fd = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_fd == -1) {
			
            perror("Epoll wait failed");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < num_fd; i++) {
			// Check if the event is for the server socket
            if (events[i].data.fd == server_socket) {
				// Client addr initialization
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_socket == -1) {
					
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
                        perror("Accept failed");
                    }
					
                    continue;
				}

                fcntl(client_socket, F_SETFL, O_NONBLOCK);

                // Epoll_eVent initialization
                struct epoll_event e_v;
                e_v.events = EPOLLIN | EPOLLET;
                e_v.data.fd = client_socket;
				
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &e_v) == -1) {
					
                    perror("Epoll_ctl failed for client socket");
                    close(client_socket);
                    continue;
                }
		    }
			else {
				// Get the client socket from the event
                int client_socket = events[i].data.fd;
                char buffer[MESSAGE_SIZE];
				ssize_t bytes_read = read(client_socket, buffer, MESSAGE_SIZE);
				
                if (bytes_read == -1) {
					
                    if (errno != EAGAIN && errno != EWOULDBLOCK) {
						
                        close(client_socket);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                    }
                    continue;
                }
                // Check if the client has closed the connection
                if (bytes_read == 0) {
					
                    close(client_socket);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                    continue;
                }
				// Check if the full message was received
                if (bytes_read == MESSAGE_SIZE) {
					
                    ssize_t bytes_sent = write(client_socket, buffer, MESSAGE_SIZE);
                    if (bytes_sent == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                        close(client_socket);
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_socket, NULL);
                    }
                }
            }
        }
		
		// END TODO
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1 && strcmp(argv[1], "server") == 0) {
        if (argc > 2) server_ip = argv[2];
        if (argc > 3) server_port = atoi(argv[3]);

        run_server();
    } else if (argc > 1 && strcmp(argv[1], "client") == 0) {
        if (argc > 2) server_ip = argv[2];
        if (argc > 3) server_port = atoi(argv[3]);
        if (argc > 4) num_client_threads = atoi(argv[4]);
        if (argc > 5) num_requests = atoi(argv[5]);

        run_client();
    } else {
        printf("Usage: %s <server|client> [server_ip server_port num_client_threads num_requests]\n", argv[0]);
    }

    return 0;
}
