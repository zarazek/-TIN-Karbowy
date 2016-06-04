#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <chrono>

typedef std::chrono::system_clock Clock;
typedef std::chrono::time_point<Clock> Timestamp;

#endif
