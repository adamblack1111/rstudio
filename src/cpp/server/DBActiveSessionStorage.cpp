#include "server/DBActiveSessionStorage.hpp"
#include "server_core/ServerDatabase.hpp"
#include "core/Database.hpp"
#include <numeric>



namespace rstudio
{
namespace server
{
namespace storage
{

using namespace server_core::database;
namespace {

   // This is the column name of the foreign key between the active_session_metadata
   // And the licensed user table. The only column that isn't a string
   std::string kUserId = "user_id";

   std::string getKeyString(const std::map<std::string, std::string>& sourceMap)
   {
      std::string keys = std::accumulate(++sourceMap.begin(), sourceMap.end(), 
                  sourceMap.begin()->first, [](std::string a, std::pair<std::string, std::string> b) {
                     return a + ", " + b.first;
                });
      return keys;
   }

   std::string getValueString(const std::map<std::string, std::string>& sourceMap)
   {
      std::string values = std::accumulate(++sourceMap.begin(), sourceMap.end(), 
                  "'" + sourceMap.begin()->second + "'", [](std::string a, std::pair<std::string, std::string> b) {
                     std::string str{a};
                     if(b.first == kUserId)
                     {
                        a += ", " + b.second;
                     }
                     else
                     {
                        a += ", '" + b.second + "'";
                     }
                     return a;
                });
      return values;

   }

   std::string getUpdateString(std::map<std::string, std::string> sourceMap)
   {
      std::string setValuesString = std::accumulate(++sourceMap.begin(), sourceMap.end(),
                  sourceMap.begin()->first + " = '" + sourceMap.begin()->second + "'", [](std::string a, std::pair<std::string, std::string> iter){
                     return a + ", " + iter.first + " = '" + iter.second + "'";
                  });
      return setValuesString;
   }

   std::string getColumnNameList(std::set<std::string> colNames)
   {
      std::string cols = std::accumulate(++colNames.begin(), colNames.end(), 
                  *(colNames.begin()), [](std::string a, std::string b) {
                  return a + ", " + b;
                });
      return cols;
   }

   void populateMapWithRow(database::RowsetIterator iter, std::map<std::string, std::string> *targetMap)
   {
      for(int i=0; i < iter->size(); i++)
      {
         std::string key = iter->get_properties(i).get_name();
         if(key != kUserId)
         {
            targetMap->insert(
               std::pair<std::string, std::string>{
                  key,
                  iter->get<std::string>(key, "")
               }
            );
         }
         else
         {
            targetMap->insert(
               std::pair<std::string, std::string>{
                  key,
                  std::to_string(iter->get<int>(key))
               }
            );
         }
      }
   }

} // anonymous namespace

   class ConnectionNotEstablished
   {
   };

   DBActiveSessionStorage::DBActiveSessionStorage()
   {
      bool success = getConnection(boost::posix_time::milliseconds(500), &connection);
      if(!success)
      {
         throw ConnectionNotEstablished();
      }
   }

   DBActiveSessionStorage::DBActiveSessionStorage(boost::shared_ptr<core::database::IConnection> conn) :
      connection{conn}
   {

   }

   Error DBActiveSessionStorage::readProperty(const std::string& id, const std::string& name, std::string* pValue)
   {
      database::Query query = connection->query("SELECT "+name+" FROM active_session_metadata WHERE session_id = :id")
         .withInput(id);

      database::Rowset results{};
      *pValue = "";
      Error error = connection->execute(query, results);

      if(!error)
      {
         database::RowsetIterator iter = results.begin();
         if(iter != results.end())
         {
            if(name != kUserId)
            {
               *pValue = iter->get<std::string>(name, "");
            }
            else
            {
               *pValue = std::to_string(iter->get<int>(name));
            }
            if(++iter != results.end()) 
            {
               return Error{"Too many sessions returned", errc::TooManySessionsReturned, ERROR_LOCATION};
            }
         }
         else
         {
            return Error{"Session does not exist", errc::SessionNotFound, ERROR_LOCATION};
         }
      }
      else
      {
         return Error{"DatabaseException", errc::DBError, "Database exception during property read [ session:"+id+" property:"+name+" ]", error, ERROR_LOCATION};
      }
      
      return error;
   }

