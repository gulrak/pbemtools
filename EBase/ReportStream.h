/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/EBase/ReportStream.h,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:55:53 $
 * $Revision: 1.2 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: ERESSEA-CR-Stream-IO, eine Hilfsklasse zum CR-Parsen
 *****************************************************************************
 * $Log: ReportStream.h,v $
 * Revision 1.2  2000/02/24 09:55:53  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/
#pragma once

#include <cstdio>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "Value.h"

class CReportStream
{
public:
    enum TOKEN { enBLOCK, enINTEGER, enSTRING, enCOMMENT, enERROR };

    CReportStream(const std::string& sFName, bool bRead = true);
    ~CReportStream();

    bool EOS() const { return m_bEOS; }

    bool Next()
    {
        if (m_bUnread) {
            m_bUnread = false;
        }
        else {
            PrepareLine();
        }
        return !m_bEOS;
    }

    bool Unget()
    {
        if (m_bUnread)
            return false;
        m_bUnread = true;
        return true;
    }

    int32_t GetLine() { return m_nLineNumber; }

    TOKEN GetType() const { return m_enType; }

    bool IsBlock() const { return m_enType == enBLOCK; }

    const std::string& GetValue() const { return m_sValue; }

    const std::string& GetComment() const { return m_sComment; }

    const std::vector<int32_t>& Data() const { return m_coVals; }

    int32_t GetNumDat() const { return int32_t(m_coVals.size()); }

    int32_t GetDat(int nIdx) const { return nIdx < GetNumDat() ? m_coVals[size_t(nIdx)] : 0; }

    void WriteBlock(const std::string sValue, const std::string sComment = "", int32_t nDat1 = 0x7fffffffL, int32_t nDat2 = 0x7fffffffL, int32_t nDat3 = 0x7fffffffL);
    void WriteLine(const std::string sValue, const std::string sComment = "");
    void WriteLine(int32_t nValue, const std::string sComment = "");
    void WriteLine(int32_t nValue1, int32_t nValue2, const std::string sComment = "");

    void Utf8Mode(bool utf8) { m_bUTF8 = utf8; }

protected:
    bool GetLine(std::string& sLine);
    void PrepareLine();

protected:
    bool m_bEOS;
    bool m_bRead;
    bool m_bUnread;
    bool m_bUTF8;
    std::fstream m_oIS;
    FILE* m_hFile;
    int32_t m_nLineNumber;
    std::string m_sLine;
    std::string m_sValue;
    std::vector<int32_t> m_coVals;
    std::vector<char> m_coBuffer;
    char* m_pcBufferPos;
    char* m_pcBufferFill;
    TOKEN m_enType;
    std::string m_sComment;
};
