# Sisop-FP-2024-MH-IT09

| Nama          | NRP          |
| ------------- | ------------ |
| Kevin Anugerah Faza | 5027231027 |
| Muhammad Hildan Adiwena | 5027231077 |
| Nayyara Ashila | 5027231083 |

## SOAL

### A. Autentikasi
- Setiap user harus memiliki username dan password untuk mengakses DiscorIT. Username, password, dan global role disimpan dalam file user.csv.
- Jika tidak ada user lain dalam sistem, user pertama yang mendaftar otomatis mendapatkan role "ROOT". Username harus bersifat unique dan password wajib di encrypt menggunakan menggunakan bcrypt.

1. Register
   Ini adalah code untuk register
   ```
   void register_user(char *username, char *password, int client_sock) {
    FILE *fp = fopen(USER_FILE_PATH, "a+");
    if (!fp) {
        perror("File open error");
        return;
   ```
  
3. Login
   Ini adalah code untuk login
   ```
   void login_user(char *username, char *password, int client_sock) {
    FILE *fp = fopen(USER_FILE_PATH, "r");
    if (!fp) {
        perror("File open error");
        return;
   ```

Apabila program dijalankan maka akan menampilkan seperti gambar berikut ini 

![Screenshot_2024-06-28_21-25-44](https://github.com/HeavenlySpectre/Sisop-FP-2024-MH-IT09/assets/80327619/ae989240-d9fb-4d07-b213-cca2ed6d136f)


Untuk melakukan register, kita bisa menggunakan command `./discorit REGISTER username -p password` kemudian apabila berhasil maka akan menampilkan pesan bahwa akun tersebut telah berhasil di register
Untuk melakukan Login, kita bisa menggunakan command `./discorit LOGIN nayya -p password` kemudian apabila berhasil maka akan menampilkan pesan bahwa akun tersebut telah berhasil login.

### B. Bagaimana DiscorIT digunakan

1. List Channel dan Room
- Setelah user dapat melihat daftar channel yang tersedia.
  
- Setelah Masuk Channel user dapat melihat list room dan pengguna dalam channel tersebut

2. Akses Channel dan Room
   ini fungsi untuk join room
   ```
   else { // Joining a room
                printf("Attempting to join room: %s in channel: %s\n", name, current_channel); // Debug info
                strcpy(current_room, name);
                join_room(client_sock, current_channel, name, username, global_role);
                char response[BUF_SIZE];
                sprintf(response, "[%s/%s/%s]", username, current_channel, current_room);
                send(client_sock, response, strlen(response), 0);
            }
        }
   ```
- Akses channel admin dan root
- Ketika user pertama kali masuk ke channel, mereka memiliki akses terbatas. Jika mereka telah masuk sebelumnya, akses mereka meningkat sesuai dengan admin dan root.
- User dapat masuk ke room setelah bergabung dengan channel.

3. Fitur Chat
   ini adalah fungsi untuk fitur chat
   ```
   else if (strncmp(buffer, "CHAT ", 5) == 0) {
                char *message = strtok(buffer + 5, "\"");
                printf("Chatting in channel %s room %s: %s\n", current_channel, current_room, message);  // Debug info
                chat_message(client_sock, current_channel, current_room, username, message);
   ```
   ini adalah fungsi untuk melihat chat
   ```
   else if (strncmp(buffer, "SEE CHAT", 8) == 0) {
                printf("Seeing chat in channel %s room %s\n", current_channel, current_room);  // Debug info
                see_chat(client_sock, current_channel, current_room);
   ```
   ini adalah fungsi untuk edit chat
   ```
   else if (strncmp(buffer, "EDIT CHAT ", 10) == 0) {
                int chat_id = atoi(strtok(buffer + 10, " "));
                char *new_message = strtok(NULL, "\"");
                printf("Editing chat %d in channel %s room %s\n", chat_id, current_channel, current_room);  // Debug info
                edit_chat(client_sock, current_channel, current_room, chat_id, new_message, username);
   ```
   ini adalah fungsi untuk delete chat
   ```
   else if (strncmp(buffer, "DEL CHAT ", 9) == 0) {
                int chat_id = atoi(strtok(buffer + 9, "\n"));
                printf("Deleting chat %d in channel %s room %s\n", chat_id, current_channel, current_room);  // Debug info
                delete_chat(client_sock, current_channel, current_room, chat_id);
   ```
   ini adalah fungsi untuk remove chat
   ```
   else if (strncmp(buffer, "REMOVE", 6) == 0 && strcmp(global_role, "ROOT") == 0) {
                printf("Command recognized as REMOVE for ROOT\n"); // Debug info
                char *target = strtok(buffer + 7, "\n");
                printf("Removing user: %s\n", target); // Debug info
                remove_user(client_sock, target);
                char response[BUF_SIZE];
                sprintf(response, "User %s dihapus\n[%s]", target, username);
                send(client_sock, response, strlen(response), 0);
   ```
   
Setiap user dapat mengirim pesan dalam chat. ID pesan chat dimulai dari 1 dan semua pesan disimpan dalam file chat.csv. User dapat melihat pesan-pesan chat yang ada dalam room. Serta user dapat edit dan delete pesan yang sudah dikirim dengan menggunakan ID pesan.

### C. Root
- Akun yang pertama kali mendaftar otomatis mendapatkan peran "root".
- Root dapat masuk ke channel manapun tanpa key dan create, update, dan delete pada channel dan room, mirip dengan admin [D].
- Root memiliki kemampuan khusus untuk mengelola user, seperti: list, edit, dan Remove.

### D. Admin Channel
- Setiap user yang membuat channel otomatis menjadi admin di channel tersebut. Informasi tentang user disimpan dalam file auth.csv.
- Admin dapat create, update, dan delete pada channel dan room, serta dapat remove, ban, dan unban user di channel mereka.

1. Channel
   ini adalah fungsi untuk channel
   ```
   void create_channel(int client_sock, char *username, char *channel, char *key) {
    // Remove "-k" prefix from key if present
    if (strncmp(key, "-k ", 3) == 0) {
        key += 3;
    }
   ```
   
   ini adalah fungsi untuk edit channel
   ```
   void edit_channel(int client_sock, char *old_channel, char *new_channel) {
    FILE *fp = fopen("channels.csv", "r+");
    if (!fp) {
        perror("File open error");
        return;
    }
   ```
   
   ini adalah fungsi untuk delete channel
   ```
   void delete_channel(int client_sock, char *channel, const char *admin_username) {
    FILE *fp = fopen("channels.csv", "r+");
    if (!fp) {
        perror("File open error");
        return;
   ```

   
Informasi tentang semua channel disimpan dalam file channel.csv. Semua perubahan dan aktivitas user pada channel dicatat dalam file users.log.

3. Room
   ini adalah fungsi untuk create room
   ```
   void create_room(int client_sock, const char *channel, const char *room, const char *admin_username) {
    char path[BUF_SIZE];
    sprintf(path, "%s/%s/%s", BASE_DIR, channel, room);
   ```

   ini adalah fungsi untuk edit room
   ```
   void edit_room(int client_sock, char *channel, char *old_room, char *new_room) {
    char old_path[BUF_SIZE], new_path[BUF_SIZE];
    sprintf(old_path, "%s/%s/%s", BASE_DIR, channel, old_room);
    sprintf(new_path, "%s/%s/%s", BASE_DIR, channel, new_room);
   ```

   ini adalah fungsi untuk delete room
   ```
   void delete_room(int client_sock, char *channel, char *room) {
    char path[BUF_SIZE];
    sprintf(path, "%s/%s/%s", BASE_DIR, channel, room);
   ```
   
Semua perubahan dan aktivitas user pada room dicatat dalam file users.log.

4. Ban
   ini adalah fungsi untuk Ban
   ```
   void ban_user(int client_sock, const char *channel, const char *username, const char *admin_username) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);
   ```
Admin dapat melakukan ban pada user yang nakal. Aktivitas ban tercatat pada users.log. Ketika di ban, role "user" berubah menjadi "banned". Data tetap tersimpan dan user tidak dapat masuk ke dalam channel.

5. Unban
   ini fungsi untuk unban
   ```
   void unban_user(int client_sock, const char *channel, const char *username, const char *admin_username) {
    char filepath[BUF_SIZE];
    snprintf(filepath, sizeof(filepath), "%s/%s/admin/auth.csv", BASE_DIR, channel);
   ```
Admin dapat melakukan unban pada user yang sudah berperilaku baik. Aktivitas unban tercatat pada users.log. Ketika di unban, role "banned" berubah kembali menjadi "user" dan dapat masuk ke dalam channel.

6. Remove user
   ini fungsi untuk remove user
   ```
   void remove_user_from_channel(int client_sock, const char *channel, const char *username, const char *admin_username) {
    char filename[BUF_SIZE];
    sprintf(filename, "/home/azrael/sisop/finalproject/%s/admin/auth.csv", channel);
   ```
Admin dapat remove user dan tercatat pada users.log.


### E. User
User dapat mengubah informasi profil mereka, user yang di ban tidak dapat masuk kedalam channel dan dapat keluar dari room, channel, atau keluar sepenuhnya dari DiscorIT.

1. Edit User Username
   
2. Edit User Password

3. Banned user

4. Exit


### F. Error Handling
Jika ada command yang tidak sesuai penggunaannya. Maka akan mengeluarkan pesan error dan tanpa keluar dari program client.


### G. Monitor
- User dapat menampilkan isi chat secara real-time menggunakan program monitor. Jika ada perubahan pada isi chat, perubahan tersebut akan langsung ditampilkan di terminal.
- Sebelum dapat menggunakan monitor, pengguna harus login terlebih dahulu dengan cara yang mirip seperti login di DiscorIT.
- Untuk keluar dari room dan menghentikan program monitor dengan perintah "EXIT".
- Monitor dapat digunakan untuk menampilkan semua chat pada room, mulai dari chat pertama hingga chat yang akan datang nantinya.

 ini adalah code untuk monitor
  ```
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
```
monitor.c berfungsi untuk menenampilkan monitor dari DiscorIT


DiscorIT.
ini adalah code untuk DiscorIT
```
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
```
discorit.c berfungsi untuk menjalankan program dari fungsi-fungsi di server.c














