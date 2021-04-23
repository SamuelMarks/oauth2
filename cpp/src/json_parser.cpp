// ----------------------------------------------------------
// Tiny JSON Parser
// Written in a C-style rather than OOP.
// I use a macro INTERNAL to indicate 'static'
// I use a macro ENTRYPOINT that indicates that this is the main entry to use the API
// ----------------------------------------------------------

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

#ifdef TEST_JSON
#include "char_tests.cpp"
#endif
#include "macros.h"


struct JsonItem;
struct JsonParseContext;
using JsonArray = std::vector<JsonItem>;            
using JsonObject = std::map<std::string, JsonItem>; 

#undef ERROR

enum class JsonItemType {
    NULL_VALUE = 0,
    TRUE_VALUE,
    FALSE_VALUE,
    TEXT,
    INTEGER,
    FLOAT,
    ARRAY,
    OBJECT,
    EMPTY,
    ERROR,
    END_OF_JSON_VALUES
};

struct JsonItem {
    JsonItemType type;
    std::string text;
    long integer;
    double real;
    JsonArray array;
    JsonObject object;
};


struct JsonParseContext 
{
    std::string buffer;
    size_t pos;
    size_t selection_start_pos;
    size_t selection_end_pos;
    bool error;
    size_t error_pos_start;
    size_t error_pos_end;
    std::string error_message;
};

// forward declare functions due to C/C++ ordering requirements
INTERNAL JsonItem json_create_from_string_buffer(JsonParseContext& context);
JsonItem json_create_from_string(std::string const & buffer);

INTERNAL
JsonItem json_create_new() {
    JsonItem item = JsonItem {};
    item.type = JsonItemType::END_OF_JSON_VALUES;
    return item;
}

INTERNAL
std::string json_get_selected_text(JsonParseContext& context) {
    return context.buffer.substr(context.selection_start_pos, context.selection_end_pos - context.selection_start_pos+1);
}

INTERNAL
void json_parse_text_value(JsonParseContext& context, JsonItem& item) 
{
    // assume index it currently pointing to the first quote indicating a string
    context.selection_start_pos = context.pos + 1;
    size_t index = context.pos+1;
    char open_quote = context.buffer[context.pos];
    while( true ) {
        index++;
        if ( index >= context.buffer.size() ) {
            context.error = true;
            context.error_pos_start = context.selection_start_pos;
            context.error_pos_end = index-1;
            context.error_message = "String without final quotes was detected.";
            return;
        }
        if ( context.buffer[index] == '\\') {
            // escape character
            index++;
        } else if ( context.buffer[index] == open_quote) {
            break;
        }
    }
    context.selection_end_pos = index - 1;
    context.pos = index+1; // move on passed the last quote
    item.type = JsonItemType::TEXT;
    item.text = json_get_selected_text(context);
}


INTERNAL
void json_parse_key(JsonParseContext& context, JsonItem& item) 
{
    // assume index it currently pointing to the first quote indicating a string
    context.selection_start_pos = context.pos;
    size_t index = context.pos;
    while( index++ ) {
        if ( index >= context.buffer.size() ) {
            context.error = true;
            context.error_pos_start = context.selection_start_pos;
            context.error_pos_end = index-1;
            context.error_message = "String without end was detected.";
            return;
        }
        if ( !is_key(context.buffer[index]) ) {
            break;
        }
    }
    context.selection_end_pos = index - 1;
    context.pos = index;
    item.type = JsonItemType::TEXT;
    item.text = json_get_selected_text(context);
}


INTERNAL
void json_parse_number_value(JsonParseContext& context, JsonItem& item) 
{
    size_t index = context.pos;
    context.selection_start_pos = context.pos;
    for( ;index < context.buffer.size() || is_digit(context.buffer[index]); ++index) {}
    if ( index < context.buffer.size() && is_decimal_point(context.buffer[index]) ) {
        // float
        for(;index < context.buffer.size() || is_digit(context.buffer[index]) ; ++index) {}
        context.selection_end_pos = index-1;
        context.pos = index;
        item.type = JsonItemType::FLOAT;
        std::string val = json_get_selected_text(context);
        item.real = std::stof(val);
    } else {
        context.selection_end_pos = index-1;
        context.pos = index;
        item.type = JsonItemType::INTEGER;
        std::string val = json_get_selected_text(context);
        item.integer = std::stol(val);
    }
}


INTERNAL inline
void json_increment_context(JsonParseContext& context) {
    context.pos++;
}

INTERNAL inline
void json_eat_whitespace(JsonParseContext& context) {
    while ( is_whitespace(context.buffer[context.pos]) ) {
        json_increment_context(context);
    }
}

INTERNAL inline
void json_eat_commas(JsonParseContext& context) {
    while ( is_comma(context.buffer[context.pos]) ) {
        json_increment_context(context);
    }
}

