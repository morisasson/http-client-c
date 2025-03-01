#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>

// Struct to hold HTTP response data
typedef struct {
    char *headers;
    unsigned char *body;
    size_t body_size;
} HttpResponse;

// Free allocated memory for the HTTP response
void free_http_response(HttpResponse *response) {
    if (response->headers) free(response->headers);
    if (response->body) free(response->body);
    response->headers = NULL;
    response->body = NULL;
    response->body_size = 0;
}

// Check if the input arguments contain a valid HTTP URL
int check_http(int argc, char *argv[]) {
    const char *test = "http://";
    if ((strncmp(argv[1], test, strlen(test)) == 0) && argc > 2 && strcmp(argv[2], "-r") != 0)
        return 0;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], test, strlen(test)) == 0) {
            return 1;
        }
    }
    return 0;
}

// Check if the string represents a positive integer
int is_positive_integer(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

// Validate that the input is in name=value format
int is_name_value_format(const char *str) {
    return strchr(str, '=') != NULL;
}

// Validate command-line input arguments
int check_input(int argc, char *argv[]) {
    if (strcmp(argv[1], "-r") != 0 && strncmp(argv[1], "http://", 7) != 0) {
        return 0;
    }

    int found_r = 0, param_count = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            found_r = 1;

            if (i + 1 >= argc || !is_positive_integer(argv[i + 1])) {
                return 0;
            }
            param_count = atoi(argv[i + 1]);

            for (int j = 0; j < param_count; j++) {
                if (i + 2 + j >= argc || !is_name_value_format(argv[i + 2 + j])) {
                    return 0;
                }
            }

            i += 1 + param_count;

            if (i + 1 < argc) {
                if (strncmp(argv[i + 1], "http://", 7) != 0) {
                    return 0;
                }
                break;
            }
        }
    }

    if (!found_r) {
    }

    return 1;
}

// Struct to hold parsed URL components
typedef struct {
    char hostname[256];
    int port;
    char path[512];
    char params[512];
} ParsedURL;

// Parse and extract URL components
// Parse and extract URL components
void fill_parsed_url(int argc, char *argv[], ParsedURL *parsed_url) {
    memset(parsed_url, 0, sizeof(ParsedURL)); // Clear the struct
    parsed_url->port = 80; // Default port
    strcpy(parsed_url->path, "/");
    strcpy(parsed_url->params, "");

    int url_index = -1;
    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "http://", 7) == 0) {
            url_index = i;
            break;
        }
    }

    if (url_index == -1) {
        fprintf(stderr, "Error: No valid URL provided.\n");
        exit(1);
    }

    const char *url = argv[url_index];
    const char *hostname_start = url + strlen("http://");
    const char *port_start = strchr(hostname_start, ':');
    const char *path_start = strchr(hostname_start, '/');

    if (port_start && (!path_start || port_start < path_start)) {
        strncpy(parsed_url->hostname, hostname_start, port_start - hostname_start);
        parsed_url->hostname[port_start - hostname_start] = '\0';
        parsed_url->port = atoi(port_start + 1);

        if (path_start) {
            strcpy(parsed_url->path, path_start);
        }
    } else if (path_start) {
        strncpy(parsed_url->hostname, hostname_start, path_start - hostname_start);
        parsed_url->hostname[path_start - hostname_start] = '\0';
        strcpy(parsed_url->path, path_start);
    } else {
        strcpy(parsed_url->hostname, hostname_start);
    }

    // אם יש אפשרות -r עם פרמטרים, צרף אותם כ-query string
    for (int i = 1; i < url_index; i++) {
        if (strcmp(argv[i], "-r") == 0) {
            if (i + 1 < url_index) {
                int param_count = atoi(argv[i + 1]);
                if (param_count > 0 && i + 1 + param_count < url_index) {
                    char query[512] = "";
                    strcat(query, "?");
                    for (int j = 0; j < param_count; j++) {
                        strcat(query, argv[i + 2 + j]);
                        if (j < param_count - 1) {
                            strcat(query, "&");
                        }
                    }
                    strncpy(parsed_url->params, query, sizeof(parsed_url->params) - 1);
                    parsed_url->params[sizeof(parsed_url->params) - 1] = '\0';
                }
            }
            break;
        }
    }
}


// Build an HTTP GET request
void build_http_request(const ParsedURL *parsed_url, char *request, size_t request_size) {
    snprintf(request, request_size,
             "GET %s%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "\r\n",
             parsed_url->path,
             parsed_url->params,
             parsed_url->hostname);
}

// Connect to the server using socket
int connect_to_server(const ParsedURL *parsed_url) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    server = gethostbyname(parsed_url->hostname);
    if (!server) {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(parsed_url->port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error connecting to server");
        exit(1);
    }

    return sockfd;
}

