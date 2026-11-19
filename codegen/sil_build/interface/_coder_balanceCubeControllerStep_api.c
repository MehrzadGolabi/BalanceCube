/*
 * File: _coder_balanceCubeControllerStep_api.c
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:08:59
 */

/* Include Files */
#include "_coder_balanceCubeControllerStep_api.h"
#include "_coder_balanceCubeControllerStep_mex.h"
#include "xil_host_interface.h"

/* Variable Definitions */
emlrtCTX emlrtRootTLSGlobal = NULL;

static boolean_T xilAlreadyInited;

emlrtContext emlrtContextGlobal = {
    true,                                                 /* bFirstTime */
    false,                                                /* bInitialized */
    131675U,                                              /* fVersionInfo */
    NULL,                                                 /* fErrorFunction */
    "balanceCubeControllerStep",                          /* fFunctionName */
    NULL,                                                 /* fRTCallStack */
    false,                                                /* bDebugMode */
    {2045744189U, 2170104910U, 2743257031U, 4284093946U}, /* fSigWrd */
    NULL                                                  /* fSigMem */
};

/* Function Declarations */
static real32_T b_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                                   const emlrtMsgIdentifier *parentId);

static const mxArray *b_emlrt_marshallOut(const boolean_T u);

static void b_xilHostDeserializer(boolean_T *r);

static void b_xilHostSerializer(const int32_T *r);

static void balanceCubeControllerStep_once(void);

static int32_T c_emlrt_marshallIn(const emlrtStack *sp, const mxArray *nullptr,
                                  const char_T *identifier);

static const mxArray *c_emlrt_marshallOut(const uint8_T u);

static void c_xilHostDeserializer(uint8_T *r);

static void c_xilHostSerializer(const boolean_T *r);

static int32_T d_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                                  const emlrtMsgIdentifier *parentId);

static void d_xilHostSerializer(const BalanceCubeConfig_T *r);

static boolean_T e_emlrt_marshallIn(const emlrtStack *sp,
                                    const mxArray *nullptr,
                                    const char_T *identifier);

static void e_xilHostSerializer(const real32_T r[4]);

static void emlrtExitTimeCleanupDtorFcn(const void *r);

static real32_T emlrt_marshallIn(const emlrtStack *sp, const mxArray *nullptr,
                                 const char_T *identifier);

static const mxArray *emlrt_marshallOut(const real32_T u);

static boolean_T f_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                                    const emlrtMsgIdentifier *parentId);

static BalanceCubeConfig_T g_emlrt_marshallIn(const emlrtStack *sp,
                                              const mxArray *nullptr,
                                              const char_T *identifier);

static BalanceCubeConfig_T
h_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                   const emlrtMsgIdentifier *parentId);

static void i_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                               const emlrtMsgIdentifier *parentId,
                               real32_T y[4]);

static real32_T j_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                                   const emlrtMsgIdentifier *msgId);

static int32_T k_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                                  const emlrtMsgIdentifier *msgId);

static boolean_T l_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                                    const emlrtMsgIdentifier *msgId);

static void m_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                               const emlrtMsgIdentifier *msgId,
                               real32_T ret[4]);

static void xilHostDeserializer(real32_T *r);

static void xilHostSerializer(const real32_T *r);

/* Function Definitions */
/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *u
 *                const emlrtMsgIdentifier *parentId
 * Return Type  : real32_T
 */
