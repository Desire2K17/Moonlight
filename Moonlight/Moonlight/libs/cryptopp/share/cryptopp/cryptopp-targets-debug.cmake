#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "cryptopp-static" for configuration "Debug"
set_property(TARGET cryptopp-static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(cryptopp-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "ASM_MASM;CXX"
  IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG "ws2_32"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/cryptopp-static.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS cryptopp-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_cryptopp-static "${_IMPORT_PREFIX}/debug/lib/cryptopp-static.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
