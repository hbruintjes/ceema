# Copyright 2017 Harold Bruintjes
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required (VERSION 3.0 FATAL_ERROR)
if(${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION} VERSION_GREATER 3.0)
	cmake_policy(VERSION 3.1)
endif()

project(ceema CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

set( PROJECT_FULLNAME "ceema")
set( PROJECT_DESCRIPTION "Threema client (library)")
set( ceema_MAJORVERSION 0)
set( ceema_MINORVERSION 0)
set( ceema_MAINTENANCEVERSION 1)
set( ceema_VERSION "${ceema_MAJORVERSION}.${ceema_MINORVERSION}.${ceema_MAINTENANCEVERSION}")
set( PROJECT_VERSION "${ceema_VERSION}")
set( ceema_NAME "ceema" )
set( ceema_DESCRIPTION ${PROJECT_DESCRIPTION} )

option(BUILD_STATIC "Build libraries statically." OFF)
option(IPV6 "Enable IPV6." ON)

include (TestBigEndian)
TEST_BIG_ENDIAN(PLATFORM_BIG_ENDIAN)

set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
# By default, build a debug version.
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type")
endif()

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic")
elseif(CMAKE_COMPILER_IS_CLANG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weverything")
endif()

set(ceema_LIBRARIES_DIR "${PROJECT_BINARY_DIR}/lib")

# Offer the user the choice of overriding the installation directories
set(INCLUDE_INSTALL_DIR include/ CACHE PATH "Installation directory for header files" )
set(LIB_INSTALL_DIR lib/ CACHE PATH "Installation directory for libraries")
set(BIN_INSTALL_DIR lib/ CACHE PATH "Installation directory for executables")

if(WIN32 AND NOT CYGWIN)
  set(DEF_INSTALL_CMAKE_DIR CMake)
else()
  set(DEF_INSTALL_CMAKE_DIR lib/CMake/ceema)
endif()
set(CMAKE_INSTALL_DIR ${DEF_INSTALL_CMAKE_DIR} CACHE PATH  "Installation directory for CMake files")

foreach(p LIB BIN INCLUDE CMAKE)
  set(var ${p}_INSTALL_DIR)
  if(NOT IS_ABSOLUTE "${${var}}")
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
  endif()
endforeach()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin CACHE PATH "Directory for built executables")

find_package(CURL QUIET REQUIRED)
find_package(OpenSSL QUIET REQUIRED)
find_package(sodium QUIET REQUIRED)
#find_package(JsonCpp QUIET REQUIRED)
add_subdirectory(3rdparty/variant)

#set(Botan_SUFFIX "2" CACHE STRING "Botan path version suffix")
#find_package(Botan QUIET REQUIRED)

find_package(GLIB2 QUIET)
find_package(Boost 1.56 QUIET REQUIRED)
add_definitions(-DBOOST_THREAD_VERSION=4)

if(CMAKE_BUILD_TYPE MATCHES DEBUG)
    set(sodium_LIBRARIES ${sodium_LIBRARY_DEBUG})
else()
    set(sodium_LIBRARIES ${sodium_LIBRARY_RELEASE})
endif()

add_subdirectory(src)
