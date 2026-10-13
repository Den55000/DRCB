#ifndef CV
#define CV

#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "Arduino.h"
#include "fb_gfx.h"

#include "esp_heap_caps.h"

struct HSL
{
	uint16_t H;
	uint8_t S;
	uint8_t L;
};

camera_config_t config;
uint16_t *frameHsl = NULL;
size_t frameSize = 0;
extern char board[8][8];
extern int counts[];
extern int countscounter;
extern int flashOccupants;
extern uint8_t * jpg_buf;
extern size_t jpg_len;

struct color{
  int count;
  uint16_t tolLow;
  uint16_t tolHigh;
};

struct piece{
  char pchar;
  int color1; // index of corresponding color at struct color colors[]
  int color2; // index of corresponding color at struct color colors[], set to -1 for none
};

struct color colors[] = {
  { 0, 340, 20 },   // red
  { 0, 260, 330 },  // pink
  { 0, 215, 250 },  // dblue
  { 0, 190, 210 },  // lblue
  { 0, 100, 150 }   // green
};

struct piece pieces[] = {
  { 'R', 0,  2 }, // black rook,   red+dblue
  { 'r', 0,  3 }, // white rook,   red+lblue
  { 'B', 0,  1 }, // black bishop, red+pink
  { 'b', 0,  4 }, // white bishop, red+green
  { 'N', 2,  3 }, // black knight, dblue+lblue
  { 'n', 2,  1 }, // white knight, dblue+pink
  { 'Q', 2,  4 }, // black queen,  dblue+green
  { 'q', 3,  4 }, // white queen,  lblue+green
  { 'K', 3,  1 }, // black king,   lblue+pink
  { 'k', 1,  4 }, // white king,   pink+green
  { 'P', 1, -1 }, // black pawn,   pink
  { 'p', 3, -1 }  // white pawn,   lblue
};

void calibrateCamera(){
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
}

void configInitCamera(){
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565; //YUV422,GRAYSCALE,RGB565,JPEG

  config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA : 320x240|352x288|640x480|800x600|1024x768|1280x1024|1600x1200
  config.jpeg_quality = 10; //0-63 lower number means higher quality
  config.fb_count = 1;
  
  // Initialize the Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 1);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
}

void getSquare(uint16_t *square, int r, int c) {
  const int boardTop = 35;
  const int boardLeft = 75;
  const int squareSize = 54;
  
  int startY = boardTop + r * squareSize;
  int startX = boardLeft + c * squareSize;
  
  int k = 0;
  for (int y = startY; y < startY + squareSize; y++) {
    for (int x = startX; x < startX + squareSize; x++) {
      int index = y * 640 + x;
      square[k++] = frameHsl[index];
    }
  }
}

bool isBetween(uint16_t hue, uint16_t low, uint16_t high){
  if(low > high){
    if(hue >= low && hue <= 360){
      return true;
    }
    if(hue >= 0 && hue <= high){
      return true;
    }
  }
  if(low < high){
    if(hue >= low && hue <= high){
      return true;
    }
  }
  return false;
}

char getColors(uint16_t * square, int r, int c){
  #ifdef CV_DEBUG_HUES
  Serial.printf("board[%d][%d], ", r, c);
  #endif
  for(int j = 0; j < sizeof(colors) / sizeof(colors[0]); j++){
    colors[j].count = 0;
  }
  
  for(int i = 0; i < 2916; i++){
    uint16_t hue = square[i];
    for(int j = 0; j < sizeof(colors) / sizeof(colors[0]); j++){
      (isBetween(hue, colors[j].tolLow, colors[j].tolHigh)) ? colors[j].count++ : 0;
    }
    #ifdef CV_DEBUG_HUES
    Serial.printf("square[%d]: %d, ", i, hue);
    #endif
  }
  #ifdef CV_DEBUG_HUES
  Serial.printf("\n");
  #endif

  #ifdef CV_DEBUG_COUNTS
  #ifndef CV_DEBUG_COUNTS_NO_SERIAL
  Serial.printf("board[%d][%d], ", r, c);
  #endif
  for(int j = 0; j < sizeof(colors) / sizeof(colors[0]); j++){
    #ifndef CV_DEBUG_COUNTS_NO_SERIAL
    Serial.printf("colors[%d].count: %d, ", j, colors[j].count);
    #endif
    counts[countscounter] = colors[j].count; countscounter++;
  }
  #ifndef CV_DEBUG_COUNTS_NO_SERIAL
  Serial.printf("\n");
  #endif
  #endif

  for(int i = 0; i < sizeof(pieces) / sizeof(pieces[0]); i++){
    if(pieces[i].color2 >= 0){
      if(colors[pieces[i].color1].count > 750 && colors[pieces[i].color2].count > 750){
        return pieces[i].pchar;
      }
    }

    if(pieces[i].color2 < 0 && colors[pieces[i].color1].count > 1500){
      return pieces[i].pchar;
    }
  }
  return ' ';
}

