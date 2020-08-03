//DEIMOS
#ifndef sqlfunctions
#define sqlfunctions

#ifndef VIBC_NO_VISUALIB

#include "VIB.h"

#include <string>
#include <map>
#include <vector>
#include <set>
#include <list>

// Defines

// current - used to determine what ExecuteInsertStatement and
// ExecuteUpdateStatement do to individual fields passed to them
#define  sql_full_processing    bit_none
#define  sql_no_uppercasing     bit_1
#define  sql_no_quoting         bit_2
#define  sql_no_left_trimming   bit_3
#define  sql_no_right_trimming  bit_4
#define  sql_no_trimming        12
#define  sql_no_processing      15        

// current - used for GetPersonName(), to specify order
// notes   - default parameter is "last, first", other is "first last"
// history - added because of links on pages vs. reports
#define first_last         0
#define last_first         1
#define first_middle_last  2
#define last_first_middle  3
#define first_last_initial 4

// Typedefs

// constant reference to string
typedef const std::string &                sqlcstr;         

// map on string to string
typedef std::map<std::string, std::string> sqlmapstrs;
typedef sqlmapstrs const &                 sqlcmapstrs;

// vector of sqlmapstrs - vecmap
typedef std::vector<sqlmapstrs>            sqlvecmap;
typedef sqlvecmap const &                  sqlcvecmap;

// list of sqlmapstrs
typedef std::list<sqlmapstrs>              sqllistmap;
typedef sqllistmap const &                 sqlclistmap;

// map on string to sqlmapstrs - mapmap
typedef std::map<std::string, sqlmapstrs>  sqlmapmap;

// map on string to int
typedef std::map<std::string, int>         sqlmapstrtoint;

// set of strings
typedef std::set<std::string>              sqlsetstr;
typedef sqlsetstr const &                  sqlcsetstr;

// vector of strings
typedef std::vector<std::string>           sqlvecstr;
typedef sqlvecstr const &                  sqlcvecstr;

// map on string to vector of strings - mapstrvec
typedef std::map<std::string, sqlvecstr>  sqlmapstrvec;
typedef sqlmapstrvec const &              sqlcmapstrvec;

// list of strings
typedef std::list<std::string>             sqlliststr;
typedef sqlliststr const &                 sqlcliststr;

// list of vector of strings - listvecstr
typedef std::list<sqlvecstr>               sqllistvecstr;
typedef sqllistvecstr const &              sqlclistvecstr;

// map on string to list of strings - mapstrlist
typedef std::map<std::string, sqlliststr>  sqlmapstrlist;
typedef sqlmapstrlist const &              sqlcmapstrlist;

// Functions

bool ConnectToDatabase(VIB::Database *dbDatabase, sqlcstr server, sqlcstr user_name, sqlcstr password);

bool LockRecordForUpdate(VIB::Transaction* dbTransaction, sqlcstr tableName, sqlcstr id);

bool ExecuteUpdateStatement(VIB::Transaction *dbTransaction, sqlcstr tableName, sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options = NULL, bool show_error = false);
bool ExecuteUpdateStatement(VIB::Database    *dbDatabase,    sqlcstr tableName, sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options = NULL, bool show_error = false);
bool ExecuteInsertStatement(VIB::Transaction *dbTransaction, sqlcstr tableName, sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options = NULL, bool show_error = false);
bool ExecuteInsertStatement(VIB::Database    *dbDatabase,    sqlcstr tableName, sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options = NULL, bool show_error = false);

std::string GetNextId(VIB::Transaction *dbTransaction, sqlcstr tableName);
std::string GetNextId(VIB::Database    *dbDatabase,    sqlcstr tableName);

std::string GetOneField(VIB::Transaction *dbTransaction, sqlcstr sqlstmt);
std::string GetOneField(VIB::Database    *dbDatabase,    sqlcstr sqlstmt);

bool FillSKVTable(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapmap &section_key_value_map);

std::string GetPersonName(VIB::Transaction *dbTransaction, sqlcstr person_id, int order = last_first);
std::string GetPersonName(VIB::Database    *dbDatabase,    sqlcstr person_id, int order = last_first);

std::string GetUserName(VIB::Transaction *dbTransaction, sqlcstr user_id, int order);
std::string GetUserName(VIB::Database    *dbDatabase,    sqlcstr user_id, int order);

bool ExecuteStatement(VIB::Transaction *dbTransaction, sqlcstr sqlstring);
bool ExecuteStatement(VIB::Database    *dbDatabase,    sqlcstr sqlstring);

bool SqlToMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrs &field_map);
bool SqlToMap(VIB::Database    *dbDatabase,    sqlcstr sql, sqlmapstrs &field_map);

