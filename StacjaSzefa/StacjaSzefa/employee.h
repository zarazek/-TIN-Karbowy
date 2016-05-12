#ifndef EMPLOYEE_H
#define EMPLOYEE_H

class Employee
{
public:
    string _login;
    string _password;
    string _name;
    bool _active;

    Employee(const string& login, const string& password, const string& name, bool active) :
        _login(login),
        _password(password),
        _name(name),
        _active(active) { }
};

#endif // EMPLOYEE_H


