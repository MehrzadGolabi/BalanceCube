#include "hal_interface.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* Simulated plant states for host firmware integration testing */
static float s_theta_rad = 0.34906585f; /* Start resting at +20 deg */
static float s_gz_rads = 0.035f;        /* +0.035 rad/s bias */
static int   s_enc_delta = 0;
static float s_last_pwm = 0.0f;
static bool  s_last_in1 = false;
static bool  s_last_in2 = false;
static char  s_pending_cli[96] = {0};

void mock_set_plant_state(float theta_deg, float gz_rads, int enc_delta)
{
    s_theta_rad = theta_deg * 0.0174532925f;
    s_gz_rads = gz_rads;
    s_enc_delta = enc_delta;
}

void mock_queue_cli(const char *cmd)
{
    snprintf(s_pending_cli, sizeof(s_pending_cli), "%s", cmd);
}

void mock_get_motor_out(float *pwm, bool *in1, bool *in2)
{
    *pwm = s_last_pwm;
    *in1 = s_last_in1;
    *in2 = s_last_in2;
}

bool hal_init(void) { return true; }

bool hal_mpu6050_read_2d(float *ax_mps2, float *ay_mps2, float *gz_rads)
{
    const float c45 = 0.70710678f;
    float ax_diag = 9.80665f * sinf(s_theta_rad);
    float ay_diag = 9.80665f * cosf(s_theta_rad);
    *ax_mps2 = c45 * (ax_diag + ay_diag);
    *ay_mps2 = c45 * (-ax_diag + ay_diag);
    *gz_rads = s_gz_rads;
    return true;
}

int hal_pcnt_read_and_clear(void) { return s_enc_delta; }

void hal_l298n_write(float pwm_duty, bool in1_high, bool in2_high)
{
    s_last_pwm = pwm_duty;
    s_last_in1 = in1_high;
    s_last_in2 = in2_high;
}

void hal_ui_update(const BalanceTelemetry_T *telem) { (void)telem; }
void hal_log_line(const char *msg) { printf("[HOST] %s\n", msg); }

int hal_uart_read_line(char *buf, int max_len)
{
    if (s_pending_cli[0] == '\0') return 0;
    snprintf(buf, (size_t)max_len, "%s", s_pending_cli);
    s_pending_cli[0] = '\0';
    return (int)strlen(buf);
}
