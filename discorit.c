#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 8080
#define BUF_SIZE 1024

void register_user(int sockfd, char *username, char *password);
void login_user(int sockfd, char *username, char *password);
void post_login_interaction(int sockfd, char *username);
void handle_edit_profile(int sockfd, char *input, char *username, char *current_context);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: ./discorit [REGISTER|LOGIN] username -p password\n");
        exit(EXIT_FAILURE);
    }

    char *command = argv[1];
    char *username = argv[2];
    char *password = argv[4];

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

    if (strcmp(command, "REGISTER") == 0) {
        register_user(sockfd, username, password);
    } else if (strcmp(command, "LOGIN") == 0) {
        login_user(sockfd, username, password);
    } else {
        printf("Invalid command. Use REGISTER or LOGIN.\n");
    }

    close(sockfd);
    return 0;
}

void register_user(int sockfd, char *username, char *password) {
    char buffer[BUF_SIZE] = {0};

    sprintf(buffer, "REGISTER %s %s", username, password);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes_received = read(sockfd, buffer, BUF_SIZE);
    buffer[bytes_received] = '\0';
    printf("%s\n", buffer);
}

void login_user(int sockfd, char *username, char *password) {
    char buffer[BUF_SIZE] = {0};

    sprintf(buffer, "LOGIN %s %s", username, password);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes_received = read(sockfd, buffer, BUF_SIZE);
    buffer[bytes_received] = '\0';
    printf("%s\n", buffer);

    if (strstr(buffer, "berhasil login")) {
        post_login_interaction(sockfd, username);
    }
}

void post_login_interaction(int sockfd, char *username) {
    char buffer[BUF_SIZE] = {0};
    char input[BUF_SIZE];
    char current_context[BUF_SIZE] = {0};  // To keep track of the current context (channel or room)

    sprintf(current_context, "[%s] ", username);  // Initial context

    printf("%s", current_context);
    while (fgets(input, BUF_SIZE, stdin) != NULL) {
        send(sockfd, input, strlen(input), 0);

        int bytes_received = read(sockfd, buffer, BUF_SIZE);
        buffer[bytes_received] = '\0';

        if (strncmp(input, "JOIN ", 5) == 0) {
            if (strstr(buffer, "Key: ") != NULL) {
                printf("%s ", buffer);
                fflush(stdout);
                fgets(input, BUF_SIZE, stdin);
                input[strcspn(input, "\n")] = 0;
                send(sockfd, input, strlen(input), 0);

                bytes_received = read(sockfd, buffer, BUF_SIZE);
                buffer[bytes_received] = '\0';
                printf("%s\n", buffer);
            } else {
                printf("%s\n", buffer);
            }

            if (strstr(buffer, "anda telah diban") != NULL) {
                printf("%s\n", buffer);
                continue;
            }

            if (strstr(buffer, "JOIN ") != NULL) {
                char *joined = strtok(buffer + 5, "\n");
                sprintf(current_context, "[%s/%s] ", username, joined);
            }
        } else if (strncmp(input, "EDIT PROFILE SELF -u ", 21) == 0 || strncmp(input, "EDIT PROFILE SELF -p ", 21) == 0) {
            handle_edit_profile(sockfd, input, username, current_context);
            continue;
        } else if (strncmp(input, "EXIT", 4) == 0) {
            char *last_slash = strrchr(current_context, '/');
            if (last_slash != NULL) {
                *last_slash = '\0'; // Remove the last context part
                char *second_last_slash = strrchr(current_context, '/');
                if (second_last_slash != NULL) {
                    sprintf(current_context, "%s] ", current_context);
                } else {
                    sprintf(current_context, "[%s] ", current_context + 1); // Restore the root user context
                }
            } else {
                printf("Exiting...\n");
                break;
            }
        } else {
            printf("%s\n", buffer);
        }

        printf("%s", current_context);
    }
}

void handle_edit_profile(int sockfd, char *input, char *username, char *current_context) {
    char buffer[BUF_SIZE] = {0};

    if (strncmp(input, "EDIT PROFILE SELF -u ", 21) == 0) {
        send(sockfd, input, strlen(input), 0);
        int bytes_received = read(sockfd, buffer, BUF_SIZE);
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);

        if (strstr(buffer, "Profil diupdate") != NULL) {
            char *new_username_start = strchr(buffer, '[') + 1;
            char *new_username_end = strchr(new_username_start, ']');
            *new_username_end = '\0';
            strcpy(username, new_username_start); // Update the username in the client
            sprintf(current_context, "[%s] ", new_username_start); // Update the context
        }
    } else if (strncmp(input, "EDIT PROFILE SELF -p ", 21) == 0) {
        send(sockfd, input, strlen(input), 0);
        int bytes_received = read(sockfd, buffer, BUF_SIZE);
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }

    printf("%s", current_context);  // Ensure prompt is printed after profile edit
}
