/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/EBase/Utility.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:55:53 $
 * $Revision: 1.6 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Algemeine Utility-Funktionen
 *****************************************************************************
 *
 * $Log: Utility.cpp,v $
 * Revision 1.6  2000/02/24 09:55:53  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.5  1999/11/17 08:58:15  S.Schuemann
 * - support für multiple CRs
 *
 * - vielfache Änderungen für Vorlage 1.4 beta 8
 *
 * Revision 1.4  1999/11/03 10:21:38  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.3  1999/10/26 13:24:31  S.Schuemann
 * - IsEqual() ignoriert nun Spaces
 *
 * Revision 1.2  1999/10/18 21:31:46  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#include "Utility.h"

#include <sys/stat.h>
#include <algorithm>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "Expression.h"
#include "Hash.h"
#include "Value.h"
#include "charencoding.h"
#include "regexp.h"
#include "utf8.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#ifndef __APPLE__
#include <termio.h>
#endif
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#define _vsnprintf vsnprintf
#endif

extern std::string GetConfigFileName();

using namespace std;

// int32_t g_nFlags = 0;
std::set<int> g_coFlags;
std::string g_sSrcFile;
int32_t g_nSrcLine = -1;
uint32_t g_nStepCount = 0;
uint32_t g_nLastErrorStep = ~0U;
int g_returnCode = 0;
std::shared_ptr<CharacterMapper> g_pOutputMapper;
bool g_bUTF8 = false;

Keywords g_coKeywords;

// ISO-8859-1 tolower-Mapping (evtl. incl. DOS-Mapping)
int g_toLowerMapping[256] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25,
                             0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b,
                             0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70, 0x71,
                             0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,

#ifdef _WIN32
                             // DOS-Mappings eingemischt (Ohne DOS-Beta bzw. �, da sonst ISO-� �berdeckt wird)
                             0xe7, 0xfc, 0xe9, 0xe2, 0xe4, 0xe0, 0xe5, 0xe7, 0xea, 0xeb, 0xe8, 0xef, 0xee, 0xec, 0xe4, 0xe5, 0xe9, 0xe6, 0xe6, 0xf4, 0xf6, 0xf2, 0xfb, 0xf9, 0xff, 0xf6, 0xfc, 0xa2, 0xa3, 0xa5, 0x9e, 0x9f, 0xe1, 0xed, 0xf3, 0xfa, 0xf1, 0xf1,
                             0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb,
                             0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1,
                             0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
#else
                             0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0xa0,
                             0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1,
                             0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2,
                             0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
#endif
};

std::string iso2utf8(const std::string& txt, bool doit)
{
    if (!doit)
        return txt;
    std::string str(txt);
    return iso885915ToUtf8(str);
}

std::string& iso885915ToUtf8(std::string& text)
{
    std::unique_ptr<std::string> pUTF8;

    //    MPF_DEBUGLOG(DecodingContext, 3, ("ToUTF8: '%s'") % text );
    for (std::string::const_iterator i = text.begin(); i != text.end(); ++i) {
        if ((unsigned char)(*i) < 128) {
            if (pUTF8) {
                *pUTF8 += *i;
            }
        }
        else if ((unsigned char)(*i) == 0xa4) {
            if (!pUTF8) {
                pUTF8.reset(new std::string());
                pUTF8->reserve(text.length() + 5);
                pUTF8->assign((std::string::const_iterator)text.begin(), i);
            }
            *pUTF8 += char(0xe2);
            *pUTF8 += char(0x82);
            *pUTF8 += char(0xac);
        }
        else {
            if (!pUTF8) {
                pUTF8.reset(new std::string());
                pUTF8->reserve(text.length() + 5);
                pUTF8->assign((std::string::const_iterator)text.begin(), i);
            }
            *pUTF8 += char((((unsigned char)(*i) >> 6) & 31) | 192);
            *pUTF8 += char(((*i) & 63) | 128);
        }
    }
    if (pUTF8) {
        text = *pUTF8;
    }
    return text;
}

std::string& Utf8toIso885915(std::string& text)
{
    std::unique_ptr<std::string> pISO;
    for (std::string::const_iterator i = text.begin(); i != text.end(); ++i) {
        if ((unsigned char)(*i) < 128) {
            if (pISO) {
                *pISO += *i;
            }
        }
        else if ((unsigned char)(*i) >= 0xe0) {
            if (!pISO) {
                pISO.reset(new std::string());
                pISO->reserve(text.length());
                pISO->assign((std::string::const_iterator)text.begin(), i);
            }
            if ((unsigned char)(*i++) == 0xe2) {
                if (i != text.end() && (unsigned char)(*i++) == 0x82) {
                    if (i != text.end() && (unsigned char)(*i) == 0xac) {
                        *pISO += char(0xa4);
                    }
                    else {
                        *pISO += '?';
                    }
                }
                else {
                    *pISO += '?';
                }
            }
            else {
                *pISO += '?';
            }
        }
        else {
            if (!pISO) {
                pISO.reset(new std::string());
                pISO->reserve(text.length());
                pISO->assign((std::string::const_iterator)text.begin(), i);
            }
            char c = char(((*i++) & 31) << 6);
            if (i != text.end()) {
                *pISO += (char)(c | (*i & 63));
            }
        }
        if (i == text.end()) {
            break;
        }
    }
    if (pISO) {
        text = *pISO;
    }
    return text;
}

static void Default(void* pDat, const char* pszTxt) {}

void (*g_pfErrorFunc)(void*, const char*) = Default;

char* FormatMsg(const char* msg, ...)
{
    char* p = new char[4096];
    va_list list;
    va_start(list, msg);
    vsnprintf(p, 4096, msg, list);
    return p;
}

void ErrorMessage(void* pDat, const char* pszStr)
{
    static std::set<std::string> coErrMsgPool;
    bool bWarn = CRegExp::Match(pszStr, "(?i)(Warnung|Warning):");
    if (!IsFlag(VF_NOWARNINGS) || !bWarn) {
        std::string sErr = std::string(pszStr) + "\n";
        if (!IsFlag(VF_SUPPRESSMULTIERRORS) || coErrMsgPool.find(sErr) == coErrMsgPool.end()) {
            g_pfErrorFunc(pDat, sErr.c_str());
            coErrMsgPool.insert(sErr);
            COutput::Target("stderr")->Write(sErr);
        }
        //        Message( sErr.c_str() );
    }
    if (!bWarn) {
        g_nLastErrorStep = g_nStepCount;
        g_returnCode = -1;
    }
    delete[] (char*)pszStr;
}

/*
void VErrorMessage( const char* pszStr )
{
    std::string sErr = std::string( pszStr ) + "\n";
    COutput::Target( "stderr" )->Write( sErr );
//    Message( sErr.c_str() );
        delete[] (char*)pszStr;
}
*/

void ConsoleMessage(const char* pszStr)
{
    if (!IsFlag(VF_NOCONSOLE)) {
        COutput::Target("console")->Write(pszStr);
    }
    delete[] (char*)pszStr;
}

void TraceMessage(const char* pszStr)
{
    COutput::Target("trace")->Write(pszStr);
    //	Message( pszStr );
    delete[] (char*)pszStr;
}

bool g_bForceEOL = false;

#ifndef _GRAPHICAL_

// extern FILE* g_hErr;
void Message(const char* pszStr)
{
    /*
        static bool bEOL = true;

        int l = strlen( pszStr );
        if( g_bForceEOL || ( !bEOL && strchr( pszStr, ':' ) ) )
        {
            fprintf( g_hErr, "\n" );
        }
            fprintf( g_hErr, "%s", pszStr );
        if( l && ( pszStr[l-1]==10 || pszStr[l-1]==13 ) )
            bEOL = true;
        else
            bEOL = false;
        g_bForceEOL = false;
    */
}

#else

void Message(const char* pszStr) {}

#endif

void (*g_pfHandler)(void);

#ifdef _WIN32

BOOL WINAPI BreakHandler(DWORD nCtrlWord)
{
    if (nCtrlWord == CTRL_BREAK_EVENT) {
        g_pfHandler();
        return TRUE;
    }
    else {
        return FALSE;
    }
}

void SetBreakHandler(void (*pfHandler)(void))
{
    SetConsoleCtrlHandler(BreakHandler, TRUE);
    g_pfHandler = pfHandler;
}

#else

void SetBreakHandler(void (*pfHandler)(void))
{
    g_pfHandler = pfHandler;
}

#endif

const char* itoa36(int i)
{
    static char s[8];
    char* dst;
    char c;
    int neg = 0;

    s[7] = 0;
    dst = s + 6;

    if (i != 0) {
        if (i < 0) {
            i = -i;
            neg = 1;
        }
        while (i) {
            int x = i % 36;
            i = i / 36;
            if (x < 10)
                *(dst--) = (char)('0' + x);
            else {
                c = (char)('a' + (x - 10));
                if (c == 'l')
                    c = 'L';
                *(dst--) = c;
            }
        }
        if (neg)
            *(dst) = '-';
        else
            ++dst;
    }
    else
        *dst = '0';

    return dst;
}

