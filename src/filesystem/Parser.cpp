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

#include "filesystem/Parser.h"

#include <assert.h>
#include <stddef.h>  // for offsetof
#include <stdint.h>
#include <string.h>  // for strdup

#include <iostream>
#include <string>

#include "boost/exception/to_string.hpp"

#include "base/Exception.h"
#include "base/LogLevel.h"
#include "base/Size.h"
#include "client/Protocol.h"
#include "configure/Default.h"
#include "configure/IncludeFuse.h"  // for fuse.h
#include "configure/Options.h"

namespace QS {

namespace FileSystem {

using QS::Exception::QSException;

namespace Parser {

namespace {

using boost::to_string;
using QS::Configure::Default::GetClientDefaultPoolSize;
using QS::Configure::Default::GetDefaultCredentialsFile;
using QS::Configure::Default::GetDefaultDiskCacheDirectory;
using QS::Configure::Default::GetDefaultLogDirectory;
using QS::Configure::Default::GetDefaultLogLevelName;
using QS::Configure::Default::GetDefaultHostName;
using QS::Configure::Default::GetDefaultMaxRetries;
using QS::Configure::Default::GetDefaultPort;
using QS::Configure::Default::GetDefaultProtocolName;
using QS::Configure::Default::GetDefaultParallelTransfers;
using QS::Configure::Default::GetDefaultTransferBufSize;
using QS::Configure::Default::GetDefaultZone;
using QS::Configure::Default::GetMaxCacheSize;
using QS::Configure::Default::GetMaxListObjectsCount;
using QS::Configure::Default::GetMaxStatCount;
using QS::Configure::Default::GetTransactionDefaultTimeDuration;

void PrintWarnMsg(const char *opt, int32_t invalidVal, int32_t defaultVal) {
  if (opt == NULL) return;
  std::cerr << "[qsfs] invalid parameter in option " << opt << "="
            << to_string(invalidVal) << ", "
            << to_string(defaultVal) << " is used" << std::endl;
}

static struct options {
  // We can't set default values for the char* fields here
  // because fuse_opt_parse would attempt to free() them
  // when user specifies different values on command line.
  const char *bucket;
  const char *mountPoint;
  const char *zone;
  const char *credentials;
  const char *logDirectory;
  const char *logLevel;        // INFO, WARN, ERROR, FATAL
  int retries;
  int32_t reqtimeout;    // in ms
  int32_t maxcache;      // in MB
  const char *diskdir;
  int32_t maxstat;       // in K
  int32_t maxlist;       // max file count for ls
  int32_t statexpire;    // in mins, negative value disable state expire
  int numtransfer;
  int32_t bufsize;       // in MB
  int threads;
  const char *host;
  const char *protocol;
  int port;
  const char *addtionalAgent;
  int clearLogDir;         // default not clear log dir
  int foreground;          // default not foreground
  int singleThread;        // default FUSE multi-thread
  int qsSingleThread;      // default qsfs single-thread
  int debug;               // default no debug
  int curldbg;             // default no curl debug msg
  int showHelp;
  int showVersion;
} options;

#define OPTION(t, p) \
  { t, offsetof(struct options, p), 1 }

static const struct fuse_opt optionSpec[] = {
    OPTION("-b=%s", bucket),         OPTION("--bucket=%s",      bucket),
    OPTION("-m=%s", mountPoint),     OPTION("--mount=%s",       mountPoint),
    OPTION("-z=%s", zone),           OPTION("--zone=%s",        zone),
    OPTION("-c=%s", credentials),    OPTION("--credentials=%s", credentials),
    OPTION("-l=%s", logDirectory),   OPTION("--logdir=%s",      logDirectory),
    OPTION("-L=%s", logLevel),       OPTION("--loglevel=%s",    logLevel),
    OPTION("-r=%i", retries),        OPTION("--retries=%i",     retries),
    OPTION("-R=%li", reqtimeout),    OPTION("--reqtimeout=%li", reqtimeout),
    OPTION("-Z=%li", maxcache),      OPTION("--maxcache=%li",   maxcache),
    OPTION("-D=%s",  diskdir),       OPTION("--diskdir=%s",     diskdir),
    OPTION("-t=%li", maxstat),       OPTION("--maxstat=%li",    maxstat),
    OPTION("-i=%li", maxlist),       OPTION("--maxlist=%li",    maxlist),
    OPTION("-e=%li", statexpire),    OPTION("--statexpire=%li", statexpire),
    OPTION("-n=%i",  numtransfer),   OPTION("--numtransfer=%i", numtransfer),
    OPTION("-u=%li", bufsize),       OPTION("--bufsize=%li",    bufsize),
    OPTION("-T=%i", threads),        OPTION("--threads=%i",     threads),
    OPTION("-H=%s", host),           OPTION("--host=%s",        host),
    OPTION("-p=%s", protocol),       OPTION("--protocol=%s",    protocol),
    OPTION("-P=%i", port),           OPTION("--port=%i",        port),
    OPTION("-a=%s", addtionalAgent), OPTION("--agent=%s",       addtionalAgent),
    OPTION("-C",    clearLogDir),    OPTION("--clearlogdir",    clearLogDir),
    OPTION("-f",    foreground),     OPTION("--foreground",     foreground),
    OPTION("-s",    singleThread),   OPTION("--single",         singleThread),
    OPTION("-S",    qsSingleThread), OPTION("--Single",         qsSingleThread),
    OPTION("-d",    debug),          OPTION("--debug",          debug),
    OPTION("-U",    curldbg),        OPTION("--curldbg",        curldbg),
    OPTION("-h",    showHelp),       OPTION("--help",           showHelp),
    OPTION("-V",    showVersion),    OPTION("--version",        showVersion),
    FUSE_OPT_END
};

}  // namespace

void Parse(int argc, char **argv) {
  // Set defaults
  // we have to use strdup so that fuse_opt_parse
  // can free the defaults if other values are specified.
  options.bucket         = strdup("");
  options.mountPoint     = strdup("");
  options.zone           = strdup(GetDefaultZone().c_str());
  options.credentials    = strdup(GetDefaultCredentialsFile().c_str());
  options.logDirectory   = strdup(GetDefaultLogDirectory().c_str());
  options.logLevel       = strdup(GetDefaultLogLevelName().c_str());
  options.retries        = GetDefaultMaxRetries();
  options.reqtimeout     = GetTransactionDefaultTimeDuration();
  options.maxcache       = GetMaxCacheSize() / QS::Size::MB1;
  options.diskdir        = strdup(GetDefaultDiskCacheDirectory().c_str());
  options.maxstat        = GetMaxStatCount() / QS::Size::K1;
  options.maxlist        = GetMaxListObjectsCount();
  options.statexpire     =  -1;
  options.numtransfer    = GetDefaultParallelTransfers();
  options.bufsize        = GetDefaultTransferBufSize() / QS::Size::MB1;
  options.threads        = GetClientDefaultPoolSize();
  options.host           = strdup(GetDefaultHostName().c_str());
  options.protocol       = strdup(GetDefaultProtocolName().c_str());
  options.port           = GetDefaultPort(GetDefaultProtocolName());
  options.addtionalAgent = strdup("");
  options.clearLogDir    = 0;
  options.foreground     = 0;
  options.singleThread   = 0;
  options.qsSingleThread = 1;  // default single
  options.debug          = 0;
  options.curldbg        = 0;
  options.showHelp       = 0;
  options.showVersion    = 0;

  // Do Parse
  QS::Configure::Options &qsOptions = QS::Configure::Options::Instance();
  qsOptions.SetFuseArgs(argc, argv);

  struct fuse_args &args = qsOptions.GetFuseArgs();
  if (0 != fuse_opt_parse(&args, &options, optionSpec, NULL)) {
    throw QSException("Error while parsing command line options.");
  }

  // Store options
  qsOptions.SetBucket(options.bucket);
  qsOptions.SetMountPoint(options.mountPoint);
  qsOptions.SetZone(options.zone);
  qsOptions.SetCredentialsFile(options.credentials);
  qsOptions.SetLogDirectory(options.logDirectory);
  qsOptions.SetLogLevel(QS::Logging::GetLogLevelByName(options.logLevel));

  if (options.retries <= 0) {
    PrintWarnMsg("-r|--retries", options.retries, GetDefaultMaxRetries());
    qsOptions.SetRetries(GetDefaultMaxRetries());
  } else {
    qsOptions.SetRetries(options.retries);
  }

  if (options.reqtimeout <= 0) {
    PrintWarnMsg("-R|--reqtimeout", options.reqtimeout,
                 GetTransactionDefaultTimeDuration());
    qsOptions.SetRequestTimeOut(GetTransactionDefaultTimeDuration());
  } else {
    qsOptions.SetRequestTimeOut(options.reqtimeout);
  }

  if (options.maxcache <= 0) {
    PrintWarnMsg("-Z|--maxcache", options.maxcache,
                 GetMaxCacheSize() / QS::Size::MB1);
    qsOptions.SetMaxCacheSizeInMB(GetMaxCacheSize() / QS::Size::MB1);
  } else {
    qsOptions.SetMaxCacheSizeInMB(options.maxcache);
  }

  qsOptions.SetDiskCacheDirectory(options.diskdir);

  if (options.maxstat <= 0) {
    PrintWarnMsg("-t|--maxstat", options.maxstat,
                 GetMaxStatCount() / QS::Size::K1);
    qsOptions.SetMaxStatCountInK(GetMaxStatCount() / QS::Size::K1);
  } else {
    qsOptions.SetMaxStatCountInK(options.maxstat);
  }

  qsOptions.SetMaxListCount(options.maxlist);
  qsOptions.SetStatExpireInMin(options.statexpire);

  if (options.numtransfer <= 0) {
    PrintWarnMsg("-n|--numtransfer", options.numtransfer,
                 GetDefaultParallelTransfers());
    qsOptions.SetParallelTransfers(GetDefaultParallelTransfers());
  } else {
    qsOptions.SetParallelTransfers(options.numtransfer);
  }

  if (options.bufsize <= 0) {
    PrintWarnMsg("-u|--bufsize", options.bufsize,
                 GetDefaultTransferBufSize() / QS::Size::MB1);
    qsOptions.SetTransferBufferSizeInMB(GetDefaultTransferBufSize() /
                                        QS::Size::MB1);
  } else {
    qsOptions.SetTransferBufferSizeInMB(options.bufsize);
  }

  if (options.threads <= 0) {
    PrintWarnMsg("-T|--threads", options.threads, GetClientDefaultPoolSize());
    qsOptions.SetClientPoolSize(GetClientDefaultPoolSize());
  } else {
    qsOptions.SetClientPoolSize(options.threads);
  }

  qsOptions.SetHost(options.host);
  qsOptions.SetProtocol(options.protocol);

  if (options.port <= 0) {
    PrintWarnMsg("-P|--port", options.port,
                 GetDefaultPort(GetDefaultProtocolName()));
    qsOptions.SetPort(GetDefaultPort(GetDefaultProtocolName()));
  } else {
    qsOptions.SetPort(options.port);
  }

  qsOptions.SetAdditionalAgent(options.addtionalAgent);
  qsOptions.SetClearLogDir(options.clearLogDir != 0);
  qsOptions.SetForeground(options.foreground != 0);
  qsOptions.SetSingleThread(options.singleThread != 0);
  qsOptions.SetQsfsSingleThread(options.qsSingleThread != 0);
  qsOptions.SetDebug(options.debug != 0);
  qsOptions.SetDebugCurl(options.curldbg != 0);
  qsOptions.SetShowHelp(options.showHelp != 0);
  qsOptions.setShowVerion(options.showVersion !=0);

  // Put signals for fuse_main.
  if (!qsOptions.GetMountPoint().empty()) {
    assert(fuse_opt_add_arg(&args, qsOptions.GetMountPoint().c_str()) == 0);
  }
  if (qsOptions.IsShowHelp()) {
    assert(fuse_opt_add_arg(&args, "-ho") == 0);  // without FUSE usage line
  }
  if (qsOptions.IsShowVersion()) {
    assert(fuse_opt_add_arg(&args, "--version") == 0);
  }
  if (qsOptions.IsForeground()) {
    assert(fuse_opt_add_arg(&args, "-f") == 0);
  }
  if (qsOptions.IsSingleThread()) {
    assert(fuse_opt_add_arg(&args, "-s") == 0);
  }
  if (qsOptions.IsDebug()) {
    assert(fuse_opt_add_arg(&args, "-d") == 0);
  }
}

}  // namespace Parser
}  // namespace FileSystem
}  // namespace QS
