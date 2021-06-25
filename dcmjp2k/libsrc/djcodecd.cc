/*
 *
 *  Copyright (C) 2007-2020, OFFIS e.V.
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
 *  Author:  Martin Willkomm, Marco Eichelberg, Uli Schlachter
 *
 *  Purpose: codec classes for JPEG-LS decoders.
 *
 */
/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "dcmtk/config/osconfig.h"
#include "dcmtk/dcmjp2k/djcodecd.h"

#include "dcmtk/ofstd/ofstream.h"    /* for ofstream */
#include "dcmtk/ofstd/ofcast.h"      /* for casts */
#include "dcmtk/ofstd/offile.h"      /* for class OFFile */
#include "dcmtk/ofstd/ofstd.h"       /* for class OFStandard */
#include "dcmtk/dcmdata/dcdatset.h"  /* for class DcmDataset */
#include "dcmtk/dcmdata/dcdeftag.h"  /* for tag constants */
#include "dcmtk/dcmdata/dcpixseq.h"  /* for class DcmPixelSequence */
#include "dcmtk/dcmdata/dcpxitem.h"  /* for class DcmPixelItem */
#include "dcmtk/dcmdata/dcvrpobw.h"  /* for class DcmPolymorphOBOW */
#include "dcmtk/dcmdata/dcswap.h"    /* for swapIfNecessary() */
#include "dcmtk/dcmdata/dcuid.h"     /* for dcmGenerateUniqueIdentifer()*/
#include "dcmtk/dcmjp2k/djcparam.h"  /* for class DJ2KCodecParameter */
#include "djerror.h"                 /* for private class DJ2KError */

// JPEG-2000 library (OpenJPEG) includes
#include "openjpeg.h"

E_TransferSyntax DJ2KLosslessDecoder::supportedTransferSyntax() const
{
  return EXS_JPEG2000LosslessOnly;
}

E_TransferSyntax DJ2KNearLosslessDecoder::supportedTransferSyntax() const
{
  return EXS_JPEG2000;
}

// --------------------------------------------------------------------------

DJ2KDecoderBase::DJ2KDecoderBase()
: DcmCodec()
{
}


DJ2KDecoderBase::~DJ2KDecoderBase()
{
}


OFBool DJ2KDecoderBase::canChangeCoding(
    const E_TransferSyntax oldRepType,
    const E_TransferSyntax newRepType) const
{
  // this codec only handles conversion from JPEG-2000 to uncompressed.

  DcmXfer newRep(newRepType);
  if (newRep.isNotEncapsulated() && (oldRepType == supportedTransferSyntax()))
     return OFTrue;

  return OFFalse;
}