const char* itoan(int32_t i, int base)
{
    static char s[66];
    char* dst;
    char c;
    int neg = 0;

    s[65] = 0;
    dst = s + 64;

    if (i != 0) {
        if (i < 0) {
            i = -i;
            neg = 1;
        }
        while (i) {
            int x = i % base;
            i = i / base;
            if (x < 10)
                *(dst--) = (char)('0' + x);
            else {
                c = (char)('a' + (x - 10));
                if (c == 'l')
                    c = 'L';
                *(dst--) = c;
            }
        }
        if (neg)
            *(dst) = '-';
        else
            ++dst;
    }
    else
        *dst = '0';

    return dst;
}

int32_t EinheitenNummer(const std::string& sENStr)
{
    int32_t en;
    if (IsFlag(VF_BASE36))
        en = (int32_t)strtol(sENStr.c_str(), NULL, 36);
    else
        en = (int32_t)strtol(sENStr.c_str(), NULL, 10);
    return en;
}

int32_t FindNextENum(const std::string& sTxt, std::string::size_type& p)
{
    std::string::size_type p1, p2 = 0, ph;

    p1 = size_t(p);

    while (p1 < sTxt.size()) {
        p1 = sTxt.find('(', p1);
        if (p1 == std::string::npos)
            return -1;

        p2 = sTxt.find(')', p1);
        if (p2 == std::string::npos)
            return -1;

        ph = sTxt.find(',', p1);
        if (ph == std::string::npos || ph > p2)
            break;

        p1++;
    }
    if (p1 >= sTxt.size()) {
        return -1;
    }

    if (!p2) {
        return -1;
    }
    p = p2;
    return EinheitenNummer(sTxt.substr(p1 + 1, p2 - p1 - 1));
}

bool FindNextRegion(const std::string& sTxt, std::string::size_type& p, int& x, int& y, int& z)
{
    size_t p1, p2 = std::string::npos, ph;

    p1 = size_t(p);

    while (p1 < sTxt.size()) {
        p1 = sTxt.find('(', p1);
        if (p1 == std::string::npos)
            return false;

        p2 = sTxt.find(')', p1);
        if (p2 == std::string::npos)
            return false;

        ph = sTxt.find(',', p1);
        if (ph != std::string::npos && ph < p2)
            break;

        p1++;
    }
    if (p1 >= sTxt.size())
        return false;

    if (p2 != std::string::npos) {
        p = p2;
    }
    x = int(atol(sTxt.c_str() + p1 + 1));
    ph = sTxt.find(',', p1 + 1);
    if (ph != std::string::npos) {
        y = int(atol(sTxt.c_str() + ph + 1));
        ph = sTxt.find(',', ph + 1);
        if (ph != std::string::npos) {
            z = int(atol(sTxt.c_str() + ph + 1));
        }
        else
            z = 0;
    }

    return true;
}

namespace detail {
struct DecodeResult
{
    char32_t cp = 0;
    std::size_t len = 0;
    bool valid = false;
};

inline DecodeResult try_decode_utf8(std::string_view s, std::size_t pos)
{
    const std::size_t rem = s.size() - pos;
    const auto b0 = static_cast<unsigned char>(s[pos]);

    if (b0 <= 0x7F)
        return {b0, 1, true};

    if (b0 < 0xC2)
        return {};

    if (b0 <= 0xDF) {
        if (rem < 2)
            return {};
        const auto b1 = static_cast<unsigned char>(s[pos + 1]);
        if ((b1 & 0xC0) != 0x80)
            return {};

        return {static_cast<char32_t>(((b0 & 0x1F) << 6) | (b1 & 0x3F)), 2, true};
    }

    if (b0 <= 0xEF) {
        if (rem < 3)
            return {};
        const auto b1 = static_cast<unsigned char>(s[pos + 1]);
        const auto b2 = static_cast<unsigned char>(s[pos + 2]);

        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80)
            return {};

        if (b0 == 0xE0 && b1 < 0xA0)
            return {};
        if (b0 == 0xED && b1 >= 0xA0)
            return {};

        return {static_cast<char32_t>(((b0 & 0x0F) << 12) | ((b1 & 0x3F) << 6) | (b2 & 0x3F)), 3, true};
    }

