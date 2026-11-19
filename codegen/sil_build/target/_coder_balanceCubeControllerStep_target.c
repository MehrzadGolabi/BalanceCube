/*
 * File: _coder_balanceCubeControllerStep_target.c
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:08:59
 */

/* Include Files */
#include "_coder_balanceCubeControllerStep_target.h"
#include "balanceCubeControllerStep.h"
#include "balanceCubeControllerStep_initialize.h"
#include "balanceCubeControllerStep_terminate.h"
#include "balanceCubeControllerStep_types.h"

/* Function Declarations */
static void b_xilTargetDeserializer(int *r);

static void b_xilTargetSerializer(const boolean_T *r);

static void c_xilTargetDeserializer(boolean_T *r);

static void c_xilTargetSerializer(const unsigned char *r);

static void d_xilTargetDeserializer(BalanceCubeConfig_T *r);

static void e_xilTargetDeserializer(float r[4]);

static void xilTargetDeserializer(float *r);

static void xilTargetSerializer(const float *r);

/* Function Definitions */
/*
 * Arguments    : int *r
 * Return Type  : void
 */
static void b_xilTargetDeserializer(int *r)
{
  xilReadData((MemUnit_T *)r, sizeof(int));
}

/*
 * Arguments    : const boolean_T *r
 * Return Type  : void
 */
static void b_xilTargetSerializer(const boolean_T *r)
{
  xilWriteData((const MemUnit_T *)r, sizeof(boolean_T));
}

/*
 * Arguments    : boolean_T *r
 * Return Type  : void
 */
static void c_xilTargetDeserializer(boolean_T *r)
{
  xilReadData((MemUnit_T *)r, sizeof(boolean_T));
}

/*
 * Arguments    : const unsigned char *r
 * Return Type  : void
 */
static void c_xilTargetSerializer(const unsigned char *r)
{
  xilWriteData((const MemUnit_T *)r, sizeof(unsigned char));
}

/*
 * Arguments    : BalanceCubeConfig_T *r
 * Return Type  : void
 */
static void d_xilTargetDeserializer(BalanceCubeConfig_T *r)
{
  xilTargetDeserializer(&r->Ts);
  c_xilTargetDeserializer(&r->imuRotated45deg);
  c_xilTargetDeserializer(&r->skipCalib);
  xilTargetDeserializer(&r->mountTrimRad);
  xilTargetDeserializer(&r->cpr4x);
  xilTargetDeserializer(&r->encLpfBeta);
  xilTargetDeserializer(&r->cfAlpha);
  xilTargetDeserializer(&r->engageAngleRad);
  xilTargetDeserializer(&r->catchLimitRad);
  e_xilTargetDeserializer(r->K);
  xilTargetDeserializer(&r->VmaxEff);
  xilTargetDeserializer(&r->Vdeadband);
  xilTargetDeserializer(&r->intWMax);
}

/*
 * Arguments    : float r[4]
 * Return Type  : void
 */
static void e_xilTargetDeserializer(float r[4])
{
  xilReadData((MemUnit_T *)&r[0], sizeof(float) << 2);
}

/*
 * Arguments    : float *r
 * Return Type  : void
 */
static void xilTargetDeserializer(float *r)
{
  xilReadData((MemUnit_T *)r, sizeof(float));
}

/*
 * Arguments    : const float *r
 * Return Type  : void
 */
static void xilTargetSerializer(const float *r)
{
  xilWriteData((const MemUnit_T *)r, sizeof(float));
}

/*
 * Arguments    : unsigned int fcnId
 * Return Type  : void
 */
void XILTarget_initialize(unsigned int fcnId)
{
  /* Calling XIL API before invoke initialize function. */
  xilPreEntryPoint(fcnId);
  /* Invoke initialize function. */
  balanceCubeControllerStep_initialize();
  /* Calling XIL API after invoke initialize function. */
  xilPostEntryPoint(fcnId);
}

/*
 * Arguments    : unsigned int fcnId
 * Return Type  : void
 */
void XILTarget_terminate(unsigned int fcnId)
{
  /* Calling XIL API before invoke terminate function. */
  xilPreEntryPoint(fcnId);
  /* Invoke terminate function. */
  balanceCubeControllerStep_terminate();
  /* Calling XIL API after invoke terminate function. */
  xilPostTerminatePoint(fcnId);
}

/*
 * Arguments    : unsigned int fcnId
 * Return Type  : XIL_PROCESSDATA_ERROR_CODE
 */
XIL_PROCESSDATA_ERROR_CODE
xilTarget_balanceCubeControllerStep(unsigned int fcnId)
{
  BalanceCubeConfig_T cfgParams;
  float axMps2;
  float ayMps2;
  float gzRads;
  float omegaWRads;
  float pwmDuty;
  float thetaHatDeg;
  int encTicksDelta;
  unsigned char modeOut;
  boolean_T in1High;
  boolean_T in2High;
  boolean_T resetState;
  /* Deserialize function input axMps2 argument. */
  xilTargetDeserializer(&axMps2);
  /* Deserialize function input ayMps2 argument. */
  xilTargetDeserializer(&ayMps2);
  /* Deserialize function input gzRads argument. */
  xilTargetDeserializer(&gzRads);
  /* Deserialize function input encTicksDelta argument. */
  b_xilTargetDeserializer(&encTicksDelta);
  /* Deserialize function input resetState argument. */
  c_xilTargetDeserializer(&resetState);
  /* Deserialize function input cfgParams argument. */
  d_xilTargetDeserializer(&cfgParams);
  /* Calling XIL API before invoke entry point. */
  xilPreEntryPoint(fcnId);
  /* Invoke target entry point. */
  balanceCubeControllerStep(axMps2, ayMps2, gzRads, encTicksDelta, resetState,
                            &cfgParams, &pwmDuty, &in1High, &in2High, &modeOut,
                            &thetaHatDeg, &omegaWRads);
  /* Calling XIL API after invoke entry point. */
  xilPostEntryPoint(fcnId);
  /* Serialize function output argument pwmDuty. */
  xilTargetSerializer(&pwmDuty);
  /* Serialize function output argument in1High. */
  b_xilTargetSerializer(&in1High);
  /* Serialize function output argument in2High. */
  b_xilTargetSerializer(&in2High);
  /* Serialize function output argument modeOut. */
  c_xilTargetSerializer(&modeOut);
  /* Serialize function output argument thetaHatDeg. */
  xilTargetSerializer(&thetaHatDeg);
  /* Serialize function output argument omegaWRads. */
  xilTargetSerializer(&omegaWRads);
  return XIL_PROCESSDATA_SUCCESS;
}

/*
 * File trailer for _coder_balanceCubeControllerStep_target.c
 *
 * [EOF]
 */
