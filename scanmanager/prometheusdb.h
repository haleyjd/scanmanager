/** @file prometheusdb.h

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
     still subject to the inefficient passing of arguments and results,
     however).

   If I had the time, importing this into Prometheus itself would probably be
   worth it for these benefits.

   @author James Haley
*/

#ifndef PROMETHEUSDB_H__
#define PROMETHEUSDB_H__

#ifndef VIBC_NO_VISUALIB

#include <map>
#include <set>
#include <string>
#include <vector>

class PrometheusTransactionPimpl;
class PrometheusDB;

/** Convenience typedefs for annoying verbose STL composite types */
namespace pdb
{
   /** Simply an alias for std::string, in case std:: is not open at the point of use. */
   typedef std::string string;
   /** Map from string to string */
   typedef std::map<std::string, std::string> stringmap;
   /** Set of strings */
   typedef std::set<std::string> stringset;
   /** Vector of strings */
   typedef std::vector<std::string> stringvec;
   /** Map from string to int */
   typedef std::map<std::string, int> strtointmap;
   /** Vector of maps from string to string; frequent sqlLib return value. */
   typedef std::vector<std::map<std::string, std::string> > vecmap;

   /** Iterator on a stringmap, for shorthand */
   typedef stringmap::iterator stringmap_iterator;
   /** Iterator on a vecmap, for shorthand */
   typedef vecmap::iterator    vecmap_iterator;
}

/**
 * Represents a transaction against the Prometheus database.
 * All methods are guaranteed to be exception-safe and never throw.
 */
class PrometheusTransaction
{
protected:
   PrometheusTransactionPimpl *pImpl; //!< Private implementation object.
   friend class PrometheusDB;

public:
   /**
    * Instantiating an instance of PrometheusTransaction will create a 
    * Borland InterBase DB TIBTransaction instance that is managed privately.
    * This class is designed so that it can be used as a stack local for
    * automatic resource allocation and release.
    */
   PrometheusTransaction();

   /**
    * Allowing a stack-allocated instance of this class to fall out of scope
    * will automatically close and free the TIBTransaction. If the transaction
    * is still active when this happens, it will be rolled back. If the rollback
    * fails, the destructor will be invoked one time anyway. If that also
    * fails, the TIBTransaction object will be deliberately leaked (destructors
    * should not throw exceptions, Borland!).
    */
   ~PrometheusTransaction();

   /**
    * Test if the transaction is active.
    * @return True if active, false otherwise.
    */
   bool isActive();

   /**
    * Commit the transaction.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool commit();

   /**
    * Rollback the transaction, undoing any changes made.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    * @note Not strictly necessary to call unless an error has occured.
    *       The default action on any PrometheusTransaction that falls
    *       out scope uncommitted is to rollback.
    */
   bool rollback();

   /**
    * Open a transaction against the indicated database in the manner
    * usually used for transactions in Prometheus.
    * @param db A valid connected instance of PrometheusDB.
    * @return True if the transaction is now active, false otherwise.
    * @pre Call db.connect() first.
    */
   bool stdTransaction(PrometheusDB &db);

   /**
    * Execute an update statement against this transaction, targeting the
    * indicated table and using the fields and options as passed.
    * @param tableName Table to update a record inside.
    * @param fieldMap Map of table field names to values.
    * @param fieldOptions Optional map of field names to options. See
    *        sqlLib.h for the definition of sql_ flags applicable to this
    *        parameter.
    * @return True if the update statement executed, false otherwise.
    * @pre The transaction must be active.
    */
   bool executeUpdateStatement(const pdb::string      &tableName,
                               const pdb::stringmap   &fieldMap,
                               const pdb::strtointmap *fieldOptions = NULL);
   /**
    * Execute an insert statement against this transaction, targeting the
    * indicated table and using the fields and options as passed.
    * @param tableName Table into which to insert the new record.
    * @param fieldMap Map of table field names to values.
    * @param fieldOptions Optional map of field names to options. See
    *        sqlLib.h for the definition of sql_ flags applicable to this
    *        parameter.
    * @return True if the insert statement executed, false otherwise.
    * @pre The transaction must be active.
    */
   bool executeInsertStatement(const pdb::string      &tableName,
                               const pdb::stringmap   &fieldMap,
                               const pdb::strtointmap *fieldOptions = NULL);

