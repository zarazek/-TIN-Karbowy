#include "timestamp.h"

int toSeconds(Duration duration)
{
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}
