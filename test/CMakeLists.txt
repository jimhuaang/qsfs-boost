if (BUILD_TESTING)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/bin)

  add_executable(
    ExceptionTest
    ExceptionTest.cpp
  )
  target_link_libraries(ExceptionTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_exception COMMAND ExceptionTest)

  add_executable(
    StringUtilsTest
    StringUtilsTest.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
  )
  target_link_libraries(StringUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_stringutils COMMAND StringUtilsTest)

  add_executable(
    LogLevelTest
    LogLevelTest.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
    ${QSFS_SOURCE_DIR}/base/LogLevel.cpp
  )
  target_link_libraries(LogLevelTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_loglevel COMMAND LogLevelTest)

  add_executable(
    HashUtilsTest
    HashUtilsTest.cpp
  )
  target_link_libraries(HashUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_hashutils COMMAND HashUtilsTest)

endif (BUILD_TESTING)