OFCondition DJ2KDecoderBase::decode(
    const DcmRepresentationParameter * /* fromRepParam */,
    DcmPixelSequence * pixSeq,
    DcmPolymorphOBOW& uncompressedPixelData,
    const DcmCodecParameter * cp,
    const DcmStack& objStack,
    OFBool& removeOldRep) const
{

  // this codec may modify the DICOM header such that the previous pixel
  // representation is not valid anymore, e.g. in the case of color images
  // where the planar configuration can change. Indicate this to the caller
  // to trigger removal.
  removeOldRep = OFTrue;

  // retrieve pointer to dataset from parameter stack
  DcmStack localStack(objStack);
  (void)localStack.pop();  // pop pixel data element from stack
  DcmObject *dobject = localStack.pop(); // this is the item in which the pixel data is located
  if ((!dobject)||((dobject->ident()!= EVR_dataset) && (dobject->ident()!= EVR_item))) return EC_InvalidTag;
  DcmItem *dataset = OFstatic_cast(DcmItem *, dobject);
  OFBool numberOfFramesPresent = OFFalse;

  // determine properties of uncompressed dataset
  Uint16 imagePixelRepresentation = 0;
  if (dataset->findAndGetUint16(DCM_PixelRepresentation, imagePixelRepresentation).bad()) return EC_TagNotFound;

  // we only handle signed or unsigned 
  if ((imagePixelRepresentation != 0) && (imagePixelRepresentation != 1)) return EC_InvalidTag;

  Uint16 imageSamplesPerPixel = 0;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, imageSamplesPerPixel).bad()) return EC_TagNotFound;

  // we only handle one or three samples per pixel
  if ((imageSamplesPerPixel != 3) && (imageSamplesPerPixel != 1)) return EC_InvalidTag;

  Uint16 imageRows = 0;
  if (dataset->findAndGetUint16(DCM_Rows, imageRows).bad()) return EC_TagNotFound;
  if (imageRows < 1) return EC_InvalidTag;

  Uint16 imageColumns = 0;
  if (dataset->findAndGetUint16(DCM_Columns, imageColumns).bad()) return EC_TagNotFound;
  if (imageColumns < 1) return EC_InvalidTag;

  // number of frames is an optional attribute - we don't mind if it isn't present.
  Sint32 imageFrames = 0;
  if (dataset->findAndGetSint32(DCM_NumberOfFrames, imageFrames).good()) numberOfFramesPresent = OFTrue;

  if (imageFrames >= OFstatic_cast(Sint32, pixSeq->card()))
    imageFrames = pixSeq->card() - 1; // limit number of frames to number of pixel items - 1
  if (imageFrames < 1)
    imageFrames = 1; // default in case the number of frames attribute is absent or contains garbage

  Uint16 imageBitsStored = 0;
  if (dataset->findAndGetUint16(DCM_BitsStored, imageBitsStored).bad()) return EC_TagNotFound;

  Uint16 imageBitsAllocated = 0;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, imageBitsAllocated).bad()) return EC_TagNotFound;

  Uint16 imageHighBit = 0;
  if (dataset->findAndGetUint16(DCM_HighBit, imageHighBit).bad()) return EC_TagNotFound;

  //we only support up to 16 bits per sample
  if ((imageBitsStored < 1) || (imageBitsStored > 16)) return EC_J2KUnsupportedBitDepth;

  // determine the number of bytes per sample (bits allocated) for the de-compressed object.
  Uint16 bytesPerSample = 1;
  if (imageBitsStored > 8) bytesPerSample = 2;
  else if (imageBitsAllocated > 8) bytesPerSample = 2;

  // compute size of uncompressed frame, in bytes
  Uint32 frameSize = bytesPerSample * imageRows * imageColumns * imageSamplesPerPixel;

  // compute size of pixel data attribute, in bytes
  Uint32 totalSize = frameSize * imageFrames;
  if (totalSize & 1) totalSize++; // align on 16-bit word boundary

  // assume we can cast the codec parameter to what we need
  const DJ2KCodecParameter *djcp = OFreinterpret_cast(const DJ2KCodecParameter *, cp);

  // determine planar configuration for uncompressed data
  OFString imageSopClass;
  OFString imagePhotometricInterpretation;
  dataset->findAndGetOFString(DCM_SOPClassUID, imageSopClass);
  dataset->findAndGetOFString(DCM_PhotometricInterpretation, imagePhotometricInterpretation);

  // allocate space for uncompressed pixel data element
  Uint16 *pixeldata16 = NULL;
  OFCondition result = uncompressedPixelData.createUint16Array(totalSize/sizeof(Uint16), pixeldata16);
  if (result.bad()) return result;

  Uint8 *pixeldata8 = OFreinterpret_cast(Uint8 *, pixeldata16);
  Sint32 currentFrame = 0;
  Uint32 currentItem = 1; // item 0 contains the offset table
  OFBool done = OFFalse;
  OFBool forceSingleFragmentPerFrame = djcp->getForceSingleFragmentPerFrame();

  OFBool isLossless = true;
  while (result.good() && !done)
  {
      DCMJ2K_DEBUG("JPEG-2000 decoder processes frame " << (currentFrame+1));

      OFBool isFrameLossless = true;
      result = decodeFrame(pixSeq, djcp, dataset, currentFrame, currentItem, pixeldata8, frameSize,
          imageFrames, imageColumns, imageRows, imageSamplesPerPixel, imagePixelRepresentation, bytesPerSample, isFrameLossless);
      isLossless = isLossless && isFrameLossless;

      // check if we should enforce "one fragment per frame" while
      // decompressing a multi-frame image even if stream suspension occurs
      if ((result == EC_J2KInvalidCompressedData) && forceSingleFragmentPerFrame)
      {
        // frame is incomplete. Nevertheless skip to next frame.
        // This permits decompression of faulty multi-frame images.
        DCMJ2K_WARN("JPEG-2000 bitstream invalid or incomplete, ignoring (but image is likely to be incomplete).");
        result = EC_Normal;
      }

      if (result.good())
      {
        // increment frame number, check if we're finished
        if (++currentFrame == imageFrames) done = OFTrue;
        pixeldata8 += frameSize;
      }
  }

  // Number of Frames might have changed in case the previous value was wrong
  if (result.good() && (numberOfFramesPresent || (imageFrames > 1)))
  {
    char numBuf[20];
    sprintf(numBuf, "%ld", OFstatic_cast(long, imageFrames));
    result = ((DcmItem *)dataset)->putAndInsertString(DCM_NumberOfFrames, numBuf);
  }

  if (result.good() && (dataset->ident() == EVR_dataset))
  {
    DcmItem *ditem = OFreinterpret_cast(DcmItem*, dataset);

    // the following operations do not affect the Image Pixel Module
    // but other modules such as SOP Common.  We only perform these
    // changes if we're on the main level of the dataset,
    // which should always identify itself as dataset, not as item.
    if (djcp->getUIDCreation() == EJ2KUC_always)
    {
        // create new SOP instance UID
        result = DcmCodec::newInstance(ditem, NULL, NULL, NULL);
    }

    // set Lossy Image Compression to "01" (see DICOM part 3, C.7.6.1.1.5)
    if (result.good() && (supportedTransferSyntax() == EXS_JPEG2000) && !isLossless) result = ditem->putAndInsertString(DCM_LossyImageCompression, "01");

  }

  return result;
}


