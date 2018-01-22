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

#include "data/FileMetaData.h"

#include <string>

#include "boost/exception/to_string.hpp"
#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"

#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "configure/Default.h"

namespace QS {

namespace Data {

using boost::make_shared;
using boost::shared_ptr;
using boost::to_string;
using QS::Configure::Default::GetDefineDirMode;
using QS::StringUtils::AccessMaskToString;
using QS::StringUtils::FormatPath;
using QS::StringUtils::ModeToString;
using QS::Utils::AppendPathDelim;
using QS::Utils::GetProcessEffectiveUserID;
using QS::Utils::GetProcessEffectiveGroupID;
using QS::Utils::IsRootDirectory;
using std::string;

// --------------------------------------------------------------------------
string GetFileTypeName(FileType::Value fileType) {
  string type;
  switch (fileType) {
    case FileType::File:
      type = "File";
      break;
    case FileType::Directory:
      type = "Directory";
      break;
    case FileType::SymLink:
      type = "Symbolic Link";
      break;
    case FileType::Block:
      type = "Block";
      break;
    case FileType::Character:
      type = "Character";
      break;
    case FileType::FIFO:
      type = "FIFO";
      break;
    case FileType::Socket:
      type = "Socket";
      break;
    default:
      break;
  }

  return type;
}

// --------------------------------------------------------------------------
shared_ptr<FileMetaData> BuildDefaultDirectoryMeta(const string &dirPath,
                                                        time_t mtime) {
  time_t atime = time(NULL);
  mode_t mode = GetDefineDirMode();
  return make_shared<FileMetaData>(
      AppendPathDelim(dirPath), 0, atime, mtime, GetProcessEffectiveUserID(),
      GetProcessEffectiveGroupID(), mode, FileType::Directory);
}

// --------------------------------------------------------------------------
FileMetaData::FileMetaData(const string &filePath, uint64_t fileSize,
                           time_t atime, time_t mtime, uid_t uid, gid_t gid,
                           mode_t fileMode, FileType::Value fileType,
                           const string &mimeType, const string &eTag,
                           bool encrypted, dev_t dev, int numlink)
    : m_filePath(filePath),
      m_fileSize(fileSize),
      m_atime(atime),
      m_mtime(mtime),
      m_ctime(mtime),
      m_cachedTime(atime),
      m_uid(uid),
      m_gid(gid),
      m_fileMode(fileMode),
      m_fileType(fileType),
      m_mimeType(mimeType),
      m_eTag(eTag),
      m_encrypted(encrypted),
      m_dev(dev),
      m_numLink(numlink),
      m_needUpload(false),
      m_fileOpen(false) {
  m_numLink = fileType == FileType::Directory ? 2 : 1;
  if (fileType == FileType::Directory) {
    m_filePath = AppendPathDelim(m_filePath);
  }
}

// --------------------------------------------------------------------------
bool FileMetaData::operator==(const FileMetaData &rhs) const {
  return m_filePath == rhs.m_filePath && m_fileSize == rhs.m_fileSize &&
         m_atime == rhs.m_atime && m_mtime == rhs.m_mtime &&
         m_ctime == rhs.m_ctime && m_cachedTime == rhs.m_cachedTime &&
         m_uid == rhs.m_uid && m_gid == rhs.m_gid &&
         m_fileMode == rhs.m_fileMode && m_fileType == rhs.m_fileType &&
         m_mimeType == rhs.m_mimeType && m_eTag == rhs.m_eTag &&
         m_encrypted == rhs.m_encrypted && m_dev == rhs.m_dev &&
         m_numLink == rhs.m_numLink && m_needUpload == rhs.m_needUpload &&
         m_fileOpen == rhs.m_fileOpen;
}

// --------------------------------------------------------------------------
struct stat FileMetaData::ToStat() const {
  struct stat st;
  st.st_size = m_fileSize;
  st.st_blocks = QS::Configure::Default::GetBlocks(st.st_size);
  st.st_blksize = QS::Configure::Default::GetBlockSize();
  st.st_atim.tv_sec = m_atime;
  st.st_mtim.tv_sec = m_mtime;
  st.st_ctim.tv_sec = m_ctime;
  st.st_atim.tv_nsec = 0;
  st.st_mtim.tv_nsec = 0;
  st.st_ctim.tv_nsec = 0;
  st.st_uid = m_uid;
  st.st_gid = m_gid;
  st.st_mode = GetFileTypeAndMode();
  st.st_dev = m_dev;
  // this may need to always set with 1, to see fuse FAQ. TODO(jim):
  st.st_nlink = m_numLink;