    if (b0 <= 0xF4) {
        if (rem < 4)
            return {};
        const auto b1 = static_cast<unsigned char>(s[pos + 1]);
        const auto b2 = static_cast<unsigned char>(s[pos + 2]);
        const auto b3 = static_cast<unsigned char>(s[pos + 3]);

        if ((b1 & 0xC0) != 0x80 || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
            return {};

        if (b0 == 0xF0 && b1 < 0x90)
            return {};
        if (b0 == 0xF4 && b1 > 0x8F)
            return {};

        return {static_cast<char32_t>(((b0 & 0x07) << 18) | ((b1 & 0x3F) << 12) | ((b2 & 0x3F) << 6) | (b3 & 0x3F)), 4, true};
    }

    return {};
}

inline std::u32string decode_mixed_utf8_latin1(std::string_view input)
{
    std::u32string out;
    out.reserve(input.size());

    std::size_t i = 0;
    while (i < input.size()) {
        DecodeResult r = try_decode_utf8(input, i);
        if (r.valid) {
            out.push_back(r.cp);
            i += r.len;
        }
        else {
            out.push_back(static_cast<unsigned char>(input[i]));
            ++i;
        }
    }

    return out;
}

struct Replacement
{
    std::u32string_view from;
    std::u32string_view to;
};

inline void append_folded_latin1(std::string& out, char32_t cp, char replacement)
{
    if (cp <= 0xFF) {
        out.push_back(static_cast<char>(static_cast<unsigned char>(cp)));
        return;
    }

    switch (cp) {
        case U'\u2018':  // ‘
        case U'\u2019':  // ’
        case U'\u2032':  // ′
            out.push_back('\'');
            return;

        case U'\u201C':  // “
        case U'\u201D':  // ”
        case U'\u2033':  // ″
            out.push_back('"');
            return;

        case U'\u2013':  // –
        case U'\u2014':  // —
        case U'\u2212':  // −
            out.push_back('-');
            return;

        case U'\u2026':  // …
            out += "...";
            return;

        case U'\u00A0':  // nbsp
            out.push_back(' ');
            return;

        default:
            out.push_back(replacement);
            return;
    }
}

inline std::string encode_latin1_with_folding(std::u32string_view input, char replacement)
{
    std::string out;
    out.reserve(input.size());

    for (char32_t cp : input)
        append_folded_latin1(out, cp, replacement);

    return out;
}

inline bool starts_with_at(std::u32string_view text, std::size_t pos, std::u32string_view needle)
{
    return pos + needle.size() <= text.size() && std::equal(needle.begin(), needle.end(), text.begin() + pos);
}

inline std::u32string apply_replacement_table(std::u32string_view input)
{
    // Conservative, explicit mojibake table.
    // Left side: common bad text as it appears after UTF-8 bytes were read as Latin-1.
    // Right side: intended Unicode text.
    static constexpr std::u32string_view A_umlaut_bad = U"Ã¤";
    static constexpr std::u32string_view O_umlaut_bad = U"Ã¶";
    static constexpr std::u32string_view U_umlaut_bad = U"Ã¼";
    static constexpr std::u32string_view A_umlaut_cap_bad = U"Ã„";
    static constexpr std::u32string_view O_umlaut_cap_bad = U"Ã–";
    static constexpr std::u32string_view U_umlaut_cap_bad = U"Ãœ";
    static constexpr std::u32string_view sz_bad = U"ÃŸ";

    static constexpr std::u32string_view agrave_bad = U"Ã ";
    static constexpr std::u32string_view aacute_bad = U"Ã¡";
    static constexpr std::u32string_view acirc_bad = U"Ã¢";
    static constexpr std::u32string_view atilde_bad = U"Ã£";
    static constexpr std::u32string_view aring_bad = U"Ã¥";
    static constexpr std::u32string_view aelig_bad = U"Ã¦";
    static constexpr std::u32string_view cced_bad = U"Ã§";
    static constexpr std::u32string_view egrave_bad = U"Ã¨";
    static constexpr std::u32string_view eacute_bad = U"Ã©";
    static constexpr std::u32string_view ecirc_bad = U"Ãª";
    static constexpr std::u32string_view euml_bad = U"Ã«";
    static constexpr std::u32string_view igrave_bad = U"Ã¬";
    static constexpr std::u32string_view iacute_bad = U"Ã­";
    static constexpr std::u32string_view icirc_bad = U"Ã®";
    static constexpr std::u32string_view iuml_bad = U"Ã¯";
    static constexpr std::u32string_view ntilde_bad = U"Ã±";
    static constexpr std::u32string_view ograve_bad = U"Ã²";
    static constexpr std::u32string_view oacute_bad = U"Ã³";
    static constexpr std::u32string_view ocirc_bad = U"Ã´";
    static constexpr std::u32string_view otilde_bad = U"Ãµ";
    static constexpr std::u32string_view oslash_bad = U"Ã¸";
    static constexpr std::u32string_view ugrave_bad = U"Ã¹";
    static constexpr std::u32string_view uacute_bad = U"Ãº";
    static constexpr std::u32string_view ucirc_bad = U"Ã»";
    static constexpr std::u32string_view yacute_bad = U"Ã½";
    static constexpr std::u32string_view thorn_bad = U"Ã¾";

    static constexpr std::u32string_view Agrave_bad = U"Ã€";
    static constexpr std::u32string_view Aacute_bad = U"Ã�";
    static constexpr std::u32string_view Acirc_bad = U"Ã‚";
    static constexpr std::u32string_view Atilde_bad = U"Ãƒ";
    static constexpr std::u32string_view Aring_bad = U"Ã…";
    static constexpr std::u32string_view Aelig_bad = U"Ã†";
    static constexpr std::u32string_view Cced_bad = U"Ã‡";
    static constexpr std::u32string_view Egrave_bad = U"Ãˆ";
    static constexpr std::u32string_view Eacute_bad = U"Ã‰";
    static constexpr std::u32string_view Ecirc_bad = U"ÃŠ";
    static constexpr std::u32string_view Euml_bad = U"Ã‹";
    static constexpr std::u32string_view Igrave_bad = U"ÃŒ";
    static constexpr std::u32string_view Iacute_bad = U"Ã�";
    static constexpr std::u32string_view Icirc_bad = U"ÃŽ";
    static constexpr std::u32string_view Iuml_bad = U"Ã�";
    static constexpr std::u32string_view Ntilde_bad = U"Ã‘";
    static constexpr std::u32string_view Ograve_bad = U"Ã’";
    static constexpr std::u32string_view Oacute_bad = U"Ã“";
    static constexpr std::u32string_view Ocirc_bad = U"Ã”";
    static constexpr std::u32string_view Otilde_bad = U"Ã•";
    static constexpr std::u32string_view Oslash_bad = U"Ã˜";
    static constexpr std::u32string_view Ugrave_bad = U"Ã™";
    static constexpr std::u32string_view Uacute_bad = U"Ãš";
    static constexpr std::u32string_view Ucirc_bad = U"Ã›";
    static constexpr std::u32string_view Yacute_bad = U"Ã�";
    static constexpr std::u32string_view Thorn_bad = U"Ãž";

    static constexpr std::u32string_view nbsp_bad = U"Â ";
    static constexpr std::u32string_view pound_bad = U"Â£";
    static constexpr std::u32string_view section_bad = U"Â§";
    static constexpr std::u32string_view copy_bad = U"Â©";
    static constexpr std::u32string_view reg_bad = U"Â®";
    static constexpr std::u32string_view degree_bad = U"Â°";
    static constexpr std::u32string_view plusmn_bad = U"Â±";

    static constexpr std::u32string_view rsquo_bad = U"â€™";
    static constexpr std::u32string_view lsquo_bad = U"â€˜";
    static constexpr std::u32string_view rdquo_bad = U"â€�";
    static constexpr std::u32string_view ldquo_bad = U"â€œ";
    static constexpr std::u32string_view endash_bad = U"â€“";
    static constexpr std::u32string_view emdash_bad = U"â€”";
    static constexpr std::u32string_view ellipsis_bad = U"â€¦";
    static constexpr std::u32string_view bullet_bad = U"â€¢";
    static constexpr std::u32string_view euro_bad = U"â‚¬";

    static constexpr Replacement table[] = {{A_umlaut_bad, U"ä"}, {O_umlaut_bad, U"ö"}, {U_umlaut_bad, U"ü"}, {A_umlaut_cap_bad, U"Ä"}, {O_umlaut_cap_bad, U"Ö"}, {U_umlaut_cap_bad, U"Ü"}, {sz_bad, U"ß"},

                                            {agrave_bad, U"à"},   {aacute_bad, U"á"},   {acirc_bad, U"â"},    {atilde_bad, U"ã"},       {aring_bad, U"å"},        {aelig_bad, U"æ"},        {cced_bad, U"ç"},     {egrave_bad, U"è"}, {eacute_bad, U"é"},
                                            {ecirc_bad, U"ê"},    {euml_bad, U"ë"},     {igrave_bad, U"ì"},   {iacute_bad, U"í"},       {icirc_bad, U"î"},        {iuml_bad, U"ï"},         {ntilde_bad, U"ñ"},   {ograve_bad, U"ò"}, {oacute_bad, U"ó"},
                                            {ocirc_bad, U"ô"},    {otilde_bad, U"õ"},   {oslash_bad, U"ø"},   {ugrave_bad, U"ù"},       {uacute_bad, U"ú"},       {ucirc_bad, U"û"},        {yacute_bad, U"ý"},   {thorn_bad, U"þ"},

                                            {Agrave_bad, U"À"},   {Aacute_bad, U"Á"},   {Acirc_bad, U"Â"},    {Atilde_bad, U"Ã"},       {Aring_bad, U"Å"},        {Aelig_bad, U"Æ"},        {Cced_bad, U"Ç"},     {Egrave_bad, U"È"}, {Eacute_bad, U"É"},
                                            {Ecirc_bad, U"Ê"},    {Euml_bad, U"Ë"},     {Igrave_bad, U"Ì"},   {Iacute_bad, U"Í"},       {Icirc_bad, U"Î"},        {Iuml_bad, U"Ï"},         {Ntilde_bad, U"Ñ"},   {Ograve_bad, U"Ò"}, {Oacute_bad, U"Ó"},
                                            {Ocirc_bad, U"Ô"},    {Otilde_bad, U"Õ"},   {Oslash_bad, U"Ø"},   {Ugrave_bad, U"Ù"},       {Uacute_bad, U"Ú"},       {Ucirc_bad, U"Û"},        {Yacute_bad, U"Ý"},   {Thorn_bad, U"Þ"},

                                            {nbsp_bad, U" "},     {pound_bad, U"£"},    {section_bad, U"§"},  {copy_bad, U"©"},         {reg_bad, U"®"},          {degree_bad, U"°"},       {plusmn_bad, U"±"},

                                            {rsquo_bad, U"’"},    {lsquo_bad, U"‘"},    {rdquo_bad, U"”"},    {ldquo_bad, U"“"},        {endash_bad, U"–"},       {emdash_bad, U"—"},       {ellipsis_bad, U"…"}, {bullet_bad, U"•"}, {euro_bad, U"€"}};

    std::u32string out;
    out.reserve(input.size());

    std::size_t i = 0;
    while (i < input.size()) {
        bool matched = false;

        for (const auto& r : table) {
            if (starts_with_at(input, i, r.from)) {
                out.append(r.to);
                i += r.from.size();
                matched = true;
                break;
            }
        }

        if (!matched) {
            out.push_back(input[i]);
            ++i;
        }
    }

    return out;
}
}  // namespace detail

std::string mixed_utf8_latin1_to_latin1(std::string_view input, char replacement)
{
    std::u32string decoded = detail::decode_mixed_utf8_latin1(input);
    std::u32string repaired = detail::apply_replacement_table(decoded);
    return detail::encode_latin1_with_folding(repaired, replacement);
}

std::string DeUmlaut(const std::string& sText)
{
    std::string::const_iterator is;
    static char Buff[256];
    char* str = Buff;
    int l = 0;

    is = sText.begin();
    while (is != sText.end()) {
        switch (*is) {
            case char(0xE4):
                *str++ = 'a';
                *str++ = 'e';
                break;  // Latin-1: ä
            case char(0xF6):
                *str++ = 'o';
                *str++ = 'e';
                break;  // Latin-1: ö
            case char(0xFC):
                *str++ = 'u';
                *str++ = 'e';
                break;  // Latin-1: ü
            case char(0xC4):
                *str++ = 'a';
                *str++ = 'e';
                break;  // Latin-1: Ä
            case char(0xD6):
                *str++ = 'o';
                *str++ = 'e';
                break;  // Latin-1: Ö
            case char(0xDC):
                *str++ = 'u';
                *str++ = 'e';
                break;  // Latin-1: Ü
            case char(0xDF):
                *str++ = 's';
                *str++ = 's';
                break;  // Latin-1: ß
#ifdef _WIN32
            case char(132):  // DOS-ä
            case char(142):  // DOS-Ä
                *str++ = 'a';
                *str++ = 'e';
                break;
            case char(148):  // DOS-ö
            case char(153):  // DOS-Ö
                *str++ = 'o';
                *str++ = 'e';
                break;
            case char(129):  // DOS-ü
            case char(154):  // DOS-Ü
                *str++ = 'u';
                *str++ = 'e';
                break;
            case char(225):  // DOS-ß
                *str++ = 's';
                *str++ = 's';
                break;
#endif
            default:
                *str++ = ToLower(*is);
        }
        is++;
        if (++l > 250)
            break;
    }
    *str = 0;
    return std::string(Buff);
}

