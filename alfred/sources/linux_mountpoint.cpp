#include <iostream>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include "log.h"
#include "configuration.h"
#include "linux_mountpoint_conflict.h" // pour _is_share_available

static bool _remount_env(void);
static bool _getIpFromAddr(std::string &dst, std::string &ip);
static bool _getShareTargetIp(std::string &targetIp);
static bool _getIpFromUnc(std::string &unc, std::string &uncIp);
static bool _isNfs(void);
static bool _isRealNfs(void);
static bool _getShareUnc(std::string &unc);

bool _getAddrFromUnc(std::string &unc, std::string &uncAddr);

bool is_pra_available(void) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	return _is_share_available(conf.unc_path_failover_share_);
}

bool is_prod_available(void) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	return _is_share_available(conf.unc_path_main_share_);
}

bool remount_prod(void) {
	Context& ctx = Context::get_instance();
	ctx.setState(0);
	return _remount_env();
}

bool remount_pra(void) {
	Context& ctx = Context::get_instance();
	ctx.setState(1);
	return _remount_env();
}

bool tryMountPRODorPRA(void) {
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
	return true;
}

bool setRealContextState(void) {
	Context& ctx = Context::get_instance();
	ConfigurationData conf = Configuration::get_instance().getConfData();

	std::string currentUncIp;
	if( !_getShareTargetIp(currentUncIp) ) {
		LOG(LOG_DEBUG, "_getShareTargetIp error");
		return false;
	}

	std::string prodUncIp, praUncIp;
	if( !_getIpFromUnc(conf.unc_path_main_share_, prodUncIp) ) {
		LOG(LOG_DEBUG, "_getIpFromUnc prod error");
		return false;
	}
	if( !_getIpFromUnc(conf.unc_path_failover_share_, praUncIp) ) {
		LOG(LOG_DEBUG, "_getIpFromUnc pra error");
		return false;
	}

	if( currentUncIp == prodUncIp ) {
		LOG(LOG_DEBUG, "We are on prod");
		ctx.setState(0);
		return true;
	} else if( currentUncIp == praUncIp ) {
		LOG(LOG_DEBUG, "We are on pra");
		ctx.setState(1);
		return true;
	} else {
		LOG(LOG_DEBUG, "We are on ??");
		ctx.setState(0);
		return false;
	}
}

#include <sys/vfs.h>
#define NFS_SUPER_MAGIC 0x6969
bool is_mountpoint_available(void) {
	ConfigurationData conf = Configuration::get_instance().getConfData();

	LOG(LOG_DEBUG, "_isNfs()");
	if( !_isNfs() ) {
		LOG(LOG_ERR, "%s in not nfs", conf.mount_point_.c_str());
		return false;
	}
	LOG(LOG_DEBUG, "_isNfs() == true");

	std::string unc;
	LOG(LOG_DEBUG, "_getShareUnc()");
	if( !_getShareUnc(unc) ) {
		LOG(LOG_DEBUG, "_getShareUnc failed. %s not mounted ?", conf.mount_point_.c_str());
		return false;
	}
	LOG(LOG_DEBUG, "_is_share_available()");
	if( !_is_share_available(unc) ) {
		LOG(LOG_DEBUG, "%s is not available (callrpc failed)", unc.c_str());
		return false;
	}

	LOG(LOG_DEBUG, "_isRealNfs()");
	if( !_isRealNfs() ) {
		LOG(LOG_ERR, "%s in not nfs", conf.mount_point_.c_str());
		return false;
	}

	LOG(LOG_DEBUG, "opendir()");
	DIR *mp = opendir(conf.mount_point_.c_str());
	if( mp == NULL ) {
		LOG(LOG_DEBUG, "Opendir error on %s : %d", conf.mount_point_.c_str(), errno);
		return false;
	}
	closedir(mp);

	LOG(LOG_DEBUG, "%s is available", conf.mount_point_.c_str());

	return true;
}

static bool _isRealNfs(void) {
	ConfigurationData conf = Configuration::get_instance().getConfData();
	struct statfs buf;
	if( statfs(conf.mount_point_.c_str(), &buf) == -1 ) {
		LOG(LOG_DEBUG, "statfs error on %s : %d", conf.mount_point_.c_str(), errno);
		return false;
	}
	if( buf.f_type == NFS_SUPER_MAGIC ) {
		LOG(LOG_DEBUG, "%s is a NFS mount point", conf.mount_point_.c_str());
		return true;
	} else {
		LOG(LOG_DEBUG, "%s is not a NFS mount point", conf.mount_point_.c_str());
		return false;
	}
}

static bool _isNfs(void) {
	std::string fsType;
	if( !_getShareParam(param_fstype, fsType) ) {
		LOG(LOG_DEBUG, "_getShareParam fstype error");
		return false;
	}
	if( fsType.find("nfs") != std::string::npos ) {
		return true;
	} else {
		return false;
	}
}

