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

#include "filesystem/Mounter.h"

#include <errno.h>
#include <string.h>  // for strerror, strcmp

#include <assert.h>
#include <dirent.h>
#include <stdio.h>   // for popen
#include <stdlib.h>  // for system
#include <sys/stat.h>

#include <string>
#include <utility>

#include "base/Exception.h"
#include "base/LogMacros.h"
#include "base/StringUtils.h"
#include "base/Utils.h"
#include "base/UtilsWithLog.h"
#include "configure/IncludeFuse.h"  // for fuse.h
#include "configure/Options.h"
#include "filesystem/Drive.h"
#include "filesystem/Operations.h"

namespace QS {

namespace FileSystem {

using QS::StringUtils::FormatPath;
using QS::Configure::Options;
using QS::Exception::QSException;
using std::make_pair;
using std::pair;
using std::string;

// --------------------------------------------------------------------------
// IsMountable only checking following things:
// the mount point is
//   - not root
//   - accessable, means exist
//   - is dir
//   - process has permission to access it
//
// Notes, IsMountable assume currently the mount point is still not mounted,
// otherwise, stat a mounted point always fail.
pair<bool, string> Mounter::IsMountable(const std::string &mountPoint,
                                        bool logOn) const {
  bool success = true;
  string msg;

  if (QS::Utils::IsRootDirectory(mountPoint)) {
    return make_pair(false, "Unable to mount to root directory");
  }

  struct stat stBuf;
  if (stat(mountPoint.c_str(), &stBuf) != 0) {
    success = false;
    msg = "Unable to access MOUNTPOINT : " + string(strerror(errno)) +
          FormatPath(mountPoint);
  } else if (!S_ISDIR(stBuf.st_mode)) {
    success = false;
    msg = "MOUNTPOINT is not a directory " + FormatPath(mountPoint);
  } else if (!QS::UtilsWithLog::HavePermission(mountPoint)) {
    success = false;
    msg = "MOUNTPOINT permisson denied " + FormatPath(mountPoint);
  }

  return make_pair(success, msg);
}

// --------------------------------------------------------------------------
bool Mounter::IsMounted(const string &mountPoint, bool logOn) const {
  bool mounted = false;
  string command("cat /etc/mtab | grep " + mountPoint + " | wc -c");
  FILE *pFile = popen(command.c_str(), "r");
  if (pFile != NULL && fgetc(pFile) != '0') {
    mounted = true;
  }
  pclose(pFile);

  return mounted;
}

// --------------------------------------------------------------------------
bool Mounter::Mount(const Options &options, bool logOn) const {
  Drive &drive = QS::FileSystem::Drive::Instance();
  if (!drive.IsMountable()) {
    throw QSException("Unable to connect bucket " + options.GetBucket() +
                      " ...");
  }
  return DoMount(options, logOn, &drive);
}

// --------------------------------------------------------------------------
void Mounter::UnMount(const string &mountPoint, bool logOn) const {
  if (IsMounted(mountPoint, logOn)) {
    string command("fusermount -u " + mountPoint + " | wc -c");
    FILE *pFile = popen(command.c_str(), "r");
    if (pFile != NULL && fgetc(pFile) != '0') {
      if (logOn) {
        Error(
            "Unable to unmout filesystem at MOUNTPOINT. Trying lazy unmount " +
            FormatPath(mountPoint));
      }
      command.assign("fusermount -uqz " + mountPoint);
      system(command.c_str());
    }
    pclose(pFile);
    if (logOn) {
      Info("Unmount qsfs sucessfully");
    }
  } else {
    if (logOn) {
      Warning("Trying to unmount filesystem at an unmounted MOUNTPOINT " +
              FormatPath(mountPoint));
    }
  }
}

// --------------------------------------------------------------------------
bool Mounter::DoMount(const Options &options, bool logOn,
                      void *user_data) const {
  static fuse_operations qsfsOperations;
  InitializeFUSECallbacks(&qsfsOperations);

  // Do really mount
  struct fuse_args &fuseArgs = const_cast<Options &>(options).GetFuseArgs();
  const string &mountPoint = options.GetMountPoint();
  static int maxTries = 3;
  int count = 0;
  do {
    if (!IsMounted(mountPoint, logOn)) {
      int ret =
          fuse_main(fuseArgs.argc, fuseArgs.argv, &qsfsOperations, user_data);
      if (0 != ret) {
        errno = ret;
        throw QSException("Unable to mount qsfs");
      } else {
        return true;
      }
    } else {
      if (++count > maxTries) {
        if (logOn) {
          Error("Unable to unmount MOUNTPOINT " + FormatPath(mountPoint));
        }
        return false;
      }
      if (logOn) {
        Warning(
            "MOUNTPOINT is already mounted. Trying to unmount, and mount "
            "again " +
            FormatPath(mountPoint));
      }
      string command("umount -l " + mountPoint);  // lazy detach filesystem
      system(command.c_str());
    }
  } while (true);

  return false;
}

}  // namespace FileSystem
}  // namespace QS
