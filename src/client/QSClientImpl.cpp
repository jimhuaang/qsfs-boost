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

#include "client/QSClientImpl.h"

#include <assert.h>

#include <string>
#include <utility>
#include <vector>

#include "boost/bind.hpp"
#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/thread.hpp"
#include "boost/thread/future.hpp"
#include "boost/typeof/typeof.hpp"

#include "qingstor/Bucket.h"
#include "qingstor/HttpCommon.h"
#include "qingstor/QingStor.h"
#include "qingstor/QsConfig.h"
#include "qingstor/QsErrors.h"  // for sdk QsError
#include "qingstor/Types.h"     // for sdk QsOutput

#include "base/LogMacros.h"
#include "base/ThreadPool.h"
#include "client/ClientConfiguration.h"
#include "client/QSClient.h"
#include "client/QSError.h"
#include "client/Utils.h"

namespace QS {

namespace Client {

using boost::bind;
using boost::make_shared;
using boost::packaged_task;
using boost::posix_time::milliseconds;
using boost::shared_ptr;
using boost::unique_future;
using QingStor::AbortMultipartUploadInput;
using QingStor::AbortMultipartUploadOutput;
using QingStor::Bucket;
using QingStor::CompleteMultipartUploadInput;
using QingStor::CompleteMultipartUploadOutput;
using QingStor::DeleteObjectInput;
using QingStor::DeleteObjectOutput;
using QingStor::GetBucketStatisticsInput;
using QingStor::GetBucketStatisticsOutput;
using QingStor::GetObjectInput;
using QingStor::GetObjectOutput;
using QingStor::HeadBucketInput;
using QingStor::HeadBucketOutput;
using QingStor::HeadObjectInput;
using QingStor::HeadObjectOutput;
using QingStor::Http::HttpResponseCode;
using QingStor::InitiateMultipartUploadInput;
using QingStor::InitiateMultipartUploadOutput;
using QingStor::ListObjectsInput;
using QingStor::ListObjectsOutput;
using QingStor::PutObjectInput;
using QingStor::PutObjectOutput;
using QingStor::QsOutput;
using QingStor::UploadMultipartInput;
using QingStor::UploadMultipartOutput;
using QS::Client::Utils::ParseRequestContentRange;
using QS::Client::Utils::ParseResponseContentRange;
using std::make_pair;
using std::pair;
using std::string;
using std::vector;

namespace {

// --------------------------------------------------------------------------
ClientError<QSError::Value> BuildQSError(QsError sdkErr,
                                         const string &exceptionName,
                                         const QsOutput &output,
                                         bool retriable) {
  // QS_ERR_NO_ERROR only says the request has sent, but it desen't not mean
  // response code is ok
  HttpResponseCode rspCode = const_cast<QsOutput &>(output).GetResponseCode();
  QSError::Value err = SDKResponseToQSError(rspCode);
  if (err == QSError::UNKNOWN) {
    err = SDKErrorToQSError(sdkErr);
  }

  if(sdkErr == QS_ERR_UNEXCEPTED_RESPONSE) {
    QingStor::ResponseErrorInfo errInfo = output.GetResponseErrInfo();
    string errMsg;
    errMsg += "[code:" + errInfo.code;
    errMsg += "; message:" + errInfo.message;
    errMsg += "; request:" + errInfo.requestID;
    errMsg += "; url:" + errInfo.url;
    errMsg += "]";
    return ClientError<QSError::Value>(err, exceptionName, errMsg, retriable);
  } else {
    return ClientError<QSError::Value>(err, exceptionName, 
        SDKResponseCodeToString(rspCode), retriable);
  }
}

// --------------------------------------------------------------------------
ClientError<QSError::Value> TimeOutError(const string &exceptionName,
                                         boost::future_state::state status) {
  ClientError<QSError::Value> err;
  switch (status) {
    case boost::future_state::uninitialized:
      // request timeout is retryable
      err = ClientError<QSError::Value>(
          QSError::REQUEST_UNINITIALIZED, exceptionName,
          QSErrorToString(QSError::REQUEST_UNINITIALIZED), true);
      break;
    case boost::future_state::waiting:
      err = ClientError<QSError::Value>(
          QSError::REQUEST_WAITING, exceptionName,
          QSErrorToString(QSError::REQUEST_WAITING), false);
      break;
    case boost::future_state::ready:  // Bypass
    default:
      err = ClientError<QSError::Value>(QSError::GOOD, exceptionName,
                                        QSErrorToString(QSError::GOOD), false);
      break;
  }

  return err;
}

}  // namespace

// --------------------------------------------------------------------------
QSClientImpl::QSClientImpl() : ClientImpl() {
  if (!m_bucket) {
    const ClientConfiguration &clientConfig = ClientConfiguration::Instance();
    const shared_ptr<QingStor::QsConfig> &qsConfig =
        QSClient::GetQingStorConfig();
    m_bucket = shared_ptr<Bucket>(new Bucket(
        *qsConfig, clientConfig.GetBucket(), clientConfig.GetZone()));
  }
}

// --------------------------------------------------------------------------
pair<QsError, GetBucketStatisticsOutput> DoGetBucketStatistics(
    const shared_ptr<Bucket> &bucket) {
  GetBucketStatisticsInput input;  // dummy input
  GetBucketStatisticsOutput output;

  QsError sdkErr = bucket->GetBucketStatistics(input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
GetBucketStatisticsOutcome QSClientImpl::GetBucketStatistics(
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorGetBucketStatistics";

  unique_future<pair<QsError, GetBucketStatisticsOutput> >
      fGetBucketStatistics = GetExecutor()->SubmitCallablePrioritized(
          DoGetBucketStatistics, m_bucket);
  fGetBucketStatistics.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fGetBucketStatistics.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, GetBucketStatisticsOutput> res = fGetBucketStatistics.get();
    QsError sdkErr = res.first;
    GetBucketStatisticsOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return GetBucketStatisticsOutcome(output);
    } else {
      return GetBucketStatisticsOutcome(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode)));
    }
  } else {
    return GetBucketStatisticsOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, HeadBucketOutput> DoHeadBucket(const shared_ptr<Bucket> &bucket) {
  HeadBucketInput input;  // dummy input
  HeadBucketOutput output;
  QsError sdkErr = bucket->HeadBucket(input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
HeadBucketOutcome QSClientImpl::HeadBucket(uint32_t msTimeDuration,
                                           bool useThreadPool) const {
  string exceptionName = "QingStorHeadBucket";

  unique_future<pair<QsError, HeadBucketOutput> > fHeadBucket;
  if (useThreadPool) {
    fHeadBucket =
        GetExecutor()->SubmitCallablePrioritized(DoHeadBucket, m_bucket);
  } else {
    shared_ptr<packaged_task<pair<QsError, HeadBucketOutput> > > task =
        make_shared<packaged_task<pair<QsError, HeadBucketOutput> > >(
            bind(boost::type<pair<QsError, HeadBucketOutput> >(), DoHeadBucket,
                 m_bucket));
    fHeadBucket = task->get_future();
    boost::thread t(boost::bind<void>(
        QS::Threading::PackageFunctor1<BOOST_TYPEOF(&DoHeadBucket),
                                       const shared_ptr<Bucket> &>(task)));
    t.detach();
  }

  fHeadBucket.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fHeadBucket.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, HeadBucketOutput> res = fHeadBucket.get();
    QsError sdkErr = res.first;
    HeadBucketOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return HeadBucketOutcome(output);
    } else {
      return HeadBucketOutcome(BuildQSError(sdkErr, exceptionName, output,
                                            SDKShouldRetry(responseCode)));
    }
  } else {
    return HeadBucketOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, ListObjectsOutput> DoListObjects(const shared_ptr<Bucket> &bucket,
                                               ListObjectsInput *input) {
  ListObjectsOutput output;
  QsError sdkErr = bucket->ListObjects(*input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
ListObjectsOutcome QSClientImpl::ListObjects(
    ListObjectsInput *input, bool *resultTruncated, uint64_t *resCount,
    uint64_t maxCount, uint32_t msTimeDuration, bool useThreadPool) const {
  string exceptionName = "QingStorListObjects";
  if (input == NULL) {
    return ListObjectsOutcome(
        ClientError<QSError::Value>(QSError::PARAMETER_MISSING, exceptionName,
                                    "Null ListObjectsInput", false));
  }
  exceptionName.append(" prefix=");
  exceptionName.append(input->GetPrefix());

  if (input->GetLimit() <= 0) {
    return ListObjectsOutcome(ClientError<QSError::Value>(
        QSError::NO_SUCH_LIST_OBJECTS, exceptionName,
        "ListObjectsInput with negative or zero count limit", false));
  }

  if (resultTruncated != NULL) {
    *resultTruncated = false;
  }
  if (resCount != NULL) {
    *resCount = 0;
  }

  bool listAllObjects = maxCount == 0;
  uint64_t count = 0;
  bool responseTruncated = true;
  vector<ListObjectsOutput> result;
  do {
    if (!listAllObjects) {
      int remainingCount = static_cast<int>(maxCount - count);
      if (remainingCount < input->GetLimit()) {
        input->SetLimit(remainingCount);
      }
    }

    unique_future<pair<QsError, ListObjectsOutput> > fListObjects;
    if (useThreadPool) {
      fListObjects = GetExecutor()->SubmitCallablePrioritized(DoListObjects,
                                                              m_bucket, input);
    } else {
      shared_ptr<packaged_task<pair<QsError, ListObjectsOutput> > > task =
          make_shared<packaged_task<pair<QsError, ListObjectsOutput> > >(
              bind(boost::type<pair<QsError, ListObjectsOutput> >(),
                   DoListObjects, m_bucket, input));
      fListObjects = task->get_future();
      boost::thread t(boost::bind<void>(
          QS::Threading::PackageFunctor2<BOOST_TYPEOF(&DoListObjects),
                                         const shared_ptr<Bucket> &,
                                         ListObjectsInput *>(task)));
      t.detach();
    }
    fListObjects.timed_wait(milliseconds(msTimeDuration));
    boost::future_state::state fState = fListObjects.get_state();
    if (fState == boost::future_state::ready) {
      pair<QsError, ListObjectsOutput> res = fListObjects.get();
      QsError sdkErr = res.first;
      ListObjectsOutput &output = res.second;

      HttpResponseCode responseCode = output.GetResponseCode();
      if (SDKResponseSuccess(sdkErr, responseCode)) {
        count += output.GetKeys().size();
        count += output.GetCommonPrefixes().size();
        responseTruncated = !output.GetNextMarker().empty();
        if (responseTruncated) {
          input->SetMarker(output.GetNextMarker());
        }
        result.push_back(output);
      } else {
        return ListObjectsOutcome(BuildQSError(sdkErr, exceptionName, output,
                                               SDKShouldRetry(responseCode)));
      }
    } else {
      return ListObjectsOutcome(TimeOutError(exceptionName, fState));
    }
  } while (responseTruncated && (listAllObjects || count < maxCount));
  if (resultTruncated != NULL) {
    *resultTruncated = responseTruncated;
  }
  if (resCount != NULL) {
    *resCount = count;
  }
  return ListObjectsOutcome(result);
}

// --------------------------------------------------------------------------
pair<QsError, DeleteObjectOutput> DoDeleteObject(
    const shared_ptr<Bucket> &bucket, const string &objKey) {
  DeleteObjectInput input;  // dummy input
  DeleteObjectOutput output;
  QsError sdkErr = bucket->DeleteObject(objKey, input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
DeleteObjectOutcome QSClientImpl::DeleteObject(const string &objKey,
                                               uint32_t msTimeDuration) const {
  string exceptionName = "QingStorDeleteObject";
  if (objKey.empty()) {
    return DeleteObjectOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName, "Empty ObjectKey", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, DeleteObjectOutput> > fDeleteObject =
      GetExecutor()->SubmitCallablePrioritized(DoDeleteObject, m_bucket,
                                               objKey);
  fDeleteObject.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fDeleteObject.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, DeleteObjectOutput> res = fDeleteObject.get();
    QsError sdkErr = res.first;
    DeleteObjectOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return DeleteObjectOutcome(output);
    } else {
      return DeleteObjectOutcome(BuildQSError(sdkErr, exceptionName, output,
                                              SDKShouldRetry(responseCode)));
    }
  } else {
    return DeleteObjectOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, GetObjectOutput> DoGetObject(const shared_ptr<Bucket> &bucket,
                                           const string &objKey,
                                           GetObjectInput *input) {
  GetObjectOutput output;
  QsError sdkErr = bucket->GetObject(objKey, *input, output);
  return make_pair(sdkErr, output);
}
// --------------------------------------------------------------------------
GetObjectOutcome QSClientImpl::GetObject(const string &objKey,
                                         GetObjectInput *input,
                                         uint32_t msTimeDuration) const {
  string exceptionName = "QingStorGetObject";
  if (objKey.empty() || input == NULL) {
    return GetObjectOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null GetObjectInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  bool askPartialContent = !input->GetRange().empty();

  unique_future<pair<QsError, GetObjectOutput> > fGetObject =
      GetExecutor()->SubmitCallablePrioritized(DoGetObject, m_bucket, objKey,
                                               input);
  fGetObject.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fGetObject.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, GetObjectOutput> res = fGetObject.get();
    QsError sdkErr = res.first;
    GetObjectOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      if (askPartialContent) {
        // qs sdk specification: if request set with range parameter, then
        // response successful with code 206 (Partial Content)
        if (output.GetResponseCode() != QingStor::Http::PARTIAL_CONTENT) {
          return GetObjectOutcome(
              BuildQSError(sdkErr, exceptionName, output, true));
        } else {
          size_t reqLen = ParseRequestContentRange(input->GetRange()).second;
          // auto rspRes = ParseResponseContentRange(output.GetContentRange());
          size_t rspLen = output.GetContentLength();
          DebugWarningIf(
              rspLen < reqLen,
              "[content range request:response=" + input->GetRange() + ":" +
                  output.GetContentRange() + "]");
        }
      }
      return GetObjectOutcome(output);
    } else {
      return GetObjectOutcome(BuildQSError(sdkErr, exceptionName, output,
                                           SDKShouldRetry(responseCode)));
    }
  } else {
    return GetObjectOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, HeadObjectOutput> DoHeadObject(const shared_ptr<Bucket> &bucket,
                                             const string &objKey,
                                             HeadObjectInput *input) {
  HeadObjectOutput output;
  QsError sdkErr = bucket->HeadObject(objKey, *input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
HeadObjectOutcome QSClientImpl::HeadObject(const string &objKey,
                                           HeadObjectInput *input,
                                           uint32_t msTimeDuration) const {
  string exceptionName = "QingStorHeadObject";
  if (objKey.empty() || input == NULL) {
    return HeadObjectOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null HeadObjectInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, HeadObjectOutput> > fHeadObject =
      GetExecutor()->SubmitCallablePrioritized(DoHeadObject, m_bucket, objKey,
                                               input);
  fHeadObject.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fHeadObject.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, HeadObjectOutput> res = fHeadObject.get();
    QsError sdkErr = res.first;
    HeadObjectOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return HeadObjectOutcome(output);
    } else {
      return HeadObjectOutcome(BuildQSError(sdkErr, exceptionName, output,
                                            SDKShouldRetry(responseCode)));
    }

  } else {
    return HeadObjectOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, PutObjectOutput> DoPutObject(const shared_ptr<Bucket> &bucket,
                                           const string &objKey,
                                           PutObjectInput *input) {
  PutObjectOutput output;
  QsError sdkErr = bucket->PutObject(objKey, *input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
PutObjectOutcome QSClientImpl::PutObject(const string &objKey,
                                         PutObjectInput *input,
                                         uint32_t msTimeDuration) const {
  string exceptionName = "QingStorPutObject";
  if (objKey.empty() || input == NULL) {
    return PutObjectOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null PutObjectInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, PutObjectOutput> > fPutObject =
      GetExecutor()->SubmitCallablePrioritized(DoPutObject, m_bucket, objKey,
                                               input);
  fPutObject.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fPutObject.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, PutObjectOutput> res = fPutObject.get();
    QsError sdkErr = res.first;
    PutObjectOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return PutObjectOutcome(output);
    } else {
      return PutObjectOutcome(BuildQSError(sdkErr, exceptionName, output,
                                           SDKShouldRetry(responseCode)));
    }
  } else {
    return PutObjectOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, InitiateMultipartUploadOutput> DoInitiateMultipartUpload(
    const shared_ptr<Bucket> &bucket, const string &objKey,
    InitiateMultipartUploadInput *input) {
  InitiateMultipartUploadOutput output;
  QsError sdkErr = bucket->InitiateMultipartUpload(objKey, *input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
InitiateMultipartUploadOutcome QSClientImpl::InitiateMultipartUpload(
    const string &objKey, InitiateMultipartUploadInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorInitiateMultipartUpload";
  if (objKey.empty() || input == NULL) {
    return InitiateMultipartUploadOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null InitiateMultipartUploadInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, InitiateMultipartUploadOutput> >
      fInitMultipartUpload = GetExecutor()->SubmitCallablePrioritized(
          DoInitiateMultipartUpload, m_bucket, objKey, input);
  fInitMultipartUpload.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fInitMultipartUpload.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, InitiateMultipartUploadOutput> res =
        fInitMultipartUpload.get();
    QsError sdkErr = res.first;
    InitiateMultipartUploadOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return InitiateMultipartUploadOutcome(output);
    } else {
      return InitiateMultipartUploadOutcome(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode)));
    }
  } else {
    return InitiateMultipartUploadOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, UploadMultipartOutput> DoUploadMultipart(
    const shared_ptr<Bucket> &bucket, const string &objKey,
    UploadMultipartInput *input) {
  UploadMultipartOutput output;
  QsError sdkErr = bucket->UploadMultipart(objKey, *input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
UploadMultipartOutcome QSClientImpl::UploadMultipart(
    const string &objKey, UploadMultipartInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorUploadMultipart";
  if (objKey.empty() || input == NULL) {
    return UploadMultipartOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null UploadMultipartInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, UploadMultipartOutput> > fUploadMultipart =
      GetExecutor()->SubmitCallablePrioritized(DoUploadMultipart, m_bucket,
                                               objKey, input);
  fUploadMultipart.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fUploadMultipart.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, UploadMultipartOutput> res = fUploadMultipart.get();
    QsError sdkErr = res.first;
    UploadMultipartOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return UploadMultipartOutcome(output);
    } else {
      return UploadMultipartOutcome(BuildQSError(sdkErr, exceptionName, output,
                                                 SDKShouldRetry(responseCode)));
    }
  } else {
    return UploadMultipartOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, CompleteMultipartUploadOutput> DoCompleteMultipartUpload(
    const shared_ptr<Bucket> &bucket, const string &objKey,
    CompleteMultipartUploadInput *input) {
  CompleteMultipartUploadOutput output;
  QsError sdkErr = bucket->CompleteMultipartUpload(objKey, *input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
CompleteMultipartUploadOutcome QSClientImpl::CompleteMultipartUpload(
    const string &objKey, CompleteMultipartUploadInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorCompleteMultipartUpload";
  if (objKey.empty() || input == NULL) {
    return CompleteMultipartUploadOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null CompleteMutlipartUploadInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, CompleteMultipartUploadOutput> >
      fCompleteMultipartUpload = GetExecutor()->SubmitCallablePrioritized(
          DoCompleteMultipartUpload, m_bucket, objKey, input);
  fCompleteMultipartUpload.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fCompleteMultipartUpload.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, CompleteMultipartUploadOutput> res =
        fCompleteMultipartUpload.get();
    QsError sdkErr = res.first;
    CompleteMultipartUploadOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return CompleteMultipartUploadOutcome(output);
    } else {
      return CompleteMultipartUploadOutcome(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode)));
    }
  } else {
    return CompleteMultipartUploadOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
pair<QsError, AbortMultipartUploadOutput> DoAbortMultipartUpload(
    const shared_ptr<Bucket> &bucket, const string &objKey,
    AbortMultipartUploadInput *input) {
  AbortMultipartUploadOutput output;
  QsError sdkErr = bucket->AbortMultipartUpload(objKey, *input, output);
  return make_pair(sdkErr, output);
}

// --------------------------------------------------------------------------
AbortMultipartUploadOutcome QSClientImpl::AbortMultipartUpload(
    const string &objKey, AbortMultipartUploadInput *input,
    uint32_t msTimeDuration) const {
  string exceptionName = "QingStorAbortMultipartUpload";
  if (objKey.empty() || input == NULL) {
    return AbortMultipartUploadOutcome(ClientError<QSError::Value>(
        QSError::PARAMETER_MISSING, exceptionName,
        "Empty ObjectKey or Null AbortMultipartUploadInput", false));
  }
  exceptionName.append(" object=");
  exceptionName.append(objKey);

  unique_future<pair<QsError, AbortMultipartUploadOutput> >
      fAbortMultipartUpload = GetExecutor()->SubmitCallablePrioritized(
          DoAbortMultipartUpload, m_bucket, objKey, input);
  fAbortMultipartUpload.timed_wait(milliseconds(msTimeDuration));
  boost::future_state::state fState = fAbortMultipartUpload.get_state();
  if (fState == boost::future_state::ready) {
    pair<QsError, AbortMultipartUploadOutput> res = fAbortMultipartUpload.get();
    QsError sdkErr = res.first;
    AbortMultipartUploadOutput &output = res.second;
    HttpResponseCode responseCode = output.GetResponseCode();
    if (SDKResponseSuccess(sdkErr, responseCode)) {
      return AbortMultipartUploadOutcome(output);
    } else {
      return AbortMultipartUploadOutcome(BuildQSError(
          sdkErr, exceptionName, output, SDKShouldRetry(responseCode)));
    }
  } else {
    return AbortMultipartUploadOutcome(TimeOutError(exceptionName, fState));
  }
}

// --------------------------------------------------------------------------
void QSClientImpl::SetBucket(const shared_ptr<Bucket> &bucket) {
  assert(bucket);
  FatalIf(!bucket, "Setting a NULL bucket!");
  m_bucket = bucket;
}

}  // namespace Client
}  // namespace QS
