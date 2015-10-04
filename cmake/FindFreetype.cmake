# - Find Freetype
# Find the Freetype includes and libraries
#
# Following variables are provided:
# FREETYPE_FOUND
#     True if Freetype has been found
# FREETYPE_INCLUDE_DIRS
#     The include directories of Freetype
# FREETYPE_LIBRARIES
#     Freetype library list

if(MSVC)
    find_path(FREETYPE_INCLUDE_DIRS NAMES freetype/freetype.h PATHS "${PROJECT_SOURCE_DIR}/dependencies/include")
    find_library(FREETPYE_LIBRARY NAMES freetype PATHS "${PROJECT_SOURCE_DIR}/dependencies/lib")
    set(FREETYPE_FOUND 1)
    set(FREETPYE_LIBRARIES ${FREETPYE_LIBRARY})
elseif(UNIX)
    include(FindPkgConfig)
    pkg_check_modules(FREETYPE freetype2)
else()
    set(FREETYPE_FOUND 0)
endif()

