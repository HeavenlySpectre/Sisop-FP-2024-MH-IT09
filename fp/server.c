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
#include <bcrypt.h>

#define PORT 8080
#define BUF_SIZE 1024

const char *USER_FILE_PATH = "/home/azrael/sisop/finalproject/users.csv";

void *handle_client(void *arg);
void register_user(char *username, char *password, int client_sock);
void login_user(char *username, char *password, int client_sock);
void handle_post_login_commands(int client_sock, char *username, char *role);
void list_channels(int client_sock);
void list_rooms(int client_sock, char *channel);
void list_users(int client_sock, char *channel);
void list_all_users(int client_sock);
void join_channel(int client_sock, char *channel, char *username, char *role);
void join_room(int client_sock, char *channel, char *room, char *username, char *role);
void edit_user(int client_sock, char *old_username, char *new_username);
void change_password(int client_sock, char *username, char *new_password);
void remove_user(int client_sock, char *username);
void initialize_files();
int is_user_banned(char *username, char *channel);
void create_channel(int client_sock, char *username, char *channel, char *key);
void edit_channel(int client_sock, char *old_channel, char *new_channel);
void delete_channel(int client_sock, char *channel);
void create_room(int client_sock, char *channel, char *room);
void edit_room(int client_sock, char *channel, char *old_room, char *new_room);
void delete_room(int client_sock, char *channel, char *room);
void delete_all_rooms(int client_sock, char *channel);
void ban_user(int client_sock, char *channel, char *user);
void unban_user(int client_sock, char *channel, char *user);
void remove_user_from_channel(int client_sock, char *channel, char *user);

