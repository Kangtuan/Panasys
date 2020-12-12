#ifndef __DEBUG_H__
#define __DEBUG_H__

#define LOG_SIZE   (64)
extern char log_buf[LOG_SIZE];
#define LOG(fmt, ...) { sprintf(log_buf, fmt, ##__VA_ARGS__); Serial.print(log_buf); }

#endif