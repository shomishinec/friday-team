#include "esp_types.h"

#include "libs/ssd1306.h"
#include "libs/fonts.h"
#include "libs/xi2c.h"

void lcd_init(i2c_port_t i2c_master_port_num,
              gpio_num_t i2c_master_sda_gpio_num,
              gpio_num_t i2c_master_scl_gpio_num,
              uint32_t i2c_master_requncy,
              size_t i2c_rx_buffer_length,
              size_t i2c_tx_buffer_length,
              uint8_t lcd_char_widht,
              uint8_t lcd_char_height,
              uint8_t lcd_widht,
              uint8_t lcd_height);
void lcd_print(char *message);