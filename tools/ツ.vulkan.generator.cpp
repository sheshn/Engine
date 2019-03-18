#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <iostream>
#include <string>
#include <locale>
#include <codecvt>
#include <algorithm>

#include "ツ.tokenizer.h"

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

internal std::string embedded_shaders_header_text = R"(
/*****************
 Embedded Shaders
 *****************/
)";

char* read_file(const wchar_t* filepath, u64* size_out = NULL)
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

    if (size_out)
    {
        *size_out = size;
    }

    return result;
}

void write_file(const wchar_t* filepath, const char* contents)
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

std::vector<std::wstring> find_files_in_directory(std::wstring directory, std::wstring extension)
{
    WIN32_FIND_DATA file_info;
    HANDLE find_handle = FindFirstFile((directory + L"\\" + extension).c_str(), &file_info);

    std::vector<std::wstring> result;
    if (find_handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            result.push_back(directory + L"\\" + file_info.cFileName);
        }
        while (FindNextFile(find_handle, &file_info));
        FindClose(find_handle);
    }
    return result;
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

void parse_vulkan_header(std::wstring filepath)
{
    char* vulkan_header = read_file(filepath.c_str());

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

std::string generate_our_vulkan_header(std::wstring input_path)
{
    char* vulkan_usage_code = read_file(input_path.c_str());

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

struct Shader_Layout
{
    std::string name;
    u64 layout_index;
    u64 open_brace_index;
    u64 close_brace_index;
};

void parse_shader_layout(Tokenizer* tokenizer, char* shader_code, u64 layout_index, std::unordered_map<std::string, Shader_Layout>* shader_layouts)
{
    Shader_Layout layout;
    layout.layout_index = layout_index;

    Token name = get_token(tokenizer);
    if (name.type == TOKEN_TYPE_IDENTIFIER)
    {
        layout.name = std::string(name.text, name.length);

        Token open_brace_token = get_token(tokenizer);
        if (open_brace_token.type == TOKEN_TYPE_OPEN_BRACE)
        {
            layout.open_brace_index = open_brace_token.text - shader_code;

            u32 brace_count = 1;
            while (true)
            {
                Token member = get_token(tokenizer);
                if (member.type == TOKEN_TYPE_CLOSE_BRACE)
                {
                    brace_count--;
                }
                else if (member.type == TOKEN_TYPE_OPEN_BRACE)
                {
                    brace_count++;
                }

                if (brace_count == 0)
                {
                    layout.close_brace_index = member.text - shader_code;
                    break;
                }
            }
        }
    }

    if (eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        shader_layouts->insert({layout.name, layout});
    }
    else
    {
        fprintf(stderr, "ERROR: Missing semicolon for layout: %s\n", layout.name.c_str());
    }
}

struct Shader_Function
{
    std::string shader_type;
    std::string name;
    u64         function_start_index;
    u64         void_index;
    u64         open_brace_index;
    u64         close_brace_index;

    std::vector<std::string> layouts;
};

std::vector<Shader_Function> shader_functions;

void parse_shader(Tokenizer* tokenizer, char* shader_code, std::string shader_type, u64 function_start_index, std::vector<Shader_Function>* shader_functions)
{
    Shader_Function function;
    function.shader_type = shader_type;
    function.function_start_index = function_start_index;

    while (true)
    {
        Token token = get_token(tokenizer);
        if (token.type == TOKEN_TYPE_CLOSE_PARENTHESIS)
        {
            break;
        }
        else if (token.type == TOKEN_TYPE_COMMA)
        {
            continue;
        }

        std::string layout_name = std::string(token.text, token.length);
        function.layouts.push_back(layout_name);
    }

    Token void_token = get_token(tokenizer);
    if (token_equals(void_token, "void"))
    {
        function.void_index = void_token.text - shader_code;

        Token name = get_token(tokenizer);
        if (name.type == TOKEN_TYPE_IDENTIFIER && eat_token(tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS) && eat_token(tokenizer, TOKEN_TYPE_CLOSE_PARENTHESIS))
        {
            function.name = std::string(name.text, name.length);

            Token open_brace_token = get_token(tokenizer);
            if (open_brace_token.type == TOKEN_TYPE_OPEN_BRACE)
            {
                function.open_brace_index = open_brace_token.text - shader_code;

                u32 brace_count = 1;
                while (true)
                {
                    Token t = get_token(tokenizer);
                    if (t.type == TOKEN_TYPE_CLOSE_BRACE)
                    {
                        brace_count--;
                    }
                    else if (t.type == TOKEN_TYPE_OPEN_BRACE)
                    {
                        brace_count++;
                    }

                    if (brace_count == 0)
                    {
                        function.close_brace_index = t.text - shader_code;
                        shader_functions->push_back(function);
                        break;
                    }
                }
            }
        }
    }
}

std::wstring get_shader_file_extension(std::string shader_type)
{
    std::wstring ext = L"";
    if (shader_type == "vertex")
    {
        ext = L"vert";
    }
    else if (shader_type == "fragment")
    {
        ext = L"frag";
    }
    return ext;
}

char hex_charset[] = "0123456789ABCDEF";

std::string bytes_to_string(char* bytes, u64 size)
{
    std::string result = "{";
    for (u64 i = 0; i < size; ++i)
    {
        char c = bytes[i];
        result += "0x";
        result += hex_charset[(c & 0xF0) >> 4];
        result += hex_charset[c & 0xF];
        result += ',';
    }
    result[result.size() - 1] = '}';
    return result;
}

std::string compile_shaders(std::wstring compiler_path, std::wstring optimizer_path, char* shader_code, std::vector<Shader_Function> shader_functions, std::unordered_map<std::string, Shader_Layout> shader_layouts)
{
    std::string result = "";
    for (u64 i = 0; i < shader_functions.size(); ++i)
    {
        std::string code = std::string(shader_code);
        std::unordered_map<std::string, Shader_Layout> layouts = shader_layouts;

        // TODO: Should probably check for duplicate function names!
        Shader_Function function = shader_functions[i];
        for (u64 j = 0; j < function.layouts.size(); ++j)
        {
            if (shader_layouts.find(function.layouts[j]) == shader_layouts.end() || shader_layouts[function.layouts[j]].layout_index > function.function_start_index)
            {
                fprintf(stderr, "ERROR: %s needs undefined layout: %s\n", function.name.c_str(), function.layouts[j].c_str());
            }
            else
            {
                Shader_Layout layout = shader_layouts[function.layouts[j]];
                code[layout.layout_index] = '/';
                code[layout.layout_index + 1] = '/';
                code[layout.open_brace_index] = ' ';
                code[layout.close_brace_index] = ' ';
                code[layout.close_brace_index + 1] = ' ';

                layouts.erase(function.layouts[j]);
            }
        }

        for (auto it : layouts)
        {
            Shader_Layout layout = it.second;
            for (u64 index = layout.layout_index; index <= layout.close_brace_index + 1; ++index)
            {
                if (!is_whitespace(code[index]))
                {
                    code[index] = ' ';
                }
            }
        }

        for (u64 j = 0; j < shader_functions.size(); ++j)
        {
            if (i != j)
            {
                Shader_Function f = shader_functions[j];
                for (u64 index = f.function_start_index; index <= f.close_brace_index; ++index)
                {
                    if (!is_whitespace(code[index]))
                    {
                        code[index] = ' ';
                    }
                }
            }
        }

        code.replace(function.void_index + 5, function.name.size(), "main");
        code.replace(function.function_start_index, function.void_index - function.function_start_index, "");

        std::wstring filename = std::wstring(function.name.begin(), function.name.end()) + L".tsd";
        write_file(filename.c_str(), code.c_str());

        std::wstring shader_type = get_shader_file_extension(function.shader_type);
        if (!shader_type.empty())
        {
            std::wstring spirv_filename = std::wstring(function.name.begin(), function.name.end()) + L".spv";
            _wsystem((compiler_path + L" -t -V --target-env vulkan1.1 -S " + shader_type + L" -o " + spirv_filename + L" " + filename).c_str());
            _wsystem((optimizer_path + L" -O " + spirv_filename + L" -o " + spirv_filename).c_str());

            u64 code_size;
            char* spirv_code = read_file(spirv_filename.c_str(), &code_size);
            result += "internal u8 " + function.name + "_code[] = " + bytes_to_string(spirv_code, code_size) + ";\n";
            result += "internal VkShaderModuleCreateInfo " + function.name + "_create_info = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO, NULL, 0, " + std::to_string(code_size) + ", (u32*)" + function.name + "_code" + "};\n";
        }
        else
        {
            fprintf(stderr, "Unsupported shader_type: %s\n", function.shader_type.c_str());
        }
    }

    _wsystem(L"del *.spv *.tsd");
    return result;
}

std::string compile_shader_file(std::wstring compiler_path, std::wstring optimizer_path, std::wstring filepath)
{
    char* shader_code = read_file(filepath.c_str());
    Tokenizer tokenizer = {shader_code};

    std::vector<Shader_Function> shader_functions;
    std::unordered_map<std::string, Shader_Layout> shader_layouts;

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
        case TOKEN_TYPE_IDENTIFIER:
        {
            if (token_equals(token, "layout"))
            {
                parse_shader_layout(&tokenizer, shader_code, token.text - shader_code, &shader_layouts);
            }
            else if ((token_equals(token, "vertex") || token_equals(token, "fragment")) && eat_token(&tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS))
            {
                parse_shader(&tokenizer, shader_code, std::string(token.text, token.length), token.text - shader_code, &shader_functions);
            }
        } break;
        }
    }

    return compile_shaders(compiler_path, optimizer_path, shader_code, shader_functions, shader_layouts);
}

