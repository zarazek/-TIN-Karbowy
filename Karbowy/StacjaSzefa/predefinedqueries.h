#ifndef PREDEFINEDQUERIES_H
#define PREDEFINEDQUERIES_H

#include <database.h>

class Employee;

void initializeDatabase();
void shutdownDatabase();

Query<std::string>& retrieveUuidQ();
Command<std::string>& insertUuidC();

Query<std::unique_ptr<Employee>, std::string>& findEmployeeByLoginQ();

#endif // PREDEFINEDQUERIES_H
