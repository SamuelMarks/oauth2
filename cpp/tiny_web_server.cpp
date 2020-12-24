// Reference: https://rosettacode.org/wiki/Hello_world/Web_server#C
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <err.h>

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

AuthenticationResponse wait_for_oauth2_redirect()
{
    AuthenticationResponse authentication_response;
    int socket_options = SO_DEBUG;
    int client_file_descriptor = 0;

    struct sockaddr_in svr_addr, cli_addr;
    socklen_t sin_len = sizeof(cli_addr);

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
        err(1, "can't open socket");

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &socket_options, sizeof(int));

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
        client_file_descriptor = accept(server_socket, (struct sockaddr *)&cli_addr, &sin_len);
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
        std::cout << "Received:\n";
        std::cout << incoming_message;

        // check if this is a GET method
        if ( incoming_message[0] != 'G' ||
            incoming_message[1] != 'E' ||
            incoming_message[2] != 'T' ) {
            write(client_file_descriptor, responseOk, sizeof(responseOk) - 1); /*-1:'\0'*/
            continue;
        }  

        // check the URL
        auto start = incoming_message.find_first_of(' ')+1;
        auto next = incoming_message.find_first_of(' ', start);
        auto path = incoming_message.substr(start, next-start);
        if ( path == EXPECTED_PATH ) {
            ok = true;
            authentication_response.raw = incoming_message;
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