//---------------------------------------------------------------------------------------
// utf8.h
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2019, Steffen Schümann <s.schuemann@pobox.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
//---------------------------------------------------------------------------------------
#pragma once

#include <cstdint>
#include <string>

#define GHC_INLINE inline

namespace ghc {
namespace detail {

    GHC_INLINE bool in_range(uint32_t c, uint32_t lo, uint32_t hi)
    {
        return (static_cast<uint32_t>(c - lo) < (hi - lo + 1));
    }

    GHC_INLINE bool is_surrogate(uint32_t c)
    {
        return in_range(c, 0xd800, 0xdfff);
    }

    GHC_INLINE bool is_high_surrogate(uint32_t c)
    {
        return (c & 0xfffffc00) == 0xd800;
    }

    GHC_INLINE bool is_low_surrogate(uint32_t c)
    {
        return (c & 0xfffffc00) == 0xdc00;
    }

    GHC_INLINE void appendUTF8(std::string& str, uint32_t unicode)
    {
        if (unicode <= 0x7f) {
            str.push_back(static_cast<char>(unicode));
        }
        else if (unicode >= 0x80 && unicode <= 0x7ff) {
            str.push_back(static_cast<char>((unicode >> 6) + 192));
            str.push_back(static_cast<char>((unicode & 0x3f) + 128));
        }
        else if ((unicode >= 0x800 && unicode <= 0xd7ff) || (unicode >= 0xe000 && unicode <= 0xffff)) {
            str.push_back(static_cast<char>((unicode >> 12) + 224));
            str.push_back(static_cast<char>(((unicode & 0xfff) >> 6) + 128));
            str.push_back(static_cast<char>((unicode & 0x3f) + 128));
        }
        else if (unicode >= 0x10000 && unicode <= 0x10ffff) {
            str.push_back(static_cast<char>((unicode >> 18) + 240));
            str.push_back(static_cast<char>(((unicode & 0x3ffff) >> 12) + 128));
            str.push_back(static_cast<char>(((unicode & 0xfff) >> 6) + 128));
            str.push_back(static_cast<char>((unicode & 0x3f) + 128));
        }
        else {
#ifdef GHC_RAISE_UNICODE_ERRORS
            throw filesystem_error("Illegal code point for unicode character.", str, std::make_error_code(std::errc::illegal_byte_sequence));
#else
            appendUTF8(str, 0xfffd);
#endif
        }
    }

    // Thanks to Bjoern Hoehrmann (https://bjoern.hoehrmann.de/utf-8/decoder/dfa/)
    // and Taylor R Campbell for the ideas to this DFA approach of UTF-8 decoding;
    // Generating debugging and shrinking my own DFA from scratch was a day of fun!
    enum utf8_states_t { S_STRT = 0, S_RJCT = 8 };

    GHC_INLINE unsigned consumeUtf8Fragment(const unsigned state, const uint8_t fragment, uint32_t& codepoint)
    {
        static const uint32_t utf8_state_info[] = {
            // encoded states
            0x11111111u, 0x11111111u, 0x77777777u, 0x77777777u, 0x88888888u, 0x88888888u, 0x88888888u, 0x88888888u, 0x22222299u, 0x22222222u, 0x22222222u, 0x22222222u, 0x3333333au, 0x33433333u, 0x9995666bu, 0x99999999u,
            0x88888880u, 0x22818108u, 0x88888881u, 0x88888882u, 0x88888884u, 0x88888887u, 0x88888886u, 0x82218108u, 0x82281108u, 0x88888888u, 0x88888883u, 0x88888885u, 0u,          0u,          0u,          0u,
        };
        uint8_t category = fragment < 128 ? 0 : (utf8_state_info[(fragment >> 3) & 0xf] >> ((fragment & 7) << 2)) & 0xf;
        codepoint = (state ? (codepoint << 6) | (fragment & 0x3fu) : (0xffu >> category) & fragment);
        return state == S_RJCT ? static_cast<unsigned>(S_RJCT) : static_cast<unsigned>((utf8_state_info[category + 16] >> (state << 2)) & 0xf);
    }

