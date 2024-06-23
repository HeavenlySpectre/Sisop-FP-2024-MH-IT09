#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <stdbool.h>
#include <bcrypt.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>

#define PORT 8080
#define BUF_SIZE 1024
#define BASE_DIR "/home/azrael/sisop/finalproject"

const char *USER_FILE_PATH = "/home/azrael/sisop/finalproject/users.csv";

typedef struct AuthenticatedChannel {
    char channel_name[BUF_SIZE];
    struct AuthenticatedChannel *next;
} AuthenticatedChannel;

void *handle_client(void *arg);
void register_user(char *username, char *password, int client_sock);
void login_user(char *username, char *password, int client_sock);
void handle_post_login_commands(int client_sock, char *username, char *role);
void list_channels(int client_sock);
void list_rooms(int client_sock, char *channel);
void lista_users(int client_sock, const char *channel);  // Ensure this is correctly defined if used
void list_all_users(int client_sock);
void join_channel(int client_sock, char *channel, char *username, char *global_role, AuthenticatedChannel **auth_channels);
void join_room(int client_sock, char *channel, char *room, char *username, char *role);
void edit_user(int client_sock, char *old_username, char *new_username);
void change_password(int client_sock, char *username, char *new_password);
void remove_user(int client_sock, char *username);
void initialize_files();
char* get_user_role_in_channel(char *username, char *channel);
void create_channel(int client_sock, char *username, char *channel, char *key);
void edit_channel(int client_sock, char *old_channel, char *new_channel);
void delete_channel(int client_sock, char *channel, const char *admin_username);
void create_room(int client_sock, const char *channel, const char *room, const char *admin_username);
void edit_room(int client_sock, char *channel, char *old_room, char *new_room);
void delete_room(int client_sock, char *channel, char *room);
void delete_all_rooms(int client_sock, char *channel, const char *admin_username);
void delete_directory(const char *path);
int is_user_banned(const char *username, const char *channel);
void ban_user(int client_sock, const char *channel, const char *username, const char *admin_username);
void unban_user(int client_sock, const char *channel, const char *username, const char *admin_username);
void remove_user_from_channel(int client_sock, const char *channel, const char *username, const char *admin_username);
int is_channel_authenticated(AuthenticatedChannel *auth_channels, char *channel);
bool is_channel_admin(const char *username, const char *channel, const char *global_role);
void list_users(int client_sock, const char *channel);  // Add this declaration
void chat_message(int client_sock, const char *channel, const char *room, const char *username, const char *message);
void see_chat(int client_sock, const char *channel, const char *room);
void edit_chat(int client_sock, const char *channel, const char *room, int chat_id, const char *new_message, const char *username);
void delete_chat(int client_sock, const char *channel, const char *room, int chat_id);
void log_user_activity(const char *format, ...);

void daemonlucknut() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Exit the parent process
        exit(EXIT_SUCCESS);
    }

    // Create a new session
    if (setsid() < 0) {
        perror("setsid failed");
        exit(EXIT_FAILURE);
    }

    // Fork again to ensure the process cannot acquire a terminal
    pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        // Exit the parent process
        exit(EXIT_SUCCESS);
    }

    // Set file permissions
    umask(0);

    // Change the working directory to the root directory
    if (chdir("/") < 0) {
        perror("chdir failed");
        exit(EXIT_FAILURE);
    }

    // Close all open file descriptors
    for (int x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
        close(x);
    }

    // Redirect standard file descriptors to log files
    int fd0 = open("/dev/null", O_RDONLY);
    int fd1 = open("/tmp/server_stdout.log", O_WRONLY | O_CREAT | O_TRUNC, 0640);
    int fd2 = open("/tmp/server_stderr.log", O_WRONLY | O_CREAT | O_TRUNC, 0640);

    if (fd0 != -1) {
        dup2(fd0, STDIN_FILENO);
        close(fd0);
    }

    if (fd1 != -1) {
        dup2(fd1, STDOUT_FILENO);
        close(fd1);
    }

    if (fd2 != -1) {
        dup2(fd2, STDERR_FILENO);
        close(fd2);
    }
}

int main() {
    daemonlucknut();

    initialize_files();

    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    while ((client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) >= 0) {
        if (pthread_create(&tid, NULL, handle_client, (void *)(intptr_t)client_sock) != 0) {
            perror("Failed to create thread");
        }
    }

    if (client_sock < 0) {
        perror("Server accept failed");
        exit(EXIT_FAILURE);
    }

    close(server_fd);
    return 0;
}

char* get_global_role(char *username) {
    FILE *fp = fopen(USER_FILE_PATH, "r");
    if (!fp) {
        perror("File open error");
        return NULL;
    }

    char line[BUF_SIZE];
    char *role = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_password = strtok(NULL, ",");
        char *saved_role = strtok(NULL, "\n");

        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            role = strdup(saved_role); // Allocate memory for the role string
            break;
        }
    }

    fclose(fp);
    return role;
}

void *handle_client(void *arg) {
    int client_sock = (intptr_t)arg;
    char buffer[BUF_SIZE] = {0};

    read(client_sock, buffer, BUF_SIZE);
    char *command = strtok(buffer, " ");
    char *username = strtok(NULL, " ");
    char *password = strtok(NULL, " ");

    if (strcmp(command, "REGISTER") == 0) {
        register_user(username, password, client_sock);
    } else if (strcmp(command, "LOGIN") == 0) {
        login_user(username, password, client_sock);
    } else {
        char *response = "Invalid command";
        send(client_sock, response, strlen(response), 0);
    }

    pthread_exit(NULL);
}

void register_user(char *username, char *password, int client_sock) {
    FILE *fp = fopen(USER_FILE_PATH, "a+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    int user_exists = 0;
    int user_count = 0;

    // Check for existing username and count users
    fseek(fp, 0, SEEK_SET);
    while (fgets(line, sizeof(line), fp)) {
        user_count++;
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            user_exists = 1;
            break;
        }
    }

    if (user_exists) {
        char response[BUF_SIZE];
        sprintf(response, "%s sudah terdaftar", username);
        send(client_sock, response, strlen(response), 0);
    } else {
        char salt[BCRYPT_HASHSIZE];
        if (bcrypt_gensalt(12, salt) != 0) {
            perror("Salt generation error");
            fclose(fp);
            return;
        }

        char hashed_password[BCRYPT_HASHSIZE];
        if (bcrypt_hashpw(password, salt, hashed_password) != 0) {
            perror("Password encryption error");
            fclose(fp);
            return;
        }

        const char *role = user_count == 0 ? "ROOT" : "USER";
        fseek(fp, 0, SEEK_END);
        fprintf(fp, "%d,%s,%s,%s\n", user_count + 1, username, hashed_password, role);
        char response[BUF_SIZE];
        sprintf(response, "%s berhasil diregister", username);  // Simplified message
        send(client_sock, response, strlen(response), 0);
    }

    fclose(fp);
}

