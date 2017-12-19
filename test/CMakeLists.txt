if (BUILD_TESTING)
  set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_LIST_DIR}/bin)

  add_executable(
    ExceptionTest
    ExceptionTest.cpp
  )
  target_link_libraries(ExceptionTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_exception COMMAND ExceptionTest)

  add_executable(
    HashUtilsTest
    HashUtilsTest.cpp
  )
  target_link_libraries(HashUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_hashutils COMMAND HashUtilsTest)

  add_executable(
    LogLevelTest
    LogLevelTest.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
    ${QSFS_SOURCE_DIR}/base/LogLevel.cpp
  )
  target_link_libraries(LogLevelTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_loglevel COMMAND LogLevelTest)

  add_executable(
    LoggingTest
    LoggingTest.cpp
    ${QSFS_SOURCE_DIR}/base/Logging.cpp
    ${QSFS_SOURCE_DIR}/base/LogLevel.cpp
    ${QSFS_SOURCE_DIR}/base/Utils.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
    ${QSFS_SOURCE_DIR}/configure/Default.cpp
  )
  target_link_libraries(LoggingTest gtest glog gflags boost_thread ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_logging COMMAND LoggingTest)

  add_executable(
    StringUtilsTest
    StringUtilsTest.cpp
    ${QSFS_SOURCE_DIR}/base/StringUtils.cpp
  )
  target_link_libraries(StringUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_stringutils COMMAND StringUtilsTest)

  add_executable(
    ThreadPoolTest
    ThreadPoolTest.cpp
    ${QSFS_SOURCE_DIR}/base/ThreadPool.cpp
    ${QSFS_SOURCE_DIR}/base/TaskHandle.cpp
  )
  target_link_libraries(ThreadPoolTest gtest boost_thread ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_threadpool COMMAND ThreadPoolTest)

  add_executable(
    TimeUtilsTest
    TimeUtilsTest.cpp
    ${QSFS_SOURCE_DIR}/base/TimeUtils.cpp
    #${QSFS_SOURCE_DIR}/base/LogLevel.cpp
    #${QSFS_SOURCE_DIR}/base/Logging.cpp
    #${QSFS_SOURCE_DIR}/base/Utils.cpp
    #${QSFS_SOURCE_DIR}/base/StringUtils.cpp
    #${QSFS_SOURCE_DIR}/configure/Default.cpp
  )
  #target_link_libraries(TimeUtilsTest gtest glog gflags boost_thread ${CMAKE_THREAD_LIBS_INIT})
  target_link_libraries(TimeUtilsTest gtest ${CMAKE_THREAD_LIBS_INIT})
  add_test(NAME qsfs_timeutils COMMAND TimeUtilsTest)

endif (BUILD_TESTING)