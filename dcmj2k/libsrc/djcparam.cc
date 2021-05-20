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
 *  Purpose: codec parameter class for JPEG-LS codecs
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmj2k/djcparam.h"
#include "dcmtk/ofstd/ofstd.h"

DJ2KCodecParameter::DJ2KCodecParameter(
     OFBool preferCookedEncoding,
     Uint16 jpls_t1,
     Uint16 jpls_t2,
     Uint16 jpls_t3,
     Uint16 jpls_reset,
     Uint32 fragmentSize,
     OFBool createOffsetTable,
     J2K_UIDCreation uidCreation,
     OFBool convertToSC,
     J2K_PlanarConfiguration planarConfiguration,
     OFBool ignoreOffsetTble,
     interleaveMode jplsInterleaveMode,
     OFBool useFFbitstreamPadding)
: DcmCodecParameter()
, preferCookedEncoding_(preferCookedEncoding)
, jpls_t1_(jpls_t1)
, jpls_t2_(jpls_t2)
, jpls_t3_(jpls_t3)
, jpls_reset_(jpls_reset)
, fragmentSize_(fragmentSize)
, createOffsetTable_(createOffsetTable)
, uidCreation_(uidCreation)
, convertToSC_(convertToSC)
, jplsInterleaveMode_(jplsInterleaveMode)
, useFFbitstreamPadding_(useFFbitstreamPadding)
, planarConfiguration_(planarConfiguration)
, ignoreOffsetTable_(ignoreOffsetTble)
, forceSingleFragmentPerFrame_(OFFalse)
{
}


DJ2KCodecParameter::DJ2KCodecParameter(
    J2K_UIDCreation uidCreation,
    J2K_PlanarConfiguration planarConfiguration,
    OFBool ignoreOffsetTble,
    OFBool forceSingleFragmentPerFrame)
: DcmCodecParameter()
, preferCookedEncoding_(OFTrue)
, jpls_t1_(0)
, jpls_t2_(0)
, jpls_t3_(0)
, jpls_reset_(0)
, fragmentSize_(0)
, createOffsetTable_(OFTrue)
, uidCreation_(uidCreation)
, convertToSC_(OFFalse)
, jplsInterleaveMode_(interleaveDefault)
, useFFbitstreamPadding_(OFTrue)
, planarConfiguration_(planarConfiguration)
, ignoreOffsetTable_(ignoreOffsetTble)
, forceSingleFragmentPerFrame_(forceSingleFragmentPerFrame)
{
}

DJ2KCodecParameter::DJ2KCodecParameter(const DJ2KCodecParameter& arg)
: DcmCodecParameter(arg)
, preferCookedEncoding_(arg.preferCookedEncoding_)
, jpls_t1_(arg.jpls_t1_)
, jpls_t2_(arg.jpls_t2_)
, jpls_t3_(arg.jpls_t3_)
, jpls_reset_(arg.jpls_reset_)
, fragmentSize_(arg.fragmentSize_)
, createOffsetTable_(arg.createOffsetTable_)
, uidCreation_(arg.uidCreation_)
, convertToSC_(arg.convertToSC_)
, jplsInterleaveMode_(arg.jplsInterleaveMode_)
, useFFbitstreamPadding_(arg.useFFbitstreamPadding_)
, planarConfiguration_(arg.planarConfiguration_)
, ignoreOffsetTable_(arg.ignoreOffsetTable_)
, forceSingleFragmentPerFrame_(arg.forceSingleFragmentPerFrame_)
{
}

DJ2KCodecParameter::~DJ2KCodecParameter()
{
}

DcmCodecParameter *DJ2KCodecParameter::clone() const
{
  return new DJ2KCodecParameter(*this);
}

const char *DJ2KCodecParameter::className() const
{
  return "DJ2KCodecParameter";
}
