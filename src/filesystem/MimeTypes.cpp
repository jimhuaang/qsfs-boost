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

#include "filesystem/MimeTypes.h"

#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "boost/bind.hpp"
#include "boost/thread/once.hpp"

#include "base/Exception.h"

namespace QS {

namespace FileSystem {

using QS::Exception::QSException;
using std::string;


static const char *CONTENT_TYPE_STREAM1 = "application/octet-stream";
// static const char *CONTENT_TYPE_STREAM2 = "binary/octet-stream";
static const char *CONTENT_TYPE_DIR = "application/x-directory";
static const char *CONTENT_TYPE_TXT = "text/plain";
// Simulate a symbolic link mime type
static const char *CONTENT_TYPE_SYMLINK = "application/symlink";


static boost::once_flag initOnceFlag;

// --------------------------------------------------------------------------
void InitializeMimeTypes(const std::string &mimeFile) {
  MimeTypes &instance = MimeTypes::Instance();
  boost::call_once(
      initOnceFlag,
      bind(boost::type<void>(),
           bind(boost::type<void>(), &MimeTypes::Initialize, &instance, _1),
           mimeFile));
}

// --------------------------------------------------------------------------
string MimeTypes::Find(const string &ext) {
  ExtToMimetypeMapIterator it = m_extToMimeTypeMap.find(ext);
  return it != m_extToMimeTypeMap.end() ? it->second : string();
}

// --------------------------------------------------------------------------
void MimeTypes::Initialize(const std::string &mimeFile) {
  std::ifstream file(mimeFile.c_str());
  if (!file) {
    throw QSException("Unable to open file " + mimeFile);
  }

  string line;
  while (getline(file, line)) {
    if (line.empty()) continue;
    if (line[0] == '#') continue;

    std::stringstream ss(line);
    string mimeType;
    ss >> mimeType;
    while (ss) {
      string ext;
      ss >> ext;
      if (ext.empty()) continue;
      m_extToMimeTypeMap[ext] = mimeType;
    }
  }
}

// --------------------------------------------------------------------------
string LookupMimeType(const string &path) {
  string defaultMimeType(CONTENT_TYPE_STREAM1);

  string::size_type lastPos = path.find_last_of('.');
  if (lastPos == string::npos) return defaultMimeType;

  // Extract the last extension
  string ext = path.substr(1 + lastPos);
  // If the last extension matches a mime type, return it
  string mimeType = MimeTypes::Instance().Find(ext);
  if (!mimeType.empty()) return mimeType;

  // Extract the second to last file exstension
  string::size_type firstPos = path.find_first_of('.');
  if (firstPos == lastPos) return defaultMimeType;  // there isn't a 2nd ext
  string ext2;
  if (firstPos != string::npos && firstPos < lastPos) {
    string prefix = path.substr(0, lastPos);
    // Now get the second to last file extension
    string::size_type nextPos = prefix.find_last_of('.');
    if (nextPos != string::npos) {
      ext2 = prefix.substr(1 + nextPos);
    }
  }
  // If the second extension matches a mime type, return it
  mimeType = MimeTypes::Instance().Find(ext2);
  if (!mimeType.empty()) return mimeType;

  return defaultMimeType;
}

// --------------------------------------------------------------------------
string GetDirectoryMimeType() { return CONTENT_TYPE_DIR; }

// --------------------------------------------------------------------------
string GetTextMimeType() { return CONTENT_TYPE_TXT; }

// --------------------------------------------------------------------------
string GetSymlinkMimeType() {
  // Stimulate a mime type for symlink
  return CONTENT_TYPE_SYMLINK;
}

}  // namespace FileSystem
}  // namespace QS
