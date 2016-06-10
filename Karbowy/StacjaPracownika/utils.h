#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <vector>
#include "timestamp.h"

QString formatTime(Duration duration);
QString formatTime(int durationInSeconds);
QString join(const std::vector<std::string>& lines);

#endif // UTILS_H
