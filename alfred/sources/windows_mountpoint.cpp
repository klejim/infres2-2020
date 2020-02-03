#include <iostream>
#include <memory>
#include <windows.h>
#include "configuration.h"
#include "windows_subprocess.h"
#include "windows_service.h"
#include "log.h"
#include <dbt.h>

static bool _is_share_available(std::string unc);
static bool _broadcast(HANDLE hToken, WPARAM wParam);
static bool _mount_drive(HANDLE hToken, DWORD dwFlags = 0);
static bool _umount_drive(HANDLE hToken, bool umountDriveLetter = false); 
static bool _remount_drive_by_sid(DWORD sessionId);
static bool _remount_drive_by_token(HANDLE hToken);
static bool _handleSession(DWORD sessionId, bool (*cb) (HANDLE));
static bool _iter_users(bool (*cb) (DWORD));
static bool _remount_env(void);


bool setRealContextState(void) {
	char buf[256];
	DWORD result;
	DWORD buf_len = sizeof (buf);

	Context& ctx = Context::get_instance();
	ConfigurationData conf = Configuration::get_instance().getConfData();

	if(! impersonateDefault() ) {
		LOG(LOG_DEBUG, "setRealContext& failed : cannot impersonate default");
		return false;
	}
	result = WNetGetConnection( (LPTSTR) conf.drive_letter_.c_str(), buf, &buf_len);
	revertToSelf();
	switch (result) {
		case NO_ERROR:
			if( buf == conf.unc_path_failover_share_ ) ctx.setState(1);
			else {
				ctx.setState(0);
				return true;
			}
		case ERROR_NOT_CONNECTED:
			//
			// The device is not a redirected device.
			//
			ctx.setState(0);
			LOG(LOG_DEBUG, "Drive letter %s ERROR_NOT_CONNECTED", conf.drive_letter_.c_str());
			return true;

		case ERROR_CONNECTION_UNAVAIL:
			//
			// The device is not currently connected, but it is a persistent connection.
			//
			ctx.setState(0);
			LOG(LOG_DEBUG, "Drive letter %s: ERROR_CONNECTION_UNAVAIL", conf.drive_letter_.c_str());
			return true;

		default:
			//
			// Handle the error.
			//
			ctx.setState(0);
			LOG(LOG_ERR, "WNetGetConnection to %s failed (%d)", conf.drive_letter_.c_str(), result);
			return false;
	}
}

bool is_pra_available(void) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	return _is_share_available(conf.unc_path_failover_share_);
}

bool is_prod_available(void) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	return _is_share_available(conf.unc_path_main_share_);
}

bool firstMount(DWORD sessionId) {
	LOG(LOG_DEBUG, "firstMount for sessionId %d", sessionId);
	return _remount_drive_by_sid(sessionId);
}

bool remount_prod(void)
{
	Context& ctx = Context::get_instance();
	ctx.setState(0);
	return _remount_env();
}

bool remount_pra(void)
{
	Context& ctx = Context::get_instance();
	ctx.setState(1);
	return _remount_env();
}

bool tryMountPRODorPRA(void) {
	Context& ctx = Context::get_instance();
	std::lock_guard<std::mutex> lock(ctx.mtx);
	if( is_prod_available() ) {
		LOG(LOG_DEBUG, "PROD is available. Remounting PROD.");
		return remount_prod();
	} else if( is_pra_available() ) {
		LOG(LOG_DEBUG, "PRA is available. Remounting PRA.");
		return remount_pra();
	} else {
		LOG(LOG_DEBUG, "PROD and PRA are unavailable");
		return false;
	}
}

static bool _umountConnectionByTokenUnc(HANDLE hToken, const char *unc) {
#define IGNORE_OPENED_FILES true
	DWORD res = 0;
	LOG(LOG_DEBUG, "Unmounting %s", unc);
	if( ! impersonateToken(hToken) )
	{
		LOG(LOG_DEBUG, "Impersonate failed");
		return false;
	}
	res = WNetCancelConnection2( unc, CONNECT_UPDATE_PROFILE, IGNORE_OPENED_FILES);
	revertToSelf();

	switch(res)
	{
		case NO_ERROR:
			LOG(LOG_DEBUG, "Unmount succeeded (WNetCancelConnection2)");
			return true;
		case ERROR_NOT_CONNECTED:
			LOG(LOG_DEBUG, "%s not mounted", unc);
			return true;
		default:
			LOG(LOG_DEBUG, "Unmount %s failed (WNetCancelConnection2 returned %d)", unc, res);
			return false;
	}
}

