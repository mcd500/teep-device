#pragma once

enum tee_log_level
{
    TEE_LOG_ERROR,
    TEE_LOG_WARN,
    TEE_LOG_INFO,
    TEE_LOG_DEBUG,
    TEE_LOG_TRACE
};

void tee_log(enum tee_log_level level, const char *msg, ...);

#define tee_log_error(...) tee_log(TEE_LOG_ERROR, __VA_ARGS__)
#define tee_log_warn(...)  tee_log(TEE_LOG_WARN, __VA_ARGS__)
#define tee_log_info(...)  tee_log(TEE_LOG_INFO, __VA_ARGS__)
#define tee_log_debug(...) tee_log(TEE_LOG_DEBUG, __VA_ARGS__)
#define tee_log_trace(...) tee_log(TEE_LOG_TRACE, __VA_ARGS__)
