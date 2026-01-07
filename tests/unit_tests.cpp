#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "tomport_core.h"
#include "tinyxml2.h"

namespace fs = std::filesystem;
using namespace tinyxml2;

// Test fixture for unit tests
class TomportUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for tests
        testDir = fs::temp_directory_path() / "tomport_unit_tests";
        fs::create_directories(testDir);
    }

    void TearDown() override {
        // Clean up test directory
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    // Helper function to create a test XML file
    void createTestXmlFile(const std::string& filename, const std::string& content) {
        std::ofstream file(testDir / filename);
        file << content;
        file.close();
    }

    fs::path testDir;
};

// Test parsing a simple server.xml with one service and connector
TEST_F(TomportUnitTest, ParseSimpleServerXml) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1" redirectPort="8443" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(config.serverPort, "8005");
    ASSERT_EQ(config.services.size(), 1);
    EXPECT_EQ(config.services[0].name, "Catalina");
    ASSERT_EQ(config.services[0].connectors.size(), 1);
    EXPECT_EQ(config.services[0].connectors[0].port, "8080");
    EXPECT_EQ(config.services[0].connectors[0].protocol, "HTTP/1.1");
    EXPECT_EQ(config.services[0].connectors[0].redirectPort, "8443");
}

// Test parsing with multiple services
TEST_F(TomportUnitTest, ParseMultipleServices) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="9005" shutdown="SHUTDOWN">
  <Service name="Service1">
    <Connector port="9080" protocol="HTTP/1.1" />
  </Service>
  <Service name="Service2">
    <Connector port="9090" protocol="HTTP/1.1" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(config.serverPort, "9005");
    ASSERT_EQ(config.services.size(), 2);
    EXPECT_EQ(config.services[0].name, "Service1");
    EXPECT_EQ(config.services[1].name, "Service2");
}

// Test parsing with multiple connectors in one service
TEST_F(TomportUnitTest, ParseMultipleConnectors) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1" redirectPort="8443" />
    <Connector port="8009" protocol="AJP/1.3" redirectPort="8443" />
    <Connector port="8443" protocol="HTTP/1.1" SSLEnabled="true" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    ASSERT_EQ(config.services[0].connectors.size(), 3);
    EXPECT_EQ(config.services[0].connectors[0].port, "8080");
    EXPECT_EQ(config.services[0].connectors[1].port, "8009");
    EXPECT_EQ(config.services[0].connectors[2].port, "8443");
}

// Test parsing with missing server port
TEST_F(TomportUnitTest, ParseMissingServerPort) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    EXPECT_TRUE(config.serverPort.empty());
}

// Test parsing with connector without port (should be skipped)
TEST_F(TomportUnitTest, ParseConnectorWithoutPort) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector protocol="HTTP/1.1" redirectPort="8443" />
    <Connector port="8080" protocol="HTTP/1.1" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    ASSERT_EQ(config.services[0].connectors.size(), 1);
    EXPECT_EQ(config.services[0].connectors[0].port, "8080");
}

// Test parsing invalid XML
TEST_F(TomportUnitTest, ParseInvalidXml) {
    std::string xmlContent = "This is not valid XML";
    
    createTestXmlFile("invalid.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "invalid.xml").string(), config);
    
    EXPECT_FALSE(result);
}

// Test parsing non-existent file
TEST_F(TomportUnitTest, ParseNonExistentFile) {
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "nonexistent.xml").string(), config);
    
    EXPECT_FALSE(result);
}

// Test PortInfo comparison operator
TEST_F(TomportUnitTest, PortInfoComparison) {
    PortInfo port1{"Server", "8005", "Shutdown"};
    PortInfo port2{"Connector", "8080", "HTTP/1.1"};
    PortInfo port3{"Server", "8005", "Shutdown"};
    
    EXPECT_TRUE(port1 < port2);
    EXPECT_FALSE(port2 < port1);
    EXPECT_FALSE(port1 < port3);
    EXPECT_FALSE(port3 < port1);
}

// Test parsing with empty redirectPort
TEST_F(TomportUnitTest, ParseEmptyRedirectPort) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    ASSERT_EQ(config.services[0].connectors.size(), 1);
    EXPECT_TRUE(config.services[0].connectors[0].redirectPort.empty());
}

// Test parsing with unknown service name
TEST_F(TomportUnitTest, ParseUnknownServiceName) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service>
    <Connector port="8080" protocol="HTTP/1.1" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    ASSERT_EQ(config.services.size(), 1);
    EXPECT_EQ(config.services[0].name, "Unknown");
}

// Test parsing with unknown protocol
TEST_F(TomportUnitTest, ParseUnknownProtocol) {
    std::string xmlContent = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" />
  </Service>
</Server>)";

    createTestXmlFile("server.xml", xmlContent);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    ASSERT_EQ(config.services[0].connectors.size(), 1);
    EXPECT_EQ(config.services[0].connectors[0].protocol, "Unknown");
}