   /**
    * Execute a generic SQL statement against this transaction.
    * @param sql A fully formed SQL statement.
    * @return True if the statement executed, false otherwise.
    * @pre The transaction must be active.
    */
   bool executeStatement(const pdb::string &sql);

   /**
    * Lock a record for update.
    * @param tableName Database table the record resides within.
    * @param id        ID of the record to lock for update. Concurrent transactions will
    *                  fail if they try to write to the record while the transaction is
    *                  active, up until the time it is committed or rolled back.
    * @return True if the lock was successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool lockForUpdate(const pdb::string &tableName, const pdb::string &id);

   /**
    * Retrieve the next ID value for a table which has a conformant ID
    * generator on this transaction.
    * (ie., for table people, the generator must be named people_gen).
    * Rolling back the transaction should, in theory at least, return the
    * generator to its initial state.
    * @param[in] tableName Name of table. "_gen" will be appended to find the generator.
    * @param[out] result Value of the next ID to use. Unmodified if this operation fails.
    * @note Test the result using IsBlankOrZero to see if a valid result was returned.
    * @pre The transaction must be active.
    */
   void getNextId(const pdb::string &tableName, pdb::string &result);
   
   /**
    * Execute a SQL statement that is expected to return a single field in a single
    * row, and return that result in the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] result String to receive the result, if the query is successful.
    * @pre The transaction must be active.
    */
   void getOneField(const pdb::string &sql, pdb::string &result);
   
   /** 
    * Execute a SQL statement that is expected to return a single row of results,
    * and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldMap A map of string to string that will receive the results if the
    *    query is successful. Values will be keyed by field name.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool sqlToMap(const pdb::string &sql, pdb::stringmap &fieldMap);

   /**
    * Get all fields from a single record in the form of a stringmap.
    * @param[in]  tableName Name of the database table in which the target record is found.
    * @param[in]  id        ID of the target record.
    * @param[out] outmap    Map that will, on successful execution, contain all selectable 
    *                       fields from the target record.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool getFullRecord(const pdb::string &tableName, const pdb::string &id, 
                      pdb::stringmap &outmap);

   /** 
    * Execute a SQL statement that is expected to multiple rows of results,
    * and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldVecMap A vector of maps on string to string that will receive the results 
    *    if the query is successful. Values will be keyed by field name, and vector
    *    rows are in the order they were returned by the database.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool sqlToVecMap(const pdb::string &sql, pdb::vecmap &fieldVecMap);

   /** 
    * Execute a SQL statement that is expected to return multiple rows of a single
    * field, and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldSet A set of strings containing each single-field row.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool sqlToSet(const pdb::string &sql, pdb::stringset &fieldSet);

   /** 
    * Execute a SQL statement that is expected to return multiple rows of a single
    * field, and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldVec A vector of strings containing each single-field row.
    * @return True if successful, false otherwise.
    * @pre The transaction must be active.
    */
   bool sqlToVec(const pdb::string &sql, pdb::stringvec &fieldVec);
};

class PrometheusDBPimpl;

/**
 * Represents a connection to the Prometheus Firebird database.
 * All methods are guaranteed to be exception-safe and never throw.
 */
class PrometheusDB
{
protected:
   PrometheusDBPimpl *pImpl;            //!< Private implementation object.
   friend class PrometheusTransaction;

public:
   /**
    * Instantiating an instance of PrometheusDB will create a 
    * Borland InterBase DB TIBDatabase instance that is managed privately.
    * This class is designed so that it can be used as a stack local for
    * automatic resource allocation and release.
    */
   PrometheusDB();

