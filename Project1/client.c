#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// 매크로 및 상수 정의
#define FBDEV "/dev/fb0"
#define SERVER_IP "10.10.141.202"
#define PORT 5000
#define FILE_PORT 5001
#define BUFFER_SIZE 1024
#define TOUCH_DEVICE "/dev/input/event2"
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

// 화면 분할: 왼쪽 500픽셀 = 채팅 영역, 오른쪽 300픽셀 = 키보드 영역
#define CHAT_AREA_WIDTH 550
#define KEYBOARD_AREA_X 550
#define KEYBOARD_AREA_WIDTH (SCREEN_WIDTH - CHAT_AREA_WIDTH)

#define COLOR_RED 0xff0000
#define COLOR_GREEN 0x00ff00
#define COLOR_BLUE 0x0000ff
#define COLOR_BLACK 0x000000
#define COLOR_WHITE 0xffffff
#define COLOR_GRAY 0xC618
#define COLOR_NAVY 0x000F
#define COLOR_DARKGREEN 0x03E0
#define COLOR_DARKCYAN 0x03EF
#define COLOR_PURPLE 0x780F
#define COLOR_OLIVE 0x7BE0
#define COLOR_DARKGREY 0x7BEF
#define COLOR_MAGENTA 0xF81F
#define COLOR_YELLOW 0xFFE0
#define COLOR_ORANGE 0xFD20
#define COLOR_GREENYELLOW 0xAFE5
#define COLOR_PINK 0xF81F

// 글자 크기 상수
#define CHAR_WIDTH_SMALL 12   // 2배 확대 (키보드용)
#define CHAR_HEIGHT_SMALL 14
#define CHAR_WIDTH_LARGE 10   // 3배 확대 (채팅용)
#define CHAR_HEIGHT_LARGE 12

// 채팅 관련 상수
#define MAX_MESSAGES 12
#define MAX_TEXT_LENGTH 48   // 최대 48자 (여러 줄)

#define MAX_USERS 10  // 최대 10명까지 상대방 저장 가능

// 상대방 닉네임을 저장할 구조체 추가
struct chat_user {
    char ip[INET_ADDRSTRLEN];  // 상대방 IP 주소 (고유값으로 사용)
    char nickname[50];         // 상대방 닉네임
};

struct chat_user users[MAX_USERS]; // 닉네임 저장용 배열
int user_count = 0;


// 전역 변수 (프레임버퍼, 네트워크 등)
pid_t pid;

struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
struct fb_var_screeninfo *vip = &vinfo;
struct fb_fix_screeninfo *fip = &finfo;
char *map;
char *fbp = NULL;
int fb_size = 0;

int client_socket;
int file_client_socket;
char nickname[50];
int mode = 0;  // mode == 0 : 닉네임 입력 모드, mode == 1 : 채팅 모드

// 채팅 메시지 구조체 및 배열
struct chat_message {
    char sender_name[50];      // 보낸 사람 이름
    char text[MAX_TEXT_LENGTH + 1]; // 메시지 내용
    int sender;                // 0: 상대방, 1: 내 메시지
    char timestamp[10];        // HH:MM 형태의 시간
};

struct chat_message messages[MAX_MESSAGES];
int message_count = 0;


// 가상 키보드 전역 변수
char input_buffer[30] = "";
int input_length = 0;
int last_touched_button = -1;
time_t last_touch_time = 0;

typedef struct {
    int x, y, width, height;
    char text[30];  // 버튼에 들어갈 문자열
    int touch_count; // 다중 터치 시 문자 순환을 위한 카운트
} Button;

