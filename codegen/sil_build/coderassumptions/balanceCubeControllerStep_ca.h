/*
 * File: balanceCubeControllerStep_ca.h
 *
 * Abstract: Tests assumptions in the generated code.
 */

#ifndef BALANCECUBECONTROLLERSTEP_CA_H
#define BALANCECUBECONTROLLERSTEP_CA_H

/* preprocessor validation checks */
#include "balanceCubeControllerStep_ca_preproc.h"
#include "coder_assumptions_hwimpl.h"

/* variables holding test results */
extern CA_ChecksTestResults CA_balanceCubeControllerStep_Res;
extern CA_PWS_TestResults CA_balanceCubeControllerStep_PWSRes;

/* variables holding "expected" and "actual" hardware implementation */
extern const CA_Checks CA_balanceCubeControllerStep_Exp;
extern CA_Checks CA_balanceCubeControllerStep_Act;
extern const int numberOfImportedTypes;

/* entry point function to run tests */
void balanceCubeControllerStep_caRunTests(void);

#endif                                 /* BALANCECUBECONTROLLERSTEP_CA_H */
