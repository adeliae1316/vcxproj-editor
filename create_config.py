import sys
import json
import subprocess
from copy import copy

# 作成と実行を同時に行うにはvcxproj_editorが必要
BIN_PATH = 'vcxproj_editor'
WITH_EXEC = False
# 作成するconfigファイル名(デフォルトはconfig.json)
CONFIG_FILE_NAME = "config.json"
# vcxprojを上書きしたくない場合はFalse
OVERRIDE = True
# デバッグしたいプロジェクトを列挙
DBG_TARGET_LIST = [
    'path/to/vcxproj_00',
    'path/to/vcxproj_01',
    'path/to/vcxproj_02'
]
# 最小構成のJSON
BASE_JSON = {
    "Override" : None,
    "EditConfig" : []
}
# GenerateDebugInformationにtrueを設定する例
EDIT_CONFIG_TEMPLETE = {
    "vcxproj_path" : "",
    "vcxproj_item" : [
        {
            "NodePath" : "Project/ItemDefinitionGroup/Link/GenerateDebugInformation",
            "Node" :
            {
                "Project" : None,
                "ItemDefinitionGroup" :
                {
                    "Attribute" :
                    {
                        "Condition" : "'$(Configuration)|$(Platform)'=='Release|x64'"
                    }
                },
                "Link" : None,
                "GenerateDebugInformation" :
                {
                    "Value" : "true"
                }
            }
        }
    ]
}

def get_edit_config(path):
    edit_config = copy(EDIT_CONFIG_TEMPLETE)
    edit_config['vcxproj_path'] = path
    return edit_config

def main():
    exit_code = 0

    json_dict = copy(BASE_JSON)
    json_dict['Override'] = OVERRIDE

    for path in DBG_TARGET_LIST:
        json_dict['EditConfig'].append(get_edit_config(path))

    # print(json.dumps(json_dict, indent=4))
    config_file = open(CONFIG_FILE_NAME, 'w')
    json.dump(json_dict, config_file, indent=4)
    config_file.close()

    if WITH_EXEC:
        cmd = '{0} {1}'.format(BIN_PATH, CONFIG_FILE_NAME)
        ret = subprocess.run(cmd.split(' '))
        exit_code = ret.returncode

    sys.exit(exit_code)


if __name__ == '__main__':
    main()