static real32_T b_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                                   const emlrtMsgIdentifier *parentId)
{
  real32_T y;
  y = j_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

/*
 * Arguments    : const boolean_T u
 * Return Type  : const mxArray *
 */
static const mxArray *b_emlrt_marshallOut(const boolean_T u)
{
  const mxArray *m;
  const mxArray *y;
  y = NULL;
  m = emlrtCreateLogicalScalar(u);
  emlrtAssign(&y, m);
  return y;
}

/*
 * Arguments    : boolean_T *r
 * Return Type  : void
 */
static void b_xilHostDeserializer(boolean_T *r)
{
  xilReadData((uint8_T *)r, (size_t)1, MEM_UNIT_BOOLEAN_TYPE);
}

/*
 * Arguments    : const int32_T *r
 * Return Type  : void
 */
static void b_xilHostSerializer(const int32_T *r)
{
  xilWriteData((uint8_T *)r, (size_t)1, MEM_UNIT_INT32_TYPE);
}

/*
 * Arguments    : void
 * Return Type  : void
 */
static void balanceCubeControllerStep_once(void)
{
  xilAlreadyInited = false;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *nullptr
 *                const char_T *identifier
 * Return Type  : int32_T
 */
static int32_T c_emlrt_marshallIn(const emlrtStack *sp, const mxArray *nullptr,
                                  const char_T *identifier)
{
  emlrtMsgIdentifier thisId;
  int32_T y;
  thisId.fIdentifier = (const char_T *)identifier;
  thisId.fParent = NULL;
  thisId.bParentIsCell = false;
  y = d_emlrt_marshallIn(sp, emlrtAlias(nullptr), &thisId);
  emlrtDestroyArray(&nullptr);
  return y;
}

/*
 * Arguments    : const uint8_T u
 * Return Type  : const mxArray *
 */
static const mxArray *c_emlrt_marshallOut(const uint8_T u)
{
  const mxArray *m;
  const mxArray *y;
  y = NULL;
  m = emlrtCreateNumericMatrix(1, 1, mxUINT8_CLASS, mxREAL);
  *(uint8_T *)emlrtMxGetData(m) = u;
  emlrtAssign(&y, m);
  return y;
}

/*
 * Arguments    : uint8_T *r
 * Return Type  : void
 */
static void c_xilHostDeserializer(uint8_T *r)
{
  xilReadData(r, (size_t)1, MEM_UNIT_UINT8_TYPE);
}

/*
 * Arguments    : const boolean_T *r
 * Return Type  : void
 */
static void c_xilHostSerializer(const boolean_T *r)
{
  xilWriteData((uint8_T *)r, (size_t)1, MEM_UNIT_BOOLEAN_TYPE);
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *u
 *                const emlrtMsgIdentifier *parentId
 * Return Type  : int32_T
 */
static int32_T d_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                                  const emlrtMsgIdentifier *parentId)
{
  int32_T y;
  y = k_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

/*
 * Arguments    : const BalanceCubeConfig_T *r
 * Return Type  : void
 */
static void d_xilHostSerializer(const BalanceCubeConfig_T *r)
{
  xilHostSerializer(&r->Ts);
  c_xilHostSerializer(&r->imuRotated45deg);
  c_xilHostSerializer(&r->skipCalib);
  xilHostSerializer(&r->mountTrimRad);
  xilHostSerializer(&r->cpr4x);
  xilHostSerializer(&r->encLpfBeta);
  xilHostSerializer(&r->cfAlpha);
  xilHostSerializer(&r->engageAngleRad);
  xilHostSerializer(&r->catchLimitRad);
  e_xilHostSerializer(r->K);
  xilHostSerializer(&r->VmaxEff);
  xilHostSerializer(&r->Vdeadband);
  xilHostSerializer(&r->intWMax);
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *nullptr
 *                const char_T *identifier
 * Return Type  : boolean_T
 */
static boolean_T e_emlrt_marshallIn(const emlrtStack *sp,
                                    const mxArray *nullptr,
                                    const char_T *identifier)
{
  emlrtMsgIdentifier thisId;
  boolean_T y;
  thisId.fIdentifier = (const char_T *)identifier;
  thisId.fParent = NULL;
  thisId.bParentIsCell = false;
  y = f_emlrt_marshallIn(sp, emlrtAlias(nullptr), &thisId);
  emlrtDestroyArray(&nullptr);
  return y;
}

/*
 * Arguments    : const real32_T r[4]
 * Return Type  : void
 */
static void e_xilHostSerializer(const real32_T r[4])
{
  xilWriteData((uint8_T *)&r[0], (size_t)4, MEM_UNIT_SINGLE_TYPE);
}

/*
 * Arguments    : const void *r
 * Return Type  : void
 */
static void emlrtExitTimeCleanupDtorFcn(const void *r)
{
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *nullptr
 *                const char_T *identifier
 * Return Type  : real32_T
 */
static real32_T emlrt_marshallIn(const emlrtStack *sp, const mxArray *nullptr,
                                 const char_T *identifier)
{
  emlrtMsgIdentifier thisId;
  real32_T y;
  thisId.fIdentifier = (const char_T *)identifier;
  thisId.fParent = NULL;
  thisId.bParentIsCell = false;
  y = b_emlrt_marshallIn(sp, emlrtAlias(nullptr), &thisId);
  emlrtDestroyArray(&nullptr);
  return y;
}

/*
 * Arguments    : const real32_T u
 * Return Type  : const mxArray *
 */
static const mxArray *emlrt_marshallOut(const real32_T u)
{
  const mxArray *m;
  const mxArray *y;
  y = NULL;
  m = emlrtCreateNumericMatrix(1, 1, mxSINGLE_CLASS, mxREAL);
  *(real32_T *)emlrtMxGetData(m) = u;
  emlrtAssign(&y, m);
  return y;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *u
 *                const emlrtMsgIdentifier *parentId
 * Return Type  : boolean_T
 */
static boolean_T f_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                                    const emlrtMsgIdentifier *parentId)
{
  boolean_T y;
  y = l_emlrt_marshallIn(sp, emlrtAlias(u), parentId);
  emlrtDestroyArray(&u);
  return y;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *nullptr
 *                const char_T *identifier
 * Return Type  : BalanceCubeConfig_T
 */
static BalanceCubeConfig_T g_emlrt_marshallIn(const emlrtStack *sp,
                                              const mxArray *nullptr,
                                              const char_T *identifier)
{
  BalanceCubeConfig_T y;
  emlrtMsgIdentifier thisId;
  thisId.fIdentifier = (const char_T *)identifier;
  thisId.fParent = NULL;
  thisId.bParentIsCell = false;
  y = h_emlrt_marshallIn(sp, emlrtAlias(nullptr), &thisId);
  emlrtDestroyArray(&nullptr);
  return y;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *u
 *                const emlrtMsgIdentifier *parentId
 * Return Type  : BalanceCubeConfig_T
 */
static BalanceCubeConfig_T
h_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                   const emlrtMsgIdentifier *parentId)
{
  static const int32_T dims = 0;
  static const char_T *fieldNames[13] = {"Ts",
                                         "imuRotated45deg",
                                         "skipCalib",
                                         "mountTrimRad",
                                         "cpr4x",
                                         "encLpfBeta",
                                         "cfAlpha",
                                         "engageAngleRad",
                                         "catchLimitRad",
                                         "K",
                                         "VmaxEff",
                                         "Vdeadband",
                                         "intWMax"};
  BalanceCubeConfig_T y;
  emlrtMsgIdentifier thisId;
  thisId.fParent = parentId;
  thisId.bParentIsCell = false;
  emlrtCheckStructR2012b((emlrtConstCTX)sp, parentId, u, 13,
                         (const char_T **)&fieldNames[0], 0U,
                         (const void *)&dims);
  thisId.fIdentifier = "Ts";
  y.Ts = b_emlrt_marshallIn(
      sp, emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 0, "Ts")),
      &thisId);
  thisId.fIdentifier = "imuRotated45deg";
  y.imuRotated45deg =
      f_emlrt_marshallIn(sp,
                         emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0,
                                                        1, "imuRotated45deg")),
                         &thisId);
  thisId.fIdentifier = "skipCalib";
  y.skipCalib = f_emlrt_marshallIn(
      sp,
      emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 2, "skipCalib")),
      &thisId);
  thisId.fIdentifier = "mountTrimRad";
  y.mountTrimRad =
      b_emlrt_marshallIn(sp,
                         emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0,
                                                        3, "mountTrimRad")),
                         &thisId);
  thisId.fIdentifier = "cpr4x";
  y.cpr4x = b_emlrt_marshallIn(
      sp, emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 4, "cpr4x")),
      &thisId);
  thisId.fIdentifier = "encLpfBeta";
  y.encLpfBeta = b_emlrt_marshallIn(
      sp,
      emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 5, "encLpfBeta")),
      &thisId);
  thisId.fIdentifier = "cfAlpha";
  y.cfAlpha = b_emlrt_marshallIn(
      sp,
      emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 6, "cfAlpha")),
      &thisId);
  thisId.fIdentifier = "engageAngleRad";
  y.engageAngleRad =
      b_emlrt_marshallIn(sp,
                         emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0,
                                                        7, "engageAngleRad")),
                         &thisId);
  thisId.fIdentifier = "catchLimitRad";
  y.catchLimitRad =
      b_emlrt_marshallIn(sp,
                         emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0,
                                                        8, "catchLimitRad")),
                         &thisId);
  thisId.fIdentifier = "K";
  i_emlrt_marshallIn(
      sp, emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 9, "K")),
      &thisId, y.K);
  thisId.fIdentifier = "VmaxEff";
  y.VmaxEff = b_emlrt_marshallIn(
      sp,
      emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 10, "VmaxEff")),
      &thisId);
  thisId.fIdentifier = "Vdeadband";
  y.Vdeadband = b_emlrt_marshallIn(
      sp,
      emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 11, "Vdeadband")),
      &thisId);
  thisId.fIdentifier = "intWMax";
  y.intWMax = b_emlrt_marshallIn(
      sp,
      emlrtAlias(emlrtGetFieldR2017b((emlrtConstCTX)sp, u, 0, 12, "intWMax")),
      &thisId);
  emlrtDestroyArray(&u);
  return y;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *u
 *                const emlrtMsgIdentifier *parentId
 *                real32_T y[4]
 * Return Type  : void
 */
