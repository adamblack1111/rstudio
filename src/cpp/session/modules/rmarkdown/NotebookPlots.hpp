/*
 * NotebookPlots.hpp
 *
 * Copyright (C) 2009-16 by RStudio, Inc.
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


#ifndef SESSION_NOTEBOOK_PLOTS_HPP
#define SESSION_NOTEBOOK_PLOTS_HPP

#include <boost/function.hpp>
#include <boost/signals/connection.hpp>
#include <core/FilePath.hpp>
#include <r/RSexp.hpp>

#define kDisplayListExt ".snapshot"

namespace rstudio {
namespace core {
   class Error;
   class FilePath;
}
}

namespace rstudio {
namespace session {
namespace modules {
namespace rmarkdown {
namespace notebook {

enum PlotSizeBehavior
{
   PlotSizeAutomatic,
   PlotSizeManual
};

class PlotCapture
{
public:
   PlotCapture();
   ~PlotCapture();
   core::Error connect(double height, double width, 
           PlotSizeBehavior sizeBehavior,
           const core::FilePath& plotFolder);
   void disconnect();
private:
   void processPlots(bool ignoreEmpty);
   void removeGraphicsDevice();
   void onNewPlot();
   void saveSnapshot();
   void onBeforeNewPlot();

   core::FilePath plotFolder_;
   bool connected_;
   bool hasPlots_;
   PlotSizeBehavior sizeBehavior_;
   core::FilePath snapshotFile_;
   r::sexp::PreservedSEXP sexpMargins_;
   boost::signals::connection onBeforeNewPlot_;
   boost::signals::connection onBeforeNewGridPage_;
   boost::signals::connection onNewPlot_;
};


core::Error initPlots();

} // namespace notebook
} // namespace rmarkdown
} // namespace modules
} // namespace session
} // namespace rstudio

#endif
