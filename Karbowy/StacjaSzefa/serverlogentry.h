#ifndef SERVERLOGENTRY_H
#define SERVERLOGENTRY_H

#include "logentry.h"

struct ServerLogEntry
{
    int _id;
    int _clientId;
    LogEntry _entry;
};

#endif // SERVERLOGENTRY_H
