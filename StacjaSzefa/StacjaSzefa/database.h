#ifndef DATABASE_H
#define DATABASE_H
#include <sqlite3.h>



class DataBase
{

public:


private:
    sqlite3* _db;
    DataBase(sqlite3* db):_db(db){}



};

#endif // DATABASE_H
