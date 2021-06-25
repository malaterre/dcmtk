/*
 *
 *  Copyright (C) 1997-2010, OFFIS e.V.
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
 *  Purpose: representation parameter for JPEG-LS
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2k/djrparam.h"
#include "dcmtk/ofstd/ofstd.h"

DJ2KRepresentationParameter::DJ2KRepresentationParameter(
    Uint16 nearlosslessDeviation,
    OFBool losslessProcess)
: DcmRepresentationParameter()
, nearlosslessDeviation_(nearlosslessDeviation)
, losslessProcess_(losslessProcess)
{
}

DJ2KRepresentationParameter::DJ2KRepresentationParameter(const DJ2KRepresentationParameter& arg)
: DcmRepresentationParameter(arg)
, nearlosslessDeviation_(arg.nearlosslessDeviation_)
, losslessProcess_(arg.losslessProcess_)
{
}

DJ2KRepresentationParameter::~DJ2KRepresentationParameter()
{
}  

DcmRepresentationParameter *DJ2KRepresentationParameter::clone() const
{
  return new DJ2KRepresentationParameter(*this);
}

const char *DJ2KRepresentationParameter::className() const
{
  return "DJ2KRepresentationParameter";
}

OFBool DJ2KRepresentationParameter::operator==(const DcmRepresentationParameter &arg) const
{
  const char *argname = arg.className();
  if (argname)
  {
    OFString argstring(argname);
    if (argstring == className())
    {
      const DJ2KRepresentationParameter& argll = OFreinterpret_cast(const DJ2KRepresentationParameter &, arg);
      if (losslessProcess_ && argll.losslessProcess_) return OFTrue;
      else if (losslessProcess_ != argll.losslessProcess_) return OFFalse;
	  else if (nearlosslessDeviation_ != argll.nearlosslessDeviation_) return OFFalse;
      return OFTrue;
    }	
  }
  return OFFalse;
}
