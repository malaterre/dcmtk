# declare additional include directories
include_directories("${dcmjp2k_SOURCE_DIR}/include" "${ofstd_SOURCE_DIR}/include" "${oflog_SOURCE_DIR}/include" "${dcmdata_SOURCE_DIR}/include" "${dcmimgle_SOURCE_DIR}/include" "${dcmimage_SOURCE_DIR}/include" ${ZLIB_INCDIR} ${LIBTIFF_INCDIR} ${LIBPNG_INCDIR})

# declare executables
foreach(PROGRAM dcmcjp2k dcmdjp2k dcmk2pnm)
  DCMTK_ADD_EXECUTABLE(${PROGRAM} ${PROGRAM})
endforeach()

# make sure executables are linked to the corresponding libraries
foreach(PROGRAM dcmcjp2k dcmdjp2k dcmk2pnm)
  DCMTK_TARGET_LINK_MODULES(${PROGRAM} dcmjp2k openjp2 dcmimage dcmimgle dcmdata oflog ofstd ofstd)
endforeach()
