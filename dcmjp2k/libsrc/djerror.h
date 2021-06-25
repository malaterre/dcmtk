/*
 *
 *  Copyright (C) 1997-2012, OFFIS e.V.
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
 *  Purpose: Helper function than converts between CharLS and dcmjpgls errors
 *
 */

#ifndef DJERROR_H
#define DJERROR_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2k/dj2kutil.h" /* For the OFCondition codes */
#include "openjpeg.h" /* OpenJPEG include */

/** Helper class for converting between dcmjp2k and OpenJPEG error codes
 */
class DJ2KError
{
private:
  /// private undefined constructor
  DJ2KError();

public:

  /** This method converts a OpenJPEG error code into a dcmjp2k OFCondition
   *  @param error The CharLS error code
   *  @return The OFCondition
   */
#if 0
  static const OFConditionConst& convert(J2K_ERROR error)
  {
    switch (error)
    {
      case OK:
        return EC_Normal;
      case UncompressedBufferTooSmall:
        return EC_J2KUncompressedBufferTooSmall;
      case CompressedBufferTooSmall:
        return EC_J2KCompressedBufferTooSmall;
      case ImageTypeNotSupported:
        return EC_J2KCodecUnsupportedImageType;
      case InvalidJlsParameters:
        return EC_J2KCodecInvalidParameters;
      case ParameterValueNotSupported:
        return EC_J2KCodecUnsupportedValue;
      case InvalidCompressedData:
        return EC_J2KInvalidCompressedData;
      case UnsupportedBitDepthForTransform:
        return EC_J2KUnsupportedBitDepthForTransform;
      case UnsupportedColorTransform:
        return EC_J2KUnsupportedColorTransform;
      case TooMuchCompressedData:
        return EC_J2KTooMuchCompressedData;
      default:
        return EC_IllegalParameter;
    }
  }
#endif
};

#endif
