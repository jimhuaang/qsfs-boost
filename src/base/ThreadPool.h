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

#ifndef QSFS_BASE_THREADPOOL_H_
#define QSFS_BASE_THREADPOOL_H_

#include <stddef.h>

#include <list>
#include <utility>
#include <vector>

#include "boost/bind.hpp"
#include "boost/function.hpp"
#include "boost/make_shared.hpp"
#include "boost/move/move.hpp"
#include "boost/noncopyable.hpp"
#include "boost/preprocessor.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/thread/condition_variable.hpp"
#include "boost/thread/future.hpp"
#include "boost/thread/locks.hpp"
#include "boost/thread/mutex.hpp"
#include "boost/typeof/typeof.hpp"
#include "boost/utility/result_of.hpp"

//
// Macros for emulating Variadic Template in C++03
//
#define NUM_PARAMETERS 3
#define TYPES(Z, N, D) \
  BOOST_PP_COMMA_IF(N) \
  BOOST_PP_CAT(A, N)
#define PARAMETERS(Z, N, D) \
  BOOST_PP_COMMA_IF(N)      \
  BOOST_FWD_REF(BOOST_PP_CAT(A, N)) BOOST_PP_CAT(a, N)
#define FORWARD(Z, N, D) \
  BOOST_PP_COMMA_IF(N)   \
  boost::forward<BOOST_PP_CAT(A, N)>(BOOST_PP_CAT(a, N))


namespace QS {

namespace Threading {

//
// Generate PackageFunctor1, ..., PackageFunctorN
// N = NUM_PARAMETERS
//
#define EXPAND(N)                                                            \
  template <typename F, BOOST_PP_ENUM_PARAMS(N, typename A)>                 \
  struct BOOST_PP_CAT(PackageFunctor, N) {                                   \
    typedef typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type \
        ReturnType;                                                          \
    boost::shared_ptr<boost::packaged_task<ReturnType> > m_task;             \
    BOOST_PP_CAT(PackageFunctor, N)                                          \
    (const boost::shared_ptr<boost::packaged_task<ReturnType> >& task)       \
        : m_task(task) {}                                                    \
    void operator()() { (*m_task)(); }                                       \
  };

#define BOOST_PP_LOCAL_MACRO(N) EXPAND(N)
#define BOOST_PP_LOCAL_LIMITS (1, NUM_PARAMETERS)  // starting from 1
#include BOOST_PP_LOCAL_ITERATE()

#undef BOOST_PP_LOCAL_MACRO
#undef BOOST_PP_LOCAL_LIMITS
#undef EXPAND


class TaskHandle;
class ThreadPoolInitializer;

typedef boost::function<void()> Task;


class ThreadPool : private boost::noncopyable {
 public:
  explicit ThreadPool(size_t poolSize);
  ~ThreadPool();

 public:
  void SubmitToThread(const Task& task, bool prioritized = false);