static bool _listConnectionByTokenUnc(HANDLE hToken, const char *unc) {
	DWORD res = 0, buflen = 0;
	char buf[256] = {0};
	if( hToken == NULL ) {
		//if(! impersonateDefault() ) return false;
	} else {
		if(! impersonateToken(hToken) ) return false;
	}
	res = WNetGetConnection( (LPTSTR) unc, buf, &buflen );
	revertToSelf();
	switch (res) {
		case NO_ERROR:
			LOG(LOG_DEBUG, "%s mounted on %s", unc, buf);
			break;
		case ERROR_NOT_CONNECTED:
			//
			// The device is not a redirected device.
			//
			LOG(LOG_DEBUG, "%s ERROR_NOT_CONNECTED", unc);
			break;

		case ERROR_CONNECTION_UNAVAIL:
			//
			// The device is not currently connected, but it is a persistent connection.
			//
			LOG(LOG_DEBUG, "%s: ERROR_CONNECTION_UNAVAIL", unc);
			break;

		default:
			//
			// Handle the error.
			//
			LOG(LOG_ERR, "WNetGetConnection to %s failed (%d)", unc, res);
			break;
	}
}

bool _iterlAllConnectionByToken(HANDLE hToken, bool (*cb)(HANDLE,const char*)) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	const char *unc[3] = { conf.drive_letter_.c_str(), conf.unc_path_main_share_.c_str(), conf.unc_path_failover_share_.c_str() };
	for(int i = 0; i < sizeof(unc)/sizeof(unc[0]); i++) {
		cb(hToken, unc[i]);
	}
	return true;
}

bool umountAllConnectionByToken(HANDLE hToken) {
	_broadcast(hToken, DBT_DEVICEREMOVEPENDING);
	_iterlAllConnectionByToken(hToken, _umountConnectionByTokenUnc);
	_broadcast(hToken, DBT_DEVICEREMOVECOMPLETE);
	return true;
}

bool listAllConnectionByToken(HANDLE hToken) {
	return _iterlAllConnectionByToken(hToken, _listConnectionByTokenUnc);
}

bool is_mountpoint_available(void) {
	bool res = false;
	ConfigurationData conf = Configuration::get_instance().getConfData();
	std::vector<std::string> args;
	args.push_back( std::string("\"") + conf.cmd_exe_absolute_path_ + std::string("\""));
	args.push_back("/C");
	args.push_back("dir");
	args.push_back( std::string("\"") + conf.drive_letter_ + std::string("\""));
	if(! impersonateDefault() ) {
		LOG(LOG_DEBUG, "is_mountpoint_available failed : cannot impersonate default");
		return false;
	}
	res = !executeProcessForMaxTime(conf.cmd_exe_absolute_path_, args, conf.max_time_for_dir_command_in_second_ * 1000);
	revertToSelf();
	return res;
}

static bool _remount_env(void){
	if( _mount_drive(NULL) ) {
		LOG(LOG_DEBUG, "_mount_drive(NULL) ok, iterating users");
		return _iter_users(_remount_drive_by_sid);
	} else {
		LOG(LOG_DEBUG, "_mount_drive(NULL) error");
		return false;
	}
}

static bool _is_share_available(std::string unc) {
	LOG(LOG_DEBUG, "Testing %s", unc.c_str());
	DWORD dwRetVal;
	DWORD dwFlags;
	LPCSTR login, password;
	dwFlags = CONNECT_TEMPORARY;

	ConfigurationData conf = Configuration::get_instance().getConfData();

	NETRESOURCE nr;
	memset(&nr, 0, sizeof (NETRESOURCE));
	nr.dwScope = RESOURCE_GLOBALNET;
	nr.dwType = RESOURCETYPE_DISK;
	nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
	nr.lpLocalName = NULL;
	nr.lpRemoteName = (LPTSTR) unc.c_str();

	if( conf.share_user_login_ ) {
		login = password = NULL;
		if( conf.share_login_.length() > 0 ) login = conf.share_login_.c_str();
		if( conf.share_password_.length() > 0 ) password = conf.share_password_.c_str();
	} else {
		login = (conf.share_login_.length() == 0)? "":conf.share_login_.c_str();
		password = (conf.share_password_.length() == 0)? "":conf.share_password_.c_str();
	}
	LOG(LOG_DEBUG, "Test with login: %s", login);

	if( ! impersonateDefault() ) {
		LOG(LOG_DEBUG, "_is_share_available failed : cannot impersonate default");
		return false;
	}
	dwRetVal = WNetAddConnection2(&nr, password, login, dwFlags);
	revertToSelf();
	if (dwRetVal == NO_ERROR) {
		LOG(LOG_DEBUG, "Test succeeded (WNetAddConnection2)");
		return true;
	} else {
		LOG(LOG_DEBUG, "Test failed (WNetAddConnection2 returned %d)", dwRetVal);
		return false;
	}
}