bool SqlToListMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqllistmap &field_list);
bool SqlToListMap(VIB::Database    *dbDatabase,    sqlcstr sql, sqllistmap &field_list);

bool SqlToVecMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlvecmap &field_vec, bool preserve_fieldname_case = false);
bool SqlToVecMap(VIB::Database    *dbDatabase,    sqlcstr sql, sqlvecmap &field_vec, bool preserve_fieldname_case = false);

bool SqlToMapMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapmap &field_map, bool preserve_fieldname_case = false);
bool SqlToMapMap(VIB::Database    *dbDatabase,    sqlcstr sql, sqlmapmap &field_map, bool preserve_fieldname_case = false);

bool SqlToSet(VIB::Transaction *dbTransaction, sqlcstr sql, sqlsetstr &field_set);
bool SqlToSet(VIB::Database    *dbDatabase,    sqlcstr sql, sqlsetstr &field_set);

bool SqlToList(VIB::Transaction *dbTransaction, sqlcstr sql, sqlliststr &result_list);
bool SqlToList(VIB::Database    *dbDatabase,    sqlcstr sql, sqlliststr &result_list);

bool SqlToVec(VIB::Transaction *dbTransaction, sqlcstr sql, sqlvecstr &result_vec);
bool SqlToVec(VIB::Database    *dbDatabase,    sqlcstr sql, sqlvecstr &result_vec);

bool SqlToCommaString(VIB::Transaction *dbTransaction, sqlcstr sql, std::string &result_string);
bool SqlToCommaString(VIB::Database    *dbDatabase,    sqlcstr sql, std::string &result_string);

bool SqlToValueMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrs &field_map);
bool SqlToValueMap(VIB::Database    *dbDatabase,    sqlcstr sql, sqlmapstrs &field_map);

bool SqlToMapStringVec(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrvec &field_map);
bool SqlToMapStringVec(VIB::Database    *dbDatabase,    sqlcstr sql, sqlmapstrvec &field_map);

bool SqlToMapStringList(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrlist &field_map);
bool SqlToMapStringlist(VIB::Database    *dbDatabase,    sqlcstr sql, sqlmapstrlist &&field_map);

bool ExecutePreparedInsert(VIB::Transaction *dbTransaction, sqlcstr tableName, sqlvecstr &field_names, sqllistvecstr &insert_data, sqlmapstrtoint *field_options = NULL);
bool ExecutePreparedInsert(VIB::Database    *dbDatabase,    sqlcstr tableName, sqlvecstr &field_names, sqllistvecstr &insert_data, sqlmapstrtoint *field_options = NULL);

bool StdTransaction(VIB::Transaction *dbTransaction, VIB::Database *dbDatabase, bool autostart = true);
bool SpyTransaction(VIB::Transaction *dbTransaction, VIB::Database *dbDatabase, bool autostart = true);

void ResetLastSqlError();

bool ForeignKeyLookupByField(VIB::Transaction *dbTransaction, sqlcstr local_table_name, sqlcstr local_field_name, std::string &foreign_table_name, std::string &foreign_field_name);
bool ForeignKeyLookupByField(VIB::Database    *dbDatabase,    sqlcstr local_table_name, sqlcstr local_field_name, std::string &foreign_table_name, std::string &foreign_field_name);

bool ForeignKeyLookupByConstraint(VIB::Transaction *dbTransaction, sqlcstr constraint_name, std::string &local_table_name, std::string &local_field_name, std::string &foreign_table_name, std::string &foreign_field_name);
bool ForeignKeyLookupByConstraint(VIB::Database    *dbDatabase,    sqlcstr constraint_name, std::string &local_table_name, std::string &local_field_name, std::string &foreign_table_name, std::string &foreign_field_name);

bool GetPrimaryKey(VIB::Transaction *dbTransaction, sqlcstr fk_table, sqlcstr fk_field, std::string &pk_table, sqlvecstr &pk_fields);
bool GetPrimaryKey(VIB::Database    *dbDatabase,    sqlcstr fk_table, sqlcstr fk_field, std::string &pk_table, sqlvecstr &pk_fields);

bool GetForeignKeys(VIB::Transaction *dbTransaction, sqlcstr pk_table, sqlcstr pk_field, sqlvecmap &fks);
bool GetForeignKeys(VIB::Database    *dbDatabase,    sqlcstr pk_table, sqlcstr pk_field, sqlvecmap &fks);

bool GetFieldsFromTable(VIB::Transaction *dbTransaction, sqlcstr table, sqlvecstr &fields);
bool GetFieldsFromTable(VIB::Database    *dbDatabase,    sqlcstr table, sqlvecstr &fields);

