#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "soloud" for configuration "Debug"
set_property(TARGET soloud APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(soloud PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C;CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/soloud.lib"
  )

list(APPEND _cmake_import_check_targets soloud )
list(APPEND _cmake_import_check_files_for_soloud "${_IMPORT_PREFIX}/lib/soloud.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
