#pragma once

bool setRealContextState(void);
bool is_mountpoint_available(void);
bool is_pra_available(void);
bool is_prod_available(void); 
bool tryMountPRODorPRA(void);
bool remount_prod(void);
bool remount_pra(void);

#if defined(_WIN32)
	#include <windows.h>
	bool firstMount(DWORD sessionId);
	bool umountAllConnectionByToken(HANDLE hToken);
	bool listAllConnectionByToken(HANDLE hToken);
#else
	bool _getAddrFromUnc(std::string &unc, std::string &uncAddr);
#endif


