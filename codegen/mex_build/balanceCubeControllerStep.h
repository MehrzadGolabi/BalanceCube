/*
 * balanceCubeControllerStep.h
 *
 * Code generation for function 'balanceCubeControllerStep'
 *
 */

#pragma once

/* Include files */
#include "balanceCubeControllerStep_types.h"
#include "rtwtypes.h"
#include "emlrt.h"
#include "mex.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Function Declarations */
void balanceCubeControllerStep(const emlrtStack *sp, real32_T axMps2,
                               real32_T ayMps2, real32_T gzRads,
                               int32_T encTicksDelta, boolean_T resetState,
                               const BalanceCubeConfig_T *cfgParams,
                               real32_T *pwmDuty, boolean_T *in1High,
                               boolean_T *in2High, uint8_T *modeOut,
                               real32_T *thetaHatDeg, real32_T *omegaWRads);

void balanceCubeControllerStep_init(void);

/* End of code generation (balanceCubeControllerStep.h) */
