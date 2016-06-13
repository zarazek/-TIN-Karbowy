#ifndef LOGPROCESSOR_H
#define LOGPROCESSOR_H

#include <string>
#include <vector>
#include <map>
#include <boost/optional.hpp>
#include "serverlogentry.h"

class AssignmentStatus;

class LogProcessor
{
public:
    LogProcessor(const std::string& employeeId);
    void checkEmployeeId();
    void process(ServerLogEntry&& entry);
    void finish();
private:
    const std::string& _employeeId;
    bool _employeeIsValid;
    boost::optional<Timestamp> _previousTimestamp;
    boost::optional<ServerLogEntry> _loginEntry;
    std::map<int, AssignmentStatus> _assignments;
    std::map<int, ServerLogEntry> _workStartEntrys;
    std::vector<int> _processed;

    bool preliminaryValidate(const ServerLogEntry& entry);
    bool checkTaskId(const ServerLogEntry& entry);
    const AssignmentStatus* getAssignment(const ServerLogEntry& entry);
    static std::ostream& invalidEntryMsg(const ServerLogEntry& entry);
};


#endif // LOGPROCESSOR_H
