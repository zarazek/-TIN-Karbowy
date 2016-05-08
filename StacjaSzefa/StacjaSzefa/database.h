#ifndef DATABASE_H
#define DATABASE_H
#include <sqlite3.h>
#include <string>

using std::string;
class Query;
class Database
{

public:

    static Database* openDatabase(const string &filename);
    Query* createQuery(const string &stmt);
    ~Database();

private:
    sqlite3* _db;
    Database(sqlite3* db):_db(db){}



};


class Query
{
public:
    ~Query();
private:
    Database &_db;
    sqlite3_stmt *_stmt;
    Query(Database &db, sqlite3_stmt *stmt);
    friend class Database;

};



#endif // DATABASE_H
