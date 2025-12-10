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

## Usage

```bash
tomport [path]
```

The tool supports three modes of operation:

1. **No arguments**: Automatically searches for `server.xml` or `conf/server.xml` in the current directory
2. **File path**: Parse a specific server.xml file
3. **Directory path**: Recursively search a directory for all server.xml files

### Examples

```bash
# Search current directory for server.xml or conf/server.xml
./build/tomport

# Parse a specific file
./build/tomport sample-server.xml
./build/tomport /opt/tomcat/conf/server.xml

# Recursively search a directory for all server.xml files
./build/tomport /opt/tomcat
./build/tomport /var/lib/tomcat-instances

# Show help
./build/tomport --help
```

### Output Examples

**Single file:**
```
Tomcat Ports Configuration:
===========================

Port: 8005
  Type: Server
  Protocol: Shutdown

Port: 8009
  Type: Connector
  Protocol: AJP/1.3

Port: 8080
  Type: Connector
  Protocol: HTTP/1.1

Port: 8443
  Type: Redirect
  Protocol: HTTP/1.1 redirect

Total ports in this file: 4
```

**Multiple files (recursive search):**
```
Found 2 server.xml file(s) in '/opt/tomcat-instances':
  - /opt/tomcat-instances/instance1/conf/server.xml
  - /opt/tomcat-instances/instance2/conf/server.xml

Parsing: /opt/tomcat-instances/instance1/conf/server.xml
------------------------------------------------------------
Tomcat Ports Configuration:
===========================

Port: 8005
  Type: Server
  Protocol: Shutdown

Port: 8080
  Type: Connector
  Protocol: HTTP/1.1

Total ports in this file: 2

Parsing: /opt/tomcat-instances/instance2/conf/server.xml
------------------------------------------------------------
Tomcat Ports Configuration:
===========================

Port: 9005
  Type: Server
  Protocol: Shutdown

Port: 9080
  Type: Connector
  Protocol: HTTP/1.1

Total ports in this file: 2

============================================================
Summary:
  Files processed: 2/2
  Unique ports found across all files: 4
```

## Sample Configuration

A sample `server.xml` file is included in the repository for testing purposes.

## How It Works

The tool uses the TinyXML2 library to parse XML files. It:

1. Loads the specified `server.xml` file
2. Traverses the XML tree to find:
   - `<Server>` elements with `port` attributes
   - `<Connector>` elements with `port` and `redirectPort` attributes
3. Extracts port numbers and associated protocol information
4. Displays the results in an organized format

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