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
    add_macro(vulkan_define_macro_map["VK_FALSE"]);
    add_macro(vulkan_define_macro_map["VK_TRUE"]);
    add_macro(vulkan_define_macro_map["VK_MAKE_VERSION"]);
    add_macro(vulkan_define_macro_map["VK_KHR_SURFACE_EXTENSION_NAME"]);
    add_macro(vulkan_define_macro_map["VK_EXT_DEBUG_UTILS_EXTENSION_NAME"]);
    add_macro(vulkan_define_macro_map["VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME"]);
    add_function(vulkan_function_map["PFN_vkCreateInstance"]);
    add_function(vulkan_function_map["PFN_vkCreateShaderModule"]);
    add_function(vulkan_function_map["PFN_vkDestroyShaderModule"]);
    add_function(vulkan_function_map["PFN_vkCreateGraphicsPipelines"]);
    add_function(vulkan_function_map["PFN_vkCreateComputePipelines"]);
    add_enum(vulkan_enum_map["VkCullModeFlagBits"]);
    add_enum(vulkan_enum_map["VkColorComponentFlagBits"]);

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

enum Shader_Struct_Member_Annotation
{
    SHADER_STRUCT_MEMBER_ANNOTATION_POSITION,
    SHADER_STRUCT_MEMBER_ANNOTATION_PER_INSTANCE,
    SHADER_STRUCT_MEMBER_ANNOTATION_PER_VERTEX,
    SHADER_STRUCT_MEMBER_ANNOTATION_FLAT
};

struct Shader_Struct_Member
{
    std::string type;
    std::string name;

    u64 name_end_index;
    u64 semicolon_index;

    std::vector<Shader_Struct_Member_Annotation> annotations;
};

struct Shader_Struct
{
    std::string name;
    u64         struct_index;
    u64         open_brace_index;
    u64         close_brace_index;

    std::vector<Shader_Struct_Member> members;
};

void parse_shader_struct_member_annotations(Tokenizer* tokenizer, std::vector<Shader_Struct_Member_Annotation>* annotations)
{
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACKET))
    {
        while (true)
        {
            Token annotation = get_token(tokenizer);
            if (annotation.type == TOKEN_TYPE_CLOSE_BRACKET)
            {
                break;
            }
            else if (annotation.type == TOKEN_TYPE_COMMA)
            {
                continue;
            }

            if (token_equals(annotation, "position"))
            {
                annotations->push_back(SHADER_STRUCT_MEMBER_ANNOTATION_POSITION);
            }
            else if (token_equals(annotation, "per_instance"))
            {
                annotations->push_back(SHADER_STRUCT_MEMBER_ANNOTATION_PER_INSTANCE);
            }
            else if (token_equals(annotation, "flat"))
            {
                annotations->push_back(SHADER_STRUCT_MEMBER_ANNOTATION_FLAT);
            }
            else
            {
                fprintf(stderr, "ERROR(%s): unsupported annotation %.*s\n", __FUNCTION__, (u32)annotation.length, annotation.text);
            }
        }
    }
}

void parse_shader_struct_member(Tokenizer* tokenizer, char* shader_code, Shader_Struct_Member* member)
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
            member->semicolon_index = token.text + token.length - shader_code - 1;
            parsing = false;
        } break;
        case TOKEN_TYPE_COLON:
        {
            parse_shader_struct_member_annotations(tokenizer, &member->annotations);
        } break;
        default:
        {
            member->name += std::string(token.text, token.length);
            member->name_end_index = token.text + token.length - shader_code - 1;
        } break;
        }
    }
}

void parse_shader_struct(Tokenizer* tokenizer, char* shader_code, u64 struct_index, std::unordered_map<std::string, Shader_Struct>* shader_structs)
{
    Token name = get_token(tokenizer);
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        Shader_Struct s = {};
        s.name = std::string(name.text, name.length);
        s.struct_index = struct_index;

        while (true)
        {
            Token member = get_token(tokenizer);
            if (member.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                s.close_brace_index = member.text + member.length - shader_code - 1;
                break;
            }

            Shader_Struct_Member struct_member = {};
            struct_member.type = std::string(member.text, member.length);

            parse_shader_struct_member(tokenizer, shader_code, &struct_member);
            s.members.push_back(struct_member);
        }
        shader_structs->insert({s.name, s});
    }
}

struct Shader_Function
{
    std::string shader_type;
    std::string name;
    u64         function_start_index;
    u64         return_type_index;
    u64         return_semicolon_index;
    u64         open_brace_index;
    u64         close_brace_index;

    std::vector<std::string> layouts;

    b32 has_output;
    b32 has_input;

    Shader_Struct return_type;
    Shader_Struct input_parameter;

    std::string return_type_name;
    std::string input_parameter_name;

    u32 return_type_count;
    u32 input_parameter_count;
};

