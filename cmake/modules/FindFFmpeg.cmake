# Locate ffmpeg
# This module defines
# FFmpeg_LIBRARIES
# FFmpeg_FOUND, if false, do not try to link to ffmpeg
# FFmpeg_INCLUDE_DIR, where to find the headers
#
# $FFMPEG_DIR is an environment variable that would
# correspond to the ./configure --prefix=$FFMPEG_DIR
#
# (Created by Robert Osfield for OpenSceneGraph.)
# Adapted by Bernardt Duvenhage for FLITr.
# Modified by Korneliusz Jarzebski for DreamDesktop Plasma Wallpaper

IF("${FFmpeg_ROOT}" STREQUAL "")
    SET(FFmpeg_ROOT "$ENV{FFMPEG_DIR}")# CACHE PATH "Path to search for custom FFmpeg library.")
ENDIF()

MACRO(FFmpeg_FIND varname shortname headername)

SET(FFmpeg_${varname}_INCLUDE_DIRS FFmpeg_${varname}_INCLUDE_DIRS-NOTFOUND)
SET(FFmpeg_${varname}_LIBRARIES FFmpeg_${varname}_LIBRARIES-NOTFOUND)
SET(FFmpeg_${varname}_FOUND FALSE)

MARK_AS_ADVANCED(FORCE FFmpeg_${varname}_INCLUDE_DIRS)
MARK_AS_ADVANCED(FORCE FFmpeg_${varname}_LIBRARIES)
MARK_AS_ADVANCED(FORCE FFmpeg_${varname}_FOUND)

    IF(FFmpeg_NO_SYSTEM_PATHS)
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFmpeg Headers"
            NO_DEFAULT_PATH
        )
    ELSE()
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS lib${shortname}/${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
            /usr/local/include
            /usr/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFmpeg Headers"
        )
    ENDIF()

    IF(FFmpeg_NO_SYSTEM_PATHS)
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS ${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
            DOC "Location of FFmpeg Headers"
            PATH_SUFFIXES ffmpeg
            NO_DEFAULT_PATH
        )
    ELSE()
        FIND_PATH(FFmpeg_${varname}_INCLUDE_DIRS ${headername}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/include
            $ENV{FFMPEG_DIR}/include
            /usr/local/include
            /usr/include
            PATH_SUFFIXES ffmpeg
            DOC "Location of FFmpeg Headers"
        )
    ENDIF()

    IF(FFmpeg_NO_SYSTEM_PATHS)
        FIND_LIBRARY(FFmpeg_${varname}_LIBRARIES
            NAMES ${shortname}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/lib${shortname}
            ${FFmpeg_ROOT}/lib
            DOC "Location of FFmpeg Libraries"
            NO_DEFAULT_PATH
        )
    ELSE()
        FIND_LIBRARY(FFmpeg_${varname}_LIBRARIES
            NAMES ${shortname}
            PATHS
            ${FFmpeg_ROOT}
            ${FFmpeg_ROOT}/lib
            $ENV{FFMPEG_DIR}/lib
            /usr/local/lib
            /usr/local/lib64
            /usr/lib
            /usr/lib64
            /usr/lib/i386-linux-gnu
            DOC "Location of FFmpeg Libraries"
        )
    ENDIF()

    IF (FFmpeg_${varname}_LIBRARIES AND FFmpeg_${varname}_INCLUDE_DIRS)
        SET(FFmpeg_${varname}_FOUND TRUE)
    ELSE()
        SET(FFmpeg_${varname}_FOUND FALSE)
    ENDIF()

    #MESSAGE("${FFmpeg_${varname}_INCLUDE_DIRS}")
    #MESSAGE("${FFmpeg_${varname}_LIBRARIES}")
    #MESSAGE("  FOUND ${FFmpeg_${varname}_FOUND}")

ENDMACRO(FFmpeg_FIND)

SET(STDINT_OK TRUE)

SET(FFmpeg_INCLUDE_DIRS FFmpeg_INCLUDE_DIRS-NOTFOUND)# CACHE STRING "docstring")
SET(FFmpeg_LIBRARIES FFmpeg_LIBRARIES-NOTFOUND)# CACHE STRING "docstring")
SET(FFmpeg_FOUND FALSE)


FFmpeg_FIND(LIBAVFORMAT avformat avformat.h)
FFmpeg_FIND(LIBAVDEVICE avdevice avdevice.h)
FFmpeg_FIND(LIBAVCODEC  avcodec  avcodec.h)
FFmpeg_FIND(LIBAVUTIL   avutil   avutil.h)
FFmpeg_FIND(LIBSWSCALE  swscale  swscale.h)

MARK_AS_ADVANCED(CLEAR FFmpeg_LIBAVFORMAT_LIBRARIES)

IF(FFmpeg_LIBAVFORMAT_FOUND AND FFmpeg_LIBAVDEVICE_FOUND AND FFmpeg_LIBAVCODEC_FOUND AND FFmpeg_LIBAVUTIL_FOUND AND FFmpeg_LIBSWSCALE_FOUND AND STDINT_OK)
    SET(FFmpeg_FOUND TRUE)

    SET(FFmpeg_INCLUDE_DIRS
        ${FFmpeg_LIBAVFORMAT_INCLUDE_DIRS} #${FFmpeg_LIBAVFORMAT_INCLUDE_DIRS}/libavformat
        ${FFmpeg_LIBAVDEVICE_INCLUDE_DIRS} #${FFmpeg_LIBAVDEVICE_INCLUDE_DIRS}/libavdevice
        ${FFmpeg_LIBAVCODEC_INCLUDE_DIRS} #${FFmpeg_LIBAVCODEC_INCLUDE_DIRS}/libavcodec
        ${FFmpeg_LIBAVUTIL_INCLUDE_DIRS} #${FFmpeg_LIBAVUTIL_INCLUDE_DIRS}/libavutil
        ${FFmpeg_LIBSWSCALE_INCLUDE_DIRS} #${FFmpeg_LIBSWSCALE_INCLUDE_DIRS}/libswscale
        #CACHE STRING  "docstring"
    )

    IF (FFmpeg_STDINT_INCLUDE_DIR)
        SET(FFmpeg_INCLUDE_DIRS
            ${FFmpeg_INCLUDE_DIRS}
            ${FFmpeg_STDINT_INCLUDE_DIR}
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavformat
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavdevice
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavcodec
            #${FFmpeg_STDINT_INCLUDE_DIR}/libavutil
            #${FFmpeg_STDINT_INCLUDE_DIR}/libswscale
            #CACHE  STRING  "docstring"
        )
    ENDIF()

    SET(FFmpeg_LIBRARIES
        ${FFmpeg_LIBAVFORMAT_LIBRARIES}
        ${FFmpeg_LIBAVDEVICE_LIBRARIES}
        ${FFmpeg_LIBAVCODEC_LIBRARIES}
        ${FFmpeg_LIBAVUTIL_LIBRARIES}
        ${FFmpeg_LIBSWSCALE_LIBRARIES}
        #CACHE  STRING  "docstring"
    )

    #MESSAGE("  ${FFmpeg_INCLUDE_DIRS}")
    #MESSAGE("  ${FFmpeg_LIBRARIES}")

ELSE()
    MESSAGE("Could not find FFmpeg")
    SET(FFmpeg_INCLUDE_DIRS FFmpeg_INCLUDE_DIRS-NOTFOUND)# CACHE  STRING  "docstring")
    SET(FFmpeg_LIBRARIES FFmpeg_LIBRARIES-NOTFOUND)# CACHE  STRING  "docstring")
    SET(FFmpeg_FOUND FALSE)
ENDIF()
