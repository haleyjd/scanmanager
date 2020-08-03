/*

   HL7Daemon - Prometheus database connection manager.
   
   I'm hiding as much of the IBDatabase/VCL nastiness as possible in here by
   wrapping sqlLib with some simple-to-use objects that employ the common
   pImpl idiom for concealing implementation details.
   
   Should make the rest of the code a lot cleaner and more stable:
   - All methods are exception safe (no need for logic-clouding try/catch).
     Success or failure is indicated with boolean return values.
   - Memory management is automated with destructors for stack-allocated
     objects, whereas the VCL components hidden here can only be heap allocated
     and are easily leaked, especially when they throw exceptions from every
     possible method, including constructors and destructors.
   - Common errors are checked for and worked around.
   - Methods take reference params and return object results into references
     as much as is practical, to improve efficiency of user code (sqlLib is
     still subject to the inefficient passing of arguments and results by
     value, however).

   If I had the time, importing this into Prometheus itself would probably be
   worth it for these benefits.

*/

#ifndef VIBC_NO_VISUALIB

#include <iostream>

#include "prometheusdb.h"

// ideally, this is the only place this will be included.
#include "sqlLib.h"
#include "util.h"
#include "inifile.h"

//=============================================================================
//
// PrometheusDB
//

static void VerboseSQLError(const char *query)
{
   std::cout << "* ibx_error = " << IntToString(last_sql_lib_error.ibx_error)     << std::endl;
   std::cout << "* sql_error = " << IntToString(last_sql_lib_error.sql_error_num) << std::endl;
   std::cout << "* message   = " << last_sql_lib_error.message.c_str()            << std::endl;
}

//
// Private implementation details for PrometheusDB
//
class PrometheusDBPimpl
{
public:
   VIB::Database db;

   PrometheusDBPimpl() : db()
   {
   }
};

//
// PrometheusDB Constructor
//
PrometheusDB::PrometheusDB()
{
   pImpl = new PrometheusDBPimpl;
}

//
// PrometheusDB Destructor
//
PrometheusDB::~PrometheusDB()
{
   if(pImpl)
   {
      delete pImpl;
      pImpl = NULL;
   }
}

