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

typedef struct {
    std::string projectNumber;
    std::string projectId;
    std::string lifecycleState;
    std::string name;
    std::string createTime; /* could make this a `std::tm`s */
} GoogleCloudProject;

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

    if (token_response.status >= 300) {
        std::cerr << token_response.raw << std::endl;
        return EXIT_FAILURE;
    }

    const JsonItem access_json = json_create_from_string(token_response.body);
    const std::string access_token = access_json.object.find("access_token")->second.text;
    std::cout << "Access Token: " << access_token << '\n';

    Request private_request = make_request(
            URL("https://cloudresourcemanager.googleapis.com/v1beta1/projects"));
    private_request.headers.emplace_back("Content-type: application/json");
    private_request.headers.push_back("Authorization: Bearer " + access_token);
    Response private_response;
    if (http_send(private_request, private_response)) {
        throw std::runtime_error("request failed to get projects");
    }
    std::cout << private_response.raw << '\n';
    const JsonItem projects = json_create_from_string(private_response.body).object.find("projects")->second;
    if (projects.array.empty()) {
        std::cerr << "A project must be created. For details on how and why, see: "
                     "https://cloud.google.com/resource-manager/docs/creating-managing-projects"
                  << std::endl;
        return EXIT_FAILURE;
    }

    GoogleCloudProject project;
    /* maybe reverse this array / iterate in reverse? */
    for(JsonItem const &item : projects.array)
        if (item.object.find("lifecycleState")->second.text == "ACTIVE") {
            project = GoogleCloudProject{
                    /*.projectNumber=*/    item.object.find("projectNumber")->second.text,
                    /*.projectId=*/    item.object.find("projectId")->second.text,
                    /*.lifecycleState=*/    item.object.find("lifecycleState")->second.text,
                    /*.name=*/    item.object.find("name")->second.text,
                    /*.createTime=*/    item.object.find("createTime")->second.text,
            };
            break;
        }
    std::cout << "Found project: " << project.name << " (" << project.projectId << ')' << std::endl;
}
