/*
 * balanceCubeControllerStep_initialize.c
 *
 * Code generation for function 'balanceCubeControllerStep_initialize'
 *
 */

/* Include files */
#include "balanceCubeControllerStep_initialize.h"
#include "_coder_balanceCubeControllerStep_mex.h"
#include "balanceCubeControllerStep.h"
#include "balanceCubeControllerStep_data.h"
#include "rt_nonfinite.h"

/* Function Declarations */
static void balanceCubeControllerStep_once(void);

/* Function Definitions */
static void balanceCubeControllerStep_once(void)
{
  mex_InitInfAndNan();
  balanceCubeControllerStep_init();
}

void balanceCubeControllerStep_initialize(void)
{
  static const volatile char_T *emlrtBreakCheckR2012bFlagVar = NULL;
  emlrtStack st = {
      NULL, /* site */
      NULL, /* tls */
      NULL  /* prev */
  };
  mexFunctionCreateRootTLS();
  st.tls = emlrtRootTLSGlobal;
  emlrtBreakCheckR2012bFlagVar = emlrtGetBreakCheckFlagAddressR2022b(&st);
  emlrtClearAllocCountR2012b(&st, false, 0U, NULL);
  emlrtEnterRtStackR2012b(&st);
  if (emlrtFirstTimeR2012b(emlrtRootTLSGlobal)) {
    balanceCubeControllerStep_once();
  }
}

/* End of code generation (balanceCubeControllerStep_initialize.c) */
