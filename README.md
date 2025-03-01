# http-client-c
 HTTP Client in C

Overview
This project is a simple HTTP client written in C using sockets. The client constructs an HTTP request, sends it to a web server, receives the response, and prints it to the terminal. It supports:

- Basic GET requests
- Handling optional query parameters (-r)
- Handling HTTP 3XX redirections (up to 5 times)
- Parsing and validating HTTP URLs

Features:
✔ Parses URLs and extracts hostname, port, and path
✔ Constructs a valid HTTP/1.1 GET request
✔ Establishes a TCP connection using sockets
✔ Reads and prints the HTTP response
✔ Supports query parameters with the -r flag
✔ Follows redirections (HTTP 3XX) with a Location header
✔ Error handling for incorrect input, socket failures, and invalid responses

Files Overview:
- client.c            -> Main C program
- README.txt         -> Project documentation
- EX2-wLocation.pdf  -> Assignment instructions

How to Compile and Run:

Prerequisites:
- Linux/macOS terminal (or MinGW for Windows users)
- GCC compiler installed

Compilation:
gcc -Wall -o client client.c

Running the Client:
./client [-r n <param1=value1 param2=value2 …>] <URL>

Command-Line Arguments:
- -r n <param1=value1 param2=value2 ...>  -> (Optional) Specifies n key-value parameters to append as a query string
- <URL>  -> The target URL (must start with http://)

Examples:
1️⃣ Basic GET request:
./client http://httpbin.org/get

Generated HTTP request:
GET /get HTTP/1.1
Host: httpbin.org
Connection: close

2️⃣ GET request with parameters:
./client -r 2 name=John age=30 http://httpbin.org/get

Generated HTTP request:
GET /get?name=John&age=30 HTTP/1.1
Host: httpbin.org
Connection: close

3️⃣ Handling Redirections: (Automatically follows HTTP 3XX responses)
./client http://bit.ly/example

Error Handling:
The client gracefully handles incorrect input:

❌ Invalid URL format:
./client nothttp://example.com

✅ Output:
Usage: client [-r n <param1=value1 param2=value2 …>] <URL>

❌ Missing parameter after -r:
./client -r http://httpbin.org/get

✅ Output:
Usage: client [-r n <param1=value1 param2=value2 …>] <URL>

Testing:
You can test the client using these public HTTP services:
- httpbin.org (Provides various test endpoints)
- Any custom HTTP server

Future Improvements:
- Add HTTPS support (currently only supports HTTP)
- Allow custom HTTP headers
- Implement POST and other request methods

License:
This project is licensed under the MIT License.

Happy coding! 🚀

