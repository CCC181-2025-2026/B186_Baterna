#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_NAME "Simple-C-Server-WinSock"

// Function to send a complete HTTP response
void send_response(SOCKET client_socket, const char *status_line, const char *content_type, const char *body) {
    char response[BUFFER_SIZE * 2];
    int content_length = strlen(body);

    snprintf(response, sizeof(response),
        "%s\r\n"
        "Server: %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        status_line, SERVER_NAME, content_type, content_length, body);

    send(client_socket, response, strlen(response), 0);
}

// Function to handle a client connection
void handle_request(SOCKET client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Received Request:\n%s\n", buffer);

        // --- 200 OK, 404 Not Found, and 400 Bad Request Handling ---
        if (strncmp(buffer, "GET ", 4) == 0) {
            // Extract requested path
            char path[256];
            sscanf(buffer, "GET %255s", path);

            // Serve main route "/"
            if (strcmp(path, "/") == 0) {
                const char *status_line = "HTTP/1.1 200 OK";
                const char *content_type = "text/html";
                const char *body =
                    "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head><title>200 OK</title></head>\n"
                    "<body><h1>Success!</h1><p>GET request received and processed.</p></body>\n"
                    "</html>\n";
                send_response(client_socket, status_line, content_type, body);

            // Example second route
            } else if (strcmp(path, "/about") == 0) {
                const char *status_line = "HTTP/1.1 200 OK";
                const char *content_type = "text/html";
                const char *body =
                    "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head><title>About Page</title></head>\n"
                    "<body><h1>About Us</h1><p>This is the About page of the server.</p></body>\n"
                    "</html>\n";
                send_response(client_socket, status_line, content_type, body);

            // Unknown path = 404 Not Found
            } else {
                const char *status_line = "HTTP/1.1 404 Not Found";
                const char *content_type = "text/html";
                const char *body =
                    "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head><title>404 Not Found</title></head>\n"
                    "<body><h1>Error 404</h1><p>The requested page was not found.</p></body>\n"
                    "</html>\n";
                send_response(client_socket, status_line, content_type, body);
            }
        } else {
            // Non-GET methods → 400 Bad Request
            const char *status_line = "HTTP/1.1 400 Bad Request";
            const char *content_type = "text/html";
            const char *body =
                "<!DOCTYPE html>\n"
                "<html>\n"
                "<head><title>400 Bad Request</title></head>\n"
                "<body><h1>Error 400</h1><p>Bad Request or unsupported method. Only GET is supported.</p></body>\n"
                "</html>\n";
            send_response(client_socket, status_line, content_type, body);
        }
    } else if (bytes_received == 0) {
        printf("Client disconnected.\n");
    } else {
        fprintf(stderr, "recv failed: %d\n", WSAGetLastError());
    }

    closesocket(client_socket);
}

int main(void) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }

    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        fprintf(stderr, "socket failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Reuse address
    char opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == SOCKET_ERROR) {
        fprintf(stderr, "setsockopt failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 2. Bind socket to IP and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        fprintf(stderr, "bind failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // 3. Listen for incoming connections
    if (listen(server_fd, 10) == SOCKET_ERROR) {
        fprintf(stderr, "listen failed: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);
    printf("Try accessing these URLs in your browser:\n");
    printf("  - http://127.0.0.1:%d/\n", PORT);
    printf("  - http://127.0.0.1:%d/about\n", PORT);
    printf("  - http://127.0.0.1:%d/somethingelse\n", PORT);

    // 4. Accept client connections
    while (1) {
        printf("\nWaiting for a new connection...\n");
        new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            fprintf(stderr, "accept failed: %d\n", WSAGetLastError());
            continue;
        }
        handle_request(new_socket);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