OFCondition DJ2KDecoderBase::decodeFrame(
    const DcmRepresentationParameter * /* fromParam */,
    DcmPixelSequence *fromPixSeq,
    const DcmCodecParameter *cp,
    DcmItem *dataset,
    Uint32 frameNo,
    Uint32& currentItem,
    void * buffer,
    Uint32 bufSize,
    OFString& decompressedColorModel,
    OFBool& isFrameLossless) const
{
  OFCondition result = EC_Normal;

  // assume we can cast the codec parameter to what we need
  const DJ2KCodecParameter *djcp = OFreinterpret_cast(const DJ2KCodecParameter *, cp);

  // determine properties of uncompressed dataset
  Uint16 imagePixelRepresentation = 0;
  if (dataset->findAndGetUint16(DCM_PixelRepresentation, imagePixelRepresentation).bad()) return EC_TagNotFound;
  // we only handle signed or unsigned 
  if ((imagePixelRepresentation != 0) && (imagePixelRepresentation != 1)) return EC_InvalidTag;

  Uint16 imageSamplesPerPixel = 0;
  if (dataset->findAndGetUint16(DCM_SamplesPerPixel, imageSamplesPerPixel).bad()) return EC_TagNotFound;
  // we only handle one or three samples per pixel
  if ((imageSamplesPerPixel != 3) && (imageSamplesPerPixel != 1)) return EC_InvalidTag;

  Uint16 imageRows = 0;
  if (dataset->findAndGetUint16(DCM_Rows, imageRows).bad()) return EC_TagNotFound;
  if (imageRows < 1) return EC_InvalidTag;

  Uint16 imageColumns = 0;
  if (dataset->findAndGetUint16(DCM_Columns, imageColumns).bad()) return EC_TagNotFound;
  if (imageColumns < 1) return EC_InvalidTag;

  Uint16 imageBitsStored = 0;
  if (dataset->findAndGetUint16(DCM_BitsStored, imageBitsStored).bad()) return EC_TagNotFound;

  Uint16 imageBitsAllocated = 0;
  if (dataset->findAndGetUint16(DCM_BitsAllocated, imageBitsAllocated).bad()) return EC_TagNotFound;

  //we only support up to 16 bits per sample
  if ((imageBitsStored < 1) || (imageBitsStored > 16)) return EC_J2KUnsupportedBitDepth;

  // determine the number of bytes per sample (bits allocated) for the de-compressed object.
  Uint16 bytesPerSample = 1;
  if (imageBitsStored > 8) bytesPerSample = 2;
  else if (imageBitsAllocated > 8) bytesPerSample = 2;

  // number of frames is an optional attribute - we don't mind if it isn't present.
  Sint32 imageFrames = 0;
  dataset->findAndGetSint32(DCM_NumberOfFrames, imageFrames).good();

  if (imageFrames >= OFstatic_cast(Sint32, fromPixSeq->card()))
    imageFrames = fromPixSeq->card() - 1; // limit number of frames to number of pixel items - 1
  if (imageFrames < 1)
    imageFrames = 1; // default in case the number of frames attribute is absent or contains garbage

  // if the user has provided this information, we trust him.
  // If the user has passed a zero, try to find out ourselves.
  if (currentItem == 0)
  {
    result = determineStartFragment(frameNo, imageFrames, fromPixSeq, currentItem);
  }

  if (result.good())
  {
    // We got all the data we need from the dataset, let's start decoding
    DCMJ2K_DEBUG("Starting to decode frame " << frameNo << " with fragment " << currentItem);
    result = decodeFrame(fromPixSeq, djcp, dataset, frameNo, currentItem, buffer, bufSize,
        imageFrames, imageColumns, imageRows, imageSamplesPerPixel, imagePixelRepresentation, bytesPerSample, isFrameLossless);
  }

  if (result.good())
  {
    // retrieve color model from given dataset
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation, decompressedColorModel);
  }

  return result;
}

struct myfile
{
  char *mem;
  char *cur;
  size_t len;
};
void gdcm_error_callback(const char* msg, void* )
{
  fprintf( stderr, "%s", msg );
}
OPJ_SIZE_T opj_read_from_memory(void * p_buffer, OPJ_SIZE_T p_nb_bytes, myfile* p_file)
{
  //OPJ_UINT32 l_nb_read = fread(p_buffer,1,p_nb_bytes,p_file);
  OPJ_SIZE_T l_nb_read;
  if( p_file->cur + p_nb_bytes <= p_file->mem + p_file->len )
    {
    l_nb_read = 1*p_nb_bytes;
    }
  else
    {
    l_nb_read = (OPJ_SIZE_T)(p_file->mem + p_file->len - p_file->cur);
    assert( l_nb_read < p_nb_bytes );
    }
  memcpy(p_buffer,p_file->cur,l_nb_read);
  p_file->cur += l_nb_read;
  assert( p_file->cur <= p_file->mem + p_file->len );
  //std::cout << "l_nb_read: " << l_nb_read << std::endl;
  return l_nb_read ? l_nb_read : ((OPJ_SIZE_T)-1);
}
OPJ_SIZE_T opj_write_from_memory (void * p_buffer, OPJ_SIZE_T p_nb_bytes, myfile* p_file)
{
  //return fwrite(p_buffer,1,p_nb_bytes,p_file);
  OPJ_SIZE_T l_nb_write;
  //if( p_file->cur + p_nb_bytes < p_file->mem + p_file->len )
  //  {
  l_nb_write = 1*p_nb_bytes;
  //  }
  //else
  //  {
  //  l_nb_write = p_file->mem + p_file->len - p_file->cur;
  //  assert( l_nb_write < p_nb_bytes );
  //  }
  memcpy(p_file->cur,p_buffer,l_nb_write);
  p_file->cur += l_nb_write;
  p_file->len += l_nb_write;
  //assert( p_file->cur < p_file->mem + p_file->len );
  return l_nb_write;
  //return p_nb_bytes;
}
OPJ_OFF_T opj_skip_from_memory (OPJ_OFF_T p_nb_bytes, myfile * p_file)
{
  //if (fseek(p_user_data,p_nb_bytes,SEEK_CUR))
  //  {
  //  return -1;
  //  }
  if( p_file->cur + p_nb_bytes <= p_file->mem + p_file->len )
    {
    p_file->cur += p_nb_bytes;
    return p_nb_bytes;
    }

  p_file->cur = p_file->mem + p_file->len;
  return -1;
}

