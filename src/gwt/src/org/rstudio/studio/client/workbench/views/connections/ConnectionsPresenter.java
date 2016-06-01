/*
 * ConnectionsPresenter.java
 *
 * Copyright (C) 2009-12 by RStudio, Inc.
 *
 * This program is licensed to you under the terms of version 3 of the
 * GNU Affero General Public License. This program is distributed WITHOUT
 * ANY EXPRESS OR IMPLIED WARRANTY, INCLUDING THOSE OF NON-INFRINGEMENT,
 * MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE. Please refer to the
 * AGPL (http://www.gnu.org/licenses/agpl-3.0.txt) for more details.
 *
 */
package org.rstudio.studio.client.workbench.views.connections;

import java.util.ArrayList;
import java.util.List;

import com.google.gwt.core.client.JsArray;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.HasClickHandlers;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.event.shared.HandlerRegistration;
import com.google.gwt.user.client.Command;
import com.google.gwt.view.client.SelectionChangeEvent;
import com.google.inject.Inject;

import org.rstudio.core.client.ListUtil;
import org.rstudio.core.client.ListUtil.FilterPredicate;
import org.rstudio.core.client.command.CommandBinder;
import org.rstudio.core.client.command.Handler;
import org.rstudio.core.client.dom.DomUtils;
import org.rstudio.core.client.events.EnsureHeightEvent;
import org.rstudio.core.client.js.JsObject;
import org.rstudio.core.client.widget.MessageDialog;
import org.rstudio.core.client.widget.Operation;
import org.rstudio.core.client.widget.OperationWithInput;
import org.rstudio.studio.client.application.events.EventBus;
import org.rstudio.studio.client.common.DelayedProgressRequestCallback;
import org.rstudio.studio.client.common.GlobalDisplay;
import org.rstudio.studio.client.common.SimpleRequestCallback;
import org.rstudio.studio.client.common.console.ConsoleProcess;
import org.rstudio.studio.client.common.console.ProcessExitEvent;
import org.rstudio.studio.client.server.VoidServerRequestCallback;
import org.rstudio.studio.client.workbench.WorkbenchListManager;
import org.rstudio.studio.client.workbench.WorkbenchView;
import org.rstudio.studio.client.workbench.commands.Commands;
import org.rstudio.studio.client.workbench.model.ClientState;
import org.rstudio.studio.client.workbench.model.Session;
import org.rstudio.studio.client.workbench.model.SessionInfo;
import org.rstudio.studio.client.workbench.model.helper.JSObjectStateValue;
import org.rstudio.studio.client.workbench.views.BasePresenter;
import org.rstudio.studio.client.workbench.views.connections.events.ActiveConnectionsChangedEvent;
import org.rstudio.studio.client.workbench.views.connections.events.ConnectionListChangedEvent;
import org.rstudio.studio.client.workbench.views.connections.events.ConnectionUpdatedEvent;
import org.rstudio.studio.client.workbench.views.connections.events.ExploreConnectionEvent;
import org.rstudio.studio.client.workbench.views.connections.model.Connection;
import org.rstudio.studio.client.workbench.views.connections.model.ConnectionId;
import org.rstudio.studio.client.workbench.views.connections.model.ConnectionsServerOperations;
import org.rstudio.studio.client.workbench.views.connections.model.NewSparkConnectionContext;
import org.rstudio.studio.client.workbench.views.connections.model.SparkVersion;
import org.rstudio.studio.client.workbench.views.connections.ui.InstallInfoPanel;
import org.rstudio.studio.client.workbench.views.connections.ui.ComponentsNotInstalledDialogs;
import org.rstudio.studio.client.workbench.views.connections.ui.NewSparkConnectionDialog;
import org.rstudio.studio.client.workbench.views.connections.ui.NewSparkConnectionDialog.Result;
import org.rstudio.studio.client.workbench.views.console.events.SendToConsoleEvent;
import org.rstudio.studio.client.workbench.views.source.events.NewDocumentWithCodeEvent;
import org.rstudio.studio.client.workbench.views.source.model.SourcePosition;
import org.rstudio.studio.client.workbench.views.vcs.common.ConsoleProgressDialog;

