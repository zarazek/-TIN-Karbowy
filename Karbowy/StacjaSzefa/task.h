#ifndef TASK
#define TASK
#include <string>

class Task
{
public:
    int _id;
    string _name;
    string _details;
    enum _status {ACTIVE, DONE, DELETED};

    Task (Database *db, int id);
};


#endif // TASK