void login_user(char *username, char *password, int client_sock) {
    FILE *fp = fopen(USER_FILE_PATH, "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    int login_success = 0;
    char user_role[BUF_SIZE] = {0};

    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_password = strtok(NULL, ",");
        char *saved_role = strtok(NULL, ",");

        if (saved_username != NULL && saved_password != NULL && strcmp(saved_username, username) == 0) {
            if (bcrypt_checkpw(password, saved_password) == 0) {
                login_success = 1;
                strcpy(user_role, saved_role);
                // Trim newline character from user_role
                user_role[strcspn(user_role, "\n")] = 0;
                break;
            }
        }
    }

    fclose(fp);

    if (login_success) {
        char response[BUF_SIZE];
        sprintf(response, "%s berhasil login", username);
        send(client_sock, response, strlen(response), 0);
        handle_post_login_commands(client_sock, username, user_role);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "Invalid username or password");
        send(client_sock, response, strlen(response), 0);
    }
}

void handle_post_login_commands(int client_sock, char *username, char *global_role) {
    char buffer[BUF_SIZE] = {0};
    int bytes_received;

    printf("User %s logged in with global role: %s\n", username, global_role);  // Debug info

    char current_channel[BUF_SIZE] = {0};  // To keep track of the current channel
    char current_room[BUF_SIZE] = {0};     // To keep track of the current room
    AuthenticatedChannel *auth_channels = NULL;  // List of authenticated channels

    while ((bytes_received = read(client_sock, buffer, BUF_SIZE)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received command: %s\n", buffer);  // Debug info
        printf("Global Role: %s\n", global_role); // Debug info

        char *channel_role = NULL;
        if (current_channel[0] != '\0') {
            channel_role = get_user_role_in_channel(username, current_channel);
            printf("Channel Role in %s: %s\n", current_channel, channel_role); // Debug info
        }

        if (strncmp(buffer, "JOIN ", 5) == 0) {
            char *name = strtok(buffer + 5, "\n");
            printf("Command recognized as JOIN channel or room\n"); // Debug info
            if (current_channel[0] == '\0') { // Joining a channel
                printf("Attempting to join channel: %s\n", name); // Debug info
                if (is_user_banned(username, name)) {
                    printf("User %s is banned from channel %s\n", username, name);  // Debug info
                    char response[BUF_SIZE];
                    sprintf(response, "anda telah diban, silahkan menghubungi admin");
                    send(client_sock, response, strlen(response), 0);
                    continue; // Skip to the next iteration to read new command
                }
                join_channel(client_sock, name, username, global_role, &auth_channels);
                if (is_channel_authenticated(auth_channels, name)) {
                    strcpy(current_channel, name);
                    current_room[0] = '\0';
                    char response[BUF_SIZE];
                    sprintf(response, "[%s/%s]", username, current_channel);
                    send(client_sock, response, strlen(response), 0);
                }
            } else { // Joining a room
                printf("Attempting to join room: %s in channel: %s\n", name, current_channel); // Debug info
                strcpy(current_room, name);
                join_room(client_sock, current_channel, name, username, global_role);
                char response[BUF_SIZE];
                sprintf(response, "[%s/%s/%s]", username, current_channel, current_room);
                send(client_sock, response, strlen(response), 0);
            }
        } else if (strcmp(buffer, "EXIT\n") == 0) {
            printf("User %s has exited context.\n", username);  // Debug info
            if (current_room[0] != '\0') {
                current_room[0] = '\0';
                char response[BUF_SIZE];
                sprintf(response, "Exiting room...");
                send(client_sock, response, strlen(response), 0);
            } else if (current_channel[0] != '\0') {
                current_channel[0] = '\0';
                char response[BUF_SIZE];
                sprintf(response, "Exiting channel...");
                send(client_sock, response, strlen(response), 0);
            } else {
                char response[BUF_SIZE];
                sprintf(response, "Exiting session...");
                send(client_sock, response, strlen(response), 0);
                break;  // Close the connection
            }
        } else {
            if (current_channel[0] != '\0' && is_user_banned(username, current_channel)) {
                printf("User %s is banned in channel %s\n", username, current_channel);  // Debug info
                char response[BUF_SIZE];
                sprintf(response, "anda telah diban, silahkan menghubungi admin");
                send(client_sock, response, strlen(response), 0);
                current_channel[0] = '\0';  // Clear the current channel
                continue; // Skip to the next iteration to read new command
            }

            if (strncmp(buffer, "LIST CHANNEL\n", 13) == 0) {
                printf("Command recognized as LIST CHANNEL\n"); // Debug info
                list_channels(client_sock);
            } else if (strncmp(buffer, "LIST ROOM\n", 10) == 0) {
                if (current_channel[0] == '\0') {
                    char response[BUF_SIZE];
                    sprintf(response, "Harap masuk kedalam channel terlebih dahulu");
                    send(client_sock, response, strlen(response), 0);
                } else {
                    printf("Command recognized as LIST ROOM\n"); // Debug info
                    list_rooms(client_sock, current_channel);
                }
            } else if (strncmp(buffer, "LIST USER\n", 10) == 0) {
                if (strcmp(global_role, "ROOT") == 0 || (channel_role != NULL && strcmp(channel_role, "ADMIN") == 0)) {
                    printf("Command recognized as LIST USER for ROOT or Channel Admin\n"); // Debug info
                    list_users(client_sock, current_channel);
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk melihat pengguna di channel ini");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "CREATE CHANNEL ", 15) == 0) {
                if (strcmp(global_role, "USER") == 0 || strcmp(global_role, "ADMIN") == 0 || strcmp(global_role, "ROOT") == 0) {
                    char *channel = strtok(buffer + 15, " ");
                    char *key = strtok(NULL, "\n");
                    printf("Creating channel: %s with key: %s\n", channel, key); // Debug info
                    create_channel(client_sock, username, channel, key);
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk membuat channel");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "EDIT CHANNEL ", 13) == 0) {
                char *old_channel = strtok(buffer + 13, " ");
                strtok(NULL, " ");
                char *new_channel = strtok(NULL, "\n");
                if (is_channel_admin(username, old_channel, global_role)) {
                    if (new_channel != NULL) {
                        while (*new_channel == ' ') new_channel++;
                        char *end = new_channel + strlen(new_channel) - 1;
                        while (end > new_channel && *end == ' ') end--;
                        *(end + 1) = '\0';
                        printf("Editing channel from: %s to: %s\n", old_channel, new_channel); // Debug info
                        edit_channel(client_sock, old_channel, new_channel);
                    } else {
                        char response[BUF_SIZE];
                        sprintf(response, "Format salah. Gunakan: EDIT CHANNEL <old_channel> TO <new_channel>");
                        send(client_sock, response, strlen(response), 0);
                    }
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk mengedit channel");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "DEL CHANNEL ", 12) == 0) {
                char *channel = strtok(buffer + 12, "\n");
                if (is_channel_admin(username, channel, global_role)) {
                    printf("Deleting channel: %s\n", channel); // Debug info
                    delete_channel(client_sock, channel, username);  // Pass username as admin_username
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk menghapus channel");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "CREATE ROOM ", 12) == 0) {
                char *room = strtok(buffer + 12, "\n");
                char *channel = current_channel;
                if (room != NULL) {
                    while (*room == ' ') room++;
                    char *end = room + strlen(room) - 1;
                    while (end > room && *end == ' ') end--;
                    *(end + 1) = '\0';
                }
                if (is_channel_admin(username, channel, global_role)) {
                    printf("Creating room: %s in channel: %s\n", room, channel); // Debug info
                    create_room(client_sock, channel, room, username);  // Pass username as admin_username
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk membuat room");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "EDIT ROOM ", 10) == 0) {
                char *old_room = strtok(buffer + 10, " ");
                strtok(NULL, " ");
                char *new_room = strtok(NULL, "\n");
                if (is_channel_admin(username, current_channel, global_role)) {
                    if (new_room != NULL) {
                        while (*new_room == ' ') new_room++;
                        char *end = new_room + strlen(new_room) - 1;
                        while (end > new_room && *end == ' ') end--;
                        *(end + 1) = '\0';
                        printf("Editing room from: %s to: %s\n", old_room, new_room); // Debug info
                        edit_room(client_sock, current_channel, old_room, new_room);
                    } else {
                        char response[BUF_SIZE];
                        sprintf(response, "Format salah. Gunakan: EDIT ROOM <old_room> TO <new_room>");
                        send(client_sock, response, strlen(response), 0);
                    }
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk mengedit room");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "DEL ROOM ALL", 12) == 0) {
                if (is_channel_admin(username, current_channel, global_role)) {
                    delete_all_rooms(client_sock, current_channel, username);  // Pass username as admin_username
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk menghapus semua room");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "DEL ROOM ", 9) == 0) {
                char *room = strtok(buffer + 9, "\n");
                if (is_channel_admin(username, current_channel, global_role)) {
                    printf("Deleting room: %s in channel: %s\n", room, current_channel); // Debug info
                    delete_room(client_sock, current_channel, room);
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk menghapus room");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "BAN ", 4) == 0) {
                char *user = strtok(buffer + 4, "\n");
                if (is_channel_admin(username, current_channel, global_role)) {
                    printf("Banning user: %s in channel: %s\n", user, current_channel); // Debug info
                    ban_user(client_sock, current_channel, user, username);  // Pass username as admin_username
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk ban user");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "UNBAN ", 6) == 0) {
                char *user = strtok(buffer + 6, "\n");
                if (is_channel_admin(username, current_channel, global_role)) {
                    printf("Unbanning user: %s in channel: %s\n", user, current_channel); // Debug info
                    unban_user(client_sock, current_channel, user, username);  // Pass username as admin_username
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk unban user");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "REMOVE USER ", 12) == 0) {
                char *user = strtok(buffer + 12, "\n");
                if (is_channel_admin(username, current_channel, global_role)) {
                    printf("Removing user: %s from channel: %s\n", user, current_channel); // Debug info
                    remove_user_from_channel(client_sock, current_channel, user, username);  // Pass username as admin_username
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "Anda tidak memiliki akses untuk menghapus user dari channel");
                    send(client_sock, response, strlen(response), 0);
                }
            } else if (strncmp(buffer, "EDIT PROFILE SELF -u ", 21) == 0) {
                char *new_username = strtok(buffer + 21, "\n");
                printf("Editing profile of %s to new username: %s\n", username, new_username); // Debug info
                edit_user(client_sock, username, new_username);
                sprintf(username, "%s", new_username); // Update the username in the current session
                char response[BUF_SIZE];
                sprintf(response, "Profil diupdate\n[%s]", username);
                send(client_sock, response, strlen(response), 0);
            } else if (strncmp(buffer, "EDIT PROFILE SELF -p ", 21) == 0) {
                char *new_password = strtok(buffer + 21, "\n");
                printf("Editing profile of %s to new password\n", username); // Debug info
                change_password(client_sock, username, new_password);
                char response[BUF_SIZE] = {0}; // Initialize the buffer to avoid leftover data
                sprintf(response, "Password diupdate\n[%s]", username); // This line can be customized as needed
                send(client_sock, response, strlen(response), 0);
            } else if (strncmp(buffer, "CHAT ", 5) == 0) {
                char *message = strtok(buffer + 5, "\"");
                printf("Chatting in channel %s room %s: %s\n", current_channel, current_room, message);  // Debug info
                chat_message(client_sock, current_channel, current_room, username, message);
            } else if (strncmp(buffer, "SEE CHAT", 8) == 0) {
                printf("Seeing chat in channel %s room %s\n", current_channel, current_room);  // Debug info
                see_chat(client_sock, current_channel, current_room);
            } else if (strncmp(buffer, "EDIT CHAT ", 10) == 0) {
                int chat_id = atoi(strtok(buffer + 10, " "));
                char *new_message = strtok(NULL, "\"");
                printf("Editing chat %d in channel %s room %s\n", chat_id, current_channel, current_room);  // Debug info
                edit_chat(client_sock, current_channel, current_room, chat_id, new_message, username);
            } else if (strncmp(buffer, "DEL CHAT ", 9) == 0) {
                int chat_id = atoi(strtok(buffer + 9, "\n"));
                printf("Deleting chat %d in channel %s room %s\n", chat_id, current_channel, current_room);  // Debug info
                delete_chat(client_sock, current_channel, current_room, chat_id);
            } else if (strncmp(buffer, "REMOVE", 6) == 0 && strcmp(global_role, "ROOT") == 0) {
                printf("Command recognized as REMOVE for ROOT\n"); // Debug info
                char *target = strtok(buffer + 7, "\n");
                printf("Removing user: %s\n", target); // Debug info
                remove_user(client_sock, target);
                char response[BUF_SIZE];
                sprintf(response, "User %s dihapus\n[%s]", target, username);
                send(client_sock, response, strlen(response), 0);
            } else {
                printf("[%s] %s\n", username, buffer);  // Echoing back the input
                send(client_sock, buffer, strlen(buffer), 0);
            }
        }

        if (channel_role != NULL) {
            free(channel_role);  // Free allocated memory for the role string
        }
    }

    AuthenticatedChannel *tmp;
    while (auth_channels != NULL) {
        tmp = auth_channels;
        auth_channels = auth_channels->next;
        free(tmp);
    }

    close(client_sock);  // Close the connection after exit
}

bool is_channel_admin(const char *username, const char *channel, const char *global_role) {
    if (strcmp(global_role, "ROOT") == 0) {
        return true;
    }
    
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        return false;  // If the file doesn't exist, the user is not an admin
    }

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");
        char *id_user = token;
        char *name = strtok(NULL, ",");
        char *role = strtok(NULL, "\n");

        if (name && strcmp(name, username) == 0 && role && strcmp(role, "ADMIN") == 0) {
            fclose(file);
            return true;  // User is an admin in the channel
        }
    }

    fclose(file);
    return false;  // User is not an admin
}

void list_channels(int client_sock) {
    FILE *fp = fopen("channels.csv", "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char response[BUF_SIZE] = {0};

    printf("Reading channels.csv\n");  // Debug info
    int first_line = 1;  // Skip the header
    while (fgets(line, sizeof(line), fp)) {
        if (first_line) {
            first_line = 0;
            continue;  // Skip the header line
        }
        printf("Read line: %s\n", line);  // Debug info
        strtok(line, ",");  // Skip the id_channel
        char *channel_name = strtok(NULL, ",");
        strcat(response, channel_name);
        strcat(response, " ");
    }

    printf("Sending channels: %s\n", response);  // Debug info
    send(client_sock, response, strlen(response), 0);
    fclose(fp);
}

void list_rooms(int client_sock, char *channel) {
    char path[BUF_SIZE];
    sprintf(path, "%s/%s", BASE_DIR, channel);
    DIR *dir = opendir(path);
    struct dirent *entry;
    char response[BUF_SIZE] = {0};

    if (!dir) {
        perror("File open error");
        return;
    }

    int room_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, "admin") != 0 &&
            strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            strcat(response, entry->d_name);
            strcat(response, " ");
            room_count++;
        }
    }

    closedir(dir);

    if (room_count == 0) {
        sprintf(response, "Tidak ada room yang tersedia, buat terlebih dahulu!");
    }

    printf("Sending rooms: %s\n", response);  // Debug info
    send(client_sock, response, strlen(response), 0);
}

void list_users(int client_sock, const char *channel) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char response[BUF_SIZE];
    strcpy(response, "Users:\n");

    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");
        char *id_user = token;
        char *name = strtok(NULL, ",");
        char *role = strtok(NULL, "\n");

        if (name && strcmp(role, "BANNED") != 0) {
            strcat(response, name);
            strcat(response, "\n");
        }
    }

    fclose(file);
    send(client_sock, response, strlen(response), 0);
}

void list_all_users(int client_sock) {
    printf("Opening %s\n", USER_FILE_PATH);  // Debug info
    FILE *fp = fopen(USER_FILE_PATH, "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char response[BUF_SIZE] = {0};

    int first_line = 1;  // Skip the header
    while (fgets(line, sizeof(line), fp)) {
        if (first_line) {
            first_line = 0;
            continue;  // Skip the header line
        }
        printf("Read line: %s\n", line);  // Debug info
        char *username = strtok(line, ",");
        username = strtok(NULL, ",");
        strcat(response, username);
        strcat(response, " ");
    }

    printf("Sending users: %s\n", response);  // Debug info
    send(client_sock, response, strlen(response), 0);
    fclose(fp);
}

void join_channel(int client_sock, char *channel, char *username, char *global_role, AuthenticatedChannel **auth_channels) {
    char filename[BUF_SIZE];
    sprintf(filename, "%s/%s/admin/auth.csv", BASE_DIR, channel);
    printf("Checking channel existence: %s\n", filename);  // Debug info

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        char response[BUF_SIZE];
        sprintf(response, "Channel %s does not exist\n", channel);
        send(client_sock, response, strlen(response), 0);
        return;
    }
    fclose(fp);

    fp = fopen("channels.csv", "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    int channel_found = 0;
    char channel_key[BCRYPT_HASHSIZE] = {0};
    while (fgets(line, sizeof(line), fp)) {
        char *channel_id = strtok(line, ",");
        char *channel_name = strtok(NULL, ",");
        char *key = strtok(NULL, "\n");

        if (strcmp(channel_name, channel) == 0) {
            channel_found = 1;
            strcpy(channel_key, key);
            break;
        }
    }
    fclose(fp);

    if (!channel_found) {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s does not exist\n", channel);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    if (strcmp(global_role, "ROOT") == 0 || strcmp(global_role, "ADMIN") == 0) {
        char response[BUF_SIZE];
        sprintf(response, "JOIN %s\n", channel);
        send(client_sock, response, strlen(response), 0);
        AuthenticatedChannel *new_auth = (AuthenticatedChannel *)malloc(sizeof(AuthenticatedChannel));
        strcpy(new_auth->channel_name, channel);
        new_auth->next = *auth_channels;
        *auth_channels = new_auth;
        return;
    }

    char auth_file[BUF_SIZE];
    sprintf(auth_file, "%s/%s/admin/auth.csv", BASE_DIR, channel);
    FILE *auth_fp = fopen(auth_file, "r");
    int user_exists = 0;
    while (fgets(line, sizeof(line), auth_fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            user_exists = 1;
            break;
        }
    }
    fclose(auth_fp);

    if (user_exists) {
        char response[BUF_SIZE];
        sprintf(response, "JOIN %s\n", channel);
        send(client_sock, response, strlen(response), 0);
        AuthenticatedChannel *new_auth = (AuthenticatedChannel *)malloc(sizeof(AuthenticatedChannel));
        strcpy(new_auth->channel_name, channel);
        new_auth->next = *auth_channels;
        *auth_channels = new_auth;
    } else {
        char response[BUF_SIZE];
        sprintf(response, "Key: ");
        send(client_sock, response, strlen(response), 0);

        int bytes_received = read(client_sock, line, BUF_SIZE);
        line[bytes_received] = '\0';
        line[strcspn(line, "\n")] = 0;

        printf("Received key: '%s'\n", line);  // Debug info
        printf("Expected encrypted key: '%s'\n", channel_key);  // Debug info

        if (bcrypt_checkpw(line, channel_key) == 0) {
            sprintf(response, "JOIN %s\n", channel);
            send(client_sock, response, strlen(response), 0);
            AuthenticatedChannel *new_auth = (AuthenticatedChannel *)malloc(sizeof(AuthenticatedChannel));
            strcpy(new_auth->channel_name, channel);
            new_auth->next = *auth_channels;
            *auth_channels = new_auth;

            auth_fp = fopen(auth_file, "a");
            if (auth_fp) {
                int user_id = -1;
                FILE *users_fp = fopen(USER_FILE_PATH, "r");
                if (users_fp) {
                    while (fgets(line, sizeof(line), users_fp)) {
                        char *saved_id = strtok(line, ",");
                        char *saved_username = strtok(NULL, ",");
                        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
                            user_id = atoi(saved_id);
                            break;
                        }
                    }
                    fclose(users_fp);
                }
                if (user_id != -1) {
                    fprintf(auth_fp, "%d,%s,USER\n", user_id, username);
                }
                fclose(auth_fp);
            }
        } else {
            sprintf(response, "Invalid key for channel %s\n", channel);
            send(client_sock, response, strlen(response), 0);
        }
    }
    log_user_activity("user %s joined channel %s\n", username, channel);
}

void clean_auth_csv(const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char users[BUF_SIZE][3][BUF_SIZE]; // to store id, name, role
    int user_count = 0;

    // Read and store unique users
    while (fgets(line, sizeof(line), fp)) {
        char *id = strtok(line, ",");
        char *name = strtok(NULL, ",");
        char *role = strtok(NULL, "\n");

        int found = 0;
        for (int i = 0; i < user_count; i++) {
            if (strcmp(users[i][0], id) == 0 && strcmp(users[i][1], name) == 0 && strcmp(users[i][2], role) == 0) {
                found = 1;
                break;
            }
        }

        if (!found) {
            strcpy(users[user_count][0], id);
            strcpy(users[user_count][1], name);
            strcpy(users[user_count][2], role);
            user_count++;
        }
    }
    fclose(fp);

    // Write unique users back to file
    fp = fopen(filename, "w");
    if (!fp) {
        perror("File open error");
        return;
    }
    for (int i = 0; i < user_count; i++) {
        fprintf(fp, "%s,%s,%s\n", users[i][0], users[i][1], users[i][2]);
    }
    fclose(fp);
}

char* get_user_role_in_channel(char *username, char *channel) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        return NULL;
    }

    char line[BUF_SIZE];
    char *role = NULL;

    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_role = strtok(NULL, "\n");

        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            role = strdup(saved_role); // Allocate memory for the role string
            break;
        }
    }

    fclose(fp);
    return role;
}

