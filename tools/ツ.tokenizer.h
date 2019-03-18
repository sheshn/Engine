#pragma once

#include "../ツ/ツ.types.h"

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

    union
    {
        f64 f64;
        u64 u64;
        s64 s64;
    };
    b32 is_float;
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
        else if (character == '-' || is_number(character))
        {
            while (is_number(*tokenizer->at))
            {
                advance_tokenizer(tokenizer, 1);
            }

            token.is_float = false;
            if (*tokenizer->at == '.' || *tokenizer->at == 'x')
            {
                if (*tokenizer->at == '.')
                {
                    token.is_float = true;
                }

                advance_tokenizer(tokenizer, 1);
                while (is_hexadecimal(*tokenizer->at))
                {
                    advance_tokenizer(tokenizer, 1);
                }
            }

            token.type = TOKEN_TYPE_NUMBER;
            token.length = tokenizer->at - token.text;
            if (token.is_float)
            {
                token.f64 = strtod(token.text, NULL);
            }
            else if (character == '-')
            {
                token.s64 = strtoll(token.text, NULL, 0);
            }
            else
            {
                token.u64 = strtoull(token.text, NULL, 0);
            }
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