// Receive and dynamically allocate memory for the HTTP response
void receive_http_response_dynamic(int sockfd, HttpResponse *response, int* total) {
    int received = 0;
    char chunk[1024];

    char *headers = NULL;
    unsigned char *body = NULL;
    int is_body = 0;

    while ((received = read(sockfd, chunk, sizeof(chunk))) > 0) {
        if (!is_body) {
            char *header_end = strstr(chunk, "\r\n\r\n");
            if (header_end) {
                int header_size = header_end - chunk + 4;
                headers = realloc(headers, *total + header_size + 1);
                memcpy(headers + *total, chunk, header_size);
                headers[*total + header_size] = '\0';
                *total += header_size;

                int body_start = header_end + 4 - chunk;
                int body_size = received - body_start;

                if (body_size > 0) {
                    body = realloc(body, response->body_size + body_size);
                    memcpy(body + response->body_size, chunk + body_start, body_size);
                    response->body_size += body_size;
                }
                is_body = 1;
            } else {
                headers = realloc(headers, *total + received);
                memcpy(headers + *total, chunk, received);
                *total += received;
            }

        } else {
            body = realloc(body, response->body_size + received);
            memcpy(body + response->body_size, chunk, received);
            response->body_size += received;
        }
    }
    response->headers = headers;
    response->body = body;
}

// Send the HTTP request to the server
void send_http_request(int sockfd, const char *request) {
    if (write(sockfd, request, strlen(request)) < 0) {
        perror("Error writing to socket");
        exit(1);
    }
}

// Extract the HTTP status code from the response
int extract_status_code(const char *response) {
    const char *status_line = strstr(response, "HTTP/1.1");
    int status_code = 0;

    if (status_line) {
        sscanf(status_line, "HTTP/1.1 %d", &status_code);
    } else {
        fprintf(stderr, "Error: Status line not found in the response.\n");
    }

    return status_code;
}

// Main function to handle arguments, make the HTTP request, and process the response
int main(int argc, char *argv[]) {
    int total = 0;
    if (argc < 2) {
        printf("Usage: client [-r n < pr1=value1 pr2=value2 …>] <URL>\n");
        return 1;
    }

    if (!check_http(argc, argv) || !check_input(argc, argv)) {
        printf("Usage: client [-r n < pr1=value1 pr2=value2 …>] <URL>\n");
        return 1;
    }

    ParsedURL parsed_url;
    fill_parsed_url(argc, argv, &parsed_url);

    char request[1024];
    build_http_request(&parsed_url, request, sizeof(request));
    printf("HTTP request =\n%s\nLEN = %ld\n", request, strlen(request));

    int sockfd = connect_to_server(&parsed_url);

    send_http_request(sockfd, request);

    HttpResponse response = {0};
    receive_http_response_dynamic(sockfd, &response, &total);

    int redirect_count = 0;
    while (response.headers && redirect_count < 5) {

        printf("%s", response.headers);
        // ***
        for(int i=0;i<response.body_size;i++){
            printf("%c", (char)response.body[i]);
        }

        int status_code = extract_status_code(response.headers);
        if (status_code < 300 || status_code > 399) {
            break;
        }

        const char *location_header = strstr(response.headers, "Location: ");
        if (!location_header) {
            break;
        }

        const char *location_start = location_header + strlen("Location: ");
        const char *location_end = strstr(location_start, "\r\n");
        if (!location_end) {
            break;
        }

        char new_url[512];
        strncpy(new_url, location_start, location_end - location_start);
        new_url[location_end - location_start] = '\0';

        int flag = 0;
        ParsedURL redirect_url;
        char *redirect_args[] = {"./client", new_url};
        if (redirect_args[1][0] != '/') {
             fill_parsed_url(2, redirect_args, &redirect_url);
             flag = 1;
        }
        else {
            strcpy(parsed_url.path, redirect_args[1]);
            build_http_request(&parsed_url, new_url, sizeof(new_url));
            printf("\n Total received response bytes: %d\n", (int)(total + response.body_size));
            total = 0;
            printf("HTTP request =\n%s\nLEN = %ld\n", new_url, strlen(new_url));

        }
        if (flag == 1) {
            build_http_request(&redirect_url, request, sizeof(request));
            printf("\n Total received response bytes: %d\n", (int)(total + response.body_size));
            total = 0;
            printf("HTTP request =\n%s\nLEN = %ld\n", request, strlen(request));
        }
        if (flag != 1) {
           strcpy(request, new_url);
           strcpy(redirect_url.hostname, parsed_url.hostname);
           strcpy(redirect_url.path, parsed_url.path);
           strcpy(redirect_url.params, parsed_url.params);
           redirect_url.port = parsed_url.port;
       }

        close(sockfd);
        sockfd = connect_to_server(&redirect_url);

        send_http_request(sockfd, request);

        free_http_response(&response);

        receive_http_response_dynamic(sockfd, &response, &total);

        redirect_count++;
    }

    printf("\n Total received response bytes: %d\n", (int)(total + response.body_size));
    free_http_response(&response);
    close(sockfd);

    return 0;
}
