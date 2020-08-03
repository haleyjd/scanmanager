/*
  Scan Manager
 
  Prometheus database connection and user authentication
*/

#include "m_argv.h"
#include "prometheusdb.h"
#include "promuser.h"
#include "util.h"

PrometheusUser::PrometheusUser() : db(), userID(0)
{
}

//
// Connect to the Prometheus Firebird database.
//
bool PrometheusUser::connect()
{
   try
   {
      db.reset(new PrometheusDB());
      return db->connect("10.1.1.109:/opt/interbase/db/prometheus.gdb", "<dbname>", "<dbpwd>");
   }
   catch(...)
   {
      return false;
   }
}

//
// Validate the Prometheus user credentials transmitted from Prometheus
//
bool PrometheusUser::checkCredentials()
{
   int idArg;
   int64_t hashCode, dbHashCode;

   if(!(idArg = M_GetArgParameter("-cred", 2)))
      return false;

   std::string id   = argv[idArg];   // user id
   std::string hash = argv[idArg+1]; // password hash
   std::string dbhash;

   if(hash.length() == 0)
      return false;

   // save integer user ID
   userID = atoi(id.c_str());

   if(!userID)
      return false;

   PrometheusDB *pDB;
   if(!(pDB = db.get()))
      return false;

   // get passed-in password hash
   hashCode = strtoll(hash.c_str(), nullptr, 10);

   // open a transaction to query the user credentials from the database
   PrometheusTransaction tr;
   if(!tr.stdTransaction(*pDB))
      return false;
   tr.getOneField("select HASH(pass) from users where id = " + IntToString(userID), dbhash);

   dbHashCode = strtoll(dbhash.c_str(), nullptr, 10);

   return (dbHashCode == hashCode);
}

// EOF