Button buttons[] = {
    {570,  90, 60, 50, "A",   0},
    {650,  90, 60, 50, "E",   0},
    {730,  90, 60, 50, "I",   0},
    {570, 150, 60, 50, "O",   0},
    {650, 150, 60, 50, "U",   0},
    {730, 150, 60, 50, "BCD", 0},
    {570, 210, 60, 50, "FGH", 0},
    {650, 210, 60, 50, "JKL", 0},
    {730, 210, 60, 50, "MNP", 0},
    {570, 270, 60, 50, "QRS", 0},
    {650, 270, 60, 50, "TVW", 0},
    {730, 270, 60, 50, "XYZ", 0},
    {570, 330, 60, 50, "DEL", 0},
    {650, 330, 60, 50, "ENT", 0},
    {730, 330, 60, 50, " ", 0},
    {570, 390, 60, 50, " = ", 0},
    {650, 390, 60, 50, ":) ", 0},
    {730, 390, 60, 50, ":( ", 0},
};
int button_count = sizeof(buttons) / sizeof(Button);

// 5x7 비트맵 폰트 데이터
const unsigned char font5x7[128][7] = {
    ['A'] = {0b01110,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001},
    ['B'] = {0b11110,0b10001,0b10001,0b11110,0b10001,0b10001,0b11110},
    ['C'] = {0b01110,0b10001,0b10000,0b10000,0b10000,0b10001,0b01110},
    ['D'] = {0b11100,0b10010,0b10001,0b10001,0b10001,0b10010,0b11100},
    ['E'] = {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b11111},
    ['F'] = {0b11111,0b10000,0b10000,0b11110,0b10000,0b10000,0b10000},
    ['G'] = {0b01110,0b10001,0b10000,0b10011,0b10001,0b10001,0b01110},
    ['H'] = {0b10001,0b10001,0b10001,0b11111,0b10001,0b10001,0b10001},
    ['I'] = {0b01110,0b00100,0b00100,0b00100,0b00100,0b00100,0b01110},
    ['J'] = {0b00001,0b00001,0b00001,0b00001,0b10001,0b10001,0b01110},
    ['K'] = {0b10001,0b10010,0b10100,0b11000,0b10100,0b10010,0b10001},
    ['L'] = {0b10000,0b10000,0b10000,0b10000,0b10000,0b10000,0b11111},
    ['M'] = {0b10001,0b11011,0b10101,0b10101,0b10001,0b10001,0b10001},
    ['N'] = {0b10001,0b11001,0b10101,0b10011,0b10001,0b10001,0b10001},
    ['O'] = {0b01110,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110},
    ['P'] = {0b11110,0b10001,0b10001,0b11110,0b10000,0b10000,0b10000},
    ['Q'] = {0b01110,0b10001,0b10001,0b10001,0b10101,0b10010,0b01101},
    ['R'] = {0b11110,0b10001,0b10001,0b11110,0b10100,0b10010,0b10001},
    ['S'] = {0b01110,0b10001,0b10000,0b01110,0b00001,0b10001,0b01110},
    ['T'] = {0b11111,0b00100,0b00100,0b00100,0b00100,0b00100,0b00100},
    ['U'] = {0b10001,0b10001,0b10001,0b10001,0b10001,0b10001,0b01110},
    ['V'] = {0b10001,0b10001,0b10001,0b10001,0b01010,0b01010,0b00100},
    ['W'] = {0b10001,0b10001,0b10001,0b10101,0b10101,0b11011,0b10001},
    ['X'] = {0b10001,0b01010,0b01010,0b00100,0b01010,0b01010,0b10001},
    ['Y'] = {0b10001,0b10001,0b01010,0b00100,0b00100,0b00100,0b00100},
    ['Z'] = {0b11111,0b00001,0b00010,0b00100,0b01000,0b10000,0b11111},
    ['!'] = {0b00100,0b00100,0b00100,0b00100,0b00100,0b00000,0b00100},
    ['@'] = {0b01110,0b10001,0b10111,0b10101,0b10111,0b10000,0b01110},
    ['#'] = {0b01010,0b01010,0b11111,0b01010,0b11111,0b01010,0b01010},
    [' '] = {0b00000,0b00000,0b00000,0b00000,0b00000,0b00000,0b00000},
    ['0'] = {0b01110,0b10001,0b10011,0b10101,0b11001,0b10001,0b01110},
    ['1'] = {0b00100,0b01100,0b00100,0b00100,0b00100,0b00100,0b01110},
    ['2'] = {0b01110,0b10001,0b00001,0b00110,0b01000,0b10000,0b11111},
    ['3'] = {0b01110,0b10001,0b00001,0b00110,0b00001,0b10001,0b01110},
    ['4'] = {0b00010,0b00110,0b01010,0b10010,0b11111,0b00010,0b00010},
    ['5'] = {0b11111,0b10000,0b11110,0b00001,0b00001,0b10001,0b01110},
    ['6'] = {0b00110,0b01000,0b10000,0b11110,0b10001,0b10001,0b01110},
    ['7'] = {0b11111,0b00001,0b00010,0b00100,0b01000,0b01000,0b01000},
    ['8'] = {0b01110,0b10001,0b10001,0b01110,0b10001,0b10001,0b01110},
    ['9'] = {0b01110,0b10001,0b10001,0b01111,0b00001,0b00010,0b01100},
    [':'] = {0b00000,0b00100,0b00100,0b00000,0b00000,0b00100,0b00100},
    ['('] = {0b00010,0b00100,0b01000,0b01000,0b01000,0b00100,0b00010},
    [')'] = {0b01000,0b00100,0b00010,0b00010,0b00010,0b00100,0b01000},
    ['='] = {0b11000,0b11000,0b11000,0b11111,0b11111,0b11111,0b11111},
};

