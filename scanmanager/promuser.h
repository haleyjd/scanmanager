/*
  Scan Manager
 
  Prometheus database connection and user authentication
*/

#ifndef PROMUSER_H__
#define PROMUSER_H__

#include <memory>

class PrometheusDB;

class PrometheusUser
{
protected:
   std::unique_ptr<PrometheusDB> db;
   int userID;

public:
   PrometheusUser();

   bool connect();
   bool checkCredentials();

   PrometheusDB &getDatabase() { return *db.get(); }
   int getUserID() const { return userID; }
};

#endif

// EOF

