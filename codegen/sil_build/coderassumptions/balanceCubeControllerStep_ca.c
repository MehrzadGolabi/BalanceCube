/*
 * File: balanceCubeControllerStep_ca.c
 *
 * Abstract: Tests assumptions in the generated code.
 */

#include "balanceCubeControllerStep_ca.h"

CA_ChecksTestResults CA_balanceCubeControllerStep_Res;
CA_PWS_TestResults CA_balanceCubeControllerStep_PWSRes;
const int numberOfImportedTypes = 0;
const CA_Checks CA_balanceCubeControllerStep_Exp = {
  8,                                   /* BitPerChar */
  16,                                  /* BitPerShort */
  32,                                  /* BitPerInt */
  64,                                  /* BitPerLong */
  64,                                  /* BitPerLongLong */
  32,                                  /* BitPerFloat */
  64,                                  /* BitPerDouble */
  64,                                  /* BitPerPointer */
  64,                                  /* BitPerSizeT */
  64,                                  /* BitPerPtrDiffT */
  CA_LITTLE_ENDIAN,                    /* Endianess */
  CA_ZERO,                             /* IntDivRoundTo */
  1,                                   /* ShiftRightIntArith */
  1,                                   /* LongLongMode */
  0,                                   /* PortableWordSizes */
  "Generic->MATLAB Host Computer",     /* HWDeviceType */
  0,                                   /* MemoryAtStartup */
  0,                                   /* DynamicMemoryAtStartup */
  0,                                   /* DenormalFlushToZero */
  0,                                   /* DenormalAsZero */
  0                                    /* Imported Types */
};

CA_Checks CA_balanceCubeControllerStep_Act = {
  0,                                   /* BitPerChar */
  0,                                   /* BitPerShort */
  0,                                   /* BitPerInt */
  0,                                   /* BitPerLong */
  0,                                   /* BitPerLongLong */
  0,                                   /* BitPerFloat */
  0,                                   /* BitPerDouble */
  0,                                   /* BitPerPointer */
  0,                                   /* BitPerSizeT */
  0,                                   /* BitPerPtrDiffT */
  CA_UNSPECIFIED,                      /* Endianess */
  CA_UNDEFINED,                        /* IntDivRoundTo */
  0,                                   /* ShiftRightIntArith */
  0,                                   /* LongLongMode */
  0,                                   /* PortableWordSizes */
  "",                                  /* HWDeviceType */
  0,                                   /* MemoryAtStartup */
  0,                                   /* DynamicMemoryAtStartup */
  0,                                   /* DenormalFlushToZero */
  0,                                   /* DenormalAsZero */
  0                                    /* Imported Types */
};

void balanceCubeControllerStep_caRunTests(void)
{
  /* verify hardware implementation */
  caVerifyPortableWordSizes(&CA_balanceCubeControllerStep_Act,
    &CA_balanceCubeControllerStep_Exp, &CA_balanceCubeControllerStep_PWSRes);
  caVerifyChecks(&CA_balanceCubeControllerStep_Act,
                 &CA_balanceCubeControllerStep_Exp,
                 &CA_balanceCubeControllerStep_Res, numberOfImportedTypes);
}