   Error DBActiveSessionStorage::readProperties(const std::string& id, const std::set<std::string>& names, std::map<std::string, std::string>* pValues)
   {
      pValues->clear();
      std::string namesString = getColumnNameList(names);
      database::Query query = connection->query("SELECT "+namesString+" FROM active_session_metadata WHERE session_id=:id")
         .withInput(id);

      database::Rowset rowset{};
      Error error = connection->execute(query, rowset);

      if(!error)
      {
         database::RowsetIterator iter = rowset.begin();
         if(iter != rowset.end())
         {
            populateMapWithRow(iter, pValues);
            // Sanity check number of returned rows, by using the pk in the where clause we should only get 1 row
            if(++iter != rowset.end())
            {
               return Error{"Too many sessions returned", errc::TooManySessionsReturned, ERROR_LOCATION};
            }
         }
         else
         {
            return Error{"Session does not exist", errc::SessionNotFound, ERROR_LOCATION};
         }
      }
      else
      {
         return Error{"DatabaseException", errc::DBError, "Database exception during proprerties read [ session:"+id+" properties:"+namesString+" ]", error, ERROR_LOCATION};
      }

      return error;
   }

   Error DBActiveSessionStorage::readProperties(const std::string& id, std::map<std::string, std::string>* pValues)
   {
      std::set<std::string> all{"*"};
      return readProperties(id, all, pValues);
   }

   Error DBActiveSessionStorage::writeProperty(const std::string& id, const std::string& name, const std::string& value)
   {

      database::Query query = connection->query("UPDATE active_session_metadata SET "+name+" = :value WHERE session_id = :id")
         .withInput(value)
         .withInput(id);

      Error error = connection->execute(query);

      if(error){
         return Error{"DatabaseException", errc::DBError, "Database error while updating session metadata [ session: "+id+" property: " + name + " ]", error, ERROR_LOCATION};
      }

      return error;
   }

   Error DBActiveSessionStorage::writeProperties(const std::string& id, const std::map<std::string, std::string>& properties)
   {
      database::Query query = connection->query("SELECT * FROM active_session_metadata WHERE session_id = :id")
         .withInput(id);
      database::Rowset rowset{};

      

      Error error = connection->execute(query, rowset);
      if(!error)
      {
         database::RowsetIterator iter = rowset.begin();
         if(iter != rowset.end())
         {
            // Sanity check number of returned rows, by using the pk in the where clause we should only get 1 row
            if(++iter != rowset.end())
            {
               return Error{"Too many sessions returned", errc::TooManySessionsReturned, ERROR_LOCATION};
            }

            database::Query updateQuery = connection->query("UPDATE active_session_metadata SET "+getUpdateString(properties)+" WHERE session_id = :id")
               .withInput(id);
            
            error = connection->execute(updateQuery);
            if(error)
            {
               return Error{"DatabaseException", errc::DBError, "Error while updating properties [ session:"+id+" properties:"+getKeyString(properties)+" ]", error, ERROR_LOCATION};
            }
         }
         else
         {
            database::Query insertQuery = connection->query("INSERT INTO active_session_metadata (session_id, "+getKeyString(properties)+") VALUES (:id, "+getValueString(properties)+")")
               .withInput(id);

            error = connection->execute(insertQuery);
            if(error)
            {
               return Error{"DatabaseException", errc::DBError, "Error while updating properties [ session:"+id+" properties:"+getKeyString(properties)+" ]", error, ERROR_LOCATION};
            }
         }
      }
      return error;
   }
} // Namespace storage
} // Namespace server
} // Namespace rstudio