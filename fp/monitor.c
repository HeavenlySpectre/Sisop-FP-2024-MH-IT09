#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>

#define PORT 8080
#define BUF_SIZE 1024

void *receive_messages(void *arg);

int main() {
    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, receive_messages, (void *)(intptr_t)sockfd) != 0) {
        perror("Failed to create thread");
    }

    pthread_join(tid, NULL);
    close(sockfd);
    return 0;
}

void *receive_messages(void *arg) {
    int sockfd = (intptr_t)arg;
    char buffer[BUF_SIZE] = {0};

    while (1) {
        int bytes_read = read(sockfd, buffer, BUF_SIZE);
        if (bytes_read <= 0) {
            break;
        }
        buffer[bytes_read] = '\0';
        printf("%s\n", buffer);
    }

    pthread_exit(NULL);
}