public class ConnectionsPresenter extends BasePresenter 
{
   public interface Display extends WorkbenchView
   {
      void setConnections(List<Connection> connections);
      void setActiveConnections(List<ConnectionId> connections);
      
      boolean isConnected(ConnectionId id);
      
      Connection getSelectedConnection();
          
      HandlerRegistration addSelectedConnectionChangeHandler(
                                 SelectionChangeEvent.Handler handler);
      
      String getSearchFilter();
      
      HandlerRegistration addSearchFilterChangeHandler(
                                       ValueChangeHandler<String> handler);
      
      HandlerRegistration addExploreConnectionHandler(
                                       ExploreConnectionEvent.Handler handler);
      
      void showConnectionExplorer(Connection connection);
      
      
      HasClickHandlers backToConnectionsButton();
      
      void showConnectionsList();
   }
   
   public interface Binder extends CommandBinder<Commands, ConnectionsPresenter> {}
   
   @Inject
   public ConnectionsPresenter(Display display, 
                               ConnectionsServerOperations server,
                               GlobalDisplay globalDisplay,
                               EventBus eventBus,
                               Binder binder,
                               final Commands commands,
                               WorkbenchListManager listManager,
                               Session session)
   {
      super(display);
      binder.bind(commands, this);
      display_ = display;
      commands_ = commands;
      server_ = server;
      globalDisplay_ = globalDisplay;
      eventBus_ = eventBus;
         
       
      // track selected connection
      manageCommands();
      display_.addSelectedConnectionChangeHandler(
                                       new SelectionChangeEvent.Handler() {
         @Override
         public void onSelectionChange(SelectionChangeEvent event)
         {
            manageCommands();
         }
      });
      
      // search filter
      display_.addSearchFilterChangeHandler(new ValueChangeHandler<String>() {

         @Override
         public void onValueChange(ValueChangeEvent<String> event)
         {
            display_.setConnections(filteredConnections());
         }
      });
      
      display_.addExploreConnectionHandler(new ExploreConnectionEvent.Handler()
      {   
         @Override
         public void onExploreConnection(ExploreConnectionEvent event)
         {
            exploreConnection(event.getConnection());
            display_.ensureHeight(EnsureHeightEvent.MAXIMIZED);
         }
      });
      
      display_.backToConnectionsButton().addClickHandler(new ClickHandler() {

         @Override
         public void onClick(ClickEvent event)
         {
            exploredConnection_ = null;
            display_.showConnectionsList();
            display_.ensureHeight(EnsureHeightEvent.NORMAL);
         }
         
      });
      
      // set connections      
      final SessionInfo sessionInfo = session.getSessionInfo();
      updateConnections(sessionInfo.getConnectionList());  
      updateActiveConnections(sessionInfo.getActiveConnections());
           
      // make the explored connection persistent
      new JSObjectStateValue(MODULE_CONNECTIONS, 
                             KEY_EXPLORED_CONNECTION, 
                             ClientState.PERSISTENT, 
                             session.getSessionInfo().getClientState(), 
                             false)
      {
         @Override
         protected void onInit(JsObject value)
         {
            // get the value
            if (value != null)
               exploredConnection_ = value.cast();
            else
               exploredConnection_ = null;
                 
            lastExploredConnection_ = exploredConnection_;
            
            // if there is an an explored connection then explore it
            if (exploredConnection_ != null)
               exploreConnection(exploredConnection_);
         }

         @Override
         protected JsObject getValue()
         {
            if (exploredConnection_ != null)
               return exploredConnection_.cast();
            else
               return null;
         }

         @Override
         protected boolean hasChanged()
         {
            if (lastExploredConnection_ != exploredConnection_)
            {
               lastExploredConnection_ = exploredConnection_;
               return true;
            }
            else
            {
               return false;
            }
         }
      };
   }
   
   public void activate()
   {
      display_.bringToFront();
   }
   
   public void onConnectionUpdated(ConnectionUpdatedEvent event)
   {     
   }
   
