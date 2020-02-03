#pragma once

/*
 * <sys/mount.h> et <libmount/libmount.h> conflict sur plein de define
 * <rpc/nfs_prot.h> et <nfs_mount.h> confict aussi sur plein de define...
 * donc on les sépare.
*/

typedef enum param_e {
	param_targetip = 0,
	param_unc,
	param_fstype,
	param_last
} param_t;

bool _is_share_available(std::string &unc);
bool _getShareParam(param_t param, std::string &paramValue);
