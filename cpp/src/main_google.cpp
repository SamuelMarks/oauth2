/**
 * OAuth2 in C/C++
 * Tested for Google auth, following this guide:
 * https://developers.google.com/identity/protocols/oauth2/native-app
 */

#include "url.h"
#include "tiny_web_client.h"
#include "random_string.h"
#include "json_parser.h"
#include "open_browser.h"
#include "tiny_web_server.h"

int main()
{
    const std::string redirect_uri = static_cast<const std::ostringstream&>(
            std::ostringstream() << "http://" << SERVER_HOST << ':'
                                 << PORT_TO_BIND << EXPECTED_PATH).str();
    const char * scope = "https://www.googleapis.com/auth/cloud-platform ";
    // "https://www.googleapis.com/auth/devstorage.read_write"
    const std::string temporary_secret_state = generate_random_string(10);
    URL authorization_url = URL("https://accounts.google.com/o/oauth2/v2/auth");
    authorization_url.add_param("scope", scope);
    authorization_url.add_param("response_type", "code");
    authorization_url.add_param("state", temporary_secret_state);
    authorization_url.add_param("client_id", CLIENT_ID);
    authorization_url.add_param("redirect_uri", redirect_uri);
    authorization_url.add_param("scope", "openid");
    open_browser(authorization_url);

    // we then need to start our web server and block
    // until we get the appropriate response
    const AuthenticationResponse oauth_response = wait_for_oauth2_redirect();

    if ( oauth_response.secret != temporary_secret_state )
        throw std::runtime_error(static_cast<const std::ostringstream&>(
                                         std::ostringstream() <<"oauth2 redirect contained the wrong secret state ("
                                                              << oauth_response.secret << ") ,"
                                                              << "expected: (" << temporary_secret_state << ")\n").str());

    const URL token_url = URL( "https://oauth2.googleapis.com/token");
    const std::map<std::string, std::string> post_fields {
            std::make_pair("grant_type", "authorization_code"),
            std::make_pair("code", oauth_response.code),
            std::make_pair("redirect_uri", redirect_uri),
            std::make_pair("client_id", CLIENT_ID),
            std::make_pair("client_secret", CLIENT_SECRET)
    };
    Request token_request = make_request(token_url, "POST");
    Response token_response;
    if ( http_send(token_request, token_response, post_fields) != 0 )
        throw std::runtime_error("request failed to get token");

    if ( token_response.status < 300 ) {
        const JsonItem access_json = json_create_from_string(token_response.body);
        const std::string access_token = access_json.object.find("access_token")->second.text;
        std::cout << "Access Token: " << access_token << '\n';
    } else {
        std::cerr << token_response.raw << std::endl;
        return EXIT_FAILURE;
    }
}
