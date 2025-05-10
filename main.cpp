
#include "device_driver.h"
#include "graphics.h"

#define LCDW (320)
#define LCDH (240)
#define X_MIN (0)
#define X_MAX (LCDW - 1)
#define Y_MIN (0)
#define Y_MAX (LCDH - 1)

#define TIMER_PERIOD (10)
#define RIGHT (1)
#define LEFT (-1)
#define HOME (0)
#define SCHOOL (1)

#define CAR_STEP (10)
#define CAR_SIZE_X (20)
#define CAR_SIZE_Y (20)
#define FROG_STEP (10)
#define FROG_SIZE_X (20)
#define FROG_SIZE_Y (20)

#define BACK_COLOR (5)
#define CAR_COLOR (0)
#define FROG_COLOR (2)

#define GAME_OVER (1)

#define TIM2_TICK (20)                  // usec
#define TIM2_FREQ (1000000 / TIM2_TICK) // Hz
#define TIME2_PLS_OF_1ms (1000 / TIM2_TICK)
#define TIM2_MAX (0xffffu)

static unsigned short color[] = {RED, YELLOW, GREEN, BLUE, WHITE, BLACK};
static unsigned short carcolor[] = {VIOLET, BLUE, WHITE, RED};

static unsigned short zombie_color[] = {BLUE, Pink, BLACK};
static unsigned short plant_color[] = {GREEN, DarkGreen, BLACK};
static unsigned short atk_color[] = {LightGrey, Purple, BLACK};

class obj
{
public:
   int obj_x, obj_y, obj_w, obj_h, obj_color, obj_dir;

   virtual void move(int k) = 0;
   virtual void draw() = 0;
   virtual void erase() = 0;
};

class car : public obj
{
public:
   int speed;
   bool visible; // true: 디스플레이에 표시

   car(int c)
       : speed(1),
         visible(false)
   {
      obj_x = 0;
      obj_y = 0;
      obj_w = CAR_SIZE_X;
      obj_h = CAR_SIZE_Y;
      obj_color = c;
      obj_dir = RIGHT;
   }

   car(int c, int s, int w, int h)
       : speed(s),
         visible(false)
   {
      obj_x = 0;
      obj_y = 0;
      obj_w = w;
      obj_h = h;
      obj_color = c;
      obj_dir = RIGHT;
   }

   virtual void move(int k) override
   {
      if (visible)
      {
         erase();

         obj_x += CAR_STEP * k * speed;

         if (obj_x + obj_w < X_MIN)
         {
            obj_x = X_MAX;
         }
         else if (obj_x > X_MAX)
         {
            obj_x = X_MIN - obj_w;
         }

         draw();
      }
   }

   virtual void draw() override
   {
      if (visible)
      {
         Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, carcolor[obj_color]);
      }
   }

   virtual void erase() override
   {
      if (visible)
      {
         Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, color[BACK_COLOR]);
      }
   }

   int Car_Acident(int frog_x, int frog_y)
   {
      if (!visible)
         return 0;

      int col = 0;

      if ((obj_x >= frog_x) && ((frog_x + FROG_STEP) >= obj_x))
         col |= 1 << 0;
      else if ((obj_x < frog_x) && ((obj_x + CAR_STEP) >= frog_x))
         col |= 1 << 0;

      if ((obj_y >= frog_y) && ((frog_y + FROG_STEP) >= obj_y))
         col |= 1 << 1;
      else if ((obj_y < frog_y) && ((obj_y + CAR_STEP) >= frog_y))
         col |= 1 << 1;

      if (col == 3)
      {
         return GAME_OVER;
      }

      return 0;
   }
};

class truck : public car
{
public:
   truck()
       : car(1, 1, 50, 25) // 색상: 1 (BLUE), 속도: 1, 너비: 50, 높이: 25
   {
   }

   virtual void draw() override
   {
      if (visible)
      {
         Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, carcolor[obj_color]);
         Uart_Printf("Truck Drawn at (%d, %d)\n", obj_x, obj_y);
      }
   }
};

