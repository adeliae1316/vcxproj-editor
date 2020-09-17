import sys
import json
import xml.etree.ElementTree as ET

def load_config_json():
    try:
        config_json = open('config.json', 'r')
    except Exception as e:
        print('Faild to open config.json.')
        return None

    try:
        ret = json.load(config_json)
    except Exception as e:
        print('Faild to load json. Check json syntax.')
        return None

    config_json.close()
    return ret

def edit_xml(path, config):
    print('{}'.format(path[path.rfind('/')+1:path.rfind('.')]))
    # namespaceを取得
    ns = config['vcxproj_ns']
    # 編集項目リストを取得
    edit_config = config['edit_config']
    # write時のnamespaceを空に設定
    ET.register_namespace('', ns)
    namespaces = {'' : ns}
    tree = ET.parse(path)
    root = tree.getroot()

    if root.tag.find(config['vcxproj_root']) == -1:
        root_tag_without_ns = root.tag[root.tag.rfind('}')+1:]
        print('  RootNode "{0}" does not matche vcxproj_root "{1}" on config.'.format(root_tag_without_ns, config['vcxproj_root']))
        return 1

    for item in edit_config:
        node_path = '.'
        for node_name in item['NodePath'].split('/'):
            node_path += '/{{{0}}}{1}'.format(ns, node_name)
        for elem in root.findall(node_path, namespaces=namespaces):
            print('  {}'.format(item['NodePath']))
            before_text = elem.text
            if len(before_text) == 0:
                before_text = 'None'
            elem.text = item['Value']
            print('    {0} -> {1}'.format(before_text, elem.text))

    tree.write(path)
    return 0

def main():
    exit_code = 0

    config = load_config_json()

    if config == None:
        exit_code = 1
        sys.exit(exit_code)

    for path in config['vcxproj_path']:
        exit_code += edit_xml(path, config)

    sys.exit(exit_code)

if __name__ == '__main__':
    main()
