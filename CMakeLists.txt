cmake_minimum_required(VERSION 3.0)


#
# options
#
option (BUILD_PACKAGING    "Enable build of distribution packages using CPack." OFF)
option (BUILD_SHARED_LIBS  "Build shared libraries." OFF)
# to turn on, specify -DBUILD_TESTS=ON in cmake command line
option (BUILD_TESTS        "Enable build of the unit tests and their execution using CTest." OFF)
option (INSTALL_HEADERS    "Request installation of headers and other development files." OFF)
option (REGISTER_BUILD_DIR "Request entry of build directory in CMake's package registry." OFF)

# for temporary test
set (CMAKE_C_COMPILER   "/opt/gcc-4.1.2/bin/gcc")
set (CMAKE_CXX_COMPILER "/opt/gcc-4.1.2/bin/g++")
SET (ENV{LIBRARY_PATH} "/usr/lib/x86_64-linux-gnu/:$LIBRARY_PATH")

execute_process(COMMAND uname -m OUTPUT_VARIABLE MACHINE_HARDWARE)

if (MACHINE_HARDWARE MATCHES "x86_64" OR MACHINE_HARDWARE MATCHES "x64"
    OR MACHINE_HARDWARE MATCHES "amd64")
  set(MACH_64 TRUE CACHE BOOL "Machine hardware is 64-bit")
endif()

if (MACH_64)
  # when GCC is built as a 64-bit only compiler
  # this only affect configure step, not affect build step
  set(ENV{LIBRARY_PATH} "/usr/lib/x86_64-linux-gnu/:$LIBRARY_PATH"
      CACHE PATH "library path env variable")
endif()

# this only affect configure step, not affect build step
#execute_process(COMMAND export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/:$LIBRARY_PATH)

#
# project information
#
set (PACKAGE_NAME          "qsfs")
set (PACKAGE_VERSION_MAJOR 1)
set (PACKAGE_VERSION_MINOR 0)
set (PACKAGE_VERSION_PATCH 0)
set (PACKAGE_VERSION       ${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_PATCH})
set (PACKAGE_STRING        "${QSFS_NAME} ${QSFS_VERSION}")
set (PACKAGE_TARNAME       "${QSFS_NAME}-${QSFS_VERSION}")
set (PACKAGE_BUGREPORT     "https://github.com/jimhuaang/qsfs/issues")
set (PACKAGE_DESCRIPTION   "A FUSE based filesystem allows you to mount a qingstor bucket in Linux.")
set (PACKAGE_URL           "https://github.com/jimhuaang/qsfs")

string (TOUPPER "${PACKAGE_NAME}"       PACKAGE_PREFIX)
string (TOLOWER "${CMAKE_SYSTEM_NAME}"  CPACK_SYSTEM_NAME)

project (${PACKAGE_NAME})
message (STATUS "Building project version: ${PACKAGE_VERSION}")

#
# directories
#
# qsfs source directory
set (QSFS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src/
     CACHE PATH "qsfs source directory")
set (THIRD_PARTY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/
     CACHE PATH "third party source directory")

#
# configure version
#
if (EXISTS ${QSFS_SOURCE_DIR}/configure/VersionConfig.h)
    FILE(REMOVE ${QSFS_SOURCE_DIR}/configure/VersionConfig.h)
endif ()
configure_file (cmake/templates/VersionConfig.h.in
                "${QSFS_SOURCE_DIR}/configure/VersionConfig.h")


# 
# dependencies
#
find_package (Threads REQUIRED)
if (NOT Threads_FOUND)
  message (FATAL_ERROR "Could not find pthread. Check the log file"
  "\n\t${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeError.log")
endif ()


#
# compile set up
#
# As we build gflags as a subproject and gflags is built as static single-threaded
# library by default. As glog depend on the gflags library and should link to gfalgs
# target, and this requires position-independent code.
# As not all systems support '-shared' option to produce a shared object
# which can then be linked with other object to form an executable.
# For predictable results, explicitly specify the -fPIC option.
# For example, on Debain (gcc 6.3.0), without explicit set -fPIC, cmake
# raise error: 
#   EXPORT glog-target requires target gflags_nothreads_static 
#   which is not in export set.
add_compile_options (-fPIC)

# do not specify -g, instead specify -DCMAKE_BUILD_TYPE=Debug/Release in command line
add_compile_options (-Wall)
#add_compile_options (-Wunused-value)

#if(MACH_64)
#  # fuse
#  add_compile_options(-D_FILE_OFFSET_BITS=64)
#endif()


#
# set up include directories
#
# Where qsfs's header files can be found
include_directories (${QSFS_SOURCE_DIR})

# Where third party files can be found
include_directories(${THIRD_PARTY_SOURCE_DIR})


#
# installation
#
# directories vars should be put before include directories
set (INSTALL_RUNTIME_DIR bin CACHE PATH "Installation directory for executables")
set (INSTALL_LIBRARY_DIR lib CACHE PATH "Installation directory for libraries")
set (INSTALL_INCLUDE_DIR include CACHE PATH "Installation directory for header files")
set (INSTALL_CMAKE_DIR  ${INSTALL_LIBRARY_DIR}/cmake/${PROJECT_NAME} CACHE PATH 
     "Installation directory for CMake files")
set (EXPORT_NAME ${PROJECT_NAME}-targets)
set (INSTALL_PREFIX_REL2CONFIG_DIR .)
file (RELATIVE_PATH INSTALL_PREFIX_REL2CONFIG_DIR
      "${CMAKE_INSTALL_PREFIX}/${INSTALL_CMAKE_DIR}" "${CMAKE_INSTALL_PREFIX}")

#
# subdirectories
#
add_subdirectory (src)


#
# testing
#
if (BUILD_TESTS)
  include(CTest)
  enable_testing ()
  add_subdirectory(test)
endif (BUILD_TESTS)