#include "setup.h"
#include <JPEGDEC.h>
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
#include <WiFi.h>
#include <HTTPClient.h>

uint16_t  dmaBuffer1[128*16]; // Toggle buffer for 128*16 MCU block, 4096bytes
uint16_t  dmaBuffer2[128*16]; // Toggle buffer for 128*16 MCU block, 4096bytes
uint16_t* dmaBufferPtr = dmaBuffer1;
bool dmaBufferSel = 0;

TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
JPEGDEC jpeg;

volatile uint16_t frame_count = 0;

// This next function will be called during decoding of the jpeg file to render each
// 16x16 or 8x8 image tile (Minimum Coding Unit) to the TFT.
int tft_output(JPEGDRAW *pDraw)
{
  
  uint16_t x = pDraw->x;
  uint16_t y = pDraw->y;
  uint16_t w = pDraw->iWidth;
  uint16_t h = pDraw->iHeight;
  uint16_t* bitmap = pDraw->pPixels;
   
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // Double buffering is used, the bitmap is copied to the buffer by pushImageDMA() the
  // bitmap can then be updated by the jpeg decoder while DMA is in progress
  if (dmaBufferSel) dmaBufferPtr = dmaBuffer2;
  else dmaBufferPtr = dmaBuffer1;
  dmaBufferSel = !dmaBufferSel; // Toggle buffer selection
  //  pushImageDMA() will clip the image block at screen boundaries before initiating DMA
  tft.pushImageDMA(x, y, w, h, bitmap, dmaBufferPtr); // Initiate DMA - blocking only if last DMA is not complete
  // The DMA transfer of image block to the TFT is now in progress...
  
  // Return 1 to decode next block.
  return 1;
}

void setup()
{
  Serial.begin(115200);
  Serial.println("esp32remote");

  // Initialise the TFT
  tft.begin();
  tft.setRotation(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.initDMA(); // To use SPI DMA you must call initDMA() to setup the DMA engine

  initNetwork();
  
  xTaskCreatePinnedToCore(
    loopNetwork,    // Function that should be called
    "loopNetwork",   // Name of the task (for debugging)
    1024*50,            // Stack size (bytes)
    NULL,            // Parameter to pass
    1,               // Task priority
    NULL,             // Task handle,
    0                // CPU
  );
}

void loop() {
  delay(1000);
  Serial.printf("FPS: %d\n", frame_count);
  frame_count = 0;
}

void initNetwork() {

  Serial.println("Connecting to: ");
  Serial.println(SSID);

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP().toString());
}

void loopNetwork(void* arg) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WIFI not connected!");
    return;
  }

  HTTPClient http;

  http.begin(STREAM_URL);
  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("HTTP GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    return;
  }
  size_t buff_len = 40*1024;
  uint8_t buff[buff_len] = { 0 };

  WiFiClient * stream = http.getStreamPtr();

  // Must use startWrite first so TFT chip select stays low during DMA and SPI channel settings remain configured
  tft.startWrite();
  
  size_t read_len = 0;
  while (http.connected()) {
    size_t size = stream->available();
    if (!size) {
      delay(10);
      continue;
    }
    if (read_len >= buff_len) {
      Serial.printf("Not enough buffer available. Needed: %d\n", read_len);
      read_len = 0;
      continue;
    }
    size_t count = stream->readBytes(buff + read_len, min(size, buff_len - read_len));
    size_t next_read_len = read_len + count;
    int jpeg_end = -1;
    for (int i = read_len; i<next_read_len; ++i) {
      if (i == 0) {
        continue;
      }
      // Note: Always look back one character in case the 0xFF was received
      // in the previous call to readBytes.
      if (buff[i-1] == 0xFF && buff[i] == 0xD9) {
        jpeg_end = i+1;
        break;
      }
    }
    if (jpeg_end > 0) {
      int jpeg_start = -1;
      for (int i = 0; i<jpeg_end-1; ++i) {
        if (buff[i] == 0xFF && buff[i+1] == 0xD8) {
          jpeg_start = i;
          break;
        }
      }
      if (jpeg_start < 0) {
        Serial.println("Could not find jpeg start.");
      } else {
        if (!drawJpeg(buff+jpeg_start, jpeg_end-jpeg_start)) {
          Serial.println("Error drawing jpeg.");
        }        
      }
      next_read_len -= jpeg_end;
      memcpy(buff, buff + jpeg_end, next_read_len);
    }
    read_len = next_read_len;
  }

  // Must use endWrite to release the TFT chip select and release the SPI channel
  tft.endWrite();
  Serial.println();
  Serial.print("[HTTP] connection closed.\n"); 
  http.end();

  // TODO: Restart the task as otherwise ESP32 panics.
  // TODO: Check the error message and find out why to learn more.
}

bool drawJpeg(uint8_t* buf, size_t len) {
  if (!jpeg.openRAM(buf, len, &tft_output)) {
    return false;
  }
  ++frame_count;  
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  bool ok = jpeg.decode(0,0,0);
  jpeg.close();
  return ok;
}
