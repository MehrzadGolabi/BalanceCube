/*
 * File: balanceCubeControllerStep.h
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:09:12
 */

#ifndef BALANCECUBECONTROLLERSTEP_H
#define BALANCECUBECONTROLLERSTEP_H

/* Include Files */
#include "balanceCubeControllerStep_types.h"
#include "rtwtypes.h"
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
extern void balanceCubeControllerStep(float axMps2, float ayMps2, float gzRads,
                                      int encTicksDelta, bool resetState,
                                      const BalanceCubeConfig_T *cfgParams,
                                      float *pwmDuty, bool *in1High,
                                      bool *in2High, unsigned char *modeOut,
                                      float *thetaHatDeg, float *omegaWRads);

void balanceCubeControllerStep_init(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for balanceCubeControllerStep.h
 *
 * [EOF]
 */