// 기본 그리기 함수들
void draw_rect(int x, int y, int w, int h, unsigned int color) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            if (xx < vinfo.xres && yy < vinfo.yres) {
                int location = xx * (vinfo.bits_per_pixel / 8) + yy * finfo.line_length;
                *(unsigned int *)(fbp + location) = color;
            }
        }
    }
}

// 2배 확대 (키보드용) 텍스트 출력 함수
void draw_text_small(int x, int y, const char *text, unsigned int color) {
    for (int i = 0; text[i] != '\0'; i++) {
        char c = text[i];
        if (c < 32 || c > 127) continue;
        for (int j = 0; j < 7; j++) {
            for (int k = 0; k < 5; k++) {
                if (font5x7[(int)c][j] & (1 << (4 - k))) {
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int location = (x + (k * 2) + dx + (i * 12)) * (vinfo.bits_per_pixel / 8)
                                           + (y + (j * 2) + dy) * finfo.line_length;
                            if ((x + (k * 2) + dx + (i * 12)) < vinfo.xres &&
                                (y + (j * 2) + dy) < vinfo.yres)
                                *(unsigned int *)(fbp + location) = color;
                        }
                    }
                }
            }
        }
    }
}

// 채팅용 텍스트 출력 함수
void draw_text_large(int x, int y, const char *text, unsigned int color) {
    while (*text) {
        char c = *text;
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == ':' || c == ' ' ||
            (c >= 'a' && c <= 'z')) {
            for (int j = 0; j < 7; j++) {
                for (int k = 0; k < 5; k++) {
                    if (font5x7[(int)c][j] & (1 << (4 - k))) {
                        for (int dy = 0; dy < 2; dy++) {   // 3x3 확대(픽셀확대를 통한 글자크기 UP)
                            for (int dx = 0; dx < 2; dx++) {
                                int location = (x + k * 2 + dx) * (vinfo.bits_per_pixel / 8) +
                                               (y + j * 2 + dy) * finfo.line_length;
                                *(unsigned int *)(fbp + location) = color;
                            }
                        }
                    }
                }
            }
        }
        x += CHAR_WIDTH_LARGE + 5; // 문자 간격 조정
        text++;
    }
}

////////////////////////
// bmp display 
////////////////////////

void Lcd_Put_Pixel_565(int xx, int yy, unsigned short value)
{
	int location = xx * (vip->bits_per_pixel/8) + yy * fip->line_length;
	*(unsigned short *)(map + location) = value;
}

#pragma pack(push, 1)