static bool _broadcast(HANDLE hToken, WPARAM wParam) {
	// Broadcast msg
	ConfigurationData conf = Configuration::get_instance().getConfData();
	LOG(LOG_DEBUG, "Sending broadcast...");
	DWORD recipients = BSM_ALLDESKTOPS | BSM_APPLICATIONS;
	DEV_BROADCAST_VOLUME msg;
	ZeroMemory(&msg, sizeof(msg));
	msg.dbcv_size = sizeof(msg);
	msg.dbcv_devicetype = DBT_DEVTYP_VOLUME;
	msg.dbcv_unitmask = 1 << ( conf.mount_point_.c_str()[0] - 'A');
	msg.dbcv_flags = DBTF_NET;
	if( impersonateToken(hToken) )
	{
		long success = BroadcastSystemMessage(BSF_IGNORECURRENTTASK|BSF_FORCEIFHUNG, &recipients, WM_DEVICECHANGE, wParam, (LPARAM)&msg);
		revertToSelf();
		if( success > 0 )
		{
			LOG(LOG_DEBUG, "BroadcastSystemMessage OK");
			return true;
		}
		else
		{
			LOG(LOG_DEBUG, "BroadcastSystemMessage error %ld (%ld)",success, GetLastError());
			return false;
		}
	}
	else
	{
		LOG(LOG_DEBUG, "Impersonate failed");
		return false;
	}
}

static bool _mount_drive(HANDLE hToken, DWORD dwFlags) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	Context& ctx = Context::get_instance();
	std::string unc;
	if( ctx.getState() == 0 ) {
		unc = conf.unc_path_main_share_;
	} else if( ctx.getState() == 1 ) {
		unc = conf.unc_path_failover_share_;
	} else {
		LOG(LOG_DEBUG, "Bad STATE (%d)", ctx.getState());
		return false;
	}

	DWORD dwRetVal;
	LPCSTR login, password;

	NETRESOURCE nr;
	memset(&nr, 0, sizeof (NETRESOURCE));
	nr.dwScope = RESOURCE_GLOBALNET;
	nr.dwType = RESOURCETYPE_DISK;
	nr.dwUsage = RESOURCEUSAGE_CONNECTABLE;
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_SHARE;
	nr.lpRemoteName = (LPTSTR) unc.c_str();
	if( hToken == NULL ) {
		// On map qu'une seule fois, avec le compte Local System. Pour les autres comptes, on se connecte juste au share.
		nr.lpLocalName = (LPTSTR) conf.drive_letter_.c_str();
	}

	if( conf.share_user_login_ ) {
		login = password = NULL;
		if( hToken == NULL ) {
			if( conf.share_login_.length() > 0 ) login = conf.share_login_.c_str();
			if( conf.share_password_.length() > 0 ) password = conf.share_password_.c_str();
		}
	} else {
		login = (conf.share_login_.length() == 0)? "":conf.share_login_.c_str();
		password = (conf.share_password_.length() == 0)? "":conf.share_password_.c_str();
	}
	LOG(LOG_DEBUG, "Mounting %s on %s with login %s (flags %d)", nr.lpRemoteName, nr.lpLocalName, login, 0);

	if( ! impersonateToken(hToken) )
	{
		LOG(LOG_DEBUG, "Impersonate failed");
		return false;
	}
	dwRetVal = WNetAddConnection2(&nr, password, login, 0);
	revertToSelf();
	switch(dwRetVal)
	{
		case NO_ERROR:
			LOG(LOG_DEBUG, "Mount succeeded (WNetAddConnection2) (%d)", dwRetVal);
			_broadcast(hToken, DBT_DEVICEARRIVAL);
			return true;
		case ERROR_ALREADY_ASSIGNED:
		{
			LOG(LOG_DEBUG, "Mount ALREADY_ASSIGNED (WNetAddConnection2) (%d)", dwRetVal);
			return true;
		}
		case ERROR_SESSION_CREDENTIAL_CONFLICT:
			if( dwFlags = 1) {
				LOG(LOG_DEBUG, "Mount failed again. Suicide me.");
				return false;
			} else {
				LOG(LOG_DEBUG, "Mount conflict. Trying with unmounting the world");
				listAllConnectionByToken(hToken);
				umountAllConnectionByToken(hToken);
				listAllConnectionByToken(hToken);
				return _mount_drive(hToken, 1);
			}
		case ERROR_DEVICE_ALREADY_REMEMBERED:
			if( nr.lpRemoteName ) {
				if( dwFlags == 0 ) {
					LOG(LOG_DEBUG, "%s is already assigned, unmounting and trying again", nr.lpRemoteName);
					_umount_drive(NULL, true);
					return _mount_drive(hToken, 1);
				}
			}

		default:
			LOG(LOG_DEBUG, "Mount failed (WNetAddConnection2 returned %d)", dwRetVal);
			return false;
	}
}

