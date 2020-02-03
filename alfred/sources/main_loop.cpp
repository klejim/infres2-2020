#include "configuration.h"
#include <time.h>
#include "log.h"
#include <memory>
#include "mountpoint.h"

#if defined(_WIN32)
	#include <windows.h>
	#include "windows_service.h"
#else
	#include <stdlib.h> //exit()
	#include <sys/time.h>
	#include "linux_service.h"
#endif


/**
 * \brief This function return the current time
 *
 * \return struct timeval containing current time in second in .tv_sec
 **/
struct timeval getCurTime() {
	struct timeval returnTime;

	time_t curTime;
	time(&curTime);

	returnTime.tv_sec = (long) curTime;
	returnTime.tv_usec = 0;

	return returnTime;
}

/**
 * \brief This function return the time duration since the date "time" provided in arg, in second.
 *
 * \param time The time we'll be doing the diff with.
 * \return An int corresponding to curTime - time
 **/

int getDiffWithCurrentTime(const struct timeval time) {
	struct timeval curTime;
	curTime = getCurTime();

	double diffSec = difftime(curTime.tv_sec, time.tv_sec);

	return (int) diffSec;
}

std::string double_quote(std::string str) {
	return std::string("\"") + str + std::string("\"");
}

void main_loop(void) {
	LOG(LOG_DEBUG, "main_loop: start");

#if defined(_WIN32)
	if (win_service_postinit()) {
		exit(1);
	}
	notify_run_win_service();
#endif
	ConfigurationData conf = Configuration::get_instance().getConfData();
	Context& ctx = Context::get_instance();
	struct timeval curTimeStart;
	unsigned int elapsedTime, timeToSleep;
	setRealContextState();
	do {
		curTimeStart = getCurTime();
		try {
			if ( is_mountpoint_available() ) {
				if( ctx.getState() == 0 ) { // On est en prod
					LOG(LOG_DEBUG, "main share is mounted");
				} else if( ctx.getState() == 1 ) { // On est en failover
					if( is_prod_available() ) {
						tryMountPRODorPRA();
					}
				} else { // Le point de montage n'est monté ni sur PROD ni sur PRA
					LOG(LOG_ERR, "Unknown mount point. Cannot continue");
				}
			} else {
				LOG(LOG_DEBUG, "Mount point is not mounted");
				tryMountPRODorPRA();
			}
		} catch (std::exception & e) {
			LOG(LOG_ERR, "Exception in main loop: %s", e.what());
		}

		elapsedTime = getDiffWithCurrentTime(curTimeStart);
		uint32_t poll_period_in_second = conf.poll_period_in_second_;
		timeToSleep = poll_period_in_second - elapsedTime;
		if (timeToSleep > poll_period_in_second) {
			timeToSleep = 0;
		}
	} while (!is_stop_main_loop_requested(timeToSleep));

	LOG(LOG_DEBUG, "main_loop: exiting from main loop");
//windows
#if defined(_WIN32)
	finish_win_service();
#endif
	LOG(LOG_DEBUG, "main_loop: end");
}
