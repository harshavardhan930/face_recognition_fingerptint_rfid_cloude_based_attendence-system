#include "esp_camera.h"
#include <WiFi.h>
#include <TFT_eSPI.h>  // Include TFT_eSPI library

// Camera model selection
#define CAMERA_MODEL_AI_THINKER  // Has PSRAM
#include "camera_pins.h"

// WiFi credentials
const char *ssid = "ybr";
const char *password = "harsha555";

// TFT display pinout (ILI9341)
#define TFT_MOSI 13    ZZ
#define TFT_SCLK 14    Z"|
#define TFT_CS   15    Z#}
#define TFT_DC    2    Z'�
#define TFT_RST   12   Z(�

TFT_eSPI tft = TFT_eSPI();  // TFT instance

void startCameraServer();
void setupLedFlash(int pin);

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Initialize TFT display
  tft.init();
  tft.setRotation(3);  // Adjust orientation
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Initializing...");

  // Camera configuration
  camera_config_t config;
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
  config.frame_size = FRAMESIZE_QVGA;  // Low resolution for display and streaming
  config.pixel_format = PIXFORMAT_RGB565;  // RGB565 for TFT display
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  // Initialize the camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera initialization failed!");
    tft.println("Camera Init Failed!");
    return;
  }

  // Initialize WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(10, 10);
  tft.println("Camera Ready!");
  tft.setCursor(10, 50);
  tft.print("IP: ");
  tft.println(WiFi.localIP());

  // Start the web server
  startCameraServer();
}

void loop() {
  // Capture a frame
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Frame capture failed");
    tft.setCursor(10, 100);
    tft.println("Frame Capture Failed!");
    return;
  }

  // Display the image on TFT (if RGB565 format)
  if (fb->format == PIXFORMAT_RGB565) {
    tft.pushImage(0, 0, fb->width, fb->height, (uint16_t *)fb->buf);
  }

  // Return framebuffer
  esp_camera_fb_return(fb);

  delay(50);  // Small delay to reduce load
}
