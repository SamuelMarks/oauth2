
#include <string>

static inline bool is_whitespace(char ch) {
    return ch == 0x00|| ch == 0x20 || ch == 0x0A || ch == 0x0D || ch == 0x09;
}

static inline bool is_sign(char ch) {
    return ch == '+' || ch == '-';
}

static inline bool is_exponent(char ch) {
    return ch == 'e' || ch == 'E';
}

static inline bool is_decimal_point(char ch) {
    return ch == '.';
}

static inline bool is_quote(char ch) {
    return ch == '"' || ch == '\'';
}

static inline bool is_comma(char ch) {
    return ch == ',';
}

static inline bool is_colon(char ch) {
    return ch == ':';
}

static inline bool is_keyword(std::string& text) {
    return text == "true" || text == "false" || text == "null";
}

static inline bool is_alpha(char a) {
    return (a >= 'a' && a <= 'z') || (a >= 'A' && a <= 'Z');
}

static inline bool is_key(char a) {
    return (a >= 'a' && a <= 'z') || (a >= 'A' && a <= 'Z') || (a>= '0' && a <= '9') || a == '_';
}

static inline bool is_digit(char n) {
    return ( n >= '0' && n <= '9');
}

static inline bool is_domain_character(char a) {
    return is_alpha(a) || is_digit(a) ||  a == '-' || a == '.';
}


static inline bool is_sub_delims(char a) {
    return a == '!' || a == '$' || a == '&' || a == '\'' || a == '(' || a == ')' ||
            a == '*' || a == '+' || a == ',' || a == ';' || a == '=';
}

static inline bool is_hex_digit(char a ) {
    return a == 'a' || a == 'A' || 
            a == 'b' || a == 'B' || 
            a == 'c' || a == 'C' || 
            a == 'd' || a == 'D' || 
            a == 'e' || a == 'E' || 
            a == 'f' || a == 'F' || 
            is_digit(a);
}

static inline bool is_pct_encoded(char a ) {
    return a == '%' || is_hex_digit(a);
}

static inline bool is_unreserved(char a ) {
    return a == '-' || a == '.' || a == '_' || a == '~' || is_alpha(a) || is_digit(a);
}

static inline bool is_pchar(char a) {
    return a == ':' || a == '@' || is_unreserved(a) || is_pct_encoded(a) || is_sub_delims(a);
}

// See the RFC 3986.
static inline bool is_fragment(char a ) {
    return a == '/' || a == '?' || is_pchar(a);
}

static inline bool is_valid_path_char(char a) {
    return a == '/' || is_fragment(a);
}



