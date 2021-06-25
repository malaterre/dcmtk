# declare additional include directories
include_directories("${dcmjp2k_SOURCE_DIR}/include" "${ofstd_SOURCE_DIR}/include" "${oflog_SOURCE_DIR}/include" "${dcmdata_SOURCE_DIR}/include" "${dcmimgle_SOURCE_DIR}/include" "${dcmimage_SOURCE_DIR}/include" ${ZLIB_INCDIR})

# create library from source files
DCMTK_ADD_LIBRARY(dcmjp2k djcparam djdecode djencode djrparam djcodecd djutils djcodece)

DCMTK_TARGET_LINK_MODULES(dcmjp2k ofstd oflog dcmdata dcmimgle dcmimage openjp2)