void parse_shader(Tokenizer* tokenizer, char* shader_code, std::string shader_type, u64 function_start_index, std::unordered_map<std::string, Shader_Function>* shader_functions, std::unordered_map<std::string, Shader_Struct> known_shader_structs)
{
    Shader_Function function;
    function.shader_type = shader_type;
    function.function_start_index = function_start_index;
    function.has_input = false;

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

    Token return_type_token = get_token(tokenizer);
    if (return_type_token.type == TOKEN_TYPE_IDENTIFIER)
    {
        if (token_equals(return_type_token, "void"))
        {
            function.return_type_index = return_type_token.text - shader_code;
            function.has_output = false;
        }
        else
        {
            std::string struct_name = std::string(return_type_token.text, return_type_token.length);
            if (known_shader_structs.find(struct_name) != known_shader_structs.end())
            {
                function.return_type = known_shader_structs[struct_name];
                function.has_output = true;
            }
            else
            {
                fprintf(stderr, "ERROR(%s): undefined struct %s used as return value for shader %s\n", __FUNCTION__, struct_name.c_str(), function.name.c_str());
                return;
            }
        }

        if (function.shader_type == "geometry")
        {
            if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACKET))
            {
                Token out_count = get_token(tokenizer);
                if (out_count.type == TOKEN_TYPE_NUMBER && eat_token(tokenizer, TOKEN_TYPE_CLOSE_BRACKET))
                {
                    function.return_type_count = (u32)out_count.u64;
                }
                else
                {
                    fprintf(stderr, "ERROR(%s): shader %s of type geometry needs to return an array\n", __FUNCTION__, function.name.c_str());
                    return;
                }
            }
            else
            {
                fprintf(stderr, "ERROR(%s): shader %s of type geometry needs to return an array\n", __FUNCTION__, function.name.c_str());
                return;
            }
        }

        Token name = get_token(tokenizer);
        if (name.type == TOKEN_TYPE_IDENTIFIER && eat_token(tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS))
        {
            function.name = std::string(name.text, name.length);

            Token input_parameter = get_token(tokenizer);
            if (input_parameter.type == TOKEN_TYPE_IDENTIFIER)
            {
                std::string struct_name = std::string(input_parameter.text, input_parameter.length);
                if (known_shader_structs.find(struct_name) != known_shader_structs.end())
                {
                    function.input_parameter = known_shader_structs[struct_name];
                }
                else
                {
                    fprintf(stderr, "ERROR(%s): undefined struct %s used as input parameter to shader %s\n", __FUNCTION__, struct_name.c_str(), function.name.c_str());
                    return;
                }

                Token input_name = get_token(tokenizer);
                while (true)
                {
                    Token t = get_token(tokenizer);
                    if (function.shader_type == "geometry")
                    {
                        if (t.type == TOKEN_TYPE_OPEN_BRACKET)
                        {
                            Token in_count = get_token(tokenizer);
                            if (in_count.type == TOKEN_TYPE_NUMBER && eat_token(tokenizer, TOKEN_TYPE_CLOSE_BRACKET))
                            {
                                function.input_parameter_count = (u32)in_count.u64;
                            }
                            else
                            {
                                fprintf(stderr, "ERROR(%s): shader %s of type geometry needs to take in an array parameter\n", __FUNCTION__, function.name.c_str());
                                return;
                            }
                        }
                        else
                        {
                            fprintf(stderr, "ERROR(%s): shader %s of type geometry needs to take in an array parameter\n", __FUNCTION__, function.name.c_str());
                            return;
                        }

                        function.has_input = true;
                        function.input_parameter_name = std::string(input_name.text, t.text - input_name.text);
                        if (!eat_token(tokenizer, TOKEN_TYPE_CLOSE_PARENTHESIS))
                        {
                            return;
                        }
                        break;
                    }
                    else if (t.type == TOKEN_TYPE_CLOSE_PARENTHESIS)
                    {
                        function.has_input = true;
                        function.input_parameter_name = std::string(input_name.text, t.text - input_name.text);
                        break;
                    }
                }
            }
            else if (input_parameter.type != TOKEN_TYPE_CLOSE_PARENTHESIS)
            {
                return;
            }

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
                    else if (t.type == TOKEN_TYPE_IDENTIFIER && token_equals(t, "return"))
                    {
                        function.return_type_index = t.text - shader_code;
                        function.return_type_name = "";
                        while (true)
                        {
                            Token n = get_token(tokenizer);
                            if (n.type == TOKEN_TYPE_SEMICOLON || n.type == TOKEN_TYPE_END_OF_STREAM)
                            {
                                function.return_semicolon_index = n.text - shader_code;
                                break;
                            }
                            function.return_type_name += std::string(n.text, n.length);
                        }
                    }

                    if (brace_count == 0)
                    {
                        function.close_brace_index = t.text - shader_code;
                        shader_functions->insert({function.name, function});
                        break;
                    }
                }
            }
        }
    }
}

// TODO: All other pipeline states/fields
struct Shader_Pipeline_Input_Assembly_State
{
    std::string topology;
};

struct Shader_Pipeline_Rasterization_State
{
    std::string rasterizer_discard_enable;
    std::string cull_mode;
    std::string front_face;
};

struct Shader_Pipeline_Blend_Attachment_State
{
    std::string blend_enable;
    std::string color_write_mask;
};

struct Shader_Pipeline_Color_Blend_State
{
    std::vector<Shader_Pipeline_Blend_Attachment_State> attachments;
};

struct Shader_Pipeline_Depth_Stencil_State
{
    std::string depth_test_enable;
    std::string depth_write_enable;
    std::string depth_compare_op;
};

struct Shader_Pipeline
{
    std::string name;
    u64         pipeline_index;
    u64         open_brace_index;
    u64         close_brace_index;

    std::vector<std::string> shader_functions;

    b32 has_color_blend_state;
    b32 has_depth_stencil_state;

    Shader_Pipeline_Input_Assembly_State input_assembly_state;
    Shader_Pipeline_Rasterization_State  rasterization_state;
    Shader_Pipeline_Color_Blend_State    color_blend_state;
    Shader_Pipeline_Depth_Stencil_State  depth_stencil_state;
};

std::string pipeline_input_assembly_state_create_info(Shader_Pipeline_Input_Assembly_State state)
{
    std::string result = "\tVkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};\n";
    result += "\tinput_assembly_state_create_info.topology = " + state.topology + ";\n";
    return result;
}

