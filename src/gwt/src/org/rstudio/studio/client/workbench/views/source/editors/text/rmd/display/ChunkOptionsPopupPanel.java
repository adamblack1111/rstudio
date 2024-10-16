/*
 * ChunkOptionsPopupPanel.java
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
package org.rstudio.studio.client.workbench.views.source.editors.text.rmd.display;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import org.rstudio.core.client.ElementIds;
import org.rstudio.core.client.StringUtil;
import org.rstudio.core.client.dom.DomUtils;
import org.rstudio.core.client.dom.DomUtils.NativeEventHandler;
import org.rstudio.core.client.files.FileSystemItem;
import org.rstudio.core.client.widget.FormLabel;
import org.rstudio.core.client.widget.LayoutGrid;
import org.rstudio.core.client.widget.MiniPopupPanel;
import org.rstudio.core.client.widget.ProgressIndicator;
import org.rstudio.core.client.widget.ProgressOperationWithInput;
import org.rstudio.core.client.widget.SmallButton;
import org.rstudio.core.client.widget.TextBoxWithCue;
import org.rstudio.core.client.widget.Toggle;
import org.rstudio.core.client.widget.VerticalSpacer;
import org.rstudio.core.client.widget.Toggle.State;
import org.rstudio.studio.client.RStudioGinjector;
import org.rstudio.studio.client.common.FileDialogs;
import org.rstudio.studio.client.common.FilePathUtils;
import org.rstudio.studio.client.common.HelpLink;
import org.rstudio.studio.client.workbench.model.RemoteFileSystemContext;
import org.rstudio.studio.client.workbench.views.source.ViewsSourceConstants;
import org.rstudio.studio.client.workbench.views.source.editors.text.DocDisplay;
import org.rstudio.studio.client.workbench.views.source.editors.text.ace.Position;

import com.google.gwt.core.client.GWT;
import com.google.gwt.core.client.Scheduler;
import com.google.gwt.dom.client.NativeEvent;
import com.google.gwt.dom.client.Style.Unit;
import com.google.gwt.event.dom.client.ClickEvent;
import com.google.gwt.event.dom.client.ClickHandler;
import com.google.gwt.event.dom.client.KeyCodes;
import com.google.gwt.event.dom.client.KeyUpEvent;
import com.google.gwt.event.logical.shared.ValueChangeEvent;
import com.google.gwt.event.logical.shared.ValueChangeHandler;
import com.google.gwt.resources.client.ClientBundle;
import com.google.gwt.resources.client.CssResource;
import com.google.gwt.user.client.Command;
import com.google.gwt.user.client.ui.FlowPanel;
import com.google.gwt.user.client.ui.Grid;
import com.google.gwt.user.client.ui.HorizontalPanel;
import com.google.gwt.user.client.ui.Label;
import com.google.gwt.user.client.ui.ListBox;
import com.google.gwt.user.client.ui.TextBox;
import com.google.gwt.user.client.ui.VerticalPanel;
import com.google.inject.Inject;

public abstract class ChunkOptionsPopupPanel extends MiniPopupPanel
{
   // Sub-classes must implement these methods.
   //
   // initOptions should fill the 'chunkOptions_' map and call 'afterInit'
   // after this has completed.
   //
   // synchronize should modify the document to reflect the current state
   // of the UI selection.
   //
   // revert should return the document state to how it was before editing
   // was initiated.
   protected abstract void initOptions(Command afterInit);
   protected abstract void synchronize();
   protected abstract void revert();
   private static final ViewsSourceConstants constants_ = GWT.create(ViewsSourceConstants.class);
   @Inject
   private void initialize(FileDialogs fileDialogs,
                           RemoteFileSystemContext rfsContext)
   {
      fileDialogs_ = fileDialogs;
      rfsContext_ = rfsContext;
   }
   
   public ChunkOptionsPopupPanel(boolean includeChunkNameUI, boolean isVisualEditor)
   {
      super(true);
      setVisible(false);
      
      RStudioGinjector.INSTANCE.injectMembers(this);
      
      isVisualEditor_ = isVisualEditor;
      chunkOptions_ = new HashMap<>();
      originalChunkOptions_ = new HashMap<>();
      
      panel_ = new VerticalPanel();
      add(panel_);
      
      header_ = new Label();
      header_.addStyleName(RES.styles().headerLabel());
      header_.setVisible(false);
      panel_.add(header_);
      
      tbChunkLabel_ = new TextBoxWithCue(constants_.unnamedChunk());
      tbChunkLabel_.addStyleName(RES.styles().textBox());
      tbChunkLabel_.addChangeHandler(changeEvent -> synchronize());
      tbChunkLabel_.setElementId(ElementIds.getElementId(ElementIds.CHUNK_OPTIONS_NAME));
      
      panel_.addHandler(keyUpEvent ->
      {
         int keyCode = keyUpEvent.getNativeKeyCode();
         if (keyCode == KeyCodes.KEY_ESCAPE ||
             keyCode == KeyCodes.KEY_ENTER)
         {
            ChunkOptionsPopupPanel.this.hide();
            display_.focus();
            return;
         }
      }, KeyUpEvent.getType());
      
      tbChunkLabel_.addKeyUpHandler(keyUpEvent ->
      {
         int keyCode = keyUpEvent.getNativeKeyCode();
         if (keyCode == KeyCodes.KEY_ESCAPE ||
             keyCode == KeyCodes.KEY_ENTER)
         {
            ChunkOptionsPopupPanel.this.hide();
            display_.focus();
            return;
         }
         
         synchronize();
         
      });
      
      int gridRows = includeChunkNameUI ? 2 : 1;
      LayoutGrid nameAndOutputGrid = new LayoutGrid(gridRows, 2);

      chunkLabel_ = new FormLabel(constants_.chunkNameColon(), tbChunkLabel_);
      chunkLabel_.addStyleName(RES.styles().chunkLabel());
      
      if (includeChunkNameUI)
         nameAndOutputGrid.setWidget(0, 0, chunkLabel_);

      tbChunkLabel_.addStyleName(RES.styles().chunkName());
      
      if (includeChunkNameUI)
         nameAndOutputGrid.setWidget(0, 1, tbChunkLabel_);
      
      outputComboBox_ = new ListBox();
      String[] options = new String[] {
            OUTPUT_USE_DOCUMENT_DEFAULT,
            OUTPUT_SHOW_OUTPUT_ONLY,
            OUTPUT_SHOW_CODE_AND_OUTPUT,
            OUTPUT_SHOW_NOTHING,
            OUTPUT_SKIP_THIS_CHUNK
      };
      
      for (String option : options)
         outputComboBox_.addItem(option);
      
      outputComboBox_.addChangeHandler(changeEvent ->
      {
         String value = outputComboBox_.getItemText(outputComboBox_.getSelectedIndex());
         if (value == OUTPUT_USE_DOCUMENT_DEFAULT)
         {
            unset("echo");
            unset("eval");
            unset("include");
         }
         else if (value == OUTPUT_SHOW_CODE_AND_OUTPUT)
         {
            set("echo", "TRUE");
            unset("eval");
            unset("include");
         }
         else if (value == OUTPUT_SHOW_OUTPUT_ONLY)
         {
            set("echo", "FALSE");
            unset("eval");
            unset("include");
         }
         else if (value == OUTPUT_SHOW_NOTHING)
         {
            unset("echo");
            unset("eval");
            set("include", "FALSE");
         }
         else if (value == OUTPUT_SKIP_THIS_CHUNK)
         {
            set("eval", "FALSE");
            set("include", "FALSE");
            unset("echo");
         }
         synchronize();
      });
      
      int row = includeChunkNameUI ? 1 : 0;
      FormLabel outputLabel = new FormLabel(constants_.outputColon());
      nameAndOutputGrid.setWidget(row, 0, outputLabel);
      nameAndOutputGrid.setWidget(row, 1, outputComboBox_);
      ElementIds.assignElementId(outputComboBox_, ElementIds.CHUNK_OPTIONS_OUTPUT);
      outputLabel.setFor(outputComboBox_);
      
      panel_.add(nameAndOutputGrid);
      
      showWarningsInOutputCb_ = makeTriStateToggle(
            constants_.showWarnings(),
            "warning",
            ElementIds.CHUNK_OPTIONS_WARNINGS);
      panel_.add(showWarningsInOutputCb_);

      showMessagesInOutputCb_ = makeTriStateToggle(
            constants_.showMessages(),
            "message",
            ElementIds.CHUNK_OPTIONS_MESSAGES);
      panel_.add(showMessagesInOutputCb_);

      cacheChunkCb_ = makeTriStateToggle(
            constants_.cacheChunk(),
            "cache",
            ElementIds.CHUNK_OPTIONS_CACHE);
      panel_.add(cacheChunkCb_);
      cacheChunkCb_.setVisible(false);

      printTableAsTextCb_ = makeTriStateToggle(
            constants_.usePagedTables(),
            "paged.print",
            ElementIds.CHUNK_OPTIONS_TABLES);
      panel_.add(printTableAsTextCb_);
      printTableAsTextCb_.setVisible(false);
      
      useCustomFigureCheckbox_ = new Toggle(
            constants_.useCustomFigureSize(),
            false,
            ElementIds.CHUNK_OPTIONS_FIGURESIZE);
      useCustomFigureCheckbox_.addStyleName(RES.styles().checkBox());
      useCustomFigureCheckbox_.addValueChangeHandler(new ValueChangeHandler<Toggle.State>()
      {
         @Override
         public void onValueChange(ValueChangeEvent<Toggle.State> event)
         {
            boolean value = event.getValue() == Toggle.State.ON;
            figureDimensionsPanel_.setVisible(value);
            
            if (!value)
            {
               figWidthBox_.setText("");
               figHeightBox_.setText("");
               unset("fig.width");
               unset("fig.height");
               synchronize();
            }
         }
      });
      panel_.add(useCustomFigureCheckbox_);
      
      figureDimensionsPanel_ = new Grid(2, 2);
      figureDimensionsPanel_.getElement().getStyle().setMarginTop(5, Unit.PX);
      
      figWidthBox_ = makeInputBox("fig.width", false);
      FormLabel widthLabel = new FormLabel(constants_.widthInchesColon(), figWidthBox_);
      widthLabel.getElement().getStyle().setMarginLeft(20, Unit.PX);
      figureDimensionsPanel_.setWidget(0, 0, widthLabel);
      figureDimensionsPanel_.setWidget(0, 1, figWidthBox_);
      
      figHeightBox_ = makeInputBox("fig.height", false);
      FormLabel heightLabel = new FormLabel(constants_.heightInchesColon(), figHeightBox_);
      heightLabel.getElement().getStyle().setMarginLeft(20, Unit.PX);
      figureDimensionsPanel_.setWidget(1, 0, heightLabel);
      figureDimensionsPanel_.setWidget(1, 1, figHeightBox_);
      
      panel_.add(figureDimensionsPanel_);
      
      enginePanel_ = new Grid(2, 3);
      enginePanel_.getElement().getStyle().setMarginTop(5, Unit.PX);
      
      enginePathBox_ = makeInputBox("engine.path", true);
      enginePathBox_.getElement().getStyle().setWidth(120, Unit.PX);
      Label enginePathLabel = new Label(constants_.enginePathColon());
      SmallButton enginePathBrowseButton = new SmallButton("...");
      enginePathBrowseButton.addClickHandler(new ClickHandler()
      {
         @Override
         public void onClick(ClickEvent event)
         {
            // infer the start navigation directory
            String path = enginePathBox_.getValue();
            FileSystemItem initialPath = path.isEmpty()
                  ? FileSystemItem.createDir("~/")
                  : FileSystemItem.createDir(FilePathUtils.dirFromFile(path));
            
            fileDialogs_.openFile(
                  constants_.selectEngine(),
                  rfsContext_,
                  initialPath,
                  new ProgressOperationWithInput<FileSystemItem>()
                  {
                     @Override
                     public void execute(FileSystemItem input, ProgressIndicator indicator)
                     {
                        if (input == null)
                        {
                           indicator.onCompleted();
                           return;
                        }
                        
                        String path = StringUtil.notNull(input.getPath());
                        path = path.replaceAll("\\\\", "\\\\\\\\");
                        enginePathBox_.setValue(path);
                        set("engine.path", StringUtil.ensureQuoted(path));
                        synchronize();
                        indicator.onCompleted();
                     }
                  });
         }
      });
      enginePanel_.setWidget(0, 0, enginePathLabel);
      enginePanel_.setWidget(0, 1, enginePathBox_);
      enginePanel_.setWidget(0, 2, enginePathBrowseButton);
      
      engineOptsBox_ = makeInputBox("engine.opts", true);
      engineOptsBox_.getElement().getStyle().setWidth(120, Unit.PX);
      Label engineOptsLabel = new Label(constants_.engineOptionsColon());
      enginePanel_.setWidget(1, 0, engineOptsLabel);
      enginePanel_.setWidget(1, 1, engineOptsBox_);
      
      panel_.add(enginePanel_);
      
      HorizontalPanel footerPanel = new HorizontalPanel();
      footerPanel.getElement().getStyle().setWidth(100, Unit.PCT);
      
      FlowPanel linkPanel = new FlowPanel();
      linkPanel.add(new VerticalSpacer("8px"));
      HelpLink helpLink = new HelpLink(constants_.chunkOptions(), "chunk-options", false);
      linkPanel.add(helpLink);
      
      HorizontalPanel buttonPanel = new HorizontalPanel();
      buttonPanel.addStyleName(RES.styles().buttonPanel());
      buttonPanel.setHorizontalAlignment(HorizontalPanel.ALIGN_RIGHT);
      
      revertButton_ = new SmallButton(constants_.revertCapitalized(), ElementIds.CHUNK_OPTIONS_REVERT);
      revertButton_.getElement().getStyle().setMarginRight(8, Unit.PX);
      revertButton_.addClickHandler(clickEvent ->
      {
         revert();
         hideAndFocusEditor();
      });
      buttonPanel.add(revertButton_);
      
      applyButton_ = new SmallButton(constants_.applyCapitalized(), ElementIds.CHUNK_OPTIONS_APPLY);
      applyButton_.addClickHandler(clickEvent ->
      {
         synchronize();
         hideAndFocusEditor();
      });
      buttonPanel.add(applyButton_);
      
      footerPanel.setVerticalAlignment(VerticalPanel.ALIGN_BOTTOM);
      footerPanel.add(linkPanel);
      
      footerPanel.setHorizontalAlignment(HorizontalPanel.ALIGN_RIGHT);
      footerPanel.add(buttonPanel);
      
      panel_.add(footerPanel);
   }
   
   protected void setHeader(String text, boolean visible)
   {
      header_.setText(text);
      header_.setVisible(visible);
   }
   
   public void focus()
   {
      tbChunkLabel_.setFocus(true);
   }
   
   private TextBox makeInputBox(final String option, final boolean enquote)
   {
      final TextBox box = new TextBox();
      DomUtils.setPlaceholder(box, "Default");
      box.setWidth("40px");
      
      DomUtils.addKeyHandlers(box, new NativeEventHandler()
      {
         @Override
         public void onNativeEvent(NativeEvent nativeEvent)
         {
            Scheduler.get().scheduleDeferred(() ->
            {
               String text = box.getText().trim();
               boolean isEmpty = StringUtil.isNullOrEmpty(text);
               
               if (enquote && !isEmpty)
               {
                  text = StringUtil.ensureQuoted(text);
                  text = text.replaceAll("\\\\", "\\\\\\\\");
               }
               
               if (isEmpty)
                  unset(option);
               else
                  set(option, text);
               
               synchronize();
            });
         }
      });
      
      return box;
   }
   
   private Toggle makeTriStateToggle(String label, final String option, final String id)
   {
      Toggle toggle = new Toggle(label, true, id);
      toggle.addValueChangeHandler((ValueChangeEvent<State> event) -> {
         State state = event.getValue();
         switch (state)
         {
         case INDETERMINATE:
            unset(option);
            break;
         case OFF:
            set(option, "FALSE");
            break;
         case ON:
            set(option, "TRUE");
            break;
         }
         synchronize();
      });
      return toggle;
   }
   
   protected boolean has(String key)
   {
      return chunkOptions_.containsKey(key);
   }
   
   protected String get(String key)
   {
      if (!has(key))
         return null;
      
      return chunkOptions_.get(key);
   }
   
   protected boolean getBoolean(String key)
   {
      if (!has(key))
         return false;
      
      return isTrue(chunkOptions_.get(key));
   }
   
   protected void set(String key, String value)
   {
      chunkOptions_.put(key,  value);
   }
   
   protected void unset(String key)
   {
      chunkOptions_.remove(key);
   }
   
   protected boolean select(String option)
   {
      for (int i = 0; i < outputComboBox_.getItemCount(); i++)
      {
         if (outputComboBox_.getItemText(i) == option)
         {
            outputComboBox_.setSelectedIndex(i);
            return true;
         }
      }
      
      return false;
   }
   
   private void updateOutputComboBox()
   {
      boolean hasEcho = has("echo");
      boolean hasEval = has("eval");
      boolean hasIncl = has("include");
      
      boolean isEcho = hasEcho && getBoolean("echo");
      boolean isEval = hasEval && getBoolean("eval");
      boolean isIncl = hasIncl && getBoolean("include");
      
      if (hasEcho && !hasEval && !hasIncl)
         select(isEcho ? OUTPUT_SHOW_CODE_AND_OUTPUT : OUTPUT_SHOW_OUTPUT_ONLY);
     
      if (!hasEcho && !hasEval && hasIncl && !isIncl)
         select(OUTPUT_SHOW_NOTHING);
      
      if (!hasEcho && hasEval && !isEval && hasIncl && !isIncl)
         select(OUTPUT_SKIP_THIS_CHUNK);
   }
   
   public void init(DocDisplay display, Position position)
   {
      display_ = display;
      position_ = position;
      chunkOptions_.clear();
      originalChunkOptions_.clear();
      
      useCustomFigureCheckbox_.setValue(false);
      figureDimensionsPanel_.setVisible(false);
            
      Command afterInit = new Command()
      {
         @Override
         public void execute()
         {
            updateOutputComboBox();
            boolean hasRelevantFigureSettings =
                  has("fig.width") ||
                  has("fig.height");

            useCustomFigureCheckbox_.setValue(hasRelevantFigureSettings);
            if (hasRelevantFigureSettings)
               useCustomFigureCheckbox_.setVisible(true);
            figureDimensionsPanel_.setVisible(hasRelevantFigureSettings);

            if (has("fig.width"))
               figWidthBox_.setText(get("fig.width"));
            else
               figWidthBox_.setText("");

            if (has("fig.height"))
               figHeightBox_.setText(get("fig.height"));
            else
               figHeightBox_.setText("");

            if (has("warning"))
               showWarningsInOutputCb_.setValue(getBoolean("warning"));

            if (has("message"))
               showMessagesInOutputCb_.setValue(getBoolean("message"));

            if (has("paged.print"))
               printTableAsTextCb_.setValue(getBoolean("paged.print"));

            if (has("cache"))
               cacheChunkCb_.setValue(getBoolean("cache"));
            
            if (has("engine.path"))
            {
               String enginePath = StringUtil.stringValue(get("engine.path"));
               enginePath = enginePath.replaceAll("\\\\\\\\", "\\\\");
               enginePathBox_.setValue(enginePath);
            }
            
            if (has("engine.opts"))
            {
               String engineOpts = StringUtil.stringValue(get("engine.opts"));
               engineOpts = engineOpts.replaceAll("\\\\\\\\", "\\\\");
               engineOptsBox_.setValue(engineOpts);
            }
            
            setVisible(true);
         }
      };
      
      initOptions(afterInit);
      
   }
   
   private boolean isTrue(String string)
   {
      return string == "TRUE" || string == "T";
   }
   
   @Override
   public void hide()
   {
      position_ = null;
      chunkOptions_.clear();
      super.hide();
   }
   
   private void hideAndFocusEditor()
   {
      hide();
      display_.focus();
   }
   
   private int getPriority(String key)
   {
      if (key == "eval")
         return 10;
      else if (key == "echo")
         return 9;
      else if (key == "warning" || key == "error" || key == "message")
         return 8;
      else if (key.startsWith("fig."))
         return 8;
      return 0;
   }
   
   protected Map<String, String> sortedOptions(Map<String, String> options)
   {
      List<Map.Entry<String, String>> entries = new ArrayList<>(options.entrySet());

      Collections.sort(entries, new Comparator<Map.Entry<String, String>>() {
         public int compare(Map.Entry<String, String> a, Map.Entry<String, String> b)
         {
            int lhsGroup = getPriority(a.getKey());
            int rhsGroup = getPriority(b.getKey());
            
            if (lhsGroup < rhsGroup)
               return 1;
            else if (lhsGroup > rhsGroup)
               return -1;
            
            return a.getKey().compareToIgnoreCase(b.getKey());
         }
      });

      LinkedHashMap<String, String> sortedMap = new LinkedHashMap<>();
      for (Map.Entry<String, String> entry : entries) {
         sortedMap.put(entry.getKey(), entry.getValue());
      }
      return sortedMap;
   }
   
   protected final VerticalPanel panel_;
   protected final Label header_;
   protected final FormLabel chunkLabel_;
   protected final TextBoxWithCue tbChunkLabel_;
   protected final ListBox outputComboBox_;
   protected final Grid figureDimensionsPanel_;
   protected final TextBox figWidthBox_;
   protected final TextBox figHeightBox_;
   protected final Grid enginePanel_;
   protected final TextBox enginePathBox_;
   protected final TextBox engineOptsBox_;
   protected final SmallButton revertButton_;
   protected final SmallButton applyButton_;
   protected final Toggle useCustomFigureCheckbox_;
   protected final Toggle showWarningsInOutputCb_;
   protected final Toggle showMessagesInOutputCb_;
   protected final Toggle printTableAsTextCb_;
   protected final Toggle cacheChunkCb_;
   
   protected String originalLine_;
   protected String chunkPreamble_;
   protected final boolean isVisualEditor_;
   
   protected HashMap<String, String> chunkOptions_;
   protected HashMap<String, String> originalChunkOptions_;
   
   protected DocDisplay display_;
   protected Position position_;
   
   private static final String OUTPUT_USE_DOCUMENT_DEFAULT =
         constants_.useDocumentDefaultParentheses();

   private static final String OUTPUT_SHOW_OUTPUT_ONLY =
         constants_.showOutputOnly();
   
   private static final String OUTPUT_SHOW_CODE_AND_OUTPUT =
         constants_.showCodeAndOutput();
   
   private static final String OUTPUT_SHOW_NOTHING =
         constants_.showNothingRunCode();
   
   private static final String OUTPUT_SKIP_THIS_CHUNK =
         constants_.showNothingDontRunCode();
   
   public interface Styles extends CssResource
   {
      String headerLabel();
      
      String textBox();
      
      String chunkLabel();
      String chunkName();
      String labelPanel();
      
      String buttonPanel();
      
      String checkBox();
   }
   
   public interface Resources extends ClientBundle
   {
      @Source("ChunkOptionsPopupPanel.css")
      Styles styles();
   }
   
   private static Resources RES = GWT.create(Resources.class);
   static {
      RES.styles().ensureInjected();
   }
   
   // Injected ----
   protected FileDialogs fileDialogs_;
   protected RemoteFileSystemContext rfsContext_;
}
