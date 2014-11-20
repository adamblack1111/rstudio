/*
 * SessionModuleContext.hpp
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * Unless you have received this program directly from RStudio pursuant
 * to the terms of a commercial license agreement with RStudio, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

#ifndef SESSION_MODULE_CONTEXT_HPP
#define SESSION_MODULE_CONTEXT_HPP

#include <string>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>

#include <core/system/System.hpp>
#include <core/system/ShellUtils.hpp>
#include <core/system/FileChangeEvent.hpp>
#include <core/http/UriHandler.hpp>
#include <core/json/JsonRpc.hpp>
#include <core/r_util/RToolsInfo.hpp>
#include <core/Thread.hpp>

#include <session/SessionOptions.hpp>
#include <session/SessionClientEvent.hpp>
#include <session/SessionSourceDatabase.hpp>

namespace rscore {
   class Error;
   class Success;
   class FilePath;
   class FileInfo;
   class Settings;
   namespace system {
      class ProcessSupervisor;
      struct ProcessResult;
   }
   namespace shell_utils {
      class ShellCommand;
   }
}

namespace r {
namespace session {
   struct RSuspendOptions;
}
}

namespace session {   
namespace module_context {

enum PackageCompatStatus
{
   COMPAT_OK      = 0,
   COMPAT_MISSING = 1,
   COMPAT_TOO_OLD = 2,
   COMPAT_TOO_NEW = 3,
   COMPAT_UNKNOWN = 4
};
    
// paths 
rscore::FilePath userHomePath();
std::string createAliasedPath(const rscore::FileInfo& fileInfo);
std::string createAliasedPath(const rscore::FilePath& path);
std::string createFileUrl(const rscore::FilePath& path);
rscore::FilePath resolveAliasedPath(const std::string& aliasedPath);
rscore::FilePath userScratchPath();
rscore::FilePath scopedScratchPath();
rscore::FilePath oldScopedScratchPath();
bool isVisibleUserFile(const rscore::FilePath& filePath);

rscore::FilePath safeCurrentPath();

rscore::json::Object createFileSystemItem(const rscore::FileInfo& fileInfo);
rscore::json::Object createFileSystemItem(const rscore::FilePath& filePath);
   
// get a temp file
rscore::FilePath tempFile(const std::string& prefix,
                        const std::string& extension);

rscore::FilePath tempDir();

// find out the location of a binary
rscore::FilePath findProgram(const std::string& name);

bool isPdfLatexInstalled();

// is the file a text file
bool isTextFile(const rscore::FilePath& targetPath);

// find the location of the R script
rscore::Error rBinDir(rscore::FilePath* pRBinDirPath);
rscore::Error rScriptPath(rscore::FilePath* pRScriptPath);
rscore::shell_utils::ShellCommand rCmd(const rscore::FilePath& rBinDir);

// get the R local help port
std::string rLocalHelpPort();

// check if a package is installed
bool isPackageInstalled(const std::string& packageName);

// check if a package is installed with a specific version
bool isPackageVersionInstalled(const std::string& packageName,
                               const std::string& version);

// check if the required versions of various packages are installed
bool isMinimumDevtoolsInstalled();
bool isMinimumRoxygenInstalled();

std::string packageVersion(const std::string& packageName);

bool hasMinimumRVersion(const std::string& version);

// check if a package is installed with a specific version and RStudio protocol
// version (used to allow packages to disable compatibility with older RStudio
// releases)
PackageCompatStatus getPackageCompatStatus(
      const std::string& packageName,
      const std::string& packageVersion,
      int protocolVersion);

rscore::Error installPackage(const std::string& pkgPath,
                           const std::string& libPath = std::string());

rscore::Error installEmbeddedPackage(const std::string& name);

// find the package name for a source file
std::string packageNameForSourceFile(const rscore::FilePath& sourceFilePath);

// register a handler for rBrowseUrl
typedef boost::function<bool(const std::string&)> RBrowseUrlHandler;
rscore::Error registerRBrowseUrlHandler(const RBrowseUrlHandler& handler);
   
// register a handler for rBrowseFile
typedef boost::function<bool(const rscore::FilePath&)> RBrowseFileHandler;
rscore::Error registerRBrowseFileHandler(const RBrowseFileHandler& handler);
   
// register an inbound uri handler (include a leading slash)
rscore::Error registerAsyncUriHandler(
                   const std::string& name,
                   const rscore::http::UriAsyncHandlerFunction& handlerFunction);

// register an inbound uri handler (include a leading slash)
rscore::Error registerUriHandler(
                        const std::string& name,
                        const rscore::http::UriHandlerFunction& handlerFunction);

// register a local uri handler (scoped by a special prefix which indicates
// a local scope)
rscore::Error registerAsyncLocalUriHandler(
                   const std::string& name,
                   const rscore::http::UriAsyncHandlerFunction& handlerFunction);

// register a local uri handler (scoped by a special prefix which indicates
// a local scope)
rscore::Error registerLocalUriHandler(
                        const std::string& name,
                        const rscore::http::UriHandlerFunction& handlerFunction);

typedef boost::function<void(int, const std::string&)> PostbackHandlerContinuation;

// register a postback handler. see docs in SessionPostback.cpp for 
// details on the requirements of postback handlers
typedef boost::function<void(const std::string&, const PostbackHandlerContinuation&)>
                                                      PostbackHandlerFunction;
rscore::Error registerPostbackHandler(
                              const std::string& name,
                              const PostbackHandlerFunction& handlerFunction,
                              std::string* pShellCommand); 
                        
// register an rpc method
rscore::Error registerAsyncRpcMethod(
                              const std::string& name,
                              const rscore::json::JsonRpcAsyncFunction& function);

// register an rpc method
rscore::Error registerRpcMethod(const std::string& name,
                              const rscore::json::JsonRpcFunction& function);


rscore::Error executeAsync(const rscore::json::JsonRpcFunction& function,
                         const rscore::json::JsonRpcRequest& request,
                         rscore::json::JsonRpcResponse* pResponse);


// create a waitForMethod function -- when called this function will:
//
//   (a) enque the passed event
//   (b) wait for the specified methodName to be returned from the client
//   (c) automatically re-issue the event after a client-init
//
typedef boost::function<bool(rscore::json::JsonRpcRequest*, const ClientEvent&)> WaitForMethodFunction;
WaitForMethodFunction registerWaitForMethod(const std::string& methodName);

namespace {

template <typename T>
rscore::Error rpcAsyncCoupleRunner(
      boost::function<rscore::Error(const rscore::json::JsonRpcRequest&, T*)> initFunc,
      boost::function<rscore::Error(const rscore::json::JsonRpcRequest&, const T&, rscore::json::JsonRpcResponse*)> workerFunc,
      const rscore::json::JsonRpcRequest& request,
      rscore::json::JsonRpcResponse* pResponse)
{
   T state;
   rscore::Error error = initFunc(request, &state);
   if (error)
      return error;

   return executeAsync(boost::bind(workerFunc, _1, state, _2),
                       request,
                       pResponse);
}

} // anonymous namespace

// Registers a two-part request handler, where "initFunc" runs on the main
// thread (and has access to everything a normal handler does, like R) and
// "workerFunc" runs on a background thread (and must not touch anything
// that isn't threadsafe). It is a Good Idea to only use workerFunc functions
// that are declared in the "workers" sub-project.
//
// The T type parameter represents the type of a value that initFunc produces
// and workerFunc consumes. This can be used to pass context between the two.
template <typename T>
rscore::Error registerRpcAsyncCoupleMethod(
      const std::string& name,
      boost::function<rscore::Error(const rscore::json::JsonRpcRequest&, T*)> initFunc,
      boost::function<rscore::Error(const rscore::json::JsonRpcRequest&, const T&, rscore::json::JsonRpcResponse*)> workerFunc)
{
   return registerRpcMethod(name, boost::bind(rpcAsyncCoupleRunner<T>,
                                              initFunc,
                                              workerFunc,
                                              _1,
                                              _2));
}

enum ConsoleOutputType
{
   ConsoleOutputNormal,
   ConsoleOutputError
};

enum ChangeSource
{
   ChangeSourceREPL,
   ChangeSourceRPC,
   ChangeSourceURI
};
   

// custom slot combiner that takes the first non empty value
template<typename T>
struct firstNonEmpty
{
  typedef T result_type;

  template<typename InputIterator>
  T operator()(InputIterator first, InputIterator last) const
  {
     for (InputIterator it = first; it != last; ++it)
     {
        if (!it->empty())
           return *it;
     }
     return T();
  }
};


// session events
struct Events : boost::noncopyable
{
   boost::signals2::signal<void ()>                    onClientInit;
   boost::signals2::signal<void ()>                    onBeforeExecute;
   boost::signals2::signal<void(const std::string&)>   onConsolePrompt;
   boost::signals2::signal<void(const std::string&)>   onConsoleInput;
   boost::signals2::signal<void (ConsoleOutputType, const std::string&)>
                                             onConsoleOutput;
   boost::signals2::signal<void (ChangeSource)>        onDetectChanges;
   boost::signals2::signal<void (rscore::FilePath)>      onSourceEditorFileSaved;
   boost::signals2::signal<void(bool)>                 onDeferredInit;
   boost::signals2::signal<void(bool)>                 onBackgroundProcessing;
   boost::signals2::signal<void(bool)>                 onShutdown;
   boost::signals2::signal<void ()>                    onQuit;
   boost::signals2::signal<void (const std::string&)>  onPackageLoaded;
   boost::signals2::signal<void ()>                    onPackageLibraryMutated;

   // signal for detecting extended type of documents
   boost::signals2::signal<std::string(boost::shared_ptr<source_database::SourceDocument>),
                 firstNonEmpty<std::string> > onDetectSourceExtendedType;
};

Events& events();

// ProcessSupervisor
rscore::system::ProcessSupervisor& processSupervisor();

// schedule incremental work. execute will be called back periodically
// (up to every 25ms if the process is completely idle). if execute
// returns true then it will be called back again, if it returns false
// then it won't ever be called again. in a given period of work the
// execute method will be called multiple times (consecutively) for up
// to the specified incrementalDuration. if you want to implement a
// stateful worker simply create a shared_ptr to your worker object
// and then bind one of its members as the execute parameter. passing
// true as the idleOnly parameter (the default) means that the execute
// function will only be called back during idle time (when the session
// is waiting for user input)
void scheduleIncrementalWork(
         const boost::posix_time::time_duration& incrementalDuration,
         const boost::function<bool()>& execute,
         bool idleOnly = true);

// variation of scheduleIncrementalWork which performs a configurable
// amount of work immediately. this work occurs synchronously with the
// call and will consist of execute being called back repeatedly for
// up to the specified initialDuration
void scheduleIncrementalWork(
         const boost::posix_time::time_duration& initialDuration,
         const boost::posix_time::time_duration& incrementalDuration,
         const boost::function<bool()>& execute,
         bool idleOnly = true);


// schedule work to done every time the specified period elapses.
// if the execute function returns true then the worker will be called
// again after the specified period. pass idleOnly = true to restrict
// periodic work to idle time.
void schedulePeriodicWork(const boost::posix_time::time_duration& period,
                          const boost::function<bool()> &execute,
                          bool idleOnly = true,
                          bool immediate = true);


// schedule work to be done after a fixed delay
void scheduleDelayedWork(const boost::posix_time::time_duration& period,
                         const boost::function<void()> &execute,
                         bool idleOnly = true);


rscore::Error readAndDecodeFile(const rscore::FilePath& filePath,
                              const std::string& encoding,
                              bool allowSubstChars,
                              std::string* pContents);

rscore::Error convertToUtf8(const std::string& encodedContent,
                          const std::string& encoding,
                          bool allowSubstChars,
                          std::string* pDecodedContent);

// source R files
rscore::Error sourceModuleRFile(const std::string& rSourceFile);
rscore::Error sourceModuleRFileWithResult(const std::string& rSourceFile,
                                        const rscore::FilePath& workingDir,
                                        rscore::system::ProcessResult* pResult);
   
// enque client events (note R methods can do this via .rs.enqueClientEvent)
void enqueClientEvent(const ClientEvent& event);

// check whether a directory is currently being monitored by one of our subsystems
bool isDirectoryMonitored(const rscore::FilePath& directory);

// check whether an R source file belongs to the package under development
bool isRScriptInPackageBuildTarget(const rscore::FilePath& filePath);

// convenience method for filtering out file listing and changes
bool fileListingFilter(const rscore::FileInfo& fileInfo);

// enque file changed events
void enqueFileChangedEvent(const rscore::system::FileChangeEvent& event);
void enqueFileChangedEvents(const rscore::FilePath& vcsStatusRoot,
                            const std::vector<rscore::system::FileChangeEvent>& events);


// register a scratch path which is monitored.
typedef boost::function<void(const rscore::system::FileChangeEvent&)> OnFileChange;
rscore::FilePath registerMonitoredUserScratchDir(const std::string& dirName,
                                               const OnFileChange& onFileChange);

// write output to the console (convenience wrapper for enquing a 
// kConsoleWriteOutput event)
void consoleWriteOutput(const std::string& output);   
   
// write an error to the console (convenience wrapper for enquing a 
// kConsoleWriteOutput event)
void consoleWriteError(const std::string& message);
   
// show an error dialog (convenience wrapper for enquing kShowErrorMessage)
void showErrorMessage(const std::string& title, const std::string& message);

void showFile(const rscore::FilePath& filePath,
              const std::string& window = "_blank");


void showContent(const std::string& title, const rscore::FilePath& filePath);

std::string resourceFileAsString(const std::string& fileName);

bool portmapPathForLocalhostUrl(const std::string& url, std::string* pPath);

std::string mapUrlPorts(const std::string& url);

std::string pathRelativeTo(const rscore::FilePath& sourcePath,
                           const rscore::FilePath& targetPath);

void activatePane(const std::string& pane);

int saveWorkspaceAction();
void syncRSaveAction();

std::string libPathsString();
bool canBuildCpp();
bool installRBuildTools(const std::string& action);
bool haveRcppAttributes();
bool isRtoolsCompatible(const rscore::r_util::RToolsInfo& rTools);
bool addRtoolsToPathIfNecessary(std::string* pPath,
                                std::string* pWarningMessage);
bool addRtoolsToPathIfNecessary(rscore::system::Options* pEnvironment,
                                std::string* pWarningMessage);

#ifdef __APPLE__
bool isOSXMavericks();
bool hasOSXMavericksDeveloperTools();
rscore::Error copyImageToCocoaPasteboard(const rscore::FilePath& filePath);
#else
inline bool isOSXMavericks()
{
   return false;
}
inline bool hasOSXMavericksDeveloperTools()
{
   return false;
}
inline rscore::Error copyImageToCocoaPasteboard(const rscore::FilePath& filePath)
{
   return rscore::systemError(boost::system::errc::not_supported, ERROR_LOCATION);
}
#endif

struct VcsContext
{
   std::string detectedVcs;
   std::vector<std::string> applicableVcs;
   std::string svnRepositoryRoot;
   std::string gitRemoteOriginUrl;
};
VcsContext vcsContext(const rscore::FilePath& workingDir);

std::string normalizeVcsOverride(const std::string& vcsOverride);

rscore::FilePath shellWorkingDirectory();

// persist state accross suspend and resume
   
typedef boost::function<void (const r::session::RSuspendOptions&,
                              rscore::Settings*)> SuspendFunction;
typedef boost::function<void(const rscore::Settings&)> ResumeFunction;

class SuspendHandler
{
public:
   SuspendHandler(const SuspendFunction& suspend,
                  const ResumeFunction& resume)
      : suspend_(suspend), resume_(resume)
   {
   }
   
   // COPYING: via compiler
   
   const SuspendFunction& suspend() const { return suspend_; }
   const ResumeFunction& resume() const { return resume_; }
   
private:
   SuspendFunction suspend_;
   ResumeFunction resume_;
};
   
void addSuspendHandler(const SuspendHandler& handler);

bool rSessionResumed();

const int kCompileOutputCommand = 0;
const int kCompileOutputNormal = 1;
const int kCompileOutputError = 2;

struct CompileOutput
{
   CompileOutput(int type, const std::string& output)
      : type(type), output(output)
   {
   }

   int type;
   std::string output;
};

rscore::json::Object compileOutputAsJson(const CompileOutput& compileOutput);


std::string previousRpubsUploadId(const rscore::FilePath& filePath);

std::string CRANReposURL();

struct UserPrompt
{
   enum Type { Info = 0, Warning = 1, Error = 2, Question = 3 };
   enum Response { ResponseYes = 0, ResponseNo = 1, ResponseCancel = 2 };

   UserPrompt(int type,
              const std::string& caption,
              const std::string& message,
              bool includeCancel = false)
   {
      commonInit(type, caption, message, "", "", includeCancel, true);
   }

   UserPrompt(int type,
              const std::string& caption,
              const std::string& message,
              bool includeCancel,
              bool yesIsDefault)
   {
      commonInit(type, caption, message, "", "", includeCancel, yesIsDefault);
   }

   UserPrompt(int type,
              const std::string& caption,
              const std::string& message,
              const std::string& yesLabel,
              const std::string& noLabel,
              bool includeCancel,
              bool yesIsDefault)
   {
      commonInit(type,
                 caption,
                 message,
                 yesLabel,
                 noLabel,
                 includeCancel,
                 yesIsDefault);
   }

   int type ;
   std::string caption;
   std::string message;
   std::string yesLabel;
   std::string noLabel;
   bool includeCancel;
   bool yesIsDefault;

private:
   void commonInit(int type,
                   const std::string& caption,
                   const std::string& message,
                   const std::string& yesLabel,
                   const std::string& noLabel,
                   bool includeCancel,
                   bool yesIsDefault)
   {
      this->type = type;
      this->caption = caption;
      this->message = message;
      this->yesLabel = yesLabel;
      this->noLabel = noLabel;
      this->includeCancel = includeCancel;
      this->yesIsDefault = yesIsDefault;
   }
};

UserPrompt::Response showUserPrompt(const UserPrompt& userPrompt);

struct PackratContext
{
   PackratContext() :
      available(false),
      applicable(false),
      packified(false),
      modeOn(false)
   {
   }

   bool available;
   bool applicable;
   bool packified;
   bool modeOn;
};

bool isRequiredPackratInstalled();

PackratContext packratContext();
rscore::json::Object packratContextAsJson();

rscore::json::Object packratOptionsAsJson();

// R command invocation -- has two representations, one to be submitted
// (shellCmd_) and one to show the user (cmdString_)
class RCommand
{
public:
   explicit RCommand(const rscore::FilePath& rBinDir)
      : shellCmd_(buildRCmd(rBinDir))
   {
#ifdef _WIN32
      cmdString_ = "Rcmd.exe";
#else
      cmdString_ = "R CMD";
#endif

      // set escape mode to files-only. this is so that when we
      // add the group of extra arguments from the user that we
      // don't put quotes around it.
      shellCmd_ << rscore::shell_utils::EscapeFilesOnly;
   }

   RCommand& operator<<(const std::string& arg)
   {
      if (!arg.empty())
      {
         cmdString_ += " " + arg;
         shellCmd_ << arg;
      }
      return *this;
   }

   RCommand& operator<<(const rscore::FilePath& arg)
   {
      cmdString_ += " " + arg.absolutePath();
      shellCmd_ << arg;
      return *this;
   }


   const std::string& commandString() const
   {
      return cmdString_;
   }

   const rscore::shell_utils::ShellCommand& shellCommand() const
   {
      return shellCmd_;
   }

private:
   static rscore::shell_utils::ShellCommand buildRCmd(
                                 const rscore::FilePath& rBinDir);

private:
   std::string cmdString_;
   rscore::shell_utils::ShellCommand shellCmd_;
};


class ViewerHistoryEntry
{
public:
   ViewerHistoryEntry() {}
   explicit ViewerHistoryEntry(const std::string& sessionTempPath)
      : sessionTempPath_(sessionTempPath)
   {
   }

   bool empty() const { return sessionTempPath_.empty(); }

   std::string url() const;

   const std::string& sessionTempPath() const { return sessionTempPath_; }

   rscore::Error copy(const rscore::FilePath& sourceDir,
                    const rscore::FilePath& destinationDir) const;

private:
   std::string sessionTempPath_;
};

void addViewerHistoryEntry(const ViewerHistoryEntry& entry);

rscore::Error recursiveCopyDirectory(const rscore::FilePath& fromDir,
                                   const rscore::FilePath& toDir);

std::string sessionTempDirUrl(const std::string& sessionTempPath);

rscore::Error uniqueSaveStem(const rscore::FilePath& directoryPath,
                           const std::string& base,
                           std::string* pStem);

rscore::json::Object plotExportFormat(const std::string& name,
                                    const std::string& extension);


rscore::Error createSelfContainedHtml(const rscore::FilePath& sourceFilePath,
                                    const rscore::FilePath& targetFilePath);

bool isUserFile(const rscore::FilePath& filePath);

} // namespace module_context
} // namespace session

#endif // SESSION_MODULE_CONTEXT_HPP

