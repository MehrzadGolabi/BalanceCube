/*
 * File: _coder_balanceCubeControllerStep_api.h
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:08:59
 */

#ifndef _CODER_BALANCECUBECONTROLLERSTEP_API_H
#define _CODER_BALANCECUBECONTROLLERSTEP_API_H

/* Include Files */
#include "emlrt.h"
#include "mex.h"
#include "tmwtypes.h"
#include <string.h>

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

/* Variable Declarations */
extern emlrtCTX emlrtRootTLSGlobal;
extern emlrtContext emlrtContextGlobal;

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
void balanceCubeControllerStepXilWrapper(
    real32_T axMps2, real32_T ayMps2, real32_T gzRads, int32_T encTicksDelta,
    boolean_T resetState, const BalanceCubeConfig_T *cfgParams,
    real32_T *pwmDuty, boolean_T *in1High, boolean_T *in2High, uint8_T *modeOut,
    real32_T *thetaHatDeg, real32_T *omegaWRads);

void balanceCubeControllerStep_api(const mxArray *const prhs[6], int32_T nlhs,
                                   const mxArray *plhs[6]);

void balanceCubeControllerStep_atexit(void);

void balanceCubeControllerStep_initialize(void);

void balanceCubeControllerStep_terminate(void);

void balanceCubeControllerStep_xil_shutdown(void);

void balanceCubeControllerStep_xil_terminate(void);

void xil_terminate(void);

#ifdef __cplusplus
}
#endif

#endif
/*
 * File trailer for _coder_balanceCubeControllerStep_api.h
 *
 * [EOF]
 */
