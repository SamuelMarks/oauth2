#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sstream>

#include "char_tests.cpp"

#define MIN(x,y)    ((x <= y) ? x : y)

// In this case we are going to define a type URL that
// will check that the use has something of the right
// type.  We could just pass a string but this is a useful
// diversion.
class URL
{
public:
    URL() = default;

    URL(std::string url)
    {
        validate_(url);
    }

    URL(URL const&) = default;
    URL(URL&& ) = default;

    std::string protocol;
    std::string domain;
    std::string path;
    std::string querystring;
    std::string fragment;

    std::ostream& print(std::ostream& out) const {
        out << "URL          : " << data_ << '\n';
        out << "Encoded      : " << encoded_data_ << '\n';
        out << "  Protocol   : " << protocol << '\n';
        out << "  Domain     : " << domain<< '\n';
        out << "  Path       : " << path<< '\n';
        out << "  Querystring: " << querystring<< '\n';
        out << "  Fragment   : " << fragment<< '\n';
        return out;
    }

    std::string encoded() const {
        return encoded_data_;
    }
private:
    std::string data_;
    std::string encoded_data_;


    size_t set_protocol_(std::string const & url ) {
        if (url.substr(0, 5) != "http:" ) {
            protocol = "http";
        }
        if (url.substr(0, 5) != "https:" ) {
            protocol = "https";
        }
        if (protocol.size() == 0)
        {
            throw std::runtime_error("no protocol given, http(s) expected at start of url");
        }
        return protocol.size();
    }

    size_t check_protocol_domain_separator_(std::string const & url) {
        auto p = url.find_first_of("/");
        if ( p == std::string::npos ) {
            throw std::runtime_error("invalid url, no // found after protocol");
        } else if ( url[p] == '/' && url.size() < p+1) {
            throw std::runtime_error("invalid url, only a single / found and string is too short");
        } else if ( url[p+1] != '/') {
            throw std::runtime_error("invalid url, // after protocol not found");
        }
        return protocol.size() + 2;
    }

    size_t set_domain_(std::string const & url, size_t pos ) {
        if ( pos >= url.size() ) {
            return pos;
        }
        pos++;
        auto end_of_domain = url.find_first_of("/", pos);
        auto querystring_pos = url.find_first_of("?", pos);
        auto fragment_pos = url.find_first_of("#", pos);
        end_of_domain = MIN(end_of_domain, fragment_pos);
        end_of_domain = MIN(end_of_domain, querystring_pos);
        auto this_domain = url.substr(pos, end_of_domain - pos);
        if ( this_domain.size() == 0 ) {
            std::ostringstream ss;
            ss << "no domain found in '" << url << "'";
            throw std::runtime_error(ss.str());
        } else {
            if ( this_domain[0] == '-' ) {
                throw std::runtime_error("invalid domain, cannot start with a hyphen");
            }
            if ( this_domain[this_domain.size()-1] == '-' ) {
                throw std::runtime_error("invalid domain, cannot end with a hyphen");
            }
            for(auto ii=0; ii < this_domain.size(); ii++ ) {
                if ( this_domain[ii] == '#' || this_domain[ii] == '?' ) {
                    break;
                }
                if ( ii < this_domain.size() -2 ) {
                    if ( this_domain[ii] == '.' && this_domain[ii+1] =='.' ) {
                        std::ostringstream ss;
                        ss << "invalid character '" << this_domain[ii+1] << "' found next to '.' in domain of '" << url << "'";
                        throw std::runtime_error(ss.str());
                    }
                }
                if ( this_domain[ii] == '.' && (this_domain[ii] == 0 || ii == this_domain.size()-1) ) {
                    std::ostringstream ss;
                    ss << "'.'' cannot appear at start or end of domain in url '" << url << "'";
                    throw std::runtime_error(ss.str());
                }
                if ( !is_domain_character(this_domain[ii]) ) {
                    std::ostringstream ss;
                    ss << "invalid character '" << this_domain[ii] << "' found in domain of '" << url << "'";
                    throw std::runtime_error(ss.str());
                }
            }
        }
        domain = this_domain;
        return pos + domain.size();
    }

    size_t set_path_(std::string const & url, size_t pos ) {
        if ( pos >= url.size() ) {
            return pos;
        }
        if ( url[pos] == '#' || url[pos] == '?' ) {
            return pos;
        }
        auto querystring_pos = url.find_first_of("?", pos);
        auto fragment_pos = url.find_first_of("#", pos);
        if ( querystring_pos == std::string::npos && fragment_pos == std::string::npos ) {
            path = url.substr(pos);
        } else {
            auto next_pos =  MIN(querystring_pos, fragment_pos);
            path = url.substr(pos, next_pos - pos);
        }
        if ( path.size() == 0 ) {
            std::ostringstream ss;
            ss << "empty path in '" << url << "'";
            throw std::runtime_error(ss.str());
        }
        for( auto ii=0; ii < path.size(); ii++ ) {
            if ( !is_valid_path_char(path[ii]) ) {
                std::ostringstream ss;
                ss << "invalid character '" << path[ii] << "' found in path of '" << url << "'";
                throw std::runtime_error(ss.str());
            }
        }
        return MIN(MIN(querystring_pos, fragment_pos), url.size());
    }

    size_t set_querystring_(std::string const& url, size_t pos ) {
        if ( pos >= url.size() ) {
            return pos;
        }
        auto querystring_pos_start = url.find_first_of("?", pos);
        auto querystring_pos_end = url.find_first_of("#", pos);
        if ( querystring_pos_start == std::string::npos) {
            return pos;
        }
        if ( querystring_pos_end == std::string::npos ) {
            querystring_pos_end = url.size();
        }
        querystring = url.substr(querystring_pos_start+1, querystring_pos_end - (querystring_pos_start+1));
        return querystring_pos_end;
    }

