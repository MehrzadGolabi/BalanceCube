/*
 * esp32_main_example.c
 *
 * Reference ESP32 (ESP-IDF / FreeRTOS) integration harness for the MATLAB Coder
 * C99 controller generated in firmware/balance_controller_c/.
 *
 * Maps directly to Schematics/Balance_cube.kicad_sch:
 *   - MPU6050 I2C (SDA / SCL / IMU_INT) -> ax, ay [m/s^2], gz [rad/s]
 *   - Quadrature Encoder (CH_A / CH_B)  -> ESP32 PCNT 4x tick delta
 *   - L298N H-Bridge (PWM / IN1 / IN2)  -> LEDC PWM duty + direction GPIOs
 *   - WS2812B (LED_DATA) & Buzzer       -> Supervisory mode feedback
 *
 * NOTE: This is a minimal single-file template. For the complete flashable
 *       PlatformIO / ESP-IDF project with MPU6050, PCNT, OLED, WS2812B, and
 *       UART CLI tuning, see firmware/platformio.ini, src/main.c, src/hal_esp32.c.
 */

#include <stdbool.h>
#include <stdint.h>
#include "balance_controller_c/balanceCubeControllerStep.h"
#include "balance_controller_c/balanceCubeControllerStep_initialize.h"

/* Placeholder board-support prototypes — wire to your ESP-IDF / Arduino HAL */
extern void hal_init_i2c_mpu6050(void);
extern void hal_init_pcnt_encoder(void);
extern void hal_init_l298n_pwm_gpio(void);
extern void hal_mpu6050_read_2d(float *ax_mps2, float *ay_mps2, float *gz_rads);
extern int  hal_pcnt_read_and_clear(void);
extern void hal_l298n_write(float pwm_duty, bool in1_high, bool in2_high);
extern void hal_update_status_ui(uint8_t mode, float theta_deg, float wheel_rads);
extern void hal_delay_until_next_5ms_tick(void);

/* Active baseline config: 140 mm acrylic + 90 mm brass + JGB37-520 6.25:1 + L298N */
static BalanceCubeConfig_T g_ctrl_cfg = {
    .Ts              = 0.005f,       /* 200 Hz deterministic loop period */
    .imuRotated45deg = true,         /* Set false if MPU6050 aligns with upright diagonal */
    .skipCalib       = false,        /* Run 2.0 s stationary gyro bias calibration at boot */
    .mountTrimRad    = 0.0f,         /* Bench-measured mechanical zero offset [rad] */
    .cpr4x           = 275.0f,       /* 11 PPR * 4x * 6.25:1 = 275 ticks/rev at flywheel */
    .encLpfBeta      = 0.4854f,      /* 30 Hz 1st-order encoder velocity LPF @ 200 Hz */
    .cfAlpha         = 0.9920f,      /* Complementary filter weight (tau ~ 0.62 s) */
    .engageAngleRad  = 0.05235988f,  /* +-3.0 deg engage window */
    .catchLimitRad   = 0.52359878f,  /* +-30.0 deg fall cutoff */
    .K               = {-128.8520f, -13.3931f, -0.3655f, -0.1456f},
    .VmaxEff         = 10.0f,        /* 12 V supply - ~2 V L298N Darlington drop */
    .Vdeadband       = 0.45f,        /* Tune on bench (0.3-0.7 V) for L298N/brush stiction */
    .intWMax         = 25.0f         /* Anti-windup clamp on wheel-speed integral */
};

void balance_cube_control_task_core1(void *pvParameters)
{
    (void)pvParameters;

    hal_init_i2c_mpu6050();
    hal_init_pcnt_encoder();
    hal_init_l298n_pwm_gpio();

    balanceCubeControllerStep_initialize();

    bool first_tick = true;

    for (;;) {
        hal_delay_until_next_5ms_tick();

        float ax_mps2 = 0.0f;
        float ay_mps2 = 0.0f;
        float gz_rads = 0.0f;
        hal_mpu6050_read_2d(&ax_mps2, &ay_mps2, &gz_rads);

        int enc_ticks_delta = hal_pcnt_read_and_clear();

        float pwm_duty = 0.0f;
        bool in1_high = false;
        bool in2_high = false;
        unsigned char mode_out = 0U;
        float theta_hat_deg = 0.0f;
        float omega_w_rads = 0.0f;

        balanceCubeControllerStep(
            ax_mps2,
            ay_mps2,
            gz_rads,
            enc_ticks_delta,
            first_tick,
            &g_ctrl_cfg,
            &pwm_duty,
            &in1_high,
            &in2_high,
            &mode_out,
            &theta_hat_deg,
            &omega_w_rads);

        first_tick = false;

        hal_l298n_write(pwm_duty, in1_high, in2_high);
        hal_update_status_ui((uint8_t)mode_out, theta_hat_deg, omega_w_rads);
    }
}
