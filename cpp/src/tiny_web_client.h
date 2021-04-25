#ifndef OAUTH2_TINY_WEB_CLIENT_H
#define OAUTH2_TINY_WEB_CLIENT_H

#include <cstdio>
#include <string>
#include <map>
#include <vector>
#include "url.h"
#include "config.h"
#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

void error(const char *);

struct URI
{
    bool use_ssl;
    std::string protocol;
    std::string protocol_version;
    std::string host;
    int port;
    std::string path;
    std::string querystring;
    std::map<std::string, std::string> parameters;
    std::string fragment;
};

struct Request
{
    std::string verb;
    URI uri;
    std::vector<std::string> headers;
    std::map<std::string, std::string> fields;
};

struct ResponseError
{
    int code;
    std::string message;

    ResponseError() : code(0){};
};

struct Response
{
    int status;
    std::string content_type;
    std::vector<std::string> headers;
    std::string body;
    std::string raw;
    ResponseError error;
};

std::string create_host(Request const &);

std::string create_message(Request const &);

Request make_request(const URL &, const std::string &verb="GET");

// Using the RAII idiom to ensure our SSL resource is cleaned up
class SSLClient
{
public:
    SSLClient() : context_(nullptr), session_(nullptr),  valid_(false), ssl_socket_file_descriptor_(0)
    {
    }

    [[nodiscard]] bool is_valid() const;

    bool connect_to_socket(int);

    static void display_errors() {
        for (auto err = ERR_get_error(); err; err = ERR_get_error()) {
            char *str = ERR_error_string(err, nullptr);
            if (!str)
                return;
            printf("%s", str);
            printf("\n");
            fflush(stdout);
        }
    }

    SSL *session();

    ~SSLClient();

    // make this unable to be copied
    SSLClient(SSLClient const &) = delete;
    SSLClient &operator=(const SSLClient &) = delete;

private:

    // private variables that we will clean up on destruction
    SSL_CTX *context_;
    SSL *session_;
    bool valid_;
    int ssl_socket_file_descriptor_;

    static void ssl_library_init()
    {
        static bool already_done_this = false;
        if (already_done_this)
        {
            return;
        }
        SSL_library_init();
        SSLeay_add_ssl_algorithms();
        SSL_load_error_strings();
        already_done_this = true;
    }

    // private methods only for our use
    bool init_class()
    {
        ssl_library_init();
        const SSL_METHOD *method = TLS_client_method();
        context_ = SSL_CTX_new(method);
        if (!context_)
        {
            shutdown();
            return false;
        }
        session_ = SSL_new(context_);
        valid_ = true;
        if (!session_)
        {
            shutdown();
        }
        return valid_;
    }

    void shutdown()
    {
        valid_ = false;
        if (session_)
        {
            SSL_shutdown(session_);
            SSL_free(session_);
        }
        if (context_)
        {
            SSL_CTX_free(context_);
        }
        context_ = nullptr;
        session_ = nullptr;
    }
};

int http_send(Request &, Response &,  const std::map<std::string, std::string> &post_fields = {});

#endif /* OAUTH2_TINY_WEB_CLIENT_H */
