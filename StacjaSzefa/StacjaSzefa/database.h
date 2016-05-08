#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>
#include <string>
#include <exception>
#include <memory>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <assert.h>

using std::string;
using std::exception;
using std::unique_ptr;
using std::ostream;

class DatabaseError : public exception
{
public:
    DatabaseError(int errorCode, const char* errorMsg);
    const char* what() const noexcept override;

protected:
    int _errorCode;
    string _errorMsg;
private:
    mutable unique_ptr<string> _whatBuffer;

    virtual void formatWhatMsg(ostream &stream) const = 0;
};

class OpenError : public DatabaseError
{
public:
    OpenError(int errorCode, const char* errorMsg, const string& fileName);

private:
    string _fileName;

    void formatWhatMsg(ostream& stream) const override;
};

class QueryError : public DatabaseError
{
public:
    QueryError(int errorCode, const char* errorMsg, const string& queryStr);
protected:
    string _queryStr;
};

class PrepareError : public QueryError
{
public:
    PrepareError(int errorCode, const char* errorMsg, const string& queryStr, int errorPosition);

private:
    int _errorPosition;

    void formatWhatMsg(ostream& stream) const override;
};

class BindError : public QueryError
{
public:
    template <class T>
    BindError(int errorCode, const char *errorMsg, const string& queryStr, int paramIdx, T paramValue) :
        QueryError(errorCode, errorMsg, queryStr),
        _paramIdx(paramIdx),
        _paramValue(paramValue) { }

private:
    typedef boost::variant<bool, boost::optional<bool>,
                           int, boost::optional<int>,
                           string, boost::optional<string> > ParamValue;
    int _paramIdx;
    ParamValue _paramValue;

    void formatWhatMsg(ostream& stream) const;
};

class ExecuteError : public QueryError
{
public:
    ExecuteError(int errorCode, const char* errorMsg, const string& queryStr);
private:
    void formatWhatMsg(ostream& stream) const override;
};


class Database
{
public:

    Database(const string &fileName);
    ~Database();
private:
    sqlite3* _db;

    const char* getErrorMsg(int errorCode);
    static const char* getErrorMsg(sqlite3* db, int erorCode);
    sqlite3_stmt* prepareQuery(const string &queryStr);

    friend class QueryBase;
};

class QueryBase
{
public:
    virtual ~QueryBase();
protected:
    Database& _db;
    string _queryStr;
    sqlite3_stmt *_stmt;

    QueryBase(Database &db, const string& queryStr);

    template <typename... Args>
    static void bindParams(sqlite3_stmt *stmt, Args&&... args)
    {
        bind(0, std::forward<Args>(args)...);
    }

    bool executeStep();

private:
    static void bind(int paramIdx)
    {
        // TODO
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, bool param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = sqlite3_bind_int(_stmt, paramIdx, param ? 1 : 0);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, std::forward<RestOfArgs>(restOfArgs)...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, boost::optional<bool> param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = param ?
                        sqlite3_bind_int(_stmt, paramIdx, *param ? 1 : 0) :
                        sqlite3_bind_null(_stmt, paramIdx);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, std::forward<RestOfArgs>(restOfArgs)...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, int param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = sqlite3_bind_int(_stmt, paramIdx, param);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, std::forward<RestOfArgs>(restOfArgs)...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, boost::optional<int> param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = param ?
                        sqlite3_bind_int(_stmt, paramIdx, *param) :
                        sqlite3_bind_null(_stmt, paramIdx);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, std::forward<RestOfArgs>(restOfArgs)...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, string param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = sqlite3_bind_text(_stmt, paramIdx, param.c_str(), param.length(), SQLITE_TRANSIENT);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, std::forward<RestOfArgs>(restOfArgs)...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, boost::optional<string> param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = param ?
                        sqlite3_bind_text(_stmt, paramIdx, param->c_str(), param->length(), SQLITE_TRANSIENT) :
                        sqlite3_bind_null(_stmt, paramIdx);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, std::forward<RestOfArgs>(restOfArgs)...);
    }

    template <typename T>
    void checkBindError(int errorCode, int paramIdx, T param)
    {
        if (errorCode != SQLITE_OK)
        {
            BindError err(errorCode, _db.getErrorMsg(errorCode), _queryStr, paramIdx, param);
            sqlite3_reset(_stmt);
            throw err;
        }
    }
};

class SimpleCommand : QueryBase
{
public:
    SimpleCommand(Database& db, const string& querStr);
    void execute();
};

template <typename... Args>
class ParametrizedCommand : QueryBase
{
public:
    ParametrizedCommand(Database& db, const string& queryStr);

    void execute(Args&&... args)
    {
        bindParams(std::forward<Args>(args)...);
        bool res = executeStep();
        assert(! res);
    }
};


#endif // DATABASE_H