   /**
    * Allowing a stack-allocated instance of this class to fall out of scope
    * will automatically close and free the TIBDatabase. If the database
    * is still connected when this happens, an attempt to disconnect will be
    * made before freeing the database object. If the connection close attempt
    * fails, the destructor will be invoked one time anyway. If that also
    * fails, the TIBDatabase object will be deliberately leaked (destructors
    * should not throw exceptions, Borland!).
    */
   ~PrometheusDB();

   /**
    * Connect to a Firebird database using the provided connection parameters.
    * @param addr Host address and database filepath, separated by a colon.
    * @param user Database username.
    * @param pw   Database username's password.
    * @return True if the connection has been established, false otherwise.
    */
   bool connect(const pdb::string &addr, const pdb::string &user,
                const pdb::string &pw);
   /**
    * Connect to a Firebird database using connection parameters that are
    * stored in the global singleton IniFile instance under the named section's
    * section's db, user, and password values.
    * @param dbname Name of an ini file section containing db, user, and password keys.
    * @return True if the connection has been established, false otherwise.
    */
   bool connect(const char *dbname);

   /**
    * Disconnect from the Firebird database, if connected.
    */
   void disconnect();

   /**
    * Test the state of the connection to the database.
    * @return True if connected, false if not.
    */
   bool isConnected();

   /**
    * Execute an update statement against a fresh transaction, targeting 
    * the indicated table and using the fields and options as passed.
    * @param tableName Table to update a record inside.
    * @param fieldMap Map of table field names to values.
    * @param fieldOptions Optional map of field names to options. See
    *        sqlLib.h for the definition of sql_ flags applicable to this
    *        parameter.
    * @return True if the update statement executed, false otherwise.
    * @pre The database must be connected.
    * @note The transaction is committed or rolled back based on the
    *   success or failure of this single statement.
    */
   bool executeUpdateStatement(const pdb::string      &tableName,
                               const pdb::stringmap   &fieldMap,
                               const pdb::strtointmap *fieldOptions = NULL);

   /**
    * Execute an insert statement against a fresh transaction, targeting 
    * the indicated table and using the fields and options as passed.
    * @param tableName Table into which to insert the new record.
    * @param fieldMap Map of table field names to values.
    * @param fieldOptions Optional map of field names to options. See
    *        sqlLib.h for the definition of sql_ flags applicable to this
    *        parameter.
    * @return True if the insert statement executed, false otherwise.
    * @pre The database must be connected.
    * @note The transaction is committed or rolled back based on the
    *    success or failure of this single statement.
    */
   bool executeInsertStatement(const pdb::string      &tableName,
                               const pdb::stringmap   &fieldMap,
                               const pdb::strtointmap *fieldOptions = NULL);

   /**
    * Execute a generic SQL statement against a fresh transaction.
    * @param sql A fully formed SQL statement.
    * @return True if the statement executed, false otherwise.
    * @pre The database must be connected.
    * @note The transaction is committed or rolled back based on the
    *    success or failure of this single statement.
    */
   bool executeStatement(const pdb::string &sql);

   /**
    * Execute a SQL statement that is expected to return a single field in a single
    * row, and return that result in the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] result String to receive the result, if the query is successful.
    * @pre The database must be connected.
    */
   void getOneField(const pdb::string &sql, pdb::string &result);

   /** 
    * Execute a SQL statement that is expected to return a single row of results,
    * and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldMap A map of string to string that will receive the results if the
    *    query is successful. Values will be keyed by field name.
    * @return True if successful, false otherwise.
    * @pre The database must be connected.
    */
   bool sqlToMap(const pdb::string &sql, pdb::stringmap &fieldMap);