void join_room(int client_sock, char *channel, char *room, char *username, char *global_role) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/%s/chat.csv", channel, room);
    printf("Checking room existence: %s\n", filename);  // Debug info

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        char response[BUF_SIZE];
        sprintf(response, "Room %s does not exist in channel %s\n", room, channel);
        send(client_sock, response, strlen(response), 0);
        return;
    }
    fclose(fp);

    char response[BUF_SIZE];
    sprintf(response, "JOIN %s/%s\n", channel, room);
    send(client_sock, response, strlen(response), 0);
    log_user_activity("user %s joined room %s in channel %s\n", username, room, channel);
}

void edit_user(int client_sock, char *old_username, char *new_username) {
    FILE *fp = fopen(USER_FILE_PATH, "r+");
    if (!fp) {
        perror("File open error");
        char response[BUF_SIZE];
        sprintf(response, "File error while updating username.");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_users.csv";
    FILE *temp_fp = fopen(temp_filename, "w");
    int user_found = 0;
    int user_updated = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_password = strtok(NULL, ",");
        char *saved_role = strtok(NULL, "\n");

        if (strcmp(saved_username, old_username) == 0) {
            fprintf(temp_fp, "%s,%s,%s,%s\n", saved_id, new_username, saved_password, saved_role);
            user_updated = 1;
        } else {
            fprintf(temp_fp, "%s,%s,%s,%s\n", saved_id, saved_username, saved_password, saved_role);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    if (user_updated) {
        remove(USER_FILE_PATH);
        rename(temp_filename, USER_FILE_PATH);
        char response[BUF_SIZE];
        sprintf(response, "Profil diupdate\n[%s]", new_username);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "User %s tidak ditemukan", old_username);
        send(client_sock, response, strlen(response), 0);
    }
}

void change_password(int client_sock, char *username, char *new_password) {
    FILE *fp = fopen(USER_FILE_PATH, "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_users.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    int user_found = 0;
    char salt[BCRYPT_HASHSIZE];
    char hashed_password[BCRYPT_HASHSIZE];

    if (bcrypt_gensalt(12, salt) != 0 || bcrypt_hashpw(new_password, salt, hashed_password) != 0) {
        perror("Password encryption error");
        fclose(fp);
        fclose(temp_fp);
        remove(temp_filename);
        return;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_password = strtok(NULL, ",");
        char *saved_role = strtok(NULL, "\n");

        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            fprintf(temp_fp, "%s,%s,%s,%s\n", saved_id, saved_username, hashed_password, saved_role);
            user_found = 1;
        } else {
            fprintf(temp_fp, "%s,%s,%s,%s\n", saved_id, saved_username, saved_password, saved_role);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(USER_FILE_PATH);
    rename(temp_filename, USER_FILE_PATH);

    char response[BUF_SIZE];
    if (user_found) {
        sprintf(response, "Password diupdate");
    } else {
        sprintf(response, "User %s tidak ditemukan", username);
    }
    send(client_sock, response, strlen(response), 0);
}

void remove_user(int client_sock, char *username) {
    FILE *fp = fopen(USER_FILE_PATH, "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_users.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    int user_found = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_password = strtok(NULL, ",");
        char *saved_role = strtok(NULL, "\n");

        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            user_found = 1;
            continue;  // Skip writing this user to the temp file
        } else {
            fprintf(temp_fp, "%s,%s,%s,%s\n", saved_id, saved_username, saved_password, saved_role);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(USER_FILE_PATH);
    rename(temp_filename, USER_FILE_PATH);

    char response[BUF_SIZE];
    if (user_found) {
        sprintf(response, "%s berhasil dihapus", username);
    } else {
        sprintf(response, "User %s tidak ditemukan", username);
    }
    send(client_sock, response, strlen(response), 0);
}

void initialize_files() {
    struct stat st = {0};

    if (stat(USER_FILE_PATH, &st) == -1) {
        FILE *fp = fopen(USER_FILE_PATH, "w");
        if (fp) {
            fprintf(fp, "id_user,name,password,global_role\n");
            fclose(fp);
        }
    }

    if (stat("channels.csv", &st) == -1) {
        FILE *fp = fopen("channels.csv", "w");
        if (fp) {
            fprintf(fp, "id_channel,channel,key\n");
            fclose(fp);
        }
    }

    if (stat("channel1", &st) == -1) {
        mkdir("channel1", 0700);
        mkdir("channel1/admin", 0700);
        mkdir("channel1/room1", 0700);
        mkdir("channel1/room2", 0700);
        mkdir("channel1/room3", 0700);

        FILE *fp = fopen("channel1/admin/auth.csv", "w");
        if (fp) {
            fprintf(fp, "id_user,name,role\n");
            fclose(fp);
        }

        fp = fopen("channel1/admin/user.log", "w");
        if (fp) {
            fclose(fp);
        }

        fp = fopen("channel1/room1/chat.csv", "w");
        if (fp) {
            fprintf(fp, "date,id_chat,sender,chat\n");
            fclose(fp);
        }

        fp = fopen("channel1/room2/chat.csv", "w");
        if (fp) {
            fprintf(fp, "date,id_chat,sender,chat\n");
            fclose(fp);
        }

        fp = fopen("channel1/room3/chat.csv", "w");
        if (fp) {
            fprintf(fp, "date,id_chat,sender,chat\n");
            fclose(fp);
        }
    }

    if (stat("channel2", &st) == -1) {
        mkdir("channel2", 0700);
        mkdir("channel2/admin", 0700);
        mkdir("channel2/room1", 0700);
        mkdir("channel2/room2", 0700);
        mkdir("channel2/room3", 0700);

        FILE *fp = fopen("channel2/admin/auth.csv", "w");
        if (fp) {
            fprintf(fp, "id_user,name,role\n");
            fclose(fp);
        }

        fp = fopen("channel2/admin/user.log", "w");
        if (fp) {
            fclose(fp);
        }

        fp = fopen("channel2/room1/chat.csv", "w");
        if (fp) {
            fprintf(fp, "date,id_chat,sender,chat\n");
            fclose(fp);
        }

        fp = fopen("channel2/room2/chat.csv", "w");
        if (fp) {
            fprintf(fp, "date,id_chat,sender,chat\n");
            fclose(fp);
        }

        fp = fopen("channel2/room3/chat.csv", "w");
        if (fp) {
            fprintf(fp, "date,id_chat,sender,chat\n");
            fclose(fp);
        }
    }
}

int is_user_banned(const char *username, const char *channel) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        return 0;  // If the file doesn't exist, the user is not banned
    }

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");
        char *id_user = token;
        char *name = strtok(NULL, ",");
        char *role = strtok(NULL, "\n");

        if (name && strcmp(name, username) == 0 && role && strcmp(role, "BANNED") == 0) {
            fclose(file);
            return 1;  // User is banned
        }
    }

    fclose(file);
    return 0;  // User is not banned
}

void create_channel(int client_sock, char *username, char *channel, char *key) {
    // Remove "-k" prefix from key if present
    if (strncmp(key, "-k ", 3) == 0) {
        key += 3;
    }

    FILE *fp = fopen("channels.csv", "a+");
    if (!fp) {
        perror("File open error");
        return;
    }

    FILE *users_fp = fopen(USER_FILE_PATH, "r");
    if (!users_fp) {
        perror("File open error");
        fclose(fp);
        return;
    }

    char line[BUF_SIZE];
    int channel_exists = 0;
    int max_channel_id = 0;
    int user_id = -1;

    // Find the user ID from users.csv
    while (fgets(line, sizeof(line), users_fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            user_id = atoi(saved_id);
            break;
        }
    }
    fclose(users_fp);

    if (user_id == -1) {
        char response[BUF_SIZE];
        sprintf(response, "User %s tidak ditemukan", username);
        send(client_sock, response, strlen(response), 0);
        fclose(fp);
        return;
    }

    // Check for existing channel and find the max channel ID
    fseek(fp, 0, SEEK_SET);
    while (fgets(line, sizeof(line), fp)) {
        char *saved_id = strtok(line, ",");
        char *saved_channel = strtok(NULL, ",");
        if (saved_channel != NULL && strcmp(saved_channel, channel) == 0) {
            channel_exists = 1;
            break;
        }
        int current_id = atoi(saved_id);
        if (current_id > max_channel_id) {
            max_channel_id = current_id;
        }
    }

    if (channel_exists) {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s sudah ada", channel);
        send(client_sock, response, strlen(response), 0);
    } else {
        int new_channel_id = max_channel_id + 1;
        fseek(fp, 0, SEEK_END);

        char salt[BCRYPT_HASHSIZE];
        char hashed_key[BCRYPT_HASHSIZE];

        if (bcrypt_gensalt(12, salt) != 0 || bcrypt_hashpw(key, salt, hashed_key) != 0) {
            perror("Key encryption error");
            fclose(fp);
            return;
        }

        fprintf(fp, "%d,%s,%s\n", new_channel_id, channel, hashed_key);

        // Create the necessary directories and files
        char path[BUF_SIZE];
        sprintf(path, "/home/azrael/sisop/finalproject/%s/admin", channel);
        if (mkdir(path, 0700) != 0) {
            if (errno == ENOENT) {
                char parent_path[BUF_SIZE];
                sprintf(parent_path, "/home/azrael/sisop/finalproject/%s", channel);
                mkdir(parent_path, 0700);
                if (mkdir(path, 0700) != 0) {
                    perror("Directory creation error for admin directory");
                }
            } else {
                perror("Directory creation error for admin directory");
            }
        }
        sprintf(path, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
        FILE *auth_fp = fopen(path, "w");
        if (auth_fp) {
            fprintf(auth_fp, "id_user,name,role\n%d,%s,ADMIN\n", user_id, username);
            fclose(auth_fp);
        } else {
            perror("File creation error for auth.csv");
        }
        sprintf(path, "/home/azrael/sisop/finalproject/%s/admin/user.log", channel);
        FILE *log_fp = fopen(path, "w");
        if (log_fp) {
            fclose(log_fp);
        } else {
            perror("File creation error for user.log");
        }

        char response[BUF_SIZE];
        sprintf(response, "Channel %s dibuat", channel);
        send(client_sock, response, strlen(response), 0);
    }

    fclose(fp);
}

void edit_channel(int client_sock, char *old_channel, char *new_channel) {
    FILE *fp = fopen("channels.csv", "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_channels.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *id_channel = strtok(line, ",");
        char *channel = strtok(NULL, ",");
        char *key = strtok(NULL, "\n");

        if (strcmp(channel, old_channel) == 0) {
            fprintf(temp_fp, "%s,%s,%s\n", id_channel, new_channel, key);
            found = 1;
        } else {
            fprintf(temp_fp, "%s,%s,%s\n", id_channel, channel, key);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove("channels.csv");
    rename(temp_filename, "channels.csv");

    if (found) {
        char old_path[BUF_SIZE], new_path[BUF_SIZE];
        sprintf(old_path, "/home/azrael/sisop/finalproject/%s", old_channel);
        sprintf(new_path, "/home/azrael/sisop/finalproject/%s", new_channel);
        rename(old_path, new_path);

        char response[BUF_SIZE];
        sprintf(response, "%s berhasil diubah menjadi %s", old_channel, new_channel);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s tidak ditemukan", old_channel);
        send(client_sock, response, strlen(response), 0);
    }
}

void delete_channel(int client_sock, char *channel, const char *admin_username) {
    FILE *fp = fopen("channels.csv", "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_channels.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        char *id_channel = strtok(line, ",");
        char *saved_channel = strtok(NULL, ",");
        char *key = strtok(NULL, "\n");

        if (strcmp(saved_channel, channel) == 0) {
            found = 1;
        } else {
            fprintf(temp_fp, "%s,%s,%s\n", id_channel, saved_channel, key);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove("channels.csv");
    rename(temp_filename, "channels.csv");

    if (found) {
        delete_directory(channel);

        char response[BUF_SIZE];
        sprintf(response, "Channel %s berhasil dihapus", channel);
        send(client_sock, response, strlen(response), 0);

        // Log the deletion
        log_user_activity("[%s] %s deleted channel %s\n", admin_username, admin_username, channel);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s tidak ditemukan", channel);
        send(client_sock, response, strlen(response), 0);
    }
}

void delete_directory(const char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            char entry_path[BUF_SIZE];
            sprintf(entry_path, "%s/%s", path, entry->d_name);

            if (entry->d_type == DT_DIR) {
                if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                    delete_directory(entry_path);
                }
            } else {
                remove(entry_path);
            }
        }
        closedir(dir);
        rmdir(path);
    }
}

void create_room(int client_sock, const char *channel, const char *room, const char *admin_username) {
    char path[BUF_SIZE];
    sprintf(path, "%s/%s/%s", BASE_DIR, channel, room);

    if (mkdir(path, 0700) == 0) {
        // Create chat.csv in the new room
        char chat_file[BUF_SIZE];
        sprintf(chat_file, "%s/chat.csv", path);
        FILE *chat_fp = fopen(chat_file, "w");
        if (chat_fp) {
            fprintf(chat_fp, "date,id_chat,sender,chat\n");
            fclose(chat_fp);
        }

        char response[BUF_SIZE];
        sprintf(response, "Room %s dibuat", room);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "Room %s sudah ada", room);
        send(client_sock, response, strlen(response), 0);
    }

    log_user_activity("[%s] %s created room %s in channel %s\n", admin_username, admin_username, room, channel);
}

void edit_room(int client_sock, char *channel, char *old_room, char *new_room) {
    char old_path[BUF_SIZE], new_path[BUF_SIZE];
    sprintf(old_path, "%s/%s/%s", BASE_DIR, channel, old_room);
    sprintf(new_path, "%s/%s/%s", BASE_DIR, channel, new_room);

    // Debugging information
    printf("Renaming room from: %s to: %s\n", old_path, new_path);

    // Check if the old room exists
    if (access(old_path, F_OK) != 0) {
        perror("Old room does not exist");
        char response[BUF_SIZE];
        sprintf(response, "Room %s tidak ditemukan", old_room);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    // Perform the rename operation
    if (rename(old_path, new_path) == 0) {
        char response[BUF_SIZE];
        sprintf(response, "Room %s berhasil diubah menjadi %s", old_room, new_room);
        send(client_sock, response, strlen(response), 0);
    } else {
        perror("Error renaming room");
        char response[BUF_SIZE];
        sprintf(response, "Room %s tidak ditemukan", old_room);
        send(client_sock, response, strlen(response), 0);
    }
}

void delete_room(int client_sock, char *channel, char *room) {
    char path[BUF_SIZE];
    sprintf(path, "%s/%s/%s", BASE_DIR, channel, room);

    DIR *dir = opendir(path);
    if (dir) {
        // Directory exists, remove it
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            char file_path[BUF_SIZE];
            sprintf(file_path, "%s/%s", path, entry->d_name);

            if (entry->d_type == DT_REG) {
                if (remove(file_path) != 0) {
                    perror("File removal error");
                    closedir(dir);
                    return;
                }
            }
        }
        closedir(dir);

        if (rmdir(path) == 0) {
            char response[BUF_SIZE];
            sprintf(response, "Room %s berhasil dihapus", room);
            send(client_sock, response, strlen(response), 0);
        } else {
            perror("Directory removal error");
        }
    } else if (errno == ENOENT) {
        // Directory does not exist
        char response[BUF_SIZE];
        sprintf(response, "Room %s tidak ditemukan", room);
        send(client_sock, response, strlen(response), 0);
    } else {
        perror("Directory open error");
    }
}

void delete_all_rooms(int client_sock, char *channel, const char *admin_username) {
    char path[BUF_SIZE];
    sprintf(path, "%s/%s", BASE_DIR, channel);
    DIR *dir = opendir(path);
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_DIR && strcmp(entry->d_name, "admin") != 0 &&
                strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                char room_path[BUF_SIZE];
                sprintf(room_path, "%s/%s", path, entry->d_name);
                delete_directory(room_path);
            }
        }
        closedir(dir);
    }

    char response[BUF_SIZE];
    sprintf(response, "Semua room dihapus");
    send(client_sock, response, strlen(response), 0);

    // Log the deletion
    log_user_activity("[%s] %s deleted all rooms in channel %s\n", admin_username, admin_username, channel);
}

