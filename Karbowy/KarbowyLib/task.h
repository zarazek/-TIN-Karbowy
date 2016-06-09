#ifndef TASK_H
#define TASK_H

#include <string>
#include <vector>

struct ClientTask
{
    int _id;
    std::string _title;
    std::vector<std::string> _description;
    int _secondsSpent;

    ClientTask() = default;
    ClientTask(int id, std::string&& title, const std::string& description, int secondsSpent);
};

#endif
