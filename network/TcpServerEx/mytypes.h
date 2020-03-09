#ifndef MYTYPES_H
#define MYTYPES_H

#define LOG_PRINT(fmt, args...) do{fprintf(stdout, fmt "%s\n",  ## args);fflush(stdout);}while(0)
#define LOG_INFO(fmt, args...) do{fprintf(stdout, "[INF][%s:%d] " fmt " \n", __FUNCTION__,__LINE__, ## args );fflush(stdout);}while(0)
#define LOG_WARN(fmt, args...) do{fprintf(stdout, "\x1B[WRN][33m[%s:%d] " fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);}while(0)
#define LOG_DEBUG(fmt, args...) do{fprintf(stdout, "\x1B[DBG][36m[%s:%d] " fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);}while(0)
#define LOG_ERROR(fmt, args...) do{fprintf(stdout, "\x1B[31m[ERR][%s:%d] " fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);}while(0)
#define LOG_FATAL(fmt, args...) do{fprintf(stdout, "\x1B[41m[FAT][%s:%d] " fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);}while(0)


#endif // MYTYPES_H
