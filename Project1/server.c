#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define PORT 5000        // 메세지 전송용 포트트
#define PORT2 5001       // 파일 전송용 포트
#define MAX_CLIENTS 10   //최대 클라이언트 수
#define BUFFER_SIZE 1024   //버퍼 사이즈
#define LOG_FILE "chat_log.txt"    //로그 파일

typedef struct {
    int socket;
    char nickname[50];
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int server_socket;
int file_server_socket;
int server_running = 1;
pid_t pid;

// 🔹 메시지 로그 저장
void log_message(const char *message) {
    FILE *file = fopen(LOG_FILE, "a");
    if (file) {
        fprintf(file, "%s\n", message);
        fclose(file);
    }
}

// 🔹 메시지 브로드캐스트
void broadcast(char *message, int sender_socket) {
    pthread_mutex_lock(&mutex);
    log_message(message);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket != sender_socket) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&mutex);
}

// 🔹 BMP 파일 전송 함수
void send_bmp_file(int sender_socket, const char *filename) {
    
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("파일을 열 수 없습니다: %s\n", filename);
        return;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    printf("BMP 파일 전송 시작: %s\n", filename);

    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        int bytes_sent = send(sender_socket, buffer, bytes_read, 0);
        if (bytes_sent < 0) {
            perror("파일 전송 중 오류 발생");
            break;
        }
        printf("전송: %d 바이트\n", bytes_sent);
    }

    fclose(file);
    printf("BMP 파일 전송 완료: %s\n", filename);
}

// 🔹 클라이언트 핸들러
void *handle_client(void *arg) {
    Client client = *(Client *)arg;
    char buffer[BUFFER_SIZE];

    sprintf(buffer, "%s: ENTERED", client.nickname);
    broadcast(buffer, client.socket);
    printf("%s\n", buffer);

    while (server_running) {
        int bytes_received = recv(client.socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            break;
        }
        buffer[bytes_received] = '\0';

        char full_message[BUFFER_SIZE + 50];
        if (strcmp(buffer,"==")==0){
            send_bmp_file(client.socket, "thumb.bmp");
        }
        else{
            sprintf(full_message, "%s: %s", client.nickname, buffer);
            broadcast(full_message, client.socket);
            printf("%s\n", full_message);
        }
    }

    close(client.socket);
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].socket == client.socket) {
            for (int j = i; j < client_count - 1; j++) {
                clients[j] = clients[j + 1];
            }
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);

    sprintf(buffer, "%s: HAS LEFT", client.nickname);
    broadcast(buffer, -1);
    printf("%s\n", buffer);
    return NULL;
}


// 🔹 파일 서버 스레드
void *file_server(void *arg) {
    struct sockaddr_in file_server_addr, file_client_addr;
    socklen_t file_client_addr_size;
    pthread_t file_thread;
    int ret;
    int optval = 1;

    file_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    ret = setsockopt(file_server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
    file_server_addr.sin_family = AF_INET;
    file_server_addr.sin_addr.s_addr = INADDR_ANY;
    file_server_addr.sin_port = htons(PORT2);

    bind(file_server_socket, (struct sockaddr *)&file_server_addr, sizeof(file_server_addr));
    listen(file_server_socket, MAX_CLIENTS);
    printf("파일 서버가 실행 중... (PORT2: %d)\n", PORT2);

    while (server_running) {
        file_client_addr_size = sizeof(file_client_addr);
        int file_client_socket = accept(file_server_socket, (struct sockaddr *)&file_client_addr, &file_client_addr_size);
        if (!server_running) break;

        pthread_create(&file_thread, NULL, (void *)send_bmp_file, (void *)&file_client_socket);
        pthread_detach(file_thread);
    }

    close(file_server_socket);
    return NULL;
}

// 🔹 종료 감지 스레드
void *shutdown_server(void *arg) {
    char command[10];
    while (server_running) {
        scanf("%s", command);
        if (strcmp(command, "/exit") == 0) {
            printf("서버 종료 중...\n");
            server_running = 0;

            pthread_mutex_lock(&mutex);
            for (int i = 0; i < client_count; i++) {
                close(clients[i].socket);
            }
            client_count = 0;
            pthread_mutex_unlock(&mutex);

            close(server_socket);
            close(file_server_socket);
            exit(0);
        }
    }
    return NULL;
}



// 🔹 메인 함수
int main() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_size;
    pthread_t tid, file_tid, shutdown_tid;
    int ret;
    int optval = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    ret = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return EXIT_FAILURE;
	}
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);


    bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, MAX_CLIENTS);

    printf("메시지 서버가 실행 중... (PORT: %d)\n", PORT);

    // 🔹 파일 서버 스레드 실행
    pthread_create(&file_tid, NULL, file_server, NULL);
    pthread_detach(file_tid);

    // 🔹 종료 감지 스레드 실행
    pthread_create(&shutdown_tid, NULL, shutdown_server, NULL);
    pthread_detach(shutdown_tid);

    while (server_running) {
        client_addr_size = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_size);
        if (!server_running) break;

        Client new_client;
        new_client.socket = client_socket;

        memset(new_client.nickname, 0, sizeof(new_client.nickname));
        recv(client_socket, new_client.nickname, sizeof(new_client.nickname) - 1, 0);
        new_client.nickname[strcspn(new_client.nickname, "\n")] = '\0';

        pthread_mutex_lock(&mutex);
        clients[client_count++] = new_client;
        pthread_mutex_unlock(&mutex);

        pthread_create(&tid, NULL, handle_client, (void *)&new_client);
        pthread_detach(tid);
    }

    close(server_socket);
    return 0;
}