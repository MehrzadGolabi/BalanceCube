/*
 * balanceCubeControllerStep.c
 *
 * Code generation for function 'balanceCubeControllerStep'
 *
 */

/* Include files */
#include "balanceCubeControllerStep.h"
#include "balanceCubeControllerStep_types.h"
#include "rt_nonfinite.h"
#include "mwmathutil.h"

/* Variable Definitions */
static boolean_T isInit_not_empty;

/* Function Definitions */
void balanceCubeControllerStep(const emlrtStack *sp, real32_T axMps2,
                               real32_T ayMps2, real32_T gzRads,
                               int32_T encTicksDelta, boolean_T resetState,
                               const BalanceCubeConfig_T *cfgParams,
                               real32_T *pwmDuty, boolean_T *in1High,
                               boolean_T *in2High, uint8_T *modeOut,
                               real32_T *thetaHatDeg, real32_T *omegaWRads)
{
  static real32_T gzBias;
  static real32_T gzSum;
  static real32_T intWheelW;
  static real32_T omegaWFilt;
  static real32_T thetaHat;
  static uint16_T calibCount;
  static uint16_T uprightCount;
  static uint8_T modeState;
  real32_T Vcmd;
  real32_T thetaAcc;
  real32_T thetaDot;
  boolean_T didReset;
  (void)sp;
  /* BALANCECUBECONTROLLERSTEP 200 Hz single-precision controller for ESP32 1D
   * Balance Cube. */
  /*  1. Accelerometer tilt projection (45 deg rotation if IMU aligns with
   * square edge) */
  if (cfgParams->imuRotated45deg) {
    thetaDot = 0.707106769F * (axMps2 - ayMps2);
    thetaAcc = 0.707106769F * (axMps2 + ayMps2);
  } else {
    thetaDot = axMps2;
    thetaAcc = ayMps2;
  }
  thetaAcc = muSingleScalarAtan2(thetaDot, thetaAcc) - cfgParams->mountTrimRad;
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
               cfgParams->encLpfBeta * (6.28318548F * (real32_T)encTicksDelta /
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
    uint32_T u;
    thetaHat = thetaAcc;
    gzSum += gzRads;
    u = calibCount + 1U;
    if (calibCount + 1U > 65535U) {
      u = 65535U;
    }
    calibCount = (uint16_T)u;
    if (calibCount >= 400) {
      gzBias = gzSum / 400.0F;
      modeState = 1U;
      uprightCount = 0U;
    }
  } break;
  case 1U: {
    intWheelW = 0.0F;
    if ((muSingleScalarAbs(thetaHat) < cfgParams->engageAngleRad) &&
        (muSingleScalarAbs(thetaDot) < 0.35F)) {
      uint32_T u;
      u = uprightCount + 1U;
      if (uprightCount + 1U > 65535U) {
        u = 65535U;
      }
      uprightCount = (uint16_T)u;
      if (uprightCount >= 20) {
        modeState = 2U;
      }
    } else {
      uprightCount = 0U;
    }
  } break;
  case 2U:
    if (muSingleScalarAbs(thetaHat) > cfgParams->catchLimitRad) {
      modeState = 3U;
      intWheelW = 0.0F;
    } else {
      thetaDot = -(((cfgParams->K[0] * thetaHat + cfgParams->K[1] * thetaDot) +
                    cfgParams->K[2] * omegaWFilt) +
                   cfgParams->K[3] * intWheelW);
      /*  Conditional anti-windup on flywheel speed integral */
      if (muSingleScalarAbs(thetaDot) < cfgParams->VmaxEff) {
        intWheelW += omegaWFilt * cfgParams->Ts;
        intWheelW = muSingleScalarMin(
            cfgParams->intWMax,
            muSingleScalarMax(-cfgParams->intWMax, intWheelW));
      }
      Vcmd = muSingleScalarMin(
          cfgParams->VmaxEff, muSingleScalarMax(-cfgParams->VmaxEff, thetaDot));
    }
    break;
  default:
    intWheelW = 0.0F;
    if ((muSingleScalarAbs(thetaHat) < cfgParams->engageAngleRad) &&
        (muSingleScalarAbs(thetaDot) < 0.35F)) {
      modeState = 1U;
      uprightCount = 0U;
    }
    break;
  }
  /*  6. H-bridge PWM & direction pin mapping (smooth anti-chatter stiction
   * ramp) */
  thetaDot = muSingleScalarAbs(Vcmd);
  if (thetaDot < 0.02F) {
    *pwmDuty = 0.0F;
    *in1High = false;
    *in2High = false;
  } else {
    *pwmDuty = muSingleScalarMin(
        1.0F, (thetaDot + muSingleScalarMin(1.0F, thetaDot / 0.25F) *
                              cfgParams->Vdeadband) /
                  cfgParams->VmaxEff);
    *in1High = (Vcmd > 0.0F);
    *in2High = (Vcmd < 0.0F);
  }
  *modeOut = modeState;
  *thetaHatDeg = thetaHat * 57.2957802F;
  *omegaWRads = omegaWFilt;
}

void balanceCubeControllerStep_init(void)
{
  isInit_not_empty = false;
}

/* End of code generation (balanceCubeControllerStep.c) */
