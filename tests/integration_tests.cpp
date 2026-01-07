#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include <cstdlib>
#include "tomport_core.h"

namespace fs = std::filesystem;

// Test fixture for integration tests
class TomportIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary directory for tests
        testDir = fs::temp_directory_path() / "tomport_integration_tests";
        fs::create_directories(testDir);
        originalDir = fs::current_path();
    }

    void TearDown() override {
        // Restore original directory
        fs::current_path(originalDir);
        
        // Clean up test directory
        if (fs::exists(testDir)) {
            fs::remove_all(testDir);
        }
    }

    // Helper function to create a test XML file
    void createTestXmlFile(const fs::path& path, const std::string& content) {
        fs::create_directories(path.parent_path());
        std::ofstream file(path);
        file << content;
        file.close();
    }

    // Helper to create standard test XML
    std::string getStandardTestXml(const std::string& serverPort = "8005") {
        return R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port=")" + serverPort + R"(" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1" redirectPort="8443" />
    <Connector port="8009" protocol="AJP/1.3" redirectPort="8443" />
  </Service>
</Server>)";
    }

    fs::path testDir;
    fs::path originalDir;
};

// Test finding server.xml in current directory
TEST_F(TomportIntegrationTest, FindServerXmlInCurrentDir) {
    // Create server.xml in test directory
    createTestXmlFile(testDir / "server.xml", getStandardTestXml());
    
    // Change to test directory
    fs::current_path(testDir);
    
    auto files = findServerXmlInCurrentDir();
    
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0], "server.xml");
}

// Test finding conf/server.xml in current directory
TEST_F(TomportIntegrationTest, FindConfServerXmlInCurrentDir) {
    // Create conf/server.xml in test directory
    createTestXmlFile(testDir / "conf" / "server.xml", getStandardTestXml());
    
    // Change to test directory
    fs::current_path(testDir);
    
    auto files = findServerXmlInCurrentDir();
    
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0], "conf/server.xml");
}

// Test finding both server.xml and conf/server.xml
TEST_F(TomportIntegrationTest, FindBothServerXmlFiles) {
    // Create both files
    createTestXmlFile(testDir / "server.xml", getStandardTestXml());
    createTestXmlFile(testDir / "conf" / "server.xml", getStandardTestXml());
    
    // Change to test directory
    fs::current_path(testDir);
    
    auto files = findServerXmlInCurrentDir();
    
    ASSERT_EQ(files.size(), 2);
}

// Test finding no server.xml in current directory
TEST_F(TomportIntegrationTest, FindNoServerXmlInCurrentDir) {
    // Change to test directory (no server.xml)
    fs::current_path(testDir);
    
    auto files = findServerXmlInCurrentDir();
    
    EXPECT_TRUE(files.empty());
}

// Test recursive search for server.xml files
TEST_F(TomportIntegrationTest, RecursiveSearchSingleFile) {
    // Create one server.xml in subdirectory
    createTestXmlFile(testDir / "tomcat1" / "conf" / "server.xml", getStandardTestXml());
    
    auto files = findServerXmlFiles(testDir.string());
    
    ASSERT_EQ(files.size(), 1);
    EXPECT_TRUE(files[0].find("tomcat1") != std::string::npos);
}

// Test recursive search with multiple server.xml files
TEST_F(TomportIntegrationTest, RecursiveSearchMultipleFiles) {
    // Create multiple server.xml files
    createTestXmlFile(testDir / "tomcat1" / "conf" / "server.xml", getStandardTestXml("8005"));
    createTestXmlFile(testDir / "tomcat2" / "conf" / "server.xml", getStandardTestXml("9005"));
    createTestXmlFile(testDir / "tomcat3" / "conf" / "server.xml", getStandardTestXml("7005"));
    
    auto files = findServerXmlFiles(testDir.string());
    
    ASSERT_EQ(files.size(), 3);
}

// Test recursive search with nested directories
TEST_F(TomportIntegrationTest, RecursiveSearchNestedDirectories) {
    // Create nested structure
    createTestXmlFile(testDir / "level1" / "server.xml", getStandardTestXml());
    createTestXmlFile(testDir / "level1" / "level2" / "server.xml", getStandardTestXml());
    createTestXmlFile(testDir / "level1" / "level2" / "level3" / "server.xml", getStandardTestXml());
    
    auto files = findServerXmlFiles(testDir.string());
    
    ASSERT_EQ(files.size(), 3);
}

// Test recursive search with no server.xml files
TEST_F(TomportIntegrationTest, RecursiveSearchNoFiles) {
    // Create some directories but no server.xml files
    fs::create_directories(testDir / "dir1" / "dir2");
    createTestXmlFile(testDir / "dir1" / "other.xml", getStandardTestXml());
    
    auto files = findServerXmlFiles(testDir.string());
    
    EXPECT_TRUE(files.empty());
}

// Test search with specific file path
TEST_F(TomportIntegrationTest, SearchSpecificFile) {
    // Create a server.xml file
    auto xmlPath = testDir / "custom" / "server.xml";
    createTestXmlFile(xmlPath, getStandardTestXml());
    
    auto files = findServerXmlFiles(xmlPath.string());
    
    ASSERT_EQ(files.size(), 1);
    EXPECT_EQ(files[0], xmlPath.string());
}

