#include "board_pins.h"
#include "hal_interface.h"

#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "driver/rmt_tx.h"
#include "driver/spi_master.h"
#include "driver/uart.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

#define I2C_PORT           I2C_NUM_0
#define LEDC_TIMER         LEDC_TIMER_0
#define LEDC_MODE          LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL       LEDC_CHANNEL_0
#define LEDC_RES_BITS      LEDC_TIMER_10_BIT
#define LEDC_MAX_DUTY      1023.0f

#define MPU_REG_SMPLRT_DIV 0x19
#define MPU_REG_CONFIG     0x1A
#define MPU_REG_GYRO_CFG   0x1B
#define MPU_REG_ACCEL_CFG  0x1C
#define MPU_REG_ACCEL_XOUT 0x3B
#define MPU_REG_PWR_MGMT_1 0x6B
#define MPU_REG_WHO_AM_I   0x75

static uint8_t s_mpu_addr = MPU6050_ADDR_HIGH;
static pcnt_unit_handle_t s_pcnt_unit = NULL;
static spi_device_handle_t s_oled_spi = NULL;
static rmt_channel_handle_t s_rmt_chan = NULL;
static rmt_encoder_handle_t s_rmt_bytes_enc = NULL;
static uint8_t s_oled_fb[128 * 8];
static uint8_t s_prev_mode = 255U;

/* Compact 5x7 uppercase/digit/symbol font (subset for live OLED telemetry) */
static const uint8_t s_font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* 0: ' ' */
    {0x3E,0x51,0x49,0x45,0x3E}, /* 1: '0' */
    {0x00,0x42,0x7F,0x40,0x00}, /* 2: '1' */
    {0x42,0x61,0x51,0x49,0x46}, /* 3: '2' */
    {0x21,0x41,0x45,0x4B,0x31}, /* 4: '3' */
    {0x18,0x14,0x12,0x7F,0x10}, /* 5: '4' */
    {0x27,0x45,0x45,0x45,0x39}, /* 6: '5' */
    {0x3C,0x4A,0x49,0x49,0x30}, /* 7: '6' */
    {0x01,0x71,0x09,0x05,0x03}, /* 8: '7' */
    {0x36,0x49,0x49,0x49,0x36}, /* 9: '8' */
    {0x06,0x49,0x49,0x29,0x1E}, /* 10: '9' */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 11: 'A' */
    {0x7F,0x49,0x49,0x49,0x36}, /* 12: 'B' */
    {0x3E,0x41,0x41,0x41,0x22}, /* 13: 'C' */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 14: 'D' */
    {0x7F,0x49,0x49,0x49,0x41}, /* 15: 'E' */
    {0x7F,0x09,0x09,0x09,0x01}, /* 16: 'F' */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 17: 'G' */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 18: 'H' */
    {0x00,0x41,0x7F,0x41,0x00}, /* 19: 'I' */
    {0x7F,0x40,0x40,0x40,0x40}, /* 20: 'L' */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 21: 'M' */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 22: 'N' */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 23: 'O' */
    {0x7F,0x09,0x09,0x09,0x06}, /* 24: 'P' */
    {0x7F,0x09,0x19,0x29,0x46}, /* 25: 'R' */
    {0x46,0x49,0x49,0x49,0x31}, /* 26: 'S' */
    {0x01,0x01,0x7F,0x01,0x01}, /* 27: 'T' */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 28: 'U' */
    {0x7F,0x20,0x18,0x20,0x7F}, /* 29: 'W' */
    {0x08,0x08,0x08,0x08,0x08}, /* 30: '-' */
    {0x00,0x60,0x60,0x00,0x00}, /* 31: '.' */
    {0x00,0x36,0x36,0x00,0x00}, /* 32: ':' */
    {0x23,0x13,0x08,0x64,0x62}, /* 33: '%' */
};

