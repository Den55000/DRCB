#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"

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

struct HSL
{
	uint16_t H;
	uint8_t S;
	uint8_t L;
};

camera_config_t config;
uint16_t *scr = NULL;
uint16_t *frameHsl = NULL;
size_t frameSize = 0;

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
  config.xclk_freq_hz = 8000000;
  config.pixel_format = PIXFORMAT_RGB565; //YUV422,GRAYSCALE,RGB565,JPEG

  // Select lower framesize if the camera doesn't support PSRAM
  if(psramFound()){
    config.frame_size = FRAMESIZE_VGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA : 320x240|352x288|640x480|800x600|1024x768|1280x1024|1600x1200
    config.jpeg_quality = 10; //10-63 lower number means higher quality
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = 25;
    config.fb_count = 1;
  }
  
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

void calibrateCamera(){
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();
  esp_camera_fb_return(fb);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  pinMode(33, OUTPUT);
  digitalWrite(33, LOW);
  ledcSetup(7, 50000, 9);
  ledcAttachPin(4, 7);
  setLamp(0);
  
  Serial.begin(115200);
  delay(1000);
  configInitCamera();
  delay(1000);
  Serial.println("setup() complete");
}

void getFrame(){
  calibrateCamera();

  setLamp(100);
  delay(75);
  flashLED(75);
  
  camera_fb_t * fb = esp_camera_fb_get();

  if(!fb) {
    Serial.println("Camera capture failed");
    return;
  }

  size_t pixel_count = fb->width * fb->height;
  
  if(frameHsl == NULL || frameSize != pixel_count) {
    if(frameHsl != NULL) {
      free(frameHsl);
    }
    frameHsl = (uint16_t*)malloc(pixel_count * sizeof(uint16_t));
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
  esp_camera_fb_return(fb);
  setLamp(0);
}

void ChessBoardCV(){
  getFrame();
  uint16_t * square = (uint16_t*)malloc(2916*sizeof(uint16_t));
  char board[8][8];

  for(int i = 0; i < 8; i++){
    for(int j = 0; j < 8; j++){
      getSquare(square, i, j);
      board[i][j]=getColors(square, i, j);
    }
  }

  free(square);
}

void getSquare(uint16_t *square, int r, int c) {
  const int boardTop = 35;
  const int boardLeft = 75;
  const int squareSize = 54;
  const int boardWidth = 432;
  const int boardHeight = 432;
  
  // Starting coordinates for this square
  int startY = boardTop + r * squareSize;
  int startX = boardLeft + c * squareSize;
  
  int k = 0;
  for (int y = startY; y < startY + squareSize; y++) {
    for (int x = startX; x < startX + squareSize; x++) {
      // Calculate index in frameHsl array
      int index = y * 640 + x;
      square[k++] = frameHsl[index];
    }
  }
}

char getColors(uint16_t * square, int r, int c){
  int n_Blue = 0;   // 215 - 225
  int n_Pink = 0;   // 265 - 340

  int n_dBlue = 0;  // 225 - 245
  int n_lBlue = 0;  // 175 - 210
  int n_Yellow = 0; //  55 -  75
  int n_lGreen = 0; //  75 - 100
  int n_dGreen = 0; // 120 - 170
  int n_Orange = 0; // 350 - 0 - 35

  for(int i = 0; i < 2916; i++){
      uint16_t hue = square[i];
      (isBetween(hue, 215, 225)) ? n_Blue++   : 0;
      (isBetween(hue, 265, 340)) ? n_Pink++   : 0;

      (isBetween(hue, 225, 245)) ? n_dBlue++  : 0;
      (isBetween(hue, 175, 210)) ? n_lBlue++  : 0;
      (isBetween(hue,  55,  75)) ? n_Yellow++ : 0;
      (isBetween(hue,  75, 100)) ? n_lGreen++ : 0;
      (isBetween(hue, 120, 170)) ? n_dGreen++ : 0;
      (isBetween(hue, 350, 360) || isBetween(hue, 0, 35)) ? n_Orange++ : 0;
  }
  
  Serial.printf("board[%d][%d], Blue: %d, Pink: %d, DBlue: %d, LBlue: %d, Yellow: %d, LGreen: %d, DGreen: %d, Orange: %d \n", r, c, n_Blue, n_Pink, n_dBlue, n_lBlue, n_Yellow, n_lGreen, n_dGreen, n_Orange);

  if(n_Blue > 900){
    if (n_dBlue > 900)  { return 'n'; }
    if (n_lBlue > 900)  { return 'b'; }
    if (n_Yellow > 900) { return 'p'; }
    if (n_lGreen > 900) { return 'q'; }
    if (n_dGreen > 900) { return 'k'; }
    if (n_Orange > 900) { return 'r'; }
  }
  else if(n_Pink > 900){
    if (n_dBlue > 900)  { return 'N'; }
    if (n_lBlue > 900)  { return 'B'; }
    if (n_Yellow > 900) { return 'P'; }
    if (n_lGreen > 900) { return 'Q'; }
    if (n_dGreen > 900) { return 'K'; }
    if (n_Orange > 900) { return 'R'; }
  }
  else{
    return ' ';
  }
}

bool isBetween(uint16_t hue, uint16_t low, uint16_t high){
  if(hue <= high && hue >= low){
    return true;
  } else{
    return false;
  }
}

void loop() {
  ChessBoardCV();
  delay(2500999);
}


uint8_t getRed(uint16_t pixel) {
  return (uint8_t)(pixel >> 11);
}
uint8_t getGreen(uint16_t pixel) {
  return (uint8_t)((pixel & ~(63519)) >> 5);
}
uint8_t getBlue(uint16_t pixel) {
  return (uint8_t)(pixel & ~(65504));
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

// Notification LED
void flashLED(int flashtime) {
    digitalWrite(33, LOW);  // On at full power.
    delay(flashtime);               // delay
    digitalWrite(33, HIGH); // turn Off
}

// Lamp Control
void setLamp(int newVal) {
    if (newVal != -1) {
        // Apply a logarithmic function to the scale.
        int brightness = round((pow(2,(1+(newVal*0.02)))-2)/6*(pow(2,9)-1));
        ledcWrite(7, brightness);
    }
}