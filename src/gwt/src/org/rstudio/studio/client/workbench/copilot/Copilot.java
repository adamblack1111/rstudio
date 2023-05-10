/*
 * Copilot.java
 *
 * Copyright (C) 2023 by Posit Software, PBC
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
package org.rstudio.studio.client.workbench.copilot;

import org.rstudio.core.client.CommandWithArg;
import org.rstudio.core.client.Debug;
import org.rstudio.core.client.JSON;
import org.rstudio.core.client.MessageDisplay;
import org.rstudio.core.client.StringUtil;
import org.rstudio.core.client.command.CommandBinder;
import org.rstudio.core.client.command.Handler;
import org.rstudio.core.client.widget.ModalDialogBase;
import org.rstudio.studio.client.common.DelayedProgressRequestCallback;
import org.rstudio.studio.client.common.GlobalDisplay;
import org.rstudio.studio.client.server.ServerError;
import org.rstudio.studio.client.server.ServerRequestCallback;
import org.rstudio.studio.client.workbench.commands.Commands;
import org.rstudio.studio.client.workbench.copilot.model.CopilotConstants;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponseTypes.CopilotInstallAgentResponse;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponseTypes.CopilotSignInResponse;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponseTypes.CopilotSignInResponseResult;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponseTypes.CopilotSignOutResponse;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponseTypes.CopilotStatusResponse;
import org.rstudio.studio.client.workbench.copilot.model.CopilotResponseTypes.CopilotVerifyInstalledResponse;
import org.rstudio.studio.client.workbench.copilot.server.CopilotServerOperations;
import org.rstudio.studio.client.workbench.copilot.ui.CopilotInstallDialog;
import org.rstudio.studio.client.workbench.copilot.ui.CopilotSignInDialog;
import org.rstudio.studio.client.workbench.views.source.SourceColumnManager;

import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.user.client.Timer;
import com.google.inject.Inject;
import com.google.inject.Provider;
import com.google.inject.Singleton;

@Singleton
public class Copilot
{
   @Inject
   public Copilot(GlobalDisplay display,
                  Commands commands,
                  Provider<SourceColumnManager> sourceColumnManager,
                  GlobalDisplay globalDisplay,
                  CopilotCommandBinder binder,
                  CopilotServerOperations server)
   {
      display_ = display;
      commands_ = commands;
      sourceColumnManager_ = sourceColumnManager;
      globalDisplay_ = globalDisplay;
      server_ = server;
      
      binder.bind(commands, this);
   }
   
   public void ensureAgentInstalled(CommandWithArg<Boolean> callback)
   {
      String progressLabel = "Verifying copilot installation...";
      server_.copilotVerifyInstalled(
            new DelayedProgressRequestCallback<CopilotVerifyInstalledResponse>(progressLabel)
      {
         @Override
         protected void onSuccess(CopilotVerifyInstalledResponse response)
         {
            if (response.installed)
            {
               callback.execute(true);
            }
            else
            {
               installAgentWithPrompt(callback);
            }
         }
      });
   }
   
   private void installAgentWithPrompt(CommandWithArg<Boolean> callback)
   {
      ClickHandler handler = (event) ->
      {
         installAgent(callback);
      };
      
      CopilotInstallDialog dialog = new CopilotInstallDialog(handler);
      dialog.showModal();
   }
   
   private void installAgent(CommandWithArg<Boolean> callback)
   {
      String progressLabel = "Installing...";
      server_.copilotInstallAgent(
            new DelayedProgressRequestCallback<CopilotInstallAgentResponse>(progressLabel)
            {
               @Override
               protected void onSuccess(CopilotInstallAgentResponse response)
               {
                  if (response.installed)
                  {
                     display_.showMessage(
                           MessageDisplay.MSG_INFO,
                           "GitHub Copilot: Install Agent",
                           "GitHub Copilot agent successfully installed.");
                  }
                  else
                  {
                     display_.showErrorMessage("An error occurred while installing GitHub Copilot.");
                  }
                  
                  callback.execute(response.installed);
               }
            });
   }
   
   @Handler
   public void onCopilotInstallAgent()
   {
      installAgentWithPrompt(installed -> {});
   }
   
   @Handler
   public void onCopilotSignIn()
   {
      server_.copilotSignIn(new DelayedProgressRequestCallback<CopilotSignInResponse>("Signing in...")
      {
         private ModalDialogBase signInDialog_;
         private Timer statusTimer_;
         
         @Override
         protected void onSuccess(CopilotSignInResponse response)
         {
            CopilotSignInResponseResult result = response.result.cast();
            if (result.status == CopilotConstants.STATUS_PROMPT_USER_DEVICE_FLOW)
            {
               // Generate the dialog.
               signInDialog_ = new CopilotSignInDialog(result.verificationUri, result.userCode);
               signInDialog_.showModal();
               
               // Start polling for status, to see when the user has finished authenticating.
               statusTimer_ = new Timer()
               {
                  @Override
                  public void run()
                  {
                     server_.copilotStatus(new ServerRequestCallback<CopilotStatusResponse>()
                     {
                        @Override
                        public void onResponseReceived(CopilotStatusResponse response)
                        {
                           if (response.result.status == CopilotConstants.STATUS_OK)
                           {
                              signInDialog_.closeDialog();
                              
                              globalDisplay_.showMessage(
                                    MessageDisplay.MSG_INFO,
                                    "GitHub Copilot: Sign in",
                                    "You are now signed in as '" + response.result.user + "'.");
                           }
                           else
                           {
                              statusTimer_.schedule(1000);
                           }
                        }

                        @Override
                        public void onError(ServerError error)
                        {
                           Debug.logError(error);
                        }
                     });
                  }
               };
               
               statusTimer_.schedule(1000);
            }
            else if (result.status == CopilotConstants.STATUS_ALREADY_SIGNED_IN)
            {
               String message =
                     "You are already signed in as '" + result.user + "'.\n\n" +
                     "If you'd like to sign in as a different user, please " +
                     "sign out from this account first.";
                     
               globalDisplay_.showMessage(
                     MessageDisplay.MSG_INFO,
                     "GitHub Copilot: Sign in",
                     message);
            }
            else
            {
               showGenericResponseMessage(
                     "GitHub Copilot: Sign In",
                     response.result);
            }
         }
      });
   }
   
   @Handler
   public void onCopilotSignOut()
   {
      server_.copilotSignOut(new DelayedProgressRequestCallback<CopilotSignOutResponse>("Signing out...")
      {
         @Override
         protected void onSuccess(CopilotSignOutResponse response)
         {
            String status = response.result.status;
            if (StringUtil.equals(status, CopilotConstants.STATUS_NOT_SIGNED_IN))
            {
               display_.showMessage(
                     MessageDisplay.MSG_INFO,
                     "GitHub Copilot: Sign out",
                     "You have successfully signed out from GitHub Copilot.");
            }
            else
            {
               showGenericResponseMessage(
                     "GitHub Copilot: Sign out",
                     response.result);
            }
         }
      });
   }
   
   @Handler
   public void onCopilotStatus()
   {
      server_.copilotStatus(new DelayedProgressRequestCallback<CopilotStatusResponse>("Checking status...")
      {
         @Override
         protected void onSuccess(CopilotStatusResponse response)
         {
            if (response.result.status == CopilotConstants.STATUS_NOT_SIGNED_IN)
            {
               display_.showMessage(
                     MessageDisplay.MSG_INFO,
                     "GitHub Copilot: Status",
                     "The GitHub Copilot agent is running, but you have not yet signed in.");
            }
            else if (response.result.status == CopilotConstants.STATUS_OK)
            {
               display_.showMessage(
                     MessageDisplay.MSG_INFO,
                     "GitHub Copilot: Status",
                     "You are currently signed in as user '" + response.result.user + "'.");
            }
            else
            {
               showGenericResponseMessage(
                     "GitHub Copilot: Check Status",
                     response.result);
            }
            
            Debug.logObject(response);
         }
      });
   }
   
   private void showGenericResponseMessage(String title, Object object)
   {
      globalDisplay_.showMessage(
            MessageDisplay.MSG_INFO,
            title,
            JSON.stringify(object, 4));
   }
   
   interface CopilotCommandBinder
         extends CommandBinder<Commands, Copilot>
   {
   }
   
   private final GlobalDisplay display_;
   private final Commands commands_;
   private final Provider<SourceColumnManager> sourceColumnManager_;
   private final GlobalDisplay globalDisplay_;
   private final CopilotServerOperations server_;
}
