#pragma once

#include <windows.h>
DWORD executeProcessForMaxTime(std::string absolute_exec_path, std::vector<std::string> args, DWORD timeout_millisecond);


