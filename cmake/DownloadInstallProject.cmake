set(_DownloadProjectDir "${CMAKE_CURRENT_LIST_DIR}")

include(CMakeParseArguments)

function(setup_download_project)

    set(options QUIET)
    set(oneValueArgs
        PROJ
        PREFIX
        DOWNLOAD_DIR
        SOURCE_DIR
        BINARY_DIR
        # Prevent the following from being passed through
        CONFIGURE_COMMAND
        BUILD_COMMAND
        INSTALL_COMMAND
        TEST_COMMAND
    )
    set(multiValueArgs "")

    cmake_parse_arguments(DL_ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # Hide output if requested
    if (DL_ARGS_QUIET)
        set(OUTPUT_QUIET "OUTPUT_QUIET")
    else()
        unset(OUTPUT_QUIET)
        message(STATUS "Downloading/updating ${DL_ARGS_PROJ}")
    endif()

    # Set up where we will put our temporary CMakeLists.txt file and also
    # the base point below which the default source and binary dirs will be.
    # The prefix must always be an absolute path.
    if (NOT DL_ARGS_PREFIX)
        set(DL_ARGS_PREFIX "${CMAKE_BINARY_DIR}")
    else()
        get_filename_component(DL_ARGS_PREFIX "${DL_ARGS_PREFIX}" ABSOLUTE
                               BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
    if (NOT DL_ARGS_DOWNLOAD_DIR)
        set(DL_ARGS_DOWNLOAD_DIR "${DL_ARGS_PREFIX}/${DL_ARGS_PROJ}")
    endif()

    # Ensure the caller can know where to find the source and build directories
    if (NOT DL_ARGS_SOURCE_DIR)
        set(DL_ARGS_SOURCE_DIR "${DL_ARGS_PREFIX}/${DL_ARGS_PROJ}/source")
    endif()
    if (NOT DL_ARGS_BINARY_DIR)
        set(DL_ARGS_BINARY_DIR "${DL_ARGS_PREFIX}/${DL_ARGS_PROJ}/build")
    endif()
    set(${DL_ARGS_PROJ}_DOWNLOAD_DIR "${DL_ARGS_DOWNLOAD_DIR}" PARENT_SCOPE)
    set(${DL_ARGS_PROJ}_SOURCE_DIR "${DL_ARGS_SOURCE_DIR}" PARENT_SCOPE)
    set(${DL_ARGS_PROJ}_BINARY_DIR "${DL_ARGS_BINARY_DIR}" PARENT_SCOPE)

    # The way that CLion manages multiple configurations, it causes a copy of
    # the CMakeCache.txt to be copied across due to it not expecting there to
    # be a project within a project.  This causes the hard-coded paths in the
    # cache to be copied and builds to fail.  To mitigate this, we simply
    # remove the cache if it exists before we configure the new project.  It
    # is safe to do so because it will be re-generated.  Since this is only
    # executed at the configure step, it should not cause additional builds or
    # downloads.
    file(REMOVE "${DL_ARGS_DOWNLOAD_DIR}/CMakeCache.txt")

    configure_file("${_DownloadProjectDir}/templates/DownloadProject.CMakeLists.cmake.in"
                   "${DL_ARGS_DOWNLOAD_DIR}/CMakeLists.txt")
endfunction()


function(download_project PROJECT)
    # Create and build a separate CMake project to carry out the download.
    # If we've already previously done these steps, they will not cause
    # anything to be updated, so extra rebuilds of the project won't occur.
    # Make sure to pass through CMAKE_MAKE_PROGRAM in case the main project
    # has this set to something not findable on the PATH.
    execute_process(COMMAND ${CMAKE_COMMAND}
        -G "${CMAKE_GENERATOR}"
        -D "CMAKE_MAKE_PROGRAM:FILE=${CMAKE_MAKE_PROGRAM}"
        .
        RESULT_VARIABLE result
        ${OUTPUT_QUIET}
        WORKING_DIRECTORY "${${PROJECT}_DOWNLOAD_DIR}"
    )
    if(result)
        message(FATAL_ERROR "CMake step for ${PROJECT} failed: ${result}")
    endif()

    execute_process(COMMAND ${CMAKE_COMMAND}
        --build .
        RESULT_VARIABLE result
        ${OUTPUT_QUIET}
        WORKING_DIRECTORY "${${PROJECT}_DOWNLOAD_DIR}"
    )
    if(result)
        message(FATAL_ERROR "Build step for ${PROJECT} failed: ${result}")
    endif()
endfunction(download_project PROJECT)

function(install_project PROJECT INSTALL_DIR)
    # Install the project
    if("{INSTALL_DIR}" STREQUAL "")
        execute_process(COMMAND ${CMAKE_COMMAND}
            -H${${PROJECT}_SOURCE_DIR}
            -B${${PROJECT}_BINARY_DIR}
            RESULT_VARIABLE result
            ${OUTPUT_QUIET}
            WORKING_DIRECTORY "${${PROJECT}_DOWNLOAD_DIR}"
        )
    else ("{INSTALL_DIR}" STREQUAL "")
        execute_process(COMMAND ${CMAKE_COMMAND}
            -H${${PROJECT}_SOURCE_DIR}
            -B${${PROJECT}_BINARY_DIR}
            -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR}
            RESULT_VARIABLE result
            ${OUTPUT_QUIET}
            WORKING_DIRECTORY "${${PROJECT}_DOWNLOAD_DIR}"
        )
    endif("{INSTALL_DIR}" STREQUAL "")
    if(result)
        message(FATAL_ERROR "Pre-install step for ${PROJECT} failed: ${result}")
    endif()

    # need to get root privileges to install project
    execute_process(COMMAND sudo cmake
        --build ${${PROJECT}_BINARY_DIR}
        --target install
        RESULT_VARIABLE result
        ${OUTPUT_QUIET}
        WORKING_DIRECTORY "${${PROJECT}_DOWNLOAD_DIR}"
    )
    if(result)
        message(FATAL_ERROR "Install step for ${PROJECT} failed: ${result}")
    endif()
    set (${PROJECT}_INSTALLED ON CACHE BOOL "Denote the library installation state" FORCE)
endfunction(install_project PROJECT)