OPJ_BOOL opj_seek_from_memory (OPJ_OFF_T p_nb_bytes, myfile * p_file)
{
  //if (fseek(p_user_data,p_nb_bytes,SEEK_SET))
  //  {
  //  return false;
  //  }
  //return true;
  assert( p_nb_bytes >= 0 );
  if( (size_t)p_nb_bytes <= p_file->len )
    {
    p_file->cur = p_file->mem + p_nb_bytes;
    return OPJ_TRUE;
    }

  p_file->cur = p_file->mem + p_file->len;
  return OPJ_FALSE;
}
opj_stream_t* OPJ_CALLCONV opj_stream_create_memory_stream (myfile* p_mem,OPJ_SIZE_T p_size,bool p_is_read_stream)
{
  opj_stream_t* l_stream = nullptr;
  if
    (! p_mem)
  {
    return nullptr;
  }
  l_stream = opj_stream_create(p_size,p_is_read_stream);
  if
    (! l_stream)
  {
    return nullptr;
  }
  opj_stream_set_user_data(l_stream,p_mem,nullptr);
  opj_stream_set_read_function(l_stream,(opj_stream_read_fn) opj_read_from_memory);
  opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_memory);
  opj_stream_set_skip_function(l_stream, (opj_stream_skip_fn) opj_skip_from_memory);
  opj_stream_set_seek_function(l_stream, (opj_stream_seek_fn) opj_seek_from_memory);
  opj_stream_set_user_data_length(l_stream, p_mem->len /* p_size*/); /* important to avoid an assert() */
  return l_stream;
}

static inline bool check_comp_valid(opj_image_t *image)
{
    int compno = 0;
    opj_image_comp_t *comp = &image->comps[compno];
    if (comp->prec > 32) // I doubt openjpeg will reach here.
        return false;

    bool invalid = false;
    if (image->numcomps == 3)
    {
        opj_image_comp_t *comp1 = &image->comps[1];
        opj_image_comp_t *comp2 = &image->comps[2];
        if (comp->prec != comp1->prec) invalid = true;
        if (comp->prec != comp2->prec) invalid = true;
        if (comp->sgnd != comp1->sgnd) invalid = true;
        if (comp->sgnd != comp2->sgnd) invalid = true;
        if (comp->h != comp1->h) invalid = true;
        if (comp->h != comp2->h) invalid = true;
        if (comp->w != comp1->w) invalid = true;
        if (comp->w != comp2->w) invalid = true;
    }
    return !invalid;
}
inline int int_ceildivpow2(int a, int b) {
  return (a + (1 << b) - 1) >> b;
}

