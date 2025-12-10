#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <filesystem>
#include "tinyxml2.h"

namespace fs = std::filesystem;
using namespace tinyxml2;

struct PortInfo {
    std::string type;
    std::string port;
    std::string protocol;
    
    bool operator<(const PortInfo& other) const {
        if (port != other.port) return port < other.port;
        if (type != other.type) return type < other.type;
        return protocol < other.protocol;
    }
};

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [path]\n";
    std::cout << "\nDescription:\n";
    std::cout << "  Parses Apache Tomcat server.xml configuration files and discovers\n";
    std::cout << "  all configured ports (server ports, connector ports, redirect ports).\n";
    std::cout << "\nArguments:\n";
    std::cout << "  path    Optional. Can be:\n";
    std::cout << "          - A direct path to a server.xml file\n";
    std::cout << "          - A directory to search recursively for server.xml files\n";
    std::cout << "          - If omitted, searches current directory for server.xml or conf/server.xml\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "                                    # Search current directory\n";
    std::cout << "  " << programName << " server.xml                        # Parse specific file\n";
    std::cout << "  " << programName << " /path/to/tomcat                   # Search directory recursively\n";
    std::cout << "  " << programName << " /path/to/tomcat/conf/server.xml   # Parse specific file\n";
}

void parseServerPorts(XMLElement* element, std::set<PortInfo>& ports) {
    if (!element) return;
    
    // Check if this is a Server element
    if (std::string(element->Name()) == "Server") {
        const char* port = element->Attribute("port");
        if (port) {
            PortInfo info;
            info.type = "Server";
            info.port = port;
            info.protocol = "Shutdown";
            ports.insert(info);
        }
    }
}

void parseConnectorPorts(XMLElement* element, std::set<PortInfo>& ports) {
    if (!element) return;
    
    // Check if this is a Connector element
    if (std::string(element->Name()) == "Connector") {
        const char* port = element->Attribute("port");
        const char* protocol = element->Attribute("protocol");
        const char* redirectPort = element->Attribute("redirectPort");
        
        // Add main connector port
        if (port) {
            PortInfo info;
            info.type = "Connector";
            info.port = port;
            info.protocol = protocol ? protocol : "Unknown";
            ports.insert(info);
        }
        
        // Add redirect port if present and different from main port
        if (redirectPort && (!port || std::string(redirectPort) != std::string(port))) {
            PortInfo info;
            info.type = "Redirect";
            info.port = redirectPort;
            info.protocol = protocol ? std::string(protocol) + " redirect" : "Redirect";
            ports.insert(info);
        }
    }
}

void traverseXML(XMLElement* element, std::set<PortInfo>& ports) {
    if (!element) return;
    
    // Parse current element
    parseServerPorts(element, ports);
    parseConnectorPorts(element, ports);
    
    // Traverse children
    for (XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
        traverseXML(child, ports);
    }
}