   /** 
    * Execute a SQL statement that is expected to multiple rows of results,
    * and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldVecMap A vector of maps on string to string that will receive the results 
    *    if the query is successful. Values will be keyed by field name, and vector
    *    rows are in the order they were returned by the database.
    * @return True if successful, false otherwise.
    * @pre The database must be connected.
    */
   bool sqlToVecMap(const pdb::string &sql, pdb::vecmap &fieldVecMap);

   /** 
    * Execute a SQL statement that is expected to return multiple rows of a single
    * field, and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldSet A set of strings containing each single-field row.
    * @return True if successful, false otherwise.
    * @pre The database must be connected.
    */
   bool sqlToSet(const pdb::string &sql, pdb::stringset &fieldSet);

   /** 
    * Execute a SQL statement that is expected to return multiple rows of a single
    * field, and return the results into the second parameter.
    * @param[in] sql Fully-formed SQL query to execute.
    * @param[out] fieldVec A vector of strings containing each single-field row.
    * @return True if successful, false otherwise.
    * @pre The database must be connected.
    */
   bool sqlToVec(const pdb::string &sql, pdb::stringvec &fieldVec);
};

/**
 * Represents a bidirectional lookup table.
 */
class PrometheusLookup
{
protected:
   pdb::string tableName;    //!< Name of the database table
   pdb::stringmap valueToID; //!< Map from values to IDs.
   pdb::stringmap idToValue; //!< Map from IDs to values.

public:
   PrometheusLookup() : tableName(), valueToID(), idToValue() {}
   PrometheusLookup(PrometheusDB &db, const pdb::string &pTableName);
   PrometheusLookup(const PrometheusLookup &other);

   /**
    * Load a lookup table from the database using the standard SQL for lookups
    * (expecting the table to contain rows id, item, and valid).
    * @param db An active database connection.
    * @param pTableName Table to download.
    * @pre The database must be connected.
    */
   void load(PrometheusDB &db, const pdb::string &pTableName);

   /**
    * Load a lookup table from the database using a custom SQL statement.
    * @param db An active database connection.
    * @param pTableName Table to download.
    * @param sql Fully-formed SQL statement for performing the download. The
    *    statement must return columns id, item, and valid.
    * @pre The database must be connected.
    */
   void loadCustom(PrometheusDB &db, const pdb::string &pTableName, const pdb::string &sql);

   /**
    * Get the name of the table this lookup represents.
    * @return Immutable reference to the table name.
    */
   const pdb::string &getTableName() const { return tableName; }
   
   /**
    * Get an ID for a value, if that value is in the table.
    * @param value A value to look for in the lookup table.
    * @return The corresponding ID, if value is in the table. Otherwise
    *   an empty string is returned.
    */
   const pdb::string &idOf(const pdb::string &value) const;
   
   /**
    * Get a value for an ID, if that ID is in the table.
    * @param id An ID to look for in the lookup table.
    * @return The corresponding value, if ID is in the table. Otherwise
    *   an empty string is returned.
    */
   const pdb::string &valueOf(const pdb::string &id) const;

   /**
    * Test if the queried ID is in this lookup table.
    * @param id A numeric ID
    * @return True if this is a valid ID, false if not.
    */
   bool isValidID(const pdb::string &id) const;
   
   /**
    * Test if the queried value is in this lookup table.
    * @param value A string value.
    * @return True if this is a valud value, false if not.
    */
   bool isValidValue(const pdb::string &value) const;
};

/**
 * Global singleton lookup table manager
 */
class PrometheusLookups
{
protected:
   /** Map of lookup table objects by table name. */
   typedef std::map<pdb::string, PrometheusLookup> LookupMap;

   LookupMap lookupTables; //!< The map of currently loaded lookup tables.

   /** Protected constructor for singleton object */
   PrometheusLookups() : lookupTables() {}

public:
   /**
    * Test if this instance has loaded the indicated lookup table.
    * @param tableName Name of a Prometheus lookup database table.
    * @return True if this table is loaded, false if not.
    */
   bool haveLookup(const pdb::string &tableName) const;

