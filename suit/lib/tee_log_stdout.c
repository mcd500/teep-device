#include <stdio.h>
#include <stdarg.h>
#include "teelog.h"

void tee_log(enum tee_log_level level, const char *msg, ...)
{
    va_list list;
    va_start(list, msg);
    vprintf(msg, list);
    va_end(list);
}