typedef struct
{
	unsigned char magic[2];
	unsigned int size_of_file;
	unsigned char rsvd1[2];
	unsigned char rsvd2[2];
	unsigned int offset;
	unsigned int size_of_dib;
	unsigned int width;
	unsigned int height;
	unsigned short color_plane;
	unsigned short bpp;
	unsigned int compression;
	unsigned int size_of_image;
	unsigned int x_res;
	unsigned int y_res;
	unsigned int i_color;
}BMP_HDR;

#pragma pack(pop)

#define BMP_FILE ((BMP_HDR *)fp)

void Lcd_Print_BMP_Info(void * fp)
{
	printf("MAGIC = %c%c\n", BMP_FILE->magic[0], BMP_FILE->magic[1]);
	printf("BMP SIZE = %d\n", BMP_FILE->size_of_file);
	printf("RAW OFFSET = %d\n", BMP_FILE->offset);
	printf("DIB SIZE = %d\n", BMP_FILE->size_of_dib);
	printf("WIDTH = %d, HEIGHT = %d\n", BMP_FILE->width, BMP_FILE->height);
	printf("BPP = %d\n", BMP_FILE->bpp);
}

typedef struct
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
}PIXEL;

#define PIX565(p, x)	((((p)[x].red & 0xF8) << 8)|(((p)[x].green & 0xFC) << 3)|(((p)[x].blue) >> 3))


void Lcd_Draw_BMP_File_24bpp(int x, int y, void * fp)
{
	int xx, yy;
	unsigned int pad = (4 - ((BMP_FILE->width * 3) % 4))%4;
	unsigned char * pix = (unsigned char *)fp + BMP_FILE->offset;

	for(yy = (BMP_FILE->height-1) + y; yy >= y; yy--)
	{
		for(xx = x; xx < BMP_FILE->width + x; xx++)
		{
			Lcd_Put_Pixel_565(xx, yy, PIX565((PIXEL *)pix, xx-x));
		}

		pix = pix + (BMP_FILE->width * sizeof(PIXEL)) + pad;
	}
}

void draw_bmp(const char *fname, int x, int y)
{
	struct stat statb;
	int fd, ret;
	int msize;

	if (stat(fname, &statb) == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return;
	}

	msize = statb.st_size + 1024;
	char *p = malloc(msize);
	if(p == NULL) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return;
	}

	fd = open(fname, O_RDONLY);	
	if(fd == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return;
	}
	ret = read(fd, p, msize); 
	if(ret == -1) {
		printf("[%d] error: %s (%d)\n", pid, strerror(errno), __LINE__);
		return;
	}
	Lcd_Print_BMP_Info(p);
	Lcd_Draw_BMP_File_24bpp(x, y, p);
}

// 초기 닉네임 입력 문구
void draw_nickname_prompt() {
    // 닉네임 입력을 위한 배경
    draw_rect(0, 0, CHAT_AREA_WIDTH, vinfo.yres, COLOR_BLUE);
    // 안내 메시지 출력 (예: 상단에)
    draw_text_large(20, 50, "PRESS ENTER YOUR NAME", COLOR_BLACK);
    // 입력 칸(테두리)을 그립니다.
    draw_rect(20, 100, 300, 50, COLOR_WHITE);
    // 현재 입력된 내용을 표시 (input_buffer가 닉네임 입력 중인 문자)
    draw_text_large(30, 115, input_buffer, COLOR_BLACK);
}

// 닉네임을 저장하는 함수 추가
const char *get_user_nickname(const char *ip) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].ip, ip) == 0) {
            return users[i].nickname;
        }
    }
    return "알 수 없음";  // 등록되지 않은 사용자는 기본 닉네임 사용
}

void add_user(const char *ip, const char *nickname) {
    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].ip, ip) == 0) {
            // **이미 등록된 사용자면 닉네임 업데이트**
            strncpy(users[i].nickname, nickname, sizeof(users[i].nickname) - 1);
            return;
        }
    }

    // **새 사용자 추가**
    if (user_count < MAX_USERS) {
        strncpy(users[user_count].ip, ip, sizeof(users[user_count].ip) - 1);
        strncpy(users[user_count].nickname, nickname, sizeof(users[user_count].nickname) - 1);
        user_count++;
    }
}

