#ifndef BOARD_PINS_H
#define BOARD_PINS_H

/*
 * Pin mapping synchronized with Schematics/Balance_cube.kicad_sch (fixed rev).
 * Target board: DOIT ESP32 DevKit V1 (30-pin)
 */

/* MPU6050 6-Axis IMU (400 kHz I2C) */
#define PIN_I2C_SDA        21
#define PIN_I2C_SCL        22
#define PIN_IMU_INT        23
#define I2C_BUS_FREQ_HZ    400000
#define MPU6050_ADDR_LOW   0x68  /* JP1 open  (AD0 -> GND/float) */
#define MPU6050_ADDR_HIGH  0x69  /* JP1 closed (AD0 -> +3.3V)   */

/* Motor Quadrature Hall Encoder (3.3V powered via J2, PCNT 4x) */
#define PIN_ENC_CHA        25
#define PIN_ENC_CHB        26

/* L298N H-Bridge Channel A (GPIO32 avoids GPIO12 VDD_SDIO boot-strap pin) */
#define PIN_MOT_PWM        14
#define PIN_MOT_IN1        13
#define PIN_MOT_IN2        32
#define MOT_PWM_FREQ_HZ    20000 /* Ultrasonic 20 kHz silent drive */

/* Status Feedback (WS2812B RGB LED + NPN-driven 5V Buzzer) */
#define PIN_WS2812_DATA    27
#define PIN_BUZZER         33    /* DOIT 30-pin physical Pin 22 */

/* 7-Pin SPI OLED Display (SSD1306 128x64, 3.3V powered via J4) */
#define PIN_OLED_SCLK      16    /* D0 */
#define PIN_OLED_MOSI      17    /* D1 */
#define PIN_OLED_RES       5
#define PIN_OLED_DC        18
#define PIN_OLED_CS        19

#endif /* BOARD_PINS_H */
