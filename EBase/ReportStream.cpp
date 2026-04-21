/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/EBase/ReportStream.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:55:53 $
 * $Revision: 1.5 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: ERESSEA-CR-Stream-IO, eine Hilfsklasse zum CR-Parsen
 *****************************************************************************
 *
 * $Log: ReportStream.cpp,v $
 * Revision 1.5  2000/02/24 09:55:53  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.4  1999/10/28 12:39:26  S.Schuemann
 * - Änderungen für den Linux-Port
 *
 * Revision 1.3  1999/10/20 02:22:33  S.Schuemann
 * - Anpassungen der Reportklassen fuer die Features von Vorlage 1.4 b 3
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

//#include "StdAfx.h"
//#pragma hdrstop

#include "ReportStream.h"
#include <cstring>
#include "Utility.h"

using namespace std;

#define RSUSEIOSTREAM

/////////////////////////////////////////////////////////////////////
//.class: CReportStream
/////////////////////////////////////////////////////////////////////

CReportStream::CReportStream(const std::string& sFName, bool bRead)
    : m_bEOS(false)
    , m_bRead(bRead)
    , m_bUnread(false)
    , m_bUTF8(false)
    , m_nLineNumber(0)
    , m_pcBufferPos(0)
    , m_pcBufferFill(0)
{
    m_coBuffer.resize(32768);
    m_pcBufferPos = &m_coBuffer[0];
    m_pcBufferFill = &m_coBuffer[0];
#ifdef RSUSEIOSTREAM
    m_oIS.open(sFName.c_str(), bRead ? ios::in : ios::out);
    if (m_oIS.fail()) {
        m_bEOS = true;
        //		Message( "Auf die Datei kann nicht zugegriffen werden!" );
    }
#else
    m_hFile = fopen(sFName.c_str(), "r+b");
    if (!m_hFile) {
        m_bEOS = true;
    }
#endif
    //	if( bRead )
    //		PrepareLine();
}

CReportStream::~CReportStream()
{
#ifdef RSUSEIOSTREAM
    m_oIS.close();
#else
    fclose(m_hFile);
#endif
}

bool CReportStream::GetLine(std::string& sLine)
{
    char* pcStart = m_pcBufferPos;
    while (true) {
        while (m_pcBufferPos < m_pcBufferFill && (unsigned char)*m_pcBufferPos && (unsigned char)*m_pcBufferPos != 10 && (unsigned char)*m_pcBufferPos != 13) {
            m_pcBufferPos++;
        }
        if (m_pcBufferPos >= m_pcBufferFill) {
#ifdef RSUSEIOSTREAM
            if (m_oIS.fail())
#else
            if (feof(m_hFile) || ferror(m_hFile))
#endif
            {
                if (m_pcBufferPos == &m_coBuffer[0])
                    return false;
                sLine.assign(pcStart, size_t(m_pcBufferPos - pcStart));
                m_pcBufferPos = &m_coBuffer[0];
                m_pcBufferFill = &m_coBuffer[0];
                return true;
            }
            // umkopieren
            char* pcPos = pcStart;
            pcStart = &m_coBuffer[0];
            std::memcpy(pcStart, pcPos, size_t(m_pcBufferPos - pcPos));
            m_pcBufferPos -= pcPos - pcStart;
            m_pcBufferFill -= pcPos - pcStart;
            if (m_coBuffer.size() + pcStart - m_pcBufferFill == 0) {
                size_t nPos = size_t(m_pcBufferPos - pcStart);
                size_t nFill = size_t(m_pcBufferFill - pcStart);
                m_coBuffer.resize(m_coBuffer.size() + 32768);
                pcStart = &m_coBuffer[0];
                m_pcBufferPos = pcStart + nPos;
                m_pcBufferFill = pcStart + nFill;
            }
#ifdef RSUSEIOSTREAM
            m_oIS.read(m_pcBufferFill, m_coBuffer.size() + pcStart - m_pcBufferFill);
            m_pcBufferFill += m_oIS.gcount();
#else
            m_pcBufferFill += fread(m_pcBufferFill, 1, m_coBuffer.size() + pcStart - m_pcBufferFill, m_hFile);
#endif
        }
        else {
            sLine.assign(pcStart, size_t(m_pcBufferPos - pcStart));
            while (m_pcBufferPos < m_pcBufferFill && (unsigned char)*m_pcBufferPos < 32)
                m_pcBufferPos++;
            // pcStart=m_pcBufferPos;
            return true;
        }
    }
}