static void i_emlrt_marshallIn(const emlrtStack *sp, const mxArray *u,
                               const emlrtMsgIdentifier *parentId,
                               real32_T y[4])
{
  m_emlrt_marshallIn(sp, emlrtAlias(u), parentId, y);
  emlrtDestroyArray(&u);
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *src
 *                const emlrtMsgIdentifier *msgId
 * Return Type  : real32_T
 */
static real32_T j_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                                   const emlrtMsgIdentifier *msgId)
{
  static const int32_T dims = 0;
  real32_T ret;
  emlrtCheckBuiltInR2012b((emlrtConstCTX)sp, msgId, src, "single", false, 0U,
                          (const void *)&dims);
  ret = *(real32_T *)emlrtMxGetData(src);
  emlrtDestroyArray(&src);
  return ret;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *src
 *                const emlrtMsgIdentifier *msgId
 * Return Type  : int32_T
 */
static int32_T k_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                                  const emlrtMsgIdentifier *msgId)
{
  static const int32_T dims = 0;
  int32_T ret;
  emlrtCheckBuiltInR2012b((emlrtConstCTX)sp, msgId, src, "int32", false, 0U,
                          (const void *)&dims);
  ret = *(int32_T *)emlrtMxGetData(src);
  emlrtDestroyArray(&src);
  return ret;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *src
 *                const emlrtMsgIdentifier *msgId
 * Return Type  : boolean_T
 */
static boolean_T l_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                                    const emlrtMsgIdentifier *msgId)
{
  static const int32_T dims = 0;
  boolean_T ret;
  emlrtCheckBuiltInR2012b((emlrtConstCTX)sp, msgId, src, "logical", false, 0U,
                          (const void *)&dims);
  ret = *emlrtMxGetLogicals(src);
  emlrtDestroyArray(&src);
  return ret;
}

