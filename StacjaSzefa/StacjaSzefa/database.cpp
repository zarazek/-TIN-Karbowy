#include "database.h"

Database* Database::openDatabase(const string &filename)
{
    sqlite3 *db = nullptr;
    int error = sqlite3_open(filename.c_str(), &db);
    if (error == SQLITE_OK)
    {
        return new Database(db);
    }
    else
    {
        return nullptr;
    }
}


Database::~Database()
{
    sqlite3_close(_db);
}

Query* Database::createQuery(const string &queryStr)
{
    sqlite3_stmt *stmt;
    const char *errorPlace;
    int error = sqlite3_prepare(_db, queryStr.c_str(), -1, &stmt, &errorPlace);
    if (error == SQLITE_OK)
    {
        return new Query(*this, stmt);
    }
    else
    {
        return nullptr;
    }
}

Query::Query(Database &db, sqlite3_stmt *stmt):_db(db), _stmt(stmt)
{

}

Query::~Query()
{
    sqlite3_finalize(_stmt);
}
