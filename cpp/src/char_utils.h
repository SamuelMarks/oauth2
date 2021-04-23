#ifndef OAUTH2_CHAR_UTILS_H
#define OAUTH2_CHAR_UTILS_H
#include <string>

static inline bool is_whitespace(char);

static inline bool is_sign(char);

static inline bool is_exponent(char);

static inline bool is_decimal_point(char);

static inline bool is_quote(char);

static inline bool is_comma(char);

static inline bool is_colon(char);

static inline bool is_keyword(const std::string&);

static inline bool is_alpha(char);

static inline bool is_key(char);

static inline bool is_digit(char);

static inline bool is_domain_character(char);

static inline bool is_sub_delims(char);

static inline bool is_hex_digit(char);

static inline bool is_pct_encoded(char);

static inline bool is_unreserved(char);

static inline bool is_pchar(char);

// See the RFC 3986.
static inline bool is_fragment(char);

static inline bool is_valid_path_char(char);

static inline bool is_whitespace(const char ch) {
    return ch == 0x00|| ch == 0x20 || ch == 0x0A || ch == 0x0D || ch == 0x09;
}

static inline bool is_sign(const char ch) {
    return ch == '+' || ch == '-';
}

static inline bool is_exponent(const char ch) {
    return ch == 'e' || ch == 'E';
}

static inline bool is_decimal_point(const char ch) {
    return ch == '.';
}

static inline bool is_quote(const char ch) {
    return ch == '"' || ch == '\'';
}

static inline bool is_comma(const char ch) {
    return ch == ',';
}

static inline bool is_colon(const char ch) {
    return ch == ':';
}

static inline bool is_keyword(const std::string& text) {
    return text == "true" || text == "false" || text == "null";
}

static inline bool is_alpha(const char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}

static inline bool is_key(const char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch>= '0' && ch <= '9') || ch == '_';
}

static inline bool is_digit(const char n) {
    return n >= '0' && n <= '9';
}

static inline bool is_domain_character(const char ch) {
    return is_alpha(ch) || is_digit(ch) || ch == '-' || ch == '.';
}


static inline bool is_sub_delims(const char ch) {
    return ch == '!' || ch == '$' || ch == '&' || ch == '\'' || ch == '(' || ch == ')' ||
           ch == '*' || ch == '+' || ch == ',' || ch == ';' || ch == '=';
}

static inline bool is_hex_digit(const char ch) {
    return ch == 'a' || ch == 'A' ||
           ch == 'b' || ch == 'B' ||
           ch == 'c' || ch == 'C' ||
           ch == 'd' || ch == 'D' ||
           ch == 'e' || ch == 'E' ||
           ch == 'f' || ch == 'F' ||
           is_digit(ch);
}

static inline bool is_pct_encoded(const char ch) {
    return ch == '%' || is_hex_digit(ch);
}

static inline bool is_unreserved(const char ch) {
    return ch == '-' || ch == '.' || ch == '_' || ch == '~' || is_alpha(ch) || is_digit(ch);
}

static inline bool is_pchar(const char ch) {
    return ch == ':' || ch == '@' || is_unreserved(ch) || is_pct_encoded(ch) || is_sub_delims(ch);
}

// See the RFC 3986.
static inline bool is_fragment(const char ch) {
    return ch == '/' || ch == '?' || is_pchar(ch);
}

static inline bool is_valid_path_char(const char ch) {
    return ch == '/' || is_fragment(ch);
}

#endif /* OAUTH2_CHAR_UTILS_H */