std::vector<std::string> findServerXmlFiles(const std::string& searchPath, int maxDepth = 100) {
    std::vector<std::string> xmlFiles;
    
    try {
        fs::path path(searchPath);
        
        // If path doesn't exist, return empty
        if (!fs::exists(path)) {
            return xmlFiles;
        }
        
        // If it's a file and named server.xml, return it
        if (fs::is_regular_file(path)) {
            if (path.filename() == "server.xml") {
                xmlFiles.push_back(path.string());
            }
            return xmlFiles;
        }
        
        // If it's a directory, search recursively with depth limit
        if (fs::is_directory(path)) {
            for (const auto& entry : fs::recursive_directory_iterator(
                path, 
                fs::directory_options::skip_permission_denied | fs::directory_options::follow_directory_symlink)) {
                try {
                    // Check depth to prevent excessive recursion
                    int depth = std::distance(path.begin(), entry.path().begin());
                    if (depth > maxDepth) {
                        continue;
                    }
                    
                    if (entry.is_regular_file() && entry.path().filename() == "server.xml") {
                        xmlFiles.push_back(entry.path().string());
                    }
                } catch (const fs::filesystem_error& e) {
                    // Log skipped files in verbose mode (for now, silently skip)
                    continue;
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error accessing path '" << searchPath << "': " << e.what() << "\n";
    }
    
    return xmlFiles;
}

std::vector<std::string> findServerXmlInCurrentDir() {
    std::vector<std::string> xmlFiles;
    
    // Check for server.xml in current directory
    if (fs::exists("server.xml") && fs::is_regular_file("server.xml")) {
        xmlFiles.push_back("server.xml");
    }
    
    // Check for conf/server.xml
    if (fs::exists("conf/server.xml") && fs::is_regular_file("conf/server.xml")) {
        xmlFiles.push_back("conf/server.xml");
    }
    
    return xmlFiles;
}

bool parseServerXmlFile(const std::string& xmlFilePath, std::set<PortInfo>& ports) {
    XMLDocument doc;
    XMLError error = doc.LoadFile(xmlFilePath.c_str());
    
    if (error != XML_SUCCESS) {
        std::cerr << "Error: Failed to load or parse file '" << xmlFilePath << "'\n";
        std::cerr << "Error code: " << error << "\n";
        return false;
    }
    
    XMLElement* root = doc.RootElement();
    if (!root) {
        std::cerr << "Error: No root element found in XML file '" << xmlFilePath << "'\n";
        return false;
    }
    
    traverseXML(root, ports);
    return true;
}

int main(int argc, char* argv[]) {
    std::vector<std::string> xmlFiles;
    
    // Determine which files to parse
    if (argc == 1) {
        // No arguments - search current directory for server.xml or conf/server.xml
        xmlFiles = findServerXmlInCurrentDir();
        
        if (xmlFiles.empty()) {
            std::cerr << "Error: No server.xml or conf/server.xml found in current directory.\n";
            std::cerr << "\nSuggestions:\n";
            std::cerr << "  - Provide a specific file path: " << argv[0] << " /path/to/server.xml\n";
            std::cerr << "  - Search a directory recursively: " << argv[0] << " /path/to/tomcat\n";
            std::cerr << "  - Run with -h or --help for more information\n";
            return 1;
        }
        
        std::cout << "Found " << xmlFiles.size() << " server.xml file(s) in current directory:\n";
        for (const auto& file : xmlFiles) {
            std::cout << "  - " << file << "\n";
        }
        std::cout << "\n";
    } else if (argc == 2) {
        std::string arg = argv[1];
        
        // Check for help flags
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        
        // Check if argument is a file or directory
        if (fs::exists(arg)) {
            if (fs::is_regular_file(arg)) {
                // Direct file path
                xmlFiles.push_back(arg);
            } else if (fs::is_directory(arg)) {
                // Directory - search recursively
                xmlFiles = findServerXmlFiles(arg);
                
                if (xmlFiles.empty()) {
                    std::cerr << "Error: No server.xml files found in directory '" << arg << "'.\n";
                    return 1;
                }
                
                std::cout << "Found " << xmlFiles.size() << " server.xml file(s) in '" << arg << "':\n";
                for (const auto& file : xmlFiles) {
                    std::cout << "  - " << file << "\n";
                }
                std::cout << "\n";
            }
        } else {
            std::cerr << "Error: Path '" << arg << "' does not exist.\n";
            return 1;
        }
    } else {
        printUsage(argv[0]);
        return 1;
    }
    
    // Parse all found XML files
    std::set<PortInfo> allPorts;
    int filesProcessed = 0;
    int filesWithErrors = 0;
    
    for (const auto& xmlFile : xmlFiles) {
        std::set<PortInfo> ports;
        
        if (xmlFiles.size() > 1) {
            std::cout << "Parsing: " << xmlFile << "\n";
            std::cout << std::string(60, '-') << "\n";
        }
        
        if (parseServerXmlFile(xmlFile, ports)) {
            filesProcessed++;
            
            if (ports.empty()) {
                std::cout << "No ports found in this configuration file.\n";
            } else {
                std::cout << "Tomcat Ports Configuration:\n";
                std::cout << "===========================\n\n";
                
                for (const auto& portInfo : ports) {
                    std::cout << "Port: " << portInfo.port << "\n";
                    std::cout << "  Type: " << portInfo.type << "\n";
                    std::cout << "  Protocol: " << portInfo.protocol << "\n";
                    std::cout << "\n";
                    
                    // Add to all ports collection
                    allPorts.insert(portInfo);
                }
                
                std::cout << "Total ports in this file: " << ports.size() << "\n";
            }
        } else {
            filesWithErrors++;
        }
        
        if (xmlFiles.size() > 1) {
            std::cout << "\n";
        }
    }
    
    // Summary if multiple files
    if (xmlFiles.size() > 1) {
        std::cout << std::string(60, '=') << "\n";
        std::cout << "Summary:\n";
        std::cout << "  Files processed: " << filesProcessed << "/" << xmlFiles.size() << "\n";
        if (filesWithErrors > 0) {
            std::cout << "  Files with errors: " << filesWithErrors << "\n";
        }
        std::cout << "  Unique ports found across all files: " << allPorts.size() << "\n";
    }
    
    return filesWithErrors > 0 ? 1 : 0;
}
