#ifndef OAUTH2_CPP_URL_H
#define OAUTH2_CPP_URL_H

#include <cstdlib>
#include <sstream>
#include <iostream>
#include <string>
#include <map>

#include "char_utils.h"
#include "macros.h"

// In this case we are going to define a type URL that
// will check that the use has something of the right
// type.  We could just pass a string but this is a useful
// diversion.
class URL
{
public:
    URL() = default;

    explicit URL(const std::string &url)
    {
        validate_(url);
    }

    URL(URL const &) = default;
    URL(URL &&) = default;

    std::string protocol;
    std::string domain;
    std::string path;
    std::string querystring;
    std::string fragment;

    std::ostream &print(std::ostream &out) const
    {
        out << "URL          : " << data_ << '\n'
            << "Encoded      : " << encoded_data_ << '\n'
            << "  Protocol   : " << protocol << '\n'
            << "  Domain     : " << domain << '\n'
            << "  Path       : " << path << '\n'
            << "  Querystring: " << querystring << '\n'
            << "  Fragment   : " << fragment << '\n';
        return out;
    }

    [[nodiscard]] std::string encoded_querystring() const
    {
        if (params_.empty())
        {
            return querystring;
        }
        else
        {
            std::string qs;
            size_t counter = 0;
            for (auto const &[key, value] : params_)
            {
                qs += key + "=" + encode_(value);
                if (counter < params_.size() - 1)
                {
                    qs += "&";
                }
                counter++;
            }
            return qs;
        }
    }

    [[nodiscard]] std::string encoded() const
    {
        std::cout << params_.size() << std::endl;
        if (params_.empty())
        {
            return encoded_data_;
        }
        else
        {
            std::string qs;
            size_t counter = 0;
            for (auto const &[key, value] : params_)
            {
                qs += key + "=" + encode_(value);
                if (counter < params_.size() - 1)
                {
                    qs += "&";
                }
                counter++;
            }
            std::string url = protocol + "://" + domain + path;
            if (!qs.empty())
            {
                url += "?" + qs;
            }
            if (!fragment.empty())
            {
                url += "#" + fragment;
            }
            return url;
        }
    }

    void add_param(const std::string& key, const std::string& value)
    {
        params_.insert(std::make_pair(key, value));
    }

private:
    std::string data_;
    std::string encoded_data_;
    std::map<std::string, std::string> params_;

    size_t set_protocol_(std::string const &url)
    {
        if (url.substr(0, 5) != "http:")
        {
            protocol = "http";
        }
        if (url.substr(0, 5) != "https:")
        {
            protocol = "https";
        }
        if (protocol.empty())
        {
            throw std::runtime_error("no protocol given, http(s) expected at start of url");
        }
        return protocol.size();
    }

    [[nodiscard]] size_t check_protocol_domain_separator_(std::string const &url) const
    {
        size_t p = url.find_first_of('/');
        if (p == std::string::npos)
        {
            throw std::runtime_error("invalid url, no // found after protocol");
        }
        else if (url[p] == '/' && url.size() < p + 1)
        {
            throw std::runtime_error("invalid url, only a single / found and string is too short");
        }
        else if (url[p + 1] != '/')
        {
            throw std::runtime_error("invalid url, // after protocol not found");
        }
        return protocol.size() + 2;
    }

    size_t set_domain_(std::string const &url, size_t pos)
    {
        if (pos >= url.size())
        {
            return pos;
        }
        pos++;
        size_t end_of_domain = url.find_first_of('/', pos),
                querystring_pos = url.find_first_of('?', pos),
                fragment_pos = url.find_first_of('#', pos);
        end_of_domain = MIN(end_of_domain, fragment_pos);
        end_of_domain = MIN(end_of_domain, querystring_pos);
        std::string this_domain = url.substr(pos, end_of_domain - pos);
        if (this_domain.empty())
        {
            throw std::runtime_error(static_cast<const std::ostringstream&>(
                                             std::ostringstream() << "no domain found in '" << url << "'").str());
        }
        else
        {
            if (this_domain[0] == '-')
            {
                throw std::runtime_error("invalid domain, cannot start with a hyphen");
            }
            if (this_domain[this_domain.size() - 1] == '-')
            {
                throw std::runtime_error("invalid domain, cannot end with a hyphen");
            }
            for (size_t ii = 0; ii < this_domain.size(); ii++)
            {
                if (this_domain[ii] == '#' || this_domain[ii] == '?')
                {
                    break;
                }
                if (ii < this_domain.size() - 2)
                {
                    if (this_domain[ii] == '.' && this_domain[ii + 1] == '.')
                    {
                        throw std::runtime_error(static_cast<const std::ostringstream&>(
                                                         std::ostringstream() <<"invalid character '"
                                                                              << this_domain[ii + 1]
                                                                              << "' found next to '.' in domain of '"
                                                                              << url << "'").str());
                    }
                }
                if (this_domain[ii] == '.' && (this_domain[ii] == 0 || ii == this_domain.size() - 1))
                {
                    throw std::runtime_error(static_cast<const std::ostringstream&>(
                                                     std::ostringstream()
                                                             << "'.'' cannot appear at start or end of domain in url '"
                                                             << url << "'").str());
                }
                if (!is_domain_character(this_domain[ii]))
                {
                    throw std::runtime_error(static_cast<const std::ostringstream&>(
                                                     std::ostringstream()
                                                             << "invalid character '" << this_domain[ii]
                                                             << "' found in domain of '" << url << "'").str());
                }
            }
        }
        domain = this_domain;
        return pos + domain.size();
    }