class motorbike : public car
{
public:
   motorbike()
       : car(0, 2, 15, 10) // 색상: 0 (VIOLET), 속도: 2, 너비: 15, 높이: 10
   {
   }

   virtual void draw() override
   {
      if (visible)
      {
         Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, carcolor[obj_color]);
         Uart_Printf("Motorbike Drawn at (%d, %d)\n", obj_x, obj_y);
      }
   }
};

class train : public car
{
public:
   train()
       : car(2, 3, 80, 20) // 색상: 2 (WHITE), 속도: 3, 너비: 80, 높이: 20
   {
   }

   virtual void draw() override
   {
      if (visible)
      {
         Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, carcolor[obj_color]);
         Uart_Printf("Train Drawn at (%d, %d)\n", obj_x, obj_y);
      }
   }
};

class sedan : public car
{
public:
   sedan()
       : car(3, 1, 20, 20) // 색상: 3 (RED), 속도: 1, 너비: 20, 높이: 20
   {
   }

   virtual void draw() override
   {
      if (visible)
      {
         Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, carcolor[obj_color]);
         Uart_Printf("Sedan Drawn at (%d, %d)\n", obj_x, obj_y);
      }
   }
};

class frog : public obj
{
public:
   static int score;
   static int lives;

   frog()
   {
      obj_x = 150;
      obj_y = 220;
      obj_w = FROG_SIZE_X;
      obj_h = FROG_SIZE_Y;
      obj_dir = SCHOOL;
      obj_color = FROG_COLOR;
   }

   virtual void draw() override
   {
      Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, color[obj_color]);
   }

   virtual void erase() override
   {
      Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, color[BACK_COLOR]);
   }

   virtual void move(int k) override
   {
      switch (k)
      {
      case 0: // UP

         if (obj_y > Y_MIN)
            obj_y -= FROG_STEP;
         break;

      case 1: // DOWN
         if (obj_y + obj_h < Y_MAX)
            obj_y += FROG_STEP;
         break;

      case 2: // LEFT
         if (obj_x > X_MIN)
            obj_x -= FROG_STEP;
         break;

      case 3: // RIGHT
         if (obj_x + obj_w < X_MAX)
            obj_x += FROG_STEP;
         break;
      }
   }

   void score_up()
   {
      if ((obj_dir == SCHOOL) && (obj_y == Y_MIN))
      {
         Uart_Printf("SCHOOL\n");
         obj_dir = HOME;
      }

      if ((obj_dir == HOME) && (obj_y == LCDH - obj_h))
      {
         obj_dir = SCHOOL;
         score++;
         Uart_Printf("HOME\n");
         Uart_Printf("Score : %d\n", score);
      }
   }

   void reset_position()
   {
      if (obj_dir == SCHOOL)
      {
         obj_x = 150;
         obj_y = 220; // 초기 위치로 복구
      }
      if (obj_dir == HOME)
      {
         obj_x = 150;
         obj_y = 220; // 초기 위치로 복구
         score -= 1;
      }
   }
};

int frog::score = 0;
int frog::lives = 3;

class obj_2
{
public:
   int obj_x, obj_y, obj_w, obj_h, obj_color, obj_life, obj_power;
   virtual void draw() = 0;
   virtual void erase() = 0;
};

class plant : public obj_2
{
public:
   plant(int x, int y, int k, int l, int p)
   {
      obj_x = x;
      obj_y = y;
      obj_w = 30;
      obj_h = 30;
      obj_color = k;
      obj_life = l;
      obj_power = p;
   }

   virtual void draw() override
   {
      Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, plant_color[obj_color]);
   }

   virtual void erase() override
   {
      Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, plant_color[BACK_COLOR]);
   }

   int gun_x = 0;

   void plant_attack(int &zombi_x, int &zombie_y, int &zombie_l)
   {
      if (obj_x + 30 + gun_x + 10 >= X_MAX)
      {
         gun_x = 0;
      }
      else if ((obj_x + 30 + gun_x + 10 >= zombi_x) && (zombie_y == obj_y))
      {
         zombie_l -= 1;
         gun_x = 0;
      }
      Lcd_Draw_Box(obj_x + 30 + gun_x, obj_y + 10, 10, 10, atk_color[BACK_COLOR]);
      gun_x += 10;
      Lcd_Draw_Box(obj_x + 30 + gun_x, obj_y + 10, 10, 10, atk_color[obj_color]);
   }
};