bool IsEqual(const char* pcS1i, const char* pcS2i)
{
#ifdef _WIN32
    static const unsigned char pcUml[] = {0xc4, 0xd6, 0xdc, 0xe4, 0xf6, 0xfc, 0xdf, 142, 153, 154, 132, 148, 129, 225, 0};
    static const unsigned char pcAlt[] = "aouaousaouaous";
#else
    static const unsigned char pcUml[] = {0xc4, 0xd6, 0xdc, 0xe4, 0xf6, 0xfc, 0xdf, 0};
    static const unsigned char pcAlt[] = "aouaous";
#endif
    const unsigned char* pcS1 = (const unsigned char*)pcS1i;
    const unsigned char* pcS2 = (const unsigned char*)pcS2i;
    const unsigned char* h1;
    const unsigned char* h2;
    unsigned char c1, c2;
    while (*pcS1 && *pcS2) {
        do {
            while (' ' == (c1 = uint8_t(ToLower(*pcS1++))))
                ;
        } while (c1 == '~');
        do {
            while (' ' == (c2 = uint8_t(ToLower(*pcS2++))))
                ;
        } while (c2 == '~');
        if (!c1 || !c2) {
            return (c1 == c2);
        }
        if (c1 != c2) {
            h1 = (const unsigned char*)strchr((const char*)pcUml, c1);
            h2 = (const unsigned char*)strchr((const char*)pcUml, c2);
#ifdef _WIN32
            if (h1 > pcUml + 6)
                h1 -= 6;
            if (h2 > pcUml + 6)
                h2 -= 6;
#endif
            if (h1 == NULL && h2 == NULL)
                return false;
            if (h1 && h2) {
                if (h1 > h2) {
                    const unsigned char* ht = h2;
                    h2 = h1;
                    h1 = ht;
                }

                if ((*h1 == (unsigned char)0xc4 && *h2 != (unsigned char)0xe4) || (*h1 == (unsigned char)0xd6 && *h2 != (unsigned char)0xf6) || (*h1 == (unsigned char)0xdc && *h2 != (unsigned char)0xfc))
                    return false;
            }
            else {
                if (h2) {
                    h1 = h2;
                    h2 = pcS1++;
                }
                else {
                    h2 = pcS2++;
                    c1 = c2;
                }
                if (pcAlt[h1 - pcUml] != c1)
                    return false;
                if (h1 - pcUml < 6) {
                    if (*h2 != 'e' && *h2 != 'E')
                        return false;
                }
                else {
                    if (*h2 != 's' && *h2 != 'S')
                        return false;
                }
            }
        }
    }

    while (*pcS1 == ' ' || *pcS1 == '~')
        pcS1++;
    while (*pcS2 == ' ' || *pcS2 == '~')
        pcS2++;

    return (!*pcS1 && !*pcS2);
}

bool IsEqual(const std::string& sS1, const char* pcS2)
{
    return IsEqual(sS1.c_str(), pcS2);
}

bool IsMetaCommand(const std::string& sLine)
{
    std::string::const_iterator si;
    si = sLine.begin();
    while (si != sLine.end() && (*si == 32 || *si == 9))
        si++;
    if (si == sLine.end() || *si++ != '/')
        return false;
    if (si == sLine.end() || *si++ != '/')
        return false;
    while (si != sLine.end() && (*si == 32 || *si == 9))
        si++;
    return si != sLine.end() && *si == '#';
}

std::string Wrap(std::string& sTxt, size_t len)
{
    std::string sOut;

    if (sTxt.length() <= size_t(len)) {
        sOut = sTxt;
        sTxt = "";
    }
    else {
        size_t nPos, nKom;
        nPos = sTxt.find_last_of(" \t", size_t(len));
        nKom = sTxt.find_last_of(",", size_t(len));
        if (nPos != std::string::npos) {
            if (nKom != std::string::npos && nPos - nKom > 0 && nPos - nKom < 5)
                nPos = nKom + 1;
            sOut = sTxt.substr(0, nPos);
            sTxt.erase(0, nPos);
        }
        else {
            sOut = sTxt.substr(0, len);
            sTxt.erase(0, len);
        }
        nPos = sTxt.find_first_not_of(" \t");
        if (nPos != std::string::npos)
            sTxt.erase(0, nPos);
        else
            sTxt = "";
    }
    return sOut;
}

std::string ToString(int32_t n)
{
    return std::to_string(n);
}

std::string ToString(double f, unsigned n)
{
    std::stringstream os;
    os << std::fixed << std::setprecision(n > 8 ? 8 : static_cast<int>(n)) << f;
    return os.str();
}

std::string ToStringS(int32_t n)
{
    return n < 0 ? std::to_string(n) : "+" + std::to_string(n);
}

std::string ToStringS(double f, unsigned n)
{
    return f < 0 ? ToString(f, n) : "-" + ToString(f, n);
}

typedef std::mersenne_twister_engine<unsigned, 32, 351, 175, 19, 0xccab8ee7, 11, 0xffffffff, 7, 0x31b6ab00, 15, 0xffe50000, 17, 1812433253> mt11213b_t;

int32_t Random(int32_t seed)
{
    static mt11213b_t prng;
    static bool bInit = false;

    if (!bInit) {
        uint32_t nSeed = (uint32_t)(seed);
        if (!seed) {
            // 0x6E788FA0
            time_t nSeedStart = time(0);
            int32_t nCount = 0;
            while (time(0) == nSeedStart)
                nCount++;
            // nSeedStart = 0x42C123E6;
            // nCount = 0x15555E;
            char pcSeed[1024];
            snprintf(pcSeed, sizeof(pcSeed), "%s, %ld, %ld", ctime(&nSeedStart), (long)nSeedStart, nCount);
            nSeed = Hash((const unsigned char*)pcSeed, (uint32_t)strlen(pcSeed), 4711);
        }
        TRACEMSG(("Random used, seed: 0x%lX\n", nSeed));
        prng.seed((mt11213b_t::result_type)nSeed);
        bInit = true;
        return seed ? seed : prng() & 0x7fffffffL;
    }
    else {
        return prng() & 0x7fffffffL;
    }
}

bool FileExists(const std::string& sFName)
{
    FILE* fh;

    //	fprintf( stderr, "checke nach %s...\n", sFName.c_str() );
    fh = fopen(sFName.c_str(), "r");
    if (fh)
        fclose(fh);
    return fh ? true : false;
}

bool IsConsole(FILE* hFile)
{
#ifdef _WIN32
    struct _stat oStat;
    _fstat(_fileno(hFile), &oStat);
    return (oStat.st_mode & _S_IFCHR) != 0;
#else
    struct stat oStat;
    fstat(fileno(hFile), &oStat);
    return (oStat.st_mode & S_IFCHR) != 0;
#endif
}

bool GetConsoleSize(int& width, int& height)
{
#if defined(_WIN32)
    {
        CONSOLE_SCREEN_BUFFER_INFO scr;
        if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &scr)) {
            height = scr.srWindow.Bottom - scr.srWindow.Top + 1;
            width = scr.srWindow.Right - scr.srWindow.Left + 1;
        }
        else
            return false;
    }
#elif defined(TIOCGWINSZ)
    {
        struct winsize w;
        int nFile;
        if (IsConsole(stderr))
            nFile = 2;
        else if (IsConsole(stderr))
            nFile = 1;
        else
            return false;
        if (ioctl(nFile, TIOCGWINSZ, &w) == 0) {
            if (w.ws_row > 0)
                height = w.ws_row;
            if (w.ws_col > 0)
                width = w.ws_col;
        }
    }
#elif defined(WIOCGETD)
    {
        struct uwdata w;
        int nFile;
        if (IsConsole(stderr))
            nFile = 2;
        else if (IsConsole(stderr))
            nFile = 1;
        else
            return false;
        if (ioctl(nFile, WIOCGETD, &w) == 0) {
            if (w.uw_height > 0)
                height = w.uw_height / w.uw_vs;
            if (w.uw_width > 0)
                width = w.uw_width / w.uw_hs;
        }
    }
#endif
    return width > 0 && height > 0;
}

int _nLineCount = 0;
std::map<FILE*, bool> _bStreamMap;

