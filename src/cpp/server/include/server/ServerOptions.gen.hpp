/*
 * ServerOptions.gen.hpp
 *
 * Copyright (C) 2022 by Posit Software, PBC
 *
 * Unless you have received this program directly from Posit Software pursuant
 * to the terms of a commercial license agreement with Posit Software, then
 * this program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */

// THIS FILE WAS AUTOMATICALLY GENERATED BY A TOOL - DO NOT HAND EDIT IT

#ifndef SERVER_SERVER_OPTIONS_GEN_HPP
#define SERVER_SERVER_OPTIONS_GEN_HPP

#include <string>
#include <map>
#include <shared_core/FilePath.hpp>
#include <core/ProgramOptions.hpp>
#include <boost/regex.hpp>
#include <boost/utility.hpp>
#include <core/system/PosixUser.hpp>
#include <core/system/PosixSystem.hpp>
#include <core/system/Xdg.hpp>
#include <monitor/MonitorConstants.hpp>
#include <core/http/Request.hpp>
#include <core/http/Cookie.hpp>


namespace rstudio {
namespace server {

class GeneratedOptions
{
public:
   virtual ~GeneratedOptions() {}
   virtual core::ProgramStatus read(int argc,
                                    char * const argv[],
                                    std::ostream& osWarnings) = 0;

protected:
   rstudio::core::program_options::OptionsDescription
   buildOptions(boost::program_options::options_description* pVerify,
                boost::program_options::options_description* pServer,
                boost::program_options::options_description* pWww,
                boost::program_options::options_description* pRsession,
                boost::program_options::options_description* pDatabase,
                boost::program_options::options_description* pAuth,
                boost::program_options::options_description* pMonitor,
                std::string* wwwSameSite,
                std::vector<std::string>* pWwwAllowedOrigins,
                std::string* pAuthLoginPageHtml,
                std::string* pAuthRdpLoginPageHtml,
                std::string* pAuthMinimumUserId)
{
   using namespace rstudio::core;
   using namespace boost::program_options;

   pVerify->add_options()
      ("verify-installation",
      value<bool>(&verifyInstallation_)->default_value(false),
      "Runs verification mode to verify the current installation.");

   pServer->add_options()
      ("server-working-dir",
      value<std::string>(&serverWorkingDir_)->default_value("/"),
      "The default working directory of the rserver process.")
      ("server-user",
      value<std::string>(&serverUser_)->default_value("rstudio-server"),
      "The user account of the rserver process.")
      ("server-daemonize",
      value<bool>(&serverDaemonize_)->default_value(core::system::effectiveUserIsRoot()),
      "Indicates whether or not the rserver process should run as a daemon.")
      ("server-pid-file",
      value<std::string>(&serverPidFile_)->default_value("/var/run/rstudio-server.pid"),
      "The path to a file where the rserver daemon's pid is written.")
      ("server-app-armor-enabled",
      value<bool>(&deprecatedServerAppArmorEnabled_)->default_value(false),
      "Indicates whether or not to enable AppArmor profiles for the rserver process.")
      ("server-set-umask",
      value<bool>(&serverSetUmask_)->default_value(true),
      "If enabled, sets the rserver process umask to 022 on startup, which causes new files to have rw-r-r permissions.")
      ("secure-cookie-key-file",
      value<std::string>(&secureCookieKeyFile_)->default_value(""),
      "If set, overrides the default path of the secure-cookie-key file used for encrypting cookies.")
      ("server-data-dir",
      value<std::string>(&serverDataDir_)->default_value("/var/run/rstudio-server"),
      "Path to the data directory where RStudio Server will write run-time state.")
      ("server-add-header",
      value<std::vector<std::string>>(&serverAddHeaders_)->default_value(std::vector<std::string>())->multitoken(),
      "Adds a header to all responses from RStudio Server. This option can be specified multiple times to add multiple headers.")
      ("server-run-automation",
      value<bool>(&serverRunAutomation_)->default_value(false),
      "Run RStudio's built-in automation tests. Requires a Google Chrome installation.");

   pWww->add_options()
      ("www-address",
      value<std::string>(&wwwAddress_)->default_value("0.0.0.0"),
      "The network address that RStudio Server will listen on for incoming connections.")
      ("www-port",
      value<std::string>(&wwwPort_)->default_value(""),
      "The port that RStudio Server will bind to while listening for incoming connections. If left empty, the port will be automatically determined based on your SSL settings (443 for SSL, 80 for no SSL).")
      ("www-socket",
      value<std::string>(&wwwSocket_)->default_value(""),
      "The socket that RStudio Server will bind to while listening for incoming connections. If left empty, a port will be used.")
      ("www-root-path",
      value<std::string>(&wwwRootPath_)->default_value(kRequestDefaultRootPath),
      "The path prefix added by a proxy to the incoming RStudio URL. This setting is used so RStudio Server knows what path it is being served from. If running RStudio Server behind a path-modifying proxy, this should be changed to match the base RStudio Server URL.")
      ("www-local-path",
      value<std::string>(&wwwLocalPath_)->default_value("www"),
      "The relative path from the RStudio installation directory, or absolute path where web assets are stored.")
      ("www-symbol-maps-path",
      value<std::string>(&wwwSymbolMapsPath_)->default_value("www-symbolmaps"),
      "The relative path from the RStudio installation directory, or absolute path, where symbol maps are stored.")
      ("www-use-emulated-stack",
      value<bool>(&wwwUseEmulatedStack_)->default_value(false),
      "Indicates whether or not to use GWT's emulated stack.")
      ("www-thread-pool-size",
      value<int>(&wwwThreadPoolSize_)->default_value(6),
      "The size of the threadpool from which requests will be serviced. This needs to have enough threads to avoid bottlenecks due to certain requests that block the request thread (e.g. a login fail might run into a delay caused by the pam configuration). For systems with lots of users a larger value is recommended. For systems with only one or two users, a value of 2 will be slightly more efficient.")
      ("www-proxy-localhost",
      value<bool>(&wwwProxyLocalhost_)->default_value(true),
      "Indicates whether or not to proxy requests to localhost ports over the main server port. This should generally be enabled, and is used to proxy HTTP traffic within a session that belongs to code running within the session (e.g. Shiny or Plumber APIs)")
      ("www-verify-user-agent",
      value<bool>(&wwwVerifyUserAgent_)->default_value(true),
      "Indicates whether or not to verify connecting browser user agents to ensure they are compatible with RStudio Server.")
      ("www-same-site",
      value<std::string>(wwwSameSite)->default_value(""),
      "The value of the 'SameSite' attribute on the cookies issued by RStudio Server. Accepted values are 'none' or 'lax'. The value 'none' should be used only when RStudio is hosted into an iframe. For compatibility with some browsers (i.e. Safari 12), duplicate cookies will be issued by RStudio Server when 'none' is used.")
      ("www-frame-origin",
      value<std::string>(&wwwFrameOrigin_)->default_value("none"),
      "Specifies the allowed origin for the iframe hosting RStudio if iframe embedding is enabled.")
      ("www-enable-origin-check",
      value<bool>(&wwwEnableOriginCheck_)->default_value(false),
      "If enabled, cause RStudio to enforce that incoming request origins are from the host domain. This can be added for additional security. See https://cheatsheetseries.owasp.org/cheatsheets/Cross-Site_Request_Forgery_Prevention_Cheat_Sheet.html#verifying-origin-with-standard-headers")
      ("www-allow-origin",
      value<std::vector<std::string>>(pWwwAllowedOrigins)->default_value(std::vector<std::string>())->multitoken(),
      "Specifies an additional origin that requests are allowed from, even if it does not match the host domain. Used if origin checking is enabled. May be specified multiple times for multiple origins.")
      ("session-use-file-storage",
      value<bool>(&sessionUseFileStorage_)->default_value(true),
      "Whether to use the file system to store metadata about the session storage or the internal database. Setting this to false may require special network configuration. See [Session Storage](../server_management/session_storage.html) for more information.")
      ("www-stats-monitor-seconds",
      value<int>(&statsMonitorSeconds_)->default_value(0),
      "The time interval in seconds to log info/debug messages with stats on server performance. Set to 0 to disable.");

   pRsession->add_options()
      ("rsession-which-r",
      value<std::string>(&rsessionWhichR_)->default_value(""),
      "The path to the main R program (e.g. /usr/bin/R). This should be set if no versions are specified in /etc/rstudio/r-versions and the default R installation is not available on the system path.")
      ("rsession-path",
      value<std::string>(&rsessionPath_)->default_value("rsession"),
      "The relative path from the RStudio installation directory, or absolute path to the rsession executable.")
      ("rldpath-path",
      value<std::string>(&rldpathPath_)->default_value("r-ldpath"),
      "The path to the r-ldpath script which specifies extra library paths for R versions.")
      ("rsession-ld-library-path",
      value<std::string>(&rsessionLdLibraryPath_)->default_value(""),
      "Specifies additional LD_LIBRARY_PATHs to use for R sessions.")
      ("rsession-config-file",
      value<std::string>(&rsessionConfigFile_)->default_value(""),
      "If set, overrides the path to the /etc/rstudio/rsession.conf configuration file. The specified path may be a relative path from the RStudio installation directory, or an absolute path.")
      ("rsession-proxy-max-wait-secs",
      value<int>(&rsessionProxyMaxWaitSeconds_)->default_value(30),
      "The maximum time to wait in seconds for a successful response when proxying requests to rsession.")
      ("rsession-memory-limit-mb",
      value<int>(&deprecatedMemoryLimitMb_)->default_value(0),
      "The limit in MB that an rsession process may consume.")
      ("rsession-stack-limit-mb",
      value<int>(&deprecatedStackLimitMb_)->default_value(0),
      "The limit in MB that an rsession process may consume for its stack.")
      ("rsession-process-limit",
      value<int>(&deprecatedUserProcessLimit_)->default_value(0),
      "The maximum number of allowable rsession processes.");

   pDatabase->add_options()
      ("database-config-file",
      value<std::string>(&databaseConfigFile_)->default_value(""),
      "If set, overrides the path to the /etc/rstudio/database.conf configuration file.")
      ("db-command",
      value<std::string>(&dbCommand_)->default_value(std::string()),
      "Executes the shell command specified injecting the current database configuration in the command.")
      ("db-connection-timeout",
      value<int>(&dbConnectionTimeout_)->default_value(15),
      "Specifies the number of seconds to wait for making a new db connection");

   pAuth->add_options()
      ("auth-none",
      value<bool>(&authNone_)->default_value(!core::system::effectiveUserIsRoot()),
      "If set, disables multi-user authentication. Workbench/Pro features may not work in this mode.")
      ("auth-validate-users",
      value<bool>(&authValidateUsers_)->default_value(core::system::effectiveUserIsRoot()),
      "Indicates whether or not to validate that authenticated users exist on the target system. Disabling this option may cause issues to start or to run a session.")
      ("auth-stay-signed-in-days",
      value<int>(&authStaySignedInDays_)->default_value(30),
      "The number of days to keep a user signed in when using the \"Stay Signed In\" option. Will only take affect when auth-timeout-minutes is 0 (disabled).")
      ("auth-timeout-minutes",
      value<int>(&authTimeoutMinutes_)->default_value(60),
      "The number of minutes a user will stay logged in while idle before required to sign in again. Set this to 0 (disabled) to enable legacy timeout auth-stay-signed-in-days.")
      ("auth-encrypt-password",
      value<bool>(&authEncryptPassword_)->default_value(true),
      "Indicates whether or not to encrypt the password sent from the login form. For security purposes, we strongly recommend you leave this enabled.")
      ("auth-login-page-html",
      value<std::string>(pAuthLoginPageHtml)->default_value("/etc/rstudio/login.html"),
      "The path to a file containing additional HTML customization for the login page.")
      ("auth-rdp-login-page-html",
      value<std::string>(pAuthRdpLoginPageHtml)->default_value("/etc/rstudio/rdplogin.html"),
      "The path to a file containing additional HTML customization for the login page, as seen by RDP users.")
      ("auth-required-user-group",
      value<std::string>(&authRequiredUserGroup_)->default_value(""),
      "Specifies a group that users must be in to be able to use RStudio.")
      ("auth-minimum-user-id",
      value<std::string>(pAuthMinimumUserId)->default_value("auto"),
      "Specifies a minimum user id value. Users with a uid lower than this value may not use RStudio.")
      ("auth-pam-helper-path",
      value<std::string>(&authPamHelperPath_)->default_value("rserver-pam"),
      "The relative path from the RStudio installation directory, or absolute path where the PAM helper binary resides.")
      ("auth-pam-require-password-prompt",
      value<bool>(&authPamRequirePasswordPrompt_)->default_value(true),
      "Indicates whether or not to require the \"Password: \" prompt before sending the password via PAM. In most cases, this should be enabled. If using a custom PAM password prompt, you may need to disable this setting if PAM logins do not work correctly.")
      ("auth-pam-requires-priv",
      value<bool>(&deprecatedAuthPamRequiresPriv_)->default_value(true),
      "Deprecated - will always be true.")
      ("auth-sign-in-throttle-seconds",
      value<int>(&authSignInThrottleSeconds_)->default_value(5),
      "The minimum amount of time a user must wait before attempting to sign in again after signing out.")
      ("auth-revocation-list-dir",
      value<std::string>(&authRevocationListDir_)->default_value(""),
      "If set, overrides the path to the directory which contains the revocation list to be used for storing expired tokens. As of RStudio Server 1.4, this has been moved to database storage, and so this setting is deprecated, but will be used to port over any existing file-based expired tokens.")
      ("auth-cookies-force-secure",
      value<bool>(&authCookiesForceSecure_)->default_value(false),
      "Indicates whether or not auth cookies should be forcefully marked as secure. This should be enabled if running an SSL terminator in front of RStudio Server. Otherwise, cookies will be marked secure if SSL is configured.");

   pMonitor->add_options()
      (kMonitorIntervalSeconds,
      value<int>(&monitorIntervalSeconds_)->default_value(60),
      "The interval in seconds at which the monitor is probed for new data.");

   FilePath defaultConfigPath = core::system::xdg::findSystemConfigFile("rserver configuration", "rserver.conf");
   std::string configFile = defaultConfigPath.exists() ?
      defaultConfigPath.getAbsolutePath() : "";
   return program_options::OptionsDescription("rserver", configFile);
}

public:
   bool verifyInstallation() const { return verifyInstallation_; }
   std::string serverWorkingDir() const { return serverWorkingDir_; }
   std::string serverUser() const { return serverUser_; }
   bool serverDaemonize() const { return serverDaemonize_; }
   std::string serverPidFile() const { return serverPidFile_; }
   bool serverSetUmask() const { return serverSetUmask_; }
   core::FilePath secureCookieKeyFile() const { return core::FilePath(secureCookieKeyFile_); }
   core::FilePath serverDataDir() const { return core::FilePath(serverDataDir_); }
   std::vector<std::string> serverAddHeaders() const { return serverAddHeaders_; }
   bool serverRunAutomation() const { return serverRunAutomation_; }
   std::string wwwAddress() const { return wwwAddress_; }
   std::string wwwRootPath() const { return wwwRootPath_; }
   std::string wwwLocalPath() const { return wwwLocalPath_; }
   core::FilePath wwwSymbolMapsPath() const { return core::FilePath(wwwSymbolMapsPath_); }
   bool wwwUseEmulatedStack() const { return wwwUseEmulatedStack_; }
   int wwwThreadPoolSize() const { return wwwThreadPoolSize_; }
   bool wwwProxyLocalhost() const { return wwwProxyLocalhost_; }
   bool wwwVerifyUserAgent() const { return wwwVerifyUserAgent_; }
   rstudio::core::http::Cookie::SameSite wwwSameSite() const { return wwwSameSite_; }
   std::string wwwFrameOrigin() const { return wwwFrameOrigin_; }
   bool wwwEnableOriginCheck() const { return wwwEnableOriginCheck_; }
   std::vector<boost::regex> wwwAllowedOrigins() const { return wwwAllowedOrigins_; }
   bool sessionUseFileStorage() const { return sessionUseFileStorage_; }
   int statsMonitorSeconds() const { return statsMonitorSeconds_; }
   std::string rsessionWhichR() const { return rsessionWhichR_; }
   std::string rsessionPath() const { return rsessionPath_; }
   std::string rldpathPath() const { return rldpathPath_; }
   std::string rsessionLdLibraryPath() const { return rsessionLdLibraryPath_; }
   std::string rsessionConfigFile() const { return rsessionConfigFile_; }
   int rsessionProxyMaxWaitSeconds() const { return rsessionProxyMaxWaitSeconds_; }
   std::string databaseConfigFile() const { return databaseConfigFile_; }
   std::string dbCommand() const { return dbCommand_; }
   int dbConnectionTimeout() const { return dbConnectionTimeout_; }
   bool authNone() const { return authNone_; }
   bool authValidateUsers() const { return authValidateUsers_; }
   int authStaySignedInDays() const { return authStaySignedInDays_; }
   int authTimeoutMinutes() const { return authTimeoutMinutes_; }
   bool authEncryptPassword() const { return authEncryptPassword_; }
   std::string authLoginPageHtml() const { return authLoginPageHtml_; }
   std::string authRdpLoginPageHtml() const { return authRdpLoginPageHtml_; }
   std::string authRequiredUserGroup() const { return authRequiredUserGroup_; }
   unsigned int authMinimumUserId() const { return authMinimumUserId_; }
   std::string authPamHelperPath() const { return authPamHelperPath_; }
   bool authPamRequirePasswordPrompt() const { return authPamRequirePasswordPrompt_; }
   int authSignInThrottleSeconds() const { return authSignInThrottleSeconds_; }
   core::FilePath authRevocationListDir() const { return core::FilePath(authRevocationListDir_); }
   bool authCookiesForceSecure() const { return authCookiesForceSecure_; }
   int monitorIntervalSeconds() const { return monitorIntervalSeconds_; }


protected:
   bool verifyInstallation_;
   std::string serverWorkingDir_;
   std::string serverUser_;
   bool serverDaemonize_;
   std::string serverPidFile_;
   bool deprecatedServerAppArmorEnabled_;
   bool serverSetUmask_;
   std::string secureCookieKeyFile_;
   std::string serverDataDir_;
   std::vector<std::string> serverAddHeaders_;
   bool serverRunAutomation_;
   std::string wwwAddress_;
   std::string wwwPort_;
   std::string wwwSocket_;
   std::string wwwRootPath_;
   std::string wwwLocalPath_;
   std::string wwwSymbolMapsPath_;
   bool wwwUseEmulatedStack_;
   int wwwThreadPoolSize_;
   bool wwwProxyLocalhost_;
   bool wwwVerifyUserAgent_;
   rstudio::core::http::Cookie::SameSite wwwSameSite_;
   std::string wwwFrameOrigin_;
   bool wwwEnableOriginCheck_;
   std::vector<boost::regex> wwwAllowedOrigins_;
   bool sessionUseFileStorage_;
   int statsMonitorSeconds_;
   std::string rsessionWhichR_;
   std::string rsessionPath_;
   std::string rldpathPath_;
   std::string rsessionLdLibraryPath_;
   std::string rsessionConfigFile_;
   int rsessionProxyMaxWaitSeconds_;
   int deprecatedMemoryLimitMb_;
   int deprecatedStackLimitMb_;
   int deprecatedUserProcessLimit_;
   std::string databaseConfigFile_;
   std::string dbCommand_;
   int dbConnectionTimeout_;
   bool authNone_;
   bool authValidateUsers_;
   int authStaySignedInDays_;
   int authTimeoutMinutes_;
   bool authEncryptPassword_;
   std::string authLoginPageHtml_;
   std::string authRdpLoginPageHtml_;
   std::string authRequiredUserGroup_;
   unsigned int authMinimumUserId_;
   std::string authPamHelperPath_;
   bool authPamRequirePasswordPrompt_;
   bool deprecatedAuthPamRequiresPriv_;
   int authSignInThrottleSeconds_;
   std::string authRevocationListDir_;
   bool authCookiesForceSecure_;
   int monitorIntervalSeconds_;
   virtual bool allowOverlay() const { return false; };
};

} // namespace server
} // namespace rstudio


#endif // SERVER_SERVER_OPTIONS_GEN_HPP
