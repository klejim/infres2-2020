#pragma once

#include <windows.h>
void start_win_service(void);
int win_service_postinit(void);
void notify_run_win_service(void);
void finish_win_service(void);
void control_handler(DWORD request);
void close_win_handle(void);
bool is_stop_main_loop_requested(const unsigned int timeout);
const std::string getProgDirectory(void);
bool impersonateToken(HANDLE handle);
bool impersonateDefault(void);
void revertToSelf(void);

#define APP_NAME "share_failover"