   public void onConnectionListChanged(ConnectionListChangedEvent event)
   {
      updateConnections(event.getConnectionList());
   }
   
   public void onActiveConnectionsChanged(ActiveConnectionsChangedEvent event)
   {
      updateActiveConnections(event.getActiveConnections());
   }
   
   public void onNewConnection()
   {
      // get the context
      server_.getNewSparkConnectionContext(
         new DelayedProgressRequestCallback<NewSparkConnectionContext>(
                                                   "New Connection...") {
   
            @Override
            protected void onSuccess(NewSparkConnectionContext context)
            {
               // prompt for no java installed
               if (!context.isJavaInstalled())
               {
                  ComponentsNotInstalledDialogs.showJavaNotInstalled(context.getJavaInstallUrl());
               }
               
               // prompt for no spark installed
               else if (context.getSparkVersions().length() == 0)
               {
                  ComponentsNotInstalledDialogs.showSparkNotInstalled();
               }
               
               // otherwise proceed with connecting
               else
               {
                  // show dialog
                  new NewSparkConnectionDialog(
                   context,
                   new OperationWithInput<NewSparkConnectionDialog.Result>() {
                     @Override
                     public void execute(final Result result)
                     {
                        withRequiredSparkInstallation(
                              result.getSparkVersion(),
                              result.getRemote(),
                              new Command() {
                                 @Override
                                 public void execute()
                                 {
                                    performConnection(result);
                                 }
                                 
                              });
                     }
                  }).showModal();
               }
            }
         });      
   }
   
   
   private void withRequiredSparkInstallation(final SparkVersion sparkVersion,
                                              boolean remote,
                                              final Command command)
   {
      if (!sparkVersion.isInstalled())
      {
         globalDisplay_.showYesNoMessage(
            MessageDialog.QUESTION, 
            "Install Spark Components",
            InstallInfoPanel.getInfoText(sparkVersion, remote, true),
            false,
            new Operation() {  public void execute() {
               server_.installSpark(
                 sparkVersion.getSparkVersionNumber(),
                 sparkVersion.getHadoopVersionNumber(),
                 new SimpleRequestCallback<ConsoleProcess>(){

                    @Override
                    public void onResponseReceived(ConsoleProcess process)
                    {
                       final ConsoleProgressDialog dialog = 
                             new ConsoleProgressDialog(process, server_);
                       dialog.showModal();
           
                       process.addProcessExitHandler(
                          new ProcessExitEvent.Handler()
                          {
                             @Override
                             public void onProcessExit(ProcessExitEvent event)
                             {
                                if (event.getExitCode() == 0)
                                {
                                   dialog.hide();
                                   command.execute();
                                } 
                             }
                          }); 
                    }

               });   
            }},
            null,
            null,
            "Install",
            "Cancel",
            true);
      }
      else
      {
         command.execute();
      }
   }
   
  
   
   private void performConnection(NewSparkConnectionDialog.Result result)
   {
      String connectVia = result.getConnectVia();
      if (connectVia.equals(
            Result.CONNECT_COPY_TO_CLIPBOARD))
      {
         DomUtils.copyCodeToClipboard(result.getConnectCode());
      }
      else if (connectVia.equals(Result.CONNECT_R_CONSOLE))
      {
         eventBus_.fireEvent(
               new SendToConsoleEvent(result.getConnectCode(), true));
      }
      else if (connectVia.equals(Result.CONNECT_NEW_R_SCRIPT) ||
               connectVia.equals(Result.CONNECT_NEW_R_NOTEBOOK))
      {
         String type;
         String code = result.getConnectCode();
         SourcePosition cursorPosition = null;
         if (connectVia.equals(Result.CONNECT_NEW_R_SCRIPT))
         {
            type = NewDocumentWithCodeEvent.R_SCRIPT;
            code = code + "\n\n";
         }
         else
         {
            type = NewDocumentWithCodeEvent.R_NOTEBOOK; 
            code = "---\n" +
                   "title: \"R Notebook\"\n" +
                   "output: html_notebook\n" +
                   "---\n" +
                   "\n" +
                   "```{r connect}\n" +
                   code + "\n" +
                   "```\n" +
                   "\n" +
                   "```{r}\n" +
                   "\n" +
                   "```\n";
            cursorPosition = SourcePosition.create(11, 0);      
         }
        
         eventBus_.fireEvent(
            new NewDocumentWithCodeEvent(type, code, cursorPosition, true));
      }
   }
   
