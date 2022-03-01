#include "setup.h"
#include <JPEGDEC.h>
#include "SPI.h"
#include <TFT_eSPI.h>              // Hardware-specific library
#include <WiFi.h>
#include <HTTPClient.h>

#define RECEIVE_BUF_SIZE 40*1024

uint16_t  dmaBuffer1[128*16]; // Toggle buffer for 128*16 MCU block, 4096bytes
uint16_t  dmaBuffer2[128*16]; // Toggle buffer for 128*16 MCU block, 4096bytes
uint16_t* dmaBufferPtr = dmaBuffer1;
bool dmaBufferSel = 0;

TFT_eSPI tft = TFT_eSPI();         // Invoke custom library
JPEGDEC jpeg;

volatile uint16_t frame_count = 0;

typedef struct
{
    uint8_t *buf;
    int len;
} DECODE_MSG;

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

void loopNetwork(void* arg) {
  // Note: we never call Wifi.end() as this loops never ends.
  WiFi.begin(SSID, PASSWORD);    
  HTTPClient http;
  // Note: We never call http.end() as this loops never ends.
  http.begin(STREAM_URL);
  // Must use startWrite first so TFT chip select stays low during DMA and SPI channel settings remain configured.
  // Note: we never call tft.endWrite() as this loops never ends.
  tft.startWrite();
  
  uint8_t* buf = (uint8_t*)malloc(RECEIVE_BUF_SIZE);
  
  int read_len = 0;
  
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.printf("[RECEIVE] Waiting to connect to %s\n.", SSID);      
      continue;
    }
    if (!http.connected()) {
      Serial.printf("[RECEIVE] Reading url %s\n.", STREAM_URL);
      int httpCode = http.GET();
      if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[RECEIVE] HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
        delay(500);
        continue;
      }
    }  
    
    WiFiClient* stream = http.getStreamPtr();
    
    int remaining_buf_size = RECEIVE_BUF_SIZE - read_len;
    if (remaining_buf_size <= 0) {
      Serial.printf("[RECEIVE] Not enough buffer available. Needed: %d\n", read_len);
      read_len = 0;
      continue;
    }
    int available = stream->available();
    if (available <= 0) {
      delay(10);
      continue;
    }
    int count = stream->readBytes(buf + read_len, min(available, remaining_buf_size));
    int next_read_len = read_len + count;
    // Note: Always look back one character in case the 0xFF was received
    // in the previous call to readBytes.
    int jpeg_end = findJpegEnd(buf, next_read_len, max(0, read_len-1));
    if (jpeg_end > 0) {
      int jpeg_start = findJpegStart(buf, jpeg_end);
      if (jpeg_start < 0) {
        Serial.println("[RECEIVE] Could not find jpeg start.");
      } else {
        if (!drawJpeg(buf+jpeg_start, jpeg_end-jpeg_start)) {
          Serial.println("[RECEIVE] Error drawing jpeg.");
        }
      }
      next_read_len -= jpeg_end;
      memcpy(buf, buf + jpeg_end, next_read_len);
    }
    read_len = next_read_len;
  }
}

int findJpegStart(uint8_t* buf, size_t len) {
  for (int i = 0; i<len-1; ++i) {
    if (buf[i] == 0xFF && buf[i+1] == 0xD8) {
      return i;
    }
  }
  return -1;  
}

int findJpegEnd(uint8_t* buf, size_t len, int pos) {
  for (int i = pos; i<len-1; ++i) {
    if (buf[i] == 0xFF && buf[i+1] == 0xD9) {
      return i+2;
    }
  }
  return -1;  
}

bool drawJpeg(uint8_t* buf, size_t len) {
  if (!jpeg.openRAM(buf, len, &tft_output)) {
    Serial.println("[JPEG] Could not call openRAM.");
    return false;
  }
  ++frame_count;  
  jpeg.setPixelType(RGB565_BIG_ENDIAN);
  bool ok = jpeg.decode(0,0,0);
  jpeg.close();
  if (!ok) {
    Serial.println("[JPEG] Could not decode.");
  }
  return ok;
}

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
