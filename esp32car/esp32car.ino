#include <WiFi.h>
#include <Wire.h>
#include "esp_camera.h"
#include "select_pins.h"
#include <OneButton.h>
#include "SSD1306.h"
#include "OLEDDisplayUi.h"
#include "SCMD.h"
#include "SCMD_config.h" //Contains #defines for common SCMD register names and values

String macAddress = "TTGO-CAMERA-08:19";
IPAddress    apIP(2, 2, 2, 1);
String ipAddress = "";

extern void startCameraServer();
extern int motor_speed_left;
extern int motor_speed_right;

OneButton button(BUTTON_1, true);

SSD1306 oled(SSD1306_ADDRESS, I2C_SDA, I2C_SCL, (OLEDDISPLAY_GEOMETRY)SSD130_MODLE_TYPE);
OLEDDisplayUi ui(&oled);
SCMD motorDriver;

bool setupSensor()
{
    pinMode(AS312_PIN, INPUT);
    return true;
}


bool deviceProbe(uint8_t addr)
{
    Wire.beginTransmission(addr);
    return Wire.endTransmission() == 0;
}

bool setupDisplay()
{
    static FrameCallback frames[] = {
        [](OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
        {
            display->setTextAlignment(TEXT_ALIGN_CENTER);
            display->setFont(ArialMT_Plain_10);
            display->drawString(64 + x, 0 + y, macAddress);
            display->drawString(64 + x, 10 + y, ipAddress);

            if (digitalRead(AS312_PIN)) {
                display->drawString(64 + x, 40 + y, "AS312 Trigger");
            }
        },
        [](OLEDDisplay * display, OLEDDisplayUiState * state, int16_t x, int16_t y)
        {
            display->setTextAlignment(TEXT_ALIGN_CENTER);
            display->setFont(ArialMT_Plain_10);
            // if (oled.getHeight() == 32) {
            display->drawString( 64 + x, 0 + y, "Camera Ready! Use");
            display->drawString(64 + x, 10 + y, "http://" + ipAddress );
            display->drawString(64 + x, 16 + y, "to connect");
            // } else {
        }
    };

    if (!deviceProbe(SSD1306_ADDRESS))return false;
    oled.init();
    // Wire.setClock(100000);  //! Reduce the speed and prevent the speed from being too high, causing the screen
    oled.setFont(ArialMT_Plain_16);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    // delay(50);
    oled.drawString( oled.getWidth() / 2, oled.getHeight() / 2 - 10, "LilyGo CAM");
    oled.display();
    ui.setTargetFPS(30);
    ui.setIndicatorPosition(BOTTOM);
    ui.setIndicatorDirection(LEFT_RIGHT);
    ui.setFrameAnimation(SLIDE_LEFT);
    ui.setFrames(frames, sizeof(frames) / sizeof(frames[0]));
    ui.setTimePerFrame(6000);
    ui.disableIndicator();
    return true;
}


bool setupPower()
{
    if (!deviceProbe(IP5306_ADDR))return false;
    bool en = true;
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(IP5306_REG_SYS_CTL0);
    if (en)
        Wire.write(0x37); // Set bit1: 1 enable 0 disable boost keep on
    else
        Wire.write(0x35); // 0x37 is default reg value
    return Wire.endTransmission() == 0;
}


bool setupCamera()
{
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
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;
    //init with high specs to pre-allocate larger buffers
    if (psramFound()) {
        config.frame_size = FRAMESIZE_UXGA;
        config.jpeg_quality = 10;
        config.fb_count = 2;
    } else {
        config.frame_size = FRAMESIZE_SVGA;
        config.jpeg_quality = 12;
        config.fb_count = 1;
    }

    // camera init
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x\n", err);
        return false;
    }

    sensor_t *s = esp_camera_sensor_get();
    //initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID) {
        s->set_vflip(s, 1);//flip it back
        s->set_brightness(s, 1);//up the blightness just a bit
        s->set_saturation(s, -2);//lower the saturation
    }
    //drop down frame size for higher initial frame rate
    s->set_framesize(s, FRAMESIZE_QVGA);

    return true;
}

void setupNetwork()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(macAddress.c_str());
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    ipAddress = WiFi.softAPIP().toString();
}

void setupButton()
{
    button.attachClick([]() {
        static bool en = false;
        sensor_t *s = esp_camera_sensor_get();
        en = en ? 0 : 1;
        s->set_vflip(s, en);
        if (en) {
            oled.resetOrientation();
        } else {
            oled.flipScreenVertically();
        }
        // delay(200);
    });

    button.attachDoubleClick([]() {
        if (PWDN_GPIO_NUM > 0) {
            pinMode(PWDN_GPIO_NUM, PULLUP);
            digitalWrite(PWDN_GPIO_NUM, HIGH);
        }

        ui.disableAutoTransition();
        oled.setTextAlignment(TEXT_ALIGN_CENTER);
        oled.setFont(ArialMT_Plain_10);
        oled.clear();
        oled.drawString(oled.getWidth() / 2 - 5, oled.getHeight() / 2 - 20, "Deepsleep");
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2 - 10, "Set to be awakened by");
        oled.drawString(oled.getWidth() / 2, oled.getHeight() / 2, "a key press");
        oled.display();
        delay(3000);
        oled.clear();
        oled.displayOff();

        // esp_sleep_enable_ext0_wakeup((gpio_num_t )BUTTON_1, LOW);
        esp_sleep_enable_ext1_wakeup(((uint64_t)(((uint64_t)1) << BUTTON_1)), ESP_EXT1_WAKEUP_ALL_LOW);
        esp_deep_sleep_start();
    });
}

void setupMotor() {
  motorDriver.settings.commInterface = I2C_MODE;
  motorDriver.settings.I2CAddress = 0x5D;  
  motorDriver.begin();
  motorDriver.enable();  
}

void setup()
{

    Serial.begin(115200);

    Wire.begin(I2C_SDA, I2C_SCL);

    bool status;
    status = setupDisplay();
    Serial.print("setupDisplay status "); Serial.println(status);

    status = setupPower();
    Serial.print("setupPower status "); Serial.println(status);

    status = setupSensor();
    Serial.print("setupSensor status "); Serial.println(status);

    status = setupCamera();
    Serial.print("setupCamera status "); Serial.println(status);
    if (!status) {
        delay(10000); esp_restart();
    }

    setupButton();

    setupNetwork();

    startCameraServer();

    setupMotor();

    Serial.print("Camera Ready! Use 'http://");
    Serial.print(ipAddress);
    Serial.println("' to connect");
}

void drive(int speedLeft, int speedRight) {
  int absLeft = min(abs(speedLeft), 255);
  int absRight = min(abs(speedRight), 255);
  motorDriver.setDrive(0, speedLeft > 0, absLeft);
  motorDriver.setDrive(1, speedRight > 0, absRight);
}

void loop()
{
    if (ui.update()) {
        button.tick();
    }
    drive(motor_speed_left, motor_speed_right);
}
