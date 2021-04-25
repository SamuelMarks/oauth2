#ifndef OAUTH2_TINY_WEB_SERVER_H
#define OAUTH2_TINY_WEB_SERVER_H

#include <string>

static char responseOk[] = "HTTP/1.0 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "\r\n"
                           "Ok. You may close this tab and return to the shell.\r\n";
static char responseErr[] = "HTTP/1.0 400 Bad Request\r\n"
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

std::map<std::string, std::string> 
split_querystring(std::string const & );

AuthenticationResponse wait_for_oauth2_redirect();

#endif /* OAUTH2_TINY_WEB_SERVER_H */