// 닉네임 입력 후 나오는 채팅 영역

void clear_chat_area() {
    // 왼쪽 영역 채팅 배경 (0~CHAT_AREA_WIDTH)
    draw_rect(0, 0, CHAT_AREA_WIDTH, vinfo.yres, COLOR_YELLOW);
}

void get_current_time_str(char *buffer, int size) {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(buffer, size, "%02d:%02d", tm.tm_hour, tm.tm_min);
}

void draw_rounded_bubble(int x, int y, int w, int h, unsigned int color) {
    // 단순 사각형으로 그리되, 모서리 여백을 둠(타원형 만들려고)
    draw_rect(x + 7, y, w - 10, h, color);
    draw_rect(x, y + 7, w, h - 10, color);
}

void draw_chat_messages() {
    clear_chat_area();
    int y_offset = 20;
    int max_y = vinfo.yres - 20;


    // 메시지가 화면에 꽉 차면 오래된 메시지 제거
    while (message_count > 0 && (y_offset + (message_count * 80) > max_y)) {
        for (int i = 1; i < message_count; i++) {
            messages[i - 1] = messages[i];
        }
        message_count--;
    }
    
    for (int i = 0; i < message_count; i++) {
        // 채팅 영역은 왼쪽 500픽셀 사용
        int bubble_width = 200 + (strlen(messages[i].text) * 7); // 말풍선 좌우 크기기
        if (bubble_width > 400) bubble_width = 400;

        int bubble_x;
        if (messages[i].sender == 0) {
            bubble_x = 10;  // 상대방: 왼쪽 정렬
        } else {
            bubble_x = CHAT_AREA_WIDTH - bubble_width - 10;  // 내 메시지: 오른쪽 정렬
        }

        int text_x = bubble_x + 15;
        int line_count = (strlen(messages[i].text) / 20) + 1; // 메시지 창 한줄 당 20개 문자가 출력
        // printf("[%d] %s\n", __LINE__, messages[i].text);

        // printf("[%d] line_count = %d\n", __LINE__, line_count);

        if (line_count > 3) line_count = 3;
        int bubble_height = 30 + (line_count * (CHAR_HEIGHT_LARGE + 5)); // 말풍선 상하 크기

        draw_text_large(bubble_x, y_offset - 20, messages[i].sender_name, COLOR_BLACK);
        // printf("[%d]\n", __LINE__);
        // // 상대방 닉네임을 말풍선 위에 출력 (PINK 사각형보다 위)
        // if (messages[i].sender == 0) {
        //     draw_text_large(bubble_x, y_offset - 20, messages[i].sender_name, COLOR_BLACK);
        // }

        // 말풍선 그리기
        if (messages[i].sender == 0){
            draw_rounded_bubble(bubble_x, y_offset, bubble_width, bubble_height, COLOR_PINK);
        }
        else{
            draw_rounded_bubble(bubble_x, y_offset, bubble_width, bubble_height, COLOR_ORANGE);
        }

        // 메시지 내용 출력 (줄바꿈 처리)
        int text_y = y_offset + 15; // 수정된 Y 좌표 (더 위로 조정)
        for (int j = 0; j < line_count; j++) {
            char line[30] = {0};    // 글자수 받아서 담아놓는 공간
            // printf("[%d] line_count = %d\n", __LINE__, line_count);
            strncpy(line, messages[i].text + (j * 20), 20);     // 줄별 텍스트 처리 : 20개 되면 다음 줄로 넘어감감
            // draw_text_large(text_x, y_offset + 14 + (j * (CHAR_HEIGHT_LARGE + 5)), line, COLOR_BLACK);
            // printf("[%d]\n", __LINE__);

            // strcpy(line, "ABC");
            strcpy(line, messages[i].text);

            draw_text_large(text_x, text_y, line, COLOR_RED);
            // printf("[%d] line= %s\n", __LINE__, line);
            // if(strcmp(line, "ABC")==0){
            // printf("test : &s", line);
            // while(1);
            // }
            text_y += CHAR_HEIGHT_LARGE + 5;  // 다음 줄로 이동
            // printf("[%d]\n", __LINE__);
        }
        // 시간 출력 (말풍선 하단)
        int timestamp_x = messages[i].sender ? bubble_x - 70 : bubble_x + bubble_width + 5;
        draw_text_large(timestamp_x, y_offset + bubble_height - 5, messages[i].timestamp, COLOR_BLACK);

        // 입력하는 문자 보이는 창 시간 출력
        timestamp_x = (messages[i].sender == 0) ? (bubble_x + bubble_width + 5) : (bubble_x - 70);
        draw_text_large(timestamp_x, y_offset + bubble_height - 5, messages[i].timestamp, COLOR_BLACK);
        // printf("[%d]\n", __LINE__);
        y_offset += bubble_height + 40;
    }

    // 입력 중인 텍스트를 표시할 영역 (BLUE 사각형)
    if (mode == 1) {
        int input_area_height = 30;
        int input_area_y = vinfo.yres - input_area_height - 10;
        // BLUE 색상의 사각형을 그립니다.
        draw_rect(10, input_area_y, CHAT_AREA_WIDTH - 20, input_area_height, COLOR_BLUE);
        // 입력한 텍스트를 흰색으로 출력합니다.
        draw_text_large(20, input_area_y + 10, input_buffer, COLOR_WHITE);
    }    
}

