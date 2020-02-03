//#include "stdafx.h"
//windows

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <iostream>

// Windows strings mess
#include <locale>
#include <codecvt>

#include <string>
#include <vector>
#include <memory>

#include "log.h"
#include <cstddef>

std::string GetLastErrorAsString() {
	//Get the error message, if any.
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
		return std::string(); //No error message has been recorded

	LPSTR messageBuffer = nullptr;
	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) & messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);

	//Free the buffer.
	LocalFree(messageBuffer);

	return message;
}

class ProcessInfoAuto : public PROCESS_INFORMATION {
public:
	// ctor required by minGW but not by visual studio 2015 .. go figure.
	ProcessInfoAuto() {
		hProcess = nullptr;
		hThread = nullptr;
		dwProcessId = 0;
		dwThreadId = 0;
	}

	~ProcessInfoAuto() {
		if (hProcess) {
			if (!TerminateProcess(hProcess, 1)) {
				// Maybe the process has already terminated when ProcessInfoAuto gets destroyed.
				//LOG(LOG_DEBUG, GetLastErrorAsString().c_str());
			}
			if (!CloseHandle(hProcess)) {
				LOG(LOG_ERR, GetLastErrorAsString().c_str());
			}
		}
		if (hThread) {
			if (!CloseHandle(hThread)) {
				LOG(LOG_ERR, GetLastErrorAsString().c_str());
			}
		}
	}

	// As the dtor closes some ressource, we prevent this class from being copied.
	ProcessInfoAuto(const ProcessInfoAuto&) = delete;
	ProcessInfoAuto& operator=(const ProcessInfoAuto&) = delete;
};

/*
 * Execute a child process but wait only for the specified "timeout_millisecond" before killing it.
 *
 * Return:
 *   The exit code of the process, or raise exception if we could not get the exit_code of the
 *   child process for whatever reason (failure to create process, process still running at timeout,
 *   or other errors thrown by sub functions).
 */
DWORD executeProcessForMaxTime(std::string absolute_exec_path, std::vector<std::string> args, DWORD timeout_millisecond) {
	DWORD exit_code = 0;
	std::wstring_convert < std::codecvt_utf8_utf16<wchar_t>> converter;

	std::string str_args;
	bool first = true;
	for (std::string & arg : args) {
		if (first) {
			first = false;
			str_args.append(arg);
		} else {
			str_args.append(" ");
			str_args.append(arg);
		}
	}
	std::wstring w_absolute_exec_path = converter.from_bytes(absolute_exec_path);
	std::wstring wstr_args = converter.from_bytes(str_args);

	// crazy windows wants a non-const string in CreateProcess so we have to duplicate it.
	std::unique_ptr<char[] > buf(new char[str_args.size() + 1]);
	strncpy(buf.get(), str_args.c_str(), str_args.size());
	buf.get()[str_args.size()] = '\0';

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof (sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	STARTUPINFO si;
	ZeroMemory(&si, sizeof (si));
	si.cb = sizeof (si);

	ProcessInfoAuto pi;
	// Start the child process.

	LOG(LOG_DEBUG, "Executing: %s, with args: %s", absolute_exec_path.c_str(), str_args.c_str());
	if (!CreateProcess(absolute_exec_path.c_str(), // No module name (use command line)
			buf.get(), // Command line
			NULL, // Process handle not inheritable
			NULL, // Thread handle not inheritable
			FALSE, // Set handle inheritance to FALSE
			0, // No creation flags
			NULL, // Use parent's environment block
			NULL, // Use parent's starting directory
			&si, // Pointer to STARTUPINFO structure
			&pi) // Pointer to PROCESS_INFORMATION structure
			) {
		throw std::runtime_error(GetLastErrorAsString());
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, timeout_millisecond);

	if (FALSE == GetExitCodeProcess(pi.hProcess, &exit_code)) {
		throw std::runtime_error(GetLastErrorAsString());
	} else if (STILL_ACTIVE == exit_code) {
		LOG(LOG_DEBUG, "Process is still running but timeout is exceeded");

		throw std::runtime_error("Timeout exceeded");
	} else {
		LOG(LOG_DEBUG, "Exit code of child process: %d", exit_code);
	}

	return exit_code;
}