uint8_t getRed(uint16_t pixel) {
  return (uint8_t)((pixel >> 11) & 0b0000000000011111);
}
uint8_t getGreen(uint16_t pixel) {
  return (uint8_t)((pixel >> 5)  & 0b0000000000111111);
}
uint8_t getBlue(uint16_t pixel) {
  return (uint8_t)(pixel &         0b0000000000011111);
}

float Min(float a, float b) {
	return a <= b ? a : b;
}

float Max(float a, float b) {
	return a >= b ? a : b;
}

struct HSL rgb2hsl(uint16_t rgb){
  struct HSL hsl;
  float r = (getRed(rgb) / 31.0f);
	float g = (getGreen(rgb) / 63.0f);
	float b = (getBlue(rgb) / 31.0f);

	float min = Min(Min(r, g), b);
	float max = Max(Max(r, g), b);
	float delta = max - min;

	hsl.L = (uint8_t)(((max + min) / 2.0f)*100.0f);

	if (delta == 0.0f)
	{
		hsl.H = 0;
		hsl.S = 0;
	}
	else
	{
    if (hsl.L < 50) {
      hsl.S = (uint8_t)((delta / (max + min)) * 100.0f);
    } else {
      hsl.S = (uint8_t)((delta / (2.0f - max - min)) * 100.0f);
    }
		float hue;

    if (max == r) {
      hue = ((g - b) / delta);
    } else if (max == g) {
      hue = (2.0f + (b - r) / delta);
    } else { // max == b
      hue = (4.0f + (r - g) / delta);
    }
    
    hue *= 60.0f; // Convert to degrees
    if (hue < 0) hue += 360.0f;
    
    hsl.H = (uint16_t)hue;
	}
  return hsl;
}

void getFrame(){
  calibrateCamera();

  flashOccupants++;
  analogWrite(GPIO_NUM_4, 255);
  delay(75);
  
  camera_fb_t * fb = esp_camera_fb_get();

  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  size_t pixel_count = fb->width * fb->height;

  if(frameHsl == NULL || frameSize != pixel_count) {
    if(frameHsl != NULL) {
      heap_caps_free(frameHsl);
      frameHsl = NULL;
    }
    //frameHsl = (struct HSL*)malloc(pixel_count * sizeof(struct HSL));
    frameHsl = (uint16_t*) heap_caps_malloc(pixel_count * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if(frameHsl == NULL) {
      Serial.println("Failed to allocate HSL buffer!");
      esp_camera_fb_return(fb);
      return;
    }
    frameSize = pixel_count;
  }

  for(size_t i = 0; i < pixel_count; i++) {
    uint16_t pixel = (fb->buf[i * 2] << 8) | fb->buf[i * 2 + 1];
    frameHsl[i] = rgb2hsl(pixel).H;
  }

  free(jpg_buf);
  bool converted = fmt2jpg(fb->buf, fb->len, fb->width, fb->height, PIXFORMAT_RGB565, 80, &jpg_buf, &jpg_len);
  if(!converted) {
    esp_camera_fb_return(fb);
    Serial.println("Jpg convertion failed");
    return;
  }

  esp_camera_fb_return(fb);
  flashOccupants--;
  if(flashOccupants == 0){
    analogWrite(GPIO_NUM_4, 0);
    delay(75);
  }
}

void ChessBoardCV(){
  getFrame();
  static uint16_t square[2916];
  countscounter = 0;
  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
      getSquare(square, i, j);
      board[i][j]=getColors(square, i, j);
    }
  }
}

#endif