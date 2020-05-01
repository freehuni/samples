#include <stdarg.h>
#include <stdio.h>
#include <thread>
#include <time.h>
#include <sys/time.h>
#include "logger.h"

using namespace std;

void printLog(const char* funcname, int codeline, char* fmt, ...)
{
    va_list ap;
    char buf[2001]={0};
    struct timeval now;
    struct tm* local = nullptr;
    
    gettimeofday(&now, nullptr);
    local = localtime(&now.tv_usec);

    va_start(ap, fmt);    
    vsnprintf(buf, 2000, fmt, ap);
    va_end(ap);

    printf("[INF][%02d:%02d:%02d.%03d][%u][%s:%d] %s\n", 
        local->tm_hour, local->tm_min, local->tm_sec, now.tv_usec / 1000,
        this_thread::get_id(),  funcname , codeline , buf);
}