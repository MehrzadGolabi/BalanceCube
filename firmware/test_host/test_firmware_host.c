#include "hal_interface.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

extern void mock_set_plant_state(float theta_deg, float gz_rads, int enc_delta);
extern void mock_get_motor_out(float *pwm, bool *in1, bool *in2);

int main(void)
{
    BalanceCubeConfig_T cfg = app_default_config();
    app_controller_init(&cfg);

    /* 1. 400 ticks calibrating at +20 deg with +0.035 rad/s bias */
    mock_set_plant_state(20.0f, 0.035f, 0);
    for (int k = 0; k < 400; k++) {
        app_controller_tick_200hz();
        BalanceTelemetry_T t = app_controller_get_telemetry();
        assert(t.pwm_duty == 0.0f);
        if (k < 399) {
            assert(t.mode == CUBE_MODE_CALIB);
        } else {
            assert(t.mode == CUBE_MODE_IDLE);
        }
    }

    /* 2. Lift upright (+1.0 deg) -> enter BALANCE after 20 steady ticks */
    mock_set_plant_state(1.0f, 0.035f + (1.0f - 20.0f) * 0.0174532925f / 0.005f, 0);
    app_controller_tick_200hz();
    mock_set_plant_state(1.0f, 0.035f, 0);
    for (int k = 0; k < 20; k++) {
        app_controller_tick_200hz();
    }
    BalanceTelemetry_T t_bal = app_controller_get_telemetry();
    assert(t_bal.mode == CUBE_MODE_BALANCE);

    /* 3. Tilt +4 deg -> check motor command polarity & smooth deadband */
    mock_set_plant_state(4.0f, 0.035f + (4.0f - 1.0f) * 0.0174532925f / 0.005f, 8);
    app_controller_tick_200hz();
    float pwm = 0.0f;
    bool in1 = false, in2 = false;
    mock_get_motor_out(&pwm, &in1, &in2);
    assert(in1 && !in2 && pwm > 0.5f);

    /* 4. Live CLI tuning tests */
    char reply[160];
    bool do_reset = false;
    BalanceCubeConfig_T live_cfg = app_controller_get_config();
    assert(cli_process_line("set db 0.35", &live_cfg, &do_reset, reply, (int)sizeof(reply)));
    assert(!do_reset && fabsf(live_cfg.Vdeadband - 0.35f) < 1e-4f);

    assert(cli_process_line("set trim 0.50", &live_cfg, &do_reset, reply, (int)sizeof(reply)));
    assert(fabsf(live_cfg.mountTrimRad - 0.50f * 0.0174532925f) < 1e-5f);

    assert(cli_process_line("status", &live_cfg, &do_reset, reply, (int)sizeof(reply)));
    assert(strstr(reply, "db=0.35V") != NULL);

    assert(cli_process_line("reset", &live_cfg, &do_reset, reply, (int)sizeof(reply)));
    assert(do_reset);

    /* 5. Knock over past +35 deg -> FAULT cutoff */
    app_controller_set_config(&live_cfg);
    mock_set_plant_state(35.0f, 0.035f + (35.0f - 4.0f) * 0.0174532925f / 0.005f, 0);
    app_controller_tick_200hz();
    t_bal = app_controller_get_telemetry();
    assert(t_bal.mode == CUBE_MODE_FAULT && t_bal.pwm_duty == 0.0f);

    printf("HOST FIRMWARE TEST PASSED: startup calib -> idle -> balance -> CLI -> fault OK!\n");
    return 0;
}