void ban_user(int client_sock, const char *channel, const char *username, const char *admin_username) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);

    FILE *file = fopen(filepath, "r+");
    if (!file) {
        char response[BUF_SIZE];
        sprintf(response, "File open error: %s\n", filepath);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char temp_filepath[BUF_SIZE];
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s/admin/auth_temp.csv", BASE_DIR, channel);
    FILE *temp_file = fopen(temp_filepath, "w");
    if (!temp_file) {
        fclose(file);
        char response[BUF_SIZE];
        sprintf(response, "File open error: %s\n", temp_filepath);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char line[BUF_SIZE];
    int user_banned = 0;
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");
        char *id_user = token;
        char *name = strtok(NULL, ",");
        char *role = strtok(NULL, "\n");

        if (name && strcmp(name, username) == 0) {
            fprintf(temp_file, "%s,%s,BANNED\n", id_user, name);
            user_banned = 1;
        } else {
            fprintf(temp_file, "%s,%s,%s\n", id_user, name, role);
        }
    }

    fclose(file);
    fclose(temp_file);

    // Replace original file with updated file
    remove(filepath);
    rename(temp_filepath, filepath);

    if (user_banned) {
        char response[BUF_SIZE];
        sprintf(response, "user %s diban\n", username);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "User %s tidak ditemukan\n", username);
        send(client_sock, response, strlen(response), 0);
    }

    log_user_activity("[%s] %s banned user %s in channel %s\n", admin_username, admin_username, username, channel);
}

