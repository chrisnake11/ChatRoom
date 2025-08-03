#pragma once
#include "Const.h"
#include "Singleton.h"
struct SectionInfo {
	std::map<std::string, std::string> _section_datas;
	std::string operator[] (const std::string& key)
	{
		if (_section_datas.find(key) == _section_datas.end()) {
			return "";
		}
		else {
			return _section_datas[key];
		}
	}

	SectionInfo operator=(const SectionInfo& section) {
		if (&section == this) {
			return *this;
		}
		this->_section_datas = section._section_datas;
		return *this;
	}
};

class ConfigManager
{
public:
	ConfigManager& operator=(const ConfigManager& config_manager) {
		if (&config_manager == this) {
			return *this;
		}
		this->_config_datas = config_manager._config_datas;
		return *this;
	}
	SectionInfo operator[] (const std::string& section_name)
	{
		if (_config_datas.find(section_name) == _config_datas.end()) {
			return SectionInfo();
		}
		else {
			return _config_datas[section_name];
		}
	}

	static ConfigManager& GetInstance()
	{
		static ConfigManager config_manager;
		return config_manager;
	}
private:
	ConfigManager();
	std::map<std::string, SectionInfo> _config_datas;
};

