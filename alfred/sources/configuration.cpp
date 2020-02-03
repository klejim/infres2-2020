#include "configuration.h"
#include <iostream>
#include <fstream>
#include <assert.h>

#if defined(_WIN32)
	#include <windows.h>
	#include "windows_service.h" // pour getProgDirectory
#else
	#include <unistd.h>
	#include "linux_service.h" // pour getProgDirectory
#endif

//windows
std::string get_default_log_file_path()
{
  std::string log_path(getProgDirectory());
#if defined(_WIN32)
  log_path.append("\\xxxxx_drp"); // "__log" will be append by the log lib ...
#else
  log_path.append("/xxxxx_drp");
#endif
  return log_path;
}

Configuration* Configuration::instance_ = NULL;

// Exit the program if we cannot construct the configuration object.
Configuration::Configuration(void) {
	std::string configFilePath (getProgDirectory());
#if defined(_WIN32)
	configFilePath.append("\\conf.cfg");
#else
	configFilePath.append("/conf.cfg");
#endif
	conf_.loadConfigurationFromFile(configFilePath);
}


// should be called only once
void Configuration::createInstanceOrDie(void) {
	assert(!instance_);
	try {
		instance_ = new Configuration();
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
		exit(1);
	}
}

bool Configuration::instanceIsCreated() {
	return (instance_ != NULL);
}

Configuration& Configuration::get_instance() {
	assert(instance_);
	return *instance_;
}

/*
 * Raises an exception if an error occurs while parsing configuration
*/
void FileConfiguration::loadConfigurationFromFile(std::string configuration_file_path) {
	Json::Reader reader;
	Json::Value root;
	std::ifstream ifs;
	ifs.open(configuration_file_path.c_str());

	if (!ifs.is_open()) {
		throw std::runtime_error(std::string("Unable to open file ") + configuration_file_path);
	}
	if(!reader.parse(ifs, root, IGNORE_COMMENTS_IN_JSON)) {
		throw std::runtime_error(std::string("Unable to parse json: ") + reader.getFormattedErrorMessages());
	}

	// Reads conf parameters and makes some controls on their value
	unc_path_main_share_ = root.get("unc_path_main_share", "").asString();
	share_user_login_ = root.get("share_user_login", false).asBool();
	share_login_ = root.get("share_login", "").asString();
	share_password_ = root.get("share_password", "").asString();
	unc_path_failover_share_ = root.get("unc_path_failover_share", "").asString();
	poll_period_in_second_ = root.get("poll_period_in_second", 10).asUInt();
	max_time_for_dir_command_in_second_ = root.get("max_time_for_dir_command_in_second", 5).asUInt();
	max_time_for_use_command_in_second_ = root.get("max_time_for_use_command_in_second", 5).asUInt();
	cmd_exe_absolute_path_ = root.get("cmd_exe_absolute_path", "").asString();
	mount_point_ = root.get("mount_point", "").asString();
	drive_letter_ = mount_point_ + std::string(":");

	if (poll_period_in_second_ == 0 || poll_period_in_second_ > 36000) {
		// very likely that this is not what the user wanted. Better tell him.
		throw std::runtime_error("Invalid configuration file (poll_period_in_second cannot be 0 or greater than 36 000)");
	}
	log_level_debug_ = root.get("log_level_debug", false).asBool();
	log_file_path_ = get_default_log_file_path();

	if (unc_path_main_share_.empty()) {
		throw std::runtime_error("Invalid configuration file (unc_path_main_share cannot be empty)");
	}
	if (unc_path_failover_share_.empty()) {
		throw std::runtime_error("Invalid configuration file (unc_path_failover_share cannot be empty)");
	}
#if defined(_WIN32)
	if (cmd_exe_absolute_path_.empty()) {
		throw std::runtime_error("Invalid configuration file (cmd_exe_absolute_path cannot be empty)");
	}
#endif
	if (mount_point_.empty()) {
		throw std::runtime_error("Invalid configuration file (mount_point cannot be empty)");
	}
}

/*
 * This function returns a copy of the conf parameters, so that the client code
 * can use the different parameters without having to deal with race condition if
 * another thread is modifying the configuration.
 */
ConfigurationData Configuration::getConfData() {
	return conf_;
}



Context *Context::instance_ = NULL;

Context::Context(void) { }

void Context::createInstanceOrDie(void) {
	assert(!instance_);
	try {
		instance_ = new Context();
	} catch (std::exception & e) {
		std::cerr << e.what() << std::endl;
		exit(1);
	}
}

bool Context::instanceIsCreated() {
	return (instance_ != NULL);
}

Context& Context::get_instance() {
	assert(instance_);
	return *instance_;
}

int Context::getState(void)
{
	return this->state;
}

void Context::setState(int _state)
{
	assert(instance_);
	this->state = _state;
}