    size_t set_path_(std::string const &url, const size_t pos)
    {
        if (pos >= url.size())
        {
            return pos;
        }
        if (url[pos] == '#' || url[pos] == '?')
        {
            return pos;
        }
        size_t querystring_pos = url.find_first_of('?', pos),
                fragment_pos = url.find_first_of('#', pos);
        if (querystring_pos == std::string::npos && fragment_pos == std::string::npos)
        {
            path = url.substr(pos);
        }
        else
        {
            size_t next_pos = MIN(querystring_pos, fragment_pos);
            path = url.substr(pos, next_pos - pos);
        }
        if (path.empty())
        {
            throw std::runtime_error(static_cast<const std::ostringstream&>(
                                             std::ostringstream() << "empty path in '" << url << "'").str());
        }
        for (char ii : path)
        {
            if (!is_valid_path_char(ii))
            {
                throw std::runtime_error(static_cast<const std::ostringstream&>(
                                                 std::ostringstream() << "invalid character '" << ii
                                                                      << "' found in path of '" << url << "'").str());
            }
        }
        return MIN(MIN(querystring_pos, fragment_pos), url.size());
    }

    size_t set_querystring_(std::string const &url, size_t pos)
    {
        if (pos >= url.size())
        {
            return pos;
        }
        size_t querystring_pos_start = url.find_first_of('?', pos);
        size_t querystring_pos_end = url.find_first_of('#', pos);
        if (querystring_pos_start == std::string::npos)
        {
            return pos;
        }
        if (querystring_pos_end == std::string::npos)
        {
            querystring_pos_end = url.size();
        }
        querystring = url.substr(querystring_pos_start + 1, querystring_pos_end - (querystring_pos_start + 1));
        return querystring_pos_end;
    }

    [[nodiscard]] static std::string encode_(std::string const &text)
    {
        std::ostringstream ss;
        for (char ii : text)
        {
            if (ii == ' ')
            {
                ss << "%20";
            }
            else if (ii == '\"')
            {
                ss << "%22";
            }
            else if (ii == '\'')
            {
                ss << "%27";
            }
            else if (ii == '<')
            {
                ss << "%3C";
            }
            else if (ii == '>')
            {
                ss << "%3E";
            }
            else if (ii == '+')
            {
                ss << "";
            }
            else
            {
                ss << ii;
            }
        }
        return ss.str();
    }

    std::string encode_querystring_(std::string const &url)
    {
        size_t querystring_pos_start = url.find_first_of('?'),
                querystring_pos_end = url.find_first_of('#', querystring_pos_start);
        // check querystring and convert any special characters to the appropriate
        // %XX code.
        if (querystring_pos_start == std::string::npos)
        {
            return "";
        }
        if (querystring_pos_end == std::string::npos)
        {
            querystring_pos_end = url.size();
        }
        std::ostringstream ss;
        for (size_t ii = querystring_pos_start + 1; ii < querystring_pos_end; ii++)
        {
            if (url[ii] == '#')
            {
                break;
            }
            if (url[ii] == ' ')
            {
                ss << "%20";
            }
            else if (url[ii] == '\"')
            {
                ss << "%22";
            }
            else if (url[ii] == '\'')
            {
                ss << "%27";
            }
            else if (url[ii] == '<')
            {
                ss << "%3C";
            }
            else if (url[ii] == '>')
            {
                ss << "%3E";
            }
            else if (url[ii] == '+')
            {
                ss << "";
            }
            else
            {
                ss << url[ii];
            }
        }
        std::string encoded_query_string = ss.str();
        querystring = encoded_query_string;
        return encoded_query_string;
    }