  //template <typename F, typename A0>
  //boost::unique_future<typename boost::result_of<F(A0)>::type> SubmitCallable1(
  //    F f, BOOST_FWD_REF(A0) a0) {
  //  typedef typename boost::result_of<F(A0)>::type ReturnType;
  //  boost::shared_ptr<boost::packaged_task<ReturnType> > task =
  //      boost::make_shared<boost::packaged_task<ReturnType> >(
  //          boost::bind(boost::type<ReturnType>(), f, boost::forward<A0>(a0)));
  //  SubmitToThread(boost::bind<void>(Functor<F, A0>(task)));
  //  return task->get_future();
  //}

//
// Perfect Forward and Variadic Template Emulation in C++03
//
#define EXPAND(N)                                                             \
  template <typename F, BOOST_PP_ENUM_PARAMS(N, typename A)>                  \
  void Submit(F f, BOOST_PP_REPEAT(N, PARAMETERS, ~)) {                       \
    typedef typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type  \
        ReturnType;                                                           \
    return SubmitToThread(boost::bind(boost::type<ReturnType>(), f,           \
                                      BOOST_PP_REPEAT(N, FORWARD, ~)));       \
  }                                                                           \
                                                                              \
  template <typename F, BOOST_PP_ENUM_PARAMS(N, typename A)>                  \
  void SubmitPrioritized(F f, BOOST_PP_REPEAT(N, PARAMETERS, ~)) {            \
    typedef typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type  \
        ReturnType;                                                           \
    return SubmitToThread(boost::bind(boost::type<ReturnType>(), f,           \
                                      BOOST_PP_REPEAT(N, FORWARD, ~)),        \
                          true);                                              \
  }                                                                           \
                                                                              \
  template <typename F, BOOST_PP_ENUM_PARAMS(N, typename A)>                  \
  boost::unique_future<                                                       \
      typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type>       \
  SubmitCallable(F f, BOOST_PP_REPEAT(N, PARAMETERS, ~)) {                    \
    typedef typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type  \
        ReturnType;                                                           \
    boost::shared_ptr<boost::packaged_task<ReturnType> > task =               \
        boost::make_shared<boost::packaged_task<ReturnType> >(boost::bind(    \
            boost::type<ReturnType>(), f, BOOST_PP_REPEAT(N, FORWARD, ~)));   \
    SubmitToThread(boost::bind(boost::type<void>(),                           \
                               BOOST_PP_CAT(PackageFunctor, N) < F,           \
                               BOOST_PP_REPEAT(N, TYPES, ~) > (task)));       \
    return task->get_future();                                                \
  }                                                                           \
                                                                              \
  template <typename F, BOOST_PP_ENUM_PARAMS(N, typename A)>                  \
  boost::unique_future<                                                       \
      typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type>       \
  SubmitCallablePrioritized(F f, BOOST_PP_REPEAT(N, PARAMETERS, ~)) {         \
    typedef typename boost::result_of<F(BOOST_PP_REPEAT(N, TYPES, ~))>::type  \
        ReturnType;                                                           \
    boost::shared_ptr<boost::packaged_task<ReturnType> > task =               \
        boost::make_shared<boost::packaged_task<ReturnType> >(boost::bind(    \
            boost::type<ReturnType>(), f, BOOST_PP_REPEAT(N, FORWARD, ~)));   \
    SubmitToThread(                                                           \
        boost::bind(boost::type<void>(), BOOST_PP_CAT(PackageFunctor, N) < F, \
                    BOOST_PP_REPEAT(N, TYPES, ~) > (task)),                   \
        true);                                                                \
    return task->get_future();                                                \
  }

#define BOOST_PP_LOCAL_MACRO(N) EXPAND(N)
#define BOOST_PP_LOCAL_LIMITS (1, NUM_PARAMETERS)
#include BOOST_PP_LOCAL_ITERATE()

#undef BOOST_PP_LOCAL_MACRO
#undef BOOST_PP_LOCAL_LIMITS
#undef EXPAND


  // template <typename ReceivedHandler, typename F, typename... Args>
  // void SubmitAsync(ReceivedHandler &&handler, F &&f, Args &&... args);
  //
  // template <typename ReceivedHandler, typename F, typename... Args>
  // void SubmitAsyncPrioritized(ReceivedHandler &&handler, F &&f,
  //                            Args &&... args);
  //

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

//
// template <typename ReceivedHandler, typename F, typename... Args>
// void ThreadPool::SubmitAsync(ReceivedHandler &&handler, F &&f,
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
// template <typename ReceivedHandler, typename F, typename... Args>
// void ThreadPool::SubmitAsyncPrioritized(ReceivedHandler &&handler, F &&f,
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


}  // namespace Threading
}  // namespace QS

#undef FORWARD
#undef PARAMETERS
#undef TYPES
#undef NUM_PARAMETERS

#endif  // QSFS_BASE_THREADPOOL_H_


