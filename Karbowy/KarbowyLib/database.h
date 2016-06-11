#ifndef DATABASE_H
#define DATABASE_H

#include "formatedexception.h"
#include "parse.h"
#include <sqlite3.h>
#include <string>
#include <exception>
#include <memory>
#include <type_traits>
#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <assert.h>

using std::string;
using std::exception;
using std::unique_ptr;
using std::ostream;

class DatabaseError : public FormatedException
{
public:
    DatabaseError(int errorCode, const char* errorMsg);

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
                           string, boost::optional<string>,
                           Timestamp, boost::optional<Timestamp>,
                           Duration, boost::optional<Duration> > ParamValue;
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
    Database(const Database&) = delete;
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
    QueryBase(Database &db, string&& queryStr);

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

    template <typename... RestOfArgs>
    void bind(int paramIdx, const Timestamp& param, RestOfArgs&&... restOfArgs)
    {
        std::string str = formatTimestamp(param);
        int errorCode = sqlite3_bind_text(_stmt, paramIdx, str.c_str(), str.length(), SQLITE_TRANSIENT);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const boost::optional<Timestamp>& param, RestOfArgs&&... restOfArgs)
    {
        int errorCode;
        if (param)
        {
            std::string str = formatTimestamp(*param);
            errorCode = sqlite3_bind_text(_stmt, paramIdx, str.c_str(), str.length(), SQLITE_TRANSIENT);
        }
        else
        {
            errorCode = sqlite3_bind_null(_stmt, paramIdx);
        }
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const Duration& param, RestOfArgs&&... restOfArgs)
    {
        int numOfSeconds = std::chrono::duration_cast<std::chrono::seconds>(param).count();
        int errorCode = sqlite3_bind_int(_stmt, paramIdx, numOfSeconds);
        checkBindError(errorCode, paramIdx, param);
        bind(paramIdx + 1, restOfArgs...);
    }

    template <typename... RestOfArgs>
    void bind(int paramIdx, const boost::optional<Duration>& param, RestOfArgs&&... restOfArgs)
    {
        int errorCode;
        if (param)
        {
            int numOfSeconds = std::chrono::duration_cast<std::chrono::seconds>(*param).count();
            errorCode = sqlite3_bind_int(_stmt, paramIdx, numOfSeconds);
        }
        else
        {
            errorCode = sqlite3_bind_null(_stmt, paramIdx);
        }
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
class Command : public QueryBase
{
public:
    Command(Database& db, const string& queryStr) :
        QueryBase(db, queryStr) { }

    Command(Database& db, string&& queryStr) :
        QueryBase(db, std::forward<string>(queryStr)) { }

    Command(const Command&) = delete;

    void execute(Args... args)
    {
        sqlite3_reset(_stmt);
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

template <typename F, typename T>
struct binder
{
    F f; T t;

    template <typename... Args>
    auto operator()(Args&&... args)
        -> decltype(f(std::move(t), std::forward<Args>(args)...))
    {
        return f(std::move(t), std::forward<Args>(args)...);
    }
};

template <typename F, typename T>
binder<typename std::decay<F>::type, typename std::decay<T>::type> bind_first(F&& f, T&& t)
{
    return { std::forward<F>(f), std::forward<T>(t) };
}

bool retrieveBoolColumn(sqlite3_stmt* stmt, int columnIdx);
boost::optional<bool> retrieveNullableBoolColumn(sqlite3_stmt* stmt, int columnIdx);
int retrieveIntColumn(sqlite3_stmt* stmt, int columnIdx);
boost::optional<int> retrieveNullableIntColumn(sqlite3_stmt* stmt, int columnIdx);
std::string retrieveStringColumn(sqlite3_stmt* stmt, int columnIdx);
boost::optional<std::string> retrieveNullableStringColumn(sqlite3_stmt* stmt, int columnIdx);

template <typename Signature>
struct CallFunctor;

template <typename Result>
struct CallFunctor<Result()>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt*, int)
    {
        return fn();
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(bool, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        bool value = retrieveBoolColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), std::move(value)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(boost::optional<bool>, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        boost::optional<bool> value = retrieveNullableBoolColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), std::move(value)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(int, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        int value = retrieveIntColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), std::move(value)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(boost::optional<int>, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        boost::optional<int> value = retrieveNullableIntColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), std::move(value)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(std::string, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        std::string value(retrieveStringColumn(stmt, columnIdx));
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), std::move(value)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(boost::optional<std::string>, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        boost::optional<std::string> value = retrieveNullableStringColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(fn, std::move(value)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(Timestamp, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        std::string value = retrieveStringColumn(stmt, columnIdx);
        Timestamp timestamp;
        bool parseOk = parse(value, TimestampToken(timestamp));
        assert(parseOk);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), std::move(timestamp)),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(boost::optional<Timestamp>, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        boost::optional<std::string> value = retrieveNullableStringColumn(stmt, columnIdx);
        if (value)
        {
            Timestamp timestamp;
            bool parseOk = parse(*value, TimestampToken(timestamp));
            assert(parseOk);
            return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), boost::optional<Timestamp>(timestamp)),
                                                            stmt,
                                                            columnIdx + 1);
        }
        else
        {
            return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), boost::none),
                                                            stmt,
                                                            columnIdx + 1);
        }
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(Duration, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        int value = retrieveIntColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn), Duration(std::chrono::seconds(value))),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename Result, typename... RestOfArgs>
struct CallFunctor<Result(boost::optional<Duration>, RestOfArgs...)>
{
    template <typename Functor>
    static Result call(Functor&& fn, sqlite3_stmt* stmt, int columnIdx)
    {
        boost::optional<int> value = retrieveNullableIntColumn(stmt, columnIdx);
        return CallFunctor<Result(RestOfArgs...)>::call(bind_first(std::move(fn),
                                                                   value ? boost::optional<Duration>(std::chrono::seconds(*value)) :
                                                                           boost::none),
                                                        stmt,
                                                        columnIdx + 1);
    }
};

