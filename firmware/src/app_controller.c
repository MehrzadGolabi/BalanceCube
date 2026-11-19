#include "hal_interface.h"
#include "balanceCubeControllerStep_initialize.h"
#include <string.h>

static BalanceCubeConfig_T s_cfg;
static BalanceTelemetry_T s_telem;
static bool s_reset_pending = true;

BalanceCubeConfig_T app_default_config(void)
{
    BalanceCubeConfig_T cfg;
    cfg.Ts              = 0.005f;       /* 200 Hz deterministic loop */
    cfg.imuRotated45deg = true;         /* Matches diamond PCB alignment */
    cfg.skipCalib       = false;        /* Run 2.0 s boot gyro bias calibration */
    cfg.mountTrimRad    = 0.0f;         /* Bench-tunable via CLI 'set trim <deg>' */
    cfg.cpr4x           = 275.0f;       /* JGB37-520 6.25:1 (11 PPR * 4x * 6.25) */
    cfg.encLpfBeta      = 0.4854f;      /* 30 Hz discrete velocity LPF @ 200 Hz */
    cfg.cfAlpha         = 0.9920f;      /* Complementary filter alpha */
    cfg.engageAngleRad  = 0.05235988f;  /* +-3.0 deg engage threshold */
    cfg.catchLimitRad   = 0.52359878f;  /* +-30.0 deg fall cutoff */
    cfg.K[0]            = -128.8520f;
    cfg.K[1]            = -13.3931f;
    cfg.K[2]            = -0.3655f;
    cfg.K[3]            = -0.1456f;
    cfg.VmaxEff         = 10.0f;        /* 12 V rail minus ~2 V L298N drop */
    cfg.Vdeadband       = 0.42f;        /* Smooth stiction compensation [V] */
    cfg.intWMax         = 25.0f;        /* Anti-windup integral clamp */
    return cfg;
}

void app_controller_init(const BalanceCubeConfig_T *initial_cfg)
{
    if (initial_cfg != NULL) {
        s_cfg = *initial_cfg;
    } else {
        s_cfg = app_default_config();
    }
    memset(&s_telem, 0, sizeof(s_telem));
    balanceCubeControllerStep_initialize();
    s_reset_pending = true;
}

void app_controller_request_reset(void)
{
    s_reset_pending = true;
}

BalanceTelemetry_T app_controller_get_telemetry(void)
{
    return s_telem;
}

BalanceCubeConfig_T app_controller_get_config(void)
{
    return s_cfg;
}

void app_controller_set_config(const BalanceCubeConfig_T *new_cfg)
{
    if (new_cfg != NULL) {
        s_cfg = *new_cfg;
    }
}

void app_controller_tick_200hz(void)
{
    float ax_mps2 = 0.0f;
    float ay_mps2 = 9.80665f;
    float gz_rads = 0.0f;
    bool imu_ok = hal_mpu6050_read_2d(&ax_mps2, &ay_mps2, &gz_rads);
    int enc_delta = hal_pcnt_read_and_clear();

    float pwm_duty = 0.0f;
    bool in1_high = false;
    bool in2_high = false;
    unsigned char mode_out = (unsigned char)CUBE_MODE_FAULT;
    float theta_deg = 0.0f;
    float omega_w = 0.0f;

    if (!imu_ok) {
        /* Fail-safe immediate motor disable if I2C read drops */
        hal_l298n_write(0.0f, false, false);
        s_telem.pwm_duty = 0.0f;
        s_telem.in1_high = false;
        s_telem.in2_high = false;
        s_telem.mode = (uint8_t)CUBE_MODE_FAULT;
        s_telem.imu_ok = false;
        s_telem.loop_count++;
        return;
    }

    balanceCubeControllerStep(
        ax_mps2,
        ay_mps2,
        gz_rads,
        enc_delta,
        s_reset_pending,
        &s_cfg,
        &pwm_duty,
        &in1_high,
        &in2_high,
        &mode_out,
        &theta_deg,
        &omega_w);

    s_reset_pending = false;

    hal_l298n_write(pwm_duty, in1_high, in2_high);

    s_telem.pwm_duty = pwm_duty;
    s_telem.in1_high = in1_high;
    s_telem.in2_high = in2_high;
    s_telem.mode = (uint8_t)mode_out;
    s_telem.theta_deg = theta_deg;
    s_telem.omega_w_rads = omega_w;
    s_telem.imu_ok = true;
    s_telem.loop_count++;
}