OFCondition DJ2KDecoderBase::decodeFrame(
    DcmPixelSequence * fromPixSeq,
    const DJ2KCodecParameter *cp,
    DcmItem *dataset,
    Uint32 frameNo,
    Uint32& currentItem,
    void * buffer,
    Uint32 bufSize,
    Sint32 imageFrames,
    Uint16 imageColumns,
    Uint16 imageRows,
    Uint16 imageSamplesPerPixel,
    Uint16 imagePixelRepresentation,
    Uint16 bytesPerSample,
    OFBool& isFrameLossless)
{
  DcmPixelItem *pixItem = NULL;
  Uint8 * j2kData = NULL;
  Uint8 * j2kFragmentData = NULL;
  Uint32 fragmentLength = 0;
  size_t compressedSize = 0;
  Uint32 fragmentsForThisFrame = 0;
  OFCondition result = EC_Normal;
  OFBool ignoreOffsetTable = cp->ignoreOffsetTable();

  // compute the number of JPEG-2000 fragments we need in order to decode the next frame
  fragmentsForThisFrame = computeNumberOfFragments(imageFrames, frameNo, currentItem, ignoreOffsetTable, fromPixSeq);
  if (fragmentsForThisFrame == 0) result = EC_J2KCannotComputeNumberOfFragments;

  // determine planar configuration for uncompressed data
  OFString imageSopClass;
  OFString imagePhotometricInterpretation;
  dataset->findAndGetOFString(DCM_SOPClassUID, imageSopClass);
  dataset->findAndGetOFString(DCM_PhotometricInterpretation, imagePhotometricInterpretation);
  Uint16 imagePlanarConfiguration = 0; // 0 is color-by-pixel, 1 is color-by-plane

  if (imageSamplesPerPixel > 1)
  {
    switch (cp->getPlanarConfiguration())
    {
      case EJ2KPC_restore:
        // get planar configuration from dataset
        imagePlanarConfiguration = 2; // invalid value
        dataset->findAndGetUint16(DCM_PlanarConfiguration, imagePlanarConfiguration);
        // determine auto default if not found or invalid
        if (imagePlanarConfiguration > 1)
          imagePlanarConfiguration = determinePlanarConfiguration(imageSopClass, imagePhotometricInterpretation);
        break;
      case EJ2KPC_auto:
        imagePlanarConfiguration = determinePlanarConfiguration(imageSopClass, imagePhotometricInterpretation);
        break;
      case EJ2KPC_colorByPixel:
        imagePlanarConfiguration = 0;
        break;
      case EJ2KPC_colorByPlane:
        imagePlanarConfiguration = 1;
        break;
    }
  }

  // get the size of all the fragments
  if (result.good())
  {
    // Don't modify the original values for now
    Uint32 fragmentsForThisFrame2 = fragmentsForThisFrame;
    Uint32 currentItem2 = currentItem;

    while (result.good() && fragmentsForThisFrame2--)
    {
      result = fromPixSeq->getItem(pixItem, currentItem2++);
      if (result.good() && pixItem)
      {
        fragmentLength = pixItem->getLength();
        if (result.good())
          compressedSize += fragmentLength;
      }
    } /* while */
  }

  // get the compressed data
  if (result.good())
  {
    Uint32 offset = 0;
    j2kData = new Uint8[compressedSize];

    while (result.good() && fragmentsForThisFrame--)
    {
      result = fromPixSeq->getItem(pixItem, currentItem++);
      if (result.good() && pixItem)
      {
        fragmentLength = pixItem->getLength();
        result = pixItem->getUint8Array(j2kFragmentData);
        if (result.good() && j2kFragmentData)
        {
          memcpy(&j2kData[offset], j2kFragmentData, fragmentLength);
          offset += fragmentLength;
        }
      }
    } /* while */
  }

  if (result.good())
  {
#if 1
    // Lifted from GDCM with permission :)

    // JlsParameters params;
    // J2K_ERROR err;
  while( compressedSize > 0 && j2kData[compressedSize-1] != 0xd9 )
    {
    compressedSize--;
    }
  myfile mysrc;
  myfile *fsrc = &mysrc;
  fsrc->mem = fsrc->cur = (char*)j2kData;
  fsrc->len = compressedSize;

  opj_stream_t *cio = nullptr;
  cio = opj_stream_create_memory_stream(fsrc,OPJ_J2K_STREAM_CHUNK_SIZE, true);
    if (!cio) return EC_MemoryExhausted;

    OPJ_CODEC_FORMAT format = OPJ_CODEC_J2K;
    // https://www.iana.org/assignments/media-types/image/jp2
#define JP2_MAGIC "\x00\x00\x00\x0c\x6a\x50\x20\x20\x0d\x0a\x87\x0a"
    if(memcmp(j2kData, JP2_MAGIC, 12) == 0 )
      format = OPJ_CODEC_JP2;  

    // err = JpegLsReadHeader(j2kData, compressedSize, &params);
    // result = DJ2KError::convert(err);
   opj_codec_t* dinfo = nullptr;  /* handle to a decompressor */
  dinfo = opj_create_decompress(format);

opj_codec_set_threads(dinfo, 1);

  //opj_set_info_handler(dinfo, gdcm_error_callback, NULL);
  //opj_set_warning_handler(dinfo, gdcm_error_callback, NULL);
  opj_set_error_handler(dinfo, gdcm_error_callback, NULL);

  opj_dparameters_t parameters;  /* decompression parameters */
  opj_set_default_decoder_parameters(&parameters);

  if(!opj_setup_decoder(dinfo, &parameters))
  {    
    opj_destroy_codec(dinfo);
    opj_stream_destroy(cio);
    result = EC_CorruptedData;
  }

opj_image_t *image = nullptr;
  if(result.good() && !opj_read_header(cio, dinfo, &image))
  {
    opj_destroy_codec(dinfo);
    opj_stream_destroy(cio);
    opj_image_destroy(image);
    result = EC_CorruptedData;
  }

    if (result.good())
    {
      int compno = 0;
      opj_image_comp_t *comp = &image->comps[compno];
      if (image->x1 != imageColumns) result = EC_J2KImageDataMismatch;
      else if (image->y1 != imageRows) result = EC_J2KImageDataMismatch;
      else if (image->numcomps != imageSamplesPerPixel) result = EC_J2KImageDataMismatch;
      else if (comp->sgnd != imagePixelRepresentation) result = EC_J2KImageDataMismatch;
      else if ((bytesPerSample == 1) && (comp->prec > 8)) result = EC_J2KImageDataMismatch;
      else if ((bytesPerSample == 2) && (comp->prec <= 8)) result = EC_J2KImageDataMismatch;
      else if (!check_comp_valid(image)) result = EC_J2KImageDataMismatch;

      // This information is available through the use of opj_get_cstr_info().
      // Admitedly this is just for the default coding parameters, not
      // per-tile. But deciding the filter per-tile is rather unusual
      // ref: https://github.com/uclouvain/openjpeg/issues/3#issuecomment-326727051
      opj_codestream_info_v2_t * d = opj_get_cstr_info(dinfo);
      opj_tile_info_v2_t *default_tile_info= &d->m_default_tile_info;
      OPJ_UINT32 mct = default_tile_info->mct;
      opj_tccp_info_t *tccp_info = default_tile_info->tccp_info;
      // discrete wavelet transform identifier: 0 = 9-7 irreversible, 1 = 5-3 reversible 
      OPJ_UINT32 qmfbid = tccp_info->qmfbid;
      // FIXME do something with mct/qmfbid
      isFrameLossless = qmfbid == 1;
      opj_destroy_cstr_info(&d);
    }

    if (!result.good())
    {
      delete[] j2kData;
    }
    else
    {

    if (!(opj_decode(dinfo, cio, image) && opj_end_decompress(dinfo,  cio))) {        
    opj_destroy_codec(dinfo); 
    opj_stream_destroy(cio); 
    opj_image_destroy(image);
    result = EC_CorruptedData;
    }
    //  err = JpegLsDecode(buffer, bufSize, j2kData, compressedSize, &params);
    //  result = DJ2KError::convert(err);
      delete[] j2kData;

opj_destroy_codec(dinfo);
    opj_stream_destroy(cio); 

    if(result.good())
{
  char * raw= OFreinterpret_cast(char*, buffer);
  for (unsigned int compno = 0; compno < (unsigned int)image->numcomps; compno++)
    {
    opj_image_comp_t *comp = &image->comps[compno];
    int w = image->comps[compno].w;
    int wr = int_ceildivpow2(image->comps[compno].w, image->comps[compno].factor);

    //int h = image.comps[compno].h;
    int hr = int_ceildivpow2(image->comps[compno].h, image->comps[compno].factor);
    //assert(  wr * hr * 1 * image->numcomps * (comp->prec/8) == len );
    assert(  w == imageColumns);
    assert(  wr == imageColumns);
    assert(  hr == imageRows );

    if (comp->prec <= 8)
      {
      uint8_t *data8 = (uint8_t*)raw + compno;
      for (int i = 0; i < wr * hr; i++)
        {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data8 = (uint8_t)v;
        data8 += image->numcomps;
        }
      }
    else if (comp->prec <= 16)
      {
      // ELSCINT1_JP2vsJ2K.dcm is a 12bits image
      uint16_t *data16 = (uint16_t*)(void*)raw + compno;
      for (int i = 0; i < wr * hr; i++)
        {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data16 = (uint16_t)v;
        data16 += image->numcomps;
        }
      }
    else
      {
      uint32_t *data32 = (uint32_t*)(void*)raw + compno;
      for (int i = 0; i < wr * hr; i++)
        {
        int v = image->comps[compno].data[i / wr * w + i % wr];
        *data32 = (uint32_t)v;
        data32 += image->numcomps;
        }
      }
    }
}
    opj_image_destroy(image);

      if (result.good() && imageSamplesPerPixel == 3)
      {
#if 0
        if (imagePlanarConfiguration == 1 && params.ilv != ILV_NONE)
        {
          // The dataset says this should be planarConfiguration == 1, but
          // it isn't -> convert it.
          DCMJ2K_WARN("different planar configuration in JPEG stream, converting to \"1\"");
          if (bytesPerSample == 1)
            result = createPlanarConfiguration1Byte(OFreinterpret_cast(Uint8*, buffer), imageColumns, imageRows);
          else
            result = createPlanarConfiguration1Word(OFreinterpret_cast(Uint16*, buffer), imageColumns, imageRows);
        }
        else if (imagePlanarConfiguration == 0 && params.ilv != ILV_SAMPLE && params.ilv != ILV_LINE)
        {
          // The dataset says this should be planarConfiguration == 0, but
          // it isn't -> convert it.
          DCMJ2K_WARN("different planar configuration in JPEG stream, converting to \"0\"");
          if (bytesPerSample == 1)
            result = createPlanarConfiguration0Byte(OFreinterpret_cast(Uint8*, buffer), imageColumns, imageRows);
          else
            result = createPlanarConfiguration0Word(OFreinterpret_cast(Uint16*, buffer), imageColumns, imageRows);
        }
#endif
      }

      if (result.good())
      {
          // decompression is complete, finally adjust byte order if necessary
          if (bytesPerSample == 1) // we're writing bytes into words
          {
              result = swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, buffer,
                      bufSize, sizeof(Uint16));
          }
      }

      // update planar configuration if we are decoding a color image
      if (result.good() && (imageSamplesPerPixel > 1))
      {
        dataset->putAndInsertUint16(DCM_PlanarConfiguration, imagePlanarConfiguration);
      }
    }
#else
  assert(0);
#endif
  }

  return result;
}