template <typename PointerToMethod>
struct ExtractSignature;

template <typename Result, typename Class, typename... Args>
struct ExtractSignature<Result (Class::*)(Args...)>
{
    typedef Result (type)(Args...);
};

template <typename Result, typename Class, typename... Args>
struct ExtractSignature<Result (Class::*)(Args...) const>
{
    typedef Result (type)(Args...);
};

template <typename Signature>
struct ExtractResult;

template <typename R, typename... Args>
struct ExtractResult<R(Args...)>
{
    typedef R type;
};

template <typename Functor>
struct FunctorTraits
{
    typedef typename std::decay<Functor>::type DecayedFunctor;
    typedef typename ExtractSignature<decltype(&DecayedFunctor::operator())>::type Signature;
    typedef typename ExtractResult<Signature>::type Result;
};

template <typename R, typename... Args>
struct FunctorTraits<R(Args...)>
{
    typedef R (Signature)(Args...);
    typedef R Result;
};

template <typename R, typename... Args>
struct FunctorTraits<R(&)(Args...)>
{
    typedef R (Signature)(Args...);
    typedef R Result;
};

template <typename R, typename... Args>
struct FunctorTraits<R(*)(Args...)>
{
    typedef R (Signature)(Args...);
    typedef R Result;
};

template <typename Signature>
struct DecayedSignature;

template <typename R>
struct DecayedSignature<R()>
{
    typedef R (type)();
};

template <typename First, typename Signature>
struct AddFirstArgument;

template <typename First, typename R, typename... Args>
struct AddFirstArgument<First, R(Args...)>
{
    typedef R (type)(First, Args...);
};

template <typename R, typename T, typename... Ts>
struct DecayedSignature<R(T, Ts...)>
{
    typedef typename AddFirstArgument<typename std::decay<T>::type,
                                      typename DecayedSignature<R(Ts...)>::type>::type type;
};

template <typename Functor>
class RowRetriever : public RowRetrieverBase<typename FunctorTraits<Functor>::Result>
{
private:
    typedef typename FunctorTraits<Functor>::Signature Signature;
    typedef typename DecayedSignature<Signature>::type CallSignature;
    typedef typename FunctorTraits<Functor>::Result Result;

    typename std::decay<Functor>::type _fn;
public:
    RowRetriever(Functor&& fn) :
        _fn(fn) { }

    Result retrieveRow(sqlite3_stmt* stmt) override
    {
        return CallFunctor<CallSignature>::call(_fn, stmt, 0);
    }
};

template <typename Result, typename... Args>
class Query : public QueryBase
{
public:
    Query(Database& db, const string& queryStr) :
        QueryBase(db, queryStr),
        _retriever(new RowRetriever<Result(Result&&)>(returnArg)) { }

    Query(Database& db, string&& queryStr) :
        QueryBase(db, std::forward<string>(queryStr)),
        _retriever(new RowRetriever<Result(Result&&)>(returnArg)) { }

    Query(const Query&) = delete;

    template <class Functor>
    Query(Database& db, const string& queryStr, Functor&& fn) :
        QueryBase(db, queryStr),
        _retriever(new RowRetriever<Functor>(std::forward<Functor>(fn))) { }

    template <class Functor>
    Query(Database& db, string&& queryStr, Functor&& fn) :
        QueryBase(db, std::forward<string>(queryStr)),
        _retriever(new RowRetriever<Functor>(std::forward<Functor>(fn))) { }

    void execute(Args... args)
    {
        sqlite3_reset(_stmt);
        bindParams(args...);
    }

    bool next(Result& result)
    {
        if (executeStep())
        {
            result = _retriever->retrieveRow(_stmt);
            return true;
        }
        else
        {
            return false;
        }
    }
private:
    unique_ptr<RowRetrieverBase<Result> > _retriever;

    static Result returnArg(Result&& arg)
    {
        return arg;
    }
};


#endif // DATABASE_H
