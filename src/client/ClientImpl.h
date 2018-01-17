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

#ifndef QSFS_CLIENT_CLIENTIMPL_H_
#define QSFS_CLIENT_CLIENTIMPL_H_

#include "boost/shared_ptr.hpp"

#include "base/ThreadPool.h"
#include "client/ClientConfiguration.h"

namespace QS {

namespace Client {

class ClientImpl {
 public:
  ClientImpl(const boost::shared_ptr<QS::Threading::ThreadPool> &executor =
                 boost::shared_ptr<QS::Threading::ThreadPool>(
                     new QS::Threading::ThreadPool(
                         ClientConfiguration::Instance().GetPoolSize())));

  virtual ~ClientImpl();

 protected:
  const boost::shared_ptr<QS::Threading::ThreadPool> &GetExecutor() const {
    return m_executor;
  }

 private:
  boost::shared_ptr<QS::Threading::ThreadPool> m_executor;
};

}  // namespace Client
}  // namespace QS

#endif  // QSFS_CLIENT_CLIENTIMPL_H_
