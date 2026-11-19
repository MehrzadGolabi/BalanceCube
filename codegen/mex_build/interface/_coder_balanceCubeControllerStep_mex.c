/*
 * _coder_balanceCubeControllerStep_mex.c
 *
 * Code generation for function '_coder_balanceCubeControllerStep_mex'
 *
 */

/* Include files */
#include "_coder_balanceCubeControllerStep_mex.h"
#include "_coder_balanceCubeControllerStep_api.h"
#include "balanceCubeControllerStep_data.h"
#include "balanceCubeControllerStep_initialize.h"
#include "balanceCubeControllerStep_terminate.h"
#include "rt_nonfinite.h"

/* Function Definitions */
void balanceCubeControllerStep_mexFunction(int32_T nlhs, mxArray *plhs[6],
                                           int32_T nrhs, const mxArray *prhs[6])
{
  emlrtStack st = {
      NULL, /* site */
      NULL, /* tls */
      NULL  /* prev */
  };
  const mxArray *outputs[6];
  int32_T i;
  st.tls = emlrtRootTLSGlobal;
  /* Check for proper number of arguments. */
  if (nrhs != 6) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:WrongNumberOfInputs", 5, 12, 6, 4,
                        25, "balanceCubeControllerStep");
  }
  if (nlhs > 6) {
    emlrtErrMsgIdAndTxt(&st, "EMLRT:runTime:TooManyOutputArguments", 3, 4, 25,
                        "balanceCubeControllerStep");
  }
  /* Call the function. */
  balanceCubeControllerStep_api(prhs, nlhs, outputs);
  /* Copy over outputs to the caller. */
  if (nlhs < 1) {
    i = 1;
  } else {
    i = nlhs;
  }
  emlrtReturnArrays(i, &plhs[0], &outputs[0]);
}

void mexFunction(int32_T nlhs, mxArray *plhs[], int32_T nrhs,
                 const mxArray *prhs[])
{
  mexAtExit(&balanceCubeControllerStep_atexit);
  balanceCubeControllerStep_initialize();
  balanceCubeControllerStep_mexFunction(nlhs, plhs, nrhs, prhs);
  balanceCubeControllerStep_terminate();
}

emlrtCTX mexFunctionCreateRootTLS(void)
{
  emlrtCreateRootTLSR2022a(&emlrtRootTLSGlobal, &emlrtContextGlobal, NULL, 1,
                           NULL, "UTF-8", true);
  return emlrtRootTLSGlobal;
}

/* End of code generation (_coder_balanceCubeControllerStep_mex.c) */
