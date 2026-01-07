# tomport

A small CLI tool written in C++ to parse Apache Tomcat server.xml configuration files and discover all configured ports.

## Features

- **Automatic Discovery**: 
  - Searches current directory for `server.xml` or `conf/server.xml`
  - Recursively walks directories to find all server.xml files
- **Port Discovery**: Discovers all configured ports:
  - **Server ports**: The shutdown port configured in the `<Server>` element
  - **Connector ports**: HTTP, HTTPS, AJP connector ports
  - **Redirect ports**: Ports used for redirecting traffic (e.g., HTTP to HTTPS)
- **Multi-file Support**: Can process multiple server.xml files in one run
- **Simple CLI**: Easy-to-use command-line interface with optional arguments
- **Fast and Lightweight**: No external dependencies, built with C++17

## Building

### Requirements

- CMake 3.10 or higher
- C++17 compatible compiler (GCC, Clang, or MSVC)

### Build Instructions

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

This will create the `tomport` executable in the `build` directory.

### Running Tests

The project includes comprehensive unit and integration tests:

```bash
# Build and run all tests
cd build
ctest --output-on-failure

# Or run test executables directly
./build/tomport_unit_tests
./build/tomport_integration_tests
```

See [tests/README.md](tests/README.md) for more details on the test suite.

## Usage

```bash
tomport [options] [path]
```

### Options

- `-m, --mode <mode>` - Output mode: `list` (default), `simple`, or `table`
- `-h, --help` - Show help message

### Output Modes

1. **list** (default) - Hierarchical tree view showing:
   - File path (for multiple files)
   - Server port
   - Each service and its connectors with ports and redirect ports

2. **simple** - Plain minimal output:
   - Server port on first line
   - Then connector ports
   - Then redirect ports (if different from connector ports)

3. **table** - Formatted table with columns:
   - Server Port
   - Service Name
   - Connector Port
   - Redirect Port

### Path Argument

The tool supports three modes of operation:

1. **No path**: Automatically searches for `server.xml` or `conf/server.xml` in the current directory
2. **File path**: Parse a specific server.xml file
3. **Directory path**: Recursively search a directory for all server.xml files

### Examples

```bash
# Search current directory (default list mode)
./build/tomport

# Parse a specific file with simple output
./build/tomport -m simple sample-server.xml

# Recursively search with table output
./build/tomport --mode table /opt/tomcat

# Parse specific file with list mode
./build/tomport /opt/tomcat/conf/server.xml

# Show help
./build/tomport --help
```

### Output Examples

#### List Mode (default)
```
Server Port: 8005
├─ Service: Catalina
   ├─ Connector Port: 8080 (Protocol: HTTP/1.1) → Redirect: 8443
   └─ Connector Port: 8009 (Protocol: AJP/1.3) → Redirect: 8443
```

#### Simple Mode
```
8005
8080
8443
8009
```

#### Table Mode
```
Server Port    Service Name        Connector Port    Redirect Port  
--------------------------------------------------------------------
8005           Catalina            8080              8443           
               Catalina            8009              8443
```

#### Multiple Files (List Mode)
```
Found 2 server.xml file(s) in '/opt/tomcat-instances':
  - /opt/tomcat-instances/instance1/conf/server.xml
  - /opt/tomcat-instances/instance2/conf/server.xml

File: /opt/tomcat-instances/instance1/conf/server.xml
------------------------------------------------------------
Server Port: 8005
├─ Service: Catalina
   └─ Connector Port: 8080 (Protocol: HTTP/1.1) → Redirect: 8443

File: /opt/tomcat-instances/instance2/conf/server.xml
------------------------------------------------------------
Server Port: 9005
├─ Service: App2Service
   └─ Connector Port: 9080 (Protocol: HTTP/1.1) → Redirect: 9443
```

## Sample Configuration

A sample `server.xml` file is included in the repository for testing purposes.

## How It Works

The tool uses the TinyXML2 library to parse XML files. It:

1. Loads the specified `server.xml` file(s)
2. Parses the hierarchical structure:
   - `<Server>` elements with `port` attributes (shutdown port)
   - `<Service>` elements with `name` attributes
   - `<Connector>` elements with `port`, `protocol`, and `redirectPort` attributes
3. Extracts port numbers and associated information
4. Displays the results in the selected output format (list, simple, or table)

## CI/CD and Releases

This project includes GitHub Actions workflows for continuous integration and releases:

- **CI Workflow**: Automatically builds and tests the project on every push and pull request to main/master branches
- **Release Workflow**: Creates Linux release binaries when a new tag is pushed

### Creating a Release

To create a new release with compiled binaries:

1. Tag your commit with a version number:
   ```bash
   git tag v1.0.0
   git push origin v1.0.0
   ```

2. The workflow will automatically:
   - Build the project on Linux
   - Run tests
   - Package the binary with documentation
   - Create a GitHub release with the `tomport-linux-x64.tar.gz` artifact

You can also manually trigger a build from the Actions tab on GitHub.

### Downloading Releases

Pre-built Linux binaries are available in the [Releases](../../releases) section. Download and extract:

```bash
wget https://github.com/JonasGao/tomport/releases/download/v1.0.0/tomport-linux-x64.tar.gz
tar -xzf tomport-linux-x64.tar.gz
cd tomport-linux-x64
./tomport sample-server.xml
```

## License

This project uses TinyXML2, which is licensed under the zlib license.