void unban_user(int client_sock, const char *channel, const char *username, const char *admin_username) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);

    FILE *file = fopen(filepath, "r+");
    if (!file) {
        char response[BUF_SIZE];
        sprintf(response, "File open error: %s\n", filepath);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char temp_filepath[BUF_SIZE];
    snprintf(temp_filepath, sizeof(temp_filepath), "%s/%s/admin/auth_temp.csv", BASE_DIR, channel);
    FILE *temp_file = fopen(temp_filepath, "w");
    if (!temp_file) {
        fclose(file);
        char response[BUF_SIZE];
        sprintf(response, "File open error: %s\n", temp_filepath);
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char line[BUF_SIZE];
    int user_unbanned = 0;
    while (fgets(line, sizeof(line), file)) {
        char *token = strtok(line, ",");
        char *id_user = token;
        char *name = strtok(NULL, ",");
        char *role = strtok(NULL, "\n");

        if (name && strcmp(name, username) == 0) {
            fprintf(temp_file, "%s,%s,USER\n", id_user, name);
            user_unbanned = 1;
        } else {
            fprintf(temp_file, "%s,%s,%s\n", id_user, name, role);
        }
    }

    fclose(file);
    fclose(temp_file);

    // Replace original file with updated file
    remove(filepath);
    rename(temp_filepath, filepath);

    if (user_unbanned) {
        char response[BUF_SIZE];
        sprintf(response, "user %s kembali\n", username);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "User %s tidak ditemukan\n", username);
        send(client_sock, response, strlen(response), 0);
    }

    log_user_activity("[%s] %s unbanned user %s in channel %s\n", admin_username, admin_username, username, channel);
}

