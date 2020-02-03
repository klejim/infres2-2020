#include <string>
#include "log.h"
#include "linux_mountpoint_conflict.h" // pour param_t
#include "mountpoint.h" // pour _getAddrFromUnc
#include "configuration.h"

bool _is_share_available(std::string &unc) {
	// todo maitriser le timeout en utilisant clnt_udpcreate, clnt_call... à la place de callrpc
	std::string addr;
	if( !_getAddrFromUnc(unc, addr) ) {
		return false;
	}

	int rpcRes = 0;
	switch( rpcRes ) {
		case 0:
			LOG(LOG_DEBUG, "callrpc succes !", rpcRes);
			return true;
		default:
			LOG(LOG_DEBUG, "callrpc failed (%d)", rpcRes);
			return false;
	}

	return false;
}

#if ! defined(NOLIBMOUNT) 
#include "libmount.h"
bool _getShareParam(param_t param, std::string &paramValue) {
	ConfigurationData conf = Configuration::get_instance().getConfData();

	struct libmnt_table *mnt_table = mnt_new_table_from_file("/proc/self/mountinfo");
	if( mnt_table == NULL ) {
		LOG(LOG_DEBUG, "mnt_new_table_from_file error");
		return false;
	}
	struct libmnt_fs * mnt_fs = mnt_table_find_mountpoint(mnt_table, conf.mount_point_.c_str(), MNT_ITER_FORWARD);
	if( mnt_fs == NULL ) {
		LOG(LOG_DEBUG, "mnt_new_table_from_file error");
		return false;
	}

	LOG(LOG_DEBUG, "find target %s (%s)", mnt_fs_get_srcpath(mnt_fs), mnt_fs_get_options(mnt_fs));

	switch( param ) {
		case param_targetip:
		{
			std::string nfs_options(mnt_fs_get_options(mnt_fs));
			std::size_t p = nfs_options.find(",addr=") + 6;
			if( p == std::string::npos ) {
				LOG(LOG_DEBUG, "cannot find server addr in nfs options");
				return false;
			}
			std::size_t q = nfs_options.find_first_of(',', p);
			paramValue = nfs_options.substr(p,q);

			LOG(LOG_DEBUG, "target ip : %s", paramValue.c_str());
		}
			break;
		case param_unc:
		{
			std::string srcpath(mnt_fs_get_srcpath(mnt_fs));
			paramValue = srcpath;
			LOG(LOG_DEBUG, "mount unc : %s", paramValue.c_str());
		}
			break;
		case param_fstype:
		{
			std::string fstype(mnt_fs_get_fstype(mnt_fs));
			paramValue = fstype;
			LOG(LOG_DEBUG, "mount fstype : %s", paramValue.c_str());
		}
			break;
		default:
			LOG(LOG_DEBUG, "_getShareParam param not implemented");
			return false;
	}

	mnt_free_fs(mnt_fs);
	mnt_free_table(mnt_table);
	return true;
}
#else
bool _getShareParam(param_t param, std::string &paramValue) {
	ConfigurationData conf = Configuration::get_instance().getConfData();

	FILE  *g = fopen("/proc/self/mountinfo", "r");
	
	if( !g ) {
		LOG(LOG_ERR, "cannot open /proc/self/mountinfo");
		return false;
	}

	char buf[2048], _mp[512], _fstype[20], _target[512], _options[512];
	bool found = false;
	while( fgets(buf, sizeof(buf), g) != NULL ) {
		if( sscanf(buf, "%*s %*s %*s %*s %512s %*s %*s - %20s %512s %512s", _mp, _fstype, _target, _options) != 4 ) {
			continue;
		}

		std::string mp = std::string(_mp);
		if( mp == conf.mount_point_ ) {
			found = true;
			break;
		}
	}

	if( !found ) {
		LOG(LOG_ERR, "%s not found in /proc/self/mountinfo", conf.mount_point_.c_str());
		return false;
	}
	fclose(g);

	switch( param ) {
		case param_targetip:
		{
			std::string nfs_options(_options);
			std::size_t p = nfs_options.find(",addr=") + 6;
			if( p == std::string::npos ) {
				LOG(LOG_DEBUG, "cannot find server addr in nfs options");
				return false;
			}
			std::size_t q = nfs_options.find_first_of(',', p);
			paramValue = nfs_options.substr(p,q);

			LOG(LOG_DEBUG, "target ip : %s", paramValue.c_str());
		}
			break;
		case param_unc:
		{
			std::string srcpath(_target);
			paramValue = srcpath;
			LOG(LOG_DEBUG, "mount unc : %s", paramValue.c_str());
		}
			break;
		case param_fstype:
		{
			std::string fstype(_fstype);
			paramValue = fstype;
			LOG(LOG_DEBUG, "mount fstype : %s", paramValue.c_str());
		}
			break;
		default:
			LOG(LOG_DEBUG, "_getShareParam param not implemented");
			return false;
	}
	

	return true;
}

#endif