void add_chat_message(const char *sender, const char *text, int sender_flag) {
    if (message_count >= MAX_MESSAGES) {
        for (int i = 1; i < MAX_MESSAGES; i++) {
            messages[i - 1] = messages[i];
        }
        message_count = MAX_MESSAGES - 1;
    }
    strncpy(messages[message_count].sender_name, sender, sizeof(messages[message_count].sender_name) - 1);
    strncpy(messages[message_count].text, text, MAX_TEXT_LENGTH);
    messages[message_count].sender = sender_flag;
    get_current_time_str(messages[message_count].timestamp, sizeof(messages[message_count].timestamp));
    message_count++;
    draw_chat_messages();
}

// 키보드 영역 함수
void clear_keyboard_area() {
    // 오른쪽 영역 배경
    draw_rect(KEYBOARD_AREA_X, 800, KEYBOARD_AREA_WIDTH, vinfo.yres, COLOR_BLACK);
}

void draw_keyboard_layout() {
    clear_keyboard_area();
    int offset = 10;
    // 버튼들을 그리되, 좌표는 그대로 사용 (total.c의 좌표는 이미 오른쪽 영역 내)
    for (int i = 0; i < button_count; i++) {
        draw_rect(buttons[i].x + offset, buttons[i].y, buttons[i].width, buttons[i].height, COLOR_GRAY);
        int text_x = buttons[i].x + (buttons[i].width - 10) / 2;
        int text_y = buttons[i].y + (buttons[i].height - 14) / 2;
        draw_text_small(text_x, text_y, buttons[i].text, COLOR_WHITE);
    }
}


