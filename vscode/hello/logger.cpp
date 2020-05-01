#include "logger.h"


void printLog(std::string msg)
{
    fprintf(stderr, "[INF] %s\n", msg.c_str());
}