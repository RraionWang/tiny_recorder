# ifndef PIN_CFG_H
#define PIN_CFG_H

// 引脚定义（RFID）
#define RC522_SPI_BUS_GPIO_MISO    (15)
#define RC522_SPI_BUS_GPIO_MOSI    (41)
#define RC522_SPI_BUS_GPIO_SCLK    (42)
#define RC522_SCANNER_GPIO_SDA     (45)
#define RC522_SCANNER_GPIO_RST     (17)

//sd卡

#define SD_CLK (39)
#define SD_CMD (38)
#define SD_D0 (40)
#define SD_D1 (21)
#define SD_D2 (48)
#define SD_D3 (47)

/* LCD pins */
#define EXAMPLE_LCD_GPIO_SCLK       (GPIO_NUM_11)
#define EXAMPLE_LCD_GPIO_MOSI       (GPIO_NUM_10)
#define EXAMPLE_LCD_GPIO_RST        (GPIO_NUM_8)
#define EXAMPLE_LCD_GPIO_DC         (GPIO_NUM_9)
#define EXAMPLE_LCD_GPIO_CS         (GPIO_NUM_3)
#define EXAMPLE_LCD_GPIO_BL         (GPIO_NUM_7)

//编码器
#define ENCODER_A_PIN       GPIO_NUM_2
#define ENCODER_B_PIN       GPIO_NUM_1
#define ENCODER_BTN_PIN     GPIO_NUM_46


//inmp441 
// GPIO 引脚定义（按你实际接线修改）
#define INMP441_I2S_BCLK_PIN        GPIO_NUM_4
#define INMP441_I2S_WS_PIN          GPIO_NUM_5
#define INMP441_I2S_DIN_PIN         GPIO_NUM_6


#endif