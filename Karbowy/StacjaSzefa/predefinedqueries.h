#ifndef PREDEFINEDQUERIES_H
#define PREDEFINEDQUERIES_H

#include <database.h>
#include "timestamp.h"

class Employee;
class ClientTask;
class ServerLogEntry;

enum TaskState
{
    TASK_STATE_ACTIVE = 0,
    TASK_STATE_FINISHED = 1,
    TASK_STATE_CANCELED = 2
};

struct AssignmentStatus
{
    Duration _timeSpent;
    bool _finished;
};

struct TaskStatus
{
    int _id;
    boost::optional<AssignmentStatus> _assignment;
};

void initializeDatabase();
void shutdownDatabase();

Query<std::string>& retrieveUuidQ();
Command<std::string>& insertUuidC();

Query<std::unique_ptr<Employee>, std::string>& findEmployeeByLoginQ();
Command<std::string>& insertClientUuidC();
Query<int, std::string>& findClientIdByUuidQ();
Query<std::unique_ptr<ClientTask>, std::string>& findTasksForLoginQ();

Query<boost::optional<Timestamp>, int>& findLastLogEntryTimeForClientQ();
Command<int, int, std::string, Timestamp, boost::optional<int> >& insertLogEntryC();
Query<ServerLogEntry, std::string>& findUnprocessedLogEntriesForEmployeeQ();
Query<TaskStatus, std::string, int>& findTaskStatusQ();
Command<int>& setLogEntryToProcessedC();
Command<bool, Duration, std::string, int>& updateEmployeeTaskStatusC();


Query<std::string>& findAllActiveEmployeesQ();
struct TaskAssignedEmployee
{
    std::string _employeeId;
    bool _assignmentActive;
};
Query<TaskAssignedEmployee, int>& findAllEmployeesAssignedToTaskQ();
Command<bool, std::string, int>& changeEmployeeTaskAssignmentStatusC();
Command<std::string, int>& addEmployeeToTaskC();

#endif // PREDEFINEDQUERIES_H
