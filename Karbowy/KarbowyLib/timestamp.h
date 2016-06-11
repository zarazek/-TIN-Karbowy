#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#include <chrono>
#include <string>

typedef std::chrono::system_clock Clock;
typedef Clock::time_point Timestamp;
typedef Clock::duration Duration;

int toSeconds(Duration duration);
std::string formatTimestamp(const Timestamp& timestamp);

#endif