std::string pipeline_rasterization_state_create_info(Shader_Pipeline_Rasterization_State state)
{
    std::string result = "\tVkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};\n";
    result += "\trasterization_state_create_info.rasterizerDiscardEnable = " + state.rasterizer_discard_enable + ";\n";
    result += "\trasterization_state_create_info.cullMode = " + state.cull_mode + ";\n";
    result += "\trasterization_state_create_info.frontFace = " + state.front_face + ";\n";
    return result;
}

std::string pipeline_color_blend_state_create_info(Shader_Pipeline_Color_Blend_State state)
{
    std::string result = "\tVkPipelineColorBlendAttachmentState color_blend_attachments[" + std::to_string(state.attachments.size()) + "] = {};\n";
    for (u64 i = 0; i < state.attachments.size(); ++i)
    {
        result += "\tcolor_blend_attachments[" + std::to_string(i) + "].blendEnable = " + state.attachments[i].blend_enable + ";\n";
        result += "\tcolor_blend_attachments[" + std::to_string(i) + "].colorWriteMask = " + state.attachments[i].color_write_mask + ";\n";
    }

    result += "\tVkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};\n";
    result += "\tcolor_blend_state_create_info.attachmentCount = sizeof(color_blend_attachments)/sizeof(color_blend_attachments[0]);\n";
    result += "\tcolor_blend_state_create_info.pAttachments = color_blend_attachments;\n";
    return result;
}

std::string pipeline_depth_stencil_state_create_info(Shader_Pipeline_Depth_Stencil_State state)
{
    std::string result = "\tVkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};\n";
    result += "\tdepth_stencil_state_create_info.depthTestEnable = " + state.depth_test_enable + ";\n";
    result += "\tdepth_stencil_state_create_info.depthWriteEnable = " + state.depth_write_enable + ";\n";
    result += "\tdepth_stencil_state_create_info.depthCompareOp = " + state.depth_compare_op + ";\n";
    return result;
}

Shader_Pipeline_Input_Assembly_State create_default_input_assembly_state()
{
    return {"VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST"};
}

Shader_Pipeline_Rasterization_State create_default_rasterization_state()
{
    return {"VK_FALSE", "VK_CULL_MODE_NONE", "VK_FRONT_FACE_COUNTER_CLOCKWISE"};
}

Shader_Pipeline_Blend_Attachment_State create_default_blend_attachment_state()
{
    return {"VK_FALSE", "VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT"};
}

Shader_Pipeline_Color_Blend_State create_default_color_blend_state()
{
    return {};
}

Shader_Pipeline_Depth_Stencil_State create_default_depth_stencil_state()
{
    return {"VK_FALSE", "VK_FALSE", "VK_COMPARE_OP_NEVER"};
}

Shader_Pipeline create_default_pipeline()
{
    Shader_Pipeline pipeline = {};

    pipeline.has_color_blend_state = false;
    pipeline.has_depth_stencil_state = false;

    pipeline.input_assembly_state = create_default_input_assembly_state();
    pipeline.rasterization_state = create_default_rasterization_state();
    return pipeline;
}

std::vector<std::string> parse_shader_pipeline_shaders(Tokenizer* tokenizer, std::string pipeline_name)
{
    std::vector<std::string> shaders;

    if (eat_token(tokenizer, TOKEN_TYPE_EQUAL) && eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACKET))
    {
        while (true)
        {
            Token t = get_token(tokenizer);
            if (t.type == TOKEN_TYPE_CLOSE_BRACKET)
            {
                break;
            }
            if (t.type == TOKEN_TYPE_IDENTIFIER)
            {
                shaders.push_back(std::string(t.text, t.length));
            }
        }
    }
    if (!eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        fprintf(stderr, "ERROR(%s): pipeline %s shaders field missing semicolon\n", __FUNCTION__, pipeline_name.c_str());
    }
    return shaders;
}