std::string generate_shaders(std::wstring shader_directory, std::wstring shader_tools_path)
{
    std::wstring compiler_path = shader_tools_path + L"/glslangValidator.exe";
    std::wstring optimizer_path = shader_tools_path + L"/spirv-opt.exe";

    std::string generated_shaders = "";

    std::vector<std::wstring> shader_files = find_files_in_directory(shader_directory, L"*.tsd*");
    for (u64 i = 0; i < shader_files.size(); ++i)
    {
        std::wstring shader_file_path = shader_files[i];

        wprintf(L"Compiling %s\n", shader_file_path.c_str());
        generated_shaders += compile_shader_file(compiler_path, optimizer_path, shader_file_path);
    }

    return generated_shaders;
}

int wmain(int argc, wchar_t* argv[])
{
    std::wstring vulkan_sdk_path = std::wstring(argv[1]);
    std::wstring input_path = std::wstring(argv[2]);
    std::wstring shader_directory = std::wstring(argv[3]);
    std::wstring output_path = std::wstring(argv[4]);

    parse_vulkan_header(vulkan_sdk_path + L"/Include/vulkan/vulkan_core.h");

    std::string generated_header = generate_our_vulkan_header(input_path);
    generated_header += "\n" + embedded_shaders_header_text + "\n" + generate_shaders(shader_directory, vulkan_sdk_path + L"/Bin");

    write_file(output_path.c_str(), generated_header.c_str());
    return 0;
}