OFCondition DJ2KDecoderBase::encode(
    const Uint16 * /* pixelData */,
    const Uint32 /* length */,
    const DcmRepresentationParameter * /* toRepParam */,
    DcmPixelSequence * & /* pixSeq */,
    const DcmCodecParameter * /* cp */,
    DcmStack & /* objStack */,
    OFBool& /* removeOldRep */) const
{
  // we are a decoder only
  return EC_IllegalCall;
}


OFCondition DJ2KDecoderBase::encode(
    const E_TransferSyntax /* fromRepType */,
    const DcmRepresentationParameter * /* fromRepParam */,
    DcmPixelSequence * /* fromPixSeq */,
    const DcmRepresentationParameter * /* toRepParam */,
    DcmPixelSequence * & /* toPixSeq */,
    const DcmCodecParameter * /* cp */,
    DcmStack & /* objStack */,
    OFBool& /* removeOldRep */) const
{
  // we don't support re-coding for now.
  return EC_IllegalCall;
}


OFCondition DJ2KDecoderBase::determineDecompressedColorModel(
    const DcmRepresentationParameter * /* fromParam */,
    DcmPixelSequence * /* fromPixSeq */,
    const DcmCodecParameter * /* cp */,
    DcmItem * dataset,
    OFString & decompressedColorModel) const
{
  OFCondition result = EC_IllegalParameter;
  if (dataset != NULL)
  {
    // retrieve color model from given dataset
    result = dataset->findAndGetOFString(DCM_PhotometricInterpretation, decompressedColorModel);
    if (result == EC_TagNotFound)
    {
        DCMJ2K_WARN("mandatory element PhotometricInterpretation " << DCM_PhotometricInterpretation << " is missing");
        result = EC_MissingAttribute;
    }
    else if (result.bad())
    {
        DCMJ2K_WARN("cannot retrieve value of element PhotometricInterpretation " << DCM_PhotometricInterpretation << ": " << result.text());
    }
    else if (decompressedColorModel.empty())
    {
        DCMJ2K_WARN("no value for mandatory element PhotometricInterpretation " << DCM_PhotometricInterpretation);
        result = EC_MissingValue;
    }
  }
  return result;
}


