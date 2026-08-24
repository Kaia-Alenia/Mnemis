# Install script for directory: /home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "/home/alejandro/Escritorio/Mnemis/build-asan/lib/libtag.a")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/taglib" TYPE FILE FILES
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/tag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/fileref.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/audioproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/taglib_export.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-build/taglib/../taglib_config.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/taglib.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tstring.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tlist.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tlist.tcc"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tstringlist.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tbytevector.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tbytevectorlist.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tbytevectorstream.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tiostream.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tfilestream.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tmap.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tmap.tcc"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tpropertymap.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/trefcounter.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/toolkit/tdebuglistener.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/mpegfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/mpegproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/mpegheader.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/xingheader.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v1/id3v1tag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v1/id3v1genres.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2extendedheader.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2frame.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2header.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2synchdata.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2footer.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2framefactory.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/id3v2tag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/attachedpictureframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/commentsframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/eventtimingcodesframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/generalencapsulatedobjectframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/ownershipframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/popularimeterframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/privateframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/relativevolumeframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/synchronizedlyricsframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/textidentificationframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/uniquefileidentifierframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/unknownframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/unsynchronizedlyricsframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/urllinkframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/chapterframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/tableofcontentsframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpeg/id3v2/frames/podcastframe.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/oggfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/oggpage.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/oggpageheader.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/xiphcomment.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/vorbis/vorbisfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/vorbis/vorbisproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/flac/oggflacfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/speex/speexfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/speex/speexproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/opus/opusfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ogg/opus/opusproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/flac/flacfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/flac/flacpicture.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/flac/flacproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/flac/flacmetadatablock.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ape/apefile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ape/apeproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ape/apetag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ape/apefooter.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/ape/apeitem.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpc/mpcfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mpc/mpcproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/wavpack/wavpackfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/wavpack/wavpackproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/trueaudio/trueaudiofile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/trueaudio/trueaudioproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/riff/rifffile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/riff/aiff/aifffile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/riff/aiff/aiffproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/riff/wav/wavfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/riff/wav/wavproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/riff/wav/infotag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/asf/asffile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/asf/asfproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/asf/asftag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/asf/asfattribute.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/asf/asfpicture.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mp4/mp4file.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mp4/mp4atom.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mp4/mp4tag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mp4/mp4item.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mp4/mp4properties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mp4/mp4coverart.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mod/modfilebase.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mod/modfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mod/modtag.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/mod/modproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/it/itfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/it/itproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/s3m/s3mfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/s3m/s3mproperties.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/xm/xmfile.h"
    "/home/alejandro/Escritorio/Mnemis/build-asan/_deps/taglib-src/taglib/xm/xmproperties.h"
    )
endif()

