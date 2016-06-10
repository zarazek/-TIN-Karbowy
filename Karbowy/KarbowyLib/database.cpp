#include "database.h"

DatabaseError::DatabaseError(int errorCode, const char* errorMsg) :
    _errorCode(errorCode),
    _errorMsg(errorMsg) { }

OpenError::OpenError(int errorCode, const char* errorMsg, const string& fileName) :
    DatabaseError(errorCode, errorMsg),
    _fileName(fileName) { }

void OpenError::formatWhatMsg(ostream& stream) const
{
    stream << "Error opening database '" << _fileName << "': " << _errorMsg
           << " (error code " << _errorCode << ')';
}

QueryError::QueryError(int errorCode, const char *errorMsg, const string& queryStr) :
    DatabaseError(errorCode, errorMsg),
    _queryStr(queryStr) { }

PrepareError::PrepareError(int errorCode, const char* errorMsg, const string& queryStr, int errorPosition) :
    QueryError(errorCode, errorMsg, queryStr),
    _errorPosition(errorPosition) { }

void PrepareError::formatWhatMsg(ostream& stream) const
{
    stream << "Error compiling query:\n" + _queryStr << '\n';
    if (_errorPosition >= 0)
    {
        stream << "error at " << _errorPosition << ": ";
    }
    stream << _errorMsg << " (error code " << _errorCode << ")";
}

class PrintVisitor : public boost::static_visitor<>
{
public:
    PrintVisitor(ostream& stream) :
        _stream(stream) { }

    void operator()(bool value) const
    {
        _stream << (value ? "TRUE" : "FALSE");
    }

    void operator()(int value) const
    {
        _stream << value;
    }

    void operator()(const string& value) const
    {
        _stream << '\'';
        for (char c: value)
        {
            if (c == '\'')
            {
                _stream << "''";
            }
            else
            {
                _stream << c;
            }
        }
        _stream << '\'';
    }

    void operator()(const Timestamp& value) const
    {
        _stream << '\'' << formatTimestamp(value) << '\'';
    }

    void operator()(const Duration& value) const
    {
        _stream << std::chrono::duration_cast<std::chrono::seconds>(value).count();
    }

    template <class T>
    void operator()(const boost::optional<T>& value) const
    {
        if (value)
        {
            operator()(*value);
        }
        else
        {
            _stream << "NULL";
        }
    }

private:
    ostream& _stream;
};

void BindError::formatWhatMsg(std::ostream &stream) const
{
    stream << "Error while binding parameter " << _paramIdx << " to value ";
    boost::apply_visitor(PrintVisitor(stream), _paramValue);
    stream << " in query:\n" << _queryStr << '\n'
           << _errorMsg << " (error code " << _errorCode << ')';

}

ExecuteError::ExecuteError(int errorCode, const char *errorMsg, const string &queryStr) :
    QueryError(errorCode, errorMsg, queryStr) { }


void ExecuteError::formatWhatMsg(std::ostream &stream) const
{
    stream << "Error while executing query\n" << _queryStr << '\n'
           << _errorMsg << " (error code" << _errorCode << ')';
}

Database::Database(const string &filename)
    : _db(nullptr)
{
    int errorCode = sqlite3_open(filename.c_str(), &_db);
    if (errorCode != SQLITE_OK)
    {
        OpenError err(errorCode, getErrorMsg(_db, errorCode), filename);
        if (_db)
        {
            sqlite3_close(_db);
        }
        throw err;
    }
}

Database::~Database()
{
    sqlite3_close(_db);
}

const char* Database::getErrorMsg(int errorCode)
{
    return getErrorMsg(_db, errorCode);
}

const char* Database::getErrorMsg(sqlite3* db, int errorCode)
{
    return db ? sqlite3_errmsg(db) : sqlite3_errstr(errorCode);
}

sqlite3_stmt* Database::prepareQuery(const string &queryStr)
{
    const char* cQueryStr = queryStr.c_str();
    sqlite3_stmt *stmt = nullptr;
    const char *errorPlace = nullptr;
    int errorCode = sqlite3_prepare_v2(_db, cQueryStr, -1, &stmt, &errorPlace);
    if (errorCode != SQLITE_OK)
    {
        int errorIdx = errorPlace ? errorPlace - cQueryStr : -1;
        if (stmt)
        {
            sqlite3_finalize(stmt);
        }
        throw PrepareError(errorCode, getErrorMsg(errorCode), queryStr, errorIdx);
    }
    return stmt;
}

