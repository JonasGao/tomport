#include <iostream>
#include <string>
#include <set>
#include <vector>
#include "tinyxml2.h"

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
    std::cout << "Usage: " << programName << " <path-to-server.xml>\n";
    std::cout << "\nDescription:\n";
    std::cout << "  Parses Apache Tomcat server.xml configuration files and discovers\n";
    std::cout << "  all configured ports (server ports, connector ports, redirect ports).\n";
    std::cout << "\nExample:\n";
    std::cout << "  " << programName << " /path/to/tomcat/conf/server.xml\n";
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

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string xmlFilePath = argv[1];
    
    // Load and parse XML file
    XMLDocument doc;
    XMLError error = doc.LoadFile(xmlFilePath.c_str());
    
    if (error != XML_SUCCESS) {
        std::cerr << "Error: Failed to load or parse file '" << xmlFilePath << "'\n";
        std::cerr << "Error code: " << error << "\n";
        return 1;
    }
    
    // Find all ports
    std::set<PortInfo> ports;
    XMLElement* root = doc.RootElement();
    
    if (!root) {
        std::cerr << "Error: No root element found in XML file\n";
        return 1;
    }
    
    traverseXML(root, ports);
    
    // Display results
    if (ports.empty()) {
        std::cout << "No ports found in configuration file.\n";
        return 0;
    }
    
    std::cout << "Tomcat Ports Configuration:\n";
    std::cout << "===========================\n\n";
    
    for (const auto& portInfo : ports) {
        std::cout << "Port: " << portInfo.port << "\n";
        std::cout << "  Type: " << portInfo.type << "\n";
        std::cout << "  Protocol: " << portInfo.protocol << "\n";
        std::cout << "\n";
    }
    
    std::cout << "Total ports found: " << ports.size() << "\n";
    
    return 0;
}
