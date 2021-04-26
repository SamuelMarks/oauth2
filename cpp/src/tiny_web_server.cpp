// Reference: https://rosettacode.org/wiki/Hello_world/Web_server#C
#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>

#include "config.h"
#include "tiny_web_server.h"

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#include <ws2tcpip.h>
#include <winsock2.h>
#include <io.h>

void err(int code, const char *message) {
    std::cerr << message << std::endl;
    exit(code == EXIT_SUCCESS? EXIT_FAILURE : code);
}

#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <err.h>
#endif

#include "macros.h"

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

AuthenticationResponse wait_for_oauth2_redirect() {
    AuthenticationResponse authentication_response;

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    // Initialize Winsock
    int iResult;
    WSADATA wsaData;
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed: " << iResult << std::endl;
    }
#endif

    int socket_options = SO_DEBUG;
    int client_file_descriptor;

    struct sockaddr_in svr_addr{}, cli_addr{};
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
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
        closesocket(server_socket);
#else
        close(server_socket);
#endif
        err(1, "Can't bind");
    }

    int listen_resp = listen(server_socket, MSG_BACKLOG);
    if (listen_resp != 0) {
        exit(EXIT_FAILURE);
    }
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
        char buffer[STACK_SIZE];
        int bytes;
        do {
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
            bytes = recv(client_file_descriptor, buffer, STACK_SIZE, 0);
#else
            bytes = read(client_file_descriptor, buffer, STACK_SIZE);
#endif
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
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
            send(client_file_descriptor, responseErr, sizeof(responseErr) - 1, 0);
#else
            write(client_file_descriptor, responseErr, sizeof(responseErr) - 1);
#endif
            continue;
        }
        std::string incoming_message = incoming_datastream.str();
        std::cout << "Received:\n"
                  << incoming_message;

        // check if this is a GET method
        if ( incoming_message[0] != 'G' ||
            incoming_message[1] != 'E' ||
            incoming_message[2] != 'T' ) {

#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
            send(client_file_descriptor, responseOk, sizeof(responseOk) - 1, 0);
#else
            write(client_file_descriptor, responseOk, sizeof(responseOk) - 1); /*-1:'\0'*/
#endif
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
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
            send( client_file_descriptor, responseOk, sizeof(responseOk) - 1, 0 );
            closesocket(client_file_descriptor);
#else
            write(client_file_descriptor, responseOk, sizeof(responseOk) - 1); /*-1:'\0'*/
            close(client_file_descriptor);
#endif
            break;
        } else {
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
            send( client_file_descriptor, responseErr, sizeof(responseErr) - 1, 0 );
            closesocket(client_file_descriptor);
#else
            write(client_file_descriptor, responseErr, sizeof(responseErr) - 1); /*-1:'\0'*/
            close(client_file_descriptor);
#endif
        }

    }
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
    WSACleanup();
#endif
    return authentication_response;
}

#ifdef TEST_TINY_WEB_SERVER
int main()
{
    wait_for_oauth2_redirect();
}
#endif
