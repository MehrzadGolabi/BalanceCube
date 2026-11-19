/*
 * File: balanceCubeControllerStep_types.h
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:08:59
 */

#ifndef BALANCECUBECONTROLLERSTEP_TYPES_H
#define BALANCECUBECONTROLLERSTEP_TYPES_H

/* Include Files */
#include "rtwtypes.h"

/* Type Definitions */
#ifndef typedef_BalanceCubeConfig_T
#define typedef_BalanceCubeConfig_T
typedef struct {
  float Ts;
  boolean_T imuRotated45deg;
  boolean_T skipCalib;
  float mountTrimRad;
  float cpr4x;
  float encLpfBeta;
  float cfAlpha;
  float engageAngleRad;
  float catchLimitRad;
  float K[4];
  float VmaxEff;
  float Vdeadband;
  float intWMax;
} BalanceCubeConfig_T;
#endif /* typedef_BalanceCubeConfig_T */

#endif
/*
 * File trailer for balanceCubeControllerStep_types.h
 *
 * [EOF]
 */