Uint16 DJ2KDecoderBase::determinePlanarConfiguration(
  const OFString& sopClassUID,
  const OFString& photometricInterpretation)
{
  // Hardcopy Color Image always requires color-by-plane
  if (sopClassUID == UID_RETIRED_HardcopyColorImageStorage) return 1;

  // The 1996 Ultrasound Image IODs require color-by-plane if color model is YBR_FULL.
  if (photometricInterpretation == "YBR_FULL")
  {
    if ((sopClassUID == UID_UltrasoundMultiframeImageStorage)
       ||(sopClassUID == UID_UltrasoundImageStorage)) return 1;
  }

  // default for all other cases
  return 0;
}

Uint32 DJ2KDecoderBase::computeNumberOfFragments(
  Sint32 numberOfFrames,
  Uint32 currentFrame,
  Uint32 startItem,
  OFBool ignoreOffsetTable,
  DcmPixelSequence * pixSeq)
{

  unsigned long numItems = pixSeq->card();
  DcmPixelItem *pixItem = NULL;

  // We first check the simple cases, that is, a single-frame image,
  // the last frame of a multi-frame image and the standard case where we do have
  // a single fragment per frame.
  if ((numberOfFrames <= 1) || (currentFrame + 1 == OFstatic_cast(Uint32, numberOfFrames)))
  {
    // single-frame image or last frame. All remaining fragments belong to this frame
    return (numItems - startItem);
  }
  if (OFstatic_cast(Uint32, numberOfFrames + 1) == numItems)
  {
    // multi-frame image with one fragment per frame
    return 1;
  }

  OFCondition result = EC_Normal;
  if (! ignoreOffsetTable)
  {
    // We do have a multi-frame image with multiple fragments per frame, and we are
    // not working on the last frame. Let's check the offset table if present.
    result = pixSeq->getItem(pixItem, 0);
    if (result.good() && pixItem)
    {
      Uint32 offsetTableLength = pixItem->getLength();
      if (offsetTableLength == (OFstatic_cast(Uint32, numberOfFrames) * 4))
      {
        // offset table is non-empty and contains one entry per frame
        Uint8 *offsetData = NULL;
        result = pixItem->getUint8Array(offsetData);
        if (result.good() && offsetData)
        {
          // now we can access the offset table
          Uint32 *offsetData32 = OFreinterpret_cast(Uint32 *, offsetData);

          // extract the offset for the NEXT frame. This offset is guaranteed to exist
          // because the "last frame/single frame" case is handled above.
          Uint32 offset = offsetData32[currentFrame+1];

          // convert to local endian byte order (always little endian in file)
          swapIfNecessary(gLocalByteOrder, EBO_LittleEndian, &offset, sizeof(Uint32), sizeof(Uint32));

          // determine index of start fragment for next frame
          Uint32 byteCount = 0;
          Uint32 fragmentIndex = 1;
          while ((byteCount < offset) && (fragmentIndex < numItems))
          {
             pixItem = NULL;
             result = pixSeq->getItem(pixItem, fragmentIndex++);
             if (result.good() && pixItem)
             {
               byteCount += pixItem->getLength() + 8; // add 8 bytes for item tag and length
               if ((byteCount == offset) && (fragmentIndex > startItem))
               {
                 // bingo, we have found the offset for the next frame
                 return fragmentIndex - startItem;
               }
             }
             else break; /* something went wrong, break out of while loop */
          } /* while */
        }
      }
    }
  }

  // So we have a multi-frame image with multiple fragments per frame and the
  // offset table is empty or wrong. Our last chance is to peek into the JPEG-LS
  // bistream and identify the start of the next frame.
  Uint32 nextItem = startItem;
  Uint8 *fragmentData = NULL;
  while (++nextItem < numItems)
  {
    pixItem = NULL;
    result = pixSeq->getItem(pixItem, nextItem);
    if (result.good() && pixItem)
    {
        fragmentData = NULL;
        result = pixItem->getUint8Array(fragmentData);
        if (result.good() && fragmentData && (pixItem->getLength() > 3))
        {
          if (isJPEG2000StartOfImage(fragmentData))
          {
            // found a JPEG-2000 SOI marker. Assume that this is the start of the next frame.
            return (nextItem - startItem);
          }
        }
        else break; /* something went wrong, break out of while loop */
    }
    else break; /* something went wrong, break out of while loop */
  }

  // We're bust. No way to determine the number of fragments per frame.
  return 0;
}


