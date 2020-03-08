#ifndef MYTYPES_H
#define MYTYPES_H

#define LOG_PRINT(fmt, args...) do{fprintf(stdout, fmt "%s\n",  ## args);fflush(stdout);break;}while(1)
#define LOG_INFO(fmt, args...) do{fprintf(stdout, "[%s:%d] " fmt " \n", __FUNCTION__,__LINE__, ## args );fflush(stdout);break;}while(1)
#define LOG_WARN(fmt, args...) do{fprintf(stdout, "\x1B[33m[%s:%d]" fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);break;}while(1)
#define LOG_DEBUG(fmt, args...) do{fprintf(stdout, "\x1B[36m[%s:%d]" fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);break;}while(1)
#define LOG_ERROR(fmt, args...) do{fprintf(stdout, "\x1B[31m[%s:%d]" fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);break;}while(1)
#define LOG_FATAL(fmt, args...) do{fprintf(stdout, "\x1B[41m[%s:%d]" fmt "%s\033[0m\n", __FUNCTION__,__LINE__, ## args);fflush(stdout);break;}while(1)


#endif // MYTYPES_H
