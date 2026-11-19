/*
 * balanceCubeControllerStep_types.h
 *
 * Code generation for function 'balanceCubeControllerStep'
 *
 */

#pragma once

/* Include files */
#include "rtwtypes.h"
#include "emlrt.h"

/* Type Definitions */
#ifndef typedef_BalanceCubeConfig_T
#define typedef_BalanceCubeConfig_T
typedef struct {
  real32_T Ts;
  boolean_T imuRotated45deg;
  boolean_T skipCalib;
  real32_T mountTrimRad;
  real32_T cpr4x;
  real32_T encLpfBeta;
  real32_T cfAlpha;
  real32_T engageAngleRad;
  real32_T catchLimitRad;
  real32_T K[4];
  real32_T VmaxEff;
  real32_T Vdeadband;
  real32_T intWMax;
} BalanceCubeConfig_T;
#endif /* typedef_BalanceCubeConfig_T */

/* End of code generation (balanceCubeControllerStep_types.h) */