OFBool DJ2KDecoderBase::isJPEG2000StartOfImage(Uint8 *fragmentData)
{
  // A valid JPEG-LS bitstream will always start with an SOI marker FFD8, followed
  // by either an SOF55 (FFF7), COM (FFFE) or APPn (FFE0-FFEF) marker.
  if ((*fragmentData++) != 0xFF) return OFFalse;
  if ((*fragmentData++) != 0xD8) return OFFalse;
  if ((*fragmentData++) != 0xFF) return OFFalse;
  if ((*fragmentData == 0xF7)||(*fragmentData == 0xFE)||((*fragmentData & 0xF0) == 0xE0))
  {
    return OFTrue;
  }
  return OFFalse;
}


OFCondition DJ2KDecoderBase::createPlanarConfiguration1Byte(
  Uint8 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *buf = new Uint8[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels));
    Uint8 *s = buf;                        // source
    Uint8 *r = imageFrame;                 // red plane
    Uint8 *g = imageFrame + numPixels;     // green plane
    Uint8 *b = imageFrame + (2*numPixels); // blue plane
    for (unsigned long i=numPixels; i; i--)
    {
      *r++ = *s++;
      *g++ = *s++;
      *b++ = *s++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}


OFCondition DJ2KDecoderBase::createPlanarConfiguration1Word(
  Uint16 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *buf = new Uint16[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels*sizeof(Uint16)));
    Uint16 *s = buf;                        // source
    Uint16 *r = imageFrame;                 // red plane
    Uint16 *g = imageFrame + numPixels;     // green plane
    Uint16 *b = imageFrame + (2*numPixels); // blue plane
    for (unsigned long i=numPixels; i; i--)
    {
      *r++ = *s++;
      *g++ = *s++;
      *b++ = *s++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}

OFCondition DJ2KDecoderBase::createPlanarConfiguration0Byte(
  Uint8 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint8 *buf = new Uint8[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels));
    Uint8 *t = imageFrame;          // target
    Uint8 *r = buf;                 // red plane
    Uint8 *g = buf + numPixels;     // green plane
    Uint8 *b = buf + (2*numPixels); // blue plane
    for (unsigned long i=numPixels; i; i--)
    {
      *t++ = *r++;
      *t++ = *g++;
      *t++ = *b++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}


OFCondition DJ2KDecoderBase::createPlanarConfiguration0Word(
  Uint16 *imageFrame,
  Uint16 columns,
  Uint16 rows)
{
  if (imageFrame == NULL) return EC_IllegalCall;

  unsigned long numPixels = columns * rows;
  if (numPixels == 0) return EC_IllegalCall;

  Uint16 *buf = new Uint16[3*numPixels + 3];
  if (buf)
  {
    memcpy(buf, imageFrame, (size_t)(3*numPixels*sizeof(Uint16)));
    Uint16 *t = imageFrame;          // target
    Uint16 *r = buf;                 // red plane
    Uint16 *g = buf + numPixels;     // green plane
    Uint16 *b = buf + (2*numPixels); // blue plane
    for (unsigned long i=numPixels; i; i--)
    {
      *t++ = *r++;
      *t++ = *g++;
      *t++ = *b++;
    }
    delete[] buf;
  } else return EC_MemoryExhausted;
  return EC_Normal;
}
