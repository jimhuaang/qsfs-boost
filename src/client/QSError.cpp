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

#include "client/QSError.h"

#include "boost/exception/to_string.hpp"
#include "boost/unordered_map.hpp"
#include "boost/unordered_set.hpp"

#include "qingstor/HttpCommon.h"
#include "qingstor/QsErrors.h"

#include "base/HashUtils.h"

namespace QS {

namespace Client {

using boost::to_string;
using boost::unordered_map;
using boost::unordered_set;
using QingStor::Http::HttpResponseCode;
using std::string;

namespace {

bool SDKResponseCodeSuccess(HttpResponseCode code){
  using namespace QingStor::Http;
  static unordered_set<HttpResponseCode, QS::HashUtils::EnumHash> successCodeSet;
  successCodeSet.insert(CONTINUE);                       //"Continue"                    100
  successCodeSet.insert(SWITCHING_PROTOCOLS);            //"SwitchingProtocols"          101
  successCodeSet.insert(PROCESSING);                     //"Processing"                  102
  successCodeSet.insert(OK);                             //"Ok"                          200
  successCodeSet.insert(CREATED);                        //"Created"                     201
  successCodeSet.insert(ACCEPTED);                       //"Accepted"                    202
  successCodeSet.insert(NON_AUTHORITATIVE_INFORMATION);  //"NonAuthoritativeInformation" 203
  successCodeSet.insert(NO_CONTENT);                     //"NoContent"                   204
  successCodeSet.insert(RESET_CONTENT);                  //"ResetContent"                205
  successCodeSet.insert(PARTIAL_CONTENT);                //"PartialContent"              206
  successCodeSet.insert(MULTI_STATUS);                   //"MultiStatus"                 207
  successCodeSet.insert(ALREADY_REPORTED);               //"AlreadyReported"             208
  successCodeSet.insert(IM_USED);                        //"IMUsed"                      226
  successCodeSet.insert(FOUND);                          //"Found"                       302
  successCodeSet.insert(NOT_MODIFIED);                   //"NotModified"                 304
  unordered_set<HttpResponseCode, QS::HashUtils::EnumHash>::iterator it =
        successCodeSet.find(code);
  return it != successCodeSet.end();
}

}  // namespace


// --------------------------------------------------------------------------
QSError::Value StringToQSError(const string &errorCode) {
  static unordered_map<string, QSError::Value, QS::HashUtils::StringHash> map;
  map["Unknow"]                     = QSError::UNKNOWN;
  map["Good"]                       = QSError::GOOD;
  map["NoSuchListMultipart"]        = QSError::NO_SUCH_LIST_MULTIPART;
  map["NoSuchListMultipartUploads"] = QSError::NO_SUCH_LIST_MULTIPART_UPLOADS;
  map["NoSuchListObjects"]          = QSError::NO_SUCH_LIST_OBJECTS;
  map["NoSuchMultipartDownload"]    = QSError::NO_SUCH_MULTIPART_DOWNLOAD;
  map["NoSuchMultipartUpload"]      = QSError::NO_SUCH_MULTIPART_UPLOAD;
  map["NoSuchUpload"]               = QSError::NO_SUCH_UPLOAD;
  map["ParameterMissing"]           = QSError::PARAMETER_MISSING;
  map["RequestDeferred"]            = QSError::REQUEST_DEFERRED;
  map["RequestExpired"]             = QSError::REQUEST_EXPIRED;
  map["SDKConfigureFileInvalid"]    = QSError::SDK_CONFIGURE_FILE_INAVLID;
  map["SDKNoRequiredParameter"]     = QSError::SDK_NO_REQUIRED_PARAMETER;
  map["SDKRequestNotMade"]          = QSError::SDK_REQUEST_NOT_MADE;
  map["SDKRequestSendError"]        = QSError::SDK_REQUEST_SEND_ERROR;
  map["SDKUnexpectedResponse"]      = QSError::SDK_UNEXPECTED_RESPONSE;
  map["KeyNotExist"]                = QSError::KEY_NOT_EXIST;

  unordered_map<string, QSError::Value, QS::HashUtils::StringHash>::iterator it =
        map.find(errorCode);
  return it != map.end() ? it->second : QSError::UNKNOWN;
}

// --------------------------------------------------------------------------
std::string QSErrorToString(QSError::Value err){
  static unordered_map<QSError::Value, string, QS::HashUtils::EnumHash> map;
  map[QSError::UNKNOWN]                        = "Unknow";
  map[QSError::GOOD]                           = "Good";
  map[QSError::NO_SUCH_LIST_MULTIPART]         = "NoSuchListMultipart";
  map[QSError::NO_SUCH_LIST_MULTIPART_UPLOADS] = "NoSuchListMultipartUploads";
  map[QSError::NO_SUCH_LIST_OBJECTS]           = "NoSuchListObjects";
  map[QSError::NO_SUCH_MULTIPART_DOWNLOAD]     = "NoSuchMultipartDownload";
  map[QSError::NO_SUCH_MULTIPART_UPLOAD]       = "NoSuchMultipartUpload";
  map[QSError::NO_SUCH_UPLOAD]                 = "NoSuchUpload";
  map[QSError::PARAMETER_MISSING]              = "ParameterMissing";
  map[QSError::REQUEST_DEFERRED]               = "RequestDeferred";
  map[QSError::REQUEST_EXPIRED]                = "RequestExpired";
  map[QSError::SDK_CONFIGURE_FILE_INAVLID]     = "SDKConfigureFileInvalid";
  map[QSError::SDK_NO_REQUIRED_PARAMETER]      = "SDKNoRequiredParameter";
  map[QSError::SDK_REQUEST_NOT_MADE]           = "SDKRequestNotMade";
  map[QSError::SDK_REQUEST_SEND_ERROR]         = "SDKRequestSendError";
  map[QSError::SDK_UNEXPECTED_RESPONSE]        = "SDKUnexpectedResponse";
  map[QSError::KEY_NOT_EXIST]                  = "KeyNotExist";
  unordered_map<QSError::Value, string, QS::HashUtils::EnumHash>::iterator it =
        map.find(err);
  return it != map.end() ? it->second : "Unknow";
}

// --------------------------------------------------------------------------
ClientError<QSError::Value> GetQSErrorForCode(const string &errorCode) {
  return ClientError<QSError::Value>(StringToQSError(errorCode),false);
}

// --------------------------------------------------------------------------
std::string GetMessageForQSError(const ClientError<QSError::Value> &error) {
  return QSErrorToString(error.GetError()) + ", " + error.GetExceptionName() +
         ":" + error.GetMessage();
}

// --------------------------------------------------------------------------
bool IsGoodQSError(const ClientError<QSError::Value> &error){
  return error.GetError() == QSError::GOOD;
}

// --------------------------------------------------------------------------
QSError::Value SDKErrorToQSError(QsError sdkErr) {
  static unordered_map<QsError, QSError::Value, QS::HashUtils::EnumHash> sdkErrToQSErrMap;
  sdkErrToQSErrMap[QS_ERR_NO_ERROR]              = QSError::GOOD;
  sdkErrToQSErrMap[QS_ERR_INVAILD_CONFIG_FILE]   = QSError::SDK_CONFIGURE_FILE_INAVLID;
  sdkErrToQSErrMap[QS_ERR_NO_REQUIRED_PARAMETER] = QSError::PARAMETER_MISSING;
  sdkErrToQSErrMap[QS_ERR_SEND_REQUEST_ERROR]    = QSError::SDK_REQUEST_SEND_ERROR;
  sdkErrToQSErrMap[QS_ERR_UNEXCEPTED_RESPONSE]   = QSError::SDK_UNEXPECTED_RESPONSE;
  unordered_map<QsError, QSError::Value, QS::HashUtils::EnumHash>::iterator it =
        sdkErrToQSErrMap.find(sdkErr);
  return it != sdkErrToQSErrMap.end() ? it->second : QSError::UNKNOWN;
}

// --------------------------------------------------------------------------
QSError::Value SDKResponseToQSError(HttpResponseCode code) {
  using namespace QingStor::Http;
  static unordered_map<HttpResponseCode, QSError::Value, QS::HashUtils::EnumHash> map;
  map[REQUEST_NOT_MADE]                = QSError::SDK_REQUEST_NOT_MADE;
  map[CONTINUE]                        = QSError::GOOD;  // 100
  map[SWITCHING_PROTOCOLS]             = QSError::GOOD;  // 101
  map[PROCESSING]                      = QSError::GOOD;  // 102
  map[OK]                              = QSError::GOOD;  // 200
  map[CREATED]                         = QSError::GOOD;  // 201
  map[ACCEPTED]                        = QSError::GOOD;  // 202
  map[NON_AUTHORITATIVE_INFORMATION]   = QSError::GOOD;  // 203
  map[NO_CONTENT]                      = QSError::GOOD;  // 204
  map[RESET_CONTENT]                   = QSError::GOOD;  // 205
  map[PARTIAL_CONTENT]                 = QSError::GOOD;  // 206
  map[MULTI_STATUS]                    = QSError::GOOD;  // 207
  map[ALREADY_REPORTED]                = QSError::GOOD;  // 208
  map[IM_USED]                         = QSError::GOOD;  // 226
  map[MULTIPLE_CHOICES]                = QSError::SDK_UNEXPECTED_RESPONSE;  // 300;
  map[MOVED_PERMANENTLY]               = QSError::SDK_UNEXPECTED_RESPONSE;  // 301;
  map[FOUND]                           = QSError::GOOD;  // 302;
  map[SEE_OTHER]                       = QSError::SDK_UNEXPECTED_RESPONSE;  // 303;
  map[NOT_MODIFIED]                    = QSError::GOOD;  // 304;
  map[USE_PROXY]                       = QSError::SDK_UNEXPECTED_RESPONSE;  // 305;
  map[SWITCH_PROXY]                    = QSError::SDK_UNEXPECTED_RESPONSE;  // 306;
  map[TEMPORARY_REDIRECT]              = QSError::SDK_UNEXPECTED_RESPONSE;  // 307;
  map[PERMANENT_REDIRECT]              = QSError::SDK_UNEXPECTED_RESPONSE;  // 308;
  map[BAD_REQUEST]                     = QSError::SDK_UNEXPECTED_RESPONSE;  // 400;
  map[UNAUTHORIZED_OR_EXPIRED]         = QSError::SDK_UNEXPECTED_RESPONSE;  // 401;
  map[DELINQUENT_ACCOUNT]              = QSError::SDK_UNEXPECTED_RESPONSE;  // 402;
  map[FORBIDDEN]                       = QSError::SDK_UNEXPECTED_RESPONSE;  // 403;
  map[NOT_FOUND]                       = QSError::KEY_NOT_EXIST;  // 404;
  map[METHOD_NOT_ALLOWED]              = QSError::SDK_UNEXPECTED_RESPONSE;  // 405;
  map[CONFLICT]                        = QSError::SDK_UNEXPECTED_RESPONSE;  // 409;
  map[PRECONDITION_FAILED]             = QSError::SDK_UNEXPECTED_RESPONSE;  // 412;
  map[INVALID_RANGE]                   = QSError::SDK_UNEXPECTED_RESPONSE;  // 416;
  map[TOO_MANY_REQUESTS]               = QSError::SDK_UNEXPECTED_RESPONSE;  // 429;
  map[INTERNAL_SERVER_ERROR]           = QSError::SDK_UNEXPECTED_RESPONSE;  // 500;
  map[SERVICE_UNAVAILABLE]             = QSError::SDK_UNEXPECTED_RESPONSE;  // 503;
  map[GATEWAY_TIMEOUT]                 = QSError::SDK_UNEXPECTED_RESPONSE;  // 504;
  map[HTTP_VERSION_NOT_SUPPORTED]      = QSError::SDK_UNEXPECTED_RESPONSE;  // 505;
  map[VARIANT_ALSO_NEGOTIATES]         = QSError::SDK_UNEXPECTED_RESPONSE;  // 506;
  map[INSUFFICIENT_STORAGE]            = QSError::SDK_UNEXPECTED_RESPONSE;  // 506;
  map[LOOP_DETECTED]                   = QSError::SDK_UNEXPECTED_RESPONSE;  // 508;
  map[BANDWIDTH_LIMIT_EXCEEDED]        = QSError::SDK_UNEXPECTED_RESPONSE;  // 509;
  map[NOT_EXTENDED]                    = QSError::SDK_UNEXPECTED_RESPONSE;  // 510;
  map[NETWORK_AUTHENTICATION_REQUIRED] = QSError::SDK_UNEXPECTED_RESPONSE;  // 511;
  map[NETWORK_READ_TIMEOUT]            = QSError::SDK_UNEXPECTED_RESPONSE;  // 598;
  map[NETWORK_CONNECT_TIMEOUT]         = QSError::SDK_UNEXPECTED_RESPONSE;  // 599;

  unordered_map<HttpResponseCode, QSError::Value, QS::HashUtils::EnumHash>::iterator it =
        map.find(code);
  return it != map.end() ? it->second : QSError::UNKNOWN;
}

// --------------------------------------------------------------------------
bool SDKShouldRetry(QingStor::Http::HttpResponseCode code){
  using namespace QingStor::Http;
  static unordered_set<HttpResponseCode, QS::HashUtils::EnumHash> retryableSet;
  retryableSet.insert(CONTINUE);                  //"Continue"               100
  retryableSet.insert(PROCESSING);                //"Processing"             102
  retryableSet.insert(TOO_MANY_REQUESTS);         //"TooManyRequests"        429
  retryableSet.insert(GATEWAY_TIMEOUT);           //"GatewayTimeout"         504
  retryableSet.insert(BANDWIDTH_LIMIT_EXCEEDED);  //"BandwithLimitExceeded"  509
  retryableSet.insert(NETWORK_READ_TIMEOUT);      //"NetworkReadTimeout"     598
  retryableSet.insert(NETWORK_CONNECT_TIMEOUT);   //"NetworkConnectTimeout"  599
  unordered_set<HttpResponseCode, QS::HashUtils::EnumHash>::iterator it =
        retryableSet.find(code);
  return it != retryableSet.end();
}

// --------------------------------------------------------------------------
bool SDKResponseSuccess(QsError sdkErr, HttpResponseCode code) {
  return (sdkErr == QS_ERR_NO_ERROR) && SDKResponseCodeSuccess(code);
}

// --------------------------------------------------------------------------
string SDKResponseCodeToName(HttpResponseCode code) {
  using namespace QingStor::Http;
  static unordered_map<HttpResponseCode, string, QS::HashUtils::EnumHash> map;
  map[REQUEST_NOT_MADE]                = "RequestNotMade";                 // 0
  map[CONTINUE]                        = "Continue";                       // 100
  map[SWITCHING_PROTOCOLS]             = "SwitchingProtocols";             // 101
  map[PROCESSING]                      = "Processing";                     // 102
  map[OK]                              = "Ok";                             // 200
  map[CREATED]                         = "Created";                        // 201
  map[ACCEPTED]                        = "Accepted";                       // 202
  map[NON_AUTHORITATIVE_INFORMATION]   = "NonAuthoritativeInformation";    // 203
  map[NO_CONTENT]                      = "NoContent";                      // 204
  map[RESET_CONTENT]                   = "ResetContent";                   // 205
  map[PARTIAL_CONTENT]                 = "PartialContent";                 // 206
  map[MULTI_STATUS]                    = "MultiStatus";                    // 207
  map[ALREADY_REPORTED]                = "AlreadyReported";                // 208
  map[IM_USED]                         = "IMUsed";                         // 226
  map[MULTIPLE_CHOICES]                = "MultipleChoices";                // 300
  map[MOVED_PERMANENTLY]               = "MovedPermanently";               // 301
  map[FOUND]                           = "Found";                          // 302
  map[SEE_OTHER]                       = "SeeOther";                       // 303
  map[NOT_MODIFIED]                    = "NotModified";                    // 304
  map[USE_PROXY]                       = "UseProxy";                       // 305
  map[SWITCH_PROXY]                    = "SwitchProxy";                    // 306
  map[TEMPORARY_REDIRECT]              = "TemporaryRedirect";              // 307
  map[PERMANENT_REDIRECT]              = "PermanentRedirect";              // 308
  map[BAD_REQUEST]                     = "BadRequest";                     // 400
  map[UNAUTHORIZED_OR_EXPIRED]         = "UnauthorizedOrExpired";          // 401
  map[DELINQUENT_ACCOUNT]              = "DelinquentAccount";              // 402
  map[FORBIDDEN]                       = "Forbidden";                      // 403
  map[NOT_FOUND]                       = "NotFound";                       // 404
  map[METHOD_NOT_ALLOWED]              = "MethodNotAllowed";               // 405
  map[CONFLICT]                        = "Conflict";                       // 409
  map[PRECONDITION_FAILED]             = "PerconditionFailed";             // 412
  map[INVALID_RANGE]                   = "InvalidRange";                   // 416
  map[TOO_MANY_REQUESTS]               = "TooManyRequests";                // 429
  map[INTERNAL_SERVER_ERROR]           = "InternalServerError";            // 500
  map[SERVICE_UNAVAILABLE]             = "ServiceUnavailable";             // 503
  map[GATEWAY_TIMEOUT]                 = "GatewayTimeout";                 // 504
  map[HTTP_VERSION_NOT_SUPPORTED]      = "HttpVersionNotSupported";        // 505
  map[VARIANT_ALSO_NEGOTIATES]         = "VariantAlsoNegotiates";          // 506
  map[INSUFFICIENT_STORAGE]            = "InsufficientStorage";            // 506
  map[LOOP_DETECTED]                   = "LoopDetected";                   // 508
  map[BANDWIDTH_LIMIT_EXCEEDED]        = "BandwithLimitExceeded";          // 509
  map[NOT_EXTENDED]                    = "NotExtended";                    // 510
  map[NETWORK_AUTHENTICATION_REQUIRED] = "NetworkAuthenticationRequired";  // 511
  map[NETWORK_READ_TIMEOUT]            = "NetworkReadTimeout";             // 598
  map[NETWORK_CONNECT_TIMEOUT]         = "NetworkConnectTimeout";          // 599
  unordered_map<HttpResponseCode, string, QS::HashUtils::EnumHash>::iterator it =
        map.find(code);
  return it != map.end() ? it->second : "UnknownQingStorResponseCode";
}

// --------------------------------------------------------------------------
int SDKResponseCodeToInt(HttpResponseCode code) {
  using namespace QingStor::Http;
  static unordered_map<HttpResponseCode, int, QS::HashUtils::EnumHash> map;
  map[REQUEST_NOT_MADE]                =   0;
  map[CONTINUE]                        = 100;
  map[SWITCHING_PROTOCOLS]             = 101;
  map[PROCESSING]                      = 102;
  map[OK]                              = 200;
  map[CREATED]                         = 201;
  map[ACCEPTED]                        = 202;
  map[NON_AUTHORITATIVE_INFORMATION]   = 203;
  map[NO_CONTENT]                      = 204;
  map[RESET_CONTENT]                   = 205;
  map[PARTIAL_CONTENT]                 = 206;
  map[MULTI_STATUS]                    = 207;
  map[ALREADY_REPORTED]                = 208;
  map[IM_USED]                         = 226;
  map[MULTIPLE_CHOICES]                = 300;
  map[MOVED_PERMANENTLY]               = 301;
  map[FOUND]                           = 302;
  map[SEE_OTHER]                       = 303;
  map[NOT_MODIFIED]                    = 304;
  map[USE_PROXY]                       = 305;
  map[SWITCH_PROXY]                    = 306;
  map[TEMPORARY_REDIRECT]              = 307;
  map[PERMANENT_REDIRECT]              = 308;
  map[BAD_REQUEST]                     = 400;
  map[UNAUTHORIZED_OR_EXPIRED]         = 401;
  map[DELINQUENT_ACCOUNT]              = 402;
  map[FORBIDDEN]                       = 403;
  map[NOT_FOUND]                       = 404;
  map[METHOD_NOT_ALLOWED]              = 405;
  map[CONFLICT]                        = 409;
  map[PRECONDITION_FAILED]             = 412;
  map[INVALID_RANGE]                   = 416;
  map[TOO_MANY_REQUESTS]               = 429;
  map[INTERNAL_SERVER_ERROR]           = 500;
  map[SERVICE_UNAVAILABLE]             = 503;
  map[GATEWAY_TIMEOUT]                 = 504;
  map[HTTP_VERSION_NOT_SUPPORTED]      = 505;
  map[VARIANT_ALSO_NEGOTIATES]         = 506;
  map[INSUFFICIENT_STORAGE]            = 506;
  map[LOOP_DETECTED]                   = 508;
  map[BANDWIDTH_LIMIT_EXCEEDED]        = 509;
  map[NOT_EXTENDED]                    = 510;
  map[NETWORK_AUTHENTICATION_REQUIRED] = 511;
  map[NETWORK_READ_TIMEOUT]            = 598;
  map[NETWORK_CONNECT_TIMEOUT]         = 599;

  unordered_map<HttpResponseCode, int, QS::HashUtils::EnumHash>::iterator it =
        map.find(code);
  return it != map.end() ? it->second : -1;
}

// --------------------------------------------------------------------------
string SDKResponseCodeToString(HttpResponseCode code) {
  return SDKResponseCodeToName(code) + "(" + to_string(SDKResponseCodeToInt(code)) + ")";
}

}  // namespace Client
}  // namespace QS

