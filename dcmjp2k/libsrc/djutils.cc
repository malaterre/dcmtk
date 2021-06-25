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
 *  Author:  Martin Willkomm, Uli Schlachter
 *
 *  Purpose: Support code for dcmjp2k
 *
 */

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2k/dj2kutil.h"
#include "dcmtk/dcmdata/dcerror.h"

OFLogger DCM_dcmjp2kLogger = OFLog::getLogger("dcmtk.dcmjp2k");

#define MAKE_DCMJ2K_ERROR(number, name, description)  \
makeOFConditionConst(EC_ ## name, OFM_dcmjp2k, number, OF_error, description)

MAKE_DCMJ2K_ERROR( 1, J2KUncompressedBufferTooSmall, "Uncompressed pixel data too short for uncompressed image");
MAKE_DCMJ2K_ERROR( 2, J2KCompressedBufferTooSmall, "Allocated too small buffer for compressed image data");
MAKE_DCMJ2K_ERROR( 3, J2KCodecUnsupportedImageType, "Codec does not support this JPEG-LS image");
MAKE_DCMJ2K_ERROR( 4, J2KCodecInvalidParameters, "Codec received invalid compression parameters");
MAKE_DCMJ2K_ERROR( 5, J2KCodecUnsupportedValue, "Codec received unsupported compression parameters");
MAKE_DCMJ2K_ERROR( 6, J2KInvalidCompressedData, "Invalid compressed image data");
MAKE_DCMJ2K_ERROR( 7, J2KUnsupportedBitDepthForTransform, "Codec does not support the image's color transformation with this bit depth");
MAKE_DCMJ2K_ERROR( 8, J2KUnsupportedColorTransform, "Codec does not support the image's color transformation");
MAKE_DCMJ2K_ERROR( 9, J2KUnsupportedBitDepth, "Unsupported bit depth in JPEG-LS transfer syntax");
MAKE_DCMJ2K_ERROR(10, J2KCannotComputeNumberOfFragments, "Cannot compute number of fragments for JPEG-LS frame");
MAKE_DCMJ2K_ERROR(11, J2KImageDataMismatch, "Image data mismatch between DICOM header and JPEG-LS bitstream");
MAKE_DCMJ2K_ERROR(12, J2KUnsupportedPhotometricInterpretation, "Unsupported photometric interpretation for near-lossless JPEG-LS compression");
MAKE_DCMJ2K_ERROR(13, J2KUnsupportedPixelRepresentation, "Unsupported pixel representation for near-lossless JPEG-LS compression");
MAKE_DCMJ2K_ERROR(14, J2KUnsupportedImageType, "Unsupported type of image for JPEG-LS compression");
MAKE_DCMJ2K_ERROR(15, J2KTooMuchCompressedData, "Too much compressed data, trailing data after image");