class zombie : public obj_2
{
public:
   zombie(int x, int y, int k, int l, int p)
   {
      obj_x = x;
      obj_y = y;
      obj_w = 30;
      obj_h = 30;
      obj_color = k;
      obj_life = l;
      obj_power = p;
   }

   virtual void draw() override
   {
      Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, zombie_color[obj_color]);
   }

   virtual void erase() override
   {
      Lcd_Draw_Box(obj_x, obj_y, obj_w, obj_h, zombie_color[BACK_COLOR]);
   }

   void move_attack(int &plant_x, int &plant_l)
   {
      if (obj_x - ((plant_x) + 30) <= 5)
      {
         plant_l -= 1;
      }
      else
      {
         erase();
         obj_x += 5 * -1;

         if (obj_x + obj_w < X_MIN)
         {
            obj_x = X_MAX;
         }
         draw();
      }
   }
};

class cursor
{
public:
   int cursor_x;
   int cursor_y;
   int s;

   cursor()
   {
      cursor_x = 0;
      cursor_y = 0;
      s = 0;
   }

   void CursorErase()
   {
      Lcd_Draw_Box(cursor_x, cursor_y, 10, 5, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x, cursor_y, 5, 10, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x, cursor_y + 35, 10, 5, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x, cursor_y + 30, 5, 10, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x + 30, cursor_y, 10, 5, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x + 35, cursor_y, 5, 10, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x + 30, cursor_y + 35, 10, 5, color[BACK_COLOR]);
      Lcd_Draw_Box(cursor_x + 35, cursor_y + 30, 5, 10, color[BACK_COLOR]);
   }

   void CursorBlink()
   {
      if (s % 2 == 0)
      {
         CursorErase();
      }
      else
      {
         Lcd_Draw_Box(cursor_x, cursor_y, 10, 5, color[1]);
         Lcd_Draw_Box(cursor_x, cursor_y, 5, 10, color[1]);
         Lcd_Draw_Box(cursor_x, cursor_y + 35, 10, 5, color[1]);
         Lcd_Draw_Box(cursor_x, cursor_y + 30, 5, 10, color[1]);
         Lcd_Draw_Box(cursor_x + 30, cursor_y, 10, 5, color[1]);
         Lcd_Draw_Box(cursor_x + 35, cursor_y, 5, 10, color[1]);
         Lcd_Draw_Box(cursor_x + 30, cursor_y + 35, 10, 5, color[1]);
         Lcd_Draw_Box(cursor_x + 35, cursor_y + 30, 5, 10, color[1]);
      }
      s++;
   }

   void move(int k)
   {
      switch (k)
      {
      case 0: // UP

         if (cursor_y > Y_MIN)
            cursor_y -= 40;
         break;

      case 1: // DOWN
         if (cursor_y + 40 < Y_MAX)
            cursor_y += 40;
         break;

      case 2: // LEFT
         if (cursor_x > X_MIN)
            cursor_x -= 40;
         break;

      case 3: // RIGHT
         if (cursor_x + 40 < X_MAX)
            cursor_x += 40;
         break;
      }
   }
};

extern volatile int TIM4_expired;
extern volatile int USART1_rx_ready;
extern volatile int USART1_rx_data;
extern volatile int Jog_key_in;
extern volatile int Jog_key;

extern "C" void abort(void)
{
   while (1)
      ;
}

static void Sys_Init(void)
{
   Clock_Init();
   LED_Init();
   Uart_Init(115200);

   SCB->VTOR = 0x08003000;
   SCB->SHCSR = 7 << 16;
}

