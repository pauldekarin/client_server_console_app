# Client-Server Application

## Overview

This project involves implementing a client-server application using sockets in C++. Below is a detailed description of the development environment, build process, and usage of the application.

## Development Environment

- **System**: MacBook Air M1 2020
- **Operating System**: macOS Ventura 13.0

### Development Tools

- **Language**: C++17
- **IDE**: Visual Studio Code
- **Containerization**: Docker Ubuntu (latest version)

## Build Project

To build the project, navigate to the project directory and run:

```sh
make build
```

## Output
`Server Application`: server.out

`Client Application`: client.out

## Usage
### Commands
`parse()`: Parses the message from the client and returns a response containing a table with the count of different characters.

`count()`: Returns the number of current connections upon client request.

`chat()`: Initiates communication with the client through the server.

`back()`: Exits the active chat.

`id()`: Displays the client's identifier as determined by the server.

`help()`: Lists all available commands.

`exit()`: Closes the application.
