#include "ConfigManager.h"

ConfigManager::ConfigManager()
{
	boost::filesystem::path current_path = boost::filesystem::current_path();
	boost::filesystem::path config_path = current_path / "config.ini";
	std::cout << "config path: " << config_path << std::endl;

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(config_path.string(), pt);

	// ����property_tree (ɭ��)
	for (const auto& section : pt) {
		// ��ȡ������
		std::string section_name = section.first;
		const boost::property_tree::ptree& section_pt = section.second;
		// �ӿ�������������
		std::map<std::string, std::string> section_datas;
		for (const auto& key_value : section_pt) {
			section_datas[key_value.first] = key_value.second.get_value<std::string>();
		}
		SectionInfo section_info;
		section_info._section_datas = section_datas;

		// ������section_info��ӵ�_config_datas��
		_config_datas[section_name] = section_info;
	}
}
