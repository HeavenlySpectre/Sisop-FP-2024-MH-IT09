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

![Screenshot_2024-06-28_21-48-42](https://github.com/HeavenlySpectre/Sisop-FP-2024-MH-IT09/assets/80327619/119129a8-cc5a-4d03-886f-8a3c6afb1149)


Untuk melakukan register, kita bisa menggunakan command `./discorit REGISTER username -p password` kemudian apabila berhasil maka akan menampilkan pesan bahwa akun tersebut telah berhasil di register
Untuk melakukan Login, kita bisa menggunakan command `./discorit LOGIN nayya -p password` kemudian apabila berhasil maka akan menampilkan pesan bahwa akun tersebut telah berhasil login.

### B. Bagaimana DiscorIT digunakan

1. List Channel dan Room
- Setelah user dapat melihat daftar channel yang tersedia.
  
- Setelah Masuk Channel user dapat melihat list room dan pengguna dalam channel tersebut

2. Akses Channel dan Room
   ini code yang kami gunakan untuk join room
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
Apabila berhasil join channel maka akan menampilkan seperti gambar berikut ini
   
![Screenshot_2024-06-28_22-11-22](https://github.com/HeavenlySpectre/Sisop-FP-2024-MH-IT09/assets/80327619/58982d0b-7ffb-4f31-803c-aed9e3b74fde)

Apabila berhasil join room maka akan menampilkan seperti gambar berikut ini
   
![Screenshot_2024-06-28_22-14-31](https://github.com/HeavenlySpectre/Sisop-FP-2024-MH-IT09/assets/80327619/fe35e7da-fcc3-4dc5-aa97-6f064f4775bc)

   

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

Apabila dijalankan dengan command `./monitor LOGIN username -p password` maka akan menampilkan seperti gambar dibawah ini 


![WhatsApp Image 2024-06-28 at 1 36 36 PM](https://github.com/HeavenlySpectre/Sisop-FP-2024-MH-IT09/assets/80327619/b544dbff-6c04-4b62-ad3f-f77289e6fc20)

Berikut penjelasan untuk code yang kami gunakan

Struct: Digunakan untuk mengirim argumen ke thread monitor chat, termasuk socket descriptor, username, nama channel, dan nama room.
```
typedef struct {
    int sockfd;
    char username[BUF_SIZE];
    char channel[BUF_SIZE];
    char room[BUF_SIZE];
} monitor_args_t;
```

Main Function: Mengatur proses utama dari aplikasi, termasuk koneksi socket, login pengguna, memilih channel dan room, membaca chat history yang ada, dan membuat thread untuk memonitor chat.
```
int main(int argc, char *argv[]) {
    // Proses parsing argumen dan inisialisasi socket

    // Melakukan login pengguna ke server
    login_user(sockfd, username, password);

    // Meminta input channel dan room dari pengguna
    // Membaca chat history yang sudah ada dari file chat.csv
    // Membuat thread untuk memonitor chat room

    // Loop untuk menerima input dan memproses pesan chat atau exit
}
```

Login Function: Mengirimkan data login ke server menggunakan socket dan membaca balasan dari server untuk memverifikasi login.
```
void login_user(int sockfd, char *username, char *password) {
    // Mengirim pesan login ke server
    // Membaca balasan dari server untuk mengetahui hasil login
}
```

Monitor Chat Function: Fungsi yang dijalankan pada thread terpisah untuk memonitor chat room menggunakan inotify untuk mendeteksi perubahan pada file chat.csv dan select untuk menerima pesan dari server atau input dari pengguna.
```
void *monitor_chat(void *args) {
    // Mengatur monitor chat menggunakan inotify dan select untuk socket
    // Menghandle pembacaan perubahan chat dari file chat.csv
    // Menghandle input dari pengguna dan mengirimkan pesan chat ke server
}
```

Read Existing Chat Function: Membaca dan mencetak chat history yang ada dari file chat.csv.
```
void read_existing_chat(const char *filepath) {
    // Membaca chat history yang sudah ada dari file chat.csv
}
```

Berikut adalah penjelasan code yang kami gunakan untuk Discorit.c

Main Function: Mengatur proses utama dari aplikasi, termasuk inisialisasi socket, parsing argumen dari command line, koneksi ke server, dan menjalankan fungsi terkait (register atau login).
```
int main(int argc, char *argv[]) {
    // Proses parsing argumen dan inisialisasi socket

    // Melakukan koneksi ke server berdasarkan perintah REGISTER atau LOGIN
    // Menjalankan fungsi terkait berdasarkan perintah yang diberikan
    // Menutup koneksi socket setelah selesai
}
```

Register User Function: Mengirimkan data register ke server menggunakan socket dan membaca balasan dari server untuk menampilkan status registrasi.
```
void register_user(int sockfd, char *username, char *password) {
    // Mengirim pesan REGISTER ke server dengan username dan password
    // Membaca balasan dari server setelah operasi register selesai
}
```

Login User Function: Mengirimkan data login ke server menggunakan socket, membaca balasan dari server untuk menampilkan status login, dan jika berhasil, memulai interaksi pasca login dengan server.
```
void login_user(int sockfd, char *username, char *password) {
    // Mengirim pesan LOGIN ke server dengan username dan password
    // Membaca balasan dari server untuk menampilkan status login
    // Jika login berhasil, menjalankan interaksi pasca login dengan server
}
```

Post Login Interaction Function: Fungsi yang mengatur interaksi setelah login berhasil. Ini termasuk menerima input dari pengguna, mengirimkan perintah ke server, dan menangani perintah khusus seperti JOIN, EDIT PROFILE, dan EXIT.
```
void post_login_interaction(int sockfd, char *username) {
    // Mengatur interaksi setelah login, menerima input dari pengguna
    // Mengirimkan pesan ke server sesuai dengan input pengguna
    // Mengelola perintah JOIN, EDIT PROFILE, dan EXIT
}
```

Handle Edit Profile Function: Fungsi untuk mengelola perintah EDIT PROFILE SELF -u (edit username) dan -p (edit password). Mengirimkan perintah ke server, membaca balasan, dan memperbarui konteks saat diperlukan.
```
void handle_edit_profile(int sockfd, char *input, char *username, char *current_context) {
    // Mengelola perintah EDIT PROFILE SELF -u dan -p
    // Mengirimkan perintah ke server untuk mengedit profil
    // Memperbarui username di sisi client jika profil diubah
}
```













