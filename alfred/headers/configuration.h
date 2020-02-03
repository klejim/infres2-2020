#pragma once
#include <string>
#include <json/json.h>
#include <mutex>

class ConfigurationData {
	public:
		std::string unc_path_main_share_;
		std::string unc_path_failover_share_;
		bool 		share_user_login_;
		std::string share_login_;
		std::string share_password_;
		uint32_t poll_period_in_second_;
		uint32_t max_time_for_dir_command_in_second_;
		uint32_t max_time_for_use_command_in_second_;
		bool log_level_debug_; // either we only log errors, or we log everything.
		std::string log_file_path_;
		std::string cmd_exe_absolute_path_;
		std::string mount_point_;
		std::string drive_letter_;
};

class FileConfiguration : public ConfigurationData {
	public:
		void loadConfigurationFromFile(std::string file_path);
};

class Configuration {
	public:
		static Configuration& get_instance();
		static void createInstanceOrDie();
		static bool instanceIsCreated();

		ConfigurationData getConfData();

		FileConfiguration conf_;

	private:
		Configuration();
		~Configuration();
		static Configuration* instance_;
};
#define IGNORE_COMMENTS_IN_JSON false

class Context {
	private:
		int state = 0; // 0 = prod, 1 = failover
		static Context *instance_;
		Context();
		~Context();
	public:
		std::mutex mtx;
		static Context& get_instance();
		static void createInstanceOrDie();
		static bool instanceIsCreated();
		int getState(void);
		void setState(int state);
};