static int font_index(char c)
{
    if (c >= '0' && c <= '9') return 1 + (c - '0');
    switch (c) {
        case 'A': return 11; case 'B': return 12; case 'C': return 13; case 'D': return 14;
        case 'E': return 15; case 'F': return 16; case 'G': return 17; case 'H': return 18;
        case 'I': return 19; case 'L': return 20; case 'M': return 21; case 'N': return 22;
        case 'O': return 23; case 'P': return 24; case 'R': return 25; case 'S': return 26;
        case 'T': return 27; case 'U': return 28; case 'W': return 29;
        case '-': return 30; case '.': return 31; case ':': return 32; case '%': return 33;
        default:  return 0;
    }
}

static esp_err_t mpu_write_reg(uint8_t addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_write_to_device(I2C_PORT, addr, buf, sizeof(buf), pdMS_TO_TICKS(10));
}

static esp_err_t mpu_read_regs(uint8_t addr, uint8_t reg, uint8_t *dst, size_t len)
{
    return i2c_master_write_read_device(I2C_PORT, addr, &reg, 1, dst, len, pdMS_TO_TICKS(10));
}

static bool init_mpu6050(void)
{
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = PIN_I2C_SDA,
        .scl_io_num = PIN_I2C_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_BUS_FREQ_HZ,
    };
    if (i2c_param_config(I2C_PORT, &conf) != ESP_OK) return false;
    if (i2c_driver_install(I2C_PORT, conf.mode, 0, 0, 0) != ESP_OK) return false;

    /* Probe JP1 closed (0x69) first, then open (0x68) */
    uint8_t who = 0;
    if (mpu_read_regs(MPU6050_ADDR_HIGH, MPU_REG_WHO_AM_I, &who, 1) == ESP_OK) {
        s_mpu_addr = MPU6050_ADDR_HIGH;
    } else if (mpu_read_regs(MPU6050_ADDR_LOW, MPU_REG_WHO_AM_I, &who, 1) == ESP_OK) {
        s_mpu_addr = MPU6050_ADDR_LOW;
    } else {
        return false;
    }

    mpu_write_reg(s_mpu_addr, MPU_REG_PWR_MGMT_1, 0x01); /* Wake, PLL X gyro clock */
    vTaskDelay(pdMS_TO_TICKS(20));
    mpu_write_reg(s_mpu_addr, MPU_REG_SMPLRT_DIV, 0x04); /* 1 kHz / (1+4) = 200 Hz */
    mpu_write_reg(s_mpu_addr, MPU_REG_CONFIG,     0x03); /* 42 Hz DLPF */
    mpu_write_reg(s_mpu_addr, MPU_REG_GYRO_CFG,   0x08); /* +-500 deg/s */
    mpu_write_reg(s_mpu_addr, MPU_REG_ACCEL_CFG,  0x08); /* +-4 g */
    return true;
}

