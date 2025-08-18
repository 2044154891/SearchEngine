#include <time.h>
#include <ctime>
#include "Timestamp.h"

Timestamp::Timestamp() : microSecondsSinceEpoch_(0){}

Timestamp::Timestamp(int64_t microSecondsSinceEpoch)
    : microSecondsSinceEpoch_(microSecondsSinceEpoch){}

Timestamp Timestamp::now(){
    return Timestamp(static_cast<int64_t>(time(NULL)));
}

std::string Timestamp::toString() const{
    char buf[128] = {0};
    tm tm_time;
    localtime_r(&microSecondsSinceEpoch_,&tm_time);
    snprintf(buf,128,"%4d/%02d/%02d %02d:%02d:%02d",
        tm_time.tm_yday + 1900,
        tm_time.tm_mon + 1,
        tm_time.tm_mday,
        tm_time.tm_hour,
        tm_time.tm_min,
        tm_time.tm_sec
    );
    return buf;
}

