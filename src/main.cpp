#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>
#include <iomanip>
#include <filesystem>
#include <algorithm>
#include "tinyxml2.h"

namespace fs = std::filesystem;
using namespace tinyxml2;

enum class OutputMode {
    LIST,
    SIMPLE,
    TABLE
};

struct ConnectorInfo {
    std::string port;
    std::string protocol;
    std::string redirectPort;
};

struct ServiceInfo {
    std::string name;
    std::vector<ConnectorInfo> connectors;
};

struct ServerConfig {
    std::string filePath;
    std::string serverPort;
    std::vector<ServiceInfo> services;
};

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
    std::cout << "Usage: " << programName << " [options] [path]\n";
    std::cout << "\nDescription:\n";
    std::cout << "  Parses Apache Tomcat server.xml configuration files and discovers\n";
    std::cout << "  all configured ports (server ports, connector ports, redirect ports).\n";
    std::cout << "\nOptions:\n";
    std::cout << "  -m, --mode <mode>  Output mode: list (default), simple, or table\n";
    std::cout << "  -h, --help         Show this help message\n";
    std::cout << "\nOutput Modes:\n";
    std::cout << "  list    Hierarchical tree with file path, server port, services and connectors\n";
    std::cout << "  simple  Plain minimal output: server port first, then connector ports\n";
    std::cout << "  table   Formatted table with columns: Server Port, Service, Connector, Redirect\n";
    std::cout << "\nArguments:\n";
    std::cout << "  path    Optional. Can be:\n";
    std::cout << "          - A direct path to a server.xml file\n";
    std::cout << "          - A directory to search recursively for server.xml files\n";
    std::cout << "          - If omitted, searches current directory for server.xml or conf/server.xml\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << programName << "                                    # Search current directory\n";
    std::cout << "  " << programName << " -m simple server.xml              # Simple output mode\n";
    std::cout << "  " << programName << " --mode table /path/to/tomcat      # Table output mode\n";
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
            fs::path basePath = fs::absolute(path);
            size_t baseDepth = std::distance(basePath.begin(), basePath.end());
            
            // By default, recursive_directory_iterator does not follow symlinks (avoids loops)
            // We only skip permission denied to continue traversal
            for (const auto& entry : fs::recursive_directory_iterator(
                path, 
                fs::directory_options::skip_permission_denied)) {
                try {
                    // Check depth to prevent excessive recursion
                    fs::path currentPath = fs::absolute(entry.path());
                    size_t currentDepth = std::distance(currentPath.begin(), currentPath.end());
                    
                    // Bounds check to prevent underflow
                    if (currentDepth < baseDepth) {
                        continue;
                    }
                    
                    int relativeDepth = static_cast<int>(currentDepth - baseDepth);
                    if (relativeDepth > maxDepth) {
                        continue;
                    }
                    
                    if (entry.is_regular_file() && entry.path().filename() == "server.xml") {
                        xmlFiles.push_back(entry.path().string());
                    }
                } catch (const fs::filesystem_error&) {
                    // Skip files/directories we can't access
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

bool parseServerXmlStructured(const std::string& xmlFilePath, ServerConfig& config) {
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
    
    config.filePath = xmlFilePath;
    
    // Get server port
    if (std::string(root->Name()) == "Server") {
        const char* port = root->Attribute("port");
        if (port) {
            config.serverPort = port;
        }
    }
    
    // Parse services
    for (XMLElement* service = root->FirstChildElement("Service"); service; service = service->NextSiblingElement("Service")) {
        ServiceInfo serviceInfo;
        const char* serviceName = service->Attribute("name");
        serviceInfo.name = serviceName ? serviceName : "Unknown";
        
        // Parse connectors
        for (XMLElement* connector = service->FirstChildElement("Connector"); connector; connector = connector->NextSiblingElement("Connector")) {
            ConnectorInfo connInfo;
            const char* port = connector->Attribute("port");
            const char* protocol = connector->Attribute("protocol");
            const char* redirectPort = connector->Attribute("redirectPort");
            
            if (port) {
                connInfo.port = port;
                connInfo.protocol = protocol ? protocol : "Unknown";
                connInfo.redirectPort = redirectPort ? redirectPort : "";
                serviceInfo.connectors.push_back(connInfo);
            }
        }
        
        config.services.push_back(serviceInfo);
    }
    
    return true;
}

void outputListMode(const std::vector<ServerConfig>& configs) {
    for (const auto& config : configs) {
        if (configs.size() > 1) {
            std::cout << "File: " << config.filePath << "\n";
            std::cout << std::string(60, '-') << "\n";
        }
        
        if (!config.serverPort.empty()) {
            std::cout << "Server Port: " << config.serverPort << "\n";
        }
        
        for (const auto& service : config.services) {
            std::cout << "├─ Service: " << service.name << "\n";
            
            for (size_t i = 0; i < service.connectors.size(); ++i) {
                const auto& conn = service.connectors[i];
                bool isLast = (i == service.connectors.size() - 1);
                std::string prefix = isLast ? "   └─" : "   ├─";
                
                std::cout << prefix << " Connector Port: " << conn.port 
                         << " (Protocol: " << conn.protocol << ")";
                if (!conn.redirectPort.empty()) {
                    std::cout << " → Redirect: " << conn.redirectPort;
                }
                std::cout << "\n";
            }
        }
        
        if (configs.size() > 1) {
            std::cout << "\n";
        }
    }
}

void outputSimpleMode(const std::vector<ServerConfig>& configs) {
    for (const auto& config : configs) {
        if (configs.size() > 1) {
            std::cout << "# " << config.filePath << "\n";
        }
        
        // Track unique ports to avoid duplicates
        std::set<std::string> printedPorts;
        
        // Print server port first
        if (!config.serverPort.empty()) {
            std::cout << config.serverPort << "\n";
            printedPorts.insert(config.serverPort);
        }
        
        // Then print connector ports and redirect ports (avoiding duplicates)
        for (const auto& service : config.services) {
            for (const auto& conn : service.connectors) {
                if (printedPorts.find(conn.port) == printedPorts.end()) {
                    std::cout << conn.port << "\n";
                    printedPorts.insert(conn.port);
                }
                if (!conn.redirectPort.empty() && printedPorts.find(conn.redirectPort) == printedPorts.end()) {
                    std::cout << conn.redirectPort << "\n";
                    printedPorts.insert(conn.redirectPort);
                }
            }
        }
        
        if (configs.size() > 1) {
            std::cout << "\n";
        }
    }
}

void outputTableMode(const std::vector<ServerConfig>& configs) {
    // Header
    std::cout << std::left 
              << std::setw(15) << "Server Port"
              << std::setw(20) << "Service Name"
              << std::setw(18) << "Connector Port"
              << std::setw(15) << "Redirect Port"
              << "\n";
    std::cout << std::string(68, '-') << "\n";
    
    for (size_t configIdx = 0; configIdx < configs.size(); ++configIdx) {
        const auto& config = configs[configIdx];
        bool firstRowOfConfig = true;
        
        for (const auto& service : config.services) {
            for (const auto& conn : service.connectors) {
                std::cout << std::left
                          << std::setw(15) << (firstRowOfConfig ? config.serverPort : "")
                          << std::setw(20) << service.name
                          << std::setw(18) << conn.port
                          << std::setw(15) << (conn.redirectPort.empty() ? "-" : conn.redirectPort)
                          << "\n";
                firstRowOfConfig = false;
            }
        }
        
        // If no services, still show server port
        if (config.services.empty() && !config.serverPort.empty()) {
            std::cout << std::left
                      << std::setw(15) << config.serverPort
                      << std::setw(20) << "-"
                      << std::setw(18) << "-"
                      << std::setw(15) << "-"
                      << "\n";
        }
        
        // Add separator between configs in table mode for clarity
        if (configs.size() > 1 && configIdx < configs.size() - 1) {
            std::cout << std::string(68, '.') << "\n";
        }
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> xmlFiles;
    OutputMode mode = OutputMode::LIST;
    std::string pathArg;
    
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-m" || arg == "--mode") {
            if (i + 1 < argc) {
                std::string modeStr = argv[++i];
                if (modeStr == "list") {
                    mode = OutputMode::LIST;
                } else if (modeStr == "simple") {
                    mode = OutputMode::SIMPLE;
                } else if (modeStr == "table") {
                    mode = OutputMode::TABLE;
                } else {
                    std::cerr << "Error: Invalid mode '" << modeStr << "'. Use: list, simple, or table\n";
                    return 1;
                }
            } else {
                std::cerr << "Error: --mode requires an argument\n";
                return 1;
            }
        } else if (arg[0] != '-') {
            // Not a flag, must be a path
            if (!pathArg.empty()) {
                std::cerr << "Warning: Multiple paths specified. Using last one: " << arg << "\n";
            }
            pathArg = arg;
        } else {
            std::cerr << "Error: Unknown option '" << arg << "'\n";
            printUsage(argv[0]);
            return 1;
        }
    }
    
    // Determine which files to parse
    if (pathArg.empty()) {
        // No path argument - search current directory for server.xml or conf/server.xml
        xmlFiles = findServerXmlInCurrentDir();
        
        if (xmlFiles.empty()) {
            std::cerr << "Error: No server.xml or conf/server.xml found in current directory.\n";
            std::cerr << "\nSuggestions:\n";
            std::cerr << "  - Provide a specific file path: " << argv[0] << " /path/to/server.xml\n";
            std::cerr << "  - Search a directory recursively: " << argv[0] << " /path/to/tomcat\n";
            std::cerr << "  - Run with -h or --help for more information\n";
            return 1;
        }
        
        if (mode == OutputMode::LIST) {
            std::cout << "Found " << xmlFiles.size() << " server.xml file(s) in current directory:\n";
            for (const auto& file : xmlFiles) {
                std::cout << "  - " << file << "\n";
            }
            std::cout << "\n";
        }
    } else {
        // Check if argument is a file or directory
        if (fs::exists(pathArg)) {
            if (fs::is_regular_file(pathArg)) {
                // Direct file path
                xmlFiles.push_back(pathArg);
            } else if (fs::is_directory(pathArg)) {
                // Directory - search recursively
                xmlFiles = findServerXmlFiles(pathArg);
                
                if (xmlFiles.empty()) {
                    std::cerr << "Error: No server.xml files found in directory '" << pathArg << "'.\n";
                    return 1;
                }
                
                if (mode == OutputMode::LIST) {
                    std::cout << "Found " << xmlFiles.size() << " server.xml file(s) in '" << pathArg << "':\n";
                    for (const auto& file : xmlFiles) {
                        std::cout << "  - " << file << "\n";
                    }
                    std::cout << "\n";
                }
            }
        } else {
            std::cerr << "Error: Path '" << pathArg << "' does not exist.\n";
            return 1;
        }
    }
    
    // Parse all found XML files
    std::vector<ServerConfig> configs;
    int filesWithErrors = 0;
    
    for (const auto& xmlFile : xmlFiles) {
        ServerConfig config;
        if (parseServerXmlStructured(xmlFile, config)) {
            configs.push_back(config);
        } else {
            filesWithErrors++;
        }
    }
    
    // Output based on mode
    if (configs.empty()) {
        std::cout << "No valid configuration files found.\n";
        return 1;
    }
    
    switch (mode) {
        case OutputMode::LIST:
            outputListMode(configs);
            break;
        case OutputMode::SIMPLE:
            outputSimpleMode(configs);
            break;
        case OutputMode::TABLE:
            outputTableMode(configs);
            break;
    }
    
    return filesWithErrors > 0 ? 1 : 0;
}
