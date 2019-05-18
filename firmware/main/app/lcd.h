#include "esp_types.h"

#include "libs/ssd1306.h"
#include "libs/fonts.h"
#include "libs/xi2c.h"

#define I2C_MASTER_NUM I2C_NUM_1      /*!< I2C port number for master dev */
#define I2C_MASTER_SCL_IO 22          /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO 21          /*!< gpio number for I2C master data  */
#define I2C_MASTER_FREQNCY 100000     /*!< I2C master clock frequency */
#define I2C_MASTER_RX_BUFFER_LENGTH 0 /*!< I2C master do not need buffer */
#define I2C_MASTER_TX_BUFFER_LENGTH 0 /*!< I2C master do not need buffer */

#define LCD_CHAR_HEIGHT 18
#define LCD_CHAR_WIDTH 11
#define LCD_WIDTH 128
#define LCD_HEIGHT 64

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