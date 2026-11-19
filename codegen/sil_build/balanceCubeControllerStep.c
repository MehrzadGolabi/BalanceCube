/*
 * File: balanceCubeControllerStep.c
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:08:59
 */

/* Include Files */
#include "balanceCubeControllerStep.h"
#include "balanceCubeControllerStep_data.h"
#include "balanceCubeControllerStep_initialize.h"
#include "balanceCubeControllerStep_types.h"
#include <math.h>

/* Variable Definitions */
static boolean_T isInit_not_empty;

/* Function Definitions */
/*
 * BALANCECUBECONTROLLERSTEP 200 Hz single-precision controller for ESP32 1D
 * Balance Cube.
 *
 * Arguments    : float axMps2
 *                float ayMps2
 *                float gzRads
 *                int encTicksDelta
 *                boolean_T resetState
 *                const BalanceCubeConfig_T *cfgParams
 *                float *pwmDuty
 *                boolean_T *in1High
 *                boolean_T *in2High
 *                unsigned char *modeOut
 *                float *thetaHatDeg
 *                float *omegaWRads
 * Return Type  : void
 */
void balanceCubeControllerStep(float axMps2, float ayMps2, float gzRads,
                               int encTicksDelta, boolean_T resetState,
                               const BalanceCubeConfig_T *cfgParams,
                               float *pwmDuty, boolean_T *in1High,
                               boolean_T *in2High, unsigned char *modeOut,
                               float *thetaHatDeg, float *omegaWRads)
{
  static float gzBias;
  static float gzSum;
  static float intWheelW;
  static float omegaWFilt;
  static float thetaHat;
  static unsigned short calibCount;
  static unsigned short uprightCount;
  static unsigned char modeState;
  float Vcmd;
  float thetaAcc;
  float thetaDot;
  boolean_T didReset;
  if (!isInitialized_balanceCubeControllerStep) {
    balanceCubeControllerStep_initialize();
  }
  /*  1. Accelerometer tilt projection (45 deg rotation if IMU aligns with
   * square edge) */
  if (cfgParams->imuRotated45deg) {
    thetaDot = 0.707106769F * (axMps2 - ayMps2);
    thetaAcc = 0.707106769F * (axMps2 + ayMps2);
  } else {
    thetaDot = axMps2;
    thetaAcc = ayMps2;
  }
  thetaAcc = atan2f(thetaDot, thetaAcc) - cfgParams->mountTrimRad;
  /*  2. Unconditional initialization for MATLAB Coder persistent analysis, plus
   * runtime reset */
  didReset = resetState;
  if (!isInit_not_empty) {
    isInit_not_empty = true;
    didReset = true;
    calibCount = 0U;
    gzSum = 0.0F;
    gzBias = 0.0F;
    thetaHat = thetaAcc;
    omegaWFilt = 0.0F;
    intWheelW = 0.0F;
    uprightCount = 0U;
    if (cfgParams->skipCalib) {
      modeState = 2U;
    } else {
      modeState = 0U;
    }
  } else if (resetState) {
    calibCount = 0U;
    gzSum = 0.0F;
    gzBias = 0.0F;
    thetaHat = thetaAcc;
    omegaWFilt = 0.0F;
    intWheelW = 0.0F;
    uprightCount = 0U;
    if (cfgParams->skipCalib) {
      modeState = 2U;
    } else {
      modeState = 0U;
    }
  }
  /*  3. Quadrature encoder angular velocity + 1st-order LPF */
  omegaWFilt = (1.0F - cfgParams->encLpfBeta) * omegaWFilt +
               cfgParams->encLpfBeta * (6.28318548F * (float)encTicksDelta /
                                        (cfgParams->cpr4x * cfgParams->Ts));
  /*  4. Complementary tilt angle filter */
  thetaDot = gzRads - gzBias;
  if (!didReset) {
    thetaHat = cfgParams->cfAlpha * (thetaHat + thetaDot * cfgParams->Ts) +
               (1.0F - cfgParams->cfAlpha) * thetaAcc;
  }
  /*  5. Four-mode supervisory state machine (0=CALIB, 1=IDLE, 2=BALANCE,
   * 3=FAULT) */
  Vcmd = 0.0F;
  switch (modeState) {
  case 0U: {
    unsigned int u;
    thetaHat = thetaAcc;
    gzSum += gzRads;
    u = calibCount + 1U;
    if (calibCount + 1U > 65535U) {
      u = 65535U;
    }
    calibCount = (unsigned short)u;
    if (calibCount >= 400) {
      gzBias = gzSum / 400.0F;
      modeState = 1U;
      uprightCount = 0U;
    }
  } break;
  case 1U: {
    intWheelW = 0.0F;
    if ((fabsf(thetaHat) < cfgParams->engageAngleRad) &&
        (fabsf(thetaDot) < 0.35F)) {
      unsigned int u;
      u = uprightCount + 1U;
      if (uprightCount + 1U > 65535U) {
        u = 65535U;
      }
      uprightCount = (unsigned short)u;
      if (uprightCount >= 20) {
        modeState = 2U;
      }
    } else {
      uprightCount = 0U;
    }
  } break;
  case 2U:
    if (fabsf(thetaHat) > cfgParams->catchLimitRad) {
      modeState = 3U;
      intWheelW = 0.0F;
    } else {
      thetaDot = -(((cfgParams->K[0] * thetaHat + cfgParams->K[1] * thetaDot) +
                    cfgParams->K[2] * omegaWFilt) +
                   cfgParams->K[3] * intWheelW);
      /*  Conditional anti-windup on flywheel speed integral */
      if (fabsf(thetaDot) < cfgParams->VmaxEff) {
        intWheelW += omegaWFilt * cfgParams->Ts;
        intWheelW =
            fminf(cfgParams->intWMax, fmaxf(-cfgParams->intWMax, intWheelW));
      }
      Vcmd = fminf(cfgParams->VmaxEff, fmaxf(-cfgParams->VmaxEff, thetaDot));
    }
    break;
  default:
    intWheelW = 0.0F;
    if ((fabsf(thetaHat) < cfgParams->engageAngleRad) &&
        (fabsf(thetaDot) < 0.35F)) {
      modeState = 1U;
      uprightCount = 0U;
    }
    break;
  }
  /*  6. H-bridge PWM & direction pin mapping (smooth anti-chatter stiction
   * ramp) */
  thetaDot = fabsf(Vcmd);
  if (thetaDot < 0.02F) {
    *pwmDuty = 0.0F;
    *in1High = false;
    *in2High = false;
  } else {
    *pwmDuty = fminf(1.0F, (thetaDot + fminf(1.0F, thetaDot / 0.25F) *
                                           cfgParams->Vdeadband) /
                               cfgParams->VmaxEff);
    *in1High = (Vcmd > 0.0F);
    *in2High = (Vcmd < 0.0F);
  }
  *modeOut = modeState;
  *thetaHatDeg = thetaHat * 57.2957802F;
  *omegaWRads = omegaWFilt;
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void balanceCubeControllerStep_init(void)
{
  isInit_not_empty = false;
}

/*
 * File trailer for balanceCubeControllerStep.c
 *
 * [EOF]
 */
