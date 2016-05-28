#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include <string>

class Employee
{
public:
    std::string _login;
    std::string _password;
    std::string _name;
    bool _active;

    Employee() = default;

    Employee(const std::string& login, const std::string& password, const std::string& name, bool active) :
        _login(login),
        _password(password),
        _name(name),
        _active(active) { }
};

#endif // EMPLOYEE_H