INTERNAL inline
void json_eat_colons(JsonParseContext& context) {
    while ( is_colon(context.buffer[context.pos]) ) {
        json_increment_context(context);
    }
}

void json_parse_object_value(JsonParseContext& context, JsonItem& objectItem ) {
    // comma separated list of items
    json_eat_whitespace(context);
    objectItem.type = JsonItemType::OBJECT;
    context.selection_start_pos = context.pos;
    context.pos++;
    while(true) {
        JsonItem new_key = json_create_from_string_buffer(context);
        if ( new_key.type != JsonItemType::TEXT && new_key.type != JsonItemType::INTEGER ) {
            context.error_message = "Key must be a string.";
            return;
        }
        if ( new_key.type == JsonItemType::INTEGER ) {
            new_key.text = std::to_string(new_key.integer);
        }
        json_eat_whitespace(context);
        if ( is_colon(context.buffer[context.pos]) ) {
            json_eat_colons(context);
        }
        json_eat_whitespace(context);
        JsonItem new_value = json_create_from_string_buffer(context);
        if ( new_value.type == JsonItemType::END_OF_JSON_VALUES ) {
            context.error = true;
            context.error_pos_start = context.selection_start_pos;
            context.error_pos_end = context.pos;
            context.error_message = "No value found for object key-value pair.";
            return;
        }
        objectItem.object.insert(std::make_pair(new_key.text, new_value));
        json_eat_whitespace(context);
        if ( is_comma(context.buffer[context.pos]) ) {
            json_eat_commas(context);
        } else {
            if ( context.buffer[context.pos] == '}' ) {
                return;
            } else {
                context.error_message = "Expected '}' for end of object";
                return;
            }
        }
        json_eat_whitespace(context);
    }
}

INTERNAL
void json_parse_array_value(JsonParseContext& context, JsonItem& arrayItem ) {
    // comma separated list of items
    json_eat_whitespace(context);
    arrayItem.type = JsonItemType::ARRAY;
    while(true) {
        context.pos++;
        JsonItem new_item = json_create_from_string_buffer(context);
        if ( new_item.type == JsonItemType::END_OF_JSON_VALUES ) {
            return;
        }
        arrayItem.array.push_back(new_item);
    }
}

/*
INTERNAL
void parse_json_text(JsonItem& textItem, std::string const & buffer, size_t& index )
{
    auto start = index;
    while( index < buffer.size() ) {
        index++;
        if ( buffer[index] == '\\') {
            // escape character so we will ignore the next
            // character
        } else {
            if ( buffer[index] == buffer[start] ) {
                break;
            }
        }
    }
    textItem.text = buffer.substr(start, index - start);
}
*/

INTERNAL
JsonItem json_create_from_string_buffer(JsonParseContext& context) {
    std::stringstream in = std::stringstream {};
    JsonItem item = json_create_new();
    json_eat_whitespace(context);
    context.selection_start_pos = context.pos;
    while( context.pos < context.buffer.size() ) {
        if ( context.buffer[context.pos] == '{' ) {
            json_parse_object_value(context, item);
            return item;
        } else if ( context.buffer[context.pos] == '[' ) {
            json_parse_array_value(context, item);
            return item;
        } else if ( context.buffer[context.pos] == '}' ||
                    context.buffer[context.pos] == ']' ||
                    context.buffer[context.pos] == ',' ) {
            return item;
        } else if ( context.pos < context.buffer.size() - 3 && 
                    context.buffer[context.pos] == 'n' && 
                    context.buffer[context.pos+1] == 'u' && 
                    context.buffer[context.pos+2] == 'l' && 
                    context.buffer[context.pos+3] == 'l') {
            item.type = JsonItemType::NULL_VALUE;
            context.pos += 4;
            return item;} else if ( context.pos < context.buffer.size() - 3 && 
                    context.buffer[context.pos] == 't' && 
                    context.buffer[context.pos+1] == 'r' && 
                    context.buffer[context.pos+2] == 'u' && 
                    context.buffer[context.pos+3] == 'e') {
            item.type = JsonItemType::TRUE_VALUE;
            context.pos += 4;
            return item;
         } else if ( context.pos < context.buffer.size() - 4 && 
                    context.buffer[context.pos] == 'f' && 
                    context.buffer[context.pos+1] == 'a' && 
                    context.buffer[context.pos+2] == 'l' && 
                    context.buffer[context.pos+3] == 's' && 
                    context.buffer[context.pos+4] == 'e') {
            item.type = JsonItemType::FALSE_VALUE;
            context.pos += 5;
            return item;
        } else if ( is_quote(context.buffer[context.pos]) ) {
            json_parse_text_value(context, item);
            return item;
        } else if ( is_alpha(context.buffer[context.pos]) ) {
            // are we in an object? 
            // if not then error
            json_parse_key(context, item);
            return item;
        } else if ( is_digit(context.buffer[context.pos]) ) {
            json_parse_number_value(context, item);
            if ( item.type == JsonItemType::INTEGER || item.type == JsonItemType::FLOAT ) {
                return item;
            }
        } else if ( is_whitespace(context.buffer[context.pos]) ) {
            // eat whitespace 
        } else {
            context.error = true;
            context.error_pos_start = context.pos;
            context.error_pos_end = context.pos;
            context.error_message = "invalid character found";
            return item;
        }
        context.pos++;
    }
    return item;
}

