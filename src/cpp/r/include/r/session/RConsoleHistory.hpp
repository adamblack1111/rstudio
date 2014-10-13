/*
 * RConsoleHistory.hpp
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

#ifndef R_SESSION_CONSOLE_HISTORY_HPP
#define R_SESSION_CONSOLE_HISTORY_HPP

#include <string>

#include <boost/utility.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/signal.hpp>

#include <core/json/Json.hpp>

namespace rstudiocore {
   class Error ;
   class FilePath;
}

namespace r {
namespace session {

// singleton
class ConsoleHistory ;
ConsoleHistory& consoleHistory();
   
class ConsoleHistory : boost::noncopyable
{
public:
   typedef boost::circular_buffer<std::string>::value_type value_type;
   typedef boost::circular_buffer<std::string>::const_iterator const_iterator;
   typedef boost::signal<void (const std::string&)> AddSignal;

private:
   ConsoleHistory();
   friend ConsoleHistory& consoleHistory();
   // COPYING: boost::noncopyable
      
public:   
   void setCapacity(int capacity);

   void setCapacityFromRHistsize();

   int capacity() const
   {
      return historyBuffer_.capacity();
   }

   void setRemoveDuplicates(bool removeDuplicates);
   
   void add(const std::string& command);
   
   const_iterator begin() const { return historyBuffer_.begin(); }
   const_iterator end() const { return historyBuffer_.end(); }
   
   int size() const
   {
      return historyBuffer_.size();
   }

   void clear();

   void remove(const std::vector<int>& indexes);

   void subset(int beginIndex, // inclusive
               int endIndex,   // exclusive,
               std::vector<std::string>* pEntries) const;

   void asJson(rstudiocore::json::Array* pHistoryArray) const;
   
   rstudiocore::Error loadFromFile(const rstudiocore::FilePath& filePath, bool verifyFile);
   rstudiocore::Error saveToFile(const rstudiocore::FilePath& filePath) const;
   
   boost::signals::connection connectOnAdd(
                                 const AddSignal::slot_function_type& slot)
   {
      return onAdd_.connect(slot);
   }

private:
   void safeRemove(int index);
   
private:   
   bool removeDuplicates_;
   boost::circular_buffer<std::string> historyBuffer_ ;
   AddSignal onAdd_; 
};
   
} // namespace session
} // namespace r

#endif // R_SESSION_CONSOLE_HISTORY_HPP 

