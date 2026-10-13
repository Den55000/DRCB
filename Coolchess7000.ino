#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define CV_DEBUG_COUNTS
#define CV_DEBUG_COUNTS_NO_SERIAL
//#define CV_DEBUG_HUES
//#define WEB_ACCESS_POINT

#include "cv.h"
#include "web.h"

int flashOccupants = 0;
uint8_t * jpg_buf = NULL;
size_t jpg_len = 0;

char board[8][8] = {
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' },
  { ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ' }
};

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  pinMode(GPIO_NUM_4, OUTPUT);
  Serial.begin(115200);
  Serial.println("Begin");
  configInitCamera();
  Serial.println("Camera init complete");
  delay(500);
  initWiFi();
}

void loop(){
  delay(2500);
  ChessBoardCV();
  broadcastBoardUpdate();
}