void remove_user_from_channel(int client_sock, const char *channel, const char *username, const char *admin_username) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        char response[BUF_SIZE];
        sprintf(response, "Error removing user from channel\n");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char temp_filename[BUF_SIZE];
    sprintf(temp_filename, "/home/azrael/sisop/finalproject/%s/admin/temp_auth.csv", channel);
    FILE *temp_fp = fopen(temp_filename, "w");
    if (!temp_fp) {
        perror("File open error");
        char response[BUF_SIZE];
        sprintf(response, "Error removing user from channel\n");
        send(client_sock, response, strlen(response), 0);
        fclose(fp);
        return;
    }

    char line[BUF_SIZE];
    int user_removed = 0;
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        char *saved_id = strtok(line, ",");
        char *saved_username = strtok(NULL, ",");
        char *saved_role = strtok(NULL, "\n");

        if (strcmp(saved_username, username) != 0) {
            fprintf(temp_fp, "%s,%s,%s\n", saved_id, saved_username, saved_role);
        } else {
            user_removed = 1;
        }
    }
    fclose(fp);
    fclose(temp_fp);

    remove(filename);
    rename(temp_filename, filename);

    if (user_removed) {
        char response[BUF_SIZE];
        sprintf(response, "user %s dikick", username);
        send(client_sock, response, strlen(response), 0);

        // Log the removal
        log_user_activity("[%s] %s removed user %s from channel %s\n", admin_username, admin_username, username, channel);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "user %s tidak ditemukan di channel", username);
        send(client_sock, response, strlen(response), 0);
    }
}

