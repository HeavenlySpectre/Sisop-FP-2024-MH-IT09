#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>
#include <time.h>
#include <sys/inotify.h>

#define PORT 8080
#define BUF_SIZE 1024
#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define EVENT_BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

typedef struct {
    int sockfd;
    char username[BUF_SIZE];
    char channel[BUF_SIZE];
    char room[BUF_SIZE];
} monitor_args_t;

void *monitor_chat(void *args);
void login_user(int sockfd, char *username, char *password);
void read_existing_chat(const char *filepath);

int main(int argc, char *argv[]) {
    if (argc < 4) {
        printf("Usage: ./monitor LOGIN username -p password\n");
        exit(EXIT_FAILURE);
    }

    char *command = argv[1];
    char *username = argv[2];
    char *password = argv[4];

    if (strcmp(command, "LOGIN") != 0) {
        printf("Invalid command. Use LOGIN.\n");
        exit(EXIT_FAILURE);
    }

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

    login_user(sockfd, username, password);

    char channel_name[BUF_SIZE];
    char room_name[BUF_SIZE];

    printf("[%s] ", username);
    scanf(" -channel %s -room %s", channel_name, room_name);

    monitor_args_t args;
    args.sockfd = sockfd;
    strncpy(args.username, username, BUF_SIZE);
    strncpy(args.channel, channel_name, BUF_SIZE);
    strncpy(args.room, room_name, BUF_SIZE);

    printf("[%s] -channel %s -room %s\n", username, channel_name, room_name);
    printf("~isi chat~\n");

    // Read existing chat history from chat.csv
    char chat_filepath[BUF_SIZE];
    snprintf(chat_filepath, BUF_SIZE, "./%s/%s/chat.csv", channel_name, room_name);
    read_existing_chat(chat_filepath);

    pthread_t tid;
    if (pthread_create(&tid, NULL, monitor_chat, (void *)&args) != 0) {
        perror("Failed to create thread");
        exit(EXIT_FAILURE);
    }

    pthread_join(tid, NULL);

    close(sockfd);
    return 0;
}

void login_user(int sockfd, char *username, char *password) {
    char buffer[BUF_SIZE] = {0};

    sprintf(buffer, "LOGIN %s %s", username, password);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes_received = read(sockfd, buffer, BUF_SIZE);
    buffer[bytes_received] = '\0';
    printf("%s\n", buffer);

    if (!strstr(buffer, "berhasil login")) {
        printf("Login failed\n");
        exit(EXIT_FAILURE);
    }
}

void *monitor_chat(void *args) {
    monitor_args_t *mon_args = (monitor_args_t *)args;
    int sockfd = mon_args->sockfd;
    char *username = mon_args->username;
    char *channel = mon_args->channel;
    char *room = mon_args->room;
    char buffer[BUF_SIZE];
    int inotify_fd, watch_fd;
    char event_buf[EVENT_BUF_LEN];

    sprintf(buffer, "JOIN %s", channel);
    send(sockfd, buffer, strlen(buffer), 0);

    int bytes_received = read(sockfd, buffer, BUF_SIZE);
    buffer[bytes_received] = '\0';
    if (!strstr(buffer, "JOIN ")) {
        printf("%s\n", buffer);
        pthread_exit(NULL);
    }

    sprintf(buffer, "JOIN %s", room);
    send(sockfd, buffer, strlen(buffer), 0);

    bytes_received = read(sockfd, buffer, BUF_SIZE);
    buffer[bytes_received] = '\0';
    if (!strstr(buffer, "JOIN ")) {
        printf("%s\n", buffer);
        pthread_exit(NULL);
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // Set up inotify to monitor changes to chat.csv
    char chat_filepath[BUF_SIZE];
    snprintf(chat_filepath, BUF_SIZE, "./%s/%s/chat.csv", channel, room);

    inotify_fd = inotify_init();
    if (inotify_fd < 0) {
        perror("inotify_init");
    }

    watch_fd = inotify_add_watch(inotify_fd, chat_filepath, IN_MODIFY);
    if (watch_fd < 0) {
        perror("inotify_add_watch");
    }

    while (1) {
        fd_set fds;
        struct timeval tv;
        int ret;

        FD_ZERO(&fds);
        FD_SET(sockfd, &fds);
        FD_SET(inotify_fd, &fds);

        tv.tv_sec = 1;
        tv.tv_usec = 0;

        ret = select(sockfd + 1, &fds, NULL, NULL, &tv);

        if (ret > 0) {
            if (FD_ISSET(sockfd, &fds)) {
                bytes_received = read(sockfd, buffer, BUF_SIZE);
                if (bytes_received > 0) {
                    buffer[bytes_received] = '\0';
                    printf("%s\n", buffer);
                }
            }

            if (FD_ISSET(inotify_fd, &fds)) {
                int length = read(inotify_fd, event_buf, EVENT_BUF_LEN);
                if (length < 0) {
                    perror("read");
                }

                int i = 0;
                while (i < length) {
                    struct inotify_event *event = (struct inotify_event *)&event_buf[i];
                    if (event->mask & IN_MODIFY) {
                        read_existing_chat(chat_filepath);
                    }
                    i += EVENT_SIZE + event->len;
                }
            }
        }

        char input[BUF_SIZE];
        if (fgets(input, BUF_SIZE, stdin) != NULL) {
            input[strcspn(input, "\n")] = 0;
            if (strcmp(input, "EXIT") == 0) {
                sprintf(buffer, "[%s/%s/%s] EXIT", username, channel, room);
                send(sockfd, buffer, strlen(buffer), 0);
                sprintf(buffer, "[%s] EXIT", username);
                send(sockfd, buffer, strlen(buffer), 0);
                break;
            }
        }
        usleep(100000); // Small delay to reduce CPU usage
    }

    inotify_rm_watch(inotify_fd, watch_fd);
    close(inotify_fd);

    pthread_exit(NULL);
}

void read_existing_chat(const char *filepath) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("fopen");
        return;
    }

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), file)) {
        printf("%s", line);
    }

    fclose(file);
}
