/****************************************************************************
 *   $Source: D:\\Development\\Repository/ETools/Vorlage/Metascript.h,v $
 *   $Author: ssh $
 *     $Date: 2003/07/01 09:39:30 $
 * $Revision: 1.1 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Klassen fuer die Metabefehlsauswertung
 *****************************************************************************
 *
 * $Log: Metascript.h,v $
 * Revision 1.1  2003/07/01 09:39:30  ssh
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/01 09:19:28  ssh
 * Initial recvsing of Source...
 *
 * Revision 1.9  2000/02/24 09:56:47  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.8  1999/11/17 08:59:13  S.Schuemann
 * - support für multiple CRs
 *
 * - vielfache Änderungen für Vorlage 1.4 beta 8
 *
 * Revision 1.7  1999/11/08 11:14:46  S.Schuemann
 * - Attribute region.gewinn, region[x,y], unit.bewache,
 *   region.pool.ding
 * - Ecaping in Strings
 * - Vars in Attributzugriffen
 * - Funktionen
 * - Sortierung nach BESCHREIBE PRIVAT
 * - Liste fremder Einheiten (normal/verbose)
 * - Bugfix: Leerzeile nach Kapitänsinfo entfernt
 *
 * Revision 1.6  1999/11/03 10:21:57  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.5  1999/10/26 13:27:02  S.Schuemann
 * - Anpassungen fuer Vorlage 1.4 b 5
 *
 * - Neue Objekt-Attribute
 *
 * - User-Variable und #while
 *
 * Revision 1.4  1999/10/24 08:02:18  S.Schuemann
 * - Anpassungen fuer 1.4 b 4
 * - CReference als Value eingefuehrt
 * - Unterprogramme mit #proc und #call implementiert
 *
 * Revision 1.3  1999/10/18 21:32:20  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.2  1999/09/27 10:27:10  S.Schuemann
 * - Scripthandling fuer das neue Objekt REPORT implementiert
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#pragma once

#include <EBase/Expression.h>
#include <EBase/Report.h>
#include <EBase/Value.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class CharacterMapper;

class CTimeoutException
{
public:
    int32_t _runtime;

    CTimeoutException(int32_t runtime)
        : _runtime(runtime)
    {
    }
};

class CMCI
{
public:
    CMCI(int32_t nIdx)
        : m_nTC(nIdx)
        , m_nLoop(0)
        , m_nLine(0)
        , m_pVal(0)
        , m_pvRef(0)
    {
    }

    ~CMCI()
    {
        if (m_pvRef)
            delete m_pvRef;
    }

    int32_t m_nTC;
    int32_t m_nLoop;
    int32_t m_nLine;
    std::string m_sOArg;
    std::string m_sArg;
    Value m_vArg;
    Value* m_pVal;
    CReference* m_pvRef;
};

class CMetaCommand
{
    typedef std::pair<int32_t, int32_t> LOCATION;
    typedef std::vector<std::string> ARGS;
    typedef std::vector<LOCATION> LOCS;

public:
    CMetaCommand(std::string sCom, int32_t nLine, bool bIsFunc = false, bool bVRef = false);
    ~CMetaCommand();

    int AddScript(std::string sCom, int32_t nLine);
    void RunScript(CMCI& oMCI, Expression::Variables& oContext, std::string* psCom, VKommandos& coCmd, int nIdx = 0);

    size_t Args() const { return m_coArgs.size() - 1; }

    void AddCommand(CMCI& oMCI, Expression::Variables& oContext, VKommandos& coCmd, bool bMulti = true);

    void Add(std::string sText) { m_coArgs.push_back(sText); }

    std::string AsString()
    {
        std::string sErg;
        for (size_t i = 0; i < m_coArgs.size(); i++) {
            sErg += m_coArgs[i];
            sErg += ' ';
        }
        return sErg;
    }

    std::string& operator[](size_t pos) { return m_coArgs[pos]; }

    bool IsFunction() const { return m_bIsFunc; }

    void SetFile(const std::string& sFileName) { m_sSrcFile = sFileName; }

    const std::string& GetFile() const { return m_sSrcFile; }

    //	void SetLine( int32_t nLine ) { m_nLine = nLine; }
    void SubBlock(CMCI& oMCI, Expression::Variables& oContext, bool bCond, std::string* psCom, VKommandos& coCmd, int nIdx = -1);

    static bool ProcExists(const std::string& sProc);
    static bool Call(const std::string& sProc, VKommandos& coCmd);

    static void ForceDeclares(bool bForce) { m_bForceDeclare = bForce; }

    static void SetTrace(int32_t nTrace) { g_nTrace = nTrace; }

    static void SetErrMsg(const std::string& sErrMsg) { g_sErrMsg = sErrMsg; }

protected:
    void Skip(CMCI& oMCI);
    bool Parse(CMCI& oMCI, Expression::Variables& oContext, VKommandos& coCmd, bool bAlone = false, bool bExpand = true);
    //	std::string& SEL() { return oMCI.m_sArg; }
    //	Value&	VAL() { return oMCI.m_vArg; }
    void DumpContext(Expression::Variables& oContext);
    void DumpVariable(const std::string& sName, const Value& oVal, bool bExtendedOutput = false);
    void QuicksortHelper1(CMCI& oMCI, VKommandos& coCmd, const std::string& sArrayName, Value* pVector, int32_t l, int32_t r, const std::string& sCmpFunc, const ArgumentList& oArgs);
    void QuicksortHelper2(CMCI& oMCI, VKommandos& coCmd, const std::string& sArrayName, Value* pVector, int32_t l, int32_t r, const std::string& sCmpFunc, const ArgumentList& oArgs);
    int32_t Partition(CMCI& oMCI, VKommandos& coCmd, const std::string& sArrayName, Value* pVector, int32_t l, int32_t r, const std::string& sCmpFunc, const ArgumentList& oArgs);

private:
    bool m_bIsFunc = false;
    bool m_bVRef = false;
    bool m_bParseError = false;
    bool m_bInplace = false;
    ARGS m_coArgs;
    LOCS m_coLocs;
    std::string m_sSrcFile;

    static std::string g_sErrMsg;
    static bool m_bForceDeclare;
    static int32_t g_nTrace;
    //    static int32_t g_nStepOver;
    static int32_t g_nTraceSteps;
};

class CScriptBase
{
public:
    typedef std::map<std::string, CMetaCommand*> COMMANDBASE;

    CScriptBase();
    ~CScriptBase();

    bool Import(const std::string& sFileName);
    bool Import(const std::string& sIFName, const std::string& sText);
    CMetaCommand* FindProc(const std::string& sPName);

protected:
    bool Import(std::istream& oIS, const std::string& sFileName);
    int32_t AddProc(const CharacterMapper* pMapper, const std::string& sFileName, int32_t nLine, std::istream& oIS, std::string& sLine, bool bIsFunc);
    static void GetLine(std::istream& oIS, std::string& sBuff, int32_t& nLineCounter);

    COMMANDBASE m_cpoSubs;
};

extern int32_t g_nLimitRuntime;
extern CKarte* g_poKarte;
extern CReport* g_poCurrentReport;
extern CRegion* g_poCurrentRegion;
extern CBauwerk* g_poCurrentBuilding;
extern CSchiff* g_poCurrentShip;
extern CEinheit* g_poCurrentUnit;
extern CScriptBase g_oScriptBase;

extern void CloseAllOpenFiles();
