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
 *  Purpose: singleton class that registers the decoder for all supported JPEG-LS processes.
 *
 */

#ifndef DCMJ2K_DJDECODE_H
#define DCMJ2K_DJDECODE_H

#include "dcmtk/config/osconfig.h"
#include "dcmtk/ofstd/oftypes.h"      /* for OFBool */
#include "dcmtk/dcmjp2k/dj2kutil.h"  /* for enums */

class DJ2KCodecParameter;
class DJ2KLosslessDecoder;
class DJ2KNearLosslessDecoder;

/** singleton class that registers decoders for all supported JPEG-LS processes.
 */
class DCMTK_DCMJ2K_EXPORT DJ2KDecoderRegistration
{
public:

  /** registers decoder for all supported JPEG-LS processes.
   *  If already registered, call is ignored unless cleanup() has
   *  been performed before.
   *  @param uidcreation flag indicating whether or not
   *    a new SOP Instance UID should be assigned upon decompression.
   *  @param planarconfig flag indicating how planar configuration
   *    of color images should be encoded upon decompression.
   *  @param ignoreOffsetTable flag indicating whether to ignore the offset table when decompressing multiframe images
   *  @param forceSingleFragmentPerFrame while decompressing a multiframe image,
   *    assume one fragment per frame even if the JPEG data for some frame is incomplete
   */
  static void registerCodecs(
    J2K_UIDCreation uidcreation = EJ2KUC_default,
    J2K_PlanarConfiguration planarconfig = EJ2KPC_restore,
    OFBool ignoreOffsetTable = OFFalse,
    OFBool forceSingleFragmentPerFrame = OFFalse);

  /** deregisters decoders.
   *  Attention: Must not be called while other threads might still use
   *  the registered codecs, e.g. because they are currently decoding
   *  DICOM data sets through dcmdata.
   */
  static void cleanup();

  /** get version information of the CharLS library.
   *  Typical output format: "CharLS, Revision 55020 (modified)"
   *  @return name and version number of the CharLS library
   */
  static OFString getLibraryVersionString();

private:

  /// flag indicating whether the decoders are already registered.
  static OFBool registered_;

  /// pointer to codec parameter shared by all decoders
  static DJ2KCodecParameter *cp_;

  /// pointer to losslessdecoder
  static DJ2KLosslessDecoder *losslessdecoder_;

  /// pointer to near-lossless encoder
  static DJ2KNearLosslessDecoder *nearlosslessdecoder_;

};

#endif
