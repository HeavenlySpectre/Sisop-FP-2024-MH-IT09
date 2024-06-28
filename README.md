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