// entry point
ENTRYPOINT
JsonItem json_create_from_string(std::string const & buffer) {
    JsonParseContext context = JsonParseContext {};
    context.buffer = buffer;
    context.error_pos_start = 0;
    context.error_pos_end = 0;
    context.selection_start_pos = 0;
    context.selection_end_pos = 0;
    context.error = false;
    JsonItem item = json_create_from_string_buffer(context);
    if ( context.error ) {
        item.type = JsonItemType::ERROR;
        item.text = static_cast<const std::ostringstream&>(
                std::ostringstream() << "Error at " << context.error_pos_start << ","
                                     << context.error_pos_end << ": " << context.error_message).str();
    }
    return item;
}

INTERNAL
std::string json_json_pretty_print_item(JsonItem const & json, const size_t indent) {
    switch( json.type ) {
    case JsonItemType::EMPTY:
        return {};
    case JsonItemType::NULL_VALUE:
        return "null";
    case JsonItemType::TRUE_VALUE:
        return "true";
    case JsonItemType::FALSE_VALUE:
        return "false";
    case JsonItemType::TEXT:{
        return static_cast<const std::ostringstream&>(
                std::ostringstream() << '"' << json.text << '"').str();
    }
    case JsonItemType::INTEGER:
    case JsonItemType::FLOAT:
        return std::to_string(json.integer);
    case JsonItemType::ARRAY:{
        std::ostringstream in;
        in << "[";
        if ( json.array.empty() ) {
            in << "]";
            return in.str();
        }
        const std::string indentation = std::string( (indent+1) * 2, ' ');
        in << '\n';
        size_t counter = 0;
        for_each(begin(json.array), end(json.array), [&](JsonItem const & item) {
            in << indentation << json_json_pretty_print_item(item, indent+1);
            if ( counter < json.array.size()-1 ) {
                in << ",\n";
            } else {
                in << '\n';
            }
            counter++;
        });
        const std::string last_indentation = std::string( indent * 2, ' ');
        in << last_indentation << "]";
        return in.str();
    }
    case JsonItemType::OBJECT:{
        std::ostringstream in;
        in << "{";
        if ( json.object.empty() ) {
            in << "}";
            return in.str();
        }
        const std::string indentation = std::string( (indent+1) * 2, ' ');
        in << '\n' << indentation;
        size_t counter = 0;
        for( auto const & [key,val] : json.object ) {
            in << key << " : " << json_json_pretty_print_item(val, indent+1);
            if ( counter < json.object.size()-1 ) {
                in << ",\n" << indentation;
            } else {
                in << "\n";
            }
            counter++;
        };
        const std::string last_indentation = std::string( indent * 2, ' ');
        in << last_indentation << "}";
        return in.str();
    }
    case JsonItemType::END_OF_JSON_VALUES:
        return "<EMPTY>";
    case JsonItemType::ERROR:
        return json.text;
    default:
        return static_cast<const std::ostringstream&>(
                std::ostringstream() << "invalid type found '" << (int) json.type).str();
    }
}

INTERNAL
void json_pretty_print(JsonItem const & json ) {
    std::cout << json_json_pretty_print_item(json, 0) << '\n';
}

#ifdef TEST_JSON
int main() {
#if 1
    {
        std::string test = "";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "null";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "true";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "false";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "[]";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "{}";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "1234567890";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "123.4567890";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "\"a string\"";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "\"a multiline\nstring\"";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "\"a string with a \\\"quote\\\" in it\"";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "[0,1,2,3,4,5,6,7,8,9]";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "[0,1,2,3,4,5,6,7,8,[[[5]]]]";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "{ keyonly }";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "{ key: value }";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "{ key: 9 }";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "{ key: 9, 1: 14 }";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "{ key: 9, 1: [14,2,3] }";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }

    {
        std::string test = "{ key: 9, 1: [14,2,3], \"a\":\"b\", g: h }";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
#endif
    {
        std::string test = "\"''\"";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
    {
        std::string test = "'\"'";
        JsonItem json = json_create_from_string(test);
        json_pretty_print(json);
    }
}
#endif