void WriteToConsole(FILE* hFile, const char* pcText)
{
    static int width, height;
    static int nPos = 0;
    static char pcBuff[512];
#ifdef _WIN32
    static char pcOEMBuff[4096];
#endif
#ifdef DARWIN
    static CharacterMapper oLatinToMacConsole("iso-8859-1", "macintosh");
#endif
    std::string sOut;

    std::map<FILE*, bool>::const_iterator si = _bStreamMap.find(hFile);
    if (si == _bStreamMap.end()) {
        si = _bStreamMap.insert(std::map<FILE*, bool>::value_type(hFile, IsConsole(hFile))).first;
    }

    if ((*si).second) {
#ifdef _WIN32
        CharToOem(pcText, pcOEMBuff);
        pcText = pcOEMBuff;
#elif defined(DARWIN)
        sOut = oLatinToMacConsole.Map(std::string(pcText));
        pcText = sOut.c_str();
#endif
    }
    else {
        if (g_pOutputMapper->IsOK()) {
            sOut = g_pOutputMapper->Map(pcText);
            pcText = sOut.c_str();
        }
    }

    if ((*si).second && IsFlag(VF_PAGER)) {
        char* pStr = pcBuff;

        if (!_nLineCount) {
            if (!GetConsoleSize(width, height)) {
                width = 80;
                height = 25;
            }
        }

        while (*pcText) {
            while (*pcText && nPos < width) {
                switch (*pcText) {
                    case 9: {
                        int nEnd = (nPos & 3) + 8;
                        while (nPos < nEnd) {
                            if (nPos++ < width)
                                *pStr++ = ' ';
                        }
                        if (nPos >= width) {
                            *pStr = 0;
                            fprintf(hFile, "%s\n", pcBuff);
                            _nLineCount++;
                            pStr = pcBuff;
                            nPos -= width;
                            if (_nLineCount >= height - 1 && height > 5) {
                                fprintf(hFile, "[Weiter mit der Eingabetaste]");
                                InputFromConsole();
                            }
                        }
                        nEnd = 0;
                        while (nEnd++ < nPos)
                            *pStr++ = ' ';
                    } break;
                    case 10:
                        *pStr = 0;
                        fprintf(hFile, "%s\n", pcBuff);
                        _nLineCount++;
                        nPos = 0;
                        pStr = pcBuff;
                        if (_nLineCount >= height - 1 && height > 5) {
                            fprintf(hFile, "[Weiter mit der Eingabetaste]");
                            InputFromConsole();
                        }
                        break;
                    case 12:
                        break;
                    case 13:
                        *pStr++ = 13;
                        *pStr = 0;
                        fprintf(hFile, "%s", pcBuff);
                        nPos = 0;
                        break;
                    default:
                        *pStr++ = *pcText;
                        nPos++;
                }
                pcText++;
            }
            if (*pcText) {
                *pStr = 0;
                fprintf(hFile, "%s", pcBuff);
                _nLineCount++;
                nPos = 0;
                pStr = pcBuff;
                if (_nLineCount >= height - 1 && height > 5) {
                    fprintf(hFile, "[Weiter mit der Eingabetaste]");
                    InputFromConsole();
                }
            }
        }
        if (nPos) {
            *pStr = 0;
            fprintf(hFile, "%s", pcBuff);
        }
    }
    else {
        fprintf(hFile, "%s", pcText);
    }
}

std::string InputFromConsole()
{
    static char pcBuffer[1024];
#ifdef _WIN32
    static char pcOEMBuffer[1024];
    auto res = std::fgets(pcOEMBuffer, 1023, stdin);
    OemToChar(pcOEMBuffer, pcBuffer);
#else
    auto res = std::fgets(pcBuffer, 1023, stdin);
#endif
    _nLineCount = 0;
    return res ? std::string(pcBuffer) : "";
}

std::string GetExecutablePathname()
{
#ifdef _WIN32
    char pcBuffer[1024];
    int i = GetModuleFileName(NULL, pcBuffer, 1023);
    return std::string(pcBuffer);
#else
    return std::string("");
#endif
}

std::string PathedFileName(const std::string& sFName)
{
    std::string sFileName(sFName);
    std::string sPath;

    if (!FileExists(sFileName)) {
        CConfigFile oCF(GetConfigFileName());
        size_t i = 1;
        while (true) {
            if (!oCF.FetchLine("Path", i++))
                break;
            sPath = oCF.GetString(0);

#ifdef _WIN32
            if (!sPath.empty() && sPath[sPath.length() - 1] != '\\' && sPath[sPath.length() - 1] != '/')
                sPath += '\\';
#else
            if (!sPath.empty() && sPath[sPath.length() - 1] != '/')
                sPath += '/';
#endif
            if (FileExists(sPath + sFileName)) {
                sFileName = sPath + sFileName;
                break;
            }
        }
    }
    return sFileName;
}

#ifndef _NEWSPLIT
void split(Args& dest, const std::string& s, const std::string& sep, const std::string& caps, char esc)
{
    std::string::size_type left;
    std::string::size_type right;
    //	std::string::size_type cpos;
    std::string fullsep = sep + caps;
    std::string fullcap;
    char c, cap;

    if (esc)
        fullcap += esc;

    right = 0;

    while (true) {
        left = s.find_first_not_of(sep, right);
        if (left == std::string::npos)
            return;
        else {
            c = s[left];
            if (caps.find(c) != std::string::npos) {
                cap = c;
                fullcap = c;
                if (esc)
                    fullcap += esc;
                left++;
            }
            else {
                cap = 0;
                fullcap = sep;
            }
        }

        right = left;
        do {
            right = s.find_first_of(fullcap, right);
            if (right == std::string::npos) {
                right = s.size();
                c = sep[0];
            }
            else
                c = s[right];

            if (c == esc)
                right = (right + 2 > s.size()) ? s.size() : right + 2;
        } while (c == esc);

        dest.push_back(s.substr(left, right - left));

        if (cap)
            right++;
    }
}
#else
void split(Args& dest, const std::string& s, const std::string& sep, const std::string& caps, char esc)
{
    std::string::size_type left;
    std::string::size_type right;
    //	std::string::size_type cpos;
    std::string fullsep = sep + caps;
    std::string fullcap;
    char c, cap;

    if (esc)
        fullcap += esc;

    right = 0;

    while (true) {
        left = s.find_first_not_of(sep, right);
        if (left == std::string::npos)
            return;
        else {
            int p = left;
            do {
                p = sCom.find_first_of(sep + caps "\t \x27", p);
                if (p == std::string::npos) {
                    p = sCom.size();
                }
                else {
                    if (sCom[p] == '\x27') {
                        do {
                            p = sCom.find_first_of("\\\x27", p + 1);
                            if (p == std::string::npos)
                                p = sCom.size();
                            else {
                                if (sCom[p] == '\\') {
                                    if (p < sCom.size())
                                        p++;
                                }
                                else {
                                    p++;
                                    break;
                                }
                            }
                        } while (p < sCom.size());
                    }
                    else {
                        break;
                    }
                }
            } while (p < sCom.size());

            c = s[left];
            if (caps.find(c) != std::string::npos) {
                cap = c;
                fullcap = c;
                if (esc)
                    fullcap += esc;
                left++;
            }
            else {
                cap = 0;
                fullcap = sep;
            }
        }

        right = left;
        do {
            right = s.find_first_of(fullcap, right);
            if (right == std::string::npos) {
                right = s.size();
                c = sep[0];
            }
            else
                c = s[right];

            if (c == esc)
                right = (right + 2 > s.size()) ? s.size() : right + 2;
        } while (c == esc);

        dest.push_back(s.substr(left, right - left));

        if (cap)
            right++;
    }
}
#endif

std::string g_LastConfigFileName;

CConfigFile::CConfigFile(const std::string& sFName)
    : m_sFName(sFName)
    , m_nLine(0)
{
    std::fstream oIS;
    std::fstream oIS2;
    size_t nLineCount = 0;

    //	if( sFName != g_LastConfigFileName )
    //		fprintf( stderr, "Configfile '%s' benutzt.\n", sFName.c_str() );

    if (!sFName.empty())
        oIS.open(sFName.c_str(), std::ios::in);
    else {
        ERRMSG(0, ("FEHLER: Interner Fehler beim Zugriff auf Konfigurations-Daten!"));
    }

    if (oIS.fail()) {
        if (sFName != g_LastConfigFileName) {
            ERRMSG(0, ("FEHLER: Auf die Datei '%s' kann nicht zugegriffen werden!", sFName.c_str()));
            g_LastConfigFileName = sFName;
        }
        return;
    }

    g_LastConfigFileName = sFName;

    std::string sFName2(sFName);
    CRegExp::RuledReplace(sFName2, "(.*)\\.(.+)rc", "$1.$2-user-rc");
    CRegExp::RuledReplace(sFName2, "(.+)\\.cfg", "$1-user.cfg");
    if (sFName == sFName2) {
        sFName2 = "";
    }

    while (1) {
        getline(oIS, m_sLine);
        if (oIS.fail())
            break;
        nLineCount++;
        while (!m_sLine.empty() && m_sLine[m_sLine.size() - 1] <= 32)
            m_sLine.erase(m_sLine.size() - 1, 1);
        while (!m_sLine.empty() && m_sLine[0] <= 32)
            m_sLine.erase(0, 1);
        if (!m_sLine.empty() && m_sLine[0] != ';') {
            m_csLines.push_back(m_sLine);
            m_cnPos.push_back(nLineCount);
        }
    }

    oIS.close();

    nLineCount = 0;
    if (!sFName2.empty())
        oIS2.open(sFName2.c_str(), std::ios::in);
    else {
        return;
    }

    if (oIS2.fail()) {
        return;
    }

    while (1) {
        getline(oIS2, m_sLine);
        if (oIS2.fail())
            break;
        nLineCount++;
        while (!m_sLine.empty() && m_sLine[m_sLine.size() - 1] <= 32)
            m_sLine.erase(m_sLine.size() - 1, 1);
        while (!m_sLine.empty() && m_sLine[0] <= 32)
            m_sLine.erase(0, 1);
        if (!m_sLine.empty() && m_sLine[0] != ';') {
            m_csLines2.push_back(m_sLine);
            m_cnPos2.push_back(nLineCount);
        }
    }

    m_sFName2 = sFName2;

    oIS2.close();
}