//
// PrometheusDB::connect
//
// Connect to a specific database, if not already connected
//
bool PrometheusDB::connect(const pdb::string &addr, const pdb::string &user, const pdb::string &pw)
{
   bool result = false;
   VIB::Database *db = &pImpl->db;

   try
   {
      if(db->TestConnected())
         result = true;       // Already connected.
      else
      {
         // Establish a new connection
         result = ConnectToDatabase(db, addr, user, pw);
      }
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Database connection failed due to an unknown exception");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusDB::connect
//
// Connect to the globally specified database, if not already connected.
//
bool PrometheusDB::connect(const char *dbname)
{
   // TODO/FIXME
   IniFile::IniMap &ini = IniFile::GetIniOptions();

   return connect(ini[dbname]["db"],
                  ini[dbname]["user"],
                  ini[dbname]["password"]);

   return false;
}

//
// PrometheusDB::disconnect
//
// Disconnect from the database, if it's connected. It can be reconnected if
// necessary by calling connect() again. This invalidates any open datasets or
// transactions.
//
void PrometheusDB::disconnect()
{
   VIB::Database *db = &pImpl->db;

   try
   {
      if(db->TestConnected())
         db->Close();
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Failed trying to close database connection");
      VerboseSQLError(NULL);
   }
}

//
// PrometheusDB::isConnected
//
// Returns true or false regarding the database's connection state.
//
bool PrometheusDB::isConnected()
{
   bool result = false;

   try
   {
      result = pImpl->db.TestConnected();
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Exception while testing database connection state");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusDB::executeUpdateStatement
//
// Send in a table name, map of fields, and optionally, a map of options for
// those fields, to update records in the database.
//
bool PrometheusDB::executeUpdateStatement
(
   const pdb::string      &tableName,
   const pdb::stringmap   &fieldMap,
   const pdb::strtointmap *fieldOptions
)
{
   bool result = false;

   try
   {
      result = ExecuteUpdateStatement(&pImpl->db, tableName, fieldMap, fieldOptions);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during ExecuteUpdateStatement");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusDB::executeInsertStatement
//
// Send in a table name, map of fields, and optionally, a map of options for
// those fields, to insert new records into the database.
//
bool PrometheusDB::executeInsertStatement
(
   const pdb::string      &tableName,
   const pdb::stringmap   &fieldMap,
   const pdb::strtointmap *fieldOptions
)
{
   bool result = false;

   try
   {
      result = ExecuteInsertStatement(&pImpl->db, tableName, fieldMap, fieldOptions);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during ExecuteInsertStatement");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusDB::executeStatement
//
// Execute a generic SQL statement against the database.
//
bool PrometheusDB::executeStatement(const pdb::string &sql)
{
   bool result = false;

   try
   {
      result = ExecuteStatement(&pImpl->db, sql);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during ExecuteStatement");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusDB::getOneField
//
// Returns the value of a single field in "result" given a SQL statement that
// generates a single column and row.
//
void PrometheusDB::getOneField(const pdb::string &sql, pdb::string &result)
{
   try
   {
      result = GetOneField(&pImpl->db, sql);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during GetOneField");
      VerboseSQLError(sql.c_str());
   }
}

//
// PrometheusDB::sqlToMap
//
// Return the single-row results of a SQL query in a map<string, string>
//
bool PrometheusDB::sqlToMap(const pdb::string &sql, pdb::stringmap &fieldMap)
{
   bool result = false;

   try
   {
      result = SqlToMap(&pImpl->db, sql, fieldMap);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToMap");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusDB::sqlToVecMap
//
// Returns the multiple-row result of a SQL query as a vector<map<string, string> >,
// which while not of maximum efficiency due to the duplication of field names
// in every map row of the vector, is MUCH easier to use than a more efficient
// data structure would be.
//
bool PrometheusDB::sqlToVecMap(const pdb::string &sql, pdb::vecmap &fieldVecMap)
{
   bool result = false;

   try
   {
      result = SqlToVecMap(&pImpl->db, sql, fieldVecMap);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToVecMap");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusDB::sqlToSet
//
// Returns the single-row results of a SQL query as a set of strings.
//
bool PrometheusDB::sqlToSet(const pdb::string &sql, pdb::stringset &fieldSet)
{
   bool result = false;

   try
   {
      result = SqlToSet(&pImpl->db, sql, fieldSet);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToSet");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusDB::sqlToVec
//
// Returns the single-row results of a SQL query as a vector of strings.
//
bool PrometheusDB::sqlToVec(const pdb::string &sql, pdb::stringvec &fieldVec)
{
   bool result = false;

   try
   {
      result = SqlToVec(&pImpl->db, sql, fieldVec);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToVec");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//=============================================================================
//
// PrometheusTransaction
//

//
// Private implementation details for PrometheusTransaction
//
class PrometheusTransactionPimpl
{
public:
   VIB::Transaction transaction;

   PrometheusTransactionPimpl() : transaction()
   {
   }
};

//
// PrometheusTransaction Constructor
//
PrometheusTransaction::PrometheusTransaction()
{
   pImpl = new PrometheusTransactionPimpl;
}

//
// PrometheusTransaction Destructor
//
// This will help with memory management enormously. By declaring this object
// on the stack, the enclosed TIBTransaction will be closed and freed
// automatically when the calling function returns. You must still commit
// any written changes first though, or the transaction will rollback by
// default for safety.
//
PrometheusTransaction::~PrometheusTransaction()
{
   if(pImpl)
   {
      delete pImpl;
      pImpl = NULL;
   }
}

//
// PrometheusTransaction::isActive
//
// Returns true or false regarding the transaction's active state.
//
bool PrometheusTransaction::isActive()
{
   bool result = false;

   try
   {
      result = pImpl->transaction.Active;
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Exception while testing transaction active state");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusTransaction::commit
//
// Commit a transaction. It should be in the active state or the resulting VCL
// exception will cause a false return value. The transaction is closed after
// this call.
//
bool PrometheusTransaction::commit()
{
   bool result = false;

   try
   {
      pImpl->transaction.Commit();
      result = true;
   }
   catch(...)
   {
      //DEBUGOUT(dbg_error, "DB: WARNING: transaction commit failure! Data may be lost!");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusTransaction::rollback
//
// Rollback a transaction. It should be in the active state or the resulting VCL
// exception will cause a false return value. The transaction is closed after
// this call.
//
bool PrometheusTransaction::rollback()
{
   bool result = false;

   try
   {
      pImpl->transaction.Rollback();
      result = true;
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Transaction rollback failure");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusTransaction::stdTransaction
//
// Attach a transaction to the database with the standard parameters.
//
bool PrometheusTransaction::stdTransaction(PrometheusDB &db)
{
   bool result = false;
   VIB::Database    *ibdb = &(db.pImpl->db);
   VIB::Transaction *ibta = &pImpl->transaction;

   result = StdTransaction(ibta, ibdb);

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusTransaction::executeUpdateStatement
//
// Send in a table name, map of fields, and optionally, a map of options for
// those fields, to update records in the database.
//
bool PrometheusTransaction::executeUpdateStatement
(
   const pdb::string      &tableName,
   const pdb::stringmap   &fieldMap,
   const pdb::strtointmap *fieldOptions
)
{
   bool result = false;

   try
   {
      result = ExecuteUpdateStatement(&pImpl->transaction, tableName, fieldMap, fieldOptions);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during ExecuteUpdateStatement");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusTransaction::executeInsertStatement
//
// Send in a table name, map of fields, and optionally, a map of options for
// those fields, to insert new records into the database.
//
bool PrometheusTransaction::executeInsertStatement
(
   const pdb::string      &tableName,
   const pdb::stringmap   &fieldMap,
   const pdb::strtointmap *fieldOptions
)
{
   bool result = false;

   try
   {
      result = ExecuteInsertStatement(&pImpl->transaction, tableName, fieldMap, fieldOptions);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during ExecuteInsertStatement");
      result = false;
   }

   if(!result)
      VerboseSQLError(NULL);

   return result;
}

//
// PrometheusTransaction::executeStatement
//
// Execute a generic SQL statement against the database.
//
bool PrometheusTransaction::executeStatement(const pdb::string &sql)
{
   bool result = false;

   try
   {
      result = ExecuteStatement(&pImpl->transaction, sql);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during ExecuteStatement");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusTransaction::lockForUpdate
//
// Protects prospective records from concurrent modification by other database
// users during the period of time between a query of all the record's existing
// field values and the actual update statement.
//
bool PrometheusTransaction::lockForUpdate(const pdb::string &tableName, const pdb::string &id)
{
   bool result = false;

   try
   {
      result = LockRecordForUpdate(&pImpl->transaction, tableName, id);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during LockRecordForUpdate");
      VerboseSQLError(NULL);
      result = false;
   }

   return result;
}

//
// PrometheusTransaction::getNextId
//
// For properly setup tables (those with a uniformly-named id generator), this
// will retrieve a suitable id for the next record, in a manner that can be
// rolled back if a subsequent insert op should fail.
//
void PrometheusTransaction::getNextId(const pdb::string &tableName, pdb::string &result)
{
   try
   {
      result = GetNextId(&pImpl->transaction, tableName);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during GetNextId");
      VerboseSQLError(NULL);
   }
}

//
// PrometheusTransaction::getOneField
//
// Returns the value of a single field in "result" given a SQL statement that
// generates a single column and row.
//
void PrometheusTransaction::getOneField(const pdb::string &sql, pdb::string &result)
{
   try
   {
      result = GetOneField(&pImpl->transaction, sql);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during GetOneField");
      VerboseSQLError(sql.c_str());
   }
}

//
// PrometheusTransaction::sqlToMap
//
// Return the single-row results of a SQL query in a map<string, string>
//
bool PrometheusTransaction::sqlToMap(const pdb::string &sql, pdb::stringmap &fieldMap)
{
   bool result = false;

   try
   {
      result = SqlToMap(&pImpl->transaction, sql, fieldMap);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToMap");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusTransaction:;getFullRecord
//
// This is largely a convenience wrapper around sqlToMap.
//
bool PrometheusTransaction::getFullRecord
(
   const pdb::string &tableName, 
   const pdb::string &id, 
   pdb::stringmap    &outmap
)
{
   std::string query = "select * from " + tableName + " where id = " + id;
   return sqlToMap(query, outmap);
}


//
// PrometheusTransaction::sqlToVecMap
//
// Returns the multiple-row result of a SQL query as a vector<map<string, string> >,
// which while not of maximum efficiency due to the duplication of field names
// in every map row of the vector, is MUCH easier to use than a more efficient
// data structure would be.
//
bool PrometheusTransaction::sqlToVecMap(const pdb::string &sql, pdb::vecmap &fieldVecMap)
{
   bool result = false;

   try
   {
      result = SqlToVecMap(&pImpl->transaction, sql, fieldVecMap);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToVecMap");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusTransaction::sqlToSet
//
// Returns the single-row results of a SQL query as a set of strings.
//
bool PrometheusTransaction::sqlToSet(const pdb::string &sql, pdb::stringset &fieldSet)
{
   bool result = false;

   try
   {
      result = SqlToSet(&pImpl->transaction, sql, fieldSet);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToSet");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//
// PrometheusTransaction::sqlToVec
//
// Returns the single-row results of a SQL query as a vector of strings.
//
bool PrometheusTransaction::sqlToVec(const pdb::string &sql, pdb::stringvec &fieldVec)
{
   bool result = false;

   try
   {
      result = SqlToVec(&pImpl->transaction, sql, fieldVec);
   }
   catch(...)
   {
      //DEBUGOUT(dbg_database, "DB: Unknown exception during SqlToVec");
      result = false;
   }

   if(!result)
      VerboseSQLError(sql.c_str());

   return result;
}

//=============================================================================
//
// PrometheusLookup
//

//
// PromtheusLookup - Parameterized Constructor
//
PrometheusLookup::PrometheusLookup(PrometheusDB &db, const std::string &pTableName)
   : tableName(), valueToID(), idToValue()
{
   load(db, pTableName);
}

//
// PrometheusLookup - Copy Constructor
//
PrometheusLookup::PrometheusLookup(const PrometheusLookup &other)
   : tableName(other.tableName), valueToID(other.valueToID), idToValue(other.idToValue)
{
}

//
// PrometheusLookup::load
//
// Load the contents of a lookup table from the passed-in database.
// Lookup tables need to have an (id, item, valid) column layout.
//
void PrometheusLookup::load(PrometheusDB &db, const pdb::string &pTableName)
{
   pdb::vecmap lookup;
   pdb::string sql;

   tableName = pTableName;
   sql = "select id, item from " + tableName + " where valid = 1";

   if(!db.sqlToVecMap(sql, lookup))
   {
      //DEBUGOUT(dbg_database, "DB: Failed to load lookup " << tableName.c_str());
      return;
   }

   valueToID.clear();
   idToValue.clear();

   // build fast bi-directional lookup maps
   for(pdb::vecmap::iterator i = lookup.begin(); i != lookup.end(); ++i)
   {
      std::string id   = (*i)["id"];
      std::string item = LowercaseString((*i)["item"]);

      valueToID[item] = id;
      idToValue[ id ] = item;
   }
}

//
// PrometheusLookup::loadCustom
//
// Load the contents of a lookup table from the passed-in database.
// Lookup tables need to have an (id, item, valid) column layout.
// This overload accepts a custom SQL query. It must return columns (id, item).
//
void PrometheusLookup::loadCustom(PrometheusDB &db, const pdb::string &pTableName,
                                  const pdb::string &sql)
{
   pdb::vecmap lookup;

   tableName = pTableName;

   if(!db.sqlToVecMap(sql, lookup))
   {
      //DEBUGOUT(dbg_database, "DB: Failed to load lookup " << tableName.c_str());
      return;
   }

   valueToID.clear();
   idToValue.clear();

   // build fast bi-directional lookup maps
   for(pdb::vecmap::iterator i = lookup.begin(); i != lookup.end(); ++i)
   {
      std::string id   = (*i)["id"];
      std::string item = LowercaseString((*i)["item"]);

      valueToID[item] = id;
      idToValue[ id ] = item;
   }
}

//
// PrometheusLookup::idOf
//
// Find the value in this lookup table; if it exists, the corresponding id
// is returned as a string. Otherwise, an empty string is returned.
//
const pdb::string &PrometheusLookup::idOf(const pdb::string &value) const
{
   static const std::string noresult = "";

   pdb::stringmap::const_iterator i = valueToID.find(LowercaseString(value));

   if(i != valueToID.end())
      return i->second;
   else
      return noresult;
}

//
// PrometheusLookup::valueOf
//
// Find the id in this lookup table; if it exists, the corresponding value is
// returned as a string. Otherwise, an empty string is returned.
//
const pdb::string &PrometheusLookup::valueOf(const pdb::string &id) const
{
   static const std::string noresult = "";

   pdb::stringmap::const_iterator i = idToValue.find(id);

   if(i != idToValue.end())
      return i->second;
   else
      return noresult;
}

//
// PrometheusLookup::isValidID
//
// Returns true if the id actually exists in the lookup, and false otherwise.
//
bool PrometheusLookup::isValidID(const pdb::string &id) const
{
   return (idToValue.find(id) != idToValue.end());
}

//
// PrometheusLookup::isValidValue
//
// Returns true if the value actually exists in the lookup, and false otherwise.
//
bool PrometheusLookup::isValidValue(const pdb::string &value) const
{
   return (valueToID.find(LowercaseString(value)) != valueToID.end());
}

//=============================================================================
//
// PrometheusLookups
//

//
// PrometheusLookups::haveLookup
//
// Test to see if the named table has been loaded as a lookup already.
//
bool PrometheusLookups::haveLookup(const pdb::string &tableName) const
{
   return (lookupTables.find(tableName) != lookupTables.end());
}

//
// PrometheusLookups::loadLookup
//
// Load the indicated lookup table into a PrometheusLookup structure, provided
// we have not already loaded that table.
//
void PrometheusLookups::loadLookup(PrometheusDB &db, const pdb::string &tableName)
{
   // one time only.
   if(!haveLookup(tableName))
   {
      PrometheusLookup &newLookup = lookupTables[tableName];
      newLookup.load(db, tableName);
   }
}

//
// PrometheusLookups::loadCustom
//
// Load the indicated lookup table into a PrometheusLookup structure, provided
// we have not already loaded that table. Use a custom SQL query to do it.
//
void PrometheusLookups::loadCustom(PrometheusDB &db, const pdb::string &tableName,
                                   const pdb::string &sql)
{
   // one time only.
   if(!haveLookup(tableName))
   {
      PrometheusLookup &newLookup = lookupTables[tableName];
      newLookup.loadCustom(db, tableName, sql);
   }
}

//
// PrometheusLookups::purgeLookups
//
// Drops all cached lookup tables.
//
void PrometheusLookups::purgeLookups()
{
   lookupTables.clear();
}

//
// PrometheusLookups::idOf
//
// Find the value for the given id in the indicated lookup table.
//
const pdb::string &PrometheusLookups::idOf(const pdb::string &tableName,
                                           const pdb::string &value) const
{
   static const std::string noresult = "";

   LookupMap::const_iterator lookup = lookupTables.find(tableName);

   if(lookup != lookupTables.end())
      return lookup->second.idOf(value);
   else
   {
      //DEBUGOUT(dbg_database, "DB: No such lookup " << tableName.c_str());
      return noresult;
   }
}

//
// PrometheusLookups::valueOf
//
// Find the id for the given value in the indicated lookup table.
//
const pdb::string &PrometheusLookups::valueOf(const pdb::string &tableName,
                                              const pdb::string &id) const
{
   static const std::string noresult = "";

   LookupMap::const_iterator lookup = lookupTables.find(tableName);

   if(lookup != lookupTables.end())
      return lookup->second.valueOf(id);
   else
   {
      //DEBUGOUT(dbg_database, "DB: No such lookup " << tableName.c_str());
      return noresult;
   }
}

//
// PrometheusLookups::isValidID
//
// Returns true if the lookup is valid and the id actually exists in the lookup,
// and false otherwise.
//
bool PrometheusLookups::isValidID(const pdb::string &tableName,
                                  const pdb::string &id) const
{
   bool result = false;

   LookupMap::const_iterator lookup = lookupTables.find(tableName);

   if(lookup != lookupTables.end())
      result = lookup->second.isValidID(id);
   else
      /*DEBUGOUT(dbg_database, "DB: No such lookup " << tableName.c_str())*/;

   return result;
}

//
// PrometheusLookups::isValidValue
//
// Returns true if the lookup is valid and the value actually exists in the
// lookup, and false otherwise.
//
bool PrometheusLookups::isValidValue(const pdb::string &tableName,
                                     const pdb::string &value) const
{
   bool result = false;

   LookupMap::const_iterator lookup = lookupTables.find(tableName);

   if(lookup != lookupTables.end())
      result = lookup->second.isValidValue(value);
   else
      /*DEBUGOUT(dbg_database, "DB: No such lookup " << tableName.c_str())*/;

   return result;
}

//
// PrometheusLookups::GetLookups
//
// Static method. The singleton lookup object will be created if it does not
// already exist, and then a reference to it will be returned.
//
PrometheusLookups &PrometheusLookups::GetLookups()
{
   static PrometheusLookups *theLookups = NULL;

   if(!theLookups)
      theLookups = new PrometheusLookups();

   return *theLookups;
}

const PrometheusLookup *PrometheusLookups::GetLookup(const pdb::string &tableName)
{
   PrometheusLookups &lookups = GetLookups();
   LookupMap::const_iterator lookup = lookups.lookupTables.find(tableName);

   if(lookup != lookups.lookupTables.end())
      return &lookup->second;

   return NULL;
}

//
// PrometheusLookups::LoadLookup
//
// Convenience static utility method to load a lookup table into the global
// singleton object.
//
void PrometheusLookups::LoadLookup(PrometheusDB &db, const pdb::string &tableName)
{
   GetLookups().loadLookup(db, tableName);
}

//
// PrometheusLookups::LoadCustom
//
// Convenience static utility method to load a lookup table into the global
// singleton object using a custom SQL query.
//
void PrometheusLookups::LoadCustom(PrometheusDB &db, const pdb::string &tableName,
                                   const pdb::string &sql)
{
   GetLookups().loadCustom(db, tableName, sql);
}

//
// PrometheusLookups::LoadLookups
//
// Load multiple lookups as named in an array of string constants.
//
void PrometheusLookups::LoadLookups(PrometheusDB &db, const char **tableNames,
                                    size_t numTables)
{
   for(size_t i = 0; i < numTables; i++)
      LoadLookup(db, tableNames[i]);
}

//
// PrometheusLookups::PurgeLookups
//
// Convenience static utility method to purge all loaded lookup tables from
// the global singleton object.
//
void PrometheusLookups::PurgeLookups()
{
   GetLookups().purgeLookups();
}

//
// PrometheusLookups::IdOf
//
// Convenience static utility method forwarding to PrometheusLookups::idOf
// for the global singleton object.
//
const pdb::string &PrometheusLookups::IdOf(const pdb::string &tableName,
                                           const pdb::string &value)
{
   return GetLookups().idOf(tableName, value);
}

//
// PrometheusLookups::ValueOf
//
// Convenience static utility method forwarding to PrometheusLookups::valueOf
// for the global singleton object.
//
const pdb::string &PrometheusLookups::ValueOf(const pdb::string &tableName,
                                              const pdb::string &id)
{
   return GetLookups().valueOf(tableName, id);
}

//
// PrometheusLookups::IsValidID
//
// Convenience static utility method forwarding to PrometheusLookups::isValidID
// for the global singleton object.
//
bool PrometheusLookups::IsValidID(const pdb::string &tableName,
                                  const pdb::string &id)
{
   return GetLookups().isValidID(tableName, id);
}

//
// PrometheusLookups::IsValidValue
//
// Convenience static utility method forwarding to
// PrometheusLookups::isValidValue for the global singleton object.
//
bool PrometheusLookups::IsValidValue(const pdb::string &tableName,
                                     const pdb::string &value)
{
   return GetLookups().isValidValue(tableName, value);
}

#endif // VIBC_NO_VISUALIB

// EOF

