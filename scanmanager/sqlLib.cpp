// DEIMOS

#ifndef VIBC_NO_VISUALIB

#include <algorithm>
#include <iostream>

#include "util.h"
#include "sqlLib.h"

//---------------------------------------------------------------------------
sql_error last_sql_lib_error;
//---------------------------------------------------------------------------
// jhaley 20121120: StdDataSet, for VIB port - this operation was repeated several
// dozen times in this file.
static void StdDataSet(VIB::DataSet &vds, VIB::Transaction *vtr, sqlcstr sql, bool canCommit)
{
   vds.Database       = vtr->DefaultDatabase;
   vds.Transaction    = vtr->getVIBTransaction();
   vds.UniDirectional = false;

   if(canCommit)
      vtr->StartTransaction();

   vds.SelectSQL->Clear();
   vds.SelectSQL->Add(sql);

   vds.Open();
   vds.First();
}
//---------------------------------------------------------------------------
// jhaley 20110318: Connect to a database
// VIB port done 20121119
bool ConnectToDatabase(VIB::Database *dbDatabase, sqlcstr server, sqlcstr user_name, sqlcstr password)
{
   bool success = false;

   try
   {
      if(!dbDatabase->TestConnected())
      {
         std::string userNameParam = "user_name=" + user_name;
         std::string passwordParam = "password="  + password;

         dbDatabase->DatabaseName = server;
         dbDatabase->Params->Add(userNameParam);
         dbDatabase->Params->Add(passwordParam);
         dbDatabase->SQLDialect  = 3;
         dbDatabase->LoginPrompt = false;
         dbDatabase->Connected   = true;
      }
      success = true;
   }
   catch(VIB::IBError &error)
   {
      last_sql_lib_error = error;
   }

   return success;
}
//--------------------------------------------------------------------------
// VIB port done 20121119
bool LockRecordForUpdate(VIB::Transaction *dbTransaction, sqlcstr tableName, sqlcstr id)
{
   VIB::Database vdb = dbTransaction->DefaultDatabase;

   if(vdb.TestConnected())
   {
      bool can_commit = true;

      if(dbTransaction->Active)
         can_commit = false;
      
      VIB::SQL dbSQL = VIB::SQL();
      std::string sqlstring;
      sqlstring = "update " + tableName + " set id = " + id + " where id = " + id;
      
      try
      {
         dbSQL.Database    = dbTransaction->DefaultDatabase;
         dbSQL.Transaction = dbTransaction->getVIBTransaction();
       
         if(can_commit)
            dbTransaction->StartTransaction();
         
         dbSQL._SQL->Text = sqlstring;
         dbSQL.ExecQuery();

         return true;
      }
      catch(VIB::IBError &error)
      {
         last_sql_lib_error = error;
         if(dbTransaction->Active == true && can_commit)
            dbTransaction->Rollback();
         return false;
      }
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecuteUpdateStatement(VIB::Transaction *dbTransaction, sqlcstr tableName, 
                            sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options, 
                            bool show_error)
{
   VIB::Database vdb = dbTransaction->DefaultDatabase;
   sqlmapstrtoint def_field_options;

   if(!field_options)
      field_options = &def_field_options;

   if(vdb.TestConnected())
   {
      VIB::SQL dbSQL = VIB::SQL();
      std::string sqlstring;
      bool returnValue;
      bool can_commit = true;

      if(dbTransaction->Active)
         can_commit = false;

      // create sql statement:
      // update tableName set field = value, field = value, (...) where id = id_number
      sqlstring = "update " + tableName + " set ";
      for(sqlmapstrs::const_iterator itr = fieldMap.begin(); itr != fieldMap.end(); itr++)
      {
         sqlstring += itr->first;
         sqlstring += " = ";
         if(itr->second.empty())
            sqlstring += "NULL";
         else
         {
            sqlmapstrtoint::const_iterator opt_itr = field_options->find(itr->first);

            if(opt_itr == field_options->end())
               sqlstring += "'" + StripLeading(StripTrailing(ReplaceStringWithString(UppercaseString(itr->second), "'", "''"), " "), " ") + "'";
            else
            {
               std::string temp_string = "";
               int opts = opt_itr->second;
               if((opts & sql_no_uppercasing) == 0)
                  temp_string = UppercaseString(itr->second);
               else
                  temp_string = itr->second;
               if((opts & sql_no_left_trimming) == 0)
                  temp_string = StripLeading(temp_string, " ");
               if((opts & sql_no_right_trimming) == 0)
                  temp_string = StripTrailing(temp_string, " ");
               if((opts & sql_no_quoting) == 0)
                  temp_string = "'" + ReplaceStringWithString(temp_string, "'", "''") + "'";
               sqlstring += temp_string;
            }
         }
         sqlstring += ", ";
      }

      if(sqlstring != "")
      {
         sqlmapstrs::const_iterator iditr = fieldMap.find("id");
         sqlstring = sqlstring.substr(0, (sqlstring.length()-2));
         sqlstring += " where id = " + iditr->second;

         // try to execute statement (transaction should already be started from
         // locking the record for update)
         try
         {
            dbSQL.Database    = dbTransaction->DefaultDatabase;
            dbSQL.Transaction = dbTransaction->getVIBTransaction();
            if(can_commit)
               dbTransaction->StartTransaction();
            dbSQL._SQL->Text = sqlstring.c_str();
            dbSQL.ExecQuery();
            if(can_commit)
               dbTransaction->Commit();
            returnValue = true;
         }
         catch(VIB::IBError &the_error)
         {
            last_sql_lib_error = the_error;
            if(dbTransaction->Active && can_commit)
               dbTransaction->Rollback();
            returnValue = false;
            if(show_error)
            {
               std::string message = the_error.getErrorMsg(); 
               std::string library = "sqlLib:SqlExecuteUpdateStatement";
               std::string extended_info = "";
               // FIXME/TODO
            }
         }
         catch(...)
         {
            if(dbTransaction->Active && can_commit)
               dbTransaction->Rollback();
            returnValue = false;
         }
      }
      else
         returnValue = true;

      return returnValue;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecuteUpdateStatement(VIB::Database *dbDatabase, sqlcstr tableName, 
                            sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options, 
                            bool show_error)
{
   if(dbDatabase->TestConnected())
   {
      bool return_value = true;
      VIB::Transaction dbTransaction;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         return_value = ExecuteUpdateStatement(&dbTransaction, tableName, fieldMap, field_options, show_error);
      }
      catch(...)
      {
         if(dbTransaction.Active)
            dbTransaction.Rollback();
         return_value = false;
      }
      return return_value;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecuteInsertStatement(VIB::Transaction *dbTransaction, sqlcstr tableName, 
                            sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options, 
                            bool show_error)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   sqlmapstrtoint default_options;

   if(!field_options)
      field_options = &default_options;

   if(db.TestConnected())
   {
      bool can_commit = true;

      if(dbTransaction->Active)
         can_commit = false;

      VIB::SQL dbSQL;
      std::string sqlstring = "";
      std::string fieldList = "";
      std::string valueList = "";

      for(sqlmapstrs::const_iterator itr = fieldMap.begin(); itr != fieldMap.end(); itr++)
      {
         fieldList += itr->first;
         fieldList += ", ";

         if(itr->second.empty())
            valueList += "NULL";
         else
         {
            sqlmapstrtoint::const_iterator opt_itr = field_options->find(itr->first);
            if(opt_itr == field_options->end())
            {
               valueList += "'" + StripLeading(StripTrailing(ReplaceStringWithString(UppercaseString(itr->second), "'", "''"), " "), " ") + "'";
            }
            else
            {
               std::string temp_string = "";
               int opts = opt_itr->second;
               if ((opts & sql_no_uppercasing) == 0)
                  temp_string = UppercaseString(itr->second);
               else
                  temp_string = itr->second;
               if ((opts & sql_no_left_trimming) == 0)
                  temp_string = StripLeading(temp_string, " ");
               if ((opts & sql_no_right_trimming) == 0)
                  temp_string = StripTrailing(temp_string, " ");
               if ((opts & sql_no_quoting) == 0)
                  temp_string = "'" + ReplaceStringWithString(temp_string, "'", "''") + "'";
               valueList += temp_string;
            }
         }
         valueList += ", ";
      }

      if(fieldList != "")
      {
         fieldList = fieldList.substr(0,(fieldList.length()-2));
         valueList = valueList.substr(0, (valueList.length()-2));
         sqlstring = "insert into " + tableName + "(" + fieldList + ") values (" + valueList + ")";
         
         try
         {
            dbSQL.Database    = dbTransaction->DefaultDatabase;
            dbSQL.Transaction = dbTransaction->getVIBTransaction();
            
            if(can_commit)
               dbTransaction->StartTransaction();
            
            dbSQL._SQL->Text = sqlstring.c_str();
            dbSQL.ExecQuery();

            if(can_commit)
               dbTransaction->Commit();
            
            return true;
         }
         catch(VIB::IBError &the_error)
         {
            last_sql_lib_error = the_error;
            
            if(dbTransaction->Active && can_commit)
               dbTransaction->Rollback();
            
            if(show_error)
            {
               std::string message = the_error.getErrorMsg();
               std::string library = "sqlLib:SqlExecuteInsertStatement";
               std::string extended_info = "";
               // FIXME/TODO
            }

            return false;
         }
         catch(...)
         {
            if(dbTransaction->Active && can_commit)
               dbTransaction->Rollback();
            return false;
         }
      }
      return true;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecuteInsertStatement(VIB::Database *dbDatabase, sqlcstr tableName, 
                            sqlcmapstrs fieldMap, const sqlmapstrtoint *field_options, 
                            bool show_error)
{
   if(dbDatabase->TestConnected())
   {
      bool return_value = true;
      VIB::Transaction dbTransaction;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         return_value = ExecuteInsertStatement(&dbTransaction, tableName, fieldMap, field_options, show_error);
      }
      catch(...)
      {
         if(dbTransaction.Active)
            dbTransaction.Rollback();
         return_value = false;
      }

      return return_value;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetNextId(VIB::Transaction *dbTransaction, sqlcstr tableName)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   
   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string returnString;
      std::string sqlstring = "select gen_id(" + tableName + "_gen, 1) from uno";

      try
      {
         StdDataSet(dbDataSet, dbTransaction, sqlstring, false);
         
         if(dbDataSet.Eof() == false)
            returnString = dbDataSet.Fields->Fields[0]->AsString;
         else
            returnString = "";

         dbDataSet.Close();
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         returnString = "";
      }
      catch(...)
      {
         returnString = "";
      }

      return returnString;
   }
   else
      return "";
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetNextId(VIB::Database *dbDatabase, sqlcstr tableName)
{
   std::string returnString = "";
   
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         returnString = GetNextId(&dbTransaction, tableName);
      }
      catch(...)
      {
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
   }

   return returnString;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetOneField(VIB::Transaction *dbTransaction, sqlcstr sqlstring)
{
   std::string returnString = "";
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      bool canCommit = true;
      VIB::DataSet dbDataSet;
      
      try
      {
         if(dbTransaction->Active)
            canCommit = false;
                  
         StdDataSet(dbDataSet, dbTransaction, sqlstring, canCommit);
         
         if(dbDataSet.Eof() == false)
            returnString = dbDataSet.Fields->Fields[0]->AsString;

         dbDataSet.Close();
         
         if(canCommit)
            dbTransaction->Commit();
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
      }
      catch (...)
      {
      }

      if(canCommit && (dbTransaction->Active))
         dbTransaction->Rollback();
   }

   return returnString;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetOneField(VIB::Database *dbDatabase, sqlcstr sqlstring)
{
   std::string returnString = "";
   
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         returnString = GetOneField(&dbTransaction, sqlstring);
      }
      catch(...)
      {
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
   }

   return returnString;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool FillSKVTable(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapmap &section_key_value_map)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   
   if(db.TestConnected())
   {
      // empty map
      section_key_value_map.clear();

      bool canCommit = true;
      VIB::DataSet dbDataSet;
      
      try
      {
         if(dbTransaction->Active)
            canCommit = false;

         StdDataSet(dbDataSet, dbTransaction, sql, canCommit);
         
         // skv processing
         while(!dbDataSet.Eof())
         {
            std::string v_section = LowercaseString(dbDataSet.Fields->Fields[0]->AsString);
            std::string v_key     = LowercaseString(dbDataSet.Fields->Fields[1]->AsString);
            std::string v_value   = LowercaseString(dbDataSet.Fields->Fields[2]->AsString);
            section_key_value_map[v_section][v_key] = v_value;
            dbDataSet.Next();
         }

         // skv processing
         dbDataSet.Close();
         if(canCommit)
            dbTransaction->Commit();
      }
      catch(VIB::IBError &error)
      {
         last_sql_lib_error = error;
      }
      catch (...)
      {
      }

      if(canCommit && (dbTransaction->Active))
         dbTransaction->Rollback();

      return true;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetPersonName(VIB::Transaction *dbTransaction, sqlcstr person_id, int order)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   
   if(db.TestConnected())
   {
      std::string returnString = "";

      if(person_id != "0" && person_id != "" && person_id != "new")
      {
         sqlmapstrs name_map;
         std::string sql = 
            "select PN.prefix, PN.first_name, PN.middle_name, PN.last_name, PN.suffix "
            "from people P inner join person_names PN on P.person_names_id = PN.id "
            "where P.id = " + person_id;

         if(SqlToMap(dbTransaction, sql, name_map))
         {
            if(!name_map.empty())
            {
               if(order == last_first || order == last_first_middle)
               {
                  returnString = name_map["last_name"] + (name_map["last_name"].size() > 0 ? ", " : "")
                     + name_map["first_name"];
                  returnString += ((returnString.size() > 0 && returnString[returnString.length()-1] != ' ' ? " " : "")
                     + (order == last_first_middle ? name_map["middle_name"] : std::string("")));
                  returnString += ((returnString.size() > 0 && returnString[returnString.length()-1] != ' ' ? " " : "")
                     + name_map["suffix"]);
               }
               else if (order == first_last || order == first_middle_last)
               {
                  returnString = name_map["first_name"];
                  returnString += (returnString.size() > 0 && returnString[returnString.length()-1] != ' ' ? " " : "")
                     + (order == first_middle_last ? name_map["middle_name"] : std::string(""));
                  returnString += ((returnString.size() > 0 && returnString[returnString.length()-1] != ' ' ? " " : "")
                     + name_map["last_name"]);
                  returnString += ((returnString.size() > 0 && returnString[returnString.length()-1] != ' ' ? " " : "")
                     + name_map["suffix"]);
               }

               returnString = PrettyString(returnString);

               if(order == first_last_initial)
               {
                  returnString = PrettyString(name_map["first_name"]);
                  returnString += ((returnString.size() > 0 && returnString[returnString.length()-1] != ' ' ? " " : "")
                     + AcronymString(name_map["last_name"]));
               }

               returnString = StripTrailing(returnString, " ");
               returnString = StripLeading(returnString, " ");
            }
            else
               returnString = "None";
         }
      }
      else
         returnString = "None";

      return returnString;
   }
   else
      return "";
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetPersonName(VIB::Database *dbDatabase, sqlcstr person_id, int order)
{
   std::string returnString = "";

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         returnString = GetPersonName(&dbTransaction, person_id, order);
      }
      catch(...)
      {
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
   }

   return returnString;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetUserName(VIB::Transaction *dbTransaction, sqlcstr user_id, int order)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      std::string returnString = "";
      
      if(user_id != "0" && user_id != "" && user_id != "new")
      {
         sqlmapstrs name_map;
         std::string sql = "select person_id from users where id = " + user_id;
         std::string person_id = GetOneField(dbTransaction, sql);
         
         if(!IsBlankOrZero(person_id))
            returnString = GetPersonName(dbTransaction, person_id, order);
         else
            returnString = "<unknown>";
      }
      else
         returnString = "None";

      return returnString;
   }

   return "";
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string GetUserName(VIB::Database *dbDatabase, sqlcstr user_id, int order)
{
   std::string returnString = "";

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         returnString = GetUserName(&dbTransaction, user_id, order);
      }
      catch(...)
      {
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
   }

   return returnString;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecuteStatement(VIB::Transaction *dbTransaction, sqlcstr sqlstring)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      bool canCommit = true;
      bool toReturn = true;
      VIB::SQL dbSQL;

      if(dbTransaction->Active)
         canCommit = false;

      try
      {
         dbSQL.Database    = dbTransaction->DefaultDatabase;
         dbSQL.Transaction = dbTransaction->getVIBTransaction();
         
         if(canCommit)
            dbTransaction->StartTransaction();
         
         dbSQL._SQL->Text = sqlstring;
         dbSQL.ExecQuery();
         
         if(canCommit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
      }
      catch (...)
      {
         toReturn = false;
      }

      if(canCommit && dbTransaction->Active)
         dbTransaction->Rollback();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecuteStatement(VIB::Database *dbDatabase, sqlcstr sqlstring)
{
   if(dbDatabase->TestConnected())
   {
      bool toReturn = true;
      VIB::Transaction dbTransaction;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = ExecuteStatement(&dbTransaction, sqlstring);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port 20121120
bool SqlToMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrs &field_map)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   
   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string field_name;
      bool toReturn = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
   
      field_map.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);
         
         if(!dbDataSet.Eof())
         {
            for(int i = 0; i < dbDataSet.Fields->Count; i++)
            {
               field_name = LowercaseString(dbDataSet.Fields->Fields[i]->FieldName);
               field_map[field_name] = dbDataSet.FieldByName(field_name)->AsString;
            }
         }
         
         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMap(VIB::Database *dbDatabase, sqlcstr sql, sqlmapstrs &field_map)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToMap(&dbTransaction, sql, field_map);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToVecMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlvecmap &field_vec, bool preserve_fieldname_case)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string field_name;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_vec.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);

         while(!dbDataSet.Eof())
         {
            sqlmapstrs temp_map;
            
            for(int i = 0; i < dbDataSet.Fields->Count; i++)
            {
               field_name = dbDataSet.Fields->Fields[i]->FieldName;
               if(!preserve_fieldname_case) 
                  field_name = LowercaseString(field_name); // default false; compat w/ Prometheus
               temp_map[field_name] = dbDataSet.FieldByName(field_name)->AsString;
            }
            field_vec.push_back(temp_map);
            dbDataSet.Next();
         }
         
         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError &error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToVecMap(VIB::Database *dbDatabase, sqlcstr sql, sqlvecmap &field_vec, bool preserve_fieldname_case)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToVecMap(&dbTransaction, sql, field_vec, preserve_fieldname_case);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToListMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqllistmap &field_list)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string field_name;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_list.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);
         
         while(!dbDataSet.Eof())
         {
            sqlmapstrs temp_map;
            
            for(int i = 0; i < dbDataSet.Fields->Count; i++)
            {
               field_name = LowercaseString(dbDataSet.Fields->Fields[i]->FieldName);
               temp_map[field_name] = dbDataSet.FieldByName(field_name.c_str())->AsString;
            }
            field_list.push_back(temp_map);
            dbDataSet.Next();
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToListMap(VIB::Database *dbDatabase, sqlcstr sql, sqllistmap &field_list)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToListMap(&dbTransaction, sql, field_list);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// SqlToMapMap is NOT TESTED.  I thought I needed it, wrote it, then realized I didn't need it, so I didn't test it.
// I -think- it should work, but you should make -sure- before you use it!
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMapMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapmap &field_map, bool preserve_fieldname_case)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string field_name, key;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_map.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);

         while(!dbDataSet.Eof())
         {
            // assume the data in the first column is the key
            key = dbDataSet.Fields->Fields[0]->AsString;

            // iterate
            sqlmapstrs temp_map;
            for(int i = 0; i < dbDataSet.Fields->Count; i++)
            {
               field_name = dbDataSet.Fields->Fields[i]->FieldName;
               if(!preserve_fieldname_case) 
                  field_name = LowercaseString(field_name); // default false; compat w/ Prometheus
               temp_map[field_name] = dbDataSet.FieldByName(field_name)->AsString;
            }

            // slap it in the map.  overwrite if necessary.  not going to bother with the more generic multimap case.
            field_map[key] = temp_map;
            dbDataSet.Next();
         }
         
         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMapMap(VIB::Database *dbDatabase, sqlcstr sql, sqlmapmap &field_map, bool preserve_fieldname_case)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToMapMap(&dbTransaction, sql, field_map, preserve_fieldname_case);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToSet(VIB::Transaction *dbTransaction, sqlcstr sql, sqlsetstr &field_set)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_set.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);
         
         while(!dbDataSet.Eof())
         {
            field_set.insert(dbDataSet.Fields->Fields[0]->AsString);
            dbDataSet.Next();
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();

         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToSet(VIB::Database *dbDatabase, sqlcstr sql, sqlsetstr &field_set)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToSet(&dbTransaction, sql, field_set);
      }
      catch (...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToList(VIB::Transaction *dbTransaction, sqlcstr sql, sqlliststr &result_list)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      result_list.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);
         
         while(!dbDataSet.Eof())
         {
            result_list.push_back(dbDataSet.Fields->Fields[0]->AsString);
            dbDataSet.Next();
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();

         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToList(VIB::Database *dbDatabase, sqlcstr sql, sqlliststr &result_list)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToList(&dbTransaction, sql, result_list);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToVec(VIB::Transaction *dbTransaction, sqlcstr sql, sqlvecstr &result_vec)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      result_vec.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);

         while(!dbDataSet.Eof())
         {
            result_vec.push_back(dbDataSet.Fields->Fields[0]->AsString);
            dbDataSet.Next();
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();

         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToVec(VIB::Database *dbDatabase, sqlcstr sql, sqlvecstr &result_vec)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToVec(&dbTransaction, sql, result_vec);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToValueMap(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrs &field_map)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string key;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_map.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);

         if(dbDataSet.FieldCount == 2)
         {
            while(!dbDataSet.Eof())
            {
               key            = dbDataSet.Fields->Fields[0]->AsString;
               field_map[key] = dbDataSet.Fields->Fields[1]->AsString;
               dbDataSet.Next();
            }
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToValueMap(VIB::Database *dbDatabase, sqlcstr sql, sqlmapstrs &field_map)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;

      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToValueMap(&dbTransaction, sql, field_map);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
      
      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMapStringVec(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrvec &field_map)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string key;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_map.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);

         if(dbDataSet.FieldCount == 2)
         {
            while(!dbDataSet.Eof())
            {
               key = dbDataSet.Fields->Fields[0]->AsString;
               field_map[key].push_back(dbDataSet.Fields->Fields[1]->AsString);
               dbDataSet.Next();
            }
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError &error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMapStringVec(VIB::Database *dbDatabase, sqlcstr sql, sqlmapstrvec &field_map)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToMapStringVec(&dbTransaction, sql, field_map);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMapStringList(VIB::Transaction *dbTransaction, sqlcstr sql, sqlmapstrlist &field_map)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   
   if(db.TestConnected())
   {
      VIB::DataSet dbDataSet;
      std::string key;
      bool toReturn   = true;
      bool can_commit = true;
      
      if(dbTransaction->Active)
         can_commit = false;
      
      field_map.clear();
      
      try
      {
         StdDataSet(dbDataSet, dbTransaction, sql, can_commit);

         if(dbDataSet.FieldCount == 2)
         {
            while(!dbDataSet.Eof())
            {
               key = dbDataSet.Fields->Fields[0]->AsString;
               field_map[key].push_back(dbDataSet.Fields->Fields[1]->AsString);
               dbDataSet.Next();
            }
         }

         dbDataSet.Close();
         
         if(can_commit)
            dbTransaction->Commit();
         
         toReturn = true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }
      catch (...)
      {
         toReturn = false;
         if ((dbTransaction->Active) && (can_commit))
            dbTransaction->Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToMapStringList(VIB::Database *dbDatabase, sqlcstr sql, sqlmapstrlist &field_map)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToMapStringList(&dbTransaction, sql, field_map);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecutePreparedInsert(VIB::Transaction *dbTransaction, sqlcstr tableName,
                           sqlvecstr &field_names,
                           sqllistvecstr &insert_data,
                           sqlmapstrtoint *field_options)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   sqlmapstrtoint default_options;

   if(!field_options)
      field_options = &default_options;

   if(db.TestConnected())
   {
      bool can_commit = true;

      if(dbTransaction->Active)
         can_commit = false;

      if(!field_names.empty())
      {
         VIB::SQL dbSQL;

         try
         {
            dbSQL.Database    = dbTransaction->DefaultDatabase;
            dbSQL.Transaction = dbTransaction->getVIBTransaction();
            
            if(can_commit)
               dbTransaction->StartTransaction();

            // sqlstring with parameter names
            unsigned int n = field_names.size();
            sqlvecstr param_names = field_names;
            
            for(unsigned int i = 0; i < n; i++)
               param_names[i] = ":" + param_names[i];

            std::string sqlstring = 
               "insert into " + tableName + "(" + VecToCommaString(field_names) + ") "
               "values (" + VecToCommaString(param_names) + ")";
            
            dbSQL._SQL->Text = sqlstring;
            dbSQL.Prepare();
            
            std::string field_value;
            std::string field_name;

            // loop through the rows
            for(sqllistvecstr::iterator i_row = insert_data.begin(); i_row != insert_data.end(); i_row++)
            {
               // get the params in there
               for(unsigned int i = 0; i < n; i++)
               {
                  field_value = (*i_row)[i];
                  field_name  = field_names[i];
                  if (field_value.empty())
                  {
                     dbSQL.Params->ByName(field_name)->IsNull = true;
                  }
                  else
                  {
                     sqlmapstrtoint::iterator opt_itr = field_options->find(field_name);
                     
                     if(opt_itr == field_options->end())
                     {
                        field_value = StripLeading(StripTrailing(UppercaseString(field_value), " "), " ");
                     }
                     else
                     {
                        int opts = opt_itr->second;
                        if ((opts & sql_no_uppercasing) == 0)
                           field_value = UppercaseString(field_value);
                        if ((opts & sql_no_left_trimming) == 0)
                           field_value = StripLeading(field_value, " ");
                        if ((opts & sql_no_right_trimming) == 0)
                           field_value = StripTrailing(field_value, " ");
                     }
                     dbSQL.Params->ByName(field_name)->AsString = field_value;
                  }
               }

               dbSQL.ExecQuery();
            }

            if(can_commit)
               dbTransaction->Commit();

            return true;
         }
         catch(VIB::IBError & error)
         {
            last_sql_lib_error = error;
            if(dbTransaction->Active && can_commit)
               dbTransaction->Rollback();
            return false;
         }
         catch (...)
         {
            if(dbTransaction->Active && can_commit)
               dbTransaction->Rollback();
            return false;
         }
      } 
      else
      {
         // no fields to insert -- we're not going to insert with zero fields (theoretically useful, but not anywhere in prometheus, that i know of.)
         return false;
      }
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecutePreparedInsert(VIB::Database * dbDatabase, sqlcstr tableName,
                            sqlvecstr &field_names,
                            sqllistvecstr &insert_data,
                            sqlmapstrtoint *field_options)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = ExecutePreparedInsert(&dbTransaction, tableName, field_names, insert_data, field_options);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ExecutePreparedSetInsert(VIB::Transaction *dbTransaction, sqlcstr table_name,
                              sqlcstr field_name, sqlcsetstr insert_data,
                              sqlmapstrtoint *field_options)
{
   VIB::Database db = dbTransaction->DefaultDatabase;
   sqlmapstrtoint default_options;

   if(!field_options)
      field_options = &default_options;

   if(db.TestConnected())
   {
      bool can_commit = true;

      if(dbTransaction->Active)
         can_commit = false;

      VIB::SQL dbSQL;

      try
      {
         dbSQL.Database    = dbTransaction->DefaultDatabase;
         dbSQL.Transaction = dbTransaction->getVIBTransaction();
         
         if(can_commit)
            dbTransaction->StartTransaction();

         std::string sqlstring = 
            "insert into " + table_name + "(" + field_name + ") "
            "values (:" + field_name + ")";
         
         dbSQL._SQL->Text = sqlstring;
         dbSQL.Prepare();
         
         std::string field_value;

         int param_type = dbSQL.Params->ByName(field_name)->SQLType;

         // loop through the rows
         for(sqlsetstr::const_iterator i = insert_data.begin(); i != insert_data.end(); i++)
         {
            field_value = (*i);
            if(field_value.empty())
               dbSQL.Params->ByName(field_name)->IsNull = true;
            else
            {
               sqlmapstrtoint::iterator opt_itr = field_options->find(field_name);
               if(opt_itr == field_options->end())
               {
                  field_value = StripLeading(StripTrailing(UppercaseString(field_value), " "), " ");
                  if(param_type == VIB_SQL_TEXT      || param_type == VIB_SQL_VARYING   || 
                     param_type == VIB_SQL_TYPE_DATE || param_type == VIB_SQL_TYPE_TIME || 
                     param_type == VIB_SQL_TIMESTAMP || param_type == VIB_SQL_BLOB      || 
                     param_type == VIB_SQL_ARRAY) // complete?
                  {
                     field_value = "'" + ReplaceStringWithString(field_value, "'", "''") + "'";
                  }
               }
               else
               {
                  int opts = opt_itr->second;
                  if((opts & sql_no_uppercasing) == 0)
                     field_value = UppercaseString(field_value);
                  if((opts & sql_no_left_trimming) == 0)
                     field_value = StripLeading(field_value, " ");
                  if((opts & sql_no_right_trimming) == 0)
                     field_value = StripTrailing(field_value, " ");
                  if((opts & sql_no_quoting) == 0)
                     field_value = "'" + ReplaceStringWithString(field_value, "'", "''") + "'";
               }

               dbSQL.Params->ByName(field_name)->AsString = field_value;
            }

            dbSQL.ExecQuery();
         }

         if(can_commit)
            dbTransaction->Commit();
         
         return true;
      }
      catch(VIB::IBError & error)
      {
         last_sql_lib_error = error;
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
         return false;
      }
      catch(...)
      {
         if(dbTransaction->Active && can_commit)
            dbTransaction->Rollback();
         return false;
      }
   }
   else
      return false;
}
//--------------------------------------------------------------------------
//
bool ExecutePreparedSetInsert(VIB::Database *dbDatabase, sqlcstr table_name,
                              sqlcstr field_name, sqlcsetstr &insert_data,
                              sqlmapstrtoint *field_options)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = ExecutePreparedSetInsert(&dbTransaction, table_name, field_name, insert_data, field_options);
      }
      catch (...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// just starts it up with normal stuff -- must already have been created though.
// VIB port done 20121120
bool StdTransaction(VIB::Transaction *dbTransaction, VIB::Database *dbDatabase, bool autostart)
{
   bool result = true;
   
   try
   {
      // create the object if needed, clear it out (maybe modify later?)
      dbTransaction->DefaultDatabase = dbDatabase->getVIBDatabase();
      dbTransaction->DefaultAction   = vib_TARollback;
      dbTransaction->Params->Add("nowait");
      if(autostart)
         dbTransaction->Active = true;
   }
   catch(VIB::IBError & error)
   {
      last_sql_lib_error = error;
      result = false;
   }
   catch(...)
   {
      last_sql_lib_error.Reset();
      last_sql_lib_error.message = "A non-InterBase error of some sort occurred.";
      result = false;
   }

   return result;
}
//--------------------------------------------------------------------------
// read committed -- must already have been created though.
// VIB port done 20121120
bool SpyTransaction(VIB::Transaction *dbTransaction, VIB::Database *dbDatabase, bool autostart)
{
   bool result = true;
   
   try
   {
      // create the object if needed, clear it out (maybe modify later?)
      dbTransaction->DefaultDatabase = dbDatabase->getVIBDatabase();
      dbTransaction->DefaultAction   = vib_TARollback;
      dbTransaction->Params->Add("nowait");
      dbTransaction->Params->Add("read_committed");
      dbTransaction->Params->Add("rec_version");
      if(autostart)
         dbTransaction->Active = true;
   }
   catch(VIB::IBError & error)
   {
      last_sql_lib_error = error;
      result = false;
   }
   catch (...)
   {
      last_sql_lib_error.Reset();
      last_sql_lib_error.message = "A non-InterBase error of some sort occurred.";
      result = false;
   }

   return result;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToCommaString(VIB::Transaction *dbTransaction, sqlcstr sql, std::string &result_string)
{
   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      bool to_return;
      sqlvecstr temp_vec;
      
      to_return = SqlToVec(dbTransaction, sql, temp_vec);
      
      if(!temp_vec.empty())
         result_string = VecToCommaString(temp_vec);
      else
         result_string = "";
      
      return to_return;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool SqlToCommaString(VIB::Database *dbDatabase, sqlcstr sql, std::string &result_string)
{
   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      try
      {
         StdTransaction(&dbTransaction, dbDatabase, false);
         toReturn = SqlToCommaString(&dbTransaction, sql, result_string);
      }
      catch (...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
void ResetLastSqlError ()
{
   last_sql_lib_error.Reset();
   return;
}
//--------------------------------------------------------------------------
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool ForeignKeyLookup(VIB::Transaction *dbTransaction, sqlcstr local_table_name, sqlcstr local_field_name, 
                      std::string &foreign_table_name, std::string &foreign_field_name)
{
   if (!dbTransaction)
      return false;

   foreign_table_name = "";
   foreign_field_name = "";

   // shamelessly pulled from paul's codeSpecialDataQuery -- many thanks to paul for getting this "just right"
   VIB::Database db = dbTransaction->DefaultDatabase;
   if(db.TestConnected())
   {
      std::string gsql;
      gsql = 
         "SELECT C.RDB$RELATION_NAME pk_table, E.RDB$FIELD_NAME pk_field "
         "FROM RDB$RELATION_CONSTRAINTS A "
         "   INNER JOIN RDB$INDICES B ON A.RDB$INDEX_NAME = B.RDB$INDEX_NAME "
         "   INNER JOIN RDB$INDICES C ON B.RDB$FOREIGN_KEY = C.RDB$INDEX_NAME "
         "   INNER JOIN RDB$INDEX_SEGMENTS D ON B.RDB$INDEX_NAME = D.RDB$INDEX_NAME "
         "   INNER JOIN RDB$INDEX_SEGMENTS E ON B.RDB$FOREIGN_KEY = E.RDB$INDEX_NAME "
         "WHERE A.RDB$RELATION_NAME='" + local_table_name + "' AND "
         "   B.RDB$RELATION_NAME='" + local_table_name + "' AND "
         "   D.RDB$FIELD_NAME='" + local_field_name + "' AND "
         "   A.RDB$CONSTRAINT_TYPE='FOREIGN KEY' "
         "ORDER BY A.RDB$CONSTRAINT_NAME ";

      sqlvecmap gsql_results;
      if(!SqlToVecMap(dbTransaction, gsql, gsql_results))
         return false;

      if(!gsql_results.empty())
      {
         foreign_table_name = StripTrailing(gsql_results[0]["pk_table"], " ");
         foreign_field_name = StripTrailing(gsql_results[0]["pk_field"], " ");
         return true;
      }
      else
         return false;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ForeignKeyLookup(VIB::Database *dbDatabase, sqlcstr local_table_name, sqlcstr local_field_name, 
                      std::string &foreign_table_name, std::string &foreign_field_name)
{
   if(!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      StdTransaction(&dbTransaction, dbDatabase);
      bool toReturn = true;
      
      try
      {
         toReturn = ForeignKeyLookup(&dbTransaction, local_table_name, local_field_name, foreign_table_name, foreign_field_name);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool ForeignKeyLookupByConstraint(VIB::Transaction *dbTransaction, sqlcstr constraint_name, 
                                  std::string &local_table_name, std::string &local_field_name, 
                                  std::string &foreign_table_name, std::string &foreign_field_name)
{
   if(!dbTransaction)
      return false;

   VIB::Database db = dbTransaction->DefaultDatabase;
   if(db.TestConnected())
   {
      std::string info_sql = 
         "select local_table.rdb$relation_name a, local_field.rdb$field_name b, foreign_table.rdb$relation_name c, foreign_field.rdb$field_name d "
         "from rdb$ref_constraints "
         "inner join rdb$relation_constraints local_constraint on local_constraint.rdb$constraint_name = rdb$ref_constraints.rdb$constraint_name "
         "   and local_constraint.rdb$constraint_type = 'FOREIGN KEY' "
         "inner join rdb$relation_constraints foreign_constraint on foreign_constraint.rdb$constraint_name = rdb$ref_constraints.rdb$const_name_uq "
         "   and foreign_constraint.rdb$constraint_type = 'PRIMARY KEY' "
         "inner join rdb$indices local_table on local_table.rdb$index_name = local_constraint.rdb$index_name "
         "inner join rdb$indices foreign_table on foreign_table.rdb$index_name = foreign_constraint.rdb$index_name "
         "inner join rdb$index_segments local_field on local_field.rdb$index_name = local_constraint.rdb$index_name "
         "inner join rdb$index_segments foreign_field on foreign_field.rdb$index_name = foreign_constraint.rdb$index_name "
         "where rdb$ref_constraints.rdb$constraint_name = '" + constraint_name + "'";
      
      sqlmapstrs info_result;
      if(!SqlToMap(dbTransaction, info_sql, info_result))
         return false;
      
      if(!info_result.empty())
      {
         local_table_name   = StripTrailing(info_result["a"], " ");
         local_field_name   = StripTrailing(info_result["b"], " ");
         foreign_table_name = StripTrailing(info_result["c"], " ");
         foreign_field_name = StripTrailing(info_result["d"], " ");
         return true;
      }
      else
         return false;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool ForeignKeyLookupByConstraint(VIB::Database *dbDatabase, sqlcstr constraint_name, 
                                  std::string &local_table_name, std::string &local_field_name, 
                                  std::string &foreign_table_name, std::string &foreign_field_name)
{
   if(!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      StdTransaction(&dbTransaction, dbDatabase);
      bool toReturn = true;
      
      try
      {
         toReturn = ForeignKeyLookupByConstraint(&dbTransaction, constraint_name, local_table_name, local_field_name, foreign_table_name, foreign_field_name);
      }
      catch (...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// return the Primary Key that given Foreign Key links to.  This can be multiple fields in one table, so table is string and fields is a vector
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool GetPrimaryKey(VIB::Transaction *dbTransaction, sqlcstr fk_table, sqlcstr fk_field, std::string &pk_table, sqlvecstr &pk_fields)
{
   if(!dbTransaction)
      return false;

   pk_table = "";

   VIB::Database db = dbTransaction->DefaultDatabase;

   if(db.TestConnected())
   {
      std::string pk_sql = 
         "select I2.rdb$relation_name pk_table, IS2.rdb$field_name pk_field "
         "from rdb$relation_constraints RC "
         "   inner join rdb$indices I1 on RC.rdb$index_name=I1.rdb$index_name "
         "   inner join rdb$indices I2 on I1.rdb$foreign_key = I2.rdb$index_name "
         "   inner join rdb$index_segments IS1 on I1.rdb$index_name = IS1.rdb$index_name "
         "   inner join rdb$index_segments IS2 on I1.rdb$foreign_key = IS2.rdb$index_name "
         "where RC.rdb$relation_name='" + fk_table + "' and "
         "   I1.rdb$relation_name='" + fk_table + "' and "
         "   IS1.rdb$field_name='" + fk_field + "' and "
         "   RC.rdb$constraint_type='FOREIGN KEY' "
         "order by RC.rdb$constraint_name ";

      sqlvecmap pk_sql_results;
      if(!SqlToVecMap(dbTransaction, pk_sql, pk_sql_results))
         return false;

      if(!pk_sql_results.empty())
      {
         pk_table = StripTrailing(pk_sql_results[0]["pk_table"], " ");
         for(unsigned int i=0; i<pk_sql_results.size(); i++)
            pk_fields.push_back(StripTrailing(pk_sql_results[i]["pk_field"], " "));
         return true;
      }
      else
         return false;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// return the Primary Key that given Foreign Key links to.  This can be multiple fields in one table, so table is string and fields is a vector
// VIB port done 20121120
bool GetPrimaryKey(VIB::Database *dbDatabase, sqlcstr fk_table, sqlcstr fk_field, std::string &pk_table, sqlvecstr &pk_fields)
{
   if(!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      StdTransaction(&dbTransaction, dbDatabase);
      bool toReturn = true;
      
      try
      {
         toReturn = GetPrimaryKey(&dbTransaction, fk_table, fk_field, pk_table, pk_fields);
      }
      catch (...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
      
      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// return the Foreign Keys that link to given Primary Key.
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool GetForeignKeys(VIB::Transaction *dbTransaction, sqlcstr pk_table, sqlcstr pk_field, sqlvecmap &fks)
{
   if(!dbTransaction)
      return false;

   VIB::Database db = dbTransaction->DefaultDatabase;
   if(db.TestConnected())
   {
      std::string fk_sql = 
         "select RC.rdb$relation_name fk_table, IS1.rdb$field_name fk_field "
         "from rdb$relation_constraints RC, rdb$indices I1, rdb$indices I2 "
         "   inner join rdb$index_segments IS1 on I1.rdb$index_name = IS1.rdb$index_name "
         "   inner join rdb$index_segments IS2 on I1.rdb$foreign_key = IS2.rdb$index_name "
         "where I2.rdb$relation_name='" + pk_table + "' and "
         "   IS2.rdb$field_name='" + pk_field + "' and "
         "   RC.rdb$constraint_type='FOREIGN KEY' and "
         "   I1.rdb$foreign_key = I2.rdb$index_name and "
         "   RC.rdb$index_name=I1.rdb$index_name and "
         "   RC.rdb$relation_name=I1.rdb$relation_name "
         "order by RC.rdb$constraint_name ";

      if(!SqlToVecMap(dbTransaction, fk_sql, fks))
         return false;
      
      if(!fks.empty())
      {
         for(unsigned int i = 0; i < fks.size(); i++)
         {
            fks[i]["fk_table"] = StripTrailing(fks[i]["fk_table"], " ");
            fks[i]["fk_field"] = StripTrailing(fks[i]["fk_field"], " ");
         }
         return true;
      }
      else
         return false;              // should zero matching FKs return false?
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// return the Foreign Keys that link to given Primary Key.
// VIB port done 20121120
bool GetForeignKeys(VIB::Database *dbDatabase, sqlcstr pk_table, sqlcstr pk_field, sqlvecmap &fks)
{
   if(!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      StdTransaction(&dbTransaction, dbDatabase);
      bool toReturn = true;
      
      try
      {
         toReturn = GetForeignKeys(&dbTransaction, pk_table, pk_field, fks);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool GetFieldsFromTable(VIB::Transaction *dbTransaction, sqlcstr table, sqlvecstr &fields)
{
   if(!dbTransaction)
      return false;

   std::string sql;
   VIB::Database db = dbTransaction->DefaultDatabase;
   if(db.TestConnected())
   {
      // get a list of all the fields in the table we were passed
      sql = 
         "select RDB$FIELD_NAME "
         "from RDB$RELATION_FIELDS where RDB$RELATION_NAME = UPPER('" + table + "') "  // jhaley: added UPPER()
         "order by RDB$FIELD_POSITION ";
      
      if(!SqlToVec(dbTransaction, sql, fields))
         return false;

      if(!fields.empty())
      {
         for(unsigned int i = 0; i < fields.size(); i++)
            fields[i] = StripTrailing(fields[i], " ");
         return true;
      }
      else
         return false;              // should zero matching fields return false?
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool GetFieldsFromTable(VIB::Database *dbDatabase, sqlcstr table, sqlvecstr &fields)
{
   if(!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      StdTransaction(&dbTransaction, dbDatabase);
      bool toReturn = true;
      
      try
      {
         toReturn = GetFieldsFromTable(&dbTransaction, table, fields);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }
      
      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// jhaley 20110107: Routine to get a list of the fields in a table in set form, for O(log(N)) searchability.
// VIB port done 20121120
bool GetFieldSetFromTable(VIB::Transaction *dbTransaction, sqlcstr table, sqlsetstr &field_set)
{
   bool result;
   sqlvecstr fields;
   result = GetFieldsFromTable(dbTransaction, table, fields);
   VecToSet(fields, field_set);
   return result;
}
//--------------------------------------------------------------------------
// jhaley 20110107: TIBDatabase overload of GetFieldSetFromTable, for convenience and to match all the other routines.
// VIB port done 20121120
bool GetFieldSetFromTable(VIB::Database *dbDatabase, sqlcstr table, sqlsetstr &field_set)
{
   bool result;
   sqlvecstr fields;
   result = GetFieldsFromTable(dbDatabase, table, fields);
   VecToSet(fields, field_set);
   return result;
}
//--------------------------------------------------------------------------
// jhaley 20110107: Test if a field set contains a given field.
bool FieldSetHasField(sqlsetstr &field_set, sqlcstr field)
{
   return (field_set.find(UppercaseString(field)) != field_set.end());
}
//--------------------------------------------------------------------------
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool GetTablePrimaryKey(VIB::Transaction *dbTransaction, sqlcstr table, sqlvecstr &pk)
{
   if(!dbTransaction)
      return false;

   std::string sql;
   VIB::Database db = dbTransaction->DefaultDatabase;
   if(db.TestConnected())
   {
      // get a list of all the fields in the table we were passed
      sql = 
         "select ISeg.rdb$field_name pk_field "
         "from rdb$relation_constraints RC inner join rdb$indices Idx on "
         "      RC.rdb$index_name=Idx.rdb$index_name and RC.rdb$relation_name=Idx.rdb$relation_name "
         "   inner join rdb$index_segments ISeg on Idx.rdb$index_name = ISeg.rdb$index_name "
         "where Idx.rdb$relation_name='" + table + "' and "
         "   RC.rdb$constraint_type='PRIMARY KEY' ";
      
      if(!SqlToVec(dbTransaction, sql, pk))
         return false;

      if(!pk.empty())
      {
         for(unsigned int i = 0; i < pk.size(); i++)
            pk[i] = StripTrailing(pk[i], " ");
         return true;
      }
      else
         return false;              // should zero matching fields return false?
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool GetTablePrimaryKey(VIB::Database *dbDatabase, sqlcstr table, sqlvecstr &pk)
{
   if (!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      bool toReturn = true;
      
      StdTransaction(&dbTransaction, dbDatabase);
      
      try
      {
         toReturn = GetTablePrimaryKey(&dbTransaction, table, pk);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// Unlike other functions in sqllib, this one looks to REQUIRE an active transaction to work
// VIB port done 20121120
bool CreateInsertMap(VIB::Transaction *dbTransaction, sqlcstr table, sqlmapstrs &ins_map)
{
   bool all_is_well = true;
   
   if(!dbTransaction)
      all_is_well = false;

   VIB::Database db = dbTransaction->DefaultDatabase;
   std::string sql;
   if(all_is_well && db.TestConnected())
   {
      sqlvecmap fields;
      std::string sql = 
         "select RelFld.rdb$field_name field_name, RelCon.rdb$constraint_type const_type "
         "from rdb$relations Rel inner join rdb$relation_fields RelFld on Rel.rdb$relation_name = RelFld.rdb$relation_name "
         "  left join ( "
         "    rdb$relation_constraints RelCon inner join rdb$indices Idx on Idx.rdb$index_name = RelCon.rdb$index_name "
         "      and Idx.rdb$relation_name = '" + UppercaseString(table) + "' and RelCon.rdb$relation_name = Idx.rdb$relation_name "
         "      and RelCon.rdb$constraint_type = 'FOREIGN KEY' "
         "    inner join rdb$index_segments ISeg on Idx.rdb$index_name = ISeg.rdb$index_name "
         "  ) on RelFld.rdb$field_name = ISeg.rdb$field_name and RelFld.rdb$relation_name = Idx.rdb$relation_name "
         "where Rel.rdb$relation_name = '" + UppercaseString(table) + "' ";

      if(all_is_well && !SqlToVecMap(dbTransaction, sql, fields))
         all_is_well = false;

      if(all_is_well && !fields.empty())
      {
         for(unsigned int i = 0; i < fields.size(); i++)
         {
            std::string field_value = LowercaseString(StripTrailing(fields[i]["field_name"], " "));
            
            if(ins_map.find(field_value)==ins_map.end())
            { 
               // If this key doesn't already exist, add a default value
               // const_type is either going to be "" or "FOREIGN KEY"
               if(StripTrailing(fields[i]["const_type"], " ")!="")
                  ins_map.insert(std::pair<std::string, std::string>(field_value, "0"));
            }
         }

         all_is_well = true;
      }
      else
         ; // ????
   }
   else
      all_is_well = false;

   return all_is_well;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool CreateInsertMap(VIB::Database *dbDatabase, sqlcstr table, sqlmapstrs &ins_map)
{
   if(!dbDatabase)
      return false;

   if(dbDatabase->TestConnected())
   {
      VIB::Transaction dbTransaction;
      StdTransaction(&dbTransaction, dbDatabase);
      bool toReturn = true;

      try
      {
         toReturn = CreateInsertMap(&dbTransaction, table, ins_map);
      }
      catch(...)
      {
         toReturn = false;
         if(dbTransaction.Active)
            dbTransaction.Rollback();
      }

      if(dbTransaction.Active)
         dbTransaction.Commit();

      return toReturn;
   }
   else
      return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool GetSqlPlan(VIB::Transaction *dbTransaction, sqlcstr sql_statement, std::string &plan)
{
   bool result = true;
   plan = "";

   if(dbTransaction && dbTransaction->Active)
   {
      VIB::DataSet dbDataSet;
      try
      {
         dbDataSet.Database        = dbTransaction->DefaultDatabase;
         dbDataSet.Transaction     = dbTransaction->getVIBTransaction();
         dbDataSet.SelectSQL->Text = sql_statement;
         dbDataSet.Prepare();
         
         plan = dbDataSet.Plan();
      }
      catch(VIB::IBError &error)
      {
         last_sql_lib_error = error;
         result = false;
      }
      catch (...)
      {
         last_sql_lib_error.Reset();
         last_sql_lib_error.message = "A non-InterBase error of some sort occurred.";
         result = false;
      }
   }

   return result;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool GetSqlPlan(VIB::Database *dbDatabase, sqlcstr sql_statement, std::string &plan)
{
   bool all_is_well = true;
   
   if(!dbDatabase) 
      all_is_well = false;

   if(dbDatabase->TestConnected() && all_is_well)
   {
      VIB::Transaction dbTransaction;
      
      if(StdTransaction(&dbTransaction, dbDatabase))
      {
         bool toReturn = true;

         try
         {
            toReturn = GetSqlPlan(&dbTransaction, sql_statement, plan);
         }
         catch(...)
         {
            toReturn = false;
            //not really necessary with a plan, but we don't want to delete an Active transaction either.
            if(dbTransaction.Active)
               dbTransaction.Rollback();
         }
         
         //not really necessary with a plan, but we don't want to delete an Active transaction either.
         if(dbTransaction.Active)
            dbTransaction.Commit();

         return toReturn;
      }
   }
   else 
      all_is_well = false;

   //if we're here, we already know all_is_well is false.
   last_sql_lib_error.Reset();
   last_sql_lib_error.message = "A non-InterBase error of some sort occurred.";
   return false;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string SqlOptionalBetween(sqlcstr field, sqlcstr a, sqlcstr b)
{
   Pdate pa(a, global_mode);
   Pdate pb(b, global_mode);
   
   if(pa.IsValidDate() && pb.IsValidDate())
      return field + " between '" + pa.OutDate(global_mode) + "' and '" + pb.OutDate(global_mode) + "'";
   else if(pa.IsValidDate())
      return field + " >= '" + pa.OutDate(global_mode) + "'";
   else if(pb.IsValidDate())
      return field + " <= '" + pb.OutDate(global_mode) + "'";
   else
      return "1 = 1";
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string TranslateSqlPlan(VIB::Transaction *dbTransaction, sqlcstr plan)
{
   std::string answer;

   sqlmapstrs index_name_to_field_names;
   sqlvecstr  big_chunks;
   sqlsetstr  index_names;
   
   SplitStringToVec(plan, " INDEX (", big_chunks);
   
   if(!big_chunks.empty())
   {
      for(unsigned int i = 1; i < big_chunks.size(); i++)
      {
         SplitStringToSet(big_chunks[i].substr(0, big_chunks[i].find(")")), ",", index_names);
         
         for(sqlsetstr::iterator j = index_names.begin(); j != index_names.end(); j++)
         {
            sqlvecstr   fields_involved;
            std::string field_name = "'" + SafeSQLString(*j) + "'";

            if(index_name_to_field_names.find(field_name) == index_name_to_field_names.end())
            {
               std::string sql = 
                  "select RDB$INDEX_SEGMENTS.RDB$FIELD_NAME "
                  "from RDB$INDEX_SEGMENTS "
                  "where RDB$INDEX_NAME = " + field_name + " order by RDB$FIELD_POSITION";

               SqlToVec(dbTransaction, sql, fields_involved);

               for(sqlvecstr::iterator k = fields_involved.begin(); k != fields_involved.end(); k++)
                  *k = StripTrailing(*k, " ");
               
               index_name_to_field_names[*j] = VecToDelimString(fields_involved, "", "", " + ");
            }
         }
      }
   }
   
   answer = plan;
   for(sqlmapstrs::iterator k = index_name_to_field_names.begin(); k != index_name_to_field_names.end(); k++)
      answer = ReplaceStringWithString(answer, k->first, k->second);

   return answer;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
bool IsIndexed(VIB::Transaction *dbTransaction, sqlcstr table_name, sqlcstr field_name)
{
   std::string sql =
      "select '1' from RDB$INDEX_SEGMENTS "
      "inner join RDB$INDICES on RDB$INDICES.RDB$INDEX_NAME = RDB$INDEX_SEGMENTS.RDB$INDEX_NAME "
      "where RDB$INDEX_SEGMENTS.RDB$FIELD_POSITION = 0 and "
      " RDB$INDEX_SEGMENTS.RDB$FIELD_NAME = '" + SafeSQLString(field_name) + "' and "
      " RDB$INDICES.RDB$RELATION_NAME = '" + SafeSQLString(table_name) + "'";

   return (GetOneField(dbTransaction, sql) == "1");
}
//--------------------------------------------------------------------------
// defaults: SafeInClause( , , "0=1", false, 1000);
//
// Firebird has a limit to the number of items in an in () clause, after which 
// it craps out.  So we'll take a list of items, split it into max_values_in_groups 
// sized chunks (at most), and write the in clause as necessary, e.g. field in (...)  ; 
// or,  (field in (...) or field in (...) ...)  ; or,  result_if_blank
//
// the result_if_blank is aimed at compound statements.  e.g., say you have
// where x = y and z in (...)
// if there are no values to stick in the parens, rather than leave your sql with 
// a trailing and ... and breaking it, we simply return false.  where x = y and false.
// which is, actually, how you'd want that to evaluate in most compound clauses.
//
// note that if the /only/ statement you have in your where clause IS the in () 
// clause, "where false" won't fly.  passing "1=0" for result_if_blank will though.
// where 1=0 will return no results, which is what you want if there's nothing to 
// match against.
//
// TODO -c oversight : if you pass a vector with only empty string(s), you'll end 
// up with a in (), which is bad (and the very thing result_if_blank is designed to
// work around); this case needs to be specifically coded for.
//
// VIB port done 20121120
//
std::string SafeInClause(sqlcstr field, sqlcvecstr values, sqlcstr result_if_blank, 
                         bool quote_values, unsigned int max_values_in_group)
{
   if(values.empty())
      return result_if_blank;
   
   int beg_of_chunk = 0;
   int end_of_chunk = (std::min)(values.size(), max_values_in_group);
   sqlvecstr   chunks;
   std::string sql;
   
   while(end_of_chunk > beg_of_chunk)
   {
      std::string ids = "";
      
      if(quote_values)
      {
         for(int i = beg_of_chunk; i < end_of_chunk; i++)
            ids += ("'" + values[i] + "',");
      }
      else
      {
         for(int i = beg_of_chunk; i < end_of_chunk; i++)
            ids += (values[i] + ",");
      }
      
      ids = ids.substr(0, ids.length() - 1);

      chunks.push_back(field + " in (" + ids + ")");
      beg_of_chunk = end_of_chunk;
      end_of_chunk = (std::min)(beg_of_chunk + max_values_in_group, values.size());
   }
   
   std::string result = VecToDelimString(chunks, "", "", " or ");
   
   if(chunks.size() > 1)
      result = "(" + result + ")";
   
   return result;
}
//--------------------------------------------------------------------------
// VIB port done 20121120
std::string SafeInClause(sqlcstr field, sqlcvecstr values)
{
   return SafeInClause(field, values, "0=1", false, 1000);
}
//--------------------------------------------------------------------------
bool DataSetToCSV(VIB::DataSet *ds, sqlcstr filename, bool headers)
{
   std::fstream f;

   try
   {
      f.open(filename.c_str(), std::ios::out | std::ios::trunc);
   }
   catch(...)
   { 
      return false; 
   }
   
   if(!f.is_open()) 
      return false;

   try
   {
      std::string answer;
      ds->First();
            
      if(headers)
      {
         for(int i = 0; i < ds->FieldCount; i++)
         {
            if(i > 0)
               answer += ",";
            answer += "\"" + ReplaceStringWithString(ReplaceAnyCharWithString(ds->Fields->Fields[i]->FieldName, "\a\f\n\r\t", "?"), "\"", "\"\"") + "\"";
         }
         f << answer << std::endl;
      }
      while(!ds->Eof())
      {
         answer = "";
         for(int i = 0; i < ds->FieldCount; i++)
         {
            if (i > 0)
               answer += ",";
            
            std::string value;
            try
            {
               value = ds->Fields->Fields[i]->AsString;
            }
            catch(...)
            {
            }

            answer += "\"" + ReplaceStringWithString(ReplaceAnyCharWithString(value, "\a\f\n\r\t", "?"), "\"", "\"\"") + "\"";
         }
         f << answer << std::endl;
         ds->Next();
      }
   }
   catch(...) 
   { 
      return false; 
   }
   
   if(f.is_open()) /* and it should be */ 
      f.close();
   
   return true;
}
//--------------------------------------------------------------------------
// jhaley 20110224: I had to write this code for debugging back when I had issues with 
// the Appt Outcome Details Report with TLargeintField's lacking of support in 
// QuickReports, so I figured I'd keep it around. Handy for viewing the contents & 
// format of a dataset in a human-readable form.
std::string DataSetDebugView(VIB::DataSet *dbDataSet)
{
   std::string msg;
   int row_num = 1;

   while(!dbDataSet->Eof())
   {
      msg += std::string("Row ") + IntToString(row_num) + ":\n";
      for(int i = 0; i < dbDataSet->Fields->Count; i++)
      {
         std::string   field_name = dbDataSet->Fields->Fields[i]->FieldName;
         VIBFieldKind  fk         = dbDataSet->Fields->Fields[i]->FieldKind;
         VIBFieldType  dt         = dbDataSet->Fields->Fields[i]->DataType;
         std::string   dt_str;
         std::string   fk_str;
         std::string   value;

         switch(fk) // Field Kind
         {
         case vib_fkData:         fk_str = "Data";    break;
         case vib_fkCalculated:   fk_str = "Calc";    break;
         case vib_fkLookup:       fk_str = "Lookup";  break;
         case vib_fkInternalCalc: fk_str = "ICalc";   break;
         default:                 fk_str = "Unknown"; break;
         }

         switch(dt) // Field Type
         {
         case vib_ftInteger:    dt_str = "Integer";    break;
         case vib_ftSmallint:   dt_str = "Smallint";   break;
         case vib_ftLargeint:   dt_str = "Largeint";   break;
         case vib_ftString:     dt_str = "String";     break;
         case vib_ftWideString: dt_str = "WideString"; break;
         case vib_ftWord:       dt_str = "Word";       break;
         case vib_ftVariant:    dt_str = "Variant";    break;
         default:               dt_str = "Other (" + IntToString((int)dt) + ")"; break;
         }

         try // Value - this isn't guaranteed to succeed (for example, blobs may not support AsString)
         {
            value = dbDataSet->Fields->Fields[i]->AsString;
         }
         catch(...)
         {
            value = "(Exception in AsString)";
         }
         
         msg += std::string("  Field ") + field_name + ": " + fk_str + ", " + dt_str + ", " + value + "\n";
      }

      dbDataSet->Next();
      ++row_num;
   }

   dbDataSet->First(); // rewind the dataset

   return msg;
}

#endif // VIBC_NO_VISUALIB

// EOF