static bool init_pcnt_encoder(void)
{
    pcnt_unit_config_t unit_cfg = {.low_limit = -32768, .high_limit = 32767};
    if (pcnt_new_unit(&unit_cfg, &s_pcnt_unit) != ESP_OK) return false;

    pcnt_glitch_filter_config_t filter_cfg = {.max_glitch_ns = 1000};
    pcnt_unit_set_glitch_filter(s_pcnt_unit, &filter_cfg);

    pcnt_chan_config_t ch_a_cfg = {.edge_gpio_num = PIN_ENC_CHA, .level_gpio_num = PIN_ENC_CHB};
    pcnt_chan_config_t ch_b_cfg = {.edge_gpio_num = PIN_ENC_CHB, .level_gpio_num = PIN_ENC_CHA};
    pcnt_channel_handle_t ch_a = NULL, ch_b = NULL;
    pcnt_new_channel(s_pcnt_unit, &ch_a_cfg, &ch_a);
    pcnt_new_channel(s_pcnt_unit, &ch_b_cfg, &ch_b);

    /* Full 4x quadrature edge counting */
    pcnt_channel_set_edge_action(ch_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE);
    pcnt_channel_set_level_action(ch_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    pcnt_channel_set_edge_action(ch_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE);
    pcnt_channel_set_level_action(ch_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE);

    pcnt_unit_enable(s_pcnt_unit);
    pcnt_unit_clear_count(s_pcnt_unit);
    pcnt_unit_start(s_pcnt_unit);
    return true;
}

static void init_l298n_pwm(void)
{
    gpio_config_t io_cfg = {
        .pin_bit_mask = (1ULL << PIN_MOT_IN1) | (1ULL << PIN_MOT_IN2) | (1ULL << PIN_BUZZER),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_cfg);
    gpio_set_level(PIN_MOT_IN1, 0);
    gpio_set_level(PIN_MOT_IN2, 0);
    gpio_set_level(PIN_BUZZER, 0);

    ledc_timer_config_t timer_cfg = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_RES_BITS,
        .timer_num = LEDC_TIMER,
        .freq_hz = MOT_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer_cfg);

    ledc_channel_config_t ch_cfg = {
        .gpio_num = PIN_MOT_PWM,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ledc_channel_config(&ch_cfg);
}

static void oled_send(bool is_data, const uint8_t *buf, size_t len)
{
    if (s_oled_spi == NULL) return;
    gpio_set_level(PIN_OLED_DC, is_data ? 1 : 0);
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = len * 8U;
    t.tx_buffer = buf;
    spi_device_transmit(s_oled_spi, &t);
}

static void init_oled_ssd1306(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << PIN_OLED_DC) | (1ULL << PIN_OLED_RES),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);
    gpio_set_level(PIN_OLED_RES, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(PIN_OLED_RES, 1);
    vTaskDelay(pdMS_TO_TICKS(10));

    spi_bus_config_t bus = {
        .mosi_io_num = PIN_OLED_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = PIN_OLED_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 1024,
    };
    spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);

    spi_device_interface_config_t dev = {
        .clock_speed_hz = 8000000,
        .mode = 0,
        .spics_io_num = PIN_OLED_CS,
        .queue_size = 4,
    };
    spi_bus_add_device(SPI2_HOST, &dev, &s_oled_spi);

    static const uint8_t init_cmds[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xCF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };
    oled_send(false, init_cmds, sizeof(init_cmds));
}

static void oled_draw_str(int page, int col, const char *str)
{
    if (page < 0 || page >= 8) return;
    while (*str && col <= 122) {
        int idx = font_index(*str++);
        for (int i = 0; i < 5; i++) {
            s_oled_fb[page * 128 + col++] = s_font5x7[idx][i];
        }
        if (col < 128) s_oled_fb[page * 128 + col++] = 0x00;
    }
}

static void init_ws2812_rmt(void)
{
    rmt_tx_channel_config_t tx_cfg = {
        .gpio_num = PIN_WS2812_DATA,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000, /* 100 ns tick */
        .mem_block_symbols = 64,
        .trans_queue_depth = 2,
    };
    if (rmt_new_tx_channel(&tx_cfg, &s_rmt_chan) != ESP_OK) return;

    rmt_bytes_encoder_config_t enc_cfg = {
        .bit0 = {.level0 = 1, .duration0 = 4, .level1 = 0, .duration1 = 8},
        .bit1 = {.level0 = 1, .duration0 = 8, .level1 = 0, .duration1 = 4},
        .flags.msb_first = 1,
    };
    rmt_new_bytes_encoder(&enc_cfg, &s_rmt_bytes_enc);
    rmt_enable(s_rmt_chan);
}

static void ws2812_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_rmt_chan == NULL || s_rmt_bytes_enc == NULL) return;
    uint8_t grb[3] = {g, r, b};
    rmt_transmit_config_t cfg = {.loop_count = 0};
    rmt_transmit(s_rmt_chan, s_rmt_bytes_enc, grb, sizeof(grb), &cfg);
}

bool hal_init(void)
{
    uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0);
    init_l298n_pwm();
    init_pcnt_encoder();
    init_oled_ssd1306();
    init_ws2812_rmt();
    return init_mpu6050();
}

