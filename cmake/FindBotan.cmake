# - Try to find Botan
# If Botan_SUFFIX isdefined, search will use this as well
# Once done this will define
#  Botan_FOUND - System has Botan
#  Botan_INCLUDE_DIR - The Botan include directory
#  Botan_LIBRARIES - The libraries needed to use Botan
#  Botan_VERSION_STRING - The version of Botan ("major.minor")

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_Botan QUIET Botan)

set(Botan_NAMES "botan")
if (Botan_SUFFIX)
    set(Botan_NAMES ${Botan_NAMES} "botan-${Botan_SUFFIX}")
endif()

find_path(Botan_INCLUDE_DIR NAMES botan/botan.h
   HINTS
   ${PC_Botan_INCLUDEDIR}
   ${PC_Botan_INCLUDE_DIRS}
   ~/.local/include
   PATH_SUFFIXES ${Botan_NAMES}
   )

find_library(Botan_LIBRARIES NAMES ${Botan_NAMES}
   HINTS
   ${PC_Botan_LIBDIR}
   ${PC_Botan_LIBRARY_DIRS}
   ~/.local/lib
   )

if(PC_Botan_VERSION)
    set(Botan_VERSION_STRING ${PC_Botan_VERSION})
elseif(Botan_INCLUDE_DIR AND EXISTS "${Botan_INCLUDE_DIR}/botan/build.h")
    file(STRINGS "${Botan_INCLUDE_DIR}/botan/build.h" Botan_major_version
         REGEX "^#define[\t ]+BOTAN_VERSION_MAJOR[\t ]+.+")
    file(STRINGS "${Botan_INCLUDE_DIR}/botan/build.h" Botan_minor_version
         REGEX "^#define[\t ]+BOTAN_VERSION_MINOR[\t ]+.+")
    string(REGEX REPLACE "^#define[\t ]+BOTAN_VERSION_MAJOR[\t ]+(.+)" "\\1"
           Botan_major_version "${Botan_major_version}")
    string(REGEX REPLACE "^#define[\t ]+BOTAN_VERSION_MINOR[\t ]+(.+)" "\\1"
           Botan_minor_version "${Botan_minor_version}")
	set(Botan_VERSION_STRING "${Botan_major_version}.${Botan_minor_version}")
    unset(Botan_major_version)
    unset(Botan_minor_version)
endif()

# handle the QUIETLY and REQUIRED arguments and set Botan_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Botan
                                  REQUIRED_VARS Botan_LIBRARIES Botan_INCLUDE_DIR
                                  VERSION_VAR Botan_VERSION_STRING)

mark_as_advanced(Botan_INCLUDE_DIR Botan_LIBRARIES Botan_VERSION_STRING)
