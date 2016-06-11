#ifndef STRINGANDTIMEUTILS_H
#define STRINGANDTIMEUTILS_H

#include <QString>
#include <vector>
#include "timestamp.h"

QString formatTime(Duration duration);
QString join(const std::vector<std::string>& lines);

#endif // STRINGANDTIMEUTILS_H
