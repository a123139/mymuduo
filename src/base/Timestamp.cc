#include "muduo/base/Timestamp.h"

#include <time.h>
#include <sys/time.h>

namespace muduo {

Timestamp::Timestamp():microSecondsSinceEpoch_(0) {}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch)
    {}

Timestamp Timestamp::now()
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return Timestamp(static_cast<int64_t>(ts.tv_sec) * 1000000 + ts.tv_nsec / 1000);
}

std::string Timestamp::toString() const
{
    char buf[128] = {0};
    struct tm tm_time;
    localtime_r(&microSecondsSinceEpoch_, &tm_time);
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d", 
        tm_time.tm_year + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec);
    return buf;
}

}  // namespace muduo

// #include <iostream>
// int main()
// {
//     std::cout << Timestamp::now().toString() << std::endl; 
//     return 0;
// }