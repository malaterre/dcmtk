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
 *  Author:  Martin Willkomm, Uli Schlachter
 *
 *  Purpose: singleton class that registers encoders for all supported JPEG-LS processes.
 *
 */

#ifndef DCMJ2K_DJENCODE_H
#define DCMJ2K_DJENCODE_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oftypes.h"  /* for OFBool */
#include "dcmtk/dcmjp2k/dj2kutil.h"
#include "dcmtk/dcmdata/dctypes.h"  /* for Uint32 */
#include "dcmtk/dcmjp2k/djcparam.h" /* for class DJ2KCodecParameter */

class DJ2KCodecParameter;
class DJ2KLosslessEncoder;
class DJ2KNearLosslessEncoder;

/** singleton class that registers encoders for all supported JPEG-LS processes.
 */
class DCMTK_DCMJ2K_EXPORT DJ2KEncoderRegistration
{
public:

  /** registers encoders for all supported JPEG-LS processes. 
   *  If already registered, call is ignored unless cleanup() has
   *  been performed before.
   *  @param jpls_t1                   JPEG-LS parameter "Threshold 1" (used for quantization)
   *  @param jpls_t2                   JPEG-LS parameter "Threshold 2"
   *  @param jpls_t3                   JPEG-LS parameter "Threshold 3"
   *  @param jpls_reset                JPEG-LS parameter "RESET", i.e., value at which the counters A, B, and N are halved.
   *  @param preferCookedEncoding      true if the "cooked" lossless encoder should be preferred over the "raw" one
   *  @param fragmentSize              maximum fragment size (in kbytes) for compression, 0 for unlimited.
   *  @param createOffsetTable         create offset table during image compression
   *  @param uidCreation               mode for SOP Instance UID creation
   *  @param convertToSC               flag indicating whether image should be converted to Secondary Capture upon compression
   *  @param jplsInterleaveMode        flag describing which interleave the JPEG-LS datastream should use
   *  @param useFFbitstreamPadding     flag indicating whether the JPEG-LS bitstream should be FF padded as required by DICOM.
   */
  static void registerCodecs(
    Uint16 jpls_t1 = 0,
    Uint16 jpls_t2 = 0,
    Uint16 jpls_t3 = 0,
    Uint16 jpls_reset = 0,
    OFBool preferCookedEncoding = OFTrue,
    Uint32 fragmentSize = 0,
    OFBool createOffsetTable = OFTrue,
    J2K_UIDCreation uidCreation = EJ2KUC_default,
    OFBool convertToSC = OFFalse,
    DJ2KCodecParameter::interleaveMode jplsInterleaveMode = DJ2KCodecParameter::interleaveDefault,
    OFBool useFFbitstreamPadding = OFTrue );

  /** deregisters encoders.
   *  Attention: Must not be called while other threads might still use
   *  the registered codecs, e.g. because they are currently encoding
   *  DICOM data sets through dcmdata.
   */
  static void cleanup();

  /** get version information of the CharLS library.
   *  Typical output format: "CharLS, Revision 55020 (modified)"
   *  @return name and version number of the CharLS library
   */
  static OFString getLibraryVersionString();

private:

  /// flag indicating whether the encoders are already registered.
  static OFBool registered_;

  /// pointer to codec parameter shared by all encoders
  static DJ2KCodecParameter *cp_;

  /// pointer to encoder for lossless JPEG-LS
  static DJ2KLosslessEncoder  *losslessencoder_;

  /// pointer to encoder for lossy JPEG-LS
  static DJ2KNearLosslessEncoder *nearlosslessencoder_;

};

#endif
