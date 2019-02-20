#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>

#include "../ツ/ツ.types.h"

internal std::string header_text = R"(/************************************************
 NOTE: This file is generated. Try not to modify!
 ************************************************/

#pragma once

#if !defined(VKAPI_CALL)
    #define VKAPI_CALL
#endif

#if !defined(VKAPI_PTR)
    #define VKAPI_PTR
#endif
)";

internal std::string vulkan_function_list_header = R"(
#define VK_FUNCTION(function) external PFN_##function function;
    #define VK_FUNCTIONS_PLATFORM          \
        VK_FUNCTION(vkGetInstanceProcAddr) \
        VK_FUNCTION(vkCreateInstance)
    VK_FUNCTIONS_PLATFORM

    #if defined(DEBUG)
        #define VK_FUNCTIONS_DEBUG VK_FUNCTION(vkCreateDebugUtilsMessengerEXT)
    #else
        #define VK_FUNCTIONS_DEBUG
    #endif
    VK_FUNCTIONS_DEBUG
)";

char* read_file(wchar_t* filepath)
{
    FILE* file = _wfopen(filepath, L"rb");
    if (!file)
    {
        fprintf(stderr, "%ws not found :(\n", filepath);
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    u64 size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* result = (char*)malloc(size + 1);
    fread(result, size, 1, file);
    fclose(file);
    result[size] = 0;

    return result;
}

void write_file(wchar_t* filepath, const char* contents)
{
    FILE* file = _wfopen(filepath, L"w");
    if (!file)
    {
        fprintf(stderr, "Unable to create %ws :(\n", filepath);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%s", contents);
    fclose(file);
}

enum Token_Type
{
    TOKEN_TYPE_UNKNOWN,

    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_STRING,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_OPEN_PARENTHESIS,
    TOKEN_TYPE_CLOSE_PARENTHESIS,
    TOKEN_TYPE_OPEN_BRACE,
    TOKEN_TYPE_CLOSE_BRACE,
    TOKEN_TYPE_OPEN_BRACKET,
    TOKEN_TYPE_CLOSE_BRACKET,
    TOKEN_TYPE_COMMA,
    TOKEN_TYPE_SEMICOLON,
    TOKEN_TYPE_COLON,
    TOKEN_TYPE_ASTERISK,
    TOKEN_TYPE_EQUAL,
    TOKEN_TYPE_HASH,

    TOKEN_TYPE_END_OF_STREAM
};

struct Token
{
    Token_Type type;

    char* text;
    u64   length;
};

struct Tokenizer
{
    char* at;
};

internal b32 is_end_of_line(char c)
{
    return c == '\n' || c == '\r';
}

internal b32 is_whitespace(char c)
{
    return is_end_of_line(c) || c == ' ' || c == '\t';
}

internal b32 is_alpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

internal b32 is_number(char c)
{
    return c >= '0' && c <= '9';
}

internal b32 is_hexadecimal(char c)
{
    return is_number(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

internal void advance_tokenizer(Tokenizer* tokenizer, u64 count)
{
    tokenizer->at += count;
}

internal void eat_whitespace_and_comments(Tokenizer* tokenizer)
{
    while (true)
    {
        if (is_whitespace(*tokenizer->at))
        {
            advance_tokenizer(tokenizer, 1);
        }
        else if (tokenizer->at[0] == '/' && tokenizer->at[1] == '/')
        {
            advance_tokenizer(tokenizer, 2);
            while (*tokenizer->at && !is_end_of_line(*tokenizer->at))
            {
                advance_tokenizer(tokenizer, 1);
            }
        }
        else if (tokenizer->at[0] == '/' && tokenizer->at[1] == '*')
        {
            advance_tokenizer(tokenizer, 2);
            while (*tokenizer->at && !(tokenizer->at[0] == '*' && tokenizer->at[1] == '/'))
            {
                advance_tokenizer(tokenizer, 1);
            }
            if (*tokenizer->at == '*')
            {
                advance_tokenizer(tokenizer, 2);
            }
        }
        else
        {
            break;
        }
    }
}

internal Token get_token(Tokenizer* tokenizer)
{
    eat_whitespace_and_comments(tokenizer);

    char character = *tokenizer->at;
    Token token = {TOKEN_TYPE_UNKNOWN, tokenizer->at, 1};
    advance_tokenizer(tokenizer, 1);

    switch (character)
    {
    case '(': token.type = TOKEN_TYPE_OPEN_PARENTHESIS; break;
    case ')': token.type = TOKEN_TYPE_CLOSE_PARENTHESIS; break;
    case '{': token.type = TOKEN_TYPE_OPEN_BRACE; break;
    case '}': token.type = TOKEN_TYPE_CLOSE_BRACE; break;
    case '[': token.type = TOKEN_TYPE_OPEN_BRACKET; break;
    case ']': token.type = TOKEN_TYPE_CLOSE_BRACKET; break;
    case ',': token.type = TOKEN_TYPE_COMMA; break;
    case ';': token.type = TOKEN_TYPE_SEMICOLON; break;
    case ':': token.type = TOKEN_TYPE_COLON; break;
    case '*': token.type = TOKEN_TYPE_ASTERISK; break;
    case '=': token.type = TOKEN_TYPE_EQUAL; break;
    case '#': token.type = TOKEN_TYPE_HASH; break;
    case '\0': token.type = TOKEN_TYPE_END_OF_STREAM; break;

    case '"':
    {
        while (*tokenizer->at && *tokenizer->at != '"')
        {
            if (tokenizer->at[0] == '\\' && tokenizer->at[1])
            {
                advance_tokenizer(tokenizer, 1);
            }
            advance_tokenizer(tokenizer, 1);
        }

        if (*tokenizer->at == '"')
        {
            advance_tokenizer(tokenizer, 1);
        }
        token.type = TOKEN_TYPE_STRING;
        token.length = tokenizer->at - token.text;
    } break;

    default:
    {
        if (is_alpha(character))
        {
            while (is_alpha(*tokenizer->at) || is_number(*tokenizer->at) || *tokenizer->at == '_')
            {
                advance_tokenizer(tokenizer, 1);
            }

            token.type = TOKEN_TYPE_IDENTIFIER;
            token.length = tokenizer->at - token.text;
        }
        else if (is_number(character))
        {
            while (is_number(*tokenizer->at))
            {
                advance_tokenizer(tokenizer, 1);
            }

            if (*tokenizer->at == '.' || *tokenizer->at == 'x')
            {
                advance_tokenizer(tokenizer, 1);
                while (is_hexadecimal(*tokenizer->at))
                {
                    advance_tokenizer(tokenizer, 1);
                }
            }

            token.type = TOKEN_TYPE_NUMBER;
            token.length = tokenizer->at - token.text;
        }
    } break;
    }

    return token;
}

internal b32 eat_token(Tokenizer* tokenizer, Token_Type token_type)
{
    Token token = get_token(tokenizer);
    return token.type == token_type;
}

internal b32 token_equals(Token token, char* token_text)
{
    for (u64 i = 0; i < token.length; ++i)
    {
        if (token.text[i] != token_text[i])
        {
            return false;
        }
    }
    return true;
}

internal b32 token_starts_with(Token token, char* prefix, u64 prefix_length)
{
    for (u64 i = 0; i < prefix_length; ++i)
    {
        if (token.text[i] != prefix[i])
        {
            return false;
        }
    }
    return true;
}

struct Struct_Member
{
    std::string type;
    std::string name;
};

struct Struct
{
    std::string type;
    b32         is_union;

    std::vector<Struct_Member> members;
};

struct Enum_Member
{
    std::string name;
    std::string value;
};

struct Enum
{
    std::string type;

    std::vector<Enum_Member> members;
};

struct Macro
{
    std::string name;
    std::string body;
    std::vector<std::string> parameters;
};

struct Typedef
{
    std::string name;
    std::string value;
};

struct Function
{
    std::string name;
    std::string return_type;

    std::vector<Struct_Member> parameters;
};

std::unordered_map<std::string, Typedef>     vulkan_typedef_map;
std::unordered_map<std::string, Struct>      vulkan_struct_map;
std::unordered_map<std::string, Enum>        vulkan_enum_map;
std::unordered_map<std::string, Enum>        vulkan_enum_value_to_enum_map;
std::unordered_map<std::string, Macro>       vulkan_define_macro_map;
std::unordered_map<std::string, std::string> vulkan_define_macro_usage_map;
std::unordered_map<std::string, Function>    vulkan_function_map;

void parse_vulkan_struct_member(Tokenizer* tokenizer, Struct_Member* member)
{
    member->name = "";

    b32 parsing = true;
    while (parsing)
    {
        Token token = get_token(tokenizer);
        switch (token.type)
        {
        case TOKEN_TYPE_SEMICOLON:
        case TOKEN_TYPE_END_OF_STREAM:
        {
            parsing = false;
        } break;
        default:
        {
            member->name += std::string(token.text, token.length);
        } break;
        }
    }
}

void parse_vulkan_struct(Tokenizer* tokenizer, b32 is_union)
{
    Token name = get_token(tokenizer);
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        Struct s = {};
        s.type = std::string(name.text, name.length);
        s.is_union = is_union;

        while (true)
        {
            Token member = get_token(tokenizer);
            if (member.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            Struct_Member struct_member = {};
            struct_member.type = std::string(member.text, member.length);

            parse_vulkan_struct_member(tokenizer, &struct_member);
            s.members.push_back(struct_member);
        }
        vulkan_struct_map[s.type] = s;
    }
}

b32 parse_vulkan_enum_member(Tokenizer* tokenizer, Enum_Member* member)
{
    member->value = "";
    b32 is_value = false;

    b32 parsing = true;
    while (parsing)
    {
        Token token = get_token(tokenizer);
        switch (token.type)
        {
        case TOKEN_TYPE_CLOSE_BRACE:
            return false;
        case TOKEN_TYPE_COMMA:
        case TOKEN_TYPE_END_OF_STREAM:
            parsing = false;
            break;
        case TOKEN_TYPE_EQUAL:
            is_value = true;
            break;
        default:
        {
            if (is_value)
            {
                member->value += std::string(token.text, token.length);
            }
        } break;
        }
    }
    return true;
}

void parse_vulkan_enum(Tokenizer* tokenizer)
{
    Token name = get_token(tokenizer);
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        Enum e = {};
        e.type = std::string(name.text, name.length);

        b32 parsing = true;
        while (parsing)
        {
            Token member = get_token(tokenizer);
            if (member.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            Enum_Member enum_member = {};
            enum_member.name = std::string(member.text, member.length);

            parsing = parse_vulkan_enum_member(tokenizer, &enum_member);
            e.members.push_back(enum_member);
        }
        vulkan_enum_map[e.type] = e;

        for (u64 i = 0; i < e.members.size(); ++i)
        {
            vulkan_enum_value_to_enum_map[e.members[i].name] = e;
        }
    }
}

b32 parse_vulkan_function_parameter(Tokenizer* tokenizer, Struct_Member* param)
{
    param->name = "";

    b32 parsing = true;
    while (parsing)
    {
        Token token = get_token(tokenizer);
        switch (token.type)
        {
        case TOKEN_TYPE_CLOSE_PARENTHESIS:
            return false;
        case TOKEN_TYPE_COMMA:
        case TOKEN_TYPE_END_OF_STREAM:
        {
            parsing = false;
        } break;
        default:
        {
            param->name += std::string(token.text, token.length);
        } break;
        }
    }
    return true;
}

void parse_vulkan_regular_typedef(Tokenizer* tokenizer, std::string value)
{
    Token name = get_token(tokenizer);

    if (name.type == TOKEN_TYPE_IDENTIFIER)
    {
        Typedef t = {};
        t.value = value;
        t.name = std::string(name.text, name.length);

        if (vulkan_typedef_map.find(t.name) == vulkan_typedef_map.end())
        {
            vulkan_typedef_map[t.name] = t;
        }
    }
    else if (name.type == TOKEN_TYPE_ASTERISK || name.type == TOKEN_TYPE_OPEN_PARENTHESIS)
    {
        Function f = {};
        f.return_type = value;

        if (name.type == TOKEN_TYPE_ASTERISK)
        {
            f.return_type += '*';
            eat_token(tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS);
        }

        if (eat_token(tokenizer, TOKEN_TYPE_IDENTIFIER) && eat_token(tokenizer, TOKEN_TYPE_ASTERISK))
        {
            Token token = get_token(tokenizer);
            if (token.type == TOKEN_TYPE_IDENTIFIER)
            {
                f.name = std::string(token.text, token.length);
                if (eat_token(tokenizer, TOKEN_TYPE_CLOSE_PARENTHESIS) && eat_token(tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS))
                {
                    b32 parsing = true;
                    while (parsing)
                    {
                        Token parameter = get_token(tokenizer);
                        if (parameter.type == TOKEN_TYPE_CLOSE_PARENTHESIS)
                        {
                            break;
                        }

                        Struct_Member function_parameter = {};
                        function_parameter.type = std::string(parameter.text, parameter.length);

                        parsing = parse_vulkan_function_parameter(tokenizer, &function_parameter);
                        f.parameters.push_back(function_parameter);
                    }

                    vulkan_function_map[f.name] = f;
                }
            }
        }
    }
}

void parse_vulkan_typedef(Tokenizer* tokenizer)
{
    Token token = get_token(tokenizer);
    if (token_equals(token, "struct"))
    {
        parse_vulkan_struct(tokenizer, false);
    }
    else if (token_equals(token, "union"))
    {
        parse_vulkan_struct(tokenizer, true);
    }
    else if (token_equals(token, "enum"))
    {
        parse_vulkan_enum(tokenizer);
    }
    else
    {
        parse_vulkan_regular_typedef(tokenizer, std::string(token.text, token.length));
    }
}

void parse_vulkan_macro(Tokenizer* tokenizer)
{
    Token token = get_token(tokenizer);
    if (token_equals(token, "define"))
    {
        Token name = get_token(tokenizer);

        Macro m = {};
        m.name = std::string(name.text, name.length);

        char last = *tokenizer->at;
        Token next = get_token(tokenizer);
        if (next.type == TOKEN_TYPE_OPEN_PARENTHESIS && !is_whitespace(last))
        {
            while (true)
            {
                Token param = get_token(tokenizer);
                if (param.type == TOKEN_TYPE_CLOSE_PARENTHESIS)
                {
                    next = param;
                    break;
                }
                else if (param.type == TOKEN_TYPE_IDENTIFIER)
                {
                    m.parameters.push_back(std::string(param.text, param.length));
                }
            }

            advance_tokenizer(tokenizer, 1);
            next.text = tokenizer->at;
        }

        char* start = next.text;
        while (*tokenizer->at && !is_end_of_line(*tokenizer->at))
        {
            if (tokenizer->at[0] == '\\' && tokenizer->at[1])
            {
                advance_tokenizer(tokenizer, 1);
                if (is_end_of_line(*tokenizer->at))
                {
                    advance_tokenizer(tokenizer, 1);
                }
            }
            advance_tokenizer(tokenizer, 1);
        }

        m.body = std::string(start, tokenizer->at - start);
        if (vulkan_define_macro_map.find(m.name) == vulkan_define_macro_map.end())
        {
            vulkan_define_macro_map[m.name] = m;
        }
    }
}

void parse_vulkan_macro_usage(Tokenizer* tokenizer, std::string macro)
{
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS))
    {
        Token parameter = get_token(tokenizer);
        std::string parameter_name = std::string(parameter.text, parameter.length);
        vulkan_define_macro_usage_map[parameter_name] = macro;
    }
}

void parse_vulkan_header(wchar_t* filepath)
{
    char* vulkan_header = read_file(filepath);

    Tokenizer tokenizer = {vulkan_header};

    b32 parsing = true;
    while (parsing)
    {
        Token token = get_token(&tokenizer);
        switch (token.type)
        {
        case TOKEN_TYPE_END_OF_STREAM:
            parsing = false;
            break;
        case TOKEN_TYPE_UNKNOWN:
            break;
        case TOKEN_TYPE_HASH:
        {
            parse_vulkan_macro(&tokenizer);
        } break;
        case TOKEN_TYPE_IDENTIFIER:
        {
            std::string token_name = std::string(token.text, token.length);
            if (token_equals(token, "typedef"))
            {
                parse_vulkan_typedef(&tokenizer);
            }
            else if (vulkan_define_macro_map.find(token_name) != vulkan_define_macro_map.end())
            {
                if (vulkan_define_macro_map[token_name].parameters.size() == 1)
                {
                    parse_vulkan_macro_usage(&tokenizer, token_name);
                }
            }
        } break;
        }
    }
}

std::unordered_map<std::string, Typedef>     needed_vulkan_typedefs;
std::unordered_map<std::string, Struct>      needed_vulkan_structs;
std::unordered_map<std::string, Enum>        needed_vulkan_enums;
std::unordered_map<std::string, Macro>       needed_vulkan_macros;
std::unordered_map<std::string, std::string> needed_vulkan_macro_usages;
std::unordered_map<std::string, Function>    needed_vulkan_functions;

enum Vulkan_Item_Type
{
    VULKAN_ITEM_TYPE_TYPEDEF,
    VULKAN_ITEM_TYPE_STRUCT,
    VULKAN_ITEM_TYPE_ENUM,
    VULKAN_ITEM_TYPE_MACRO,
    VULKAN_ITEM_TYPE_MACRO_USAGE,
    VULKAN_ITEM_TYPE_FUNCTION
};
struct Vulkan_Item
{
    Vulkan_Item_Type type;
    std::string name;
};

std::vector<Vulkan_Item> needed_vulkan_items_order;

void add_typedef(Typedef t)
{
    if (needed_vulkan_typedefs.find(t.value) == needed_vulkan_typedefs.end() && vulkan_typedef_map.find(t.value) != vulkan_typedef_map.end())
    {
        add_typedef(vulkan_typedef_map[t.value]);
    }
    if (needed_vulkan_typedefs.find(t.name) == needed_vulkan_typedefs.end())
    {
        needed_vulkan_typedefs[t.name] = t;
        needed_vulkan_items_order.push_back({VULKAN_ITEM_TYPE_TYPEDEF, t.name});
    }
}

void add_enum(Enum e)
{
    if (needed_vulkan_enums.find(e.type) == needed_vulkan_enums.end())
    {
        needed_vulkan_enums[e.type] = e;
        needed_vulkan_items_order.push_back({VULKAN_ITEM_TYPE_ENUM, e.type});
    }
}

void add_macro(Macro m)
{
    if (needed_vulkan_macros.find(m.name) == needed_vulkan_macros.end())
    {
        needed_vulkan_macros[m.name] = m;
        needed_vulkan_items_order.push_back({VULKAN_ITEM_TYPE_MACRO, m.name});
    }
}

void add_macro_usage(std::string u)
{
    if (needed_vulkan_macro_usages.find(u) == needed_vulkan_macro_usages.end())
    {
        needed_vulkan_macro_usages[u] = vulkan_define_macro_usage_map[u];
        add_macro(vulkan_define_macro_map[vulkan_define_macro_usage_map[u]]);
        needed_vulkan_items_order.push_back({VULKAN_ITEM_TYPE_MACRO_USAGE, u});
    }
    else
    {
        add_macro(vulkan_define_macro_map[vulkan_define_macro_usage_map[u]]);
    }
}

void add_function(Function f);

void add_struct(Struct s)
{
    for (u64 i = 0; i < s.members.size(); ++i)
    {
        std::string struct_type = s.members[i].type;
        if (struct_type == "const")
        {
            struct_type = s.members[i].name;
            struct_type = struct_type.substr(0, struct_type.find("*"));
        }
        else if (s.members[i].name.find("[") != -1)
        {
            u64 open = s.members[i].name.find("[") + 1;
            u64 close = s.members[i].name.find("]");
            std::string macro = s.members[i].name.substr(open, close - open);
            if (vulkan_define_macro_map.find(macro) != vulkan_define_macro_map.end())
            {
                add_macro(vulkan_define_macro_map[macro]);
            }
        }

        if (needed_vulkan_structs.find(struct_type) == needed_vulkan_structs.end() && vulkan_struct_map.find(struct_type) != vulkan_struct_map.end())
        {
            Struct dependency = vulkan_struct_map[struct_type];
            add_struct(dependency);
        }
        else if (vulkan_enum_map.find(struct_type) != vulkan_enum_map.end())
        {
            Enum dependency = vulkan_enum_map[struct_type];
            add_enum(dependency);
        }
        else if (vulkan_typedef_map.find(struct_type) != vulkan_typedef_map.end())
        {
            Typedef dependency = vulkan_typedef_map[struct_type];
            add_typedef(dependency);
        }
        else if (vulkan_function_map.find(struct_type) != vulkan_function_map.end())
        {
            Function dependency = vulkan_function_map[struct_type];
            add_function(dependency);
        }
        else if (vulkan_define_macro_usage_map.find(struct_type) != vulkan_define_macro_usage_map.end() && vulkan_define_macro_map.find(vulkan_define_macro_usage_map[struct_type]) != vulkan_define_macro_map.end())
        {
            add_macro_usage(struct_type);
        }
    }
    if (needed_vulkan_structs.find(s.type) == needed_vulkan_structs.end())
    {
        needed_vulkan_structs[s.type] = s;
        needed_vulkan_items_order.push_back({VULKAN_ITEM_TYPE_STRUCT, s.type});
    }
}

void add_function(Function f)
{
    if (needed_vulkan_typedefs.find(f.return_type) == needed_vulkan_typedefs.end() && vulkan_typedef_map.find(f.return_type) != vulkan_typedef_map.end())
    {
        add_typedef(vulkan_typedef_map[f.return_type]);
    }
    else if (needed_vulkan_structs.find(f.return_type) == needed_vulkan_structs.end() && vulkan_struct_map.find(f.return_type) != vulkan_struct_map.end())
    {
        add_struct(vulkan_struct_map[f.return_type]);
    }
    else if (needed_vulkan_enums.find(f.return_type) == needed_vulkan_enums.end() && vulkan_enum_map.find(f.return_type) != vulkan_enum_map.end())
    {
        add_enum(vulkan_enum_map[f.return_type]);
    }
    else if (vulkan_define_macro_usage_map.find(f.return_type) != vulkan_define_macro_usage_map.end() && vulkan_define_macro_map.find(vulkan_define_macro_usage_map[f.return_type]) != vulkan_define_macro_map.end())
    {
        add_macro_usage(f.return_type);
    }
    else if (needed_vulkan_functions.find(f.return_type) == needed_vulkan_functions.end() && vulkan_function_map.find(f.return_type) != vulkan_function_map.end())
    {
        add_function(vulkan_function_map[f.return_type]);
    }

    for (u64 i = 0; i < f.parameters.size(); ++i)
    {
        std::string parameter_type = f.parameters[i].type;
        if (parameter_type == "const")
        {
            parameter_type = f.parameters[i].name;
            parameter_type = parameter_type.substr(0, parameter_type.find("*"));
        }

        if (needed_vulkan_typedefs.find(parameter_type) == needed_vulkan_typedefs.end() && vulkan_typedef_map.find(parameter_type) != vulkan_typedef_map.end())
        {
            add_typedef(vulkan_typedef_map[parameter_type]);
        }
        else if (needed_vulkan_structs.find(parameter_type) == needed_vulkan_structs.end() && vulkan_struct_map.find(parameter_type) != vulkan_struct_map.end())
        {
            add_struct(vulkan_struct_map[parameter_type]);
        }
        else if (needed_vulkan_enums.find(parameter_type) == needed_vulkan_enums.end() && vulkan_enum_map.find(parameter_type) != vulkan_enum_map.end())
        {
            add_enum(vulkan_enum_map[parameter_type]);
        }
        else if (vulkan_define_macro_usage_map.find(parameter_type) != vulkan_define_macro_usage_map.end() && vulkan_define_macro_map.find(vulkan_define_macro_usage_map[parameter_type]) != vulkan_define_macro_map.end())
        {
            add_macro_usage(parameter_type);
        }
    }
    if (needed_vulkan_functions.find(f.name) == needed_vulkan_functions.end())
    {
        needed_vulkan_functions[f.name] = f;
        needed_vulkan_items_order.push_back({VULKAN_ITEM_TYPE_FUNCTION, f.name});
    }
}

void add_needed_items(Tokenizer* tokenizer)
{
    add_macro(vulkan_define_macro_map["VK_MAKE_VERSION"]);
    add_macro(vulkan_define_macro_map["VK_KHR_SURFACE_EXTENSION_NAME"]);
    add_macro(vulkan_define_macro_map["VK_EXT_DEBUG_UTILS_EXTENSION_NAME"]);
    add_macro(vulkan_define_macro_map["VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME"]);
    add_function(vulkan_function_map["PFN_vkCreateInstance"]);

    b32 parsing = true;
    while (parsing)
    {
        Token token = get_token(tokenizer);
        switch (token.type)
        {
        case TOKEN_TYPE_END_OF_STREAM:
            parsing = false;
            break;
        case TOKEN_TYPE_IDENTIFIER:
        {
            if (token_starts_with(token, "Vk", 2) || token_starts_with(token, "VK", 2) || token_starts_with(token, "vk", 2))
            {
                std::string token_name = std::string(token.text, token.length);
                if (vulkan_struct_map.find(token_name) != vulkan_struct_map.end())
                {
                    add_struct(vulkan_struct_map[token_name]);
                }
                else if (vulkan_enum_map.find(token_name) != vulkan_enum_map.end())
                {
                    add_enum(vulkan_enum_map[token_name]);
                }
                else if (vulkan_enum_value_to_enum_map.find(token_name) != vulkan_enum_value_to_enum_map.end())
                {
                    add_enum(vulkan_enum_value_to_enum_map[token_name]);
                }
                else if (vulkan_define_macro_usage_map.find(token_name) != vulkan_define_macro_usage_map.end() && vulkan_define_macro_map.find(vulkan_define_macro_usage_map[token_name]) != vulkan_define_macro_map.end())
                {
                    add_macro_usage(token_name);
                }
                else if (vulkan_define_macro_map.find(token_name) != vulkan_define_macro_map.end())
                {
                    add_macro(vulkan_define_macro_map[token_name]);
                }
                else if (vulkan_typedef_map.find(token_name) != vulkan_typedef_map.end())
                {
                    add_typedef(vulkan_typedef_map[token_name]);
                }
                else if (vulkan_function_map.find("PFN_" + token_name) != vulkan_function_map.end())
                {
                    add_function(vulkan_function_map["PFN_" + token_name]);
                }
            }
        } break;
        }
    }
}

std::string vulkan_macro_to_string(Macro m)
{
    std::string result = "#define ";
    result += m.name;

    if (m.parameters.size() > 0)
    {
        result += '(';
        for (u64 i = 0; i < m.parameters.size(); ++i)
        {
            result += m.parameters[i];
            if (i != m.parameters.size() - 1)
            {
                result += ',';
            }
        }
        result += ')';
    }
    result += ' ' + m.body;
    return result;
}

std::string vulkan_macro_usage_to_string(std::string macro, std::string parameter)
{
    return macro + "(" + parameter + ")";
}

std::string vulkan_enum_to_string(Enum e)
{
    std::string result = "enum " + e.type + "\n{\n";
    for (u64 i = 0; i < e.members.size(); ++i)
    {
        result += "\t" + e.members[i].name + " = " + e.members[i].value;
        if (i != e.members.size() - 1)
        {
            result += ",\n";
        }
    }
    result += "\n};";
    return result;
}

std::string vulkan_typedef_to_string(Typedef t)
{
    return "typedef " + t.value + " " + t.name + ";";
}

std::string vulkan_struct_to_string(Struct s)
{
    std::string result = (s.is_union ? "union " : "struct ") + s.type + "\n{\n";
    for (u64 i = 0; i < s.members.size(); ++i)
    {
        result += "\t" + s.members[i].type + " " + s.members[i].name + ";";
        if (i != s.members.size() - 1)
        {
            result += "\n";
        }
    }
    result += "\n};";
    return result;
}

std::string vulkan_function_to_string(Function f)
{
    std::string result = "typedef " + f.return_type + " (VKAPI_PTR *" + f.name + ")(";
    for (u64 i = 0; i < f.parameters.size(); ++i)
    {
        result += f.parameters[i].type + " " + f.parameters[i].name;
        if (i != f.parameters.size() - 1)
        {
            result += ", ";
        }
    }
    result += ");";
    return result;
}

std::string generate_our_vulkan_header(wchar_t* input_path)
{
    char* vulkan_usage_code = read_file(input_path);

    Tokenizer tokenizer = {vulkan_usage_code};
    add_needed_items(&tokenizer);

    std::string generated_header = header_text;
    std::string platform_functions = "\n\t#define VK_FUNCTIONS_PLATFORM \\\n";
    std::string debug_functions = "\n\t#define VK_FUNCTIONS_DEBUG \\\n";
    std::string instance_functions = "\n\t#define VK_FUNCTIONS_INSTANCE \\\n";
    std::string device_functions = "\n\t#define VK_FUNCTIONS_DEVICE \\\n";

    for (u64 i = 0; i < needed_vulkan_items_order.size(); ++i)
    {
        std::string name = needed_vulkan_items_order[i].name;
        switch (needed_vulkan_items_order[i].type)
        {
        case VULKAN_ITEM_TYPE_TYPEDEF:
            generated_header += '\n' + vulkan_typedef_to_string(needed_vulkan_typedefs[name]);
            break;
        case VULKAN_ITEM_TYPE_STRUCT:
            generated_header += '\n' + vulkan_struct_to_string(needed_vulkan_structs[name]);
            break;
        case VULKAN_ITEM_TYPE_ENUM:
            generated_header += '\n' + vulkan_enum_to_string(needed_vulkan_enums[name]);
            break;
        case VULKAN_ITEM_TYPE_MACRO:
            generated_header += '\n' + vulkan_macro_to_string(needed_vulkan_macros[name]);
            break;
        case VULKAN_ITEM_TYPE_MACRO_USAGE:
            generated_header += '\n' + vulkan_macro_usage_to_string(needed_vulkan_macro_usages[name], name);
            break;
        case VULKAN_ITEM_TYPE_FUNCTION:
        {
            Function f = needed_vulkan_functions[name];
            generated_header += '\n' + vulkan_function_to_string(f);

            if (f.parameters.size() > 0)
            {
                std::string* function_type = NULL;
                if (f.parameters[f.parameters.size() - 1].type == "VkInstance" || f.name == "PFN_vkGetInstanceProcAddr")
                {
                    function_type = &platform_functions;
                }
                else if (f.name == "PFN_vkCreateDebugUtilsMessengerEXT")
                {
                    function_type = &debug_functions;
                }
                else if (f.parameters[0].type == "VkPhysicalDevice" || f.parameters[0].type == "VkInstance" || f.name == "PFN_vkGetDeviceProcAddr")
                {
                    function_type = &instance_functions;
                }
                else if (f.parameters[0].type == "VkDevice" || f.parameters[0].type == "VkCommandBuffer" || f.parameters[0].type == "VkQueue")
                {
                    function_type = &device_functions;
                }

                if (function_type)
                {
                    *function_type += "\t\tVK_FUNCTION(" + f.name.replace(0, 4, "") + ") \\\n";
                }
            }
        } break;
        default:
            // Invalid code path
            exit(EXIT_FAILURE);
            break;
        }
    }

    // TODO: Turn this into a function that can just be called to initalize all of the Vulkan functions?
    std::string function_list = "\n#define VK_FUNCTION(function) external PFN_##function function;";
    function_list += platform_functions.substr(0, platform_functions.size() - 2) + "\n\tVK_FUNCTIONS_PLATFORM\n";
    function_list += "\n\t#if defined(DEBUG)" + debug_functions.substr(0, debug_functions.size() - 2) + "\n\t#else\n\t\t#define VK_FUNCTIONS_DEBUG\n\t#endif\n\tVK_FUNCTIONS_DEBUG\n";
    function_list += instance_functions.substr(0, instance_functions.size() - 2) + "\n\tVK_FUNCTIONS_INSTANCE\n";
    function_list += device_functions.substr(0, device_functions.size() - 2) + "\n\tVK_FUNCTIONS_DEVICE\n#undef VK_FUNCTION";
    generated_header += function_list;

    return generated_header;
}

int wmain(int argc, wchar_t* argv[])
{
    wchar_t* vulkan_header_path = argv[1];
    wchar_t* input_path = argv[2];
    wchar_t* output_path = argv[3];

    parse_vulkan_header(vulkan_header_path);

    std::string generated_header = generate_our_vulkan_header(input_path);
    write_file(output_path, generated_header.c_str());

    return 0;
}