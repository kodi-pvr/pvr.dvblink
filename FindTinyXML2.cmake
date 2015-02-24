# - Find TinyXML2
# Find the native TinyXML includes and library
#
#   TINYXML2_FOUND        - True if TinyXML found.
#   TINYXML2_INCLUDE_DIRS - where to find tinyxml.h, etc.
#   TINYXML2_LIBRARIES    - List of libraries when using TinyXML.
#

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
  pkg_check_modules (TINYXML2 tinyxml2)
  list(APPEND TINYXML2_INCLUDE_DIRS ${TINYXML2_INCLUDEDIR})
endif()
if(NOT TINYXML2_FOUND)
  find_path( TINYXML2_INCLUDE_DIRS "tinyxml2.h"
             PATH_SUFFIXES "tinyxml2" )

  find_library( TINYXML2_LIBRARIES
                NAMES "tinyxml2"
                PATH_SUFFIXES "tinyxml2" )
endif()

# handle the QUIETLY and REQUIRED arguments and set TINYXML2_FOUND to TRUE if
# all listed variables are TRUE
include( "FindPackageHandleStandardArgs" )
find_package_handle_standard_args(TinyXML2 DEFAULT_MSG TINYXML2_INCLUDE_DIRS TINYXML2_LIBRARIES )

mark_as_advanced(TINYXML2_INCLUDE_DIRS TINYXML2_LIBRARIES)
