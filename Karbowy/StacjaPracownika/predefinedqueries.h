#ifndef PREDEFINEDQUERIES_H
#define PREDEFINEDQUERIES_H

#include <database.h>

void initializeDatabase();
void shutdownDatabase();

Query<std::string>& retrieveUuidQ();
Command<std::string>& insertUuidC();

Query<int, std::string>& findUserIdByLoginQ();
Command<std::string>& insertUserC();

Command<int>& deleteTaskAssociationsForUserC();
Command<int, std::string, std::string>& insertTaskC();
Command<int, int, int>& insertTaskAssociationC();

class ClientTask;
Query<std::unique_ptr<ClientTask>, int>& findActiveTasksForEmployeeQ();

Command<int, int, Timestamp, boost::optional<int> >& insertLogEntryC();

#endif
