#ifndef CIMPLE_CGET_H
#define CIMPLE_CGET_H

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>

int create_socket(const std::string& host, int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Error creating socket.\n";
        return -1;
    }

    struct hostent *server = gethostbyname(host.c_str());
    if (!server) {
        std::cerr << "No such host.\n";
        return -1;
    }

    struct sockaddr_in server_addr;
    std::memset((char *)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    std::memcpy((char *)&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server.\n";
        return -1;
    }

    return sockfd;
}

void download_file(const std::string& host, const std::string& resource, const std::string& path, int port=80) {
    int sockfd = create_socket(host, port);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket.\n";
        return;
    }

    std::string request = "GET " + resource + " HTTP/1.1\r\n";
    request += "Host: " + host + "\r\n";
    request += "Connection: close\r\n\r\n";
    send(sockfd, request.c_str(), request.size(), 0);

    std::ofstream outfile(path, std::ios::binary);
    char buffer[4096];
    bool header_ended = false;
    while (true) {
        ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            break;
        }
        if (!header_ended) {
            std::string response(buffer, bytes_received);
            size_t header_end = response.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                header_ended = true;
                outfile.write(response.c_str() + header_end + 4, bytes_received - header_end - 4);
            }
        } else {
            outfile.write(buffer, bytes_received);
        }
    }
    close(sockfd);
    outfile.close();
    std::cout << "File downloaded successfully to " << path << "\n";
}


#endif // CIMPLE_CGET_H