#ifndef TOMPORT_CORE_H
#define TOMPORT_CORE_H

#include <string>
#include <vector>
#include <set>
#include <unordered_set>

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

// Core functions for testing
bool parseServerXmlStructured(const std::string& xmlFilePath, ServerConfig& config);
std::vector<std::string> findServerXmlInCurrentDir();
std::vector<std::string> findServerXmlFiles(const std::string& searchPath, int maxDepth = 100);

#endif // TOMPORT_CORE_H