    size_t set_fragment_(std::string const &url, const size_t pos)
    {
        size_t fragment_pos_start = url.find_first_of('#'),
                fragment_pos_end = url.find_first_of('?', fragment_pos_start);
        if (fragment_pos_start == std::string::npos)
        {
            return pos;
        }
        if (fragment_pos_end == std::string::npos)
        {
            fragment_pos_end = url.size();
        }
        for (size_t ii = fragment_pos_start + 1; ii < url.size(); ii++)
        {
            if (!is_fragment(url[ii]))
            {
                throw std::runtime_error(static_cast<const std::ostringstream&>(
                                                 std::ostringstream() << "invalid character '" << url[ii]
                                                                      << "' found in fragment of '" << url
                                                                      << "'").str());
            }
        }
        fragment = url.substr(fragment_pos_start, fragment_pos_end - fragment_pos_start);
        return fragment_pos_end;
    }

    // Our validate function, we are going to
    // throw an exception if our url fails.
    // https://isocpp.org/wiki/faq/exceptions#ctors-can-throw
    void validate_(std::string const &url)
    {
        data_ = url;
        encoded_data_ = url;
        if (encoded_data_.empty())
        {
            throw std::runtime_error("empty string is not a valid url");
        }
        if (encoded_data_.size() < 5)
        {
            throw std::runtime_error(static_cast<const std::ostringstream&>(
                                             std::ostringstream() << "invalid url found, string '"
                                                                  << url << "' is too short").str());
        }
        size_t pos;
        set_protocol_(url);
        pos = set_domain_(url, check_protocol_domain_separator_(url));
        if (pos == url.size())
        {
            return;
        }

        pos = set_path_(url, pos);
        pos = set_querystring_(url, pos);
        encode_querystring_(url);
        pos = set_fragment_(url, pos);
        encoded_data_ = protocol + "://" + domain + path + querystring + fragment;
        if (pos != url.size())
        {
            throw std::runtime_error("unexpected characters in url after domain, expected ? or #");
        }
    }
};

inline std::string to_string(const URL& url) {
    return url.encoded();
}

/*
static std::ostream &operator<<(std::ostream &out, URL &url)
{
#ifdef TEST_URL
    return url.print(out);
#else
    out << url.encoded();
    return out;
#endif
}
*/

/*
static std::ostream &operator<<(std::ostream &out, URL const &url)
{
#ifdef TEST_URL
    return url.print(out);
#else
    out << url.encoded();
    return out;
#endif
}
*/

/*
static std::ostream &operator<<(std::ostream &out, URL *url)
{
#ifdef TEST_URL
    return url->print(out);
#else
    out << url->encoded();
    return out;
#endif
}
*/

/*
static std::ostream &operator<<(std::ostream &out, URL const *url)
{
#ifdef TEST_URL
    return url->print(out);
#else
    out << url->encoded();
    return out;
#endif
}
*/

#ifdef TEST_URL
#define ASSERT_EXCEPTION(x)                                                  \
    do                                                                       \
    {                                                                        \
        try                                                                  \
        {                                                                    \
            x;                                                               \
            throw std::runtime_error("unexpected exception raised in test"); \
        }                                                                    \
        catch (...)                                                          \
        {                                                                    \
        }                                                                    \
    } while (0)

int main()
{
    {
        ASSERT_EXCEPTION(URL(""));
    }
    {
        ASSERT_EXCEPTION(URL("abc"));
    }
    {
        ASSERT_EXCEPTION(URL("/abc"));
    }
    {
        ASSERT_EXCEPTION(URL("http"));
    }
    {
        ASSERT_EXCEPTION(URL("https"));
    }
    {
        ASSERT_EXCEPTION(URL("https://"));
    }
    {
        ASSERT_EXCEPTION(URL("http://"));
    }
    {
        ASSERT_EXCEPTION(URL("http:///"));
    }
    {
        ASSERT_EXCEPTION(URL("https:///"));
    }
    {
        ASSERT_EXCEPTION(URL("http://."));
    }
    {
        ASSERT_EXCEPTION(URL("https://a..."));
    }
    {
        ASSERT_EXCEPTION(URL("https://www.example_.com"));
    }
    {
        URL("https://www.example.com#abc");
    }
    {
        URL("https://www.example.com?key=value");
    }
    {
        Url a = URL("https://www.example.com?key=value'abc");
        std::cout << a << std::endl;
    }
    {
        Url a = URL("https://www.example.com/?key=value'abc#f");
        std::cout << a << std::endl;
    }
    {
        Url a = URL("https://31f5ff35.eu-gb.api.example.cloud/private-test/Hello");
        std::cout << a << std::endl;
    }
}
#endif

#endif /* OAUTH2_CPP_URL_H */