void Game_Win()
{
   TIM4_Repeat_Interrupt_Enable(0, 0);
   Uart_Printf("Congratulations! You Win!\n");
   Uart_Printf("Please press any key to restart.\n");
   Jog_Wait_Key_Pressed();
   Jog_Wait_Key_Released();
   Uart_Printf("Game Restarting...\n");
}

void Game_Over()
{
   TIM4_Repeat_Interrupt_Enable(0, 0);
   Uart_Printf("Game Over, Please press any key to continue.\n");
   Jog_Wait_Key_Pressed();
   Jog_Wait_Key_Released();
   Uart_Printf("Game Start\n");
}

void Display_Lives_Message(int life)
{
   Lcd_Clr_Screen();
   if (life > 0)
   {
      Lcd_Printf(LCDW / 2 - 35, LCDH / 2 - 30, WHITE, BLACK, 1, 1, "You Died!");
      Lcd_Printf(LCDW / 2 - 60, LCDH / 2 + 0, WHITE, BLACK, 1, 1, "Life Remaining: %d", life);
      Lcd_Printf(LCDW / 2 - 100, LCDH / 2 + 30, WHITE, BLACK, 1, 1, "Press any key to continue.");
   }
}

void Game_Over_Animation()
{
   int i = 0;
   Lcd_Clr_Screen();
   while (i < 100)
   {

      Lcd_Printf(LCDW / 2 - 50, LCDH / 2 - 20, WHITE, BLACK, 2, 2, "Game Over");

      i++;
   }
}

void Handle_Frog_Death(frog &f)
{
   frog::lives--;
   if (f.lives > 0)
   {
      Uart_Printf("You died! Please press any key to continue.\n");
      Jog_Wait_Key_Pressed();
      Jog_Wait_Key_Released();
      Lcd_Clr_Screen();
      f.reset_position();
      f.draw();
   }
   else
   {
      Game_Over();
      Game_Over_Animation();
   }
}

void replace_car(car &vehicle, int y, int type)
{
   switch (type)
   {
   case 0:
      vehicle = sedan();
      break;
   case 1:
      vehicle = train();
      break;
   case 2:
      vehicle = motorbike();
      break;
   case 3:
      vehicle = truck();
      break;
   }

   vehicle.obj_y = y;
   vehicle.visible = true;
}


void Wait_For_Start()
{
   while (!Jog_key_in && !USART1_rx_ready)
   {
   }
   Jog_key_in = 0;
   USART1_rx_ready = 0;
}

void display_menu()
{

   Lcd_Clr_Screen();
   Lcd_Printf(30, 40, WHITE, BLACK, 2, 2, "1. Frog Game");
   Lcd_Printf(30, 80, WHITE, BLACK, 2, 2, "2. Zombie");
   Lcd_Printf(30, 120, WHITE, BLACK, 1, 2, "Enter selection (1 or 2): ");
}

int get_user_input()
{
   int user_input = -1;
   while (user_input == -1)
   {

      if (Jog_key_in)
      {
         user_input = Jog_key;
         Jog_key_in = 0;
      }
   }
   return user_input;
}