void CReportStream::PrepareLine()
{
    char c;
    // int i;
    size_t p = 0;

    m_sValue = "";
    m_sComment = "";
    m_enType = enERROR;
    m_coVals.clear();

    if (m_bEOS)
        return;

    do {
        /*
                        getline( m_oIS, m_sLine );
                        if( m_oIS.fail() )
                        {
                                m_bEOS = true;
                                return;
                        }
        */
        if (!GetLine(m_sLine)) {
            m_bEOS = true;
            return;
        }
        else {
            // TODO: Real UTF8-Handling
            if (!m_nLineNumber) {
                // ggf. BOM l�schen
                if (m_sLine.substr(0, 3) == "\xEF\xBB\xBF")
                    m_sLine.erase(0, 3);
            }
            if (m_bUTF8)
                Utf8toIso885915(m_sLine);
        }

        m_nLineNumber++;

        //		while( !m_sLine.empty() && m_sLine[m_sLine.size()-1]<32 && m_sLine[m_sLine.size()-1]>0 )
        //			m_sLine.erase( m_sLine.size()-1, 1 );
    } while (m_sLine.empty());

    // i = 0;
    if (!m_sLine.empty()) {
        c = m_sLine[0];
        if (c == 34) {
            p = m_sLine.find_last_of(34);
            m_sValue = DeEscape(m_sLine.substr(1, p - 1));
            m_enType = enSTRING;
            p++;
        }
        else if (c >= 'A' && c <= 'Z') {
            p = m_sLine.find_first_of(" \t;", 1);
            if (p == string::npos)
                m_sValue = m_sLine;
            else {
                m_sValue = m_sLine.substr(0, p);
                p = m_sLine.find_first_not_of(" \t", p);
                if (p != string::npos && (m_sLine[p] == '-' || isdigit(m_sLine[p]))) {
                    do {
                        m_coVals.push_back(int32_t(std::strtol(m_sLine.c_str() + p, NULL, 10)));
                        p = m_sLine.find_first_of(" \t;", p);
                        if (p != string::npos) {
                            p = m_sLine.find_first_not_of(" \t", p);
                        }
                    } while (p != string::npos && m_sLine[p] != ';');
                }
            }
            m_enType = enBLOCK;
        }
        else if (c == '-' || isdigit(c)) {
            do {
                m_coVals.push_back(int32_t(std::strtol(m_sLine.c_str() + p, NULL, 10)));
                p = m_sLine.find_first_of(" \t;,", p);
                if (p != string::npos) {
                    p = m_sLine.find_first_not_of(" \t,", p);
                }
            } while (p != string::npos && m_sLine[p] != ';');
            m_enType = enINTEGER;
        }
        else {
            char Buff[256];
            snprintf(Buff, sizeof(Buff), "Systax error in line %ld!\n", m_nLineNumber);
            //			Message( Buff );
            m_enType = enERROR;
        }
        if (m_enType != enERROR) {
            if (p != string::npos && p < m_sLine.length() && m_sLine[p] == ';') {
                m_sComment = m_sLine.substr(p + 1, 256);
                AddKeyword(m_sComment);
            }
        }
    }
}

void CReportStream::WriteBlock(const std::string sValue, const std::string sComment, int32_t nDat1, int32_t nDat2, int32_t nDat3)
{
    if (nDat1 != 0x7fffffffL) {
        if (nDat2 != 0x7fffffffL) {
            if (nDat3 != 0x7fffffffL)
                m_oIS << sValue << " " << nDat1 << " " << nDat2 << " " << nDat3;
            else
                m_oIS << sValue << " " << nDat1 << " " << nDat2;
        }
        else {
            m_oIS << sValue << " " << nDat1;
        }
    }
    else
        m_oIS << sValue;

    if (!sComment.empty())
        m_oIS << ";" << sComment << endl;
    else
        m_oIS << endl;
}

void CReportStream::WriteLine(const std::string sValue, const std::string sComment)
{
    m_oIS << "\x22" << sValue << "\x22";

    if (!sComment.empty())
        m_oIS << ";" << sComment << endl;
    else
        m_oIS << endl;
}

void CReportStream::WriteLine(int32_t nValue, const std::string sComment)
{
    m_oIS << nValue;

    if (!sComment.empty())
        m_oIS << ";" << sComment << endl;
    else
        m_oIS << endl;
}

void CReportStream::WriteLine(int32_t nValue1, int32_t nValue2, const std::string sComment)
{
    m_oIS << nValue1 << " " << nValue2;

    if (!sComment.empty())
        m_oIS << ";" << sComment << endl;
    else
        m_oIS << endl;
}