// 터치 이벤트 처리 스레드 (키보드)
void *touch_listener(void *arg) {
    struct input_event ev;
    int ret;
    int x = -1, y = -1;
    int touch_detected = 0;
    int margin = 5;  // 버튼 터치 범위 여유
    int fd = open(TOUCH_DEVICE, O_RDONLY);
    char a[10];
    if (fd == -1) {
        perror("터치 장치 열기 실패");
        return NULL;
    }
    printf("[INFO] 터치 장치 열림\n");
    while (1) {
        ret = read(fd, &ev, sizeof(ev));
        if (ret == -1) {
            printf("터치 읽기 오류: %s\n", strerror(errno));
            break;
        }
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_X)
                x = ev.value * SCREEN_WIDTH / 4096;
            if (ev.code == ABS_Y)
                y = ev.value * SCREEN_HEIGHT / 4096;
        }
        if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
            if (ev.value == 1) {
                // 터치 시작
                touch_detected = 1;
            } else if (ev.value == 0 && touch_detected) {
                // 터치 해제 시, 각 버튼과 충돌 체크
                for (int i = 0; i < button_count; i++) {
                    if (x >= buttons[i].x - margin && x <= buttons[i].x + buttons[i].width + margin &&
                        y >= buttons[i].y - margin && y <= buttons[i].y + buttons[i].height + margin) {

                        if (strcmp(buttons[i].text, "ENT") == 0) {
                            if (mode == 0) {  // 닉네임 입력 모드
                                if (input_length > 0) {
                                    strncpy(nickname, input_buffer, sizeof(nickname) - 1);
                                    nickname[sizeof(nickname) - 1] = '\0';
                                    // 닉네임을 서버로 전송 (필요하면)
                                    send(client_socket, nickname, strlen(nickname), 0);
                                    mode = 1;  // 채팅 모드로 전환
                                    memset(input_buffer, 0, sizeof(input_buffer));
                                    input_length = 0;
                                    add_chat_message(" ","WELCOME TO 5 TEAM" , 0);
                                }
                            } else {  // 채팅 모드
                                if (input_length > 0) {
                                    send(client_socket, input_buffer, strlen(input_buffer), 0);
                                    add_chat_message(nickname, input_buffer, 1);
                                    memset(input_buffer, 0, sizeof(input_buffer));
                                    input_length = 0;
                                }
                            }
                        } else if (strcmp(buttons[i].text, "DEL") == 0) {
                            if (input_length > 0) {
                                input_buffer[input_length - 1] = '\0';
                                input_length--;
                            }
                        } else if (strcmp(buttons[i].text, " ") == 0) {
                            if (input_length < 30) {
                                input_buffer[input_length++] = ' ';
                                input_buffer[input_length] = '\0';
                            }
                        } else if (strcmp(buttons[i].text, " = ") == 0) {
                            strcpy(a,"==");
                            send(client_socket,a,strlen("=="),0);
                            receive_bmp_file(client_socket, "received.bmp");
                            continue;
                        } else {
                            // 키에 3개가 있는 경우
                            time_t current_time = time(NULL);
                            int text_len = strlen(buttons[i].text);
                            if (i == last_touched_button && (current_time - last_touch_time) < 2) {
                                if (buttons[i].touch_count < text_len - 1)
                                    buttons[i].touch_count++;
                                else
                                    buttons[i].touch_count = 0;
                                // 이전 문자 제거 (현재 입력 중인 값 업데이트)
                                if (input_length > 0)
                                    input_length--;
                            } else {
                                buttons[i].touch_count = 0;
                            }
                            char selected_char = buttons[i].text[buttons[i].touch_count];
                            printf("터치한 문자 : %c\n", selected_char);
                            if (input_length < 30) {
                                input_buffer[input_length++] = selected_char;
                                input_buffer[input_length] = '\0';
                            }
                            last_touched_button = i;
                            last_touch_time = current_time;
                        }
                    }
                }
                touch_detected = 0;
                // 모드에 따라 화면 갱신: 닉네임 입력 모드이면 프롬프트, 채팅 모드이면 키보드 레이아웃
                if (mode == 0)
                    draw_nickname_prompt();
                else
                    draw_keyboard_layout();
            }
        }
    }
    close(fd);
    return NULL;
}

