#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidjson/rapidjson.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include <iostream>
#include <sstream>
#include <cstdint>
#include <string>
#include <vector>

/* prototype */
std::vector<std::string> Split(const std::string & target, const char & delimiter);

enum class ExitCode : int32_t
{
	Success = 0x00,
	Error = 0xFF,
};

std::vector<std::string> dbg_;

int main(int argc, char **argv)
{
	ExitCode exit_code = ExitCode::Success;

	/* Parse Args */
	std::string config_json_path = std::string();
	switch (argc)
	{
	case 1:
		config_json_path = "config.json";
		break;
	case 2:
		config_json_path = argv[1];
		break;
	default:
		std::cout << "The number of arguments is invalid." << std::endl;
		return static_cast<int32_t>(ExitCode::Error);
	}

	/* Validate First Config Node */
	std::ifstream ifs(config_json_path.c_str());
	if (!ifs.is_open())
	{
		std::cout << config_json_path.c_str() << " is not found." << std::endl;
		return static_cast<int32_t>(ExitCode::Error);
	}
	rapidjson::IStreamWrapper isw(ifs);
	rapidjson::Document json_doc;
	json_doc.ParseStream(isw);

	const rapidjson::Value& edit_config = json_doc["EditConfig"];
	if (!edit_config.IsArray())
	{
		std::cout << "EditConfig is invalid." << std::endl;
		return static_cast<int32_t>(ExitCode::Error);
	}

	/* Parse vcxproj as xml */
	for (rapidjson::SizeType config_i = 0; config_i < edit_config.Size(); config_i++)
	{
		if (edit_config[config_i].ObjectEmpty())
		{
			continue;
		}
		const std::string vcxproj_path = edit_config[config_i]["vcxproj_path"].GetString();
		if (vcxproj_path.empty())
		{
			std::cout << "vcxproj_path is not defined." << std::endl;
			continue;
		}

		const rapidjson::Value& vcxproj_item = edit_config[config_i]["vcxproj_item"];
		if (!vcxproj_item.IsArray())
		{
			std::cout << "vcxproj_item is invalid." << std::endl;
			continue;
		}

		bool is_edited = false;

		rapidxml::file<> xml_file(vcxproj_path.c_str());
		rapidxml::xml_document<> xml_doc;
		/* Ref: https://stackoverflow.com/questions/15054771/c-rapidxml-edit-values-in-the-xml-file */
		xml_doc.parse<rapidxml::parse_full | rapidxml::parse_no_data_nodes>(xml_file.data());

		rapidxml::xml_node <> * target_node = nullptr;
		rapidxml::xml_node <> * pre_target_node = nullptr;

		// 編集対象項目数分ループ
		for (rapidjson::SizeType item_i = 0; item_i < vcxproj_item.Size(); item_i++)
		{
			if (vcxproj_item[item_i].ObjectEmpty())
			{
				continue;
			}

			if (!vcxproj_item[item_i].HasMember("NodePath") && !vcxproj_item[item_i]["NodePath"].IsString())
			{
				continue;
			}
			const auto node_path = vcxproj_item[item_i]["NodePath"].GetString();
			const auto node_nest = Split(node_path, '/');

			if (!vcxproj_item[item_i].HasMember("Node") && !vcxproj_item[item_i]["Node"].IsObject())
			{
				continue;
			}

			for (const auto node_name : node_nest)
			{
				pre_target_node = target_node;
				// ルートノード
				if (node_name == node_nest.front())
				{
					target_node = xml_doc.first_node(node_name.c_str());
					continue;
				}

				bool break_flag = false;
				for (rapidxml::xml_node <> * tmp_node = target_node->first_node(node_name.c_str()); tmp_node; tmp_node = tmp_node->next_sibling(node_name.c_str()))
				{
					dbg_.emplace_back(tmp_node->name());
					if (vcxproj_item[item_i]["Node"][node_name.c_str()].IsNull())
					{
						target_node = tmp_node;
						break;
					}
					if (vcxproj_item[item_i]["Node"][node_name.c_str()].HasMember("Attribute"))
					{
						for (auto & node_attr : vcxproj_item[item_i]["Node"][node_name.c_str()]["Attribute"].GetObject())
						{
							for (rapidxml::xml_attribute <> * attr = tmp_node->first_attribute(node_attr.name.GetString()); attr; attr = attr->next_attribute(node_attr.name.GetString()))
							{
								if (std::string(attr->value()) == std::string(node_attr.value.GetString()))
								{
									target_node = tmp_node;
									break_flag = true;
								}
								if (break_flag) break;
							}
							if (break_flag) break;
						}
						if (break_flag) break;
					}

					// 編集対象
					if (node_name == node_nest.back())
					{
						if (vcxproj_item[item_i]["Node"][node_name.c_str()].HasMember("Value") &&
							vcxproj_item[item_i]["Node"][node_name.c_str()]["Value"].IsString() &&
							node_name == std::string(tmp_node->name()))
						{
							tmp_node->value(vcxproj_item[item_i]["Node"][node_name.c_str()]["Value"].GetString());
							target_node = tmp_node;
							is_edited = true;
						}
						break;
					}
				}

				// 辿れていないのでノードを新規作成
				if (pre_target_node == target_node)
				{
					// TODO: allocate_node
					std::cout << "編集対象までノードが続いていない" << std::endl;
					break;
				}
			}
		}

		if (!is_edited) continue;


		bool is_override = false;
		if (json_doc.HasMember("Override") && json_doc["Override"].IsBool())
		{
			is_override = json_doc["Override"].GetBool();
		}

		/* Ref: https://semidtor.wordpress.com/2012/12/15/readwrite-xml-file-with-rapidxml/ */
		std::string output_str;
		rapidxml::print(std::back_inserter(output_str), xml_doc);
		const std::string output_path = is_override ? vcxproj_path : vcxproj_path + "_copy";
		std::ofstream output_xml(output_path.c_str());
		output_xml << xml_doc;
		output_xml.close();
		xml_doc.clear();
	}

	return static_cast<int32_t>(exit_code);
}

std::vector<std::string> Split(const std::string & target, const char & delimiter)
{
	std::vector<std::string> result;

	std::stringstream sstream{ target };
	std::string buf;
	while (std::getline(sstream, buf, delimiter))
	{
		result.emplace_back(buf);
	}
	if (!target.empty() && target.back() == delimiter)
	{
		result.emplace_back();
	}

	return result;
}
