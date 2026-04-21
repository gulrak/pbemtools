//---------------------------------------------------------------------------------------
// regexp.h
//---------------------------------------------------------------------------------------
//
// Copyright (c) 2004, Steffen Schümann <s.schuemann@pobox.com>
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
#include "regexp.h"

#include "iso8859-1.c"

const unsigned char* CRegExp::m_pCharTables = pcre_ISO_8859_1_tables;

CRegExp::CRegExp(const std::string& sRE, int nMode)
    : m_pRE(NULL)
    , m_pMatchData(NULL)
    , m_nCount(0)
    , m_pOffsets(NULL)
{
    if (!sRE.empty())
        Prepare(sRE, nMode);
}

CRegExp::~CRegExp()
{
    if (m_pMatchData)
        pcre2_match_data_free(m_pMatchData);
    if (m_pRE)
        pcre2_code_free(m_pRE);
}

bool CRegExp::Prepare(const std::string& sRE, int nMode)
{
    int errorcode;
    PCRE2_SIZE erroroffset;

    m_nCount = 0;

    if (m_sRE == sRE)
        return true;

    m_sLastError = "";

    if (m_pMatchData) {
        pcre2_match_data_free(m_pMatchData);
        m_pMatchData = NULL;
    }
    if (m_pRE) {
        pcre2_code_free(m_pRE);
        m_pRE = NULL;
    }

    pcre2_compile_context* ccontext = pcre2_compile_context_create(NULL);
    if (m_pCharTables)
        pcre2_set_character_tables(ccontext, m_pCharTables);

    m_pRE = pcre2_compile((PCRE2_SPTR)sRE.c_str(), PCRE2_ZERO_TERMINATED, (uint32_t)nMode, &errorcode, &erroroffset, ccontext);
    pcre2_compile_context_free(ccontext);

    if (!m_pRE) {
        PCRE2_UCHAR buffer[256];
        pcre2_get_error_message(errorcode, buffer, sizeof(buffer));
        m_sLastError = (const char*)buffer;
        return false;
    }

    m_pMatchData = pcre2_match_data_create_from_pattern(m_pRE, NULL);
    m_pOffsets = pcre2_get_ovector_pointer(m_pMatchData);

    m_sRE = sRE;

    return true;
}

bool CRegExp::Find(const std::string& sText, int nPos)
{
    m_nCount = pcre2_match(m_pRE, (PCRE2_SPTR)sText.c_str(), sText.size(), nPos, PCRE2_NOTEMPTY, m_pMatchData, NULL);
    return m_nCount > 0;
}

void CRegExp::UseLocale()
{
    m_pCharTables = pcre2_maketables(NULL);
}

bool CRegExp::Match(const std::string& sText, const std::string& sRE)
{
    CRegExp oRE;

    if (oRE.Prepare(sRE)) {
        return oRE.Find(sText, 0);
    }
    return false;
}

bool CRegExp::Replace(std::string& sText, const std::string& sRE, const std::string& sRep)
{
    CRegExp oRE;
    bool bLoop = true;
    int pos = 0;

    if (oRE.Prepare(sRE)) {
        do {
            if (oRE.Find(sText, pos)) {
                sText.replace(size_t(oRE.Begin()), size_t(oRE.Size()), sRep);
                pos = oRE.Begin() + int(sRep.size());
            }
            else
                bLoop = false;
        } while (bLoop);
        return true;
    }
    else
        return false;
}

bool CRegExp::Replace(std::string& sText, const std::vector<std::string>& coRE, const std::vector<std::string>& coRep)
{
    if (coRE.size() != coRep.size())
        return false;
    for (size_t i = 0; i < coRE.size(); i++)
        if (!Replace(sText, coRE[i], coRep[i]))
            return false;
    return true;
}

bool CRegExp::Replace(std::string& sText, const char** ppcRE, const char** ppcRep)
{
    while (*ppcRE && *ppcRep)
        if (!Replace(sText, std::string(*ppcRE++), std::string(*ppcRep++)))
            return false;
    return true;
}