CConfigFile::~CConfigFile() {}

bool CConfigFile::FetchLine(const std::string& sChapter, size_t nIdx, bool bForce)
{
    std::string::size_type p;
    size_t i = 0;
    m_sLine = "";

    if (sChapter == m_sChapter && m_nLine == nIdx)
        return true;
    std::string sChapTitle = std::string("[") + sChapter + std::string("]");

    // optionales Zweit-File befragen
    if (!m_sFName2.empty()) {
        for (i = 0; i < m_csLines2.size(); i++) {
            if (IsEqual(m_csLines2[i].c_str(), sChapTitle.c_str()))
                break;
        }

        if ((i < m_csLines2.size()) && (i + nIdx < m_csLines2.size())) {
            m_sChapter = sChapter;
            m_nLine = 0;
            m_nChapterStart = i;
            m_sLine = m_csLines2[i + nIdx];
        }
    }

    if (m_sLine.empty()) {
        for (i = 0; i < m_csLines.size(); i++) {
            if (IsEqual(m_csLines[i].c_str(), sChapTitle.c_str()))
                break;
        }

        if (i >= m_csLines.size()) {
            static std::string sLastChapter;
            if (bForce && sLastChapter != sChapTitle) {
                if (g_nSrcLine > 0)
                    ERRMSG(0, ("%s(%ld) : FEHLER: Kapitel '%s' in der Config-Datei '%s' nicht gefunden!", g_sSrcFile.c_str(), g_nSrcLine, sChapTitle.c_str(), m_sFName.c_str()));
                else
                    ERRMSG(0, ("FEHLER: Kapitel '%s' in der Config-Datei '%s' nicht gefunden!", sChapTitle.c_str(), m_sFName.c_str()));
                sLastChapter = sChapTitle;
            }
            return false;
        }

        m_sChapter = sChapter;
        m_nLine = 0;
        m_nChapterStart = i;

        if (i + nIdx >= m_csLines.size())
            return false;

        m_sLine = m_csLines[i + nIdx];
    }

    m_csStrings.clear();

    if (m_sLine.empty() || m_sLine[0] == '[')
        return false;

    std::string sTemp;
    p = 0;
    while (true) {
        p = m_sLine.find_first_not_of(" \t", p);
        if (p == std::string::npos)
            break;
        if (m_sLine[p] == '\x22') {
            // String-Eintrag
            std::string::size_type s = p++;
            p = m_sLine.find('\x22', p);
            if (p == std::string::npos) {
                ERRMSG(0, ("%s(%ld) : FEHLER: Fehlendes schliessendes Anfuehrungszeichen!", m_sFName.c_str(), m_cnPos[i + nIdx]));
                break;
            }
            p++;
            m_csStrings.push_back(m_sLine.substr(s, p - s));
        }
        else if (IsDigit(m_sLine[p]) || m_sLine[p] == '-' || m_sLine[p] == '+') {
            // numerischer Eintrag
            std::string::size_type s = p;
            p = m_sLine.find_first_not_of("0123456789.+-", p);
            if (p == std::string::npos) {
                p = m_sLine.length();
            }
            sTemp = m_sLine.substr(s, p - s);
            if (!CRegExp::Match(std::string("#") + sTemp + '#', "#[-+]?((\\d*\\.\\d+)|(\\d+))#")) {
                ERRMSG(0, ("%s(%ld) : FEHLER: Fehlerhaftes Zahlenformat (%s)!", m_sFName.c_str(), m_cnPos[i + nIdx], sTemp.c_str()));
                break;
            }
            m_csStrings.push_back(sTemp);
        }
        else if (!p && IsAlpha(m_sLine[0])) {
            // potentielle Options-Zuweisung
            p = m_sLine.find_first_of("\x22=");
            if (p == std::string::npos || m_sLine[p] != '=') {
                ERRMSG(0, ("%s(%ld) : FEHLER: Fehlendes oeffnendes Anfuehrungszeichen!", m_sFName.c_str(), m_cnPos[i + nIdx]));
                break;
            }
            m_csStrings.push_back(m_sLine);
            break;
        }
        else {
            ERRMSG(0, ("%s(%ld) : FEHLER: Unerwartetes Zeichen an Offset %lu!", m_sFName.c_str(), m_cnPos[i + nIdx], p));
            break;
        }
        p = m_sLine.find_first_not_of(" \t", p);
        if (p == std::string::npos)
            break;
        if (m_sLine[p] != ',') {
            ERRMSG(0, ("%s(%ld) : FEHLER: Fehlendes Komma zur Trennung der Eintraege!", m_sFName.c_str(), m_cnPos[i + nIdx]));
            break;
        }
        p++;
    }

    /*
            m_cnStart.clear();
            m_cnStart.push_back( 0 );

        CRegExp oRE;
        p = 0;
        oRE.Prepare( "([^ \\t\\n\\r\\f,\"]+|(\"([^\"\\\\]|\\\\.)*\"))[ \t]*," );
        while( oRE.Find( m_sLine, p ) )
        {
            m_cnStart.push_back( oRE.End() );
            p = oRE.End();
        }
            m_cnStart.push_back( m_sLine.size()+1 );
    */

    /*
        p = 0;
            while( true )
            {
                    p = m_sLine.find( ',', p );
                    if( p == std::string::npos )
                            break;
                    p++;
                    m_cnStart.push_back( p );
            }
            m_cnStart.push_back( m_sLine.size()+1 );
    */
    m_nLine = nIdx;

    return (m_sLine[0] == '[') || m_csStrings.empty() ? false : true;
}

bool CConfigFile::IsString(size_t nIdx)
{
    if (nIdx >= m_csStrings.size()) {
        return false;
    }
    if (m_csStrings[nIdx][0] == '\x22') {
        return true;
    }
    return false;
}

std::string CConfigFile::GetString(size_t nIdx)
{
    if (size_t(nIdx) >= m_csStrings.size()) {
        return std::string("");
    }
    if (IsString(nIdx)) {
        return m_csStrings[nIdx].substr(1, m_csStrings[nIdx].length() - 2);
    }
    else {
        return m_csStrings[nIdx];
    }
}

int32_t CConfigFile::GetLong(size_t nIdx)
{
    if (size_t(nIdx) >= m_csStrings.size()) {
        return 0;
    }
    return std::atoi(m_csStrings[nIdx].c_str());
}

double CConfigFile::GetReal(size_t nIdx)
{
    if (size_t(nIdx) >= m_csStrings.size()) {
        return 0;
    }
    return std::atof(m_csStrings[nIdx].c_str());
}

COutputTable& COutputTable::Col(const std::string& sText, COutputTable::FORMAT enFormat)
{
    m_nCol++;
    if (m_nCol > m_nMaxCols) {
        m_nMaxCols = m_nCol;
    }
    m_pRow->push_back(TABENTRY(enFormat, sText));
    return *this;
}

COutputTable& COutputTable::Col(const char* pcText, COutputTable::FORMAT enFormat)
{
    m_nCol++;
    if (m_nCol > m_nMaxCols) {
        m_nMaxCols = m_nCol;
    }
    m_pRow->push_back(TABENTRY(enFormat, std::string(pcText)));
    return *this;
}

COutputTable& COutputTable::Col(int32_t nNum, COutputTable::FORMAT enFormat)
{
    m_nCol++;
    if (m_nCol > m_nMaxCols) {
        m_nMaxCols = m_nCol;
    }
    m_pRow->push_back(TABENTRY(enFormat, std::to_string(nNum)));
    return *this;
}

COutputTable& COutputTable::Col(double fNum, int nScale, COutputTable::FORMAT enFormat)
{
    char Fmt[16];
    char Buff[80];
    m_nCol++;
    if (m_nCol > m_nMaxCols) {
        m_nMaxCols = m_nCol;
    }
    snprintf(Fmt, sizeof(Fmt), "%%.%df", nScale < 10 ? nScale : 10);
    snprintf(Buff, sizeof(Buff), Fmt, fNum);
    m_pRow->push_back(TABENTRY(enFormat, std::string(Buff)));
    return *this;
}

