#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <chrono>

typedef std::chrono::system_clock Clock;
typedef Clock::time_point Timestamp;
typedef Clock::duration Duration;

int toSeconds(Duration duration);

#endif