void play_game_A()
{

   Lcd_Clr_Screen();
   Lcd_Printf(100, 100, WHITE, BLACK, 2, 2, "Frog Game");
   TIM2_Delay(700);

   for (;;)
   {
      Lcd_Clr_Screen();
      frog f;

      car cars[4] = {sedan(), truck(), train(), motorbike()};

      for (int i = 0; i < 4; i++)
      {
         cars[i].obj_y = 30 + i * 50;
         cars[i].visible = false;
      }

      cars[0].visible = true;

      frog::score = 0;
      frog::lives = 3;
      f.draw();

      TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD * 10);

      for (;;)
      {
         Lcd_Printf(0, 0, WHITE, BLACK, 1, 1, "Score: %02d", frog::score);
         Lcd_Printf(250, 0, WHITE, BLACK, 1, 1, "Life: %02d", frog::lives);

         if (Jog_key_in)
         {
            f.erase();
            f.move(Jog_key);
            f.draw();
            Jog_key_in = 0;
         }

         f.score_up();

         int score = frog::score;
         int x = score;

         switch (score)
         {
         case 1:
            if (!cars[score].visible)
            {
               replace_car(cars[score], 50 + score * 30, x);
               Uart_Printf("New Vehicle Added.\n");
            }
            break;

         case 2:
            if (!cars[score].visible)
            {
               replace_car(cars[score], 50 + score * 30, x);
               Uart_Printf("New Vehicle Added.\n");
            }
            break;

         case 3:
            if (!cars[score].visible)
            {
               replace_car(cars[score], 50 + score * 30, x);
               Uart_Printf("New Vehicle Added.\n");
            }
            break;

         case 4:
            if (cars[0].speed == 1)
            {
               for (int i = 0; i < 4; i++)
               {
                  cars[i].speed = cars[i].speed + 1;
               }
               Uart_Printf("All Vehicle Speeds Increased.\n");
            }

            break;

         case 5:
            Game_Win();
            break;

         default:
            break;
         }

         if (TIM4_expired)
         {
            for (int j = 0; j < 4; j++)
            {
               int k = 0;
               if (j % 2)
                  k = 1;
               else
                  k = -1;
               cars[j].move(k);
            }
            TIM4_expired = 0;

            bool game_over = false;

            for (int j = 0; j < 4; j++)
            {
               if (cars[j].visible && cars[j].Car_Acident(f.obj_x, f.obj_y))
               {
                  game_over = true;
                  Display_Lives_Message(frog::lives - 1);
                  break;
               }
            }

            if (game_over)
            {
               Handle_Frog_Death(f); // 개구리가 죽었을 때 처리
               if (frog::lives <= 0)
                  break;
            }
         }
      }
   }
}

void play_game_B()
{

   Lcd_Clr_Screen();
   Lcd_Printf(100, 100, WHITE, BLACK, 2, 2, "Zombie");
   TIM2_Delay(700);

   cursor C;

   Sys_Init();
   Uart_Printf("Game Example\n");

   Lcd_Init();
   Jog_Poll_Init();
   Jog_ISR_Enable(1);
   Uart1_RX_Interrupt_Enable(1);
   zombie Z(X_MAX - 35, 5, 1, 3, 3);

   plant p(C.cursor_x + 5, C.cursor_y + 5, 2, 3, 5);

   for (;;)
   {
      Lcd_Clr_Screen();
      Z.draw();
      TIM4_Repeat_Interrupt_Enable(1, TIMER_PERIOD * 50);

      for (;;)
      {
         if (Jog_key_in)
         {
            if (Jog_key < 4)
            {
               C.CursorErase();
               C.move(Jog_key);
            }
            else
            {
               p.erase();
               p = plant(C.cursor_x + 5, C.cursor_y + 5, Jog_key - 4, 3, 5); // 새로운 식물 생성
               p.draw();
            }
            Jog_key_in = 0;
         }

         if (TIM4_expired)
         {
            if (p.obj_color != 2)
            {
               p.plant_attack(Z.obj_x, Z.obj_y, Z.obj_life);
            }
            Z.draw();
            Z.move_attack(p.obj_x, p.obj_life);
            if (Z.obj_life == 0)
            {
               Z.erase();
               Z.obj_life = 3;
               Z.obj_x = X_MAX - 35;
            }
            C.CursorBlink();
            TIM4_expired = 0;
         }
      }
   }
}

void select_game()
{
   display_menu();
   int selection = get_user_input(); // 사용자 입력 받기

   switch (selection)
   {
   case 1:
      // 게임 A 실행
      play_game_A();
      break;
   case 2:
      // 게임 B 실행
      play_game_B();
      break;
   default:
      Lcd_Printf(30, 160, WHITE, BLACK, 2, 2, "Invalid selection, try again.");
      select_game(); // 유효하지 않으면 다시 선택
      break;
   }
}

extern "C" void Main()
{
   Sys_Init();
   Uart_Printf("Game Example\n");

   Lcd_Init();
   Jog_Poll_Init();
   Jog_ISR_Enable(1);
   Uart1_RX_Interrupt_Enable(1);

   for (;;)
   {
      select_game();
   }
}