bool hal_mpu6050_read_2d(float *ax_mps2, float *ay_mps2, float *gz_rads)
{
    uint8_t raw[14];
    if (mpu_read_regs(s_mpu_addr, MPU_REG_ACCEL_XOUT, raw, sizeof(raw)) != ESP_OK) {
        return false;
    }
    int16_t ax_raw = (int16_t)((raw[0] << 8) | raw[1]);
    int16_t ay_raw = (int16_t)((raw[2] << 8) | raw[3]);
    int16_t gz_raw = (int16_t)((raw[12] << 8) | raw[13]);

    /* +-4g = 8192 LSB/g; +-500 deg/s = 65.5 LSB/(deg/s) */
    *ax_mps2 = ((float)ax_raw / 8192.0f) * 9.80665f;
    *ay_mps2 = ((float)ay_raw / 8192.0f) * 9.80665f;
    *gz_rads = ((float)gz_raw / 65.5f) * 0.0174532925f;
    return true;
}

int hal_pcnt_read_and_clear(void)
{
    int count = 0;
    if (s_pcnt_unit != NULL) {
        pcnt_unit_get_count(s_pcnt_unit, &count);
        pcnt_unit_clear_count(s_pcnt_unit);
    }
    return count;
}

void hal_l298n_write(float pwm_duty, bool in1_high, bool in2_high)
{
    if (pwm_duty < 0.0f) pwm_duty = 0.0f;
    if (pwm_duty > 1.0f) pwm_duty = 1.0f;
    uint32_t duty_counts = (uint32_t)(pwm_duty * LEDC_MAX_DUTY);

    gpio_set_level(PIN_MOT_IN1, in1_high ? 1 : 0);
    gpio_set_level(PIN_MOT_IN2, in2_high ? 1 : 0);
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty_counts);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

void hal_ui_update(const BalanceTelemetry_T *telem)
{
    static const char *mode_names[] = {"CALIB", "IDLE", "BAL", "FAULT"};
    uint8_t m = (telem->mode <= 3U) ? telem->mode : 3U;

    if (m != s_prev_mode) {
        s_prev_mode = m;
        switch (m) {
            case CUBE_MODE_CALIB:   ws2812_set_rgb(0, 0, 40);  break;
            case CUBE_MODE_IDLE:    ws2812_set_rgb(0, 35, 35); gpio_set_level(PIN_BUZZER, 1); vTaskDelay(pdMS_TO_TICKS(40)); gpio_set_level(PIN_BUZZER, 0); break;
            case CUBE_MODE_BALANCE: ws2812_set_rgb(0, 50, 0);  break;
            default:                ws2812_set_rgb(60, 0, 0);  break;
        }
    }

    memset(s_oled_fb, 0, sizeof(s_oled_fb));
    char line[22];
    snprintf(line, sizeof(line), "MODE: %s", mode_names[m]);
    oled_draw_str(0, 0, line);
    snprintf(line, sizeof(line), "TILT: %.2f", (double)telem->theta_deg);
    oled_draw_str(2, 0, line);
    snprintf(line, sizeof(line), "RPM : %.0f", (double)(telem->omega_w_rads * 9.549297f));
    oled_draw_str(4, 0, line);
    snprintf(line, sizeof(line), "PWM : %.0f%%", (double)(telem->pwm_duty * 100.0f));
    oled_draw_str(6, 0, line);

    static const uint8_t window_cmds[] = {0x21, 0, 127, 0x22, 0, 7};
    oled_send(false, window_cmds, sizeof(window_cmds));
    oled_send(true, s_oled_fb, sizeof(s_oled_fb));
}

void hal_log_line(const char *msg)
{
    printf("%s\n", msg);
}

int hal_uart_read_line(char *buf, int max_len)
{
    if (max_len <= 1) return 0;
    uint8_t ch;
    int idx = 0;
    while (idx < (max_len - 1) && uart_read_bytes(UART_NUM_0, &ch, 1, 0) == 1) {
        if (ch == '\n' || ch == '\r') {
            buf[idx] = '\0';
            return idx;
        }
        buf[idx++] = (char)ch;
    }
    buf[idx] = '\0';
    return 0;
}
