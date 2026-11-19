#include "hal_interface.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int parse_float_arg(const char *s, float *out)
{
    char *end = NULL;
    double val;
    if ((s == NULL) || (*s == '\0')) {
        return 0;
    }
    val = strtod(s, &end);
    if (end == s) {
        return 0;
    }
    *out = (float)val;
    return 1;
}

bool cli_process_line(const char *line, BalanceCubeConfig_T *cfg,
                      bool *reset_requested, char *reply, int reply_cap)
{
    char buf[96];
    size_t len;
    char *p;
    float v = 0.0f;

    if ((line == NULL) || (cfg == NULL) || (reply == NULL) || (reply_cap <= 0)) {
        return false;
    }

    if (reset_requested != NULL) {
        *reset_requested = false;
    }

    len = strlen(line);
    if (len >= sizeof(buf)) {
        len = sizeof(buf) - 1U;
    }
    memcpy(buf, line, len);
    buf[len] = '\0';

    while ((len > 0U) && ((buf[len - 1U] == '\r') || (buf[len - 1U] == '\n') ||
                          (buf[len - 1U] == ' ') || (buf[len - 1U] == '\t'))) {
        buf[--len] = '\0';
    }
    p = buf;
    while ((*p == ' ') || (*p == '\t')) {
        p++;
    }
    if (*p == '\0') {
        reply[0] = '\0';
        return false;
    }

    if (strcmp(p, "help") == 0) {
        snprintf(reply, (size_t)reply_cap,
                 "Commands: status | reset | set k1..k4 <v> | set trim <deg> | set db <V> | set vmax <V> | set cpr <ticks>");
        return true;
    }
    if (strcmp(p, "status") == 0) {
        snprintf(reply, (size_t)reply_cap,
                 "CFG K=[%.2f,%.2f,%.4f,%.4f] db=%.2fV vmax=%.1fV trim=%.2fdeg cpr=%.1f",
                 (double)cfg->K[0], (double)cfg->K[1], (double)cfg->K[2], (double)cfg->K[3],
                 (double)cfg->Vdeadband, (double)cfg->VmaxEff,
                 (double)(cfg->mountTrimRad * 57.29578f), (double)cfg->cpr4x);
        return true;
    }
    if (strcmp(p, "reset") == 0) {
        if (reset_requested != NULL) {
            *reset_requested = true;
        }
        snprintf(reply, (size_t)reply_cap, "OK: controller reset & recalibrating");
        return true;
    }

    if (strncmp(p, "set ", 4) == 0) {
        const char *arg = p + 4;
        if ((strncmp(arg, "k1 ", 3) == 0) && parse_float_arg(arg + 3, &v)) {
            cfg->K[0] = v;
        } else if ((strncmp(arg, "k2 ", 3) == 0) && parse_float_arg(arg + 3, &v)) {
            cfg->K[1] = v;
        } else if ((strncmp(arg, "k3 ", 3) == 0) && parse_float_arg(arg + 3, &v)) {
            cfg->K[2] = v;
        } else if ((strncmp(arg, "k4 ", 3) == 0) && parse_float_arg(arg + 3, &v)) {
            cfg->K[3] = v;
        } else if ((strncmp(arg, "trim ", 5) == 0) && parse_float_arg(arg + 5, &v)) {
            cfg->mountTrimRad = v * 0.0174532925f;
        } else if ((strncmp(arg, "db ", 3) == 0) && parse_float_arg(arg + 3, &v) && (v >= 0.0f) && (v <= 3.0f)) {
            cfg->Vdeadband = v;
        } else if ((strncmp(arg, "vmax ", 5) == 0) && parse_float_arg(arg + 5, &v) && (v >= 3.0f) && (v <= 24.0f)) {
            cfg->VmaxEff = v;
        } else if ((strncmp(arg, "cpr ", 4) == 0) && parse_float_arg(arg + 4, &v) && (v >= 16.0f)) {
            cfg->cpr4x = v;
        } else {
            snprintf(reply, (size_t)reply_cap, "ERR: invalid set command (%s)", p);
            return false;
        }
        snprintf(reply, (size_t)reply_cap, "OK: updated %s", arg);
        return true;
    }

    snprintf(reply, (size_t)reply_cap, "ERR: unknown command '%s' (type 'help')", p);
    return false;
}
