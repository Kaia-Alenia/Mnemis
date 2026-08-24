# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-src"
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-build"
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix"
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix/tmp"
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix/src/taglib-populate-stamp"
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix/src"
  "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix/src/taglib-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix/src/taglib-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/alejandro/Escritorio/Mnemis/build-baseline/_deps/taglib-subbuild/taglib-populate-prefix/src/taglib-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