int is_channel_authenticated(AuthenticatedChannel *auth_channels, char *channel) {
    AuthenticatedChannel *current = auth_channels;
    while (current != NULL) {
        if (strcmp(current->channel_name, channel) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

void chat_message(int client_sock, const char *channel, const char *room, const char *username, const char *message) {
    char chat_file[BUF_SIZE];
    snprintf(chat_file, sizeof(chat_file), "%s/%s/%s/chat.csv", BASE_DIR, channel, room);

    FILE *fp = fopen(chat_file, "a+");
    if (!fp) {
        perror("File open error");
        return;
    }

    // Determine the next chat ID
    int chat_id = 1; // Start chat IDs from 1
    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        int current_id;
        sscanf(line, "[%*[^]]][%d]", &current_id);
        if (current_id >= chat_id) {
            chat_id = current_id + 1;
        }
    }

    // Get the current timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char date[BUF_SIZE];
    strftime(date, sizeof(date), "%d/%m/%Y %H:%M:%S", t);

    // Append the new message
    fprintf(fp, "[%s][%d][%s] \"%s\"\n", date, chat_id, username, message);
    fclose(fp);

    // Send confirmation to the client
    char response[BUF_SIZE];
    snprintf(response, sizeof(response), "Message sent");
    send(client_sock, response, strlen(response), 0);

    // Debug information
    printf("Chatting in channel %s room %s: %s\n", channel, room, message);
    printf("New chat ID: %d\n", chat_id);
}

void see_chat(int client_sock, const char *channel, const char *room) {
    char chat_file[BUF_SIZE];
    snprintf(chat_file, sizeof(chat_file), "%s/%s/%s/chat.csv", BASE_DIR, channel, room);

    FILE *fp = fopen(chat_file, "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    int has_messages = 0;

    // Prepare response buffer
    char response[BUF_SIZE * 10] = {0}; // Increase the buffer size if needed
    char *response_ptr = response;

    // Read each line and append to the response buffer
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "date,id_chat,sender,chat") != NULL) {
            continue; // Skip header line if it exists
        }

        has_messages = 1; // Set flag to indicate there are messages

        // Remove newline character from the line
        line[strcspn(line, "\n")] = 0;

        // Append the line to the response buffer
        response_ptr += sprintf(response_ptr, "%s\n", line);
    }

    fclose(fp);

    // Check if there were any messages
    if (!has_messages) {
        snprintf(response, sizeof(response), "Tidak ada Chat satupun, mulailah chat bersama-sama!");
    }

    send(client_sock, response, strlen(response), 0);

    // Debug information
    printf("Seeing chat in channel %s room %s\n", channel, room);
}