int main() {
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

void handle_post_login_commands(int client_sock, char *username, char *role) {
    char buffer[BUF_SIZE] = {0};
    int bytes_received;

    printf("User %s logged in with role: %s\n", username, role);  // Debug info

    char current_channel[BUF_SIZE] = {0};  // To keep track of the current channel
    char current_room[BUF_SIZE] = {0};     // To keep track of the current room

    while ((bytes_received = read(client_sock, buffer, BUF_SIZE)) > 0) {
        buffer[bytes_received] = '\0';
        printf("Received command: %s\n", buffer);  // Debug info
        printf("Role: %s\n", role); // Debug info

        if (strcmp(buffer, "LIST CHANNEL\n") == 0) {
            printf("Command recognized as LIST CHANNEL\n"); // Debug info
            list_channels(client_sock);
        } else if (strcmp(buffer, "LIST ROOM\n") == 0) {
            printf("Command recognized as LIST ROOM\n"); // Debug info
            list_rooms(client_sock, current_channel);
        } else if (strcmp(buffer, "LIST USER\n") == 0) {
            if (strcmp(role, "ROOT") == 0) {
                printf("Command recognized as LIST USER for ROOT\n"); // Debug info
                list_all_users(client_sock);
            } else {
                printf("Command recognized as LIST USER in a channel\n"); // Debug info
                list_users(client_sock, current_channel);
            }
        } else if (strncmp(buffer, "CREATE CHANNEL ", 15) == 0) {
            char *channel = strtok(buffer + 15, " ");
            char *key = strtok(NULL, "\n");
            create_channel(client_sock, username, channel, key);
        } else if (strncmp(buffer, "EDIT CHANNEL ", 13) == 0) {
            // Split command by " TO "
            char *old_channel = strtok(buffer + 13, " ");
            strtok(NULL, " ");  // Skip "TO"
            char *new_channel = strtok(NULL, "\n");

            if (new_channel != NULL) {
                // Remove leading spaces
                while (*new_channel == ' ') new_channel++;
                // Remove trailing spaces
                char *end = new_channel + strlen(new_channel) - 1;
                while (end > new_channel && *end == ' ') end--;
                *(end + 1) = '\0';
                edit_channel(client_sock, old_channel, new_channel);
            } else {
                char response[BUF_SIZE];
                sprintf(response, "Format salah. Gunakan: EDIT CHANNEL <old_channel> TO <new_channel>");
                send(client_sock, response, strlen(response), 0);
            }
        } else if (strncmp(buffer, "DEL CHANNEL ", 12) == 0) {
            char *channel = strtok(buffer + 12, "\n");
            delete_channel(client_sock, channel);
        } else if (strncmp(buffer, "CREATE ROOM ", 12) == 0) {
            char *room = strtok(buffer + 12, "\n");
            create_room(client_sock, current_channel, room);
        } else if (strncmp(buffer, "EDIT ROOM ", 10) == 0) {
            char *old_room = strtok(buffer + 10, " ");
            strtok(NULL, " ");  // Skip "TO"
            char *new_room = strtok(NULL, "\n");

            if (new_room != NULL) {
                // Remove leading spaces
                while (*new_room == ' ') new_room++;
                // Remove trailing spaces
                char *end = new_room + strlen(new_room) - 1;
                while (end > new_room && *end == ' ') end--;
                *(end + 1) = '\0';
                edit_room(client_sock, current_channel, old_room, new_room);
            } else {
                char response[BUF_SIZE];
                sprintf(response, "Format salah. Gunakan: EDIT ROOM <old_room> TO <new_room>");
                send(client_sock, response, strlen(response), 0);
            }
        } else if (strncmp(buffer, "DEL ROOM ", 9) == 0) {
            char *room = strtok(buffer + 9, "\n");
            if (strcmp(room, "ALL") == 0) {
                delete_all_rooms(client_sock, current_channel);
            } else {
                delete_room(client_sock, current_channel, room);
            }
        } else if (strncmp(buffer, "BAN ", 4) == 0) {
            char *user = strtok(buffer + 4, "\n");
            ban_user(client_sock, current_channel, user);
        } else if (strncmp(buffer, "UNBAN ", 6) == 0) {
            char *user = strtok(buffer + 6, "\n");
            unban_user(client_sock, current_channel, user);
        } else if (strncmp(buffer, "REMOVE USER ", 12) == 0) {
            char *user = strtok(buffer + 12, "\n");
            remove_user_from_channel(client_sock, current_channel, user);
        } else if (strncmp(buffer, "EDIT PROFILE SELF -u ", 21) == 0) {
            char *new_username = strtok(buffer + 21, "\n");
            edit_user(client_sock, username, new_username);
            sprintf(username, "%s", new_username); // Update the username in the current session
        } else if (strncmp(buffer, "EDIT PROFILE SELF -p ", 21) == 0) {
            char *new_password = strtok(buffer + 21, "\n");
            change_password(client_sock, username, new_password);
        } else if (strncmp(buffer, "REMOVE", 6) == 0 && strcmp(role, "ROOT") == 0) {
            printf("Command recognized as REMOVE for ROOT\n"); // Debug info
            char *target = strtok(buffer + 7, "\n");
            remove_user(client_sock, target);
        } else if (strncmp(buffer, "JOIN ", 5) == 0) {
            printf("Command recognized as JOIN channel or room\n"); // Debug info
            char *name = strtok(buffer + 5, "\n");
            if (current_channel[0] == '\0') {
                // Join a channel if no current channel is set
                if (!is_user_banned(username, name)) {
                    strcpy(current_channel, name);  // Set current channel
                    current_room[0] = '\0';  // Clear current room
                    join_channel(client_sock, name, username, role);
                } else {
                    char response[BUF_SIZE];
                    sprintf(response, "anda telah diban, silahkan menghubungi admin");
                    send(client_sock, response, strlen(response), 0);
                }
            } else {
                // Join a room within the current channel
                strcpy(current_room, name);  // Set current room
                join_room(client_sock, current_channel, name, username, role);
            }
        } else if (strncmp(buffer, "EXIT", 4) == 0) {
            printf("User %s has exited context.\n", username);  // Debug info
            if (current_room[0] != '\0') {
                // Exiting a room
                current_room[0] = '\0';
                char response[BUF_SIZE];
                sprintf(response, "Exiting room...\n");
                send(client_sock, response, strlen(response), 0);
            } else if (current_channel[0] != '\0') {
                // Exiting a channel
                current_channel[0] = '\0';
                char response[BUF_SIZE];
                sprintf(response, "Exiting channel...\n");
                send(client_sock, response, strlen(response), 0);
            } else {
                // Exiting the client session
                char response[BUF_SIZE];
                sprintf(response, "Exiting session...\n");
                send(client_sock, response, strlen(response), 0);
                break;  // Close the connection
            }
        } else {
            printf("[%s] %s\n", username, buffer);  // Echoing back the input
            send(client_sock, buffer, strlen(buffer), 0);
        }
    }

    close(client_sock);  // Close the connection after exit
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
    sprintf(path, "/home/azrael/sisop/finalproject/%s", channel);
    DIR *dir = opendir(path);
    struct dirent *entry;
    char response[BUF_SIZE] = {0};

    if (!dir) {
        perror("File open error");
        return;
    }

    int room_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, "admin") != 0 && strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
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

void list_users(int client_sock, char *channel) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char response[BUF_SIZE] = {0};

    while (fgets(line, sizeof(line), fp)) {
        strcat(response, strtok(line, ","));
        strcat(response, " ");
    }

    send(client_sock, response, strlen(response), 0);
    fclose(fp);
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

void join_channel(int client_sock, char *channel, char *username, char *role) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
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
    while (fgets(line, sizeof(line), fp)) {
        char *channel_id = strtok(line, ",");
        char *channel_name = strtok(NULL, ",");
        char *channel_key = strtok(NULL, ",");

        // Remove trailing newline from channel_key
        channel_key[strcspn(channel_key, "\n")] = 0;

        if (strcmp(channel_name, channel) == 0) {
            channel_found = 1;
            // Check if the user is an admin
            char auth_file[BUF_SIZE];
            sprintf(auth_file, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
            FILE *auth_fp = fopen(auth_file, "r");
            char auth_line[BUF_SIZE];
            int is_admin = 0;
            while (fgets(auth_line, sizeof(auth_line), auth_fp)) {
                char *saved_id = strtok(auth_line, ",");
                char *saved_username = strtok(NULL, ",");
                char *saved_role = strtok(NULL, ",");
                if (saved_username != NULL && strcmp(saved_username, username) == 0 && strcmp(saved_role, "admin\n") == 0) {
                    is_admin = 1;
                    break;
                }
            }
            fclose(auth_fp);

            if (is_admin) {
                char response[BUF_SIZE];
                sprintf(response, "JOIN %s\n", channel);
                send(client_sock, response, strlen(response), 0);
            } else {
                // Verify key for non-admin users
                char response[BUF_SIZE];
                sprintf(response, "Key: ");
                send(client_sock, response, strlen(response), 0);

                int bytes_received = read(client_sock, line, BUF_SIZE);
                line[bytes_received] = '\0';
                line[strcspn(line, "\n")] = 0;  // Remove trailing newline

                printf("Received key: '%s'\n", line);  // Debug info
                printf("Expected key: '%s'\n", channel_key);  // Debug info

                if (strcmp(line, channel_key) == 0) {
                    sprintf(response, "JOIN %s\n", channel);
                    send(client_sock, response, strlen(response), 0);
                } else {
                    sprintf(response, "Invalid key for channel %s\n", channel);
                    send(client_sock, response, strlen(response), 0);
                }
            }
            break;
        }
    }
    fclose(fp);

    if (!channel_found) {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s does not exist\n", channel);
        send(client_sock, response, strlen(response), 0);
    }
}

void join_room(int client_sock, char *channel, char *room, char *username, char *role) {
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

int is_user_banned(char *username, char *channel) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        char *saved_username = strtok(line, ",");
        char *saved_role = strtok(NULL, "\n");

        if (saved_username != NULL && strcmp(saved_username, username) == 0) {
            if (strcmp(saved_role, "banned") == 0) {
                fclose(fp);
                return 1;
            }
            break;
        }
    }

    fclose(fp);
    return 0;
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

    char line[BUF_SIZE];
    int channel_exists = 0;
    int channel_count = 0;

    // Check for existing channel and count channels
    fseek(fp, 0, SEEK_SET);
    while (fgets(line, sizeof(line), fp)) {
        channel_count++;
        char *saved_id = strtok(line, ",");
        char *saved_channel = strtok(NULL, ",");
        if (saved_channel != NULL && strcmp(saved_channel, channel) == 0) {
            channel_exists = 1;
            break;
        }
    }

    if (channel_exists) {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s sudah ada", channel);
        send(client_sock, response, strlen(response), 0);
    } else {
        fseek(fp, 0, SEEK_END);
        fprintf(fp, "%d,%s,%s\n", channel_count + 1, channel, key);

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
            fprintf(auth_fp, "id_user,name,role\n1,%s,admin\n", username);
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

void delete_channel(int client_sock, char *channel) {
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
        char *username = strtok(line, ",");
        char *channel_name = strtok(NULL, ",");
        char *key = strtok(NULL, "\n");

        if (strcmp(channel_name, channel) != 0) {
            fprintf(temp_fp, "%s,%s,%s\n", username, channel_name, key);
        } else {
            found = 1;
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove("channels.csv");
    rename(temp_filename, "channels.csv");

    if (found) {
        char path[BUF_SIZE];
        sprintf(path, "/home/azrael/sisop/finalproject/%s", channel);
        // Use system command to remove directory recursively
        char command[BUF_SIZE];
        sprintf(command, "rm -rf %s", path);
        system(command);

        char response[BUF_SIZE];
        sprintf(response, "channel %s berhasil dihapus", channel);
        send(client_sock, response, strlen(response), 0);
    } else {
        char response[BUF_SIZE];
        sprintf(response, "Channel %s tidak ditemukan", channel);
        send(client_sock, response, strlen(response), 0);
    }
}

void create_room(int client_sock, char *channel, char *room) {
    char path[BUF_SIZE];
    sprintf(path, "/home/azrael/sisop/finalproject/%s/%s", channel, room);

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
}

void edit_room(int client_sock, char *channel, char *old_room, char *new_room) {
    char old_path[BUF_SIZE], new_path[BUF_SIZE];
    sprintf(old_path, "/home/azrael/sisop/finalproject/%s/%s", channel, old_room);
    sprintf(new_path, "/home/azrael/sisop/finalproject/%s/%s", channel, new_room);

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
    sprintf(path, "/home/azrael/sisop/finalproject/%s/%s", channel, room);

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

void delete_all_rooms(int client_sock, char *channel) {
    char path[BUF_SIZE];
    sprintf(path, "/home/azrael/sisop/finalproject/%s", channel);
    DIR *dir = opendir(path);
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR && strcmp(entry->d_name, "admin") != 0 &&
            strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char room_path[BUF_SIZE];
            sprintf(room_path, "%s/%s", path, entry->d_name);
            char command[BUF_SIZE];
            sprintf(command, "rm -rf %s", room_path);
            system(command);
        }
    }

    closedir(dir);

    char response[BUF_SIZE];
    sprintf(response, "Semua room dihapus");
    send(client_sock, response, strlen(response), 0);

    // Log the deletion
    FILE *log_fp = fopen("users.log", "a");
    if (log_fp) {
        fprintf(log_fp, "%s deleted all rooms in channel %s\n", channel);
        fclose(log_fp);
    }
}

void ban_user(int client_sock, char *channel, char *user) {
    char path[BUF_SIZE];
    sprintf(path, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_auth.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    while (fgets(line, sizeof(line), fp)) {
        char *saved_user = strtok(line, ",");
        char *role = strtok(NULL, "\n");

        if (strcmp(saved_user, user) == 0) {
            fprintf(temp_fp, "%s,banned\n", saved_user);
        } else {
            fprintf(temp_fp, "%s,%s\n", saved_user, role);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(path);
    rename(temp_filename, path);

    char response[BUF_SIZE];
    sprintf(response, "user %s diban", user);
    send(client_sock, response, strlen(response), 0);

    // Log the ban
    FILE *log_fp = fopen("users.log", "a");
    if (log_fp) {
        fprintf(log_fp, "%s banned user %s in channel %s\n", channel, user);
        fclose(log_fp);
    }
}

void unban_user(int client_sock, char *channel, char *user) {
    char path[BUF_SIZE];
    sprintf(path, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_auth.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    while (fgets(line, sizeof(line), fp)) {
        char *saved_user = strtok(line, ",");
        char *role = strtok(NULL, "\n");

        if (strcmp(saved_user, user) == 0) {
            fprintf(temp_fp, "%s,user\n", saved_user);
        } else {
            fprintf(temp_fp, "%s,%s\n", saved_user, role);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(path);
    rename(temp_filename, path);

    char response[BUF_SIZE];
    sprintf(response, "user %s kembali", user);
    send(client_sock, response, strlen(response), 0);

    // Log the unban
    FILE *log_fp = fopen("users.log", "a");
    if (log_fp) {
        fprintf(log_fp, "%s unbanned user %s in channel %s\n", channel, user);
        fclose(log_fp);
    }
}

void remove_user_from_channel(int client_sock, char *channel, char *user) {
    char path[BUF_SIZE];
    sprintf(path, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
    FILE *fp = fopen(path, "r+");
    if (!fp) {
        perror("File open error");
        return;
    }

    char line[BUF_SIZE];
    char temp_filename[] = "/home/azrael/sisop/finalproject/temp_auth.csv";
    FILE *temp_fp = fopen(temp_filename, "w");

    while (fgets(line, sizeof(line), fp)) {
        char *saved_user = strtok(line, ",");
        char *role = strtok(NULL, "\n");

        if (strcmp(saved_user, user) != 0) {
            fprintf(temp_fp, "%s,%s\n", saved_user, role);
        }
    }

    fclose(fp);
    fclose(temp_fp);

    remove(path);
    rename(temp_filename, path);

    char response[BUF_SIZE];
    sprintf(response, "user %s dikick", user);
    send(client_sock, response, strlen(response), 0);

    // Log the removal
    FILE *log_fp = fopen("users.log", "a");
    if (log_fp) {
        fprintf(log_fp, "%s removed user %s in channel %s\n", channel, user);
        fclose(log_fp);
    }
}