/*
 * Arguments    : const emlrtStack *sp
 *                const mxArray *src
 *                const emlrtMsgIdentifier *msgId
 *                real32_T ret[4]
 * Return Type  : void
 */
static void m_emlrt_marshallIn(const emlrtStack *sp, const mxArray *src,
                               const emlrtMsgIdentifier *msgId, real32_T ret[4])
{
  static const int32_T dims[2] = {1, 4};
  real32_T(*r)[4];
  emlrtCheckBuiltInR2012b((emlrtConstCTX)sp, msgId, src, "single", false, 2U,
                          (const void *)&dims[0]);
  r = (real32_T(*)[4])emlrtMxGetData(src);
  ret[0] = (*r)[0];
  ret[1] = (*r)[1];
  ret[2] = (*r)[2];
  ret[3] = (*r)[3];
  emlrtDestroyArray(&src);
}

/*
 * Arguments    : real32_T *r
 * Return Type  : void
 */
static void xilHostDeserializer(real32_T *r)
{
  xilReadData((uint8_T *)r, (size_t)1, MEM_UNIT_SINGLE_TYPE);
}

/*
 * Arguments    : const real32_T *r
 * Return Type  : void
 */
static void xilHostSerializer(const real32_T *r)
{
  xilWriteData((uint8_T *)r, (size_t)1, MEM_UNIT_SINGLE_TYPE);
}

