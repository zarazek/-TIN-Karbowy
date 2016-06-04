#ifndef PREDEFINEDQUERIES_H
#define PREDEFINEDQUERIES_H

#include <database.h>
#include "timestamp.h"

class Employee;
class Task;

void initializeDatabase();
void shutdownDatabase();

Query<std::string>& retrieveUuidQ();
Command<std::string>& insertUuidC();

Query<std::unique_ptr<Employee>, std::string>& findEmployeeByLoginQ();
Command<std::string>& insertClientUuidC();
Query<int, std::string>& findClientIdByUuidQ();
Query<std::unique_ptr<Task>, std::string>& findTasksForLoginQ();
Query<boost::optional<Timestamp>, int>& findLastEntryTimeQ();
Command<int, int, std::string, Timestamp, boost::optional<int> >& insertLogEntryC();

#endif // PREDEFINEDQUERIES_H
