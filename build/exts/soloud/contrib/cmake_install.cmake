# Install script for directory: A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "A:/Program Files (x86)/engine-lab-env")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
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

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/Debug/soloud.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/Release/soloud.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/MinSizeRel/soloud.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/RelWithDebInfo/soloud.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/soloud" TYPE FILE FILES
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_audiosource.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_bassboostfilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_biquadresonantfilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_bus.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_c.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_dcremovalfilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_echofilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_error.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_fader.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_fft.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_fftfilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_file.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_file_hack_off.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_file_hack_on.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_filter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_flangerfilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_internal.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_lofifilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_monotone.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_openmpt.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_queue.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_robotizefilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_sfxr.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_speech.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_tedsid.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_thread.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_vic.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_vizsn.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_wav.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_waveshaperfilter.h"
    "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/exts/soloud/contrib/../include/soloud_wavstream.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/soloud-config.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/soloud-config.cmake"
         "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/soloud-config.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/soloud-config-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/cmake/soloud-config.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/soloud-config.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/soloud-config-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/soloud-config-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/soloud-config-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/cmake" TYPE FILE FILES "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/CMakeFiles/Export/272ceadb8458515b2ae4b5630a6029cc/soloud-config-release.cmake")
  endif()
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "A:/Users/amthe/OneDrive/Dokument/GitHub/LabEngineComponentSys/build/exts/soloud/contrib/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