  return st;
}

// --------------------------------------------------------------------------
mode_t FileMetaData::GetFileTypeAndMode() const {
  mode_t stmode;
  switch (m_fileType) {
    case FileType::File:
      stmode = S_IFREG | m_fileMode;
      break;
    case FileType::Directory:
      stmode = S_IFDIR | m_fileMode;
      break;
    case FileType::SymLink:
      stmode = S_IFLNK | m_fileMode;
      break;
    case FileType::Block:
      stmode = S_IFBLK | m_fileMode;
      break;
    case FileType::Character:
      stmode = S_IFCHR | m_fileMode;
      break;
    case FileType::FIFO:
      stmode = S_IFIFO | m_fileMode;
      break;
    case FileType::Socket:
      stmode = S_IFSOCK | m_fileMode;
      break;
    default:
      stmode = S_IFREG | m_fileMode;
      break;
  }
  return stmode;
}

// --------------------------------------------------------------------------
string FileMetaData::MyDirName() const {
  return QS::Utils::GetDirName(m_filePath);
}

// --------------------------------------------------------------------------
string FileMetaData::MyBaseName() const {
  return QS::Utils::GetBaseName(m_filePath);
}

// --------------------------------------------------------------------------
bool FileMetaData::FileAccess(uid_t uid, gid_t gid, int amode) const {
  DebugInfo("Check access permission " + FormatPath(m_filePath));
  DebugInfo("[uid:gid:mode process=" + to_string(uid) + ":" + to_string(gid)+
           ":" + AccessMaskToString(amode) +
           ", file=" + to_string(m_uid) + ":" + to_string(m_gid) +
           ":" + ModeToString(m_fileMode) + "]");

  if (m_filePath.empty()) {
    DebugWarning("object file path is empty");
    return false;
  }

  // Check file existence
  if (amode & F_OK) {
    return true;  // there is a file, always allowed
  }

  bool ret = false;
  // Check read permission
  if (amode & R_OK) {
    if ((uid == m_uid || uid == 0) && (m_fileMode & S_IRUSR)) {
      ret = true;
    } else if ((gid == m_gid || gid == 0) && (m_fileMode & S_IRGRP)) {
      ret = true;
    } else if (m_fileMode & S_IROTH) {
      ret = true;
    } else {
      return false;
    }
  }
  // Check write permission
  if (amode & W_OK) {
    if ((uid == m_uid || uid == 0) && (m_fileMode & S_IWUSR)) {
      ret = true;
    } else if ((gid == m_gid || gid == 0) && (m_fileMode & S_IWGRP)) {
      ret = true;
    } else if (m_fileMode & S_IWOTH) {
      ret = true;
    } else {
      return false;
    }
  }
  // Check execute permission
  if (amode & X_OK) {
    if (uid == 0) {
      // if execute permission is allowed for any user,
      // root shall get execute permission too.
      if ((m_fileMode & S_IXUSR) || (m_fileMode & S_IXGRP) ||
          (m_fileMode & S_IXOTH)) {
        ret = true;
      } else {
        return false;
      }
    } else {
      if ((uid == m_uid) && (m_fileMode & S_IXUSR)) {
        ret = true;
      } else if ((gid == m_gid) && (m_fileMode & S_IXGRP)) {
        ret = true;
      } else if (m_fileMode & S_IXOTH) {
        ret = true;
      } else {
        return false;
      }
    }
  }

  return ret;
}

}  // namespace Data
}  // namespace QS