static bool _umount_drive(HANDLE hToken, bool umountDriveLetter) {
#define IGNORE_OPENED_FILES true
	DWORD dwRetVal;
	ConfigurationData conf = Configuration::get_instance().getConfData();
	Context& ctx = Context::get_instance();
	const char *unmountedDevice = NULL;

	if( hToken == NULL && !umountDriveLetter ) {
		if( ! _umount_drive(hToken, true) ) return false;
	}

	if( umountDriveLetter ) {
		_broadcast(hToken, DBT_DEVICEREMOVEPENDING);
		unmountedDevice = conf.drive_letter_.c_str();
	} else {
		switch(ctx.getState())
		{
			case 0:
				unmountedDevice = conf.unc_path_main_share_.c_str();
				break;
			case 1:
				unmountedDevice = conf.unc_path_failover_share_.c_str();
				break;
		}
	}

	LOG(LOG_DEBUG, "Unmounting %s", unmountedDevice);
	if( ! impersonateToken(hToken) )
	{
		LOG(LOG_DEBUG, "Impersonate failed");
		return false;
	}
	dwRetVal = WNetCancelConnection2( unmountedDevice, CONNECT_UPDATE_PROFILE, IGNORE_OPENED_FILES);
	revertToSelf();

	switch(dwRetVal)
	{
		case NO_ERROR:
			LOG(LOG_DEBUG, "Unmount succeeded (WNetCancelConnection2)");
			if( umountDriveLetter ) _broadcast(hToken, DBT_DEVICEREMOVECOMPLETE);
			return true;
		case ERROR_NOT_CONNECTED:
			LOG(LOG_DEBUG, "Drive %s not mounted", unmountedDevice);
			if( umountDriveLetter ) _broadcast(hToken, DBT_DEVICEQUERYREMOVEFAILED);
			return true;
		default:
			LOG(LOG_DEBUG, "Unmount failed (WNetCancelConnection2 returned %d)", dwRetVal);
			if( umountDriveLetter ) _broadcast(hToken, DBT_DEVICEQUERYREMOVEFAILED);
			return false;
	}
}

static bool _remount_drive_by_sid(DWORD sessionId)
{
	return _handleSession(sessionId, _remount_drive_by_token);
}

static bool _remount_drive_by_token(HANDLE hToken)
{
	if( _umount_drive(hToken) ) {
		LOG(LOG_DEBUG, "_remount_drive_by_token umount_drive ok, mounting");
		return _mount_drive(hToken);
	} else {
		LOG(LOG_DEBUG, "_remount_drive_by_token umount_drive error");
		return false;
	}
}

#include <wtsapi32.h>
//#pragma comment(lib,"wtsapi32.lib")
static bool _handleSession(DWORD sessionId, bool (*cb) (HANDLE))
{
	HANDLE hToken = NULL;
	if( WTSQueryUserToken(sessionId, &hToken) == 0 )
	{
		LOG(LOG_DEBUG, "WTSQueryUserToken error (%lu)", GetLastError());
		return false;
	}
	else
	{
		bool res = false;
		LOG(LOG_DEBUG, "WTSQueryUserToken ok");
		res = cb(hToken);
		CloseHandle(hToken);
		return res;
	}
}

#include <ntsecapi.h>
//#pragma comment(lib,"secur32.lib")
static bool _iter_users(bool (*cb) (DWORD))
{
	ULONG sessionsCount = 0;
	PLUID sessionsList = NULL;
	LsaEnumerateLogonSessions(&sessionsCount, &sessionsList);
	LOG(LOG_DEBUG, "%lu user sessions user", sessionsCount);
	for(ULONG i=0; i<sessionsCount; i++)
	{
		PSECURITY_LOGON_SESSION_DATA sessionData = NULL;
		LsaGetLogonSessionData(sessionsList+i, &sessionData);
		LOG(LOG_DEBUG, "Session user %ls, session id %lu", sessionData->UserName.Buffer, sessionData->Session);
		cb(sessionData->Session);
		LsaFreeReturnBuffer(sessionData);
	}
	LsaFreeReturnBuffer(sessionsList);
	return true;
}

