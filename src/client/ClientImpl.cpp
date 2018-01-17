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

#include "client/ClientImpl.h"

#include "boost/shared_ptr.hpp"

#include "base/ThreadPoolInitializer.h"

namespace QS {

namespace Client {

using boost::shared_ptr;

// --------------------------------------------------------------------------
ClientImpl::ClientImpl(const shared_ptr<QS::Threading::ThreadPool> &executor)
    : m_executor(executor) {
  QS::Threading::ThreadPoolInitializer::Instance().Register(m_executor.get());
}

// --------------------------------------------------------------------------
ClientImpl::~ClientImpl() {
  // do nothing
}

}  // namespace Client
}  // namespace QS
