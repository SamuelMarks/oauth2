// Reference: https://rosettacode.org/wiki/Hello_world/Web_server#C
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <sys/types.h>

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#include <intrin.h>

#ifndef _WIN64
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <winsock2.h>
#define close closesocket
#define write _write
#define read _read

void err(int code, const char *message) {
    fprintf(stderr, message);
    exit(code);
}

#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#endif

#include "macros.h"

// this is what we would define in our authentication settings in App ID
#define SERVER_ADDR "127.0.0.1"
#define SERVER_HOST "localhost"
#define PORT_TO_BIND 3000
#define MSG_BACKLOG 5
#define EXPECTED_PATH "/ibm/cloud/appid/callback"

char responseOk[] = "HTTP/1.0 200 OK\r\n"
                    "Content-Type: text/plain\r\n"
                    "\r\n"
                    "Ok. You may close this tab and return to the shell.\r\n";
char responseErr[] = "HTTP/1.0 400 Bad Request\r\n"
                     "Content-Type: text/plain\r\n"
                     "\r\n"
                     "Bad Request\r\n";

// This is a specialised web server type functionality
// that waits on IBM to call us back.
// This is doing what startHttpServer is doing
// in PHP.
struct AuthenticationResponse{
    std::string raw;
    std::string secret;
    std::string code;
};

#include <map>
std::map<std::string, std::string> 
split_querystring(std::string const & querystring ) 
{
    std::map<std::string, std::string> result;
    size_t lastpos = 0;
    std::string key;
    std::string value;
    for(size_t ii=0; ii < querystring.size(); ii++ ) {
        if ( querystring[ii] == '=' ) {
            key = querystring.substr(lastpos, ii-lastpos);
            lastpos = ii+1;
        } else if ( querystring[ii] == '&' ) {
            value = querystring.substr(lastpos, ii-lastpos);
            result.insert(std::make_pair(key,value));
            lastpos = ii+1;
            key.clear();
            value.clear();
        }
    }
    if ( !key.empty() ) {
        value = querystring.substr(lastpos);
        result.insert(std::make_pair(key,value));
    }
    return result;
}

AuthenticationResponse wait_for_oauth2_redirect()
{
    AuthenticationResponse authentication_response;
    int socket_options = SO_DEBUG;
    int client_file_descriptor = 0;

    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);

    int server_socket = (int)socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
        err(1, "can't open socket");

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&socket_options, sizeof(int));

    int port = PORT_TO_BIND;
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr *)&svr_addr, sizeof(svr_addr)) == -1)
    {
        close(server_socket);
        err(1, "Can't bind");
    }

    listen(server_socket, MSG_BACKLOG);
    bool ok = false;
    while (!ok)
    {
        client_file_descriptor = (int)accept(server_socket, (struct sockaddr *)&cli_addr, &sin_len);
        printf("got incoming connection\n");

        if (client_file_descriptor == -1)
        {
            perror("Can't accept");
            continue;
        }

        // keep on reading until we have digest everything
        std::ostringstream incoming_datastream;
        char buffer[65535];
        int bytes=0;
        do {
            bytes = read(client_file_descriptor, buffer, 65535);
            if ( bytes > 0 ) {
                incoming_datastream << buffer;
                // if we do not fill up our buffer then we have 
                // got the whole message
                if ( bytes < sizeof(buffer) ) {
                    break;
                }
            } 
        } while( bytes != 0);

        if ( bytes < 0 ) {
            printf("Error while reading incoming data stream.\n");
            write(client_file_descriptor, responseErr, sizeof(responseErr) - 1); 
            continue;
        }
        std::string incoming_message = incoming_datastream.str();
        std::cout << "Received:\n"
                  << incoming_message;

        // check if this is a GET method
        if ( incoming_message[0] != 'G' ||
            incoming_message[1] != 'E' ||
            incoming_message[2] != 'T' ) {
            write(client_file_descriptor, responseOk, sizeof(responseOk) - 1); /*-1:'\0'*/
            continue;
        }  

        // check the URL
        size_t start = incoming_message.find_first_of(' ')+1,
               end_of_target_url = incoming_message.find_first_of(' ', start);
        std::string target_url = incoming_message.substr(start, end_of_target_url-start);
        size_t querystring = incoming_message.find_first_of('?', start),
               fragment = incoming_message.find_first_of('#', start),
               end_of_path = MIN(end_of_target_url, querystring);
        end_of_path = MIN(end_of_path, fragment);
        std::string path = incoming_message.substr(start, end_of_path - start);
        if ( path == EXPECTED_PATH && querystring != std::string::npos ) {
            ok = true;
            authentication_response.raw = incoming_message;
            // the code and state are passed as GET parameters
            size_t end_of_qs = MIN(end_of_target_url, fragment);
            std::string qs = incoming_message.substr(querystring+1, end_of_qs - querystring-1);
            auto params = split_querystring(qs);
            // note we do not have to check if the map contains these,
            // if they do not exist they will return the blank string.
            authentication_response.code = params["code"];
            authentication_response.secret = params["state"];
        }
        
        if ( ok ) {
            write(client_file_descriptor, responseOk, sizeof(responseOk) - 1); /*-1:'\0'*/
            close(client_file_descriptor);
            break;
        } else {
            write(client_file_descriptor, responseOk, sizeof(responseOk) - 1); /*-1:'\0'*/
        }
    }
    return authentication_response;
}

#ifdef TEST_TINY_WEB_SERVER
int main()
{
    wait_for_oauth2_redirect();
}
#endif