#include <netdb.h>
#include <sys/socket.h>
#include <string.h>
static bool _getIpFromAddr(std::string &dst, std::string &ip) {
	struct addrinfo hints, *info;
	int gai_result;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_CANONNAME;

	if ((gai_result = getaddrinfo(dst.c_str(), NULL, &hints, &info)) != 0) {
		LOG(LOG_ERR, "getaddrinfo: %s\n", gai_strerror(gai_result));
		return false;
	} else {
		char _ip[50] = {0};
		gai_result = getnameinfo(info->ai_addr, info->ai_addrlen, _ip, sizeof(_ip), NULL, 0, NI_NUMERICHOST);
		if( gai_result != 0 ) {
			LOG(LOG_ERR, "getnameinfo: %s\n", gai_strerror(gai_result));
			freeaddrinfo(info);
			return false;
		} else {
			freeaddrinfo(info);
			ip = std::string(_ip);
			return true;
		}
	}
}

static bool _getShareUnc(std::string &unc) {
	_getShareParam(param_unc, unc);
	return true;
}

static bool _getShareTargetIp(std::string &targetIp) {
	_getShareParam(param_targetip, targetIp);
	return true;
}

bool _getAddrFromUnc(std::string &unc, std::string &uncAddr) {
	std::size_t r = unc.find_first_of(':');
	if( r == std::string::npos ) {
		return false;
	} else {
		uncAddr = unc.substr(0,r);
	}
	return true;
}

static bool _getIpFromUnc(std::string &unc, std::string &uncIp) {
	std::string uncAddr;
	if( !_getAddrFromUnc(unc, uncAddr) ) {
		LOG(LOG_DEBUG, "getAddrFromUnc error");
		return false;
	} else {
		LOG(LOG_DEBUG, "addr from unc: %s", uncAddr.c_str());
	}

	if( ! _getIpFromAddr(uncAddr, uncIp) ) {
		LOG(LOG_DEBUG, "getIpFromAddr error");
		return false;
	} else {
		LOG(LOG_DEBUG, "ip from add : %s", uncIp.c_str());
	}
	return true;
}

//#include <linux/nfs_mount.h>
#include <sys/mount.h>
// since 2.4.11
static bool _remount_env(void) {
	Context& ctx = Context::get_instance();
	ConfigurationData conf = Configuration::get_instance().getConfData();

	// MNT_DETACH n'existe pas dans les vieux centos....
	if( umount2(conf.mount_point_.c_str(), MNT_FORCE) == -1 ) {
		switch( errno ) {
			case EINVAL:
				LOG(LOG_DEBUG, "%s is not a mount point", conf.mount_point_.c_str());
				break;
			case EPERM:
				LOG(LOG_ERR, "i do not have the requied privileges to umount %s", conf.mount_point_.c_str());
				return false;
			default:
				LOG(LOG_ERR, "umount %s failed : %s (%d)", conf.mount_point_.c_str(), strerror(errno), errno);
				return false;
		}
	} else {
		LOG(LOG_DEBUG, "umount %s succeed", conf.mount_point_.c_str());
	}

	std::string unc;
	if( ctx.getState() == 0 ) {
		unc = conf.unc_path_main_share_;
	} else if( ctx.getState() == 1 ) {
		unc = conf.unc_path_failover_share_;
	} else {
		LOG(LOG_DEBUG, "_remount_env unknown state %d", ctx.getState());
		return false;
	}

	//nfs_mount_data nd = {0};
	std::string uncIp;
	if( !_getIpFromUnc(unc, uncIp) ) {
		LOG(LOG_DEBUG, "_getIpFromUnc error");
		return false;
	}
	std::string nfs_options("addr="+uncIp);
	LOG(LOG_DEBUG, "Mounting <%s> to <%s> with options <%s>", unc.c_str(), conf.mount_point_.c_str(), nfs_options.c_str());
	std::string cmd = "mount -t nfs -o timeo=100,retrans=2 " + unc + " " + conf.mount_point_;
	int res = system(cmd.c_str());
	// todo mount() donne une erreur Permission Denied (13) <- pourquoi ?? pour l'instant on reste sur system.
	//int res = mount(unc.c_str(), conf.mount_point_.c_str(), "nfs", 0, nfs_options.c_str());
	switch( res ) {
		case 0:
			LOG(LOG_DEBUG, "mount %s to %s succeed", unc.c_str(), conf.mount_point_.c_str());
			return true;
		case 1:
			LOG(LOG_ERR, "i do not have the requied privileges to mount %s", conf.mount_point_.c_str());
			return false;
		default:
			LOG(LOG_ERR, "Mount error : %s (%d)", strerror(errno), errno);
			return false;
	}
}