Shader_Pipeline_Input_Assembly_State parse_shader_pipeline_input_assembly_state(Tokenizer* tokenizer, std::string pipeline_name)
{
    Shader_Pipeline_Input_Assembly_State state = create_default_input_assembly_state();
    if (eat_token(tokenizer, TOKEN_TYPE_EQUAL) && eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        while (true)
        {
            Token t = get_token(tokenizer);
            if (t.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            if (token_equals(t, "topology") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.topology = std::string(value.text, value.length);
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR(%s): pipeline %s missing equal/brace for field input_assembly_state\n", __FUNCTION__, pipeline_name.c_str());
    }
    if (!eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        fprintf(stderr, "ERROR(%s): pipeline %s input_assembly_state field missing semicolon\n", __FUNCTION__, pipeline_name.c_str());
    }
    return state;
}

Shader_Pipeline_Rasterization_State parse_shader_pipeline_rasterization_state(Tokenizer* tokenizer, std::string pipeline_name)
{
    Shader_Pipeline_Rasterization_State state = create_default_rasterization_state();
    if (eat_token(tokenizer, TOKEN_TYPE_EQUAL) && eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        while (true)
        {
            Token t = get_token(tokenizer);
            if (t.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            if (token_equals(t, "cull_mode") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.cull_mode = std::string(value.text, value.length);
            }
            else if (token_equals(t, "front_face") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.front_face = std::string(value.text, value.length);
            }
            else if (token_equals(t, "rasterizer_discard_enable") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.rasterizer_discard_enable = std::string(value.text, value.length);
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR(%s): pipeline %s missing equal/brace for field rasterization_state\n", __FUNCTION__, pipeline_name.c_str());
    }
    if (!eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        fprintf(stderr, "ERROR(%s): pipeline %s rasterization_state field missing semicolon\n", __FUNCTION__, pipeline_name.c_str());
    }
    return state;
}

Shader_Pipeline_Blend_Attachment_State parse_shader_pipeline_blend_attachment_state(Tokenizer* tokenizer, std::string pipeline_name)
{
    Shader_Pipeline_Blend_Attachment_State state = create_default_blend_attachment_state();
    if (eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        b32 parsing = true;
        while (parsing)
        {
            Token t = get_token(tokenizer);
            if (t.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            if (token_equals(t, "blend_enable") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.blend_enable = std::string(value.text, value.length);
            }
            else if (token_equals(t, "color_write_mask") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token begin = get_token(tokenizer);
                while (true)
                {
                    Token value = get_token(tokenizer);
                    if (value.type == TOKEN_TYPE_COMMA)
                    {
                        state.color_write_mask = std::string(begin.text, value.text - begin.text);
                        break;
                    }
                    else if (value.type == TOKEN_TYPE_CLOSE_BRACE)
                    {
                        parsing = false;
                        break;
                    }
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR(%s): pipeline %s missing brace for field blend_attachment_state\n", __FUNCTION__, pipeline_name.c_str());
    }
    return state;
}

Shader_Pipeline_Color_Blend_State parse_shader_pipeline_color_blend_state(Tokenizer* tokenizer, std::string pipeline_name)
{
    Shader_Pipeline_Color_Blend_State state = create_default_color_blend_state();
    if (eat_token(tokenizer, TOKEN_TYPE_EQUAL) && eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACKET))
    {
        while (true)
        {
            state.attachments.push_back(parse_shader_pipeline_blend_attachment_state(tokenizer, pipeline_name));

            Token t = get_token(tokenizer);
            if (t.type == TOKEN_TYPE_CLOSE_BRACKET)
            {
                break;
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR(%s): pipeline %s missing equal/bracket for field color_blend_state\n", __FUNCTION__, pipeline_name.c_str());
    }
    if (!eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        fprintf(stderr, "ERROR(%s): pipeline %s color_blend_state field missing semicolon\n", __FUNCTION__, pipeline_name.c_str());
    }
    return state;
}

Shader_Pipeline_Depth_Stencil_State parse_shader_pipeline_depth_stencil_state(Tokenizer* tokenizer, std::string pipeline_name)
{
    Shader_Pipeline_Depth_Stencil_State state = create_default_depth_stencil_state();
    if (eat_token(tokenizer, TOKEN_TYPE_EQUAL) && eat_token(tokenizer, TOKEN_TYPE_OPEN_BRACE))
    {
        while (true)
        {
            Token t = get_token(tokenizer);
            if (t.type == TOKEN_TYPE_CLOSE_BRACE)
            {
                break;
            }

            if (token_equals(t, "depth_test_enable") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.depth_test_enable = std::string(value.text, value.length);
            }
            else if (token_equals(t, "depth_write_enable") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.depth_write_enable = std::string(value.text, value.length);
            }
            else if (token_equals(t, "depth_compare_op") && eat_token(tokenizer, TOKEN_TYPE_EQUAL))
            {
                Token value = get_token(tokenizer);
                state.depth_compare_op = std::string(value.text, value.length);
            }
        }
    }
    else
    {
        fprintf(stderr, "ERROR(%s): pipeline %s missing equal/brace for field depth_stencil_state\n", __FUNCTION__, pipeline_name.c_str());
    }
    if (!eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        fprintf(stderr, "ERROR(%s): pipeline %s depth_stencil_state field missing semicolon\n", __FUNCTION__, pipeline_name.c_str());
    }
    return state;
}

void parse_shader_pipeline(Tokenizer* tokenizer, char* shader_code, u64 pipeline_index, std::vector<Shader_Pipeline>* shader_pipelines)
{
    Shader_Pipeline pipeline = create_default_pipeline();
    pipeline.pipeline_index = pipeline_index;

    Token name = get_token(tokenizer);
    if (name.type == TOKEN_TYPE_IDENTIFIER)
    {
        pipeline.name = std::string(name.text, name.length);

        Token open_brace_token = get_token(tokenizer);
        if (open_brace_token.type == TOKEN_TYPE_OPEN_BRACE)
        {
            pipeline.open_brace_index = open_brace_token.text - shader_code;

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

                // NOTE: Ignoring multisample
                if (member.type == TOKEN_TYPE_IDENTIFIER)
                {
                    if (token_equals(member, "shaders"))
                    {
                        pipeline.shader_functions = parse_shader_pipeline_shaders(tokenizer, pipeline.name);
                    }
                    else if (token_equals(member, "input_assembly_state"))
                    {
                        pipeline.input_assembly_state = parse_shader_pipeline_input_assembly_state(tokenizer, pipeline.name);
                    }
                    else if (token_equals(member, "rasterization_state"))
                    {
                        pipeline.rasterization_state = parse_shader_pipeline_rasterization_state(tokenizer, pipeline.name);
                    }
                    else if (token_equals(member, "color_blend_state"))
                    {
                        pipeline.has_color_blend_state = true;
                        pipeline.color_blend_state = parse_shader_pipeline_color_blend_state(tokenizer, pipeline.name);
                    }
                    else if (token_equals(member, "depth_stencil_state"))
                    {
                        pipeline.has_depth_stencil_state = true;
                        pipeline.depth_stencil_state = parse_shader_pipeline_depth_stencil_state(tokenizer, pipeline.name);
                    }
                    else
                    {
                        fprintf(stderr, "ERROR(%s): pipeline %s specifies unsupported pipeline state (%.*s)\n", __FUNCTION__, pipeline.name.c_str(), (u32)member.length, member.text);
                    }
                }

                if (brace_count == 0)
                {
                    pipeline.close_brace_index = member.text - shader_code;
                    break;
                }
            }
        }
    }

    if (eat_token(tokenizer, TOKEN_TYPE_SEMICOLON))
    {
        shader_pipelines->push_back(pipeline);
    }
    else
    {
        fprintf(stderr, "ERROR: Missing semicolon for pipeline: %s\n", pipeline.name.c_str());
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
    else if (shader_type == "geometry")
    {
        ext = L"geom";
    }
    else if (shader_type == "compute")
    {
        ext = L"comp";
    }
    return ext;
}

std::string get_shader_stage_bit(std::string shader_type)
{
    std::string bit = "";
    if (shader_type == "vertex")
    {
        bit = "VK_SHADER_STAGE_VERTEX_BIT";
    }
    else if (shader_type == "fragment")
    {
        bit = "VK_SHADER_STAGE_FRAGMENT_BIT";
    }
    else if (shader_type == "geometry")
    {
        bit = "VK_SHADER_STAGE_GEOMETRY_BIT";
    }
    else if (shader_type == "compute")
    {
        bit = "VK_SHADER_STAGE_COMPUTE_BIT";
    }
    return bit;
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

std::string get_geometry_shader_layout_in(Shader_Function function)
{
    // NOTE: Ignoring the other ones for now
    if (function.input_parameter_count == 1)
    {
        return "points";
    }
    else if (function.input_parameter_count == 2)
    {
        return "lines";
    }
    else if (function.input_parameter_count == 3)
    {
        return "triangles";
    }
    else
    {
        fprintf(stderr, "ERROR(%s): unknown input count to geometry shader %s (count: %u)", __FUNCTION__, function.name.c_str(), (u32)function.input_parameter_count);
    }

    return "";
}

std::string get_geometry_shader_layout_out(Shader_Function function)
{
    // NOTE: This is not necessarily correct!
    if (function.return_type_count == 1)
    {
        return "points";
    }
    else if (function.return_type_count == 2)
    {
        return "line_strip";
    }
    else if (function.return_type_count >= 3)
    {
        return "triangle_strip";
    }
    else
    {
        fprintf(stderr, "ERROR(%s): unknown output count from geometry shader %s (count: %u)", __FUNCTION__, function.name.c_str(), (u32)function.return_type_count);
    }

    return "";
}

std::string generate_new_shader_function(std::string code, Shader_Function function)
{
    std::string new_shader_code = "";

    if (function.shader_type == "geometry")
    {
        new_shader_code += "layout(" + get_geometry_shader_layout_in(function) + ") in;\n";
        new_shader_code += "layout(" + get_geometry_shader_layout_out(function) + ", max_vertices = " + std::to_string(function.return_type_count) + ") out;\n";
    }

    u64 offset = 0;
    if (function.has_input)
    {
        std::string brackets = "";
        if (function.shader_type == "geometry")
        {
            brackets = "[]";
        }

        for (u64 i = 0; i < function.input_parameter.members.size(); ++i)
        {
            Shader_Struct_Member m = function.input_parameter.members[i];
            if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_POSITION) != m.annotations.end())
            {
                offset++;
            }
            else
            {
                new_shader_code += "layout(location = " + std::to_string(i - offset) + ")" + (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_FLAT) != m.annotations.end() ? " flat in " : " in ") + m.type + " in_" + m.name + brackets + ";\n";
            }
        }
    }

    offset = 0;
    if (function.has_output)
    {
        for (u64 i = 0; i < function.return_type.members.size(); ++i)
        {
            Shader_Struct_Member m = function.return_type.members[i];
            if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_POSITION) != m.annotations.end())
            {
                offset++;
            }
            else
            {
                new_shader_code += "layout(location = " + std::to_string(i - offset) + ")" + (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_FLAT) != m.annotations.end() ? " flat out " : " out ") + m.type + " out_" + m.name + ";\n";
            }
        }
    }

    new_shader_code += "void main()\n{\n";

    if (function.has_input)
    {
        if (function.shader_type == "geometry")
        {
            new_shader_code += function.input_parameter.name + " " + function.input_parameter_name + "[" + std::to_string(function.input_parameter_count) + "];\n";
            new_shader_code += "for (u32 i = 0; i < " + std::to_string(function.input_parameter_count) + "; ++i)\n{\n";

            for (u64 i = 0; i < function.input_parameter.members.size(); ++i)
            {
                Shader_Struct_Member m = function.input_parameter.members[i];
                if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_POSITION) == m.annotations.end())
                {
                    new_shader_code += function.input_parameter_name + "[i]." + m.name + " = " + "in_" + m.name + "[i];\n";
                }
            }
            new_shader_code += "}\n";
        }
        else
        {
            new_shader_code += function.input_parameter.name + " " + function.input_parameter_name + ";\n";
            for (u64 i = 0; i < function.input_parameter.members.size(); ++i)
            {
                Shader_Struct_Member m = function.input_parameter.members[i];
                if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_POSITION) == m.annotations.end())
                {
                    new_shader_code += function.input_parameter_name + "." + m.name + " = " + "in_" + m.name + ";\n";
                }
            }
        }
    }


    if (function.has_output)
    {
        new_shader_code += code.substr(function.open_brace_index - function.function_start_index + 1, function.return_type_index - function.open_brace_index - 1) + "\n";
        if (function.shader_type == "geometry")
        {
            new_shader_code += "for (u32 i = 0; i < " + std::to_string(function.return_type_count) + "; ++i)\n{\n";

            for (u64 i = 0; i < function.return_type.members.size(); ++i)
            {
                Shader_Struct_Member m = function.return_type.members[i];
                std::string out_variable = "out_" + m.name;

                if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_POSITION) != m.annotations.end())
                {
                    out_variable = "gl_Position";
                }

                new_shader_code += out_variable + " = " + function.return_type_name + "[i]" + "." + m.name + ";\n";
            }
            new_shader_code += "EmitVertex();\n}\nEndPrimitive();\n";
        }
        else
        {
            for (u64 i = 0; i < function.return_type.members.size(); ++i)
            {
                Shader_Struct_Member m = function.return_type.members[i];
                std::string out_variable = "out_" + m.name;

                if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_POSITION) != m.annotations.end())
                {
                    out_variable = "gl_Position";
                }

                new_shader_code += out_variable + " = " + function.return_type_name + "." + m.name + ";\n";
            }
        }
        new_shader_code += code.substr(function.return_semicolon_index - function.function_start_index + 1, function.close_brace_index - function.return_semicolon_index) + "\n";
    }
    else
    {
        new_shader_code += code.substr(function.open_brace_index - function.function_start_index + 1, function.close_brace_index - function.open_brace_index) + "\n";
    }

    return new_shader_code;
}

