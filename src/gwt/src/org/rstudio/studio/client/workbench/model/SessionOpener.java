/*
 * SessionOpener.java
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
package org.rstudio.studio.client.workbench.model;

import java.util.function.Consumer;

import javax.inject.Inject;

import org.rstudio.core.client.Debug;
import org.rstudio.core.client.StringUtil;
import org.rstudio.studio.client.application.Application;
import org.rstudio.studio.client.application.ApplicationAction;
import org.rstudio.studio.client.application.events.EventBus;
import org.rstudio.studio.client.application.model.ActiveSession;
import org.rstudio.studio.client.application.model.ApplicationServerOperations;
import org.rstudio.studio.client.application.model.RVersionSpec;
import org.rstudio.studio.client.application.model.SuspendOptions;
import org.rstudio.studio.client.common.GlobalDisplay;
import org.rstudio.studio.client.server.ServerError;
import org.rstudio.studio.client.server.ServerRequestCallback;
import org.rstudio.studio.client.server.VoidServerRequestCallback;
import org.rstudio.studio.client.workbench.views.console.events.ConsoleRestartRCompletedEvent;

import com.google.gwt.core.client.GWT;
import com.google.gwt.core.client.JavaScriptObject;
import com.google.gwt.core.client.Scheduler;
import com.google.gwt.core.client.Scheduler.RepeatingCommand;
import com.google.gwt.user.client.Command;
import com.google.inject.Provider;
import com.google.inject.Singleton;

@Singleton
public class SessionOpener
{
   @Inject
   public SessionOpener(Provider<Application> pApplication,
                        Provider<GlobalDisplay> pGlobalDisplay,
                        Provider<ApplicationServerOperations> pServer,
                        Provider<EventBus> pEventBus)
   {
      pApplication_ = pApplication;
      pDisplay_ = pGlobalDisplay;
      pServer_ = pServer;
      pEventBus_ = pEventBus;
   }

   /**
    * Prepare for navigation to a session
    * @param session
    * @param navigate url to load
    */
   public void navigateToActiveSession(ActiveSession session,
                                       Consumer<String> navigate)
   {
        navigateToSession(session.getUrl(), navigate);
   }
   
   /**
    * Prepare for navigation to a session
    * @param sessionUrl
    * @param navigate url to load
    */
   protected void navigateToSession(String sessionUrl,
                                    Consumer<String> navigate)
   {
      navigate.accept(sessionUrl);
   }
   
   /**
    * Switch to a session
    * @param nextSessionUrl session to switch to
    */
   public void switchSession(String nextSessionUrl)
   {
      // if we are switching projects then reload after a delay (to allow
      // the R session to fully exit on the server)
      if (!StringUtil.isNullOrEmpty(nextSessionUrl))
      {
         // forward any query string parameters (e.g. the edit_published
         // parameter might follow an action=switch_project)
         String query = ApplicationAction.getQueryStringWithoutAction();
         if (query.length() > 0)
            nextSessionUrl = nextSessionUrl + "?" + query;
         
         pApplication_.get().navigateWindowWithDelay(nextSessionUrl);
      }
      else
      {
         pApplication_.get().reloadWindowWithDelay(true);
      }
   }
   
   /**
    * Create a new session with supplied launchParams and
    * and prepare to navigate to it
    */
   public void navigateToNewSession(boolean isProject,
                                    String directory,
                                    RVersionSpec rVersion,
                                    JavaScriptObject launchParams,
                                    Consumer<String> navigate)
   {
      pServer_.get().getNewSessionUrl(
                    GWT.getHostPageBaseURL(),
                    isProject,
                    directory,
                    rVersion,
                    launchParams,
        new ServerRequestCallback<String>() {

         @Override
         public void onResponseReceived(String url)
         {
            navigate.accept(url);
         }
         
         @Override
         public void onError(ServerError error)
         {
            Debug.logError(error);
            pDisplay_.get().showErrorMessage(
                  constants_.createSessionCaption(),
                  constants_.createSessionMessage() +
                        (!StringUtil.isNullOrEmpty(error.getMessage()) ?
                        "\n\n" + error.getMessage() : ""));
         }
      });
   }
   
   /**
    * Create a new session and prepare to navigate to it. Prompts for
    * launch params if necessary.
    */
   public void navigateToNewSession(boolean isProject,
                                    String directory,
                                    RVersionSpec rVersion,
                                    Consumer<String> navigate)
   {
      navigateToNewSession(isProject, directory, rVersion, null, /*launchParams*/ navigate);
   }
   
   /**
    * Suspend and restart current session
    */
   public void suspendForRestart(SuspendOptions options,
                                 Command onCompleted,
                                 Command onFailed)
   {
      pServer_.get().suspendForRestart(options, new VoidServerRequestCallback()
      {
         @Override
         protected void onSuccess()
         {
            waitForSessionJobExit(
                  () -> waitForSessionRestart(onCompleted),
                  onFailed);
         }
         @Override
         protected void onFailure()
         {
            onFailed.execute();
         }
      });
   }

   /**
    *  Streams the session job's current connection details
    */
   public void getJobConnectionStatus(final ServerRequestCallback<String> connectionStatusCallback)
   {
   }
   
   protected void waitForSessionJobExit(Command onClosed, Command onFailure)
   {
      // for regular sessions, no job to wait for
      onClosed.execute();
   }
   
   protected void waitForSessionRestart(Command onCompleted)
   {
      sendPing(200, 25, onCompleted);
   }
   
   private void sendPing(int delayMs,
                         final int maxRetries,
                         final Command onCompleted)
   {
      Scheduler.get().scheduleFixedDelay(new RepeatingCommand()
      {
         private long startTime_ = System.currentTimeMillis();
         private int retries_ = 0;
         private boolean pingDelivered_ = false;
         private boolean pingInFlight_ = false;
         
         @Override
         public boolean execute()
         {
            // if we've already delivered the ping, return false
            if (pingDelivered_)
            {
               return false;
            }
            
            // if we hit our retry count, give up
            if (retries_++ > maxRetries)
            {
               Debug.logWarning("Error connecting with session.");
               return false;
            }
            
            // if we have a ping in flight, but we don't receive a response
            // after a few seconds, try again
            long currentTime = System.currentTimeMillis();
            if (pingInFlight_ && currentTime - startTime_ > 1000)
            {
               startTime_ = currentTime;
               pingInFlight_ = false;
            }
            
            if (!pingInFlight_)
            {
               pingInFlight_ = true;
               pServer_.get().ping(new VoidServerRequestCallback()
               {
                  @Override
                  protected void onSuccess()
                  {
                     pingInFlight_ = false;
                     
                     // if the ping was already handled separately, discard this
                     if (pingDelivered_)
                        return;
                     
                     pingDelivered_ = true;
                     pEventBus_.get().fireEvent(new ConsoleRestartRCompletedEvent());
                     
                     if (onCompleted != null)
                        onCompleted.execute();
                  }
                  
                  @Override
                  protected void onFailure()
                  {
                     pingInFlight_ = false;
                     
                     // if the ping was already handled separately, discard this
                     if (pingDelivered_)
                        return;
                     
                     if (onCompleted != null)
                        onCompleted.execute();
                  }
               });
            }
            
            // keep trying until the ping is delivered
            return true;
         }
         
      }, delayMs);
   }
   
   // injected
   protected final Provider<Application> pApplication_;
   protected final Provider<GlobalDisplay> pDisplay_;
   protected final Provider<ApplicationServerOperations> pServer_;
   protected final Provider<EventBus> pEventBus_;
   private static final ModelConstants constants_ = GWT.create(ModelConstants.class);
}
