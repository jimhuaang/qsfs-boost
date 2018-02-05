cmake_minimum_required(VERSION 3.0)


#
# options
#
option (BUILD_PACKAGING    "Enable build of distribution packages using CPack." OFF)
option (BUILD_SHARED_LIBS  "Build shared libraries." OFF)
# to turn on, specify -DBUILD_TESTING=ON in cmake command line
option (BUILD_TESTING      "Enable build of the unit tests and their execution using CTest." OFF)
option (INSTALL_HEADERS    "Request installation of headers and other development files." OFF)
option (REGISTER_BUILD_DIR "Request entry of build directory in CMake's package registry." OFF)



#
# system info
#
execute_process(COMMAND uname -m OUTPUT_VARIABLE MACHINE_HARDWARE)
if (MACHINE_HARDWARE MATCHES "x86_64" OR MACHINE_HARDWARE MATCHES "x64"
    OR MACHINE_HARDWARE MATCHES "amd64")
  set(IS_MACH_64 TRUE CACHE BOOL "Machine hardware is 64-bit")
endif()

execute_process(COMMAND whoami OUTPUT_VARIABLE CURRENT_USER)
if(CURRENT_USER MATCHES "root")
  set (IS_ROOT TRUE CACHE BOOL "Current user is root")
endif()

if (IS_MACH_64)
  # when GCC is built as a 64-bit only compiler
  # this only affect configure step, not affect build step
  set(ENV{LIBRARY_PATH} "/usr/lib/x86_64-linux-gnu/:$LIBRARY_PATH"
      CACHE PATH "library path env variable")
endif()


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
set (QSFS_SOURCE_DIR ${CMAKE_SOURCE_DIR}/src
     CACHE PATH "qsfs source directory")
set (THIRD_PARTY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party
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

# download and install dependencies under ./third_party/install
set (EXTERNAL_PROJECT_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/third_party/install
     CACHE PATH "Installation prefix for dependencies")
if (NOT googletest_INSTALLED)
  include (cmake/gtest.cmake)
endif()

if(NOT googleflags_INSTALLED)
  include (cmake/gflags.cmake)
endif()

if(NOT googlelog_INSTALLED)
  include(cmake/glog.cmake)
endif()

if (NOT qingstorsdk_INSTALLED)
  include (cmake/qingstorsdk.cmake)
endif()

#
# linter target (make lint)
#
add_custom_target (lint COMMAND ${CMAKE_COMMAND} -P
                   ${CMAKE_SOURCE_DIR}/cmake/lint.cmake)


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
# add_compile_options (-Wall)  # for dev
# disable warnings for boost, this is only for gcc 4.8
# add_compile_options (-Wno-unused-local-typedefs -Wno-delete-non-virtual-dtor)
# add_compile_options (-g -o0 -fno-inline)  # for debugging

if(IS_MACH_64)
  # fuse need to uses 64 bit off_t *
  add_compile_options(-D_FILE_OFFSET_BITS=64)
endif()


#
# set up include/link directories
#
# Where qsfs's header files can be found
include_directories (${QSFS_SOURCE_DIR})

# Where boost files can be found
include_directories(${THIRD_PARTY_SOURCE_DIR}/boost_1_49_0)
# Where boost lib can be found
if (IS_MACH_64)
  # link to boost lib built on x86_64 with gcc4.1.2
  link_directories(${THIRD_PARTY_SOURCE_DIR}/boost_1_49_0/lib64)
else (IS_MACH_64)
  # link to boost lib built on x86 with gcc4.1.2
  link_directories(${THIRD_PARTY_SOURCE_DIR}/boost_1_49_0/lib)
endif ()

# other dependencies (glog/gtest) built with source code and
# installed under third-party/install in cmake configure.
# Where dependencies's header files can be found.
include_directories(${EXTERNAL_PROJECT_INSTALL_PREFIX}/include)
# Where dependencies's targets can be found.
# On ubuntu x86_64 all libs get installed under lib
link_directories(${EXTERNAL_PROJECT_INSTALL_PREFIX}/lib)
if (IS_MACH_64)
  # On Centos x86_64 glog/gtest libs get installed under lib64
  link_directories(${EXTERNAL_PROJECT_INSTALL_PREFIX}/lib64)
endif()


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
# uninstall
#
include(cmake/UninstallProject.cmake)
setup_uninstall_project(${PROJECT_NAME})

set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_${PROJECT_NAME})

if(TARGET uninstall_qingstorsdk)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_qingstorsdk)
endif()

if(TARGET uninstall_googletest)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_googletest)
endif()

if(TARGET uninstall_googlelog)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_googlelog)
endif()

if(TARGET uninstall_googleflags)
  set (UNINSTALL_ALL ${UNINSTALL_ALL} uninstall_googleflags)
endif()

add_custom_target(
  uninstall DEPENDS uninstall_${PROJECT_NAME}
)

add_custom_target(
  uninstall_all DEPENDS ${UNINSTALL_ALL}
)


#
# subdirectories
#
add_subdirectory (src)


#
# testing
#
if (BUILD_TESTING)
  include(CTest)
  enable_testing ()
  add_subdirectory(test)
endif (BUILD_TESTING)
