#include <iostream>
#include "configuration.h"
#include "log.h"
#include "main_loop.h"
#include "version.h"

#if defined(_WIN32)
	#include "windows_service.h"
	#include "windows_subprocess.h"
#else
	#include <stdlib.h>
	#include <unistd.h>
	#include <fcntl.h>
	#include "linux_service.h"
	#include <libgen.h>
	#include <errno.h>
	#include <string.h>
#endif

int main(int argc, char* argv[]) {
	Configuration::createInstanceOrDie();
	Context::createInstanceOrDie();

	ConfigurationData conf = Configuration::get_instance().getConfData();
	int priority;
	if (conf.log_level_debug_) {
		priority = LOG_DEBUG;
	} else {
		priority = LOG_INFO;
	}
	InitLog("", conf.log_file_path_.c_str(), priority, FILE_LOG_OUT);
	LOG(LOG_DEBUG, "%s starting (version %s #%s)", argv[0], FULL_VERSION, _nCOMMIT_TAG);

#if defined(_WIN32)
	start_win_service();
#elif defined(NOFORK)
	setSignalHandler();
	main_loop();
#else
	//DAEMONIZE
	int fork_status = fork();
	if (fork_status < 0 ) {
		LOG(LOG_CRIT, "Error during daemonizing...");
		exit(-1);
	}
	if (fork_status > 0 ) exit(0);

	int dev_null_fd = open("/dev/null", O_WRONLY);
	if(dup2(dev_null_fd, STDOUT_FILENO) == -1) {
		LOG(LOG_CRIT, "Error during daemonizing (Unable to redirect stdout)");
		exit(-1);
	}

	if(dup2(dev_null_fd, STDERR_FILENO) == -1) {
		LOG(LOG_CRIT, "Error during daemonizing (Unable to redirect stderr)");
		exit(-1);
	}

	close(dev_null_fd);
	dev_null_fd = open("/dev/null", O_RDONLY);
	if(dup2(dev_null_fd, STDIN_FILENO) == -1) {
		LOG(LOG_CRIT, "Error during daemonizing (Unable to redirect stdin)");
		exit(-1);
	}

	close(dev_null_fd);
	// Close all others FD we might have inherited from our parent.
	for (int i = 3; i < 1024; i++)
		close(i);

	char pid[6] = {0};
	size_t pidLen = snprintf(pid, 6, "%d", getpid());
	std::string baseName(basename(argv[0]));
	std::string pidFilePath("/var/run/"+baseName+".pid");
	LOG(LOG_DEBUG, "pidfile : %s", pidFilePath.c_str());
	int pidfile = open(pidFilePath.c_str(), O_WRONLY|O_CREAT);
	if( pidfile ) {
		int r = write(pidfile, pid, pidLen);
		if( r == -1 ) {
			LOG(LOG_DEBUG, "write failed in %s : %s (%d)", pidFilePath.c_str(), strerror(errno), errno);
		} else {
			LOG(LOG_DEBUG, "write %d in %s (%s)", r, pidFilePath.c_str(), pid);
		}
		close(pidfile);
	} else {
		LOG(LOG_DEBUG, "cannot open pidfile %s", pidFilePath.c_str());
	}

	setSignalHandler();
	main_loop();
	
	unlink(pidFilePath.c_str());
#endif

	LOG(LOG_DEBUG, "main end");
}
