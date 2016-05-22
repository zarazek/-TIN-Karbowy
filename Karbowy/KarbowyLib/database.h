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
    void bindParams(Args... args)
    {
        bind(1, args...);
    }

    bool executeStep();

private:
    static void bind(int)
    {
        // TODO
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, bool param, RestOfArgs... restOfArgs)
    {
        int errorCode = sqlite3_bind_int(_stmt, paramIdx, param ? 1 : 0);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const boost::optional<bool>& param, RestOfArgs... restOfArgs)
    {
        int errorCode = param ?
                        sqlite3_bind_int(_stmt, paramIdx, *param ? 1 : 0) :
                        sqlite3_bind_null(_stmt, paramIdx);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, int param, RestOfArgs... restOfArgs)
    {
        int errorCode = sqlite3_bind_int(_stmt, paramIdx, param);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const boost::optional<int>& param, RestOfArgs... restOfArgs)
    {
        int errorCode = param ?
                        sqlite3_bind_int(_stmt, paramIdx, *param) :
                        sqlite3_bind_null(_stmt, paramIdx);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const string& param, RestOfArgs... restOfArgs)
    {
        int errorCode = sqlite3_bind_text(_stmt, paramIdx, param.c_str(), param.length(), SQLITE_TRANSIENT);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const boost::optional<string>& param, RestOfArgs&&... restOfArgs)
    {
        int errorCode = param ?
                        sqlite3_bind_text(_stmt, paramIdx, param->c_str(), param->length(), SQLITE_TRANSIENT) :
                        sqlite3_bind_null(_stmt, paramIdx);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
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

template <typename... Args>
class Command : QueryBase
{
public:
    Command(Database& db, const string& queryStr) :
        QueryBase(db, queryStr) { }

    void execute(Args... args)
    {
        bindParams(args...);
        bool res = executeStep();
        assert(! res);
    }
};

template <typename Result>
class RowRetrieverBase
{
public:
    virtual ~RowRetrieverBase() { }
    virtual Result retrieveRow(sqlite3_stmt* stmt) = 0;
};

template <typename Functor, typename Bound>
struct bound_first
{
    Functor _functor;
    Bound _bound;

    template <typename... RestOfArgs>
    typename std::result_of<Functor&(Bound, RestOfArgs...)>::type operator()(RestOfArgs&&... args)
    {
        return _functor(std::move(_bound), std::forward<RestOfArgs>(args)...);
    }
};

template <typename Functor, typename Bound>
bound_first<typename std::decay<Functor>::type, typename std::decay<Bound>::type>
bind_first(Functor&& functor, Bound&& bound)
{
    return  { std::forward<Functor>(functor), std::forward<Bound>(bound) };
}

template <typename Result, typename... Args>
class RowRetriever : public RowRetrieverBase<Result>
{
public:
    RowRetriever(std::function<Result(Args...)> fn) :
        _fn(fn) { }

    Result retrieveRow(sqlite3_stmt* stmt) override
    {
        _stmt = stmt;
        return callFunction(0, _fn);
    }

private:
    std::function<Result(Args...)> _fn;
    sqlite3_stmt* _stmt;

    Result callFunction(int, const std::function<Result()>& fn)
    {
        return fn();
    }

    template <typename... RestOfArgs>
    Result callFunction(int columnIdx, const std::function<Result(bool, RestOfArgs...)>& fn)
    {
        bool value = retrieveBoolColumn(columnIdx);
        return callFunction(columnIdx + 1,
                            std::function<Result(RestOfArgs...)>(bind_first(fn, value)));
    }

    template <typename... RestOfArgs>
    Result callFuncton(int columnIdx, const std::function<Result(boost::optional<bool>, RestOfArgs...)>& fn)
    {
        boost::optional<bool> value;
        int type = sqlite3_column_type(_stmt, columnIdx);
        switch (type)
        {
        case SQLITE_NULL:
            value = boost::none;
            break;
        case SQLITE_TEXT:
            *value = retrieveBoolColumn(columnIdx);
             break;
        default:
            //TODO: throw proper exception
            throw std::runtime_error("not bool nor null");
            break;
        }
        return callFunction(columnIdx + 1,
                            std::function<Result(RestOfArgs...)>(bind_first(fn, value)));
    }

    bool retrieveBoolColumn(int columnIdx)
    {
        int type = sqlite3_column_type(_stmt, columnIdx);
        switch (type)
        {
        case SQLITE_TEXT:
            return textToBool(reinterpret_cast<const char*>(sqlite3_column_text(_stmt, columnIdx)));
        case SQLITE_INTEGER:
            return sqlite3_column_int(_stmt, columnIdx);
        default:
            //TODO: throw proper exception
            throw std::runtime_error("not bool");
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

    template <typename... RestOfArgs>
    Result callFunction(int columnIdx, const std::function<Result(string, RestOfArgs...)>& fn)
    {
       if (sqlite3_column_type(_stmt, columnIdx) != SQLITE_TEXT)
       {
           // TODO: throw exception
       }
       string value(reinterpret_cast<const char*>(sqlite3_column_text(_stmt, columnIdx)));
       return callFunction(columnIdx + 1,
                           std::function<Result(RestOfArgs...)>(bind_first(fn, value)));
    }
};

template <typename Result, typename... Args>
class Query : QueryBase
{
public:
    Query(Database& db, const string& queryStr, RowRetrieverBase<Result>& retriever) :
        QueryBase(db, queryStr),
        _retriever(retriever) { }

    void execute(Args... args)
    {
        bindParams(args...);
    }

    bool next(Result& result)
    {
        if (executeStep())
        {
            result = _retriever.retrieveRow(_stmt);
            return true;
        }
        else
        {
            return false;
        }
    }
private:
    RowRetrieverBase<Result>& _retriever;
};


#endif // DATABASE_H
