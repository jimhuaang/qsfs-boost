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

#ifndef INCLUDE_BASE_THREADPOOL_H_
#define INCLUDE_BASE_THREADPOOL_H_

#include <stddef.h>

#include <functional>
#include <list>
//#include <type_traits>
#include <utility>
#include <vector>

#include "boost/function.hpp"
#include "boost/noncopyable.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/mutex.hpp"

namespace QS {

namespace Threading {

class TaskHandle;
class ThreadPoolInitializer;

typedef boost::function<void()> Task;

class ThreadPool : private boost::noncopyable {
 public:
  explicit ThreadPool(size_t poolSize);
  ~ThreadPool();

 public:
  void SubmitToThread(const Task &task, bool prioritized = false);

  //template <typename F, typename... Args>
  //void Submit(F &&f, Args &&... args);
//
  //template <typename F, typename... Args>
  //void SubmitPrioritized(F &&f, Args &&... args);
//
  //template <typename F, typename... Args>
  //auto SubmitCallable(F &&f, Args &&... args)
  //    -> std::future<typename std::result_of<F(Args...)>::type>;
//
  //template <typename F, typename... Args>
  //auto SubmitCallablePrioritized(F &&f, Args &&... args)
  //    -> std::future<typename std::result_of<F(Args...)>::type>;
//
  //template <typename ReceivedHandler, typename F, typename... Args>
  //void SubmitAsync(ReceivedHandler &&handler, F &&f, Args &&... args);
//
  //template <typename ReceivedHandler, typename F, typename... Args>
  //void SubmitAsyncPrioritized(ReceivedHandler &&handler, F &&f,
  //                            Args &&... args);
//
  //template <typename ReceivedHandler, typename CallerContext, typename F,
  //          typename... Args>
  //void SubmitAsyncWithContext(ReceivedHandler &&handler,
  //                            CallerContext &&context, F &&f, Args &&... args);
//
  //template <typename ReceivedHandler, typename CallerContext, typename F,
  //          typename... Args>
  //void SubmitAsyncWithContextPrioritized(ReceivedHandler &&handler,
  //                                       CallerContext &&context, F &&f,
  //                                       Args &&... args);

 private:
  Task* PopTask();
  bool HasTasks();

  // Initialize create needed TaskHandlers (worker thread)
  // Normally, this should only get called once
  void Initialize();

  // This is intended for a interrupt test only, do not use this
  // except in destructor. After this has been called once, all tasks
  // will never been handled since then.
  void StopProcessing();

 private:
  size_t m_poolSize;
  std::list<Task*> m_tasks;
  boost::mutex m_queueLock;
  std::vector<TaskHandle*> m_taskHandles;
  boost::mutex m_syncLock;
  boost::condition_variable m_syncConditionVar;

  friend class TaskHandle;
  friend class ThreadPoolInitializer;
  friend class ThreadPoolTest;
};

//template <typename F, typename... Args>
//void ThreadPool::Submit(F &&f, Args &&... args) {
//  return SubmitToThread(
//      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
//}
//
//template <typename F, typename... Args>
//void ThreadPool::SubmitPrioritized(F &&f, Args &&... args) {
//  auto fun = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
//  return SubmitToThread(fun, true);
//}
//
//template <typename F, typename... Args>
//auto ThreadPool::SubmitCallable(F &&f, Args &&... args)
//    -> std::future<typename std::result_of<F(Args...)>::type> {
//  using ReturnType = typename std::result_of<F(Args...)>::type;
//
//  auto task = std::make_shared<std::packaged_task<ReturnType()>>(
//      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
//
//  std::future<ReturnType> res = task->get_future();
//  {
//    std::lock_guard<std::mutex> lock(m_queueLock);
//    m_tasks.emplace_back([task]() { (*task)(); });
//  }
//  m_syncConditionVar.notify_one();
//  return res;
//}
//
//template <typename F, typename... Args>
//auto ThreadPool::SubmitCallablePrioritized(F &&f, Args &&... args)
//    -> std::future<typename std::result_of<F(Args...)>::type> {
//  using ReturnType = typename std::result_of<F(Args...)>::type;
//
//  auto task = std::make_shared<std::packaged_task<ReturnType()>>(
//      std::bind(std::forward<F>(f), std::forward<Args>(args)...));
//
//  std::future<ReturnType> res = task->get_future();
//  {
//    std::lock_guard<std::mutex> lock(m_queueLock);
//    m_tasks.emplace_front([task]() { (*task)(); });
//  }
//  m_syncConditionVar.notify_one();
//  return res;
//}
//
//template <typename ReceivedHandler, typename F, typename... Args>
//void ThreadPool::SubmitAsync(ReceivedHandler &&handler, F &&f,
//                             Args &&... args) {
//  auto task =
//      std::bind(std::forward<ReceivedHandler>(handler),
//                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
//                std::forward<Args>(args)...);
//  {
//    std::lock_guard<std::mutex> lock(m_queueLock);
//    m_tasks.emplace_back([task]() { task(); });
//  }
//  m_syncConditionVar.notify_one();
//}
//
//template <typename ReceivedHandler, typename F, typename... Args>
//void ThreadPool::SubmitAsyncPrioritized(ReceivedHandler &&handler, F &&f,
//                                        Args &&... args) {
//  auto task =
//      std::bind(std::forward<ReceivedHandler>(handler),
//                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
//                std::forward<Args>(args)...);
//  {
//    std::lock_guard<std::mutex> lock(m_queueLock);
//    m_tasks.emplace_front([task]() { task(); });
//  }
//  m_syncConditionVar.notify_one();
//}
//
//template <typename ReceivedHandler, typename CallerContext, typename F,
//          typename... Args>
//void ThreadPool::SubmitAsyncWithContext(ReceivedHandler &&handler,
//                                        CallerContext &&context, F &&f,
//                                        Args &&... args) {
//  auto task =
//      std::bind(std::forward<ReceivedHandler>(handler),
//                std::forward<CallerContext>(context),
//                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
//                std::forward<Args>(args)...);
//  {
//    std::lock_guard<std::mutex> lock(m_queueLock);
//    m_tasks.emplace_back([task]() { task(); });
//  }
//  m_syncConditionVar.notify_one();
//}
//
//template <typename ReceivedHandler, typename CallerContext, typename F,
//          typename... Args>
//void ThreadPool::SubmitAsyncWithContextPrioritized(ReceivedHandler &&handler,
//                                                   CallerContext &&context,
//                                                   F &&f, Args &&... args) {
//  auto task =
//      std::bind(std::forward<ReceivedHandler>(handler),
//                std::forward<CallerContext>(context),
//                std::bind(std::forward<F>(f), std::forward<Args>(args)...),
//                std::forward<Args>(args)...);
//  {
//    std::lock_guard<std::mutex> lock(m_queueLock);
//    m_tasks.emplace_front([task]() { task(); });
//  }
//  m_syncConditionVar.notify_one();
//}

}  // namespace Threading
}  // namespace QS


#endif  // INCLUDE_BASE_THREADPOOL_H_