   @Handler
   public void onRemoveConnection()
   {
      final Connection connection = display_.getSelectedConnection();
      if (connection != null)
      {
         globalDisplay_.showYesNoMessage(
            MessageDialog.QUESTION,
            "Remove Connection",
            "Are you sure you want to remove the selected connection from " +
            "the list?", 
            new Operation() {
   
               @Override
               public void execute()
               {
                  server_.removeConnection(
                    connection.getId(), new VoidServerRequestCallback());   
               }
            },
            true);
      }
      else
      {
         globalDisplay_.showErrorMessage(
           "Remove Connection", "No connection currently selected.");
      }
   }
  
   @Handler
   public void onConnectConnection()
   {
      Connection selectedConnection = display_.getSelectedConnection();
      String connectCode = selectedConnection.getConnectCode();
      eventBus_.fireEvent(new SendToConsoleEvent(connectCode, true));
   }
   
   @Handler
   public void onDisconnectConnection()
   {
      Connection selectedConnection = display_.getSelectedConnection();
      server_.getDisconnectCode(selectedConnection, 
                                new SimpleRequestCallback<String>() {
         @Override
         public void onResponseReceived(String disconnectCode)
         {
            eventBus_.fireEvent(new SendToConsoleEvent(disconnectCode, true));
         }
      });
   }
   
   private void updateConnections(JsArray<Connection> connections)
   {
      // update all connections
      allConnections_.clear();
      for (int i = 0; i<connections.length(); i++)
         allConnections_.add(connections.get(i)); 
      
      // set filtered connections
      display_.setConnections(filteredConnections());
   }
   
   private void updateActiveConnections(JsArray<ConnectionId> connections)
   {
      activeConnections_.clear();
      for (int i = 0; i<connections.length(); i++)
         activeConnections_.add(connections.get(i));  
      display_.setActiveConnections(activeConnections_);
      manageCommands();
   }
   
   private void exploreConnection(Connection connection)
   {
      exploredConnection_ = connection;
      display_.showConnectionExplorer(connection);
   }
   
   private void manageCommands()
   {
      Connection selectedConnection = display_.getSelectedConnection();
      if (selectedConnection != null)
      {
         boolean connected = display_.isConnected(selectedConnection.getId());
         commands_.connectConnection().setVisible(!connected);  
         commands_.disconnectConnection().setVisible(connected);
      }
      else
      {
         commands_.connectConnection().setVisible(false);
         commands_.disconnectConnection().setVisible(false);
      }
   }
   
   private List<Connection> filteredConnections()
   {
      String query = display_.getSearchFilter();
      final String[] splat = query.toLowerCase().split("\\s+");
      return ListUtil.filter(allConnections_, 
                                   new FilterPredicate<Connection>()
      {
         @Override
         public boolean test(Connection connection)
         {
            for (String el : splat)
            {
               boolean match =
                   connection.getHost().toLowerCase().contains(el);
               if (!match)
                  return false;
            }
            return true;
         }
      });
   }
   
   private final GlobalDisplay globalDisplay_;
   
   private final Display display_ ;
   private final EventBus eventBus_;
   private final Commands commands_;
   private final ConnectionsServerOperations server_ ;
   
   // client state
   public static final String MODULE_CONNECTIONS = "connections-pane";
   private static final String KEY_EXPLORED_CONNECTION = "exploredConnection";
   private Connection exploredConnection_;
   private Connection lastExploredConnection_;
   
   private ArrayList<Connection> allConnections_ = new ArrayList<Connection>();
   private ArrayList<ConnectionId> activeConnections_ = new ArrayList<ConnectionId>();
}