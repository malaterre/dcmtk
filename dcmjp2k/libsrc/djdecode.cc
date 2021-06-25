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
 *  Module:  dcmjp2k
 *
 *  Author:  Martin Willkomm
 *
 *  Purpose: singleton class that registers decoders for all supported JPEG-LS processes.
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2k/djdecode.h"
#include "dcmtk/dcmdata/dccodec.h"  /* for DcmCodecStruct */
#include "dcmtk/dcmjp2k/djcparam.h"
#include "dcmtk/dcmjp2k/djcodecd.h"

// initialization of static members
OFBool DJ2KDecoderRegistration::registered_                            = OFFalse;
DJ2KCodecParameter *DJ2KDecoderRegistration::cp_                       = NULL;
DJ2KLosslessDecoder *DJ2KDecoderRegistration::losslessdecoder_         = NULL;
DJ2KNearLosslessDecoder *DJ2KDecoderRegistration::nearlosslessdecoder_ = NULL;

void DJ2KDecoderRegistration::registerCodecs(
    J2K_UIDCreation uidcreation,
    J2K_PlanarConfiguration planarconfig,
    OFBool ignoreOffsetTable,
    OFBool forceSingleFragmentPerFrame)
{
  if (! registered_)
  {
    cp_ = new DJ2KCodecParameter(uidcreation, planarconfig, ignoreOffsetTable, forceSingleFragmentPerFrame);
    if (cp_)
    {
      losslessdecoder_ = new DJ2KLosslessDecoder();
      if (losslessdecoder_) DcmCodecList::registerCodec(losslessdecoder_, NULL, cp_);

      nearlosslessdecoder_ = new DJ2KNearLosslessDecoder();
      if (nearlosslessdecoder_) DcmCodecList::registerCodec(nearlosslessdecoder_, NULL, cp_);
      registered_ = OFTrue;
    }
  }
}

void DJ2KDecoderRegistration::cleanup()
{
  if (registered_)
  {
    DcmCodecList::deregisterCodec(losslessdecoder_);
    DcmCodecList::deregisterCodec(nearlosslessdecoder_);
    delete losslessdecoder_;
    delete nearlosslessdecoder_;
    delete cp_;
    registered_ = OFFalse;
#ifdef DEBUG
    // not needed but useful for debugging purposes
    losslessdecoder_ = NULL;
    nearlosslessdecoder_ = NULL;
    cp_      = NULL;
#endif
  }
}

OFString DJ2KDecoderRegistration::getLibraryVersionString()
{
    return DCMJ2K_OPENJPEG_VERSION_STRING;
}
