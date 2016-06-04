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

#endif