void edit_chat(int client_sock, const char *channel, const char *room, int chat_id, const char *new_message, const char *username) {
    char chat_file_path[BUF_SIZE];
    snprintf(chat_file_path, sizeof(chat_file_path), "/home/azrael/sisop/finalproject/%s/%s/chat.csv", channel, room);

    FILE *fp = fopen(chat_file_path, "r");
    if (!fp) {
        perror("Error opening chat file");
        char response[BUF_SIZE];
        sprintf(response, "Failed to open chat file for editing");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char temp_file_path[BUF_SIZE];
    snprintf(temp_file_path, sizeof(temp_file_path), "/home/azrael/sisop/finalproject/%s/%s/chat_temp.csv", channel, room);

    FILE *temp_fp = fopen(temp_file_path, "w");
    if (!temp_fp) {
        perror("Error opening temporary chat file");
        fclose(fp);
        char response[BUF_SIZE];
        sprintf(response, "Failed to open temporary chat file for editing");
        send(client_sock, response, strlen(response), 0);
        return;
    }

    char line[BUF_SIZE];
    char date[20];
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        int id;
        char sender[50];
        char message[BUF_SIZE];

        if (sscanf(line, "[%19[^]]][%d][%49[^]]][^\n]", date, &id, sender) == 3) {
            char *msg_start = strchr(line, ']') + 2;  // Find the start of the message
            if (msg_start) {
                strcpy(message, msg_start);
                if (id == chat_id) {
                    fprintf(temp_fp, "[%s][%d][%s] \"%s\"\n", date, id, username, new_message);
                    found = 1;
                } else {
                    fprintf(temp_fp, "[%s][%d][%s] %s", date, id, sender, message);
                }
            }
        } else {
            fprintf(temp_fp, "%s", line);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    if (found) {
        remove(chat_file_path);
        rename(temp_file_path, chat_file_path);

        char response[BUF_SIZE];
        sprintf(response, "Message edited");
        send(client_sock, response, strlen(response), 0);
    } else {
        remove(temp_file_path);

        char response[BUF_SIZE];
        sprintf(response, "Message with ID %d not found", chat_id);
        send(client_sock, response, strlen(response), 0);
    }
}

void delete_chat(int client_sock, const char *channel, const char *room, int chat_id) {
    char chat_file[BUF_SIZE];
    snprintf(chat_file, sizeof(chat_file), "%s/%s/%s/chat.csv", BASE_DIR, channel, room);
    char temp_file[BUF_SIZE];
    snprintf(temp_file, sizeof(temp_file), "%s/%s/%s/temp_chat.csv", BASE_DIR, channel, room);

    FILE *fp = fopen(chat_file, "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    FILE *temp_fp = fopen(temp_file, "w");
    if (!temp_fp) {
        perror("Temp file open error");
        fclose(fp);
        return;
    }

    char line[BUF_SIZE];
    int found = 0;

    while (fgets(line, sizeof(line), fp)) {
        int current_id;
        sscanf(line, "[%*[^]]][%d]", &current_id);
        if (current_id != chat_id) {
            fputs(line, temp_fp);
        } else {
            found = 1;
        }
    }

    fclose(fp);
    fclose(temp_fp);

    if (found) {
        rename(temp_file, chat_file);
        char response[BUF_SIZE];
        snprintf(response, sizeof(response), "Message deleted");
        send(client_sock, response, strlen(response), 0);
    } else {
        remove(temp_file);
        char response[BUF_SIZE];
        snprintf(response, sizeof(response), "Message ID not found");
        send(client_sock, response, strlen(response), 0);
    }

    // Debug information
    printf("Deleting chat %d in channel %s room %s\n", chat_id, channel, room);
}

void log_user_activity(const char *format, ...) {
    va_list args;
    va_start(args, format);

    FILE *log_fp = fopen("/home/azrael/sisop/finalproject/users.log", "a");
    if (log_fp) {
        // Get the current timestamp
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char date[BUF_SIZE];
        strftime(date, sizeof(date), "%d/%m/%Y %H:%M:%S", t);

        // Write the timestamp to the log
        fprintf(log_fp, "[%s] ", date);

        // Write the formatted message to the log
        vfprintf(log_fp, format, args);

        fclose(log_fp);
    }

    va_end(args);
}
