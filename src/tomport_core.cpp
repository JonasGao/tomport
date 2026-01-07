#include "tomport_core.h"
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;
using namespace tinyxml2;

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

std::vector<std::string> findServerXmlFiles(const std::string& searchPath, int maxDepth) {
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