/*
 * Arguments    : real32_T axMps2
 *                real32_T ayMps2
 *                real32_T gzRads
 *                int32_T encTicksDelta
 *                boolean_T resetState
 *                const BalanceCubeConfig_T *cfgParams
 *                real32_T *pwmDuty
 *                boolean_T *in1High
 *                boolean_T *in2High
 *                uint8_T *modeOut
 *                real32_T *thetaHatDeg
 *                real32_T *omegaWRads
 * Return Type  : void
 */
void balanceCubeControllerStepXilWrapper(
    real32_T axMps2, real32_T ayMps2, real32_T gzRads, int32_T encTicksDelta,
    boolean_T resetState, const BalanceCubeConfig_T *cfgParams,
    real32_T *pwmDuty, boolean_T *in1High, boolean_T *in2High, uint8_T *modeOut,
    real32_T *thetaHatDeg, real32_T *omegaWRads)
{
  /* Serialize function input argument axMps2. */
  xilHostSerializer(&axMps2);
  /* Serialize function input argument ayMps2. */
  xilHostSerializer(&ayMps2);
  /* Serialize function input argument gzRads. */
  xilHostSerializer(&gzRads);
  /* Serialize function input argument encTicksDelta. */
  b_xilHostSerializer(&encTicksDelta);
  /* Serialize function input argument resetState. */
  c_xilHostSerializer(&resetState);
  /* Serialize function input argument cfgParams. */
  d_xilHostSerializer(cfgParams);
  /* Calling XIL to invoke target side. */
  xilEntryPointHost(1U);
  /* Deserialize function output argument pwmDuty. */
  xilHostDeserializer(pwmDuty);
  /* Deserialize function output argument in1High. */
  b_xilHostDeserializer(in1High);
  /* Deserialize function output argument in2High. */
  b_xilHostDeserializer(in2High);
  /* Deserialize function output argument modeOut. */
  c_xilHostDeserializer(modeOut);
  /* Deserialize function output argument thetaHatDeg. */
  xilHostDeserializer(thetaHatDeg);
  /* Deserialize function output argument omegaWRads. */
  xilHostDeserializer(omegaWRads);
}

/*
 * Arguments    : const mxArray * const prhs[6]
 *                int32_T nlhs
 *                const mxArray *plhs[6]
 * Return Type  : void
 */
