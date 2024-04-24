#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>

// Define the maximum number of ports per core
#define PORTS_PER_CORE 5000

// Define the maximum number of threads (cores) to use
#define MAX_THREADS (get_nprocs() - 1)

// Define the structure to hold thread parameters
struct ThreadParams {
    char *target_ip;
    int start_port;
    int end_port;
};

// Function to perform port scanning for a range of ports
void *port_scan(void *arg) {
    // Extract parameters from argument
    struct ThreadParams *params = (struct ThreadParams *)arg;

    // Perform port scanning for the specified range of ports
    for (int port = params->start_port; port <= params->end_port; port++) {
        // Open a socket and attempt to connect to the target port
        int sockfd;
        struct sockaddr_in serv_addr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket");
            exit(EXIT_FAILURE);
        }
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(params->target_ip);
        serv_addr.sin_port = htons(port);
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            // Port is closed
            // printf("Port %d is closed\n", port);
        } else {
            printf("Port %d is open\n", port);
            // Try to identify the service and retrieve version information
            char buffer[1024];
            ssize_t bytes_read = read(sockfd, buffer, sizeof(buffer));
            if (bytes_read > 0) {
                // Check if the response contains version information
                printf("%s", buffer);
                if (strstr(buffer, "HTTP/") != NULL) {
                  // This is an HTTP response, extract more information
                  char *status_start = strstr(buffer, "HTTP/");   //Find the
                                                                  //start if the
                                                                  //status line
                  if (status_start != NULL) {
                    // Extract the status code and reason phrase
                    int status_code;
                    char reason_phrase[1024];
                    sscanf(status_start, "HTTP/%*s %d %1023[^\r\n]", &status_code, reason_phrase);
                    printf("Status Code: %d\n", status_code);
                    printf("Reason Phrase: %s\n", reason_phrase);
                  }
                  // Extract and print other headers if needed
                  char *header_start = strstr(buffer, "\r\n");
                  if(header_start != NULL) {
                    header_start += 2;
                    while(*header_start != '\r' && *(header_start + 1) != '\n') {
                      char *header_end = strstr(header_start, ": ");
                      if(header_end != NULL) {
                        *header_end = '\0';
                        printf("%s: ", header_start);
                        header_end += 2;
                        char *header_value_end = strstr(header_end, "\r\n");
                        if(header_value_end != NULL) {
                          *header_value_end = '\0';
                          printf("%s\n", header_end);
                          header_start = header_value_end + 2;
                        } else {
                          printf("ERROR: Header value not terminated properly\n");
                          break;
                        }
                      } else {
                        printf("ERROR: Malformed header\n");
                        break;
                      }
                    }
                  }
                } else if (strstr(buffer, "SSH-") != NULL) {
                    // This is an SSH service, extract version information
                    // Example: SSH-2.0-OpenSSH_7.6p1 Ubuntu-4ubuntu0.3
                    // Extract version from the response
                    // Print the version information
                }
                // Add more checks for other services as needed
            }
            close(sockfd);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    // Parse command-line arguments (target IP, start port, end port)
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <start_port> <end_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *target_ip = argv[1];
    int start_port = atoi(argv[2]);
    int end_port = atoi(argv[3]);

    // Calculate the number of ports to scan per core
    int total_ports = end_port - start_port + 1;
    int ports_per_core = total_ports / MAX_THREADS;

    // Create an array to hold thread IDs
    pthread_t threads[MAX_THREADS];

    // Launch threads for port scanning
    for (int i = 0; i < MAX_THREADS; i++) {
        // Define the port range for this thread
        int thread_start_port = start_port + i * ports_per_core;
        int thread_end_port = thread_start_port + ports_per_core - 1;
        if (i == MAX_THREADS - 1) {
            // Adjust the end port for the last thread to cover remaining ports
            thread_end_port = end_port;
        }

        // Allocate memory for thread parameters
        struct ThreadParams *params = malloc(sizeof(struct ThreadParams));
        if (params == NULL) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        params->target_ip = target_ip;
        params->start_port = thread_start_port;
        params->end_port = thread_end_port;

        // Create a thread for port scanning
        if (pthread_create(&threads[i], NULL, port_scan, (void *)params) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