u32 sizeof_type(std::string type)
{
    u32 result = 0;
    if (type == "s32" || type == "u32" || type == "f32")
    {
        result = 4;
    }
    else if (type == "f64" || type == "uv2" || type == "sv2" || type == "v2")
    {
        result = 8;
    }
    else if (type == "uv3" || type == "sv3" || type == "v3")
    {
        result = 12;
    }
    else if (type == "uv4" || type == "sv4" || type == "v4")
    {
        result = 16;
    }
    else if (type == "m3x3")
    {
        result = 36;
    }
    else if (type == "m4x4")
    {
        result = 64;
    }
    return result;
}

std::string type_to_vk_format(std::string type)
{
    std::string result = "";
    if (type == "uv2")
    {
        result = "VK_FORMAT_R32G32_UINT";
    }
    else if (type == "v2")
    {
        result = "VK_FORMAT_R32G32_SFLOAT";
    }
    else if (type == "v3")
    {
        result = "VK_FORMAT_R32G32B32_SFLOAT";
    }
    else if (type == "v4")
    {
        result = "VK_FORMAT_R32G32B32A32_SFLOAT";
    }
    return result;
}

std::string generate_pipeline_create_function(Shader_Pipeline pipeline, std::unordered_map<std::string, Shader_Function> shader_functions)
{
    Shader_Function vertex_shader, compute_shader;

    b32 vertex_shader_found = false;
    b32 compute_shader_found = false;
    for (u64 i = 0; i < pipeline.shader_functions.size(); ++i)
    {
        std::string shader_name = pipeline.shader_functions[i];
        if (shader_functions.find(shader_name) != shader_functions.end())
        {
            if (shader_functions[shader_name].shader_type == "vertex")
            {
                vertex_shader = shader_functions[shader_name];
                vertex_shader_found = true;
                break;
            }
            else if (shader_functions[shader_name].shader_type == "compute")
            {
                compute_shader = shader_functions[shader_name];
                compute_shader_found = true;
            }
        }
    }
    if (!vertex_shader_found && !compute_shader_found)
    {
        fprintf(stderr, "ERROR(%s): pipeline %s must have a vertex shader or be a compute pipeline\n", __FUNCTION__, pipeline.name.c_str());
        return "";
    }
    else if (compute_shader_found && pipeline.shader_functions.size() > 1)
    {
        fprintf(stderr, "ERROR(%s): pipeline %s must only have one compute shader\n", __FUNCTION__, pipeline.name.c_str());
        return "";
    }

    std::string result = "";
    std::transform(pipeline.name.begin(), pipeline.name.end(), pipeline.name.begin(), ::tolower);
    if (vertex_shader_found)
    {
        result += "internal VkResult vulkan_create_" + pipeline.name + "(VkDevice device, VkPipelineLayout pipeline_layout, VkRenderPass render_pass, u32 subpass, VkPipeline* pipeline)\n{\n";
        result += pipeline_input_assembly_state_create_info(pipeline.input_assembly_state);
        result += pipeline_rasterization_state_create_info(pipeline.rasterization_state);
        if (pipeline.has_color_blend_state)
        {
            result += pipeline_color_blend_state_create_info(pipeline.color_blend_state);
        }
        if (pipeline.has_depth_stencil_state)
        {
            result += pipeline_depth_stencil_state_create_info(pipeline.depth_stencil_state);
        }
        result += "\tVkResult result;\n";

        result += "\tVkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};\n";
        if (vertex_shader.has_input)
        {
            u32 binding_size = 0;
            s32 current_binding = -1;
            Shader_Struct_Member_Annotation last_annotation = SHADER_STRUCT_MEMBER_ANNOTATION_POSITION;

            std::string binding_descriptions = "\tVkVertexInputBindingDescription vertex_binding_descriptions[] = {";
            std::string attribute_descriptions = "\tVkVertexInputAttributeDescription vertex_attribute_descriptions[] = {";

            for (u64 i = 0; i < vertex_shader.input_parameter.members.size(); ++i)
            {
                Shader_Struct_Member_Annotation current_annotation = SHADER_STRUCT_MEMBER_ANNOTATION_PER_VERTEX;
                Shader_Struct_Member m = vertex_shader.input_parameter.members[i];
                if (std::find(m.annotations.begin(), m.annotations.end(), SHADER_STRUCT_MEMBER_ANNOTATION_PER_INSTANCE) != m.annotations.end())
                {
                    current_annotation = SHADER_STRUCT_MEMBER_ANNOTATION_PER_INSTANCE;
                }

                if (current_annotation != last_annotation || i == vertex_shader.input_parameter.members.size() - 1)
                {
                    if (current_binding != -1)
                    {
                        std::string line_ending = "}, ";
                        if (i == vertex_shader.input_parameter.members.size() - 1)
                        {
                            binding_size += sizeof_type(m.type);
                            line_ending = "}};\n";
                        }
                        binding_descriptions += "{" + std::to_string(current_binding) + ", " + std::to_string(binding_size) + ", " + (last_annotation == SHADER_STRUCT_MEMBER_ANNOTATION_PER_INSTANCE ? "VK_VERTEX_INPUT_RATE_INSTANCE" : "VK_VERTEX_INPUT_RATE_VERTEX") + line_ending;
                    }
                    if (i != vertex_shader.input_parameter.members.size() - 1)
                    {
                        ++current_binding;
                        binding_size = 0;
                    }
                }

                if (current_binding != -1 || i == vertex_shader.input_parameter.members.size() - 1)
                {
                    std::string line_ending = std::to_string(binding_size) + "}, ";
                    attribute_descriptions += "{" + std::to_string(i) + ", " + std::to_string(current_binding) + ", " + type_to_vk_format(m.type) + ", ";
                    if (i == vertex_shader.input_parameter.members.size() - 1)
                    {
                        line_ending = std::to_string(binding_size - sizeof_type(m.type)) + "}};\n";
                    }
                    attribute_descriptions += line_ending;
                }

                binding_size += sizeof_type(m.type);
                last_annotation = current_annotation;
            }
            result += binding_descriptions + attribute_descriptions;
            result += "\tvertex_input_state_create_info.vertexBindingDescriptionCount = sizeof(vertex_binding_descriptions) / sizeof(vertex_binding_descriptions[0]);\n";
            result += "\tvertex_input_state_create_info.pVertexBindingDescriptions = vertex_binding_descriptions;\n";
            result += "\tvertex_input_state_create_info.vertexAttributeDescriptionCount = sizeof(vertex_attribute_descriptions) / sizeof(vertex_attribute_descriptions[0]);\n";
            result += "\tvertex_input_state_create_info.pVertexAttributeDescriptions = vertex_attribute_descriptions;\n";
        }

        result += "\tVkDynamicState dynamic_states[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};\n";
        result += "\tVkPipelineDynamicStateCreateInfo dynamic_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};\n";
        result += "\tdynamic_state_create_info.dynamicStateCount = 2;\n";
        result += "\tdynamic_state_create_info.pDynamicStates = dynamic_states;\n";

        result += "\tVkPipelineViewportStateCreateInfo viewport_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};\n";
        result += "\tviewport_state_create_info.viewportCount = 1;\n";
        result += "\tviewport_state_create_info.scissorCount = 1;\n";

        result += "\tVkPipelineMultisampleStateCreateInfo multisample_state_create_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};\n";
        result += "\tmultisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;\n";

        result += "\tVkShaderModule shader_modules[" + std::to_string(pipeline.shader_functions.size()) + "];\n";
        result += "\tVkPipelineShaderStageCreateInfo shader_stages[" + std::to_string(pipeline.shader_functions.size()) + "] = {};\n";
        for (u64 i = 0; i < pipeline.shader_functions.size(); ++i)
        {
            result += "\tresult = vkCreateShaderModule(device, &" + pipeline.shader_functions[i] + "_create_info, NULL, shader_modules + " + std::to_string(i) + ");\n";
            result += "\tif (result != VK_SUCCESS)\n\t{\n\t\treturn result;\n\t}\n";
            result += "\tshader_stages[" + std::to_string(i) + "].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;\n";
            result += "\tshader_stages[" + std::to_string(i) + "].pName = \"main\";\n";
            result += "\tshader_stages[" + std::to_string(i) + "].stage = " + get_shader_stage_bit(shader_functions[pipeline.shader_functions[i]].shader_type) + ";\n";
            result += "\tshader_stages[" + std::to_string(i) + "].module = shader_modules[" + std::to_string(i) + "];\n";
        }

        result += "\tVkGraphicsPipelineCreateInfo pipeline_create_info = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};\n";
        result += "\tpipeline_create_info.stageCount = sizeof(shader_stages) / sizeof(shader_stages[0]);\n";
        result += "\tpipeline_create_info.pStages = shader_stages;\n";
        result += "\tpipeline_create_info.pVertexInputState = &vertex_input_state_create_info;\n";
        result += "\tpipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;\n";
        result += "\tpipeline_create_info.pDynamicState = &dynamic_state_create_info;\n";
        result += "\tpipeline_create_info.pViewportState = &viewport_state_create_info;\n";
        result += "\tpipeline_create_info.pRasterizationState = &rasterization_state_create_info;\n";
        result += "\tpipeline_create_info.pMultisampleState = &multisample_state_create_info;\n";
        result += "\tpipeline_create_info.layout = pipeline_layout;\n";
        result += "\tpipeline_create_info.renderPass = render_pass;\n";
        result += "\tpipeline_create_info.subpass = subpass;\n";
        if (pipeline.has_color_blend_state)
        {
            result += "\tpipeline_create_info.pColorBlendState = &color_blend_state_create_info;\n";
        }
        if (pipeline.has_depth_stencil_state)
        {
            result += "\tpipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;\n";
        }

        result += "\tresult = vkCreateGraphicsPipelines(device, NULL, 1, &pipeline_create_info, NULL, pipeline);\n";
        for (u64 i = 0; i < pipeline.shader_functions.size(); ++i)
        {
            result += "\tvkDestroyShaderModule(device, shader_modules[" + std::to_string(i) + "], NULL);\n";
        }

        result += "\treturn result;\n";
        result += "}\n";
    }
    else
    {
        result += "internal VkResult vulkan_create_" + pipeline.name + "(VkDevice device, VkPipelineLayout pipeline_layout, VkPipeline* pipeline)\n{\n";
        result += "\tVkShaderModule compute_shader_module;\n";
        result += "\tVkResult result = vkCreateShaderModule(device, &" + pipeline.shader_functions[0] + "_create_info, NULL, &compute_shader_module);\n";

        result += "\tVkComputePipelineCreateInfo compute_pipeline_create_info = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};\n";
        result += "\tcompute_pipeline_create_info.stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};\n";
        result += "\tcompute_pipeline_create_info.stage.pName = \"main\";\n";
        result += "\tcompute_pipeline_create_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;\n";
        result += "\tcompute_pipeline_create_info.stage.module = compute_shader_module;\n";
        result += "\tcompute_pipeline_create_info.layout = pipeline_layout;\n";
        result += "\tresult = vkCreateComputePipelines(device, NULL, 1, &compute_pipeline_create_info, NULL, pipeline);\n";

        result += "\tvkDestroyShaderModule(device, compute_shader_module, NULL);\n";

        result += "\treturn result;\n";
        result += "}\n";
    }

    return result;
}