bool CRegExp::ReplaceCall(std::string& sText, const std::string& sRE, ModifyFunction pfMod)
{
    std::string sRep;
    CRegExp oRE;
    bool bLoop = true;
    int pos = 0;

    if (oRE.Prepare(sRE)) {
        do {
            if (oRE.Find(sText, pos)) {
                sRep = sText.substr(size_t(oRE.Begin()), size_t(oRE.Size()));
                pfMod(sRep);
                sText.replace(size_t(oRE.Begin()), size_t(oRE.Size()), sRep);
                pos = oRE.Begin() + int(sRep.size());
            }
            else
                bLoop = false;
        } while (bLoop);
        return true;
    }
    else
        return false;
}

bool CRegExp::ReplaceCall(std::string& sText, const std::string& sRE, const std::string& sRep, ModifyFunction2 pfMod)
{
    std::string sRepStr;
    CRegExp oRE;
    bool bLoop = true;
    int pos = 0;

    if (oRE.Prepare(sRE)) {
        do {
            if (oRE.Find(sText, pos)) {
                sRepStr = sText.substr(size_t(oRE.Begin()), size_t(oRE.Size()));
                pfMod(sRep, sRepStr);
                sText.replace(size_t(oRE.Begin()), size_t(oRE.Size()), sRepStr);
                pos = oRE.Begin() + int(sRepStr.size());
            }
            else
                bLoop = false;
        } while (bLoop);
        return true;
    }
    else
        return false;
}

bool CRegExp::RuledReplace(std::string& sText, const std::string& sRE, const std::string& sRep)
{
    typedef std::pair<int, int> PatternPosition;
    typedef std::list<PatternPosition> Patterns;
    std::string sRepStr;
    Patterns coPattPos;
    CRegExp oRE;
    bool bLoop = true;
    int pos = 0;

    // TODO: Verify accent acute solution!
    if (oRE.Prepare(R"((?!\\)\$([+&`']|[\d]+))")) {
        do {
            if (oRE.Find(sRep, pos)) {
                coPattPos.push_back(PatternPosition(oRE.Begin(), oRE.Size()));
                pos = oRE.Begin() + oRE.Size();
            }
            else
                bLoop = false;
        } while (bLoop);
    }
    else
        return false;

    bLoop = true;
    pos = 0;
    if (oRE.Prepare(sRE)) {
        do {
            if (oRE.Find(sText, pos)) {
                sRepStr = sRep;
                std::string sR;
                for (Patterns::const_iterator pi = coPattPos.begin(); pi != coPattPos.end(); pi++) {
                    switch (sRep[size_t((*pi).first + 1)]) {
                        case '0':
                        case '&':
                            sR = oRE.SubStr(sText);
                            break;
                        case '`':
                            sR = sText.substr(0, size_t(oRE.Begin()));
                            break;
                        case '\'':
                            sR = sText.substr(size_t(oRE.Begin() + oRE.Size()));
                            break;
                        case '+':
                            sR = oRE.SubStr(sText, oRE.Count() - 1);
                            break;
                        default:
                            int idx = atoi(sRep.c_str() + (*pi).first + 1);
                            sR = oRE.SubStr(sText, idx);
                    }
                    sRepStr.replace(size_t((*pi).first), size_t((*pi).second), sR);
                }
                sText.replace(size_t(oRE.Begin()), size_t(oRE.Size()), sRepStr);
                pos = oRE.Begin() + int(sRepStr.size());
            }
            else
                bLoop = false;
        } while (bLoop);
        return true;
    }
    else
        return false;
}

bool CRegExp::RuledReplace(std::string& sText, const std::vector<std::string>& coRE, const std::vector<std::string>& coRep)
{
    if (coRE.size() != coRep.size())
        return false;
    for (size_t i = 0; i < coRE.size(); i++)
        if (!RuledReplace(sText, coRE[i], coRep[i]))
            return false;
    return true;
}

bool CRegExp::RuledReplace(std::string& sText, const char** ppcRE, const char** ppcRep)
{
    while (*ppcRE && *ppcRep)
        if (!RuledReplace(sText, std::string(*ppcRE++), std::string(*ppcRep++)))
            return false;
    return true;
}
