#ifndef PREDEFINEDQUERIES_H
#define PREDEFINEDQUERIES_H

#include <database.h>

class Employee;
class Task;

void initializeDatabase();
void shutdownDatabase();

Query<std::string>& retrieveUuidQ();
Command<std::string>& insertUuidC();

Query<std::unique_ptr<Employee>, std::string>& findEmployeeByLoginQ();
Query<std::unique_ptr<Task>, std::string>& findTasksForLoginQ();

#endif // PREDEFINEDQUERIES_H
