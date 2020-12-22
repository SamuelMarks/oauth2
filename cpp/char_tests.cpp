
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
