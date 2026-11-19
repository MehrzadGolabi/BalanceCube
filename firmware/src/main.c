#include "hal_interface.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>

static void control_task_core1(void *pvParameters)
{
    (void)pvParameters;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period_ticks = pdMS_TO_TICKS(5);

    for (;;) {
        vTaskDelayUntil(&last_wake, period_ticks);
        app_controller_tick_200hz();
    }
}

static void telemetry_cli_task_core0(void *pvParameters)
{
    (void)pvParameters;
    char cmd_line[96];
    char reply[160];
    uint32_t decimate = 0U;

    for (;;) {
        BalanceTelemetry_T telem = app_controller_get_telemetry();
        hal_ui_update(&telem);

        if (hal_uart_read_line(cmd_line, (int)sizeof(cmd_line)) > 0) {
            BalanceCubeConfig_T cfg = app_controller_get_config();
            bool do_reset = false;
            if (cli_process_line(cmd_line, &cfg, &do_reset, reply, (int)sizeof(reply))) {
                app_controller_set_config(&cfg);
                if (do_reset) {
                    app_controller_request_reset();
                }
            }
            if (reply[0] != '\0') {
                hal_log_line(reply);
            }
        }

        if ((decimate++ % 10U) == 0U) {
            snprintf(reply, sizeof(reply),
                     "mode=%u th=%.2fdeg w=%.1fRPM pwm=%.0f%%",
                     (unsigned)telem.mode, (double)telem.theta_deg,
                     (double)(telem.omega_w_rads * 9.549297f),
                     (double)(telem.pwm_duty * 100.0f));
            hal_log_line(reply);
        }

        vTaskDelay(pdMS_TO_TICKS(50)); /* 20 Hz UI/CLI loop on Core 0 */
    }
}

void app_main(void)
{
    if (!hal_init()) {
        hal_log_line("ERR: HAL initialization failed (check MPU6050 wiring/JP1)");
    }

    BalanceCubeConfig_T cfg = app_default_config();
    app_controller_init(&cfg);
    hal_log_line("BalanceCube 1D Controller ready (200 Hz Core 1). Type 'help' on UART.");

    /* Pin 200 Hz control exclusively to Core 1; keep OLED/LED/UART on Core 0 */
    xTaskCreatePinnedToCore(control_task_core1, "ctrl_200hz", 4096, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(telemetry_cli_task_core0, "ui_cli", 4096, NULL, 2, NULL, 0);
}