void COutputTable::Format()
{
    const std::string sSPC("                                                            ");
    TABLE::iterator iT;
    size_t nWidth;

    if (m_coTable.back().empty()) {
        m_coTable.pop_back();
    }
    for (size_t c = 0; c < m_nMaxCols; c++) {
        // Spaltenbreite ermitteln
        nWidth = 0;
        for (iT = m_coTable.begin(); iT != m_coTable.end(); iT++) {
            if ((*iT).size() <= c) {
                (*iT).push_back(TABENTRY(enLEFT, std::string("")));
            }
            if ((*iT)[c].second.size() > nWidth) {
                nWidth = (*iT)[c].second.size();
            }
        }

        if (nWidth > 60) {
            break;
        }

        // Spalten formatieren
        for (iT = m_coTable.begin(); iT != m_coTable.end(); iT++) {
            if ((*iT)[c].second.size() < nWidth) {
                switch ((*iT)[c].first) {
                    case enRIGHT:
                        (*iT)[c].second = sSPC.substr(0, nWidth - (*iT)[c].second.size()) + (*iT)[c].second;
                        break;
                    case enCENTER:
                        (*iT)[c].second = sSPC.substr(0, (nWidth - (*iT)[c].second.size()) / 2) + (*iT)[c].second;
                        break;
                    case enLEFT:
                        (*iT)[c].second += sSPC.substr(0, nWidth - (*iT)[c].second.size());
                        break;
                }
            }
        }
    }
}

void COutputTable::Output(const std::string& sTarget, const std::string& sPfx)
{
    TABLE::iterator iT;
    bool bBorder;
    Format();

    for (iT = m_coTable.begin(); iT != m_coTable.end(); iT++) {
        COutput::TPrintf(sTarget, "%s", sPfx.c_str());
        bBorder = false;
        for (size_t c = 0; c < (*iT).size(); c++) {
            if (!strcmp((*iT)[c].second.c_str(), "|")) {
                COutput::TPrintf(sTarget, "|");
                bBorder = true;
            }
            else {
                if (c && !bBorder) {
                    COutput::TPrintf(sTarget, " %s", (*iT)[c].second.c_str());
                }
                else {
                    COutput::TPrintf(sTarget, "%s", (*iT)[c].second.c_str());
                }
                bBorder = false;
            }
        }
        COutput::TPrintf(sTarget, "\n");
    }
}

void COutputTable::Output(CValArray& oDump, const std::string& sPfx)
{
    TABLE::iterator iT;
    bool bBorder;
    Format();
    std::string sLine;

    for (iT = m_coTable.begin(); iT != m_coTable.end(); iT++) {
        sLine = sPfx;
        bBorder = false;
        for (size_t c = 0; c < (*iT).size(); c++) {
            if (!strcmp((*iT)[c].second.c_str(), "|")) {
                sLine += "|";
                bBorder = true;
            }
            else {
                if (c && !bBorder) {
                    sLine += " ";
                }
                sLine += (*iT)[c].second;
                bBorder = false;
            }
        }
        oDump.push_back(sLine);
    }
}

void COutputTable::Output(FILE* pStream)
{
    TABLE::iterator iT;
    bool bBorder;
    Format();
    std::string sLine;

    for (iT = m_coTable.begin(); iT != m_coTable.end(); iT++) {
        sLine = "";
        bBorder = false;
        for (size_t c = 0; c < (*iT).size(); c++) {
            if (!strcmp((*iT)[c].second.c_str(), "|")) {
                sLine += "|";
                bBorder = true;
            }
            else {
                if (c && !bBorder) {
                    sLine += " ";
                }
                sLine += (*iT)[c].second;
                bBorder = false;
            }
        }
        fprintf(pStream, "%s\n", sLine.c_str());
    }
}

std::string Flatten(const std::string& sText)
{
    std::string::const_iterator is;
    std::string sOut;
    static char Buff[256];
    char* str = Buff;
    int l = 0;

    is = sText.begin();
    while (is != sText.end()) {
        switch (static_cast<unsigned char>(*is)) {
            case 196:
                *str++ = 'a';
                *str++ = 'e';
                break;
            case 214:
                *str++ = 'o';
                *str++ = 'e';
                break;
            case 220:
                *str++ = 'u';
                *str++ = 'e';
                break;
            case 228:
                *str++ = 'a';
                *str++ = 'e';
                break;
            case 246:
                *str++ = 'o';
                *str++ = 'e';
                break;
            case 252:
                *str++ = 'u';
                *str++ = 'e';
                break;
            case 223:
                *str++ = 's';
                *str++ = 's';
                break;
#ifdef _WIN32
            case 132:  // DOS-�
            case 142:  // DOS-�
                *str++ = 'a';
                *str++ = 'e';
                break;
            case 148:  // DOS-�
            case 153:  // DOS-�
                *str++ = 'o';
                *str++ = 'e';
                break;
            case 129:  // DOS-�
            case 154:  // DOS-�
                *str++ = 'u';
                *str++ = 'e';
                break;
            case 225:  // DOS-�
                *str++ = 's';
                *str++ = 's';
                break;
#endif
            case '~':
            case ' ':
            case '\t':
                break;
            default:
                *str++ = static_cast<char>(tolower(*is));
        }
        is++;
        if (++l > 250) {
            *str = 0;
            sOut += Buff;
            str = Buff;
            l = 0;
        }
    }
    *str = 0;
    return std::string(Buff);
}

static void EscapeStringContent(std::string& sTxt)
{
    CRegExp::Replace(sTxt, "\"", "");
    CRegExp::Replace(sTxt, "[\\s]+", "~");
}

std::string Escape(const std::string& sText, bool bTildenize)
{
    std::string sErg(sText);
    std::string sEsc("\\");
    std::string::size_type p = 0;

    if (bTildenize) {
        CRegExp::ReplaceCall(sErg, "\"[^\"]*[\"]?", EscapeStringContent);
    }
    else {
        while (true) {
            p = sErg.find_first_of("\\\"", p);
            if (p == std::string::npos) {
                break;
            }
            sErg.insert(p, sEsc);
            p += 2;
        }
    }
    return sErg;
}

std::string DeEscape(const std::string& sText)
{
    std::string sErg(sText);
    std::string::size_type p = 0;

    while (true) {
        p = sErg.find('\\', p);
        if (p == std::string::npos) {
            break;
        }
        sErg.erase(p, 1);
        p++;
    }
    return sErg;
}

bool IsIdentifier(const std::string& sText, bool bVar)
{
    std::string::const_iterator i = sText.begin();

    if (bVar && i != sText.end() && *i++ != '$') {
        return false;
    }

    if (i != sText.end() && (!(IsAlpha(*i) || IsUmlaut(*i)) || *i == '@')) {
        return false;
    }
    i++;

    while (i != sText.end()) {
        if (*i == '@' || !(IsAlpha(*i) || IsDigit(*i) || IsUmlaut(*i))) {
            return false;
        }
        i++;
    }
    return true;
}

/*
int32_t CStringDB::m_nStrCnt = 0;
std::map<std::string,int32_t> CStringDB::m_coStrToSID;
std::map<int32_t,std::string> CStringDB::m_coSIDToStr;

int32_t CStringDB::Insert( const std::string& sText )
{
        std::string sDText = DeUmlaut( sText );
        std::map<std::string,int32_t>::iterator i = m_coStrToSID.find( sDText );
        if( i == m_coStrToSID.end() )
        {
                m_coStrToSID.insert( std::map<std::string,int32_t>::value_type( sDText, ++m_nStrCnt ) );
                m_coSIDToStr.insert( std::map<int32_t,std::string>::value_type( m_nStrCnt, sText ) );
                return m_sStrCnt;
        }
        else
        {
                return (*i).second;
        }
}

const std::string& CStringDB::Get( int32_t nSID )
{
        static const std::string sUnknown( "-unbekannt-" );
        std::map<int32_t,std::string>::iterator i = m_coSIDToStr.find( nSID );
        if( i == m_coSIDToStr.end() )
        {
                return sUnknown;
        }
        else
        {
                return (*i).second;
        }
}
*/

std::map<std::string, size_t> CStringDB::m_coS2I;
std::map<size_t, std::string> CStringDB::m_coI2S;

int32_t CStringDB::Str2SID(const std::string& sStr)
{
    std::string sFlat = Flatten(sStr);
    auto i = m_coS2I.find(sFlat);
    if (i == m_coS2I.end()) {
        size_t s = m_coS2I.size() + 1;
        m_coS2I[sFlat] = s;
        m_coI2S[s] = sStr;
        return static_cast<int32_t>(s);
    }
    else {
        return static_cast<int32_t>((*i).second);
    }
}

const std::string& CStringDB::SID2Str(int32_t nSID)
{
    static std::string sUnknown("unknown sid");
    auto i = m_coI2S.find(static_cast<size_t>(nSID));
    if (i == m_coI2S.end()) {
        return sUnknown;
    }
    else {
        return (*i).second;
    }
}

std::map<std::string, std::string> CTranslationDB::m_coI2L;
std::map<std::string, std::string> CTranslationDB::m_coL2I;

std::map<std::string, std::string> COutput::g_csFilter;
std::map<std::string, COutput*> COutput::g_cpoTargets;
std::map<std::string, COutput*> COutput::g_cpoDestinations;

