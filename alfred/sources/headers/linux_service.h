#pragma once

void setSignalHandler(void);
bool is_stop_main_loop_requested(const unsigned int timeout);
const std::string getProgDirectory();

#define APP_NAME "share_failover"
