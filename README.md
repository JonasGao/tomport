# tomport

A small CLI tool written in C++ to parse Apache Tomcat server.xml configuration files and discover all configured ports.

## Features

- Parses Apache Tomcat `server.xml` configuration files
- Discovers all configured ports:
  - **Server ports**: The shutdown port configured in the `<Server>` element
  - **Connector ports**: HTTP, HTTPS, AJP connector ports
  - **Redirect ports**: Ports used for redirecting traffic (e.g., HTTP to HTTPS)
- Simple command-line interface
- Fast and lightweight with no external dependencies

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
tomport <path-to-server.xml>
```

### Example

```bash
# Using the sample configuration file
./build/tomport sample-server.xml

# Using a real Tomcat installation
./build/tomport /opt/tomcat/conf/server.xml
```

### Output Example

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

Total ports found: 4
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

## License

This project uses TinyXML2, which is licensed under the zlib license.