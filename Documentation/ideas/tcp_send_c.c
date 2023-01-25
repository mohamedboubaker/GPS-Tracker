#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[256];

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set the server address
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Error setting server address");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        exit(EXIT_FAILURE);
    }

    // Prepare the message to send
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "Hello, Server!");

    // Send the message
    if (send(sockfd, buffer, sizeof(buffer), 0) < 0) {
        perror("Error sending data");
        exit(EXIT_FAILURE);
    }

    printf("Data sent: %s\n", buffer);

    // Close the socket
    close(sockfd);
    return 0;
}
