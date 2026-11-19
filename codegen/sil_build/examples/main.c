/*
 * File: main.c
 *
 * MATLAB Coder version            : 25.2
 * C/C++ source code generated on  : 18-Nov-2026 06:08:59
 */

/*************************************************************************/
/* This automatically generated example C main file shows how to call    */
/* entry-point functions that MATLAB Coder generated. You must customize */
/* this file for your application. Do not modify this file directly.     */
/* Instead, make a copy of this file, modify it, and integrate it into   */
/* your development environment.                                         */
/*                                                                       */
/* This file initializes entry-point function arguments to a default     */
/* size and value before calling the entry-point functions. It does      */
/* not store or use any values returned from the entry-point functions.  */
/* If necessary, it does pre-allocate memory for returned values.        */
/* You can use this file as a starting point for a main function that    */
/* you can deploy in your application.                                   */
/*                                                                       */
/* After you copy the file, and before you deploy it, you must make the  */
/* following changes:                                                    */
/* * For variable-size function arguments, change the example sizes to   */
/* the sizes that your application requires.                             */
/* * Change the example values of function arguments to the values that  */
/* your application requires.                                            */
/* * If the entry-point functions return values, store these values or   */
/* otherwise use them as required by your application.                   */
/*                                                                       */
/*************************************************************************/

/* Include Files */
#include "main.h"
#include "balanceCubeControllerStep.h"
#include "balanceCubeControllerStep_initialize.h"
#include "balanceCubeControllerStep_terminate.h"
#include "balanceCubeControllerStep_types.h"

/* Function Declarations */
static void argInit_1x4_real32_T(float result[4]);

static BalanceCubeConfig_T argInit_BalanceCubeConfig_T(void);

static boolean_T argInit_boolean_T(void);

static int argInit_int32_T(void);

static float argInit_real32_T(void);

/* Function Definitions */
/*
 * Arguments    : float result[4]
 * Return Type  : void
 */
static void argInit_1x4_real32_T(float result[4])
{
  int idx1;
  /* Loop over the array to initialize each element. */
  for (idx1 = 0; idx1 < 4; idx1++) {
    /* Set the value of the array element.
Change this value to the value that the application requires. */
    result[idx1] = argInit_real32_T();
  }
}

/*
 * Arguments    : void
 * Return Type  : BalanceCubeConfig_T
 */
static BalanceCubeConfig_T argInit_BalanceCubeConfig_T(void)
{
  BalanceCubeConfig_T result;
  float b_result_tmp;
  boolean_T result_tmp;
  /* Set the value of each structure field.
Change this value to the value that the application requires. */
  result_tmp = argInit_boolean_T();
  b_result_tmp = argInit_real32_T();
  result.Ts = b_result_tmp;
  result.imuRotated45deg = result_tmp;
  result.skipCalib = result_tmp;
  result.mountTrimRad = b_result_tmp;
  result.cpr4x = b_result_tmp;
  result.encLpfBeta = b_result_tmp;
  result.cfAlpha = b_result_tmp;
  result.engageAngleRad = b_result_tmp;
  result.catchLimitRad = b_result_tmp;
  argInit_1x4_real32_T(result.K);
  result.VmaxEff = b_result_tmp;
  result.Vdeadband = b_result_tmp;
  result.intWMax = b_result_tmp;
  return result;
}

/*
 * Arguments    : void
 * Return Type  : boolean_T
 */
static boolean_T argInit_boolean_T(void)
{
  return false;
}

/*
 * Arguments    : void
 * Return Type  : int
 */
static int argInit_int32_T(void)
{
  return 0;
}

/*
 * Arguments    : void
 * Return Type  : float
 */
static float argInit_real32_T(void)
{
  return 0.0F;
}

/*
 * Arguments    : int argc
 *                char **argv
 * Return Type  : int
 */
int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  /* Initialize the application.
You do not need to do this more than one time. */
  balanceCubeControllerStep_initialize();
  /* Invoke the entry-point functions.
You can call entry-point functions multiple times. */
  main_balanceCubeControllerStep();
  /* Terminate the application.
You do not need to do this more than one time. */
  balanceCubeControllerStep_terminate();
  return 0;
}

/*
 * Arguments    : void
 * Return Type  : void
 */
void main_balanceCubeControllerStep(void)
{
  BalanceCubeConfig_T r;
  float axMps2_tmp;
  float omegaWRads;
  float pwmDuty;
  float thetaHatDeg;
  unsigned char modeOut;
  boolean_T in1High;
  boolean_T in2High;
  /* Initialize function 'balanceCubeControllerStep' input arguments. */
  axMps2_tmp = argInit_real32_T();
  /* Initialize function input argument 'cfgParams'. */
  /* Call the entry-point 'balanceCubeControllerStep'. */
  r = argInit_BalanceCubeConfig_T();
  balanceCubeControllerStep(axMps2_tmp, axMps2_tmp, axMps2_tmp,
                            argInit_int32_T(), argInit_boolean_T(), &r,
                            &pwmDuty, &in1High, &in2High, &modeOut,
                            &thetaHatDeg, &omegaWRads);
}

/*
 * File trailer for main.c
 *
 * [EOF]
 */
