#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <iostream>
#include <sstream>
#include <stdio.h> /* printf, sprintf */
#include <stdlib.h> /* exit */
#include <unistd.h> /* read, write, close */
#include <string.h> /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */

#ifdef TEST_TINY_WEB_SERVER
#include "json_parser.cpp"
#endif

void error(const char *msg) { perror(msg); exit(0); }

struct URI {
    std::string protocol;
    std::string protocol_version;
    std::string host;
    int port;
    std::string path;
    std::map<std::string, std::string> parameters;
    std::string fragment;
};

struct Request {
    std::string verb;
    URI uri;
    std::vector<std::string> headers;
    std::map<std::string, std::string> fields;
};

std::string create_uri(URI uri) {
    return {};
}

struct ResponseError {
    int code;
    std::string message;

    ResponseError() : code(0) {};
};

struct Response {
    int status;
    std::string content_type;
    std::vector<std::string> headers;
    std::string body;
    std::string raw;
    ResponseError error;
};

std::string create_host(Request const & request) {
    std::ostringstream ss;
    ss << request.uri.host;
    if ( request.uri.port != 0 && request.uri.port != 80 ) {
        ss << ":" << request.uri.port;
    }
    return ss.str();
}

std::string create_message(Request const & request) {
    //char *message_fmt = "GET / HTTP/1.0\r\n\r\n";
    std::ostringstream oss;
    oss << request.verb;
    oss << " ";
    oss << request.uri.path;
    oss << " ";
    oss << request.uri.protocol;
    oss << "/";
    oss << request.uri.protocol_version;
    oss << "\r\n\r\n";
    return oss.str();
}

int http_send(Request const & request, Response& response) {
    std::string message = create_message(request);
    std::cout << "Target: " << create_host(request) << '\n';
    std::cout << "Sending: " << message;

    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;

    /* create the socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        response.error.message = "ERROR opening socket";
        response.error.code = 1001;
        return response.error.code;
    }

    /* lookup the ip address */
    server = gethostbyname(request.uri.host.c_str());
    if (server == NULL) {
        response.error.message = "ERROR no such host";
        response.error.code = 1002;
        return response.error.code;
    }

    /* fill in the structure */
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(request.uri.port);
    memcpy(&serv_addr.sin_addr.s_addr,server->h_addr,server->h_length);

    /* connect the socket */
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        response.error.message = "ERROR connecting";
        response.error.code = 1003;
        return response.error.code;
    }

    /* send the request */
    
    int bytes;
    auto total = message.size();
    int sent = 0;
    do {
        bytes = write(sockfd,message.c_str()+sent,total-sent);
        if (bytes < 0) {
            response.error.message = "ERROR writing message to socket";
            response.error.code = 1004;
            return response.error.code;
        }
        if (bytes == 0)
            break;
        sent+=bytes;
    } while (sent < total);

    /* receive the response */
    std::ostringstream incoming_data;
    char buffer[65535];
    total = sizeof(buffer)-1;
    int received = 0;
    do {
        bytes = read(sockfd,buffer+received,total-received);
        if (bytes < 0) {
            response.error.message = "ERROR reading response from socket";
            response.error.code = 1005;
            return response.error.code;
        }
        if (bytes == 0)
            break;
        received += bytes;
        if ( received == total ) {
            incoming_data << buffer;
            // reset to nulls
            memset(&buffer,0,sizeof(buffer));
            received = 0;
        }
    } while (true);
    incoming_data << buffer;

    /* close the socket */
    close(sockfd);

    response.raw = incoming_data.str();
    size_t pos = 0;
    for(auto ii=0; ii < response.raw.size(); ii++ ) {
        if ( response.raw[ii] == '\r' && response.raw[ii+1] == '\n' &&
                response.raw[ii+2] == '\r' && response.raw[ii+3] == '\n' ) {
            response.body = response.raw.substr(ii+4, response.raw.size() - (ii+4));
            break;
        }
        if ( response.raw[ii] == '\r' && response.raw[ii+1] == '\n') {
            auto header = response.raw.substr(pos, ii-pos);
            response.headers.push_back(header);
            if ( response.headers.size() == 1 ) {
                auto start = header.find_first_of(" ")+1;
                auto end = header.find_last_of(" ");
                auto status = header.substr(start, end-start);
                response.status = std::stol(status);
            }
            auto lc_header = header;
            std::transform(lc_header.begin(), lc_header.end(), lc_header.begin(),
                    [](unsigned char c){ return std::tolower(c); });
            if ( lc_header.find("content-type: ") == 0 ) {
                auto start = lc_header.find(" ")+1;
                auto end = lc_header.find(";");
                if ( end == std::string::npos ) {
                    end = lc_header.size();
                }
                response.content_type = lc_header.substr(start, end - start);
            }
            pos = ii + 2;
        }
    }


    return 0;
}


#ifdef TEST_TINY_WEB_SERVER
int main() {
    auto req = Request {};
    req.verb = "GET";
    req.uri.protocol = "HTTP";
    req.uri.protocol_version = "1.0";
    // https://31f5ff35.eu-gb.apigw.appdomain.cloud/authtest/GetApplicationEndpoint
    req.uri.host = "31f5ff35.eu-gb.apigw.appdomain.cloud";
    req.uri.port = 80;
    req.uri.path = "/authtest/GetApplicationEndpoint";
    
    auto resp = Response{};

    if ( http_send(req, resp) ) {
        std::cout << resp.error.message << std::endl;
    } else {
        std::cout << resp.status << std::endl;
        std::cout << resp.content_type << std::endl;
        std::cout << resp.body << std::endl;
    }

}
#endif