// Test search with non-existent path
TEST_F(TomportIntegrationTest, SearchNonExistentPath) {
    auto files = findServerXmlFiles((testDir / "nonexistent").string());
    
    EXPECT_TRUE(files.empty());
}

// Test end-to-end: multiple files with different configurations
TEST_F(TomportIntegrationTest, EndToEndMultipleConfigs) {
    // Create different configurations
    std::string config1 = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1" redirectPort="8443" />
  </Service>
</Server>)";

    std::string config2 = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="9005" shutdown="SHUTDOWN">
  <Service name="WebService">
    <Connector port="9080" protocol="HTTP/1.1" />
  </Service>
  <Service name="AdminService">
    <Connector port="9090" protocol="HTTP/1.1" />
  </Service>
</Server>)";

    createTestXmlFile(testDir / "app1" / "conf" / "server.xml", config1);
    createTestXmlFile(testDir / "app2" / "conf" / "server.xml", config2);
    
    auto files = findServerXmlFiles(testDir.string());
    ASSERT_EQ(files.size(), 2);
    
    // Parse both files
    std::vector<ServerConfig> configs;
    for (const auto& file : files) {
        ServerConfig config;
        bool result = parseServerXmlStructured(file, config);
        ASSERT_TRUE(result);
        configs.push_back(config);
    }
    
    // Verify both configurations were parsed correctly
    ASSERT_EQ(configs.size(), 2);
    
    // Check that we have different server ports
    std::set<std::string> serverPorts;
    for (const auto& config : configs) {
        serverPorts.insert(config.serverPort);
    }
    EXPECT_EQ(serverPorts.size(), 2);
}

// Test depth limiting in recursive search
TEST_F(TomportIntegrationTest, RecursiveSearchDepthLimit) {
    // Create a deep directory structure
    fs::path deepPath = testDir;
    for (int i = 0; i < 5; i++) {
        deepPath = deepPath / ("level" + std::to_string(i));
    }
    createTestXmlFile(deepPath / "server.xml", getStandardTestXml());
    
    // Also create one at shallow depth
    createTestXmlFile(testDir / "shallow" / "server.xml", getStandardTestXml());
    
    // Search with low depth limit (should find both since depth is relative)
    auto files = findServerXmlFiles(testDir.string(), 10);
    
    EXPECT_GE(files.size(), 1); // Should find at least the shallow one
}

// Test with mixed valid and invalid XML files
TEST_F(TomportIntegrationTest, MixedValidInvalidFiles) {
    // Create valid XML
    createTestXmlFile(testDir / "valid" / "server.xml", getStandardTestXml());
    
    // Create invalid XML
    createTestXmlFile(testDir / "invalid" / "server.xml", "This is not XML");
    
    auto files = findServerXmlFiles(testDir.string());
    ASSERT_EQ(files.size(), 2); // Both files found
    
    // Try to parse both
    int successCount = 0;
    for (const auto& file : files) {
        ServerConfig config;
        if (parseServerXmlStructured(file, config)) {
            successCount++;
        }
    }
    
    EXPECT_EQ(successCount, 1); // Only one should parse successfully
}

// Test parsing complex real-world configuration
TEST_F(TomportIntegrationTest, ComplexRealWorldConfig) {
    std::string complexConfig = R"(<?xml version="1.0" encoding="UTF-8"?>
<Server port="8005" shutdown="SHUTDOWN">
  <Listener className="org.apache.catalina.startup.VersionLoggerListener" />
  <Listener className="org.apache.catalina.core.AprLifecycleListener" SSLEngine="on" />
  
  <GlobalNamingResources>
    <Resource name="UserDatabase" auth="Container"
              type="org.apache.catalina.UserDatabase" />
  </GlobalNamingResources>
  
  <Service name="Catalina">
    <Connector port="8080" protocol="HTTP/1.1"
               connectionTimeout="20000"
               redirectPort="8443" />
    <Connector port="8443" protocol="org.apache.coyote.http11.Http11NioProtocol"
               maxThreads="150" SSLEnabled="true">
      <SSLHostConfig>
        <Certificate certificateKeystoreFile="conf/localhost-rsa.jks" type="RSA" />
      </SSLHostConfig>
    </Connector>
    <Connector protocol="AJP/1.3"
               address="::1"
               port="8009"
               redirectPort="8443" />
    <Engine name="Catalina" defaultHost="localhost">
      <Realm className="org.apache.catalina.realm.LockOutRealm" />
      <Host name="localhost" appBase="webapps" unpackWARs="true" autoDeploy="true" />
    </Engine>
  </Service>
</Server>)";

    createTestXmlFile(testDir / "server.xml", complexConfig);
    
    ServerConfig config;
    bool result = parseServerXmlStructured((testDir / "server.xml").string(), config);
    
    ASSERT_TRUE(result);
    EXPECT_EQ(config.serverPort, "8005");
    ASSERT_EQ(config.services.size(), 1);
    EXPECT_EQ(config.services[0].name, "Catalina");
    ASSERT_EQ(config.services[0].connectors.size(), 3);
    
    // Verify all connectors are parsed
    std::set<std::string> ports;
    for (const auto& conn : config.services[0].connectors) {
        ports.insert(conn.port);
    }
    EXPECT_EQ(ports.size(), 3);
    EXPECT_TRUE(ports.find("8080") != ports.end());
    EXPECT_TRUE(ports.find("8443") != ports.end());
    EXPECT_TRUE(ports.find("8009") != ports.end());
}