    GHC_INLINE bool validUtf8(const std::string& utf8String)
    {
        std::string::const_iterator iter = utf8String.begin();
        unsigned utf8_state = S_STRT;
        std::uint32_t codepoint = 0;
        while (iter < utf8String.end()) {
            if ((utf8_state = consumeUtf8Fragment(utf8_state, static_cast<uint8_t>(*iter++), codepoint)) == S_RJCT) {
                return false;
            }
        }
        if (utf8_state) {
            return false;
        }
        return true;
    }

}  // namespace detail

GHC_INLINE int characterWidth(std::uint32_t codepoint)
{
#ifndef _WIN32
    return ::wcwidth(static_cast<wchar_t>(codepoint));
#else
    return 1 + (codepoint >= 0x1100 && (codepoint <= 0x115f ||                                                                                                // Hangul Jamo init. consonants
                                        codepoint == 0x2329 || codepoint == 0x232a || (codepoint >= 0x2e80 && codepoint <= 0xa4cf && codepoint != 0x303f) ||  // CJK ... Yi
                                        (codepoint >= 0xac00 && codepoint <= 0xd7a3) ||                                                                       // Hangul Syllables
                                        (codepoint >= 0xf900 && codepoint <= 0xfaff) ||                                                                       // CJK Compatibility Ideographs
                                        (codepoint >= 0xfe10 && codepoint <= 0xfe19) ||                                                                       // Vertical forms
                                        (codepoint >= 0xfe30 && codepoint <= 0xfe6f) ||                                                                       // CJK Compatibility Forms
                                        (codepoint >= 0xff00 && codepoint <= 0xff60) ||                                                                       // Fullwidth Forms
                                        (codepoint >= 0xffe0 && codepoint <= 0xffe6) || (codepoint >= 0x20000 && codepoint <= 0x2fffd) || (codepoint >= 0x30000 && codepoint <= 0x3fffd)));
#endif
}

GHC_INLINE std::uint32_t utf8Increment(std::string::const_iterator& iter, const std::string::const_iterator& end)
{
    unsigned utf8_state = detail::S_STRT;
    std::uint32_t codepoint = 0;
    while (iter != end) {
        if ((utf8_state = detail::consumeUtf8Fragment(utf8_state, (uint8_t)*iter++, codepoint)) == detail::S_STRT) {
            return codepoint;
        }
        else if (utf8_state == detail::S_RJCT) {
            return 0xfffd;
        }
    }
    return 0xfffd;
}

template <class StringType, typename std::enable_if<(sizeof(typename StringType::value_type) == 1)>::type* = nullptr>
inline StringType fromUtf8(const std::string& utf8String, const typename StringType::allocator_type& alloc = typename StringType::allocator_type())
{
    return StringType(utf8String.begin(), utf8String.end(), alloc);
}

template <class StringType, typename std::enable_if<(sizeof(typename StringType::value_type) == 2)>::type* = nullptr>
inline StringType fromUtf8(const std::string& utf8String, const typename StringType::allocator_type& alloc = typename StringType::allocator_type())
{
    StringType result(alloc);
    result.reserve(utf8String.length());
    std::string::const_iterator iter = utf8String.begin();
    unsigned utf8_state = detail::S_STRT;
    std::uint32_t codepoint = 0;
    while (iter < utf8String.end()) {
        if ((utf8_state = detail::consumeUtf8Fragment(utf8_state, static_cast<uint8_t>(*iter++), codepoint)) == detail::S_STRT) {
            if (codepoint <= 0xffff) {
                result += static_cast<typename StringType::value_type>(codepoint);
            }
            else {
                codepoint -= 0x10000;
                result += static_cast<typename StringType::value_type>((codepoint >> 10) + 0xd800);
                result += static_cast<typename StringType::value_type>((codepoint & 0x3ff) + 0xdc00);
            }
            codepoint = 0;
        }
        else if (utf8_state == detail::S_RJCT) {
#ifdef GHC_RAISE_UNICODE_ERRORS
            throw filesystem_error("Illegal byte sequence for unicode character.", utf8String, std::make_error_code(std::errc::illegal_byte_sequence));
#else
            result += static_cast<typename StringType::value_type>(0xfffd);
            utf8_state = detail::S_STRT;
            codepoint = 0;
#endif
        }
    }
    if (utf8_state) {
#ifdef GHC_RAISE_UNICODE_ERRORS
        throw filesystem_error("Illegal byte sequence for unicode character.", utf8String, std::make_error_code(std::errc::illegal_byte_sequence));
#else
        result += static_cast<typename StringType::value_type>(0xfffd);
#endif
    }
    return result;
}

template <class StringType, typename std::enable_if<(sizeof(typename StringType::value_type) == 4)>::type* = nullptr>
inline StringType fromUtf8(const std::string& utf8String, const typename StringType::allocator_type& alloc = typename StringType::allocator_type())
{
    StringType result(alloc);
    result.reserve(utf8String.length());
    std::string::const_iterator iter = utf8String.begin();
    unsigned utf8_state = detail::S_STRT;
    std::uint32_t codepoint = 0;
    while (iter < utf8String.end()) {
        if ((utf8_state = detail::consumeUtf8Fragment(utf8_state, static_cast<uint8_t>(*iter++), codepoint)) == detail::S_STRT) {
            result += static_cast<typename StringType::value_type>(codepoint);
            codepoint = 0;
        }
        else if (utf8_state == detail::S_RJCT) {
#ifdef GHC_RAISE_UNICODE_ERRORS
            throw filesystem_error("Illegal byte sequence for unicode character.", utf8String, std::make_error_code(std::errc::illegal_byte_sequence));
#else
            result += static_cast<typename StringType::value_type>(0xfffd);
            utf8_state = detail::S_STRT;
            codepoint = 0;
#endif
        }
    }
    if (utf8_state) {
#ifdef GHC_RAISE_UNICODE_ERRORS
        throw filesystem_error("Illegal byte sequence for unicode character.", utf8String, std::make_error_code(std::errc::illegal_byte_sequence));
#else
        result += static_cast<typename StringType::value_type>(0xfffd);
#endif
    }
    return result;
}

template <typename charT, typename traits, typename Alloc, typename std::enable_if<(sizeof(charT) == 1), int>::type size = 1>
inline std::string toUtf8(const std::basic_string<charT, traits, Alloc>& unicodeString)
{
    return std::string(unicodeString.begin(), unicodeString.end());
}

template <typename charT, typename traits, typename Alloc, typename std::enable_if<(sizeof(charT) == 2), int>::type size = 2>
inline std::string toUtf8(const std::basic_string<charT, traits, Alloc>& unicodeString)
{
    std::string result;
    for (auto iter = unicodeString.begin(); iter != unicodeString.end(); ++iter) {
        char32_t c = *iter;
        if (detail::is_surrogate(c)) {
            ++iter;
            if (iter != unicodeString.end() && detail::is_high_surrogate(c) && detail::is_low_surrogate(*iter)) {
                detail::appendUTF8(result, (char32_t(c) << 10) + *iter - 0x35fdc00);
            }
            else {
#ifdef GHC_RAISE_UNICODE_ERRORS
                throw filesystem_error("Illegal code point for unicode character.", result, std::make_error_code(std::errc::illegal_byte_sequence));
#else
                detail::appendUTF8(result, 0xfffd);
                if (iter == unicodeString.end()) {
                    break;
                }
#endif
            }
        }
        else {
            detail::appendUTF8(result, c);
        }
    }
    return result;
}

template <typename charT, typename traits, typename Alloc, typename std::enable_if<(sizeof(charT) == 4), int>::type size = 4>
inline std::string toUtf8(const std::basic_string<charT, traits, Alloc>& unicodeString)
{
    std::string result;
    for (auto c : unicodeString) {
        detail::appendUTF8(result, static_cast<uint32_t>(c));
    }
    return result;
}

template <typename charT>
inline std::string toUtf8(const charT* unicodeString)
{
    return toUtf8(std::basic_string<charT, std::char_traits<charT>>(unicodeString));
}

}  // namespace ghc

#undef GHC_INLINE
