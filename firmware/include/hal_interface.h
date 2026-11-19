#ifndef HAL_INTERFACE_H
#define HAL_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>
#include "balanceCubeControllerStep.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CUBE_MODE_CALIB   = 0U,
    CUBE_MODE_IDLE    = 1U,
    CUBE_MODE_BALANCE = 2U,
    CUBE_MODE_FAULT   = 3U
} CubeMode_T;

typedef struct {
    float pwm_duty;
    bool in1_high;
    bool in2_high;
    uint8_t mode;
    float theta_deg;
    float omega_w_rads;
    uint32_t loop_count;
    bool imu_ok;
} BalanceTelemetry_T;

/* Hardware abstraction callbacks implemented in hal_esp32.c or test_host */
bool hal_init(void);
bool hal_mpu6050_read_2d(float *ax_mps2, float *ay_mps2, float *gz_rads);
int  hal_pcnt_read_and_clear(void);
void hal_l298n_write(float pwm_duty, bool in1_high, bool in2_high);
void hal_ui_update(const BalanceTelemetry_T *telem);
void hal_log_line(const char *msg);
int  hal_uart_read_line(char *buf, int max_len);

/* Application & CLI API (shared between ESP32 main and host tests) */
BalanceCubeConfig_T app_default_config(void);
void app_controller_init(const BalanceCubeConfig_T *initial_cfg);
void app_controller_tick_200hz(void);
void app_controller_request_reset(void);
BalanceTelemetry_T app_controller_get_telemetry(void);
BalanceCubeConfig_T app_controller_get_config(void);
void app_controller_set_config(const BalanceCubeConfig_T *new_cfg);
bool cli_process_line(const char *line, BalanceCubeConfig_T *cfg,
                      bool *reset_requested, char *reply, int reply_cap);

#ifdef __cplusplus
}
#endif

#endif /* HAL_INTERFACE_H */