    std::string encode_querystring_(std::string const& url) {
        auto querystring_pos_start = url.find_first_of("?");
        auto querystring_pos_end = url.find_first_of("#", querystring_pos_start);
        // check querystring and convert any special characters to the appropriate
        // %XX code.
        if ( querystring_pos_start == std::string::npos ) {
            return "";
        }
        if ( querystring_pos_end == std::string::npos) {
            querystring_pos_end = url.size();
        }
        std::ostringstream ss;
        auto ii = querystring_pos_start+1;
        for(; ii < querystring_pos_end; ii++ ) {
            if ( url[ii] == '#' ) {
                break;
            }
            if ( url[ii] == ' ' ) {
                ss << "%20";
            } else if ( url[ii] == '\"' ) {
                ss << "%22";
            } else if ( url[ii] == '\'' ) {
                ss << "%27";
            } else if ( url[ii] == '<' ) {
                ss << "%3C";
            } else if ( url[ii] == '>' ) {
                ss << "%3E";
            } else if ( url[ii] == '+' ) {
                ss << "";
            } else {
                ss << url[ii];
            }
        }
        auto encoded_query_string = ss.str();
        querystring = encoded_query_string;
        return encoded_query_string;
    }

    size_t set_fragment_(std::string const & url, size_t pos ) {
        auto fragment_pos_start = url.find_first_of("#");
        auto fragment_pos_end = url.find_first_of("?", fragment_pos_start);
        if ( fragment_pos_start == std::string::npos ) {
            return pos;
        }
        if ( fragment_pos_end == std::string::npos ) {
            fragment_pos_end = url.size();
        }
        for( auto ii=fragment_pos_start+1; ii < url.size(); ii++ ) {
            if ( !is_fragment(url[ii]) ) {
                std::ostringstream ss;
                ss << "invalid character '" << url[ii] << "' found in fragment of '" << url << "'";
                throw std::runtime_error(ss.str());
            }
        }
        fragment = url.substr(fragment_pos_start, fragment_pos_end - fragment_pos_start);
        return fragment_pos_end;
    }

    // Our validate function, we are going to
    // throw an exception if our url fails.
    // https://isocpp.org/wiki/faq/exceptions#ctors-can-throw
    void validate_(std::string const & url)
    {
        data_ = url;
        encoded_data_ = url;
        if ( encoded_data_.size() == 0 ) {
            throw std::runtime_error("empty string is not a valid url");
        }
        if ( encoded_data_.size() < 5 ) {
            std::ostringstream ss;
            ss << "invalid url found, string '" << url << "' is too short";
            throw std::runtime_error(ss.str());
        }
        auto pos = 0UL;
        set_protocol_(url);
        pos = check_protocol_domain_separator_(url);
        pos = set_domain_(url, pos);
        if ( pos  == url.size() ) {
            return;
        }

        pos = set_path_(url, pos);
        pos = set_querystring_(url,pos);
        encode_querystring_(url);
        pos = set_fragment_(url, pos);
        encoded_data_ = protocol + "//"+domain+path+querystring+fragment;
        std::cout << pos << " " << url.size() << '\n';
        if ( pos != url.size() ) {
            throw std::runtime_error("unexpected characters in url after domain, expected ? or #");
        }
    }

    
};

std::ostream& operator<<(std::ostream& out, URL& url) {
#ifdef TEST_URL
    return url.print(out);
#else
    out << url.encoded();
    return out;
#endif
}

std::ostream& operator<<(std::ostream& out, URL const & url) {
#ifdef TEST_URL
    return url.print(out);
#else
    out << url.encoded();
    return out;
#endif
}

std::ostream& operator<<(std::ostream& out, URL* url) {
#ifdef TEST_URL
    return url->print(out);
#else
    out << url->encoded();
    return out;
#endif
}

std::ostream& operator<<(std::ostream& out, URL const * url) {
#ifdef TEST_URL
    return url->print(out);
#else
    out <<  url->encoded();
    return out;
#endif
}


#ifdef TEST_URL
#define ASSERT_EXCEPTION(x)     do {\
                                    try {\
                                        x;\
                                        throw std::runtime_error("unexpected exception raised in test");\
                                    } catch(...) {}\
                                } while(0)

int main() 
{
    {
        ASSERT_EXCEPTION(URL(""));
    }
    {
        ASSERT_EXCEPTION(URL ("abc"));
    }
    {
        ASSERT_EXCEPTION(URL ("/abc"));
    }
    {
        ASSERT_EXCEPTION(URL ("http"));
    }
    {
        ASSERT_EXCEPTION(URL ("https"));
    }
    {
        ASSERT_EXCEPTION(URL ("https://"));
    }
    {
        ASSERT_EXCEPTION(URL ("http://"));
    }
    {
        ASSERT_EXCEPTION(URL ("http:///"));
    }
    {
        ASSERT_EXCEPTION(URL ("https:///"));
    }
    {
        ASSERT_EXCEPTION(URL ("http://."));
    }
    {
        ASSERT_EXCEPTION(URL ("https://a..."));
    }
    {
        ASSERT_EXCEPTION(URL ("https://www.example_.com"));
    }
    {
        URL ("https://www.example.com#abc");
    }
    {
        URL ("https://www.example.com?key=value");
    }
    {
        auto a = URL ("https://www.example.com?key=value'abc");
        std::cout << a << std::endl;
    }
    {
        auto a = URL ("https://www.example.com/?key=value'abc#f");
        std::cout << a << std::endl;
    }
    {
        auto a = URL ("https://31f5ff35.eu-gb.api.example.cloud/private-test/Hello");
        std::cout << a << std::endl;
    }
    
}
#endif