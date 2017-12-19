// +-------------------------------------------------------------------------
// | Copyright (C) 2017 Yunify, Inc.
// +-------------------------------------------------------------------------
// | Licensed under the Apache License, Version 2.0 (the "License");
// | You may not use this work except in compliance with the License.
// | You may obtain a copy of the License in the LICENSE file, or at:
// |
// | http://www.apache.org/licenses/LICENSE-2.0
// |
// | Unless required by applicable law or agreed to in writing, software
// | distributed under the License is distributed on an "AS IS" BASIS,
// | WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// | See the License for the specific language governing permissions and
// | limitations under the License.
// +-------------------------------------------------------------------------

#ifndef INCLUDE_BASE_TASKHANDLE_H_
#define INCLUDE_BASE_TASKHANDLE_H_

#include "boost/noncopyable.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/thread/thread.hpp"

namespace QS {

namespace Threading {

class ThreadPool;

class TaskHandle : private boost::noncopyable {
 public:
  explicit TaskHandle(ThreadPool &threadPool);  // NOLINT
  ~TaskHandle();

 private:
  void Stop();
  bool ShouldContinue();
  void operator()();
  bool Predicate();

 private:
  bool m_continue;
  boost::mutex m_continueLock;
  ThreadPool &m_threadPool;
  boost::thread m_thread;

  friend class ThreadPool;
};

}  // namespace Threading
}  // namespace QS


#endif  // INCLUDE_BASE_TASKHANDLE_H_