std::string compile_shaders(std::wstring compiler_path, std::wstring optimizer_path, char* shader_code, std::string shader_header, std::unordered_map<std::string, Shader_Function> shader_functions, std::unordered_map<std::string, Shader_Layout> shader_layouts, std::unordered_map<std::string, Shader_Struct> shader_structs, std::vector<Shader_Pipeline> shader_pipelines)
{
    std::string result = "";
    for (auto sf_it : shader_functions)
    {
        std::string code = std::string(shader_code);
        std::unordered_map<std::string, Shader_Layout> layouts = shader_layouts;

        // TODO: Should probably check for duplicate function names!
        Shader_Function function = sf_it.second;
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

        for (auto it : shader_structs)
        {
            Shader_Struct s = it.second;
            for (u64 k = 0; k < s.members.size(); ++k)
            {
                for (u64 l = s.members[k].name_end_index + 1; l < s.members[k].semicolon_index; ++l)
                {
                    code[l] = ' ';
                }
            }
        }

        for (auto sf : shader_functions)
        {
            if (sf.second.name != sf_it.second.name)
            {
                Shader_Function f = sf.second;
                for (u64 index = f.function_start_index; index <= f.close_brace_index; ++index)
                {
                    if (!is_whitespace(code[index]))
                    {
                        code[index] = ' ';
                    }
                }
            }
        }

        for (u64 j = 0; j < shader_pipelines.size(); ++j)
        {
            Shader_Pipeline p = shader_pipelines[j];
            for (u64 index = p.pipeline_index; index <= p.close_brace_index + 1; ++index)
            {
                if (!is_whitespace(code[index]))
                {
                    code[index] = ' ';
                }
            }
        }

        std::string new_function = generate_new_shader_function(code.substr(function.function_start_index, function.close_brace_index + 1 - function.function_start_index), function);
        code.replace(function.function_start_index, function.close_brace_index + 1 - function.function_start_index, "");
        code.insert(function.function_start_index, new_function);

        code = shader_header + code;

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

    for (u64 i = 0; i < shader_pipelines.size(); ++i)
    {
        result += generate_pipeline_create_function(shader_pipelines[i], shader_functions);
    }

    _wsystem(L"del *.spv *.tsd");
    return result;
}

std::string compile_shader_file(std::wstring compiler_path, std::wstring optimizer_path, std::wstring filepath)
{
    char* shader_code = read_file(filepath.c_str());
    Tokenizer tokenizer = {shader_code};

    std::unordered_map<std::string, Shader_Function> shader_functions;
    std::unordered_map<std::string, Shader_Layout> shader_layouts;
    std::unordered_map<std::string, Shader_Struct> shader_structs;
    std::vector<Shader_Pipeline> shader_pipelines;

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
            else if (token_equals(token, "struct"))
            {
                parse_shader_struct(&tokenizer, shader_code, token.text - shader_code, &shader_structs);
            }
            else if (token_equals(token, "pipeline"))
            {
                parse_shader_pipeline(&tokenizer, shader_code, token.text - shader_code, &shader_pipelines);
            }
            else if ((token_equals(token, "vertex") || token_equals(token, "fragment") || token_equals(token, "geometry") || token_equals(token, "compute")) && eat_token(&tokenizer, TOKEN_TYPE_OPEN_PARENTHESIS))
            {
                parse_shader(&tokenizer, shader_code, std::string(token.text, token.length), token.text - shader_code, &shader_functions, shader_structs);
            }
        } break;
        }
    }

    std::string shader_header = R"(
        #version 450
        #extension GL_EXT_nonuniform_qualifier : enable
        #extension GL_EXT_samplerless_texture_functions : enable

        #define s32 int
        #define u32 uint
        #define f32 float
        #define f64 double

        #define v2 vec2
        #define v3 vec3
        #define v4 vec4
        #define quat v4
        #define uv2 uvec2
        #define uv3 uvec3
        #define uv4 uvec4
        #define sv2 ivec2
        #define sv3 ivec3
        #define sv4 ivec4
        #define m3x3 mat3
        #define m4x4 mat4

        #define PI 3.14159265358979323846
    )";

    return compile_shaders(compiler_path, optimizer_path, shader_code, shader_header, shader_functions, shader_layouts, shader_structs, shader_pipelines);
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