// 네트워크 메시지 수신 스레드
void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];

    while (1) {
        struct sockaddr_in sender_addr;
        socklen_t addr_size = sizeof(sender_addr);

        // 수신할 때 보낸 사람 주소도 같이 받음
        int bytes_received = recvfrom(client_socket, buffer, sizeof(buffer) - 1, 0, 
                                      (struct sockaddr *)&sender_addr, &addr_size);
        if (bytes_received <= 0) break;
        buffer[bytes_received] = '\0';

        char sender_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, sizeof(sender_ip));

        printf("[수신] %s\n", buffer);

        char sender[50] = {0};
        char message[BUFFER_SIZE] = {0};

        // 메시지 파싱 (닉네임:내용 형식)
        char *token = strtok(buffer, ":");
        if (token != NULL) {
            strncpy(sender, token, sizeof(sender) - 1);
            token = strtok(NULL, "\0");
            if (token != NULL) {
                strncpy(message, token + 1, sizeof(message) - 1);
            } else {
                strcpy(message, "");
            }
        } else {
            strcpy(sender, "알 수 없음");
            strncpy(message, buffer, sizeof(message) - 1);
        }

        // **닉네임이 등록되지 않은 경우 추가**
        add_user(sender_ip, sender);

        // **저장된 닉네임 가져오기 (IP 기반)**
        const char *stored_nickname = get_user_nickname(sender_ip);

        // **채팅 메시지 추가**
        add_chat_message(stored_nickname, message, 0);
    }

    return NULL;
}

// 🔹 BMP 파일 수신 함수
void receive_bmp_file(int sock, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("파일 저장 실패");
        return;
    }

    char buffer[BUFFER_SIZE];
    int bytes_received;
    printf("BMP 파일 수신 중...\n");

    while ((bytes_received = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, bytes_received, file);
        if (bytes_received < BUFFER_SIZE) break;  // 마지막 패킷 확인
    }

    printf("BMP 파일 저장 완료: %s\n", filename);
    fclose(file);
}

// 메인 함수
int main() {
    // 프레임버퍼 초기화
    int fb_fd = open(FBDEV, O_RDWR);
    if (fb_fd == -1) {
        perror("프레임버퍼 장치 열기 실패");
        return EXIT_FAILURE;
    }
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("가변 화면 정보 읽기 실패");
        return EXIT_FAILURE;
    }
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("고정 화면 정보 읽기 실패");
        return EXIT_FAILURE;
    }
    fb_size = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    fbp = (char *)mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);
    if (fbp == MAP_FAILED) {
        perror("프레임버퍼 메모리 매핑 실패");
        return EXIT_FAILURE;
    }

    // 네트워크 소켓 생성 및 서버 연결
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("소켓 생성 실패");
        return EXIT_FAILURE;
    }
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("서버 연결 실패");
        return EXIT_FAILURE;
    }
    printf("[INFO] 서버에 연결됨.\n");

    //파일 서버 연결결
    file_client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (file_client_socket == -1) {
        perror("소켓 생성 실패");
        return EXIT_FAILURE;
    }
    struct sockaddr_in file_server_addr;
    file_server_addr.sin_family = AF_INET;
    file_server_addr.sin_port = htons(FILE_PORT);
    inet_pton(AF_INET, SERVER_IP, &file_server_addr.sin_addr);
    if (connect(file_client_socket, (struct sockaddr *)&file_server_addr, sizeof(file_server_addr)) == -1) {
        perror("파일 서버 연결 실패");
        return EXIT_FAILURE;
    }
    printf("[INFO] 파일 서버에 연결됨.\n");
    
    draw_nickname_prompt();

    // 초기 화면 그리기: 채팅 영역과 키보드 영역 분리
    clear_chat_area();
    clear_keyboard_area();
    draw_keyboard_layout();
    draw_chat_messages();

    // 스레드 생성: 터치 이벤트와 네트워크 수신
    pthread_t recv_thread, touch_thread;
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_create(&touch_thread, NULL, touch_listener, NULL);
    pthread_detach(recv_thread);
    pthread_detach(touch_thread);

        // 메인 루프: 주기적으로 두 영역을 갱신 (필요시)
        // strcpy(messages[0].text, WE)
        while (1) {
            usleep(300000);  // 0.1초마다 갱신
            if (mode == 0) {
                draw_nickname_prompt();
            } else {
                draw_chat_messages(); 
                draw_keyboard_layout();
            }
        }

    munmap(fbp, fb_size);
    close(fb_fd);
    close(client_socket);
    close(file_client_socket);
    return 0;
}