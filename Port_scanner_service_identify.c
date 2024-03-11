#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>

#define TARGET_HOST "192.168.1.5"
#define START_PORT 1
#define END_PORT 1000
#define MAX_BUFFER_SIZE 1024

void banner_grabbing(int sockfd) {
        char buffer[MAX_BUFFER_SIZE];
        int bytes_received;

        bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if(bytes_received < 0) {
                perror("Error receiving data from socket");
                return;
        }

        if(bytes_received < 0) {
                perror("Error receiving data from socket");
                return;
        }
        buffer[bytes_received] = '\0';

        printf("%s\n", buffer);

        if(strstr(buffer, "Telnet") != NULL || strstr(buffer, "Welcome") != NULL) {
                printf("Telnet\n");
        } else if (strstr(buffer, "HTTP") != NULL) {
                printf("HTTP\n");
        } else {
                printf("Unknown service\n");
        }
}

int main() {
        int sockfd;
        struct sockaddr_in  target_addr;
        int port;

        for(port=START_PORT; port <= END_PORT; port++) {
                sockfd = socket(AF_INET, SOCK_STREAM, 0);
                if(sockfd < 0) {
                        perror("Socket creation failed");
                        exit(EXIT_FAILURE);
                }
                target_addr.sin_family = AF_INET;
                target_addr.sin_port = htons(port);
                target_addr.sin_addr.s_addr = inet_addr(TARGET_HOST);

                if (connect(sockfd, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
                        continue;
                        printf("Port %d: Closed or filtered\n", port);
                } else {
                        printf("Port %d: Open\t", port);
                        //printf("Attempting to detect service on port %d...\n",  port);
                        banner_grabbing(sockfd);
                //      printf("\n");
                        close(sockfd);
                }
        }
        return 0;
}