COutput::COutput(const std::string& sFileName, bool bFlushed)
    : m_bIsOkay(false)
    , m_bStdStream(false)
    , m_bFlushed(bFlushed)
    , m_nRefCount(1)
    , m_sFileName(sFileName)
    , m_hFile(0)
    , m_poRoute(0)
{
    std::map<std::string, COutput*>::const_iterator di = g_cpoDestinations.find(sFileName);
    if (di == g_cpoDestinations.end()) {
        m_hFile = fopen(sFileName.c_str(), "w");
        if (m_hFile) {
            m_bIsOkay = true;
        }
        g_cpoDestinations[sFileName] = this;

        if (m_bFlushed && m_hFile) {
            fclose(m_hFile);
            m_hFile = 0;
        }
    }
    else {
        (*di).second->m_nRefCount++;
        m_poRoute = (*di).second;
        m_bIsOkay = true;
    }
}

COutput::COutput(FILE* hFile)
    : m_bIsOkay(false)
    , m_bStdStream(true)
    , m_bFlushed(false)
    , m_nRefCount(1)
    , m_hFile(hFile)
    , m_poRoute(0)
{
    if (!g_pOutputMapper.get()) {
        g_pOutputMapper.reset(new CharacterMapper("iso-8859-1", "iso-8859-1"));
    }

    if (m_hFile) {
        m_bIsOkay = true;
    }
}

COutput::~COutput()
{
    if (m_nRefCount > 1) {
        fprintf(stderr, "FEHLER: Freigabe eines benutzen Ausgabekanals! (interner Fehler)\n");
    }

    Disconnect(false);
}

bool COutput::Disconnect(bool bSuicide)
{
    if (m_poRoute) {
        m_poRoute->Disconnect();
    }

    m_nRefCount--;
    if (!m_nRefCount) {
        if (!m_poRoute) {
            if (!m_bStdStream) {
                g_cpoDestinations.erase(m_sFileName);
                if (m_hFile) {
                    fclose(m_hFile);
                }
                m_hFile = 0;
            }
        }
        m_poRoute = 0;
        m_bIsOkay = false;
        if (bSuicide) {
            delete this;
        }
        return true;
    }
    return false;
}

void COutput::DoWrite(const char* pcTxt)
{
    if (m_poRoute) {
        m_poRoute->DoWrite(pcTxt);
        return;
    }

    std::string sOut;
    if (g_pOutputMapper->IsOK()) {
        if (m_bWritePrefix && g_pOutputMapper->IsToUtf8()) {
            sOut = std::string("\xEF\xBB\xBF", 3) + g_pOutputMapper->Map(pcTxt);
            m_bWritePrefix = false;
        }
        else {
            sOut = g_pOutputMapper->Map(pcTxt);
        }
        pcTxt = sOut.c_str();
    }

    if (m_bStdStream) {
        // static bool bEOL = true;
        size_t l = strlen(pcTxt);
        if ((g_bForceEOL && strstr(pcTxt, "\n")) /*|| ( !bEOL && strchr( pcTxt, ':' ) )*/) {
            WriteToConsole(m_hFile, "\n");
            g_bForceEOL = false;
        }
        WriteToConsole(m_hFile, pcTxt);
        /*
        if( l && ( pcTxt[l-1]==10 || pcTxt[l-1]==13 ) ) {
            bEOL = true;
        }
        else {
            bEOL = false;
        }
        */
    }
    else if (m_bFlushed) {
        FILE* hFile;
        hFile = fopen(m_sFileName.c_str(), "a");
        if (hFile) {
            fprintf(hFile, "%s", pcTxt);
            fclose(hFile);
        }
        else {
            fprintf(stderr, "%s", pcTxt);
        }
    }
    else if (m_hFile) {
        fprintf(m_hFile, "%s", pcTxt);
    }
    else {
        fprintf(stderr, "%s", pcTxt);
    }
}

extern bool DoUserFunction(const std::string& sName, ArgumentList& coArgs, Value* poVal);

void COutput::FilterWrite(const char* pcTxt)
{
    std::map<std::string, std::string>::iterator i = g_csFilter.find(m_sTargetName);
    if (i != g_csFilter.end()) {
        ArgumentList coArgs;
        Value oVal;
        coArgs.push_back(Value(pcTxt));
        if (DoUserFunction((*i).second, coArgs, &oVal)) {
            DoWrite(oVal.asString().c_str());
        }
        else {
            // error
            if (oVal.getType() == VT_ERROR) {
                ERRMSG(0, ("FEHLER: Fehler in Ausgabefilter '%s', Filter wird abgeschaltet: %s", (*i).second.c_str(), oVal.asString().c_str()));
            }
            COutput::SetFilter((*i).first.c_str(), "");
            DoWrite(pcTxt);
        }
    }
    else {
        DoWrite(pcTxt);
    }
}

void COutput::Write(const char* pcTxt)
{
    std::map<std::string, std::string>::iterator i = g_csFilter.find(m_sTargetName);
    if (i == g_csFilter.end()) {
        DoWrite(pcTxt);
        return;
    }

    std::string sTxt = pcTxt;
    size_t p;

    while (true) {
        p = sTxt.find_first_of("\r\n");
        if (p == std::string::npos) {
            break;
        }
        m_sOutBuffer += sTxt.substr(0, p + 1);
        FilterWrite(m_sOutBuffer.c_str());
        sTxt.erase(0, p + 1);
        m_sOutBuffer = "";
    }
    m_sOutBuffer += sTxt;
}

void COutput::Write(const std::string& sTxt)
{
    Write(sTxt.c_str());
}

void COutput::Printf(const char* msg, ...)
{
    static char pcBuff[4096];
    va_list list;
    va_start(list, msg);
    _vsnprintf(pcBuff, 4095, msg, list);
    pcBuff[4095] = 0;
    Write(pcBuff);
}

void COutput::TWrite(const std::string& sID, const std::string& sTxt)
{
    Target(sID)->Write(sTxt.c_str());
}

void COutput::TPrintf(const std::string& sID, const char* msg, ...)
{
    static char pcBuff[4096];
    va_list list;
    va_start(list, msg);
    _vsnprintf(pcBuff, 4095, msg, list);
    pcBuff[4095] = 0;
    Target(sID)->Write(pcBuff);
}

void COutput::CloseTargets()
{
    std::map<std::string, COutput*>::iterator ti = g_cpoTargets.begin();
    std::list<COutput*> cpoHelp;

    while (ti != g_cpoTargets.end()) {
        if ((*ti).second->m_poRoute) {
            delete (*ti++).second;
        }
        else {
            cpoHelp.push_back((*ti++).second);
        }
    }
    g_cpoTargets.clear();
    std::list<COutput*>::iterator hi = cpoHelp.begin();
    while (hi != cpoHelp.end()) {
        delete (*hi++);
    }
    cpoHelp.clear();
    g_cpoDestinations.clear();
}

void COutput::SetTarget(const std::string& sID, COutput* poTarget)
{
    std::map<std::string, COutput*>::iterator ti = g_cpoTargets.find(sID);
    poTarget->m_sTargetName = sID;

    if (ti == g_cpoTargets.end()) {
        g_cpoTargets.insert(std::map<std::string, COutput*>::value_type(sID, poTarget));
    }
    else {
        if ((*ti).second->m_nRefCount <= 1) {
            delete (*ti).second;
        }
        (*ti).second = poTarget;
    }
}

void COutput::SetFilter(const std::string& sID, const std::string& sFilter)
{
    if (sFilter.empty()) {
        g_csFilter.erase(sID);
    }
    else {
        g_csFilter[sID] = sFilter;
    }
}

void COutput::RenameTarget(const std::string& sIDOld, const std::string& sIDNew)
{
    std::map<std::string, COutput*>::iterator ti = g_cpoTargets.find(sIDOld);
    COutput* poTarget;
    if (ti != g_cpoTargets.end()) {
        poTarget = (*ti).second;
        g_cpoTargets.erase(sIDOld);
        SetTarget(sIDNew, poTarget);
    }
}

COutput* COutput::Target(const std::string& sID)
{
    std::map<std::string, COutput*>::const_iterator ti = g_cpoTargets.find(sID);
    if (ti == g_cpoTargets.end()) {
        SetTarget(sID, new COutput(stderr));
        ERRMSG(0, ("FEHLER: Der interne Ausgabekanal '%s' wurde nicht gefunden!", sID.c_str()));
        return Target(sID);
    }
    return (*ti).second;
}

void COutput::SetEncoding(const std::string& sEnc)
{
    if (IsEqual(sEnc, "utf-8") || IsEqual(sEnc, "utf8")) {
        g_bUTF8 = true;
        g_pOutputMapper.reset(new CharacterMapper("iso-8859-1", "utf8"));
    }
    else if (CharacterMapper::IsSupported(sEnc)) {
        g_bUTF8 = false;
        g_pOutputMapper.reset(new CharacterMapper("iso-8859-1", "iso-8859-1"));
    }
}
