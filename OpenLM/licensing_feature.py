import xml.etree.ElementTree as ET
import os
import argparse

def extract_device_names(file_paths):
    device_names = []

    for file_path in file_paths:
        if not os.path.exists(file_path):
            print(f"Warning: File not found - {file_path}")
            continue

        if file_path.endswith('.xml'):
            tree = ET.parse(file_path)
            root = tree.getroot()
            for device in root.findall('device'):
                name = device.get('name')
                # Prepend 'RS' if name starts with an integer
                if name[0].isdigit():
                    name = 'RS' + name
                name = name.replace("-", "_")
                if name not in device_names:
                    device_names.append(name)
        elif file_path.endswith('.txt'):
            with open(file_path, 'r') as file:
                for line in file:
                    name = line.strip()
                    name = name.replace("-", "_")
                    if name and name not in device_names:
                        device_names.append(name)
    
    return device_names

def create_cpp_header(device_names, header_file_path):
    if os.path.exists(header_file_path):
        os.remove(header_file_path)
    with open(header_file_path, 'w') as file:
        file.write("#pragma once\n\n")
        file.write("#include <iostream>\n")
        file.write("#include <vector>\n")
        file.write("#include <map>\n")
        file.write("#include <cstdlib>\n")
        file.write("#include <unordered_map>\n")
        file.write("#include <licensecc/licensecc.h>\n")
        file.write("#include <fstream>\n")
        file.write("#include <string.h>\n")
        file.write("#include <iomanip>\n")
        file.write("#include <sstream>\n")
        file.write("#include <string>\n")
        file.write("using namespace std;\n\n")
        file.write("class License_Manager {\n")
        file.write("public:\n\n")
        file.write("    enum class LicensedProductName {\n")
        for name in device_names:
            file.write(f"        {name},\n")
        file.write(f"        DE,\n")    
        file.write("    };\n\n")
        file.write("    map<License_Manager::LicensedProductName, string> licensedProductNameMap = {\n")
        for i, name in enumerate(device_names):              
            enum_name = 'RS' + name if name[0].isdigit() else name  # Prepend 'RS' if name starts with a digit
            file.write(f"        {{License_Manager::LicensedProductName::{enum_name}, \"{name}\"}},\n")
        file.write(f"        {{License_Manager::LicensedProductName::DE, \"de\"}}\n")
        file.write("    };\n\n")      
        file.write("    License_Manager(LicensedProductName licensedProductName);\n")
        file.write("    License_Manager(string licensedProductName);\n")
        file.write("    bool licenseCheckout(const string &productName);\n")
        file.write("    const vector<string> split_string(const string& stringToBeSplit, const char splitchar);\n")
        file.write("    struct LicenseFatalException : public exception {\n")
        file.write("        const char *what() const throw() {\n")
        file.write("            return \"License was not acquired due to a fatal error\";\n")
        file.write("        }\n")
        file.write("    };\n\n")
        file.write("    struct LicenseCorrectableException : public exception {\n")
        file.write("        const char *what() const throw() {\n")
        file.write("            return \"License was not acquired due to a correctable error\";\n")
        file.write("        }\n")
        file.write("    };\n\n")
        file.write("    map<string, LicensedProductName> licensedProductNameEnumMap = {\n")
        for i, name in enumerate(device_names):              
            enum_name = 'RS' + name if name[0].isdigit() else name  # Prepend 'RS' if name starts with a digit
            separator = "," if i < len(device_names) - 1 else ""
            file.write(f"        {{\"{name}\", License_Manager::LicensedProductName::{enum_name}}}{separator}\n")       
        file.write("    };\n\n")
        file.write("    ~License_Manager();\n")
        file.write("private:\n")
        file.write("    License_Manager();\n")
        file.write("};\n")

def main ():
    parser = argparse.ArgumentParser()
    parser.add_argument('--xml_filepath',
                        type=str,default='',metavar='',required=True,help='Path for the device.xml which will be used', dest="XML_filepath")
    parser.add_argument('--feature_txt_path',
                        type=str,default='1',metavar='',help='Path of txt file having other feature list', dest="features_txt")
    args = parser.parse_args()
    print("XML file that will be update:\n\t\t",args.XML_filepath)
    print("features.txt file path:\n\t\t",args.features_txt)
    file_paths = [args.XML_filepath, args.features_txt]  # Replace with actual paths
    header_file_path = 'License_manager.hpp'  # Path to the header file
    device_names = extract_device_names(file_paths)
    create_cpp_header(device_names, header_file_path)
    print(f"Header file created at {header_file_path}")

if __name__ == "__main__":
    main()




