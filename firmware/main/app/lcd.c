#include "lcd.h"

uint8_t _lcd_char_width;
uint8_t _lcd_char_height;
uint8_t _lcd_height;
uint8_t _lcd_width;

void lcd_init(i2c_port_t i2c_master_port_num,
              gpio_num_t i2c_master_sda_gpio_num,
              gpio_num_t i2c_master_scl_gpio_num,
              uint32_t i2c_master_requncy,
              size_t i2c_rx_buffer_length,
              size_t i2c_tx_buffer_length,
              uint8_t lcd_char_widht,
              uint8_t lcd_char_height,
              uint8_t lcd_widht,
              uint8_t lcd_height)
{
    printf("Init i2c\r\n");
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = i2c_master_sda_gpio_num;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = i2c_master_scl_gpio_num;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = i2c_master_requncy;
    i2c_param_config(i2c_master_port_num, &conf);
    i2c_driver_install(i2c_master_port_num, conf.mode, i2c_rx_buffer_length, i2c_tx_buffer_length, 0);
    printf("Init lcd\r\n");
    _lcd_width = lcd_widht;
    _lcd_height = lcd_height;
    _lcd_char_width = lcd_char_widht;
    _lcd_char_height = lcd_char_height;
    SSD1306_Init();
}

void lcd_print(char *message)
{
    uint8_t length = strlen(message);
    uint8_t messageWidth = length * _lcd_char_width;
    uint8_t messageHeight = _lcd_char_height;
    printf("Message width %d, mesage height %d\r\n", messageWidth, messageHeight);
    SSD1306_Fill(SSD1306_COLOR_BLACK);
    SSD1306_GotoXY((_lcd_width / 2) - (messageWidth / 2), (_lcd_height / 2) - (messageHeight / 2));
    SSD1306_Puts(message, &Font_11x18, SSD1306_COLOR_WHITE);
    SSD1306_UpdateScreen();
}
