/*
 *
 *  Copyright (C) 1997-2019, OFFIS e.V.
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
 *  Module:  dcmj2k
 *
 *  Author:  Martin Willkomm, Uli Schlachter
 *
 *  Purpose: singleton class that registers encoders for all supported JPEG processes.
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmj2k/djencode.h"
#include "dcmtk/dcmdata/dccodec.h"  /* for DcmCodecStruct */
#include "dcmtk/dcmj2k/djcparam.h"
#include "dcmtk/dcmj2k/djcodece.h"

// initialization of static members
OFBool DJ2KEncoderRegistration::registered_                             = OFFalse;
DJ2KCodecParameter *DJ2KEncoderRegistration::cp_                        = NULL;
DJ2KLosslessEncoder *DJ2KEncoderRegistration::losslessencoder_          = NULL;
DJ2KNearLosslessEncoder *DJ2KEncoderRegistration::nearlosslessencoder_  = NULL;


void DJ2KEncoderRegistration::registerCodecs(
    Uint16 jpls_t1,
    Uint16 jpls_t2,
    Uint16 jpls_t3,
    Uint16 jpls_reset,
    OFBool preferCookedEncoding,
    Uint32 fragmentSize,
    OFBool createOffsetTable,
    J2K_UIDCreation uidCreation,
    OFBool convertToSC,
    DJ2KCodecParameter::interleaveMode jplsInterleaveMode,
    OFBool useFFbitstreamPadding)
{
  if (! registered_)
  {
    cp_ = new DJ2KCodecParameter(preferCookedEncoding, jpls_t1, jpls_t2, jpls_t3,
      jpls_reset, fragmentSize, createOffsetTable, uidCreation,
      convertToSC, EJ2KPC_restore, OFFalse, jplsInterleaveMode, useFFbitstreamPadding);

    if (cp_)
    {
      losslessencoder_ = new DJ2KLosslessEncoder();
      if (losslessencoder_) DcmCodecList::registerCodec(losslessencoder_, NULL, cp_);
      nearlosslessencoder_ = new DJ2KNearLosslessEncoder();
      if (nearlosslessencoder_) DcmCodecList::registerCodec(nearlosslessencoder_, NULL, cp_);
      registered_ = OFTrue;
    }
  }
}

void DJ2KEncoderRegistration::cleanup()
{
  if (registered_)
  {
    DcmCodecList::deregisterCodec(losslessencoder_);
    DcmCodecList::deregisterCodec(nearlosslessencoder_);
    delete losslessencoder_;
    delete nearlosslessencoder_;
    delete cp_;
    registered_ = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    losslessencoder_ = NULL;
    nearlosslessencoder_ = NULL;
    cp_     = NULL;
#endif
  }
}

OFString DJ2KEncoderRegistration::getLibraryVersionString()
{
    return DCMJ2K_OPENJPEG_VERSION_STRING;
}