void balanceCubeControllerStep_api(const mxArray *const prhs[6], int32_T nlhs,
                                   const mxArray *plhs[6])
{
  BalanceCubeConfig_T cfgParams;
  emlrtStack st = {
      NULL, /* site */
      NULL, /* tls */
      NULL  /* prev */
  };
  int32_T encTicksDelta;
  real32_T axMps2;
  real32_T ayMps2;
  real32_T gzRads;
  real32_T omegaWRads;
  real32_T pwmDuty;
  real32_T thetaHatDeg;
  uint8_T modeOut;
  boolean_T in1High;
  boolean_T in2High;
  boolean_T resetState;
  st.tls = emlrtRootTLSGlobal;
  /* Marshall function inputs */
  axMps2 = emlrt_marshallIn(&st, emlrtAliasP(prhs[0]), "axMps2");
  ayMps2 = emlrt_marshallIn(&st, emlrtAliasP(prhs[1]), "ayMps2");
  gzRads = emlrt_marshallIn(&st, emlrtAliasP(prhs[2]), "gzRads");
  encTicksDelta =
      c_emlrt_marshallIn(&st, emlrtAliasP(prhs[3]), "encTicksDelta");
  resetState = e_emlrt_marshallIn(&st, emlrtAliasP(prhs[4]), "resetState");
  cfgParams = g_emlrt_marshallIn(&st, emlrtAliasP(prhs[5]), "cfgParams");
  /* Calling XIL to setup. */
  xilPreEntryPointHost(1U);
  /* Invoke the wrapper function */
  balanceCubeControllerStepXilWrapper(
      axMps2, ayMps2, gzRads, encTicksDelta, resetState, &cfgParams, &pwmDuty,
      &in1High, &in2High, &modeOut, &thetaHatDeg, &omegaWRads);
  /* Calling Xil to clean-up. */
  xilPostEntryPointHost(1U);
  /* Marshall function outputs */
  plhs[0] = emlrt_marshallOut(pwmDuty);
  if (nlhs > 1) {
    plhs[1] = b_emlrt_marshallOut(in1High);
  }
  if (nlhs > 2) {
    plhs[2] = b_emlrt_marshallOut(in2High);
  }
  if (nlhs > 3) {
    plhs[3] = c_emlrt_marshallOut(modeOut);
  }
  if (nlhs > 4) {
    plhs[4] = emlrt_marshallOut(thetaHatDeg);
  }
  if (nlhs > 5) {
    plhs[5] = emlrt_marshallOut(omegaWRads);
  }
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void balanceCubeControllerStep_atexit(void)
{
  emlrtStack st = {
      NULL, /* site */
      NULL, /* tls */
      NULL  /* prev */
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtPushHeapReferenceStackR2021a(
      &st, false, NULL, (void *)&emlrtExitTimeCleanupDtorFcn, NULL, NULL, NULL);
  emlrtEnterRtStackR2012b(&st);
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
  balanceCubeControllerStep_xil_terminate();
  balanceCubeControllerStep_xil_shutdown();
  emlrtExitTimeCleanup(&emlrtContextGlobal);
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void balanceCubeControllerStep_initialize(void)
{
  emlrtStack st = {
      NULL, /* site */
      NULL, /* tls */
      NULL  /* prev */
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtClearAllocCountR2012b(&st, false, 0U, NULL);
  emlrtEnterRtStackR2012b(&st);
  if (emlrtFirstTimeR2012b(emlrtRootTLSGlobal)) {
    balanceCubeControllerStep_once();
  }
  if (!xilAlreadyInited) {
    xilInitializeHost(&xil_terminate);
    xilAlreadyInited = true;
  }
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void balanceCubeControllerStep_terminate(void)
{
  emlrtDestroyRootTLS(&emlrtRootTLSGlobal);
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void xil_terminate(void)
{
  xilAlreadyInited = false;
  balanceCubeControllerStep_terminate();
}

/*
 * File trailer for _coder_balanceCubeControllerStep_api.c
 *
 * [EOF]
 */