   /**
    * Load a lookup table into this instance by calling PrometheusLookup::loadLookup.
    * @param db An active database connection.
    * @param tableName The table to load. An instance of PrometheusLookup will be added
    *   to the lookupTables map for this table.
    * @pre The database must be connected.
    */
   void loadLookup(PrometheusDB &db, const pdb::string &tableName);

   /**
    * Load a lookup table into this instance by calling PrometheusLookup::loadCustom.
    * @param db An active database connection.
    * @param tableName The table to load. An instance of PrometheusLookup will be added
    *   to the lookupTables map for this table.
    * @param sql Fully-formed SQL statement to load the lookup table. The query must
    *   return columns id, item, and valid.
    * @pre The database must be connected.
    */
   void loadCustom(PrometheusDB &db, const pdb::string &tableName,
                   const pdb::string &sql);
   
   /**
    * Dump all loaded lookups from this instance. The lookupTables map will be empty
    * after this call.
    */
   void purgeLookups();

   /**
    * Find an ID for the indicated value in the indicated lookup table in this instance.
    * @param tableName Name of the lookup table to target.
    * @param value A value string.
    * @return ID corresponding to value in that table, if that table is loaded
    *   and the value is valid. Otherwise a blank string is returned.
    */
   const pdb::string &idOf(const pdb::string &tableName, const pdb::string &value) const;
   
   /**
    * Find a value for the indicated ID in the indicated lookup table in this isntance.
    * @param tableName Name of the lookup table to target.
    * @param id A numeric ID.
    * @return The value corresponding to the ID in that table, if that table is loaded
    *   and the ID is valid. Otherwise a blank string is returned.
    */
   const pdb::string &valueOf(const pdb::string &tableName, const pdb::string &id) const;

   /**
    * Test this instance for an ID in the given lookup table.
    * @param tableName Name of the lookup table to target.
    * @param id ID to look for in the lookup table.
    * @return True if the table is loaded and contains this ID, false otherwise.
    */
   bool isValidID(const pdb::string &tableName, const pdb::string &id) const;
   
   /**
    * Test this instance for a value in the given lookup table.
    * @param tableName Name of the lookup table to target.
    * @param value Value to look for in the lookup table.
    * @return True if the table is loaded and contains this value, false otherwise.
    */
   bool isValidValue(const pdb::string &tableName, const pdb::string &value) const;

   /** Fetch the global singleton instance of this class.*/
   static PrometheusLookups &GetLookups();
   /** Invoke getLookup on the global singleton instance. */
   static const PrometheusLookup *GetLookup(const pdb::string &tableName);
   /** Invoke loadLookup on the global singleton instance. */
   static void LoadLookup(PrometheusDB &db, const pdb::string &tableName);
   /** Invoke loadCustom on the global singleton instance. */
   static void LoadCustom(PrometheusDB &db, const pdb::string &tableName,
                          const pdb::string &sql);
   /** Invoke loadLookups on the global singleton instance. */
   static void LoadLookups(PrometheusDB &db, const char **tableNames, size_t numTables);
   /** Invoke purgeLookups on the global singleton instance. */
   static void PurgeLookups();
   /** Invoke idOf on the global singleton instance. */
   static const pdb::string &IdOf(const pdb::string &tableName, const pdb::string &value);
   /** Invoke valueOf on the global singleton instance */
   static const pdb::string &ValueOf(const pdb::string &tableName, const pdb::string &id);
   /** Invoke isValidID on the global singleton instance */
   static bool IsValidID(const pdb::string &tableName, const pdb::string &id);
   /** Invoke isValidValue on the global singleton instance */
   static bool IsValidValue(const pdb::string &tableName, const pdb::string &value);
};

#endif // VIBC_NO_VISUALIB

#endif // PROMETHEUSDB_H__

// EOF