bool GetFieldSetFromTable(VIB::Transaction *dbTransaction, sqlcstr table, sqlsetstr &field_set);
bool GetFieldSetFromTable(VIB::Database    *dbDatabase,    sqlcstr table, sqlsetstr &field_set); 

bool FieldSetHasField(sqlcsetstr field_set, sqlcstr field);

bool GetTablePrimaryKey(VIB::Transaction *dbDatabase, sqlcstr table, sqlvecstr &pk);
bool GetTablePrimaryKey(VIB::Database    *dbDatabase, sqlcstr table, sqlvecstr &pk);

bool CreateInsertMap(VIB::Transaction *dbTransaction, sqlcstr table, sqlmapstrs &ins_map);
bool CreateInsertMap(VIB::Database    *dbDatabase,    sqlcstr table, sqlmapstrs &ins_map);

bool ExecutePreparedSetInsert(VIB::Transaction *dbTransaction, sqlcstr table_name, sqlcstr field_name, sqlcsetstr insert_data, sqlmapstrtoint *field_options = NULL);
bool ExecutePreparedSetInsert(VIB::Database    *dbDatabase,    sqlcstr table_name, sqlcstr field_name, sqlcsetstr insert_data, sqlmapstrtoint *field_options = NULL);

bool GetSqlPlan(VIB::Transaction *dbTransaction, sqlcstr sql_statement, std::string &plan);
bool GetSqlPlan(VIB::Database    *dbDatabase,    sqlcstr sql_statement, std::string &plan);

std::string SqlOptionalBetween(sqlcstr field, sqlcstr a, sqlcstr b);

std::string TranslateSqlPlan(VIB::Transaction *dbTransaction, sqlcstr plan);

bool IsIndexed(VIB::Transaction *dbTransaction, sqlcstr table_name, sqlcstr field_name);

std::string SafeInClause(sqlcstr field, sqlcvecstr values, sqlcstr result_if_blank, bool quote_values, unsigned int max_values_in_group);
std::string SafeInClause(sqlcstr field, sqlcvecstr values);

bool DataSetToCSV(VIB::DataSet *ds, sqlcstr filename, bool headers);

//--------------------------------------------------------------------------
class sql_error
{
public:
   int ibx_error;
   int sql_error_num;
   std::string message;
   
   sql_error() { Reset(); }
   sql_error(VIB::IBError &error)
   {
      ibx_error     = error.getIBErrorCode(); 
      sql_error_num = error.getSQLCode(); 
      message       = error.getErrorMsg(); 
   }

   void Reset()
   {
      ibx_error     = 0;
      sql_error_num = 0;
      message       = "No error since last reset.";
   }

   bool IsLockConflict() const { return (ibx_error == 335544345L); }
  
   bool GetLockInfo(VIB::Transaction *dbTransaction, std::string &local_table_name, 
                    std::string &local_field_name, std::string &foreign_table_name, 
                    std::string &foreign_field_name)
   {
      unsigned int dbl_quote_1 = message.find('"');
      
      if(dbl_quote_1 != std::string::npos)
      {
         unsigned int dbl_quote_2 = message.find('"', dbl_quote_1 + 1);
         if(dbl_quote_2 != std::string::npos)
         {
            std::string constraint_name = message.substr(dbl_quote_1 + 1, dbl_quote_2 - (dbl_quote_1 + 1));
            return ForeignKeyLookupByConstraint(dbTransaction, constraint_name, 
                                                local_table_name, local_field_name, 
                                                foreign_table_name, foreign_field_name);
         } else
            return false;
      } else
         return false;
   }

   bool GetLockInfo(VIB::Database *dbDatabase, std::string &local_table_name, 
                    std::string &local_field_name, std::string &foreign_table_name, 
                    std::string &foreign_field_name)
   {
      unsigned int dbl_quote_1 = message.find('"');
      
      if(dbl_quote_1 != std::string::npos)
      {
         unsigned int dbl_quote_2 = message.find('"', dbl_quote_1 + 1);
         
         if(dbl_quote_2 != std::string::npos)
         {
            std::string constraint_name = message.substr(dbl_quote_1 + 1, dbl_quote_2 - (dbl_quote_1 + 1));
            return ForeignKeyLookupByConstraint(dbDatabase, constraint_name, 
                                                local_table_name, local_field_name, 
                                                foreign_table_name, foreign_field_name);
         } else
            return false;
      } else
         return false;
   }
};
//---------------------------------------------------------------------------
extern sql_error last_sql_lib_error;
//---------------------------------------------------------------------------

#endif // VIBC_NO_VISUALIB

#endif // sqlfunctions
