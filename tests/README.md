# Tomport Tests

This directory contains unit tests and integration tests for the tomport application.

## Test Structure

- **unit_tests.cpp**: Unit tests for core parsing functionality
  - Tests XML parsing with various configurations
  - Tests edge cases (missing ports, invalid XML, etc.)
  - Tests data structure operations

- **integration_tests.cpp**: Integration tests for end-to-end functionality
  - Tests automatic file discovery
  - Tests recursive directory search
  - Tests complex real-world configurations
  - Tests multi-file processing

## Running Tests

### Build and Run All Tests

```bash
mkdir build
cd build
cmake ..
cmake --build .
ctest --output-on-failure
```

### Run Specific Test Suite

```bash
# Run only unit tests
./build/tomport_unit_tests

# Run only integration tests
./build/tomport_integration_tests
```

### Run Specific Test

```bash
./build/tomport_unit_tests --gtest_filter=TomportUnitTest.ParseSimpleServerXml
```

## Test Coverage

The test suite includes 25 tests covering:

### Unit Tests (11 tests)
- Simple server.xml parsing
- Multiple services
- Multiple connectors
- Missing server port
- Connector without port
- Invalid XML
- Non-existent file
- PortInfo comparison
- Empty redirect port
- Unknown service name
- Unknown protocol

### Integration Tests (14 tests)
- Finding server.xml in current directory
- Finding conf/server.xml
- Finding both files
- No files found
- Recursive search (single file)
- Recursive search (multiple files)
- Nested directories
- No files in search
- Specific file path
- Non-existent path
- End-to-end multiple configs
- Depth limit testing
- Mixed valid/invalid files
- Complex real-world configuration

## Building Without Tests

To disable tests during build:

```bash
cmake -DBUILD_TESTS=OFF ..
cmake --build .
```

## Test Framework

Tests are built using [Google Test](https://github.com/google/googletest) framework, which is automatically downloaded during the CMake configuration step.
