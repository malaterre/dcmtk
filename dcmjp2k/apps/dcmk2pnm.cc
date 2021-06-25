/*
 *
 *  Copyright (C) 2001-2011, OFFIS e.V.
 *  All rights reserved.  See COPYRIGHT file for details.
 *
 *  This software and supporting documentation were developed by
 *
 *    OFFIS e.V.
 *    R&D Division Health
 *    Escherweg 2
 *    D-26121 Oldenburg, Germany
 *
 *
 *  Module:  dcmjp2k
 *
 *  Author:  Uli Schlachter
 *
 *  Purpose: Convert DICOM Images to PPM or PGM using the dcmimage/dcmjp2k library.
 *
 */


// compile "dcm2pnm" with dcmjp2k support
#define BUILD_DCM2PNM_AS_DCMK2PNM

// include full implementation of "dcm2pnm"
#include "../../dcmimage/apps/dcm2pnm.cc"