QueryBase::QueryBase(Database &db, const string& queryStr) :
    _db(db),
    _queryStr(queryStr),
    _stmt(db.prepareQuery(_queryStr)) { }

QueryBase::QueryBase(Database &db, string&& queryStr) :
    _db(db),
    _queryStr(std::forward<std::string>(queryStr)),
    _stmt(db.prepareQuery(_queryStr)) { }


QueryBase::~QueryBase()
{
    sqlite3_finalize(_stmt);
}

bool QueryBase::executeStep()
{
    int result = sqlite3_step(_stmt);
    switch (result)
    {
    case SQLITE_DONE:
        sqlite3_reset(_stmt);
        return false;
    case SQLITE_ROW:
        return true;
    default:
        {
            ExecuteError err(result, _db.getErrorMsg(result), _queryStr);
            sqlite3_reset(_stmt);
            throw err;
        }
    }
}

static bool textToBool(const char* strValue)
{
    if (strcmp(strValue, "TRUE") == 0)
    {
        return true;
    }
    else if (strcmp(strValue, "FALSE") == 0)
    {
        return false;
    }
    else
    {
        //TODO: throw proper exception
        throw std::runtime_error(strValue);
    }
}

bool retrieveBoolColumn(sqlite3_stmt* stmt, int columnIdx)
{
    int type = sqlite3_column_type(stmt, columnIdx);
    switch (type)
    {
    case SQLITE_TEXT:
        return textToBool(reinterpret_cast<const char*>(sqlite3_column_text(stmt, columnIdx)));
    case SQLITE_INTEGER:
        return sqlite3_column_int(stmt, columnIdx);
    default:
        //TODO: throw proper exception
        throw std::runtime_error("not bool");
    }
}

boost::optional<bool> retrieveNullableBoolColumn(sqlite3_stmt* stmt, int columnIdx)
{
    int type = sqlite3_column_type(stmt, columnIdx);
    switch (type)
    {
    case SQLITE_NULL:
        return boost::none;
    case SQLITE_TEXT:
    case SQLITE_INTEGER:
        return boost::optional<bool>(retrieveBoolColumn(stmt, columnIdx));
    default:
        //TODO: throw proper exception
        throw std::runtime_error("not bool nor null");
    }
}

int retrieveIntColumn(sqlite3_stmt* stmt, int columnIdx)
{
    if (sqlite3_column_type(stmt, columnIdx) == SQLITE_INTEGER)
    {
        return sqlite3_column_int(stmt, columnIdx);
    }
    else
    {
        throw std::runtime_error("not int");
    }
}

boost::optional<int> retrieveNullableIntColumn(sqlite3_stmt* stmt, int columnIdx)
{
    int type = sqlite3_column_type(stmt, columnIdx);
    switch (type)
    {
    case SQLITE_NULL:
        return boost::none;
    case SQLITE_INTEGER:
        return boost::optional<int>(sqlite3_column_int(stmt, columnIdx));
    default:
        throw std::runtime_error("not int nor null");
    }
}

std::string retrieveStringColumn(sqlite3_stmt* stmt, int columnIdx)
{
    if (sqlite3_column_type(stmt, columnIdx) != SQLITE_TEXT)
    {
        throw std::runtime_error("not string");
    }
    return reinterpret_cast<const char*>(sqlite3_column_text(stmt, columnIdx));
}

boost::optional<std::string> retrieveNullableStringColumn(sqlite3_stmt* stmt, int columnIdx)
{
    int type = sqlite3_column_type(stmt, columnIdx);
    switch (type)
    {
    case SQLITE_NULL:
        return boost::none;
    case SQLITE_TEXT:
        return boost::optional<std::string>(retrieveStringColumn(stmt, columnIdx));
    default:
        // TODO: throw proper exception
        throw std::runtime_error("not string nor null");
    }
}
