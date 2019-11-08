cmake_minimum_required(VERSION 3.10.2)
# cmake_policy(SET CMP0048 NEW)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_VERBOSE_MAKEFILE TRUE)
# Do not enable compiler specific extensions, for eg on GCC use -std=c++1z (=c++17) and not -std=gnu++17
set(CMAKE_CXX_EXTENSIONS OFF)

# Use ccache is available, has to be before "project()"
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  # Support Unix Makefiles and Ninja
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
endif()

project(test_SSLError)

# The PROJECT_SOURCE_DIR here is OpenStudioApplication/, which we will use instead of using CMAKE_SOURCE_DIR
set(PROJECT_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
# We will use BINARY_DIR instead of CMAKE_BINARY_DIR
set(PROJECT_BINARY_DIR "${CMAKE_BINARY_DIR}")

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

###############################################################################
#                                  N I N J A                                  #
###############################################################################
# Ninja support: has to be atop for it to take effect before anything else is done
# Add Color Output if Using Ninja
macro(AddCXXFlagIfSupported flag test)
  CHECK_CXX_COMPILER_FLAG(${flag} ${test})
  if(${${test}})
    message("adding ${flag}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
  endif()
endmacro()

if("Ninja" STREQUAL ${CMAKE_GENERATOR})
  # Clang
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    AddCXXFlagIfSupported(-fcolor-diagnostics COMPILER_SUPPORTS_fcolor-diagnostics)
  endif()

  # g++
  if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    # For some reason it doesn't say its supported, but it works...
    # AddCXXFlagIfSupported(-fdiagnostics-color COMPILER_SUPPORTS_fdiagnostics-color)
    message(STATUS "Ninja: Forcing -fdiagnostics-color=always")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fdiagnostics-color=always")
  endif()
endif()

# Xcode/Ninja generators undefined MAKE
if(CMAKE_GENERATOR MATCHES "Make")
  set(MAKE "$(MAKE)")
else()
  set(MAKE make)
endif()

###############################################################################
#                         C M A K E    C O N T R O L                          #
###############################################################################

# High level project configuration
# Do we actually want everything to go to CMAKE_BINARY_DIR/Products,
# so that when you build OpenStudioApplication you get both OpenStudio (core) and Application in the same place?
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/Products")

set(LIBRARY_SEARCH_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Release" "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/Debug")

###############################################################################
#          C O M P I L E R    A N D    S Y S T E M    O P T I O N S           #
###############################################################################

# Compiler and system specific options
if(UNIX)

  #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC -fno-strict-aliasing")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-strict-aliasing")

  # all warnings
  #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -fno-strict-aliasing -Winvalid-pch -Wnon-virtual-dtor")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -fPIC -fno-strict-aliasing -Winvalid-pch")
  # Treat all warnings as errors
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual -ftemplate-depth=1024")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-overloaded-virtual")
    # DLM: had to add this due to issues with boost optional and gcc, may be resolved if we move to std::optional
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
  endif()

  # Note: CMAKE_CXX_STANDARD set to 17 should already take care of adding -std=c++17 or equivalent
  if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
  endif()
endif()

if(APPLE)
  find_library(SECURITY_FRAMEWORK Security)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  add_definitions(-D_LIBCPP_ENABLE_CXX17_REMOVED_AUTO_PTR)
  #add_definitions(-DBOOST_SYSTEM_NO_DEPRECATED)
endif()

#######################################################################
#                              C O N A N                              #
#######################################################################

set(CMAKE_CONAN_EXPECTED_HASH 709180234748692a642f9e5c4d80d328)
set(CMAKE_CONAN_VERSION "v0.14")

if(EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  file(MD5 "${CMAKE_BINARY_DIR}/conan.cmake" CMAKE_CONAN_HASH)
endif()
if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake" OR NOT "${CMAKE_CONAN_HASH}" MATCHES "${CMAKE_CONAN_EXPECTED_HASH}")
  # Put it in CMAKE_BINARY_DIR so we don't end up with two when building OpenStudioApplication
  message(STATUS "openstudio: Downloading conan.cmake ${CMAKE_CONAN_VERSION} from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://github.com/conan-io/cmake-conan/raw/${CMAKE_CONAN_VERSION}/conan.cmake"
     "${CMAKE_BINARY_DIR}/conan.cmake")
else()
  message(STATUS "openstudio: using existing conan.cmake")
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_check(VERSION 1.0.0 REQUIRED)

message(STATUS "openstudio: RUNNING CONAN")

conan_add_remote(NAME bincrafters
  URL https://api.bintray.com/conan/bincrafters/public-conan)
conan_add_remote(NAME nrel
  URL https://api.bintray.com/conan/commercialbuilding/nrel)

set(BOOST_VERSION "1.69.0")

list(APPEND CONAN_OPTIONS "zlib:minizip=True")
# You do want to rebuild packages if there's a newer recipe in the remote (which applies mostly to our own openstudio_ruby where we don't
# bump the actual package version when we make changes) than the binaries were built with
# 'outdated' also acts like 'missing': if no binary, will build them.
list(APPEND CONAN_BUILD "outdated")

if (MSVC)
  # No-op
elseif (APPLE)
  # No-op
else()
  list(APPEND CONAN_OPTIONS "jsoncpp:use_pic=True")
endif()

conan_cmake_run(REQUIRES
  OpenSSL/1.1.0g@conan/stable

  boost_asio/${BOOST_VERSION}@bincrafters/stable
  boost_program_options/${BOOST_VERSION}@bincrafters/stable
  boost_regex/${BOOST_VERSION}@bincrafters/stable
  boost_filesystem/${BOOST_VERSION}@bincrafters/stable
  boost_crc/${BOOST_VERSION}@bincrafters/stable
  boost_algorithm/${BOOST_VERSION}@bincrafters/stable
  boost_uuid/${BOOST_VERSION}@bincrafters/stable
  boost_log/${BOOST_VERSION}@bincrafters/stable
  boost_numeric_ublas/${BOOST_VERSION}@bincrafters/stable
  boost_functional/${BOOST_VERSION}@bincrafters/stable
  boost_geometry/${BOOST_VERSION}@bincrafters/stable
  pugixml/1.9@bincrafters/stable
  jsoncpp/1.8.4@theirix/stable
  zlib/1.2.11@conan/stable
  fmt/5.2.1@bincrafters/stable

  cpprestsdk/2.10.13@bincrafters/stable
  websocketpp/0.8.1@bincrafters/stable # A dependency of cpprestsdk
  BASIC_SETUP CMAKE_TARGETS NO_OUTPUT_DIRS
  OPTIONS ${CONAN_OPTIONS}
  BUILD ${CONAN_BUILD}
  # Passes `-u, --update`    to conan install: Check updates exist from upstream remotes
  # That and build=outdated should ensure we track the right
  UPDATE
)

message("CONAN_LIBS=${CONAN_LIBS}")

###############################################################################
#                              E X E C U T A B L E                            #
###############################################################################

set(target_name OpenStudioApp)

set(${target_name}_depends
  CONAN_PKG::jsoncpp
  CONAN_PKG::zlib
  CONAN_PKG::fmt
  CONAN_PKG::pugixml
  CONAN_PKG::boost_filesystem
  CONAN_PKG::boost_crc
  CONAN_PKG::boost_regex
  CONAN_PKG::boost_algorithm
  CONAN_PKG::boost_uuid
  CONAN_PKG::boost_log
  CONAN_PKG::boost_program_options
  CONAN_PKG::boost_numeric_ublas
  CONAN_PKG::boost_functional
  CONAN_PKG::boost_geometry
  CONAN_PKG::cpprestsdk
  CONAN_PKG::OpenSSL
)

if(WIN32)
  list(APPEND ${target_name}_depends mpr)
elseif(APPLE)
  list(APPEND ${target_name}_depends ${SECURITY_FRAMEWORK})
  # For cpprest
  list(APPEND ${target_name}_depends ${COREFOUNDATION_LIBRARY})
endif()

add_executable(${target_name}
  main.cpp
)

target_link_libraries(${target_name} ${${target_name}_depends})


