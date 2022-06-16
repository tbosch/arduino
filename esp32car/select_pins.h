/*

 _                      _       _   _
| |                    (_)     | | | |
| |      ___ __      __ _  ___ | |_| |  ___
| |     / _ \\ \ /\ / /| |/ __||  _  | / _ \
| |____|  __/ \ V  V / | |\__ \| | | ||  __/
\_____/ \___|  \_/\_/  |_||___/\_| |_/ \___|

Compatible with all TTGO camera products, written by LewisHe
03/28/2020
*/

#define PWDN_GPIO_NUM       -1
#define RESET_GPIO_NUM      -1
#define XCLK_GPIO_NUM       32
#define SIOD_GPIO_NUM       13
#define SIOC_GPIO_NUM       12

#define Y9_GPIO_NUM         39
#define Y8_GPIO_NUM         36
#define Y7_GPIO_NUM         23
#define Y6_GPIO_NUM         18
#define Y5_GPIO_NUM         15
#define Y4_GPIO_NUM         4
#define Y3_GPIO_NUM         14
#define Y2_GPIO_NUM         5

#define VSYNC_GPIO_NUM      27
#define HREF_GPIO_NUM       25
#define PCLK_GPIO_NUM       19

#define I2C_SDA             21
#define I2C_SCL             22

#define BUTTON_1            34

#define SSD130_MODLE_TYPE   0   // 0 : GEOMETRY_128_64  // 1: GEOMETRY_128_32

#define AS312_PIN           33

#define ENABLE_IP5306
#define IP5306_ADDR 0X75
#define IP5306_REG_SYS_CTL0 0x00

#define SSD1306_ADDRESS 0x3c
