/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/Vorlage/Metascript.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:56:47 $
 * $Revision: 1.11 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Klassen fuer die Metabefehlsauswertung
 *****************************************************************************
 *
 * $Log: Metascript.cpp,v $
 * Revision 1.11  2000/02/24 09:56:47  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.10  1999/11/28 17:38:41  S.Schuemann
 * - Mannigfaltige Änderungen für Vorlage V1.4 beta 9
 *
 * Revision 1.9  1999/11/17 08:59:12  S.Schuemann
 * - support für multiple CRs
 *
 * - vielfache Anderungen für Vorlage 1.4 beta 8
 *
 * Revision 1.8  1999/11/08 11:14:46  S.Schuemann
 * - Attribute region.gewinn, region[x,y], unit.bewache,
 *   region.pool.ding
 * - Ecaping in Strings
 * - Vars in Attributzugriffen
 * - Funktionen
 * - Sortierung nach BESCHREIBE PRIVAT
 * - Liste fremder Einheiten (normal/verbose)
 * - Bugfix: Leerzeile nach Kapitänsinfo entfernt
 *
 * Revision 1.7  1999/11/03 10:21:56  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.6  1999/10/28 12:39:58  S.Schuemann
 * - Änderungen für den Linux-Port
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

#include "Metascript.h"
#include <EBase/Hash.h>
#include <EBase/ReportStream.h>
#include <EBase/Utility.h>
#include <EBase/charencoding.h>
#include <EBase/regexp.h>

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>

#include "CRNE.h"


extern std::string GetConfigFileName();
extern FILE* g_hErr;
extern int32_t g_nTimeCorrection;

struct StackInfo
{
    CMetaCommand* m_pMC;
    CMCI* m_pMCI;
};

std::vector<std::string> g_coCallStack;
std::vector<std::string> g_coBreakConditions;
int g_nBreakCondition = 0;
bool g_bNoMoreBreaks = false;
int32_t g_nLimitRuntime = 0;
time_t g_nStartTime = 0;

Value DoUnit(CObjectPart* poPart);
Value DoPartei(CObjectPart* poPart);
Value DoBuilding(CObjectPart* poPart);
Value DoShip(CObjectPart* poPart);
Value DoRegion(CObjectPart* poPart);
Value DoGrenze(CObjectPart* poPart);
Value DoReport(CObjectPart* poPart);
Value DoThings(CObjectPart* poPart);
Value DoRaces(CObjectPart* poPart);

// Internals
Value _DoRegion(CObjectPart* poPart, CRegion* pReg, CRegion* pRegQ);
Value _DoBuilding(CObjectPart* poPart, CRegion* pReg);
Value _DoShip(CObjectPart* poPart, CRegion* pReg);
Value _DoGrenze(CObjectPart* poPart, CRegion* pReg);
Value _DoUnit(CObjectPart* poPart, CEinheit* poUnit, CEinheit* poUnitQ);

static std::map<std::string, std::string> g_pseudoFiles;
CScriptBase g_oScriptBase;

int32_t CMetaCommand::g_nTrace = 0;
int32_t CMetaCommand::g_nTraceSteps = -1;
std::string CMetaCommand::g_sErrMsg;
CMCI* g_poStepOver = nullptr;
CMCI* g_poStepOut = nullptr;

std::map<std::string, int> g_cnMetaTokens;
#define GMT(x) {"#" #x, GMT_##x}

enum METATOKENID {
    GMT_after,
    GMT_array,
    GMT_assert,
    GMT_break,
    GMT_call,
    GMT_config,
    GMT_continue,
    GMT_debug,
    GMT_default,
    GMT_dict,
    GMT_error,
    GMT_every,
    GMT_forever,
    GMT_if,
    GMT_ifregion,
    GMT_ifunit,
    GMT_input,
    GMT_message,
    GMT_next,
    GMT_notrace,
    GMT_return,
    GMT_sort,
    GMT_table,
    GMT_tag,
    GMT_trace,
    GMT_var,
    GMT_warning,
    GMT_while
};

static struct
{
    const char* name;
    int value;
} MetaTokens[] = {GMT(after),  GMT(array), GMT(assert),  GMT(break), GMT(call),    GMT(config), GMT(continue), GMT(debug), GMT(default), GMT(dict),  GMT(error), GMT(every),   GMT(forever), GMT(if), GMT(ifregion),
                  GMT(ifunit), GMT(input), GMT(message), GMT(next),  GMT(notrace), GMT(return), GMT(sort),     GMT(table), GMT(tag),     GMT(trace), GMT(var),   GMT(warning), GMT(while),   {0, 0}};

class CBreakException
{
public:
    CBreakException() {}

    ~CBreakException() {}
};

class CContinueException
{
public:
    CContinueException() {}

    ~CContinueException() {}
};

class CReturnException
{
public:
    CReturnException(const Value& oVal)
        : m_oVal(oVal)
    {
    }

    ~CReturnException() {}

    Value m_oVal;
};

//------------------------------------------------------------------------
// THINGS[<gegenstand>].NAME
//                     .GEWICHT
//                     .KAPAZITAET
//                     .PLURAL
//------------------------------------------------------------------------
Value DoThings(CObjectPart* poPart)
{
    CObjectPart* pOP;
    Value oVal;
    if (poPart->index.size() != 1) {
        oVal.error("Falsche Indizierung fuer Objekt THINGS");
        return oVal;
    }
    pOP = poPart->next;
    if (!pOP) {
        oVal.error("Objekt ohne Attribut benutzt");
        return oVal;
    }
    if (pOP->next) {
        oVal.error("Subattribute werden fuer THINGS.<attribut> nicht unterstuetzt");
        return oVal;
    }
    return CGegenstandsInfo::Lookup(poPart->index[0].asString()).GetValue(pOP->label);
}

//------------------------------------------------------------------------
// RACES[<rasse>].<ErsteSpalteImCFG-KapitelRaces>
//------------------------------------------------------------------------
Value DoRaces(CObjectPart* poPart)
{
    CObjectPart* pOP;
    Value oVal;
    if (poPart->index.size() != 1) {
        oVal.error("Falsche Indizierung fuer Objekt RACES");
        return oVal;
    }
    pOP = poPart->next;
    if (!pOP) {
        oVal.error("Objekt ohne Attribut benutzt");
        return oVal;
    }
    if (pOP->next) {
        oVal.error("Subattribute werden fuer RACES.<attribut> nicht unterstuetzt");
        return oVal;
    }
    return CRasse::Lookup(poPart->index[0].asString()).GetValue(pOP->label);
}

//------------------------------------------------------------------------
// DB.REGION.SIZE
//   .REGION[<index>].<sieheREGION>
//   .UNIT.SIZE
//   .UNIT[<index>].<sieheUNIT>
//------------------------------------------------------------------------
Value DoDB(CObjectPart* poPart)
{
    CObjectPart* pOP;
    Value oVal;

    pOP = poPart->next;

    if (!pOP) {
        oVal.error("Objekt ohne Attribut benutzt");
        return oVal;
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "REGION") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        oVal = Value((int32_t)g_coRDB.size());
    }
    else if (IsEqual(pOP->label.c_str(), "REGION")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt 'db.region' unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || pOP->index[0].asLong() >= (int)g_coRDB.size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt 'db.region[]' korrupt");
            return oVal;
        }
        if (g_nRDBIndex < 0 || !pOP->index[0].asLong()) {
            g_iRDB = g_coRDB.begin();
            g_nRDBIndex = 0;
        }
        int32_t nPos = pOP->index[0].asLong();
        if (nPos > g_nRDBIndex) {
            while (g_nRDBIndex < nPos) {
                g_iRDB++;
                g_nRDBIndex++;
            }
        }
        else {
            while (g_nRDBIndex > nPos) {
                g_iRDB--;
                g_nRDBIndex--;
            }
        }
        CRegion* pReg = *((*g_iRDB).second.begin());
        oVal = _DoRegion(pOP, pReg, pReg);
    }
    else if (pOP->next && pOP->index.empty() && (IsEqual(pOP->label.c_str(), "UNIT") || IsEqual(pOP->label.c_str(), "EINHEIT")) && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        oVal = Value((int32_t)g_coEDB.size());
    }
    else if (IsEqual(pOP->label.c_str(), "UNIT") || IsEqual(pOP->label.c_str(), "EINHEIT")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt 'db.unit' unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || pOP->index[0].asLong() >= (int)g_coEDB.size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt 'db.unit[]' korrupt");
            return oVal;
        }
        CObjectPart oPart;
        oPart.label = "UNIT";
        if (g_nEDBIndex < 0 || !pOP->index[0].asLong()) {
            g_iEDB = g_coEDB.begin();
            g_nEDBIndex = 0;
        }
        int32_t nPos = pOP->index[0].asLong();
        if (nPos > g_nEDBIndex) {
            while (g_nEDBIndex < nPos) {
                g_iEDB++;
                g_nEDBIndex++;
            }
        }
        else {
            while (g_nEDBIndex > nPos) {
                g_iEDB--;
                g_nEDBIndex--;
            }
        }
        CEinheit* pUnit = (*g_iEDB).second;
        oVal = _DoUnit(pOP, pUnit, pUnit);
    }
    else {
        oVal.error("Unbekanntes Attribut des Objekts 'db'");
    }
    return oVal;
}

//------------------------------------------------------------------------
// GRUPPE.NUMMER
//       .ALLIANZ[<pnr>]
//       .<crtags>
//------------------------------------------------------------------------
static Value _DoGruppe(CObjectPart* poPart, CGruppe::Ptr pG)
{
    CObjectPart* pOP = poPart;

    if (IsEqual(pOP->label, "nummer")) {
        return Value(itoan(pG->GetKey(0).asLong(), g_poCurrentReport->PNrBase()));
    }
    if (IsEqual(pOP->label, "allianz") && pOP->index.size() == 1) {
        int32_t nStat = ((CGruppe*)pG.get())->GetAllianz((int32_t)strtol(pOP->index[0].asString().c_str(), 0, g_poCurrentReport->PNrBase()));
        return Value(nStat);
    }
    if (pOP->next)
        return ((CBlockBase*)pG.get())->GetValue(pOP);
    else
        return pG->GetValue(pOP->label);
}

//------------------------------------------------------------------------
// PARTEI.NUMMER
//       .ALLIANZ[<pnr>]
//       .<crtags>
//------------------------------------------------------------------------
static Value _DoPartei(CObjectPart* poPart, CPartei::Ptr pP)
{
    CObjectPart* pOP = poPart;

    if (IsEqual(pOP->label, "nummer")) {
        return Value(itoan(pP->GetKey(0).asLong(), g_poCurrentReport->PNrBase()));
    }
    if (IsEqual(pOP->label, "allianz") && pOP->index.size() == 1) {
        int32_t nStat = ((CPartei*)pP.get())->GetAllianz((int32_t)strtol(pOP->index[0].asString().c_str(), 0, g_poCurrentReport->PNrBase()));
        return Value(nStat);
    }
    if (pOP->next)
        return ((CBlockBase*)pP.get())->GetValue(pOP);
    else
        return pP->GetValue(pOP->label);
}

//------------------------------------------------------------------------
// REPORT[<dr>].UNIT[<enr>].<sieheUNIT>
//             .REGION[<x>,<y>].<sieheREGION>
//             .REGION[<x>,<y>,<z>].<sieheREGION>
// REPORT.REGION.SIZE
//       .REGION[<index>].<sieheREGION>
//       .MESSAGE.SIZE
//       .MESSAGE[<index>].<CR-MESSAGE-Tag> (RENDERED auch, wenn nicht im CR)
//       .RUNDE
//       .OPTIONEN.SIZE
//       .OPTIONEN[<index>].NAME
//       .OPTIONEN[<index>].AKTIV
//       .PARTEI
//       .PARTEI.SIZE
//       .PARTEI[<index>].<siehePARTEI>
//       .GRUPPE.SIZE
//       .GRUPPE[<index>].<siehePARTEI>
//       .REKRUTIERUNGSKOSTEN
//       .PERSONEN
//       .SPIEL
//       .<NeueCRTagsImKopf>
//------------------------------------------------------------------------
Value DoReport(CObjectPart* poPart)
{
    CObjectPart* pOP;
    CRegion* pReg;
    CRegion* pRegQ;
    CEinheit* poUnit;
    CEinheit* poUnitQ;
    Value oVal;

    if (poPart->index.size() > 1) {
        oVal.error("Objekt REPORT unterstuetzt nur einen Index");
        return oVal;
    }

    if (poPart->index.size() == 1) {
        int32_t nDRunde = poPart->index[0].asLong();
        int32_t nRunde = g_poCurrentReport->Runde() + nDRunde;
        pOP = poPart->next;
        if (!pOP) {
            return g_coRRegionDB[-nDRunde].empty() ? Value(0) : Value(1);
        }
        else if (IsEqual(pOP->label.c_str(), "REGION")) {
            if (pOP->next && pOP->index.empty() && IsEqual(pOP->next->label.c_str(), "SIZE")) {
                return Value((int32_t)g_coRRegionDB[-nDRunde].size());
            }
            if (pOP->index.size() == 1) {
                if (pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int32_t)g_coRRegionDB[-nDRunde].size()) {
                    oVal.error("Index in REPORT[dr].REGION[idx] korrupt");
                    return oVal;
                }
                int32_t i = pOP->index[0].asLong();
                RegionDB::iterator rdbi;
                for (rdbi = g_coRRegionDB[-nDRunde].begin(); i && rdbi != g_coRRegionDB[-nDRunde].end(); ++rdbi, ++i) {
                }
                if (rdbi != g_coRRegionDB[-nDRunde].end()) {
                    pReg = *((*rdbi).second.begin());
                    pRegQ = *((*rdbi).second.begin());
                }
                else {
                    pReg = 0;
                    pRegQ = 0;
                }
                return _DoRegion(pOP, pReg, pRegQ);
            }
            if (pOP->index.size() < 2 || pOP->index.size() > 3) {
                oVal.error("Objekt REPORT[dr].REGION[x,y[,z]] braucht zwei oder drei Indizes");
                return oVal;
            }
            RegionDB::iterator rdbi;
            if (pOP->index.size() == 2)
                rdbi = g_coRRegionDB[-nDRunde].find(CRegion::CalcKey(pOP->index[0].asLong(), pOP->index[1].asLong(), 0));
            else
                rdbi = g_coRRegionDB[-nDRunde].find(CRegion::CalcKey(pOP->index[0].asLong(), pOP->index[1].asLong(), pOP->index[2].asLong()));
            if (rdbi != g_coRRegionDB[-nDRunde].end()) {
                pReg = *((*rdbi).second.begin());
                pRegQ = *((*rdbi).second.begin());
            }
            else {
                pReg = 0;
                pRegQ = 0;
            }
            return _DoRegion(pOP, pReg, pRegQ);
        }
        else if (IsEqual(pOP->label.c_str(), "UNIT") || IsEqual(pOP->label.c_str(), "EINHEIT")) {
            if (pOP->index.size() != 1) {
                oVal.error("Objekt REPORT[dr].UNIT[enr] braucht einen Index");
                return oVal;
            }
            EinheitenDB::iterator edbi;
            edbi = g_coREinheitenDB[-nDRunde].find(EinheitenNummer(pOP->index[0].asString()));
            if (edbi != g_coREinheitenDB[-nDRunde].end()) {
                poUnit = (*edbi).second;
                poUnitQ = (*edbi).second;
            }
            else {
                poUnit = 0;
                poUnitQ = 0;
            }
            return _DoUnit(pOP, poUnit, poUnitQ);
        }
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "MESSAGE") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
            return Value((int32_t)CMessage::Messages(nRunde));
        }
        else if (IsEqual(pOP->label.c_str(), "MESSAGE")) {
            if (pOP->index.size() != 1) {
                oVal.error("Objekt REPORT.MESSAGE unterstuetzt nur einen Index");
                return oVal;
            }
            if (g_poCurrentReport->NumMessage()) {
                CMessage* pMsg = CMessage::FindMessage(nRunde, pOP->index[0].asLong());
                if (!pOP->next) {
                    return pMsg ? Value(1) : Value(0);
                }
                if (!pMsg) {
                    oVal.error("Index von REPORT.MESSAGE[] korrupt");
                    return oVal;
                }
                if (IsEqual(pOP->next->label.c_str(), "rendered")) {
                    return Value(pMsg->Render(g_poCurrentReport));
                }
                else if (IsEqual(pOP->next->label.c_str(), "section")) {
                    return (*(g_poCurrentReport->MessageSections()))[pMsg->GetValue("type", Value(0)).asLong()];
                }
                else {
                    return pMsg->GetValue(pMsg, pOP->next, Value(0));
                }
            }
        }
        /*
                else if( IsEqual( pOP->label.c_str(), "MESSAGE" ) )
                {
                    if( g_poCurrentReport->NumMessage() )
                        return Value( (int32_t)g_poCurrentReport->NumMessage() );
                    else
                        return Value( (int32_t)g_poCurrentReport->NumNachrichten() );
                }
        */
        else {
            oVal.error("Unbekanntes Subattribut oder Subobjekt fuer REPORT[dr]");
        }

        return oVal;
    }

    if (!g_poCurrentReport) {
        oVal.error("Objekt REPORT ausserhalb des gueltigen Kontextes benutzt");
        return oVal;
    }

    pOP = poPart->next;

    if (!pOP) {
        oVal.error("Objekt ohne Attribut benutzt");
        return oVal;
    }

    /*
        if( !IsKeyword( pOP->label ) )
        {
            std::string sErr = std::string("Unbekanntes Attribut '") + pOP->label + std::string("' benutzt");
            oVal.error( sErr.c_str() );
            return oVal;
        }
    */
    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "REGION") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        oVal = Value((int32_t)g_poCurrentReport->GetMap()->Regions().size());
    }
    else if (IsEqual(pOP->label.c_str(), "REGION")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REPORT.REGION unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || pOP->index[0].asLong() >= (int)g_poCurrentReport->GetMap()->Regions().size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt REPORT.REGION[] korrupt");
            return oVal;
        }
        pReg = g_poCurrentReport->GetMap()->VRegions()[(size_t)pOP->index[0].asLong()];
        oVal = _DoRegion(pOP, pReg, pReg);
        /*
        CObjectPart oPart;
        oPart.label = "REGION";
        CKarte::RegionMap::iterator rmi = g_poCurrentReport->GetMap()->Regions().begin();
        int cnt = 0;
        int idx = pOP->index[0].asLong();
        while( rmi != g_poCurrentReport->GetMap()->Regions().end() )
        {
            if( cnt == idx )
                break;
            rmi++; cnt++;
        }
        pReg = (*rmi).second;
        oPart.index.push_back( Value( pReg->GetEX() ) );
        oPart.index.push_back( Value( pReg->GetEY() ) );
        oPart.index.push_back( Value( pReg->GetEZ() ) );
        oPart.next = pOP->next;
        int i = oPart.index.size();
        oVal = DoRegion( &oPart );
        oPart.next = 0;
        */
    }
    //***PARTEI*****************************************
    else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "PARTEI") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        return Value((int32_t)g_poCurrentReport->GetParteiNum());
    }
    else if (pOP->next && pOP->index.size() && IsEqual(pOP->label.c_str(), "PARTEI")) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int32_t)g_poCurrentReport->GetParteiNum()) {
            oVal.error("Index von REPORT.PARTEI[<idx>] korrupt");
            return oVal;
        }
        CPartei::Ptr pP(g_poCurrentReport->GetNthParteiInfo(pOP->index[0].asLong()));
        if (pP.get()) {
            return _DoPartei(pOP->next, pP);
        }
    }
    //***GRUPPE*****************************************
    else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "GRUPPE") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        return Value((int32_t)CReport::GetGruppenNum());
    }
    else if (pOP->next && pOP->index.size() && IsEqual(pOP->label.c_str(), "GRUPPE")) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int32_t)CReport::GetGruppenNum()) {
            oVal.error("Index von REPORT.GRUPPE[<idx>] korrupt");
            return oVal;
        }
        CGruppe::Ptr pG(CReport::GetNthGruppe(pOP->index[0].asLong()));
        if (pG.get()) {
            return _DoGruppe(pOP->next, pG);
        }
    }
    //***OPTIONEN*****************************************
    else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "OPTIONEN") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        return Value((int32_t)g_poCurrentReport->Options().size());
    }
    else if (pOP->next && pOP->index.size() && IsEqual(pOP->label.c_str(), "OPTIONEN")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REPORT.OPTIONEN unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)g_poCurrentReport->Options().size()) {
            oVal.error("Index von REPORT.OPTIONEN[] korrupt");
            return oVal;
        }
        if (IsEqual(pOP->next->label, "NAME")) {
            std::string sOpt = g_poCurrentReport->Options()[(size_t)pOP->index[0].asLong()];
            return Value(sOpt.substr(1));
        }
        if (IsEqual(pOP->next->label, "AKTIV")) {
            std::string sOpt = g_poCurrentReport->Options()[(size_t)pOP->index[0].asLong()];
            return Value(sOpt[0] == '+' ? 1 : 0);
        }
        oVal.error("Unbekanntes Subattribut von REPORT.OPTIONEN[]");
        return oVal;
    }
    else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "MESSAGE") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        if (g_poCurrentReport->NumMessage())
            return Value((int32_t)g_poCurrentReport->NumMessage());
        else
            return Value((int32_t)g_poCurrentReport->NumNachrichten());
    }
    else if (IsEqual(pOP->label.c_str(), "MESSAGE")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REPORT.MESSAGE unterstuetzt nur einen Index");
            return oVal;
        }
        if (g_poCurrentReport->NumMessage()) {
            CMessage* pMsg = g_poCurrentReport->GetMessage((size_t)pOP->index[0].asLong());
            if (!pOP->next) {
                return pMsg ? Value(1) : Value(0);
            }
            if (!pMsg) {
                oVal.error("Index von REPORT.MESSAGE[] korrupt");
                return oVal;
            }
            if (IsEqual(pOP->next->label.c_str(), "rendered")) {
                return Value(pMsg->Render(g_poCurrentReport));
            }
            else if (IsEqual(pOP->next->label.c_str(), "section")) {
                return (*(g_poCurrentReport->MessageSections()))[pMsg->GetValue("type", Value(0)).asLong()];
            }
            else {
                return pMsg->GetValue(pMsg, pOP->next, Value(0));
            }
        }
        else {
            std::string sMsg = g_poCurrentReport->GetNachricht(pOP->index[0].asLong());
            if (!pOP->next) {
                return sMsg.empty() ? Value(0) : Value(1);
            }
            if (sMsg.empty()) {
                oVal.error("Index von REPORT.MESSAGE[] korrupt");
                return oVal;
            }
            if (IsEqual(pOP->next->label.c_str(), "rendered")) {
                return Value(sMsg);
            }
            else {
                return Value(0);
            }
        }
    }
    else if (pOP->next || pOP->index.size()) {
        oVal = ((CBlockBase*)g_poCurrentReport)->GetValue(pOP);
        // std::string sErr = std::string("Attribut '") + pOP->label + std::string("' unterstuetzt weder Index noch Subattribute");
        // oVal.error( sErr.c_str() );
    }
    else {
        oVal = g_poCurrentReport->GetValue(pOP->label);
    }
    return oVal;
}

//------------------------------------------------------------------------
// PARTEI[<parteinummer>].<CRTagAusPARTEI/ALLIANZ/ADRESSEN/ALLIIERTE>
//------------------------------------------------------------------------
Value DoPartei(CObjectPart* poPart)
{
    CObjectPart* pOP;
    CPartei::Ptr pP;

    Value oVal;
    if (poPart->index.size() != 1 || poPart->index.empty() || poPart->index[0].asLong() < 0) {
        oVal.error("Falsche Indizierung fuer Objekt PARTEI");
        return oVal;
    }

    pP = CReport::GetGlobalParteiInfo((int32_t)strtol(poPart->index[0].asString().c_str(), 0, g_poCurrentReport->PNrBase()));
    pOP = poPart->next;

    if (!pOP) {
        return pP.get() ? Value(1) : Value(0);
    }

    if (!pP.get()) {
        oVal.error("Falsche Indizierung fuer Objekt PARTEI");
        return oVal;
    }

    if (!pOP) {
        oVal.error("Objekt ohne Attribut benutzt");
        return oVal;
    }
    /*
        if( pOP->next )
        {
            oVal.error( "Subattribute werden fuer PARTEI[].<attribut> nicht unterstuetzt" );
            return oVal;
        }
    */
    return _DoPartei(pOP, pP);
}

//------------------------------------------------------------------------
//              REGION.
//     REGION[<x>,<y>].
// REGION[<x>,<y>,<z>]
//                    .AUSGABEN
//                    .BAEUME
//                    .BAUERN
//                    .BAUWERKE
//                    .BESCHR
//                    .BUILDING.SIZE
//                    .BUILDING[<index>].<sieheBUILDING>
//                    .CHAR
//                    .DURCHREISE.SIZE
//                    .DURCHREISE[<index>]
//                    .DURCHSCHIFFUNGEN.SIZE
//                    .DURCHSCHIFFUNGEN[<index>]
//                    .EFFECTS.SIZE
//                    .EFFECTS[<idx>]
//                    .EINHEITEN
//                    .EINNAHMEN
//                    .EISEN
//                    .GEWINN
//                    .GRENZE.SIZE
//                    .GRENZE[<index>].<sieheGRENZE>
//                    .HERB (In CRs aus anderen Tools)
//                    .INSEL
//                    .LAEN
//                    .LAND
//                    .LOHN
//                    .MALLORN (Nur das Mallorn-Flag, Zahl in BAEUME)
//                    .NAME
//                    .PERSONEN
//                    .PFERDE
//                    .POOL.SILBER
//                    .POOL.<gegenstand>
//                    .PREISE.SIZE
//                    .PREISE[<index>].SILBER
//                    .PREISE[<index>].NAME
//                    .PREISE.<luxusgut>
//                    .REKRUTEN
//                    .RESOURCE.SIZE
//                    .RESOURCE[<index>].<attribut>
//                    .RESOURCE[<typ>].<attribut>
//                    .RUNDE
//                    .SCHIFFE
//                    .SHIP.SIZE
//                    .SHIP.<sieheSHIP>
//                    .SILBER
//                    .SILBERPOOL
//                    .STRASSE (f�r Spiele ohne GRENZE-Bl�cke)
//                    .TERRAIN
//                    .UNIT.SIZE
//                    .UNIT[<index>].<sieheUNIT>
//                    .UNTERHALT
//                    .VERORKT
//                    .X
//                    .Y
//                    .Z
//                    .<NeueCRTags>
//------------------------------------------------------------------------
Value _DoRegion(CObjectPart* poPart, CRegion* pReg, CRegion* pRegQ)
{
    static CRegion::Materialpool coMPool;
    static CRegion* poLastReg = 0;
    static int32_t nLastPartei = 0;
    Value oVal;
    CObjectPart* pOP;

    pOP = poPart->next;

    if (!pOP) {
        return pReg ? Value(1) : Value(0);
    }

    if (!pReg) {
        oVal.error("Objekt REGION ausserhalb des gueltigen Kontextes benutzt");
        return oVal;
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "GRENZE") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        return Value((int32_t)pReg->GetVGrenzen()->size());
    }
    if (IsEqual(pOP->label.c_str(), "GRENZE")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REGION.GRENZE unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || pOP->index[0].asLong() >= (int)pReg->GetVGrenzen()->size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt REGION.GRENZE[] korrupt");
            return oVal;
        }
        CObjectPart oPart;
        oPart.label = "GRENZE";
        oPart.index.push_back(Value(pOP->index[0].asLong()));
        oPart.index.push_back(Value(0));
        oPart.next = pOP->next;
        // int i = oPart.index.size();
        oVal = _DoGrenze(&oPart, pReg);
        oPart.next = 0;
        return oVal;
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "DURCHREISE") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)pRegQ->GetDurchreisen().size());
    }
    else if (!pOP->next && IsEqual(pOP->label.c_str(), "DURCHREISE") && pOP->index.size()) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)pRegQ->GetDurchreisen().size()) {
            oVal.error("Falsche Indizierung in 'region.durchreise[idx]'");
            return oVal;
        }
        return Value(pRegQ->GetDurchreisen()[(size_t)pOP->index[0].asLong()]);
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "DURCHSCHIFFUNG") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)pRegQ->GetDurchschiffungen().size());
    }
    else if (!pOP->next && IsEqual(pOP->label.c_str(), "DURCHSCHIFFUNG") && pOP->index.size()) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)pRegQ->GetDurchschiffungen().size()) {
            oVal.error("Falsche Indizierung in 'region.durchschiffung[idx]'");
            return oVal;
        }
        return Value(pRegQ->GetDurchschiffungen()[(size_t)pOP->index[0].asLong()]);
    }

    if (pOP->next && pOP->index.empty() && (IsEqual(pOP->label.c_str(), "REGIONSBOTSCHAFTEN") || IsEqual(pOP->label.c_str(), "REGIONSEREIGNISSE")) && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value(pRegQ->CBlockBase::GetValue(pOP));
    }
    else if (!pOP->next && (IsEqual(pOP->label.c_str(), "REGIONSBOTSCHAFTEN") || IsEqual(pOP->label.c_str(), "REGIONSEREIGNISSE")) && pOP->index.size()) {
        int numIdx = 0;
        bool foundClass = false;
        std::shared_ptr<CBlockBase> pBlock = pRegQ->CBlockBase::GetBlock(pOP->label, foundClass);
        if (pBlock)
            numIdx = (int)pBlock->NumValues();
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= numIdx) {
            oVal.error((std::string("Falsche Indizierung in 'region.") + pOP->label + "[idx]'").c_str());
            return oVal;
        }
        return Value(pRegQ->GetValue(std::string("@") + pOP->index[0].asString()));
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "EFFECTS") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)pRegQ->GetEffects().size());
    }
    else if (!pOP->next && IsEqual(pOP->label.c_str(), "EFFECTS") && pOP->index.size()) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)pRegQ->GetEffects().size()) {
            oVal.error("Falsche Indizierung in 'region.effects[idx]'");
            return oVal;
        }
        return Value(pRegQ->GetEffects()[(size_t)pOP->index[0].asLong()]);
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "RESOURCE") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)pRegQ->GetResourcen().size());
    }
    else if (pOP->next && IsEqual(pOP->label.c_str(), "RESOURCE") && pOP->index.size()) {
        if (pOP->index.size() == 1 && pOP->index[0].getType() == VT_STRING) {
            CResource* pRes = pRegQ->GetResource(pOP->index[0].asString());
            if (pRes)
                return Value(pRes->GetValue(pOP->next->label));
            return Value(0);
        }
        else if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)pRegQ->GetResourcen().size()) {
            oVal.error("Falsche Indizierung in 'region.resource[idx]'");
            return oVal;
        }
        return Value(pRegQ->GetResourcen()[(size_t)pOP->index[0].asLong()]->GetValue(pOP->next->label));
    }

    if (IsEqual(pOP->label.c_str(), "Einheiten"))
        return Value((int32_t)pReg->GetVEinheiten().size());

    if (IsEqual(pOP->label.c_str(), "unit") || IsEqual(pOP->label.c_str(), "EINHEIT")) {
        if (pOP->next && pOP->index.empty() && IsEqual(pOP->next->label.c_str(), "SIZE")) {
            return Value((int32_t)pReg->GetVEinheiten().size());
        }
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REGION.UNIT unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || pOP->index[0].asLong() >= (int)pReg->GetVEinheiten().size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt REGION.UNIT[] korrupt");
            return oVal;
        }
        CObjectPart oPart;
        oPart.label = "UNIT";
        //      if( IsFlag( VF_BASE36 ) )
        oPart.index.push_back(Value(itoan(pReg->GetVEinheiten()[(size_t)pOP->index[0].asLong()]->Nummer(), g_poCurrentReport->ENrBase())));
        //        else
        //          oPart.index.push_back( Value( pReg->GetVEinheiten()[pOP->index[0].asLong()]->Nummer() )  );
        oPart.next = pOP->next;
        // int i = oPart.index.size();
        oVal = DoUnit(&oPart);
        oPart.next = 0;
        return oVal;
    }

    if (IsEqual(pOP->label.c_str(), "building") || IsEqual(pOP->label.c_str(), "burg")) {
        if (pOP->next && pOP->index.empty() && IsEqual(pOP->next->label.c_str(), "SIZE")) {
            return Value(pReg->GetVBauwerke() ? (int32_t)pReg->GetVBauwerke()->size() : 0);
        }
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REGION.BUILDING unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || !pReg->GetVBauwerke() || pOP->index[0].asLong() >= (int)pReg->GetVBauwerke()->size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt REGION.BUILDING[] korrupt");
            return oVal;
        }
        CObjectPart oPart;
        oPart.label = "BUILDING";
        oPart.index.push_back(Value(itoan((*(pReg->GetVBauwerke()))[(size_t)pOP->index[0].asLong()] -> Nummer(), g_poCurrentReport -> BNrBase())));
        oPart.next = pOP->next;
        // int i = oPart.index.size();
        oVal = _DoBuilding(&oPart, pReg);
        oPart.next = 0;
        return oVal;
        /*
                if( pOP->index.size()!=1 )
                {
                    oVal.error( "Objekt REGION.BUILDING unterstuetzt nur einen Index" );
                    return oVal;
                }
                if( pOP->index[0].getType()!=VT_INT ||
                    !pReg->GetBuilding( pOP->index[0].asLong() ) )
                {
                    oVal.error( "Index von Objekt REGION.BUILDING[] korrupt" );
                    return oVal;
                }
                CObjectPart* pNOP = pOP->next;
                CBauwerk* pB = pReg->GetBuilding( pOP->index[0].asLong() );

                if( pNOP && IsEqual( pNOP->label.c_str(), "Typ" ) )
                {
                    return Value( pB->Typ() );
                }
                else if( pNOP && IsEqual( pNOP->label.c_str(), "Groesse" ) )
                {
                    return Value( pB->Groesse() );
                }

                oVal.error( "Unbekanntes Attribut des Objektes BUILDING verwendet" );
                return oVal;
        */
    }

    if (IsEqual(pOP->label.c_str(), "ship") || IsEqual(pOP->label.c_str(), "schiff")) {
        if (pOP->next && pOP->index.empty() && IsEqual(pOP->next->label.c_str(), "SIZE")) {
            return Value(pReg->GetVSchiffe() ? (int32_t)pReg->GetVSchiffe()->size() : 0);
        }
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REGION.SHIP unterstuetzt nur einen Index");
            return oVal;
        }
        if (pOP->index[0].getType() != VT_INT || !pReg->GetVSchiffe() || pOP->index[0].asLong() >= (int)pReg->GetVSchiffe()->size() || pOP->index[0].asLong() < 0) {
            oVal.error("Index von Objekt REGION.SHIP[] korrupt");
            return oVal;
        }
        CObjectPart oPart;
        oPart.label = "SHIP";
        oPart.index.push_back(Value(itoan((*(pReg->GetVSchiffe()))[(size_t)pOP->index[0].asLong()] -> Nummer(), g_poCurrentReport -> BNrBase())));
        oPart.next = pOP->next;
        // int i = oPart.index.size();
        oVal = _DoShip(&oPart, pReg);
        oPart.next = 0;
        return oVal;
        /*


                if( pOP->index.size()!=1 )
                {
                    oVal.error( "Objekt REGION.SHIP unterstuetzt nur einen Index" );
                    return oVal;
                }
                if( pOP->index[0].getType()!=VT_INT ||
                    !pReg->GetShip( pOP->index[0].asLong() ) )
                {
                    oVal.error( "Index von Objekt REGION.SHIP[] korrupt" );
                    return oVal;
                }
                CObjectPart* pNOP = pOP->next;
                CSchiff* pS = pReg->GetShip( pOP->index[0].asLong() );

                if( pNOP && IsEqual( pNOP->label.c_str(), "Typ" ) )
                {
                    return Value( pS->Typ() );
                }
                else if( pNOP && IsEqual( pNOP->label.c_str(), "MaxLadung" ) )
                {
                    return Value( pS->MaxLadung() );
                }

                oVal.error( "Unbekanntes Attribut des Objektes SHIP verwendet" );
                return oVal;
        */
    }

    if (IsEqual(pOP->label.c_str(), "Pool")) {
        CRegion::Materialpool::iterator mi;
        CObjectPart* pPOP;
        int32_t nP = g_poCurrentReport->Partei();
        pPOP = pOP->next;
        if (!pPOP) {
            std::string sErr = std::string("Attribut POOL ohne Subattribute benutzt");
            oVal.error(sErr.c_str());
            return oVal;
        }

        if (IsEqual(pPOP->label.c_str(), "Silber")) {
            return Value(pReg->SilverOf(g_poCurrentReport->Partei()));
        }
        if (nP != nLastPartei || pReg != poLastReg) {
            coMPool.clear();
            pReg->AddMaterialpool(nP, coMPool);
            nLastPartei = nP;
            poLastReg = pReg;
        }

        if (!pPOP->next && IsEqual(pPOP->label.c_str(), "Size")) {
            return Value((int32_t)coMPool.size());
        }

        if (pOP->next && pOP->index.size() == 1) {
            int32_t nIdx = pOP->index[0].asLong();
            mi = coMPool.begin();
            while (mi != coMPool.end() && nIdx) {
                mi++;
                nIdx--;
            }
            if (mi == coMPool.end()) {
                oVal.error("Index auf REGION.POOL[] korrupt");
                return oVal;
            }
            if (IsEqual(pPOP->label, "name")) {
                return Value((*mi).first);
            }
            if (IsEqual(pPOP->label, "anzahl")) {
                return Value((int32_t)((*mi).second));
            }
            oVal.error("Unbekanntes Subattribut von REGION.POOL[]");
            return oVal;
        }

        mi = coMPool.find(DeUmlaut(pPOP->label));
        if (mi != coMPool.end())
            oVal = Value((int32_t)((*mi).second));
        else {
            oVal = Value(0);
            for (mi = coMPool.begin(); mi != coMPool.end(); mi++) {
                if (Flatten((*mi).first) == Flatten(pPOP->label)) {
                    oVal = Value((int32_t)((*mi).second));
                    break;
                }
            }
        }

        return oVal;
    }

    if (IsEqual(pOP->label.c_str(), "PREISE")) {
        if (pOP->next && pOP->index.empty() && IsEqual(pOP->next->label.c_str(), "SIZE")) {
            return Value((int32_t)pReg->GetLuxusgueter().size());
        }

        if (pOP->index.size() == 1 && pOP->next) {
            if (pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)pReg->GetLuxusgueter().size()) {
                oVal.error("Index von Objekt REGION.PREISE[] korrupt");
                return oVal;
            }
            if (IsEqual(pOP->next->label, "Silber")) {
                return Value((int32_t)pReg->GetLuxusgueter()[(size_t)pOP->index[0].asLong()].second);
            }
            else if (IsEqual(pOP->next->label, "Name")) {
                return Value(pReg->GetLuxusgueter()[(size_t)pOP->index[0].asLong()].first);
            }
            oVal.error("Unbekanntes Subattribut fuer REGION.PREISE[]");
            return oVal;
        }

        CObjectPart* pPOP;
        pPOP = pOP->next;
        if (!pPOP) {
            std::string sErr = std::string("Attribut PREISE ohne Subattribute benutzt");
            oVal.error(sErr.c_str());
            return oVal;
        }

        for (size_t li = 0; li < pReg->GetLuxusgueter().size(); li++) {
            if (IsEqual(pReg->GetLuxusgueter()[li].first, pPOP->label.c_str()))
                return Value((int32_t)pReg->GetLuxusgueter()[li].second);
        }

        return Value(0);
    }
    else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "MESSAGE") && IsEqual(pOP->next->label.c_str(), "SIZE")) {
        return Value((int32_t)pReg->NumMessage());
    }
    else if (IsEqual(pOP->label.c_str(), "MESSAGE")) {
        if (pOP->index.size() != 1) {
            oVal.error("Objekt REGION.MESSAGE unterstuetzt nur einen Index");
            return oVal;
        }
        if (pReg->NumMessage()) {
            CMessage* pMsg = pReg->GetMessage(pOP->index[0].asLong());
            if (!pOP->next) {
                return pMsg ? Value(1) : Value(0);
            }
            if (!pMsg) {
                oVal.error("Index von REGION.MESSAGE[] korrupt");
                return oVal;
            }
            if (IsEqual(pOP->next->label.c_str(), "rendered")) {
                return Value(pMsg->Render(g_poCurrentReport));
            }
            else if (IsEqual(pOP->next->label.c_str(), "section")) {
                return (*(g_poCurrentReport->MessageSections()))[pMsg->GetValue("type", Value(0)).asLong()];
            }
            else {
                return pMsg->GetValue(pMsg, pOP->next, Value(0));
            }
        }
    }

    if (pOP->next || pOP->index.size()) {
        oVal = ((CBlockBase*)pRegQ)->GetValue(pOP);
        //      std::string sErr = std::string("Attribut '") + pOP->label + std::string("' unterstuetzt weder Index noch Subattribute");
        //      oVal.error( sErr.c_str() );
        return oVal;
    }

    if (IsEqual(pOP->label.c_str(), "Gewinn"))
        oVal = Value(pRegQ->CalcProfit());
    else if (IsEqual(pOP->label.c_str(), "Land"))
        oVal = Value(pRegQ->IsLand() ? 1 : 0);
    else if (IsEqual(pOP->label.c_str(), "Runde"))
        oVal = Value((int32_t)(pRegQ->Runde()));
    else if (IsEqual(pOP->label.c_str(), "Name"))
        oVal = Value(pRegQ->GetName());
    else if (IsEqual(pOP->label.c_str(), "Beschr"))
        oVal = Value(pRegQ->Beschr());
    else if (IsEqual(pOP->label.c_str(), "Terrain"))
        oVal = Value(pRegQ->GetRegionTypeName());
    else if (IsEqual(pOP->label.c_str(), "Personen"))
        oVal = Value(pReg->PersonsOf(g_poCurrentReport->Partei()));
    else if (IsEqual(pOP->label.c_str(), "Silberpool"))
        oVal = Value(pReg->SilverOf(g_poCurrentReport->Partei()));
    else if (IsEqual(pOP->label.c_str(), "Bauwerke"))
        oVal = Value((int32_t)pReg->NumBuildings());
    else if (IsEqual(pOP->label.c_str(), "Schiffe"))
        oVal = Value((int32_t)pReg->NumShips());
    else {
        if (pOP->next)
            oVal = ((CBlockBase*)pRegQ)->GetValue(pOP);
        else
            oVal = pRegQ->DeepGetValue(pOP->label);
        /*
                oVal = pRegQ->GetValue( pOP->label );
                if( oVal.getType() == VT_EMPTY )
                {
                    RegionDB::iterator rdbi;
                    rdbi = g_coRDB.find( pRegQ->GetKey() );
                    if( rdbi != g_coRDB.end() )
                    {
                        RegionSet::iterator rsi = (*rdbi).second.begin();
                        while( rsi != (*rdbi).second.end() )
                        {
                            oVal = (*rsi)->GetValue( pOP->label );
                            if( oVal.getType() != VT_EMPTY )
                                break;
                            rsi++;
                        }
                        if( oVal.getType() == VT_EMPTY )
                        {
                            oVal = Value( 0 );
                        }
                    }
                }
        */
    }
    return oVal;
}

Value DoRegion(CObjectPart* poPart)
{
    CRegion* pReg = g_poCurrentRegion;
    CRegion* pRegQ = pReg;
    Value oVal;

    if (poPart->index.size() == 1 || poPart->index.size() > 3) {
        oVal.error("Falsche Indizierung fuer Objekt REGION");
        return oVal;
    }
    if (poPart->index.size() == 2) {
        RegionDB::iterator rdbi;

        if (!g_poKarte) {
            oVal.error("Objekt REGION[x,y] ausserhalb des gueltigen Kontextes benutzt");
            return oVal;
        }

        pReg = g_poKarte->GetFromECords(poPart->index[0].asLong(), poPart->index[1].asLong(), 0);
        if (pReg->GetEX() != poPart->index[0].asLong() || pReg->GetEY() != poPart->index[1].asLong() || pReg->GetEZ() != 0) {
            rdbi = g_coRDB.find(CRegion::CalcKey(poPart->index[0].asLong(), poPart->index[1].asLong(), 0));
            if (rdbi != g_coRDB.end()) {
                pReg = *((*rdbi).second.begin());
                pRegQ = *((*rdbi).second.begin());
            }
            else {
                pReg = 0;
                pRegQ = 0;
            }
        }
        else {
            rdbi = g_coRDB.find(pReg->GetKey());
            if (rdbi != g_coRDB.end())
                pRegQ = *((*rdbi).second.begin());
        }
    }
    if (poPart->index.size() == 3) {
        RegionDB::iterator rdbi;

        if (!g_poKarte) {
            oVal.error("Objekt REGION[x,y,z] ausserhalb des gueltigen Kontextes benutzt");
            return oVal;
        }

        pReg = g_poKarte->GetFromECords(poPart->index[0].asLong(), poPart->index[1].asLong(), poPart->index[2].asLong());
        if (pReg->GetEX() != poPart->index[0].asLong() || pReg->GetEY() != poPart->index[1].asLong() || pReg->GetEZ() != poPart->index[2].asLong()) {
            rdbi = g_coRDB.find(CRegion::CalcKey(poPart->index[0].asLong(), poPart->index[1].asLong(), poPart->index[2].asLong()));
            if (rdbi != g_coRDB.end()) {
                pReg = *((*rdbi).second.begin());
                pRegQ = *((*rdbi).second.begin());
            }
            else {
                pReg = 0;
                pRegQ = 0;
            }
        }
        else {
            rdbi = g_coRDB.find(pReg->GetKey());
            if (rdbi != g_coRDB.end())
                pRegQ = *((*rdbi).second.begin());
        }
    }

    if (pReg && pRegQ == pReg) {
        RegionDB::iterator rdbi;
        rdbi = g_coRDB.find(pReg->GetKey());
        if (rdbi != g_coRDB.end()) {
            pRegQ = *((*rdbi).second.begin());
        }
    }

    return _DoRegion(poPart, pReg, pRegQ);

    //  pReg = g_poKarte->GetFromECords( coArgs[1].asLong(), coArgs[2].asLong() );
    //  if( pReg )
    //  {
    //      oVal = Value( pReg->GetValue( coArgs[3].asString() ) );
    //  }
    //  return oVal;
}

//------------------------------------------------------------------------
// GRENZE.RICHTUNG
//       .TYP
//       .PROZENT
//       .EFFECTS.SIZE
//       .EFFECTS[<idx>]
//       .<NeueCRTags>
//------------------------------------------------------------------------
Value _DoGrenze(CObjectPart* poPart, CRegion* pReg)
{
    //  CRegion::Einheiten::iterator i;
    CObjectPart* pOP;
    CGrenze* poGrenze = 0;
    Value oVal;

    // int i = poPart->index.size();

    pOP = poPart->next;

    int32_t nGNr = poPart->index[0].asLong();

    if (poPart->index.size() == 1)
        poGrenze = pReg->GetFrontier(nGNr);
    else
        poGrenze = (*(pReg->GetVGrenzen()))[(size_t)nGNr];

    if (!pOP) {
        return poGrenze ? Value(1) : Value(0);
    }

    if (!poGrenze) {
        oVal.error("Falsche Indizierung fuer Objekt GRENZE");
        return oVal;
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "EFFECTS") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)poGrenze->GetEffects().size());
    }
    else if (!pOP->next && IsEqual(pOP->label.c_str(), "EFFECTS") && pOP->index.size()) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poGrenze->GetEffects().size()) {
            oVal.error("Falsche Indizierung in 'grenze.effects[idx]'");
            return oVal;
        }
        return Value(poGrenze->GetEffects()[(size_t)pOP->index[0].asLong()]);
    }

    if (IsEqual(pOP->label.c_str(), "Richtung"))
        oVal = Value((int32_t)poGrenze->Richtung());
    else if (IsEqual(pOP->label.c_str(), "Typ"))
        oVal = Value(poGrenze->Typ());
    else if (IsEqual(pOP->label.c_str(), "Prozent"))
        oVal = Value((int32_t)poGrenze->Prozent());
    else {
        if (pOP->next)
            oVal = ((CBlockBase*)poGrenze)->GetValue(pOP);
        else
            oVal = poGrenze->GetValue(pOP->label);  //.error("Unbekanntes Attribut von Objekt GRENZE benutzt.");
    }
    return oVal;
}

Value DoGrenze(CObjectPart* poPart)
{
    //  CRegion::Einheiten::iterator i;
    //  CObjectPart* pOP;
    //  CGrenze* poGrenze = 0;
    Value oVal;

    if (poPart->index.size() != 1) {
        oVal.error("Objekt GRENZE unterstuetzt nur einen Index");
        return oVal;
    }

    return _DoGrenze(poPart, g_poCurrentRegion);
    /*
        int i = poPart->index.size();

        pOP = poPart->next;

        int32_t nGNr = poPart->index[0].asLong();

        if( poPart->index.size()==1 )
            poGrenze = g_poCurrentRegion->GetFrontier( nGNr );
        else
            poGrenze = (*(g_poCurrentRegion->GetVGrenzen()))[nGNr];

        if( !pOP )
        {
            return poGrenze ? Value( 1 ) : Value( 0 );
        }

        if( !poGrenze )
        {
            oVal.error( "Falsche Indizierung fuer Objekt GRENZE" );
            return oVal;
        }

        if( IsEqual( pOP->label.c_str(), "Richtung" ) )
            oVal = Value( (int32_t)poGrenze->Richtung() );
        else if( IsEqual( pOP->label.c_str(), "Typ" ) )
                oVal = Value( poGrenze->Typ() );
        else if( IsEqual( pOP->label.c_str(), "Prozent" ) )
                oVal = Value( (int32_t)poGrenze->Prozent() );
        else
        {
            oVal = poGrenze->GetValue( pOP->label );//.error("Unbekanntes Attribut von Objekt GRENZE benutzt.");
        }
        return oVal;
    */
}

//------------------------------------------------------------------------
//            UNIT.
// UNIT[<einheit>].
//                .ALIAS
//                .ANZAHL
//                .AURA
//                .AURAMAX
//                .BAUWERK
//                .BESCHR
//                .BEWACHT
//                .COMMANDS.SIZE
//                .COMMANDS[<index>]
//                .EFFECTS.SIZE
//                .EFFECTS[<idx>]
//                .EINHEITSBOTSCHAFTEN.SIZE
//                .EINHEITSBOTSCHAFTEN[<index>]
//                .FREI.REITEN
//                .FREI.GEHEN
//                .GEGENSTAENDE.SIZE
//                .GEGENSTAENDE[<index>].NAME
//                .GEGENSTAENDE[<index>].ANZAHL
//                .<Gegenstand>
//                .GEWICHT
//                .GRUPPE
//                .GRUPPE.<tags>
//                .HASMETAS (1, wenn Metabefehle in der Einheit)
//                .HP
//                .HUNGER
//                .KAMPFSTATUS
//                .KAMPFZAUBER.SIZE
//                .KAMPFZAUBER[<index>].KEY
//                .KAMPFZAUBER[<index>].<tags>
//                .KAP.REITEN
//                .KAP.GEHEN
//                .NAME
//                .NUMMER
//                .PARTEI
//                .PARTEINAME
//                .PARTEITARNUNG
//                .POSITION (in Bauwerk)
//                .PRIVAT
//                .REGION.<sieheREGION>
//                .RUNDE
//                .SCHIFF
//                .SILBER
//                .TALENTE.SIZE
//                .TALENTE[<index>].NAME
//                .TALENTE[<index>].STUFE
//                .TALENTE[<index>].TAGE
//                .TALENTE.<talent>[<index>]
//                .<talent>.STUFE
//                .<talent>.TAGE
//                .TEMP
//                .TYP
//                .VERKLEIDET
//                .VERRAETER
//                .WAHRERTYP
//                .X
//                .Y
//                .Z
//                .<NeueCRTags>
//------------------------------------------------------------------------
Value _DoUnit(CObjectPart* poPart, CEinheit* poUnit, CEinheit* poUnitQ)
{
    CObjectPart* pOP;
    Value oVal;

    pOP = poPart->next;

    if (!pOP) {
        return poUnit ? Value(1) : Value(0);
    }

    if (!poUnit) {
        if (poPart->index.size())
            oVal.error("Falsche Indizierung fuer Objekt UNIT");
        else
            oVal.error("Objekt UNIT ausserhalb gueltigen Kontextes");
        return oVal;
    }

    if (poUnitQ) {
        if (IsEqual(pOP->label.c_str(), "Nummer")) {
            oVal = Value(itoan(poUnitQ->Nummer(), g_poCurrentReport->ENrBase()));
        }
        else if (IsEqual(pOP->label.c_str(), "Typ")) {
            oVal = Value(poUnitQ->Typ());
        }
        else if (IsEqual(pOP->label.c_str(), "WahrerTyp")) {
            oVal = Value(poUnitQ->WahrerTyp());
        }
        else if (IsEqual(pOP->label.c_str(), "Name")) {
            oVal = Value(poUnitQ->Name());
        }
        else if (IsEqual(pOP->label.c_str(), "Gewicht")) {
            oVal = Value(poUnitQ->Gewicht());
        }
        else if (IsEqual(pOP->label.c_str(), "hp")) {
            oVal = Value(poUnitQ->HP());
        }
        else if (IsEqual(pOP->label.c_str(), "Runde")) {
            oVal = Value((int32_t)(poUnitQ->Runde()));
        }
        else if (IsEqual(pOP->label.c_str(), "Region")) {
            if (pOP->index.size() != 2) {
                oVal.error("Falsche Indizierung in 'unit.region[dx,dy]'");
                return oVal;
            }
            CObjectPart oPart;
            oPart.label = "REGION";
            oPart.index.push_back(Value(poUnitQ->Region()->GetEX() + pOP->index[0].asLong()));
            oPart.index.push_back(Value(poUnitQ->Region()->GetEY() + pOP->index[1].asLong()));
            oPart.index.push_back(Value(poUnitQ->Region()->GetEZ()));
            oPart.next = pOP->next;
            // int i = oPart.index.size();
            oVal = DoRegion(&oPart);
            oPart.next = 0;
            return oVal;
        }
        else if (IsEqual(pOP->label.c_str(), "X")) {
            oVal = Value(poUnitQ->Region()->GetEX());
        }
        else if (IsEqual(pOP->label.c_str(), "Y")) {
            oVal = Value(poUnitQ->Region()->GetEY());
        }
        else if (IsEqual(pOP->label.c_str(), "Z")) {
            oVal = Value(poUnitQ->Region()->GetEZ());
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "frei") && IsEqual(pOP->next->label.c_str(), "reiten")) {
            double fKapReiten, fFKapReiten;
            double fKapGehen, fFKapGehen;
            int32_t nRHO, nGHO;
            poUnitQ->CalcKapazitaeten(fKapReiten, fFKapReiten, nRHO, fKapGehen, fFKapGehen, nGHO);
            oVal = Value(fFKapReiten);
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "frei") && IsEqual(pOP->next->label.c_str(), "gehen")) {
            double fKapReiten, fFKapReiten;
            double fKapGehen, fFKapGehen;
            int32_t nRHO, nGHO;
            poUnitQ->CalcKapazitaeten(fKapReiten, fFKapReiten, nRHO, fKapGehen, fFKapGehen, nGHO);
            oVal = Value(fFKapGehen);
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "kap") && IsEqual(pOP->next->label.c_str(), "reiten")) {
            double fKapReiten, fFKapReiten;
            double fKapGehen, fFKapGehen;
            int32_t nRHO, nGHO;
            poUnitQ->CalcKapazitaeten(fKapReiten, fFKapReiten, nRHO, fKapGehen, fFKapGehen, nGHO);
            oVal = Value(fKapReiten);
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "kap") && IsEqual(pOP->next->label.c_str(), "gehen")) {
            double fKapReiten, fFKapReiten;
            double fKapGehen, fFKapGehen;
            int32_t nRHO, nGHO;
            poUnitQ->CalcKapazitaeten(fKapReiten, fFKapReiten, nRHO, fKapGehen, fFKapGehen, nGHO);
            oVal = Value(fKapGehen);
        }
        // ************ Kampfzauber ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "kampfzauber") && IsEqual(pOP->next->label.c_str(), "size")) {
            oVal = Value((int32_t)poUnitQ->CSpells().size());
        }
        else if (pOP->next && pOP->index.size() == 1 && IsEqual(pOP->label.c_str(), "kampfzauber")) {
            if (pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->CSpells().size()) {
                return Value(0);
            }

            CKampfzauber::Ptr pK = poUnitQ->CSpells()[(size_t)pOP->index[0].asLong()];

            if (IsEqual(pOP->next->label.c_str(), "key")) {
                if (pOP->next->index.size()) {
                    return pK->GetKey(0);
                }
                return Value(0);
            }
            return pK->GetValue(pOP->next->label);
        }
        // ************ Talente ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "talente") && IsEqual(pOP->next->label.c_str(), "size")) {
            oVal = Value((int32_t)poUnitQ->Talents().size());
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "talente") &&
                 (IsEqual(pOP->next->label.c_str(), "name") || IsEqual(pOP->next->label.c_str(), "tage") || IsEqual(pOP->next->label.c_str(), "stufe") || IsEqual(pOP->next->label.c_str(), "punkte") || IsEqual(pOP->next->label.c_str(), "mod"))) {
            if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->Talents().size()) {
                oVal.error("Falsche Indizierung in 'unit.talente[idx]'");
                return oVal;
            }
            const CTalent* pT = &(poUnitQ->Talents()[(size_t)pOP->index[0].asLong()]);
            std::string sTalent = pT->m_sTyp;
            if (IsEqual(pOP->next->label.c_str(), "name"))
                oVal = Value(pT->m_sTyp);
            else if (IsEqual(pOP->next->label.c_str(), "tage"))
                oVal = poUnitQ->GetValue(sTalent, "tage");
            else if (IsEqual(pOP->next->label.c_str(), "stufe"))
                oVal = Value((int32_t)pT->m_nStufe);
            else if (IsFlag(VF_NOSKILLPOINTS) && IsEqual(pOP->next->label.c_str(), "mod"))
                oVal = poUnitQ->GetValue(sTalent, "mod");
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "talente") && pOP->index.empty() && pOP->next && !pOP->next->next) {
            //            const CTalent* pT;
            for (size_t i = 0; i < poUnitQ->Talents().size(); i++) {
                if (IsEqual(poUnitQ->Talents()[i].m_sTyp, pOP->next->label.c_str())) {
                    int idx = pOP->next->index.empty() ? 0 : pOP->next->index[0].asLong();
                    switch (idx) {
                        case 0:
                            return Value(poUnitQ->Talents()[i].m_nTage);
                        case 1:
                            return Value((int32_t)(poUnitQ->Talents()[i].m_nStufe));
                        case 2:
                            return Value(poUnitQ->Talents()[i].m_nAddon);
                        default:
                            return Value(0);
                    }
                }
            }
            return Value(0);
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "talente")) {
            oVal.error("Unbekanntes Subattribut fuer 'unit.talente'");
            return oVal;
        }
        // ************ Kommandos ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "COMMANDS") && IsEqual(pOP->next->label.c_str(), "size")) {
            oVal = Value((int32_t)poUnitQ->GetKommandos().size());
        }
        else if (!pOP->next && IsEqual(pOP->label.c_str(), "COMMANDS") && pOP->index.size()) {
            if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->GetKommandos().size()) {
                oVal.error("Falsche Indizierung in 'unit.commands[idx]'");
                return oVal;
            }
            oVal = Value(poUnitQ->GetKommandos()[pOP->index[0].asLong()]);
        }
        // ************ MetaOut ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "OUTPUT") && IsEqual(pOP->next->label.c_str(), "size")) {
            oVal = Value((int32_t)poUnitQ->GetMetaOut().size());
        }
        else if (!pOP->next && IsEqual(pOP->label.c_str(), "OUTPUT") && pOP->index.size()) {
            if (Expression::inAssignment() && pOP->index.size() == 1) {
                if (pOP->index[0].asLong() == (int32_t)poUnitQ->GetMetaOut().size()) {
                    poUnitQ->GetMetaOut().push_back(Value(""));
                    poUnitQ->GetMetaOut().changed(true);
                    return Value(poUnitQ->GetMetaOut()[pOP->index[0].asLong()], true);
                }
                if (pOP->index[0].asLong() >= 0 || pOP->index[0].asLong() < (int)poUnitQ->GetMetaOut().size()) {
                    poUnitQ->GetMetaOut().changed(true);
                    return Value(poUnitQ->GetMetaOut()[pOP->index[0].asLong()], true);
                }
            }
            if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->GetMetaOut().size()) {
                oVal.error("Falsche Indizierung in 'unit.output[idx]'");
                return oVal;
            }
            return Value(poUnitQ->GetMetaOut()[pOP->index[0].asLong()], false);
        }
        else if (!pOP->next && IsEqual(pOP->label.c_str(), "OUTPUT") && !pOP->index.size()) {
            return Value(poUnitQ->GetMetaOut());
        }
        // ************ Effekte ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "EFFECTS") && IsEqual(pOP->next->label.c_str(), "size")) {
            return Value((int32_t)poUnitQ->GetEffects().size());
        }
        else if (!pOP->next && IsEqual(pOP->label.c_str(), "EFFECTS") && pOP->index.size()) {
            if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->GetEffects().size()) {
                oVal.error("Falsche Indizierung in 'unit.effects[idx]'");
                return oVal;
            }
            return Value(poUnitQ->GetEffects()[(size_t)pOP->index[0].asLong()]);
        }
        // ************ Einheitsbotschaften ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "EINHEITSBOTSCHAFTEN") && IsEqual(pOP->next->label.c_str(), "size")) {
            oVal = Value((int32_t)poUnitQ->GetBotschaften().size());
        }
        else if (!pOP->next && IsEqual(pOP->label.c_str(), "EINHEITSBOTSCHAFTEN") && pOP->index.size()) {
            if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->GetBotschaften().size()) {
                oVal.error("Falsche Indizierung in 'unit.einheitsbotschaften[idx]'");
                return oVal;
            }
            oVal = Value(poUnitQ->GetBotschaften()[(size_t)pOP->index[0].asLong()]);
        }
        // ************ Gruppe ************
        else if (!pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "GRUPPE")) {
            CGruppe::Ptr pG = poUnitQ->GetGruppe();
            oVal = Value(pG.get() ? 1 : 0);
        }
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "GRUPPE")) {
            CGruppe::Ptr pG = poUnitQ->GetGruppe();
            if (!pG.get()) {
                oVal.error("Es existiert kein Subobjekt 'unit.gruppe'");
                return oVal;
            }
            return _DoGruppe(pOP->next, pG);
        }
        // ************ Gegenstaende ************
        else if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "GEGENSTAENDE") && IsEqual(pOP->next->label.c_str(), "size")) {
            oVal = Value((int32_t)poUnitQ->Things().size());
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "GEGENSTAENDE") && (IsEqual(pOP->next->label.c_str(), "name") || IsEqual(pOP->next->label.c_str(), "anzahl"))) {
            if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poUnitQ->Things().size()) {
                oVal.error("Falsche Indizierung in 'unit.gegenstaende[idx]'");
                return oVal;
            }
            const CEinheit::CGegenstand* pG = &(poUnitQ->Things()[(size_t)pOP->index[0].asLong()]);
            if (IsEqual(pOP->next->label.c_str(), "name"))
                oVal = Value(pG->first);
            else if (IsEqual(pOP->next->label.c_str(), "anzahl"))
                oVal = Value((int32_t)pG->second);
        }
        else if (pOP->next && IsEqual(pOP->label.c_str(), "GEGENSTAENDE")) {
            oVal.error("Unbekanntes Subattribut fuer 'unit.gegenstaende'");
            return oVal;
        }
        else {
            if (pOP->next == 0 || (IsEqual(pOP->next->label.c_str(), "Tage") || IsEqual(pOP->next->label.c_str(), "Stufe"))) {
                if (pOP->index.empty())
                    return poUnitQ->GetValue(pOP->label, pOP->next ? pOP->next->label : std::string(""));
            }
            if (pOP->next || pOP->index.size()) {
                oVal = ((CBlockBase*)poUnitQ)->GetValue(pOP);
                /*
                                char Buff[256];
                                sprintf( Buff, "Unbekanntes Subattribut '%s'", pOP->next->label.c_str() );
                                oVal.error( Buff );
                */
            }
            else {
                oVal = Value(0);
            }
        }
    }
    else {
        oVal.error("Unbekannte Einheitennummer");
    }
    return oVal;
}

Value DoUnit(CObjectPart* poPart)
{
    //  CRegion::Einheiten::iterator i;
    //  CObjectPart* pOP;
    CEinheit* poUnit = 0;
    CEinheit* poUnitQ = 0;
    Value oVal;

    if (poPart->index.size() > 1) {
        oVal.error("Objekt UNIT unterstuetzt nur einen Index");
        return oVal;
    }

    // int i = poPart->index.size();

    //  pOP = poPart->next;

    /*
        if( pOP->next && pOP->next->index.size() )
        {
            std::string sErr = std::string("Attribut '") + pOP->label + std::string("' unterstuetzt keinen Index");
            oVal.error( sErr.c_str() );
            return oVal;
        }
    */
    poUnit = g_poCurrentUnit;
    poUnitQ = poUnit;

    if (poPart->index.size() == 1) {
        int32_t nENr = EinheitenNummer(poPart->index[0].asString());
        poUnit = g_poCurrentReport->SearchUnit(nENr, false);
        poUnitQ = poUnit;
        if (!poUnit || poUnit->GetQuality() != 10) {
            EinheitenDB::iterator edbi;
            edbi = g_coEDB.find(nENr);
            if (edbi != g_coEDB.end()) {
                poUnitQ = (*edbi).second;
                if (!poUnit)
                    poUnit = poUnitQ;
            }
        }
    }

    return _DoUnit(poPart, poUnit, poUnitQ);
}

//------------------------------------------------------------------------
// SHIP.
//     .BESCHR
//     .EFFECTS.SIZE
//     .EFFECTS[<idx>]
//     .GROESSE
//     .MAXGROESSE
//     .INSASSEN
//     .KAPAZITAET
//     .KAPITAEN
//     .KUESTE
//     .LADUNG
//     .MAXLADUNG
//     .NAME
//     .NUMMER
//     .PROZENT
//     .SCHADEN
//     .TYP
//     .<NeueCRTags>
//------------------------------------------------------------------------
Value _DoShip(CObjectPart* poPart, CRegion* pReg)
{
    //  CRegion::Einheiten::iterator i;
    CObjectPart* pOP;
    CSchiff* poShip = 0;
    Value oVal;

    // int i = poPart->index.size();

    pOP = poPart->next;

    int32_t nSNr = (int32_t)strtol(poPart->index[0].asString().c_str(), 0, g_poCurrentReport->BNrBase());

    poShip = g_poCurrentReport->GetShip(nSNr);
    if (!poShip)
        poShip = pReg->GetShip(nSNr);

    if (!pOP) {
        return poShip ? Value(1) : Value(0);
    }

    if (!poShip) {
        oVal.error("Falsche Indizierung fuer Objekt SHIP");
        return oVal;
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "EFFECTS") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)poShip->GetEffects().size());
    }
    else if (!pOP->next && IsEqual(pOP->label.c_str(), "EFFECTS") && pOP->index.size()) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poShip->GetEffects().size()) {
            oVal.error("Falsche Indizierung in 'ship.effects[idx]'");
            return oVal;
        }
        return Value(poShip->GetEffects()[(size_t)pOP->index[0].asLong()]);
    }

    if (IsEqual(pOP->label.c_str(), "Nummer"))
        oVal = Value(itoan(poShip->Nummer(), g_poCurrentReport->BNrBase()));
    else if (IsEqual(pOP->label.c_str(), "Typ"))
        oVal = Value(poShip->Typ());
    else if (IsEqual(pOP->label.c_str(), "Name"))
        oVal = Value(poShip->Name());
    else if (IsEqual(pOP->label.c_str(), "Kapitaen"))
        oVal = Value(itoan(poShip->Kapitaen(), g_poCurrentReport->ENrBase()));
    else if (IsEqual(pOP->label.c_str(), "Kueste"))
        oVal = Value(poShip->Kueste());
    else if (IsEqual(pOP->label.c_str(), "Anzahl"))
        oVal = Value(poShip->Anzahl());
    else if (IsEqual(pOP->label.c_str(), "Schaden"))
        oVal = Value(poShip->Schaden());
    else if (IsEqual(pOP->label.c_str(), "Prozent"))
        oVal = Value(poShip->Prozent());
    else if (IsEqual(pOP->label.c_str(), "Ladung"))
        oVal = Value(poShip->Ladung());
    else if (IsEqual(pOP->label.c_str(), "MaxLadung"))
        oVal = Value(poShip->MaxLadung());
    else if (IsEqual(pOP->label.c_str(), "MaxGroesse"))
        oVal = Value(poShip->MaxHolz());
    else if (IsEqual(pOP->label.c_str(), "Kapazitaet"))
        oVal = Value(poShip->Kapazitaet());
    else if (IsEqual(pOP->label.c_str(), "Insassen"))
        oVal = Value(poShip->Insassen());
    else if (IsEqual(pOP->label.c_str(), "Beschr"))
        oVal = Value(poShip->Beschreibung());
    else {
        if (pOP->next)
            oVal = ((CBlockBase*)poShip)->GetValue(pOP);
        else
            oVal = poShip->GetValue(pOP->label);
        // oVal.error("Unbekanntes Attribut von Objekt SHIP benutzt.");
    }
    return oVal;
}

Value DoShip(CObjectPart* poPart)
{
    //  CRegion::Einheiten::iterator i;
    //  CObjectPart* pOP;
    //  CSchiff* poShip = 0;
    Value oVal;

    if (!poPart->index.size() && g_poCurrentShip) {
        CObjectPart oPart;
        oPart.label = "SHIP";
        oPart.index.push_back(Value(itoan(g_poCurrentShip->Nummer(), g_poCurrentReport->BNrBase())));
        oPart.next = poPart->next;
        oVal = _DoShip(&oPart, g_poCurrentRegion);
        oPart.next = 0;
        return oVal;
    }

    if (poPart->index.size() != 1) {
        oVal.error("Objekt SHIP unterstuetzt nur einen Index");
        return oVal;
    }

    return _DoShip(poPart, g_poCurrentRegion);
}

//------------------------------------------------------------------------
// BUILDING.
//         .BESCHR
//         .BESITZER
//         .BELAGERER
//         .BONUS
//         .EFFECTS.SIZE
//         .EFFECTS[<idx>]
//         .GROESSE
//         .INSASSEN
//         .NAME
//         .NUMMER
//         .TYP
//         .UNTERHALT
//         .<NeueCRTags>
//------------------------------------------------------------------------
Value _DoBuilding(CObjectPart* poPart, CRegion* pReg)
{
    CObjectPart* pOP;
    CBauwerk* poBuilding = 0;
    Value oVal;

    pOP = poPart->next;

    int32_t nBNr = (int32_t)strtol(poPart->index[0].asString().c_str(), 0, g_poCurrentReport->BNrBase());

    poBuilding = g_poCurrentReport->GetBuilding(nBNr);
    if (!poBuilding)
        poBuilding = pReg->GetBuilding(nBNr);

    if (!pOP) {
        return poBuilding ? Value(1) : Value(0);
    }

    if (!poBuilding) {
        oVal.error("Falsche Indizierung fuer Objekt BUILDING");
        return oVal;
    }

    if (pOP->next && pOP->index.empty() && IsEqual(pOP->label.c_str(), "EFFECTS") && IsEqual(pOP->next->label.c_str(), "size")) {
        return Value((int32_t)poBuilding->GetEffects().size());
    }
    else if (!pOP->next && IsEqual(pOP->label.c_str(), "EFFECTS") && pOP->index.size()) {
        if (pOP->index.size() != 1 || pOP->index[0].asLong() < 0 || pOP->index[0].asLong() >= (int)poBuilding->GetEffects().size()) {
            oVal.error("Falsche Indizierung in 'building.effects[idx]'");
            return oVal;
        }
        return Value(poBuilding->GetEffects()[(size_t)pOP->index[0].asLong()]);
    }

    if (IsEqual(pOP->label.c_str(), "Nummer"))
        oVal = Value(itoan(poBuilding->Nummer(), g_poCurrentReport->BNrBase()));
    else if (IsEqual(pOP->label.c_str(), "Typ"))
        oVal = Value(poBuilding->Typ());
    else if (IsEqual(pOP->label.c_str(), "Name"))
        oVal = Value(poBuilding->Name());
    else if (IsEqual(pOP->label.c_str(), "Besitzer"))
        oVal = Value(itoan(poBuilding->Besitzer(), g_poCurrentReport->ENrBase()));
    else if (IsEqual(pOP->label.c_str(), "Belagerer"))
        oVal = Value(itoan(poBuilding->Belagerer(), g_poCurrentReport->ENrBase()));
    else if (IsEqual(pOP->label.c_str(), "Groesse"))
        oVal = Value(poBuilding->Groesse());
    else if (IsEqual(pOP->label.c_str(), "Unterhalt"))
        oVal = Value(poBuilding->Unterhalt());
    else if (IsEqual(pOP->label.c_str(), "Insassen"))
        oVal = Value(poBuilding->Insassen());
    else if (IsEqual(pOP->label.c_str(), "Beschr"))
        oVal = Value(poBuilding->Beschreibung());
    else if (IsEqual(pOP->label.c_str(), "Bonus")) {
        if (IsEqual(poBuilding->Typ().c_str(), "Burg") || CBurgInfo::Lookup(poBuilding->Typ()).GetValue("Groesse").asLong()) {
            oVal = Value(CBurgInfo::Lookup(poBuilding->Typ()).GetValue("Bonus").asLong());
        }
        else
            oVal = Value(0);
    }
    else {
        if (pOP->next)
            oVal = ((CBlockBase*)poBuilding)->GetValue(pOP);
        else
            oVal = poBuilding->GetValue(pOP->label);
        // oVal.error("Unbekanntes Attribut von Objekt SHIP benutzt.");
    }
    return oVal;
}

Value DoBuilding(CObjectPart* poPart)
{
    //  CRegion::Einheiten::iterator i;
    //  CObjectPart* pOP;
    //  CBauwerk* poBuilding = 0;
    Value oVal;

    if (!poPart->index.size() && g_poCurrentBuilding) {
        CObjectPart oPart;
        oPart.label = "BUILDING";
        oPart.index.push_back(Value(itoan(g_poCurrentBuilding->Nummer(), g_poCurrentReport->BNrBase())));
        oPart.next = poPart->next;
        oVal = _DoBuilding(&oPart, g_poCurrentRegion);
        oPart.next = 0;
        return oVal;
    }

    if (poPart->index.size() != 1) {
        oVal.error("Objekt BUILDING unterstuetzt nur einen Index");
        return oVal;
    }

    return _DoBuilding(poPart, g_poCurrentRegion);

    /*
        int i = poPart->index.size();

        pOP = poPart->next;

        int32_t nBNr = strtol( poPart->index[0].asString().c_str(), 0, g_poCurrentReport->BNrBase() );

        poBuilding = g_poCurrentRegion->GetBuilding( nBNr );

        if( !pOP )
        {
            return poBuilding ? Value( 1 ) : Value( 0 );
        }

        if( !poBuilding )
        {
            oVal.error( "Falsche Indizierung fuer Objekt BUILDING" );
            return oVal;
        }

        if( IsEqual( pOP->label.c_str(), "Nummer" ) )
            oVal = Value( itoan( poBuilding->Nummer(), g_poCurrentReport->BNrBase() ) );
        else if( IsEqual( pOP->label.c_str(), "Typ" ) )
                oVal = Value( poBuilding->Typ() );
        else if( IsEqual( pOP->label.c_str(), "Name" ) )
                oVal = Value( poBuilding->Name() );
        else if( IsEqual( pOP->label.c_str(), "Besitzer" ) )
                oVal = Value( itoan( poBuilding->Besitzer(), g_poCurrentReport->ENrBase() ) );
        else if( IsEqual( pOP->label.c_str(), "Belagerer" ) )
                oVal = Value( itoan( poBuilding->Belagerer(), g_poCurrentReport->ENrBase() ) );
        else if( IsEqual( pOP->label.c_str(), "Groesse" ) )
                oVal = Value( poBuilding->Groesse() );
        else if( IsEqual( pOP->label.c_str(), "Unterhalt" ) )
                oVal = Value( poBuilding->Unterhalt() );
        else if( IsEqual( pOP->label.c_str(), "Insassen" ) )
                oVal = Value( poBuilding->Insassen() );
        else
        {
            oVal = poBuilding->GetValue( pOP->label );
            // oVal.error("Unbekanntes Attribut von Objekt SHIP benutzt.");
        }
        return oVal;
    */
}

Value FGetUnitOfRegion(Expression* poContext, ArgumentList& coArgs)
{
    CRegion::VEinheiten* pVE;
    //  CEinheit* poUnit;
    Value oVal;

    pVE = &g_poCurrentRegion->GetVEinheiten();
    if (coArgs[0].getType() != VT_INT) {
        oVal.error("Falscher Parameter fuer 'localunit(idx)'!");
        return oVal;
    }
    if (coArgs[0].asLong() >= (int)pVE->size()) {
        oVal.error("Index ausserhalb des gueltigen Bereichen fuer 'localunit(idx)'!");
        return oVal;
    }
    oVal = Value(itoan((*pVE)[(size_t)coArgs[0].asLong()]->Nummer(), g_poCurrentReport->ENrBase()));
    return oVal;
}

Value FRandom(Expression* poContext, ArgumentList& coArgs)
{
    if (!coArgs.empty()) {
        Value oVal;
        oVal.error("Unerwartete(r) Parameter fuer 'random()'!");
        return oVal;
    }
    return Value(Random());
}

Value FEquals(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'equals(val1,val2)'!");
        return oVal;
    }
    return Value(IsEqual(coArgs[0].asString().c_str(), coArgs[1].asString().c_str()) ? 1 : 0);
}

Value FMatch(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'match(val,regx)'!");
        return oVal;
    }
    if (coArgs[1].asString().empty()) {
        Value oVal;
        oVal.error("Leerer regulaerer Ausdruck fuer 'match(val,regx)'!");
        return oVal;
    }

    CRegExp oRE;
    std::string sText = coArgs[0].asString();
    bool rc;
    if (oRE.Prepare(coArgs[1].asString())) {
        rc = oRE.Find(sText, 0);
    }
    else {
        rc = false;
    }
    Value oVal;
    oVal = Value(oRE.SubStr(sText));
    poContext->setValue("$&", &oVal);
    oVal = Value(oRE.Begin() < 0 ? std::string() : sText.substr(0, (size_t)oRE.Begin()));
    poContext->setValue("$`", &oVal);
    oVal = Value(oRE.Begin() < 0 ? std::string() : sText.substr((size_t)(oRE.Begin() + oRE.Size())));
    poContext->setValue("$\xB4", &oVal);
    oVal = Value(oRE.SubStr(sText, oRE.Count() - 1));
    poContext->setValue("$+", &oVal);
    for (int32_t spc = 1; spc < oRE.Count(); spc++) {
        oVal = Value(oRE.SubStr(sText, spc));
        poContext->setValue((std::string("$") + ToString(spc)).c_str(), &oVal);
    }

    return Value(rc ? 1 : 0);
}

Value FBefore(Expression* poContext, ArgumentList& coArgs)
{
    CRegExp oRE;
    std::string sText;

    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'before(val,regx)'!");
        return oVal;
    }
    if (coArgs[1].asString().empty()) {
        Value oVal;
        oVal.error("Leerer regulaerer Ausdruck fuer 'before(val,regx)'!");
        return oVal;
    }

    sText = coArgs[0].asString();

    if (oRE.Prepare(coArgs[1].asString())) {
        if (oRE.Find(sText, 0))
            sText.erase((size_t)oRE.Begin());
    }
    else {
        Value oVal;
        oVal.error("Fehlerhafter regulaerer Ausdruck fuer 'before(val,regx)'!");
        return oVal;
    }

    return Value(sText);
}

Value FAfter(Expression* poContext, ArgumentList& coArgs)
{
    CRegExp oRE;
    std::string sText;

    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'after(val,regx)'!");
        return oVal;
    }
    if (coArgs[1].asString().empty()) {
        Value oVal;
        oVal.error("Leerer regulaerer Ausdruck fuer 'after(val,regx)'!");
        return oVal;
    }

    sText = coArgs[0].asString();

    if (oRE.Prepare(coArgs[1].asString())) {
        if (oRE.Find(sText, 0))
            sText.erase(0, (size_t)oRE.End());
        else
            sText = "";
    }
    else {
        Value oVal;
        oVal.error("Fehlerhafter regulaerer Ausdruck fuer 'after(val,regx)'!");
        return oVal;
    }

    return Value(sText);
}

Value FCrop(Expression* poContext, ArgumentList& coArgs)
{
    CRegExp oRE;
    std::string sText;

    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'crop(val,regx)'!");
        return oVal;
    }
    if (coArgs[1].asString().empty()) {
        Value oVal;
        oVal.error("Leerer regulaerer Ausdruck fuer 'crop(val,regx)'!");
        return oVal;
    }

    sText = coArgs[0].asString();

    if (oRE.Prepare(coArgs[1].asString())) {
        if (oRE.Find(sText, 0))
            sText = sText.substr((size_t)oRE.Begin(), (size_t)oRE.Size());
        else
            sText = "";
    }
    else {
        Value oVal;
        oVal.error("Fehlerhafter regulaerer Ausdruck fuer 'crop(val,regx)'!");
        return oVal;
    }

    return Value(sText);
}

Value FChange(Expression* poContext, ArgumentList& coArgs)
{
    std::string sText;

    if (coArgs.size() != 3) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'change(val,regx,rep)'!");
        return oVal;
    }
    if (coArgs[1].asString().empty()) {
        Value oVal;
        oVal.error("Leerer regulaerer Ausdruck fuer 'change(val,regx,rep)'!");
        return oVal;
    }

    sText = coArgs[0].asString();
    if (!CRegExp::RuledReplace(sText, coArgs[1].asString(), coArgs[2].asString())) {
        Value oVal;
        oVal.error("Fehlerhafter regulaerer Ausdruck fuer 'change(val,regx,rep)'!");
        return oVal;
    }

    return Value(sText);
}

Value FSubStr(Expression* poContext, ArgumentList& coArgs)
{
    std::string sText;

    if (coArgs.size() != 3) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'substr(val,pos,num)'!");
        return oVal;
    }
    sText = coArgs[0].asString();
    int nPos = coArgs[1].asLong();
    int nLen = coArgs[2].asLong();

    if (nPos < 0) {
        nPos = (int32_t)sText.length() + nPos;
        if (nPos < 0)
            nPos = 0;
    }
    if (nLen < 0) {
        nLen = (int32_t)sText.length() + nLen;
        if (nLen < 0)
            nLen = 0;
    }
    if (nPos >= (int)sText.length())
        return Value("");

    if (nPos + nLen > (int)sText.length()) {
        nLen = (int32_t)sText.length() - nPos;
    }

    return Value(sText.substr((size_t)nPos, (size_t)nLen));
}

Value FCeil(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'ceil(val)'!");
        return oVal;
    }
    return Value((int32_t)ceil(coArgs[0].asReal()));
}

Value FFloor(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'floor(val)'!");
        return oVal;
    }
    return Value((int32_t)floor(coArgs[0].asReal()));
}

Value FAbs(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'abs(val)'!");
        return oVal;
    }
    return Value(fabs(coArgs[0].asReal()));
}

Value FSign(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'sign(val)'!");
        return oVal;
    }
    if (coArgs[0].asReal() < 0) {
        return Value(-1);
    }
    else if (coArgs[0].asReal() > 0) {
        return Value(1);
    }
    else {
        return Value(0);
    }
}

Value FSqrt(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'sqrt(val)'!");
        return oVal;
    }
    double val = coArgs[0].asReal();
    if (val >= 0) {
        return Value(sqrt(coArgs[0].asReal()));
    }
    else {
        Value oVal;
        oVal.error("Negativer Parameter fuer 'sqrt(val)'!");
        return oVal;
    }
}

Value FExp(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'exp(val)'!");
        return oVal;
    }
    return Value(exp(coArgs[0].asReal()));
}

Value FLog(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'log(val)'!");
        return oVal;
    }
    double val = coArgs[0].asReal();
    if (val > 0) {
        return Value(log(val));
    }
    else {
        Value oVal;
        oVal.error("Negativer Parameter fuer 'log(val)'!");
        return oVal;
    }
}

Value FLog10(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'log10(val)'!");
        return oVal;
    }
    double val = coArgs[0].asReal();
    if (val > 0) {
        return Value(log10(val));
    }
    else {
        Value oVal;
        oVal.error("Negativer Parameter fuer 'log10(val)'!");
        return oVal;
    }
}

Value FFloat(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'float(val)'!");
        return oVal;
    }
    return Value(coArgs[0].asReal());
}

Value FInt(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'int(val)'!");
        return oVal;
    }
    return Value(coArgs[0].asLong());
}

Value FIsNothing(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'isempty(val)'!");
        return oVal;
    }
    return Value(coArgs[0].getType() == VT_EMPTY ? 1 : 0);
}

Value FLength(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'length(val)'!");
        return oVal;
    }
    return Value(coArgs[0].getType() == VT_EMPTY ? 0 : (int32_t)(coArgs[0].asString().length()));
}

Value FFlatten(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'flatten(val)'!");
        return oVal;
    }
    return Value(Flatten(coArgs[0].asString()));
}

Value FItoan(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'itoan(val,base)'!");
        return oVal;
    }
    if (coArgs[1].asLong() < 2 || coArgs[1].asLong() > 36) {
        Value oVal;
        oVal.error("Falsche Basis fuer 'itoan(val,base)'!");
        return oVal;
    }
    return Value(std::string(itoan(coArgs[0].asLong(), coArgs[1].asLong())));
}

Value FAntoi(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'antoi(val,base)'!");
        return oVal;
    }
    if (coArgs[1].asLong() < 2 || coArgs[1].asLong() > 36) {
        Value oVal;
        oVal.error("Falsche Basis fuer 'antoi(val,base)'!");
        return oVal;
    }
    return Value((int32_t)strtol(coArgs[0].asString().c_str(), 0, coArgs[1].asLong()));
}

Value FXName(Expression* poContext, ArgumentList& coArgs)
{
    static std::set<std::string> coNames;
    CRNENode* poRNE;
    Value oVal;
    int32_t nRep = 10;

    if (coArgs.size() < 1 || coArgs.size() > 2) {
        oVal.error("Falsche Parameterzahl fuer 'xname(rne,maxrep)'!");
        return oVal;
    }

    if (coArgs[1].asLong() > 0)
        nRep = coArgs[1].asLong();

    if (coNames.empty()) {
        EinheitenDB::iterator ei;
        for (ei = g_coEDB.begin(); ei != g_coEDB.end(); ei++) {
            coNames.insert((*ei).second->Name());
        }
        RegionDB::iterator ri;
        for (ri = g_coRDB.begin(); ri != g_coRDB.end(); ri++) {
            coNames.insert((*(*ri).second.begin())->GetName());
        }
    }

    if (coArgs[0].asString().empty()) {
        oVal.error("Fehlende Regelangabe fuer 'xname(rne,maxrep)'!");
        return oVal;
    }

    try {
        poRNE = CRNENode::CreateFromString(coArgs[0].asString());
    }
    catch (CRNEException e) {
        oVal.error((std::string("Fehler in Namensausdruck beim Aufruf von xname(rne,maxrep): ") + e.why()).c_str());
        return oVal;
    }

    if (poRNE) {
        std::string sStr;
        for (int i = 0; i < nRep; i++) {
            sStr = poRNE->GenAVal();
            if (coNames.find(sStr) == coNames.end()) {
                coNames.insert(sStr);
                break;
            }
        }
        oVal = Value(sStr);
        delete poRNE;
    }
    return oVal;
}

static int myTolower(int c)
{
    switch (c) {
        case 0xC4:
            return 0xE4;
        case 0xD6:
            return 0xF6;
        case 0xDC:
            return 0xFC;
    }
    return tolower(c);
}

static int myToupper(int c)
{
    switch (c) {
        case 0xE4:
            return 0xC4;
        case 0xF6:
            return 0xD6;
        case 0xFC:
            return 0xDC;
    }
    return toupper(c);
}

Value FToLower(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'tolower(val)'!");
        return oVal;
    }

    std::string sTxt = coArgs[0].asString();
    std::transform(sTxt.begin(), sTxt.end(), sTxt.begin(), static_cast<int (*)(int)>(myTolower));

    return Value(sTxt);
}

Value FToUpper(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'tolower(val)'!");
        return oVal;
    }

    std::string sTxt = coArgs[0].asString();
    std::transform(sTxt.begin(), sTxt.end(), sTxt.begin(), static_cast<int (*)(int)>(myToupper));

    return Value(sTxt);
}

Value FTime(Expression* poContext, ArgumentList& coArgs)
{
    if (!coArgs.empty()) {
        Value oVal;
        oVal.error("Unerwartete(r) Parameter fuer 'time()'!");
        return oVal;
    }
    return Value(int32_t(double(clock() - (uint32_t)g_nTimeCorrection) / CLOCKS_PER_SEC * 1000));
}

Value FAnd(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'and(val1,val2)'!");
        return oVal;
    }
    return Value(coArgs[0].asLong() & coArgs[1].asLong());
}

Value FOr(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'or(val1,val2)'!");
        return oVal;
    }
    return Value(coArgs[0].asLong() | coArgs[1].asLong());
}

Value FXor(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 2) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'xor(val1,val2)'!");
        return oVal;
    }
    return Value(coArgs[0].asLong() ^ coArgs[1].asLong());
}

Value FNot(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'not(val)'!");
        return oVal;
    }
    return Value(~coArgs[0].asLong());
}

Value FTypeOf(Expression* poContext, ArgumentList& coArgs)
{
    if (coArgs.size() != 1) {
        Value oVal;
        oVal.error("Falsche Parameterzahl fuer 'typeof(exp)'!");
        return oVal;
    }
    return Value(int32_t(coArgs[0].getType()));
}

Value Fgv(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    if (coArgs.size() != 4) {
        oVal.error("Falsche Parameterzahl fuer '_gv_(,,,)'!");
        return oVal;
    }

    std::string sText = Flatten(coArgs[3].asString());
    if (!coArgs[2].asString().empty())
        sText += coArgs[2].asString();
    uint32_t nAusweis = Hash((const ub1*)(sText.c_str()), (uint32_t)sText.length(), (uint32_t)coArgs[0].asLong());
    int nUntil = coArgs[1].asLong();
    sText = itoan((int32_t)nAusweis, 36);
    if (sText.length() > 4)
        sText = sText.substr(sText.length() - 4);
    while (sText.length() < 4)
        sText = std::string("0") + sText;
    std::string sUntil = itoan(nUntil > 0 ? nUntil : 0, 36);
    if (sUntil.length() < 2)
        sUntil = std::string("0") + sUntil;
    sText = std::string("(") + sUntil + sText;

    if (coArgs[2].asString().empty())
        sText = std::string("G") + sText;
    else
        sText = std::string("L") + sText;
    if (nUntil < 0)
        sText = std::string("P") + sText;
    else
        sText = std::string("V") + sText;

    uint32_t nChk = Hash((const ub1*)(sText.c_str()), (uint32_t)sText.length(), (uint32_t)coArgs[0].asLong());
    std::string sChk = itoan((int32_t)nChk, 36);
    if (sChk.length() > 2)
        sChk = sChk.substr(sChk.length() - 2);
    if (sChk.length() < 2)
        sChk = std::string("0") + sChk;
    sText += sChk + ")";
    return Value(sText);
}

Value Fcv(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    if (coArgs.size() != 4) {
        oVal.error("Falsche Parameterzahl fuer '_cv_(,,,)'!");
        return oVal;
    }

    std::string sAusweis = coArgs[1].asString();
    std::string sText = Flatten(coArgs[3].asString());
    if (sAusweis.length() > 2 && sAusweis[1] == 'L')
        sText += coArgs[2].asString();
    uint32_t nAusweis = Hash((const ub1*)(sText.c_str()), (uint32_t)sText.length(), (uint32_t)coArgs[0].asLong());
    sText = itoan((int32_t)nAusweis, 36);
    if (sText.length() > 4)
        sText = sText.substr(sText.length() - 4);
    while (sText.length() < 4)
        sText = std::string("0") + sText;
    if (sAusweis.length() < 12 && sText != sAusweis.substr(5, 4))
        return Value(4);
    uint32_t nChk = Hash((const ub1*)(sAusweis.c_str()), 9, (uint32_t)coArgs[0].asLong());
    std::string sChk = itoan((int32_t)nChk, 36);
    if (sChk.length() > 2)
        sChk = sChk.substr(sChk.length() - 2);
    if (sChk.length() < 2)
        sChk = std::string("0") + sChk;
    if (sChk != sAusweis.substr(9, 2))
        return Value(3);
    int32_t nUntil = (int32_t)strtol(sAusweis.substr(3, 2).c_str(), 0, 36);
    if (nUntil && nUntil < g_poCurrentReport->Runde())
        return Value(2);
    if (nUntil == g_poCurrentReport->Runde())
        return Value(1);

    return Value(0);
}

namespace GF {
enum GF_MODE { enREAD, enWRITE, enAPPEND, enPIPE };

enum GF_STATE { enOK, enEOF = -1, enERROR = 1 };
}  // namespace GF

struct FileInfo
{
    std::string sName;
    std::string sMessage;
    FILE* hFile;
    int nState;
    int nMode;
};

int32_t g_nHandleCount;
std::map<int32_t, FileInfo> g_coOpenFiles;

static FileInfo* GetFileInfo(int32_t nHandle)
{
    std::map<int32_t, FileInfo>::iterator fi = g_coOpenFiles.find(nHandle);
    if (fi == g_coOpenFiles.end())
        return 0;
    return &((*fi).second);
}

#define RL2SBUFFSIZE 512

static bool ReadLine2String(FILE* hFile, std::string& sLine)
{
    char Buff[RL2SBUFFSIZE];
    char* rc;
    sLine = "";
    do {
        Buff[0] = 0;
        rc = fgets(Buff, 512, hFile);
        sLine += Buff;
        if (feof(hFile))
            break;
    } while (strlen(Buff) == RL2SBUFFSIZE - 1 && (Buff[RL2SBUFFSIZE - 2] != 10 || Buff[RL2SBUFFSIZE - 2] != 13));
    while (!sLine.empty() && (sLine[sLine.size() - 1] == 10 || sLine[sLine.size() - 1] == 13))
        sLine.erase(sLine.size() - 1, 1);
    return rc != 0;
}

void CloseAllOpenFiles()
{
    std::map<int32_t, FileInfo>::iterator i = g_coOpenFiles.begin();
    while (i != g_coOpenFiles.end()) {
        if (i->second.hFile) {
            if (i->second.nMode == GF::enPIPE) {
#ifdef _WIN32
                i->second.sMessage = ToString((int32_t)_pclose(i->second.hFile));
#else
                i->second.sMessage = ToString((int32_t)pclose(i->second.hFile));
#endif
            }
            else {
                fclose(i->second.hFile);
            }
            i->second.hFile = 0;
        }
        i++;
    }
}

Value FSystem(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    if (coArgs.size() != 1) {
        oVal.error("Falsche Parameterzahl fuer 'system(command)'!");
        return oVal;
    }
    g_nHandleCount++;
    FileInfo* pfi = &(g_coOpenFiles[g_nHandleCount]);
    pfi->sName = coArgs[0].asString();
    pfi->nMode = GF::enPIPE;
    pfi->nState = 0;
#ifdef _WIN32
    pfi->hFile = _popen(pfi->sName.c_str(), "rt");
#else
    pfi->hFile = popen(pfi->sName.c_str(), "r");
#endif
    if (!pfi->hFile) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = std::string("Couldn't execute command '") + pfi->sName + "'";
    }
    return Value(g_nHandleCount);
}

Value FOpen(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    if (coArgs.size() != 2) {
        oVal.error("Falsche Parameterzahl fuer 'open(filename,mode)'!");
        return oVal;
    }
    g_nHandleCount++;
    FileInfo* pfi = &(g_coOpenFiles[g_nHandleCount]);
    pfi->sName = coArgs[0].asString();
    pfi->nMode = coArgs[1].asLong();
    pfi->nState = 0;
    if (pfi->nMode == GF::enREAD) {
        pfi->hFile = fopen(pfi->sName.c_str(), "r");
    }
    else if (pfi->nMode == GF::enWRITE) {
        pfi->hFile = fopen(pfi->sName.c_str(), "w");
    }
    else if (pfi->nMode == GF::enAPPEND) {
        pfi->hFile = fopen(pfi->sName.c_str(), "a");
    }
    else {
        oVal.error("Falscher Modus fuer 'open(filename,mode)'!");
        return oVal;
    }
    if (!pfi->hFile) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = std::string("Couldn't open file '") + pfi->sName + "'";
    }
    return Value(g_nHandleCount);
}

Value FClose(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Filehandle!");
        return oVal;
    }
    pfi->nState = 0;
    pfi->sMessage = "";
    if (!pfi || !pfi->hFile) {
        return Value(0);
    }
    if (pfi->nMode == GF::enPIPE) {
#ifdef _WIN32
        pfi->sMessage = ToString((int32_t)_pclose(pfi->hFile));
#else
        pfi->sMessage = ToString((int32_t)pclose(pfi->hFile));
#endif
    }
    else {
        fclose(pfi->hFile);
    }
    pfi->hFile = 0;
    return Value(0);
}

Value FSeek(Expression* poContext, ArgumentList& coArgs);
Value FRead(Expression* poContext, ArgumentList& coArgs);
Value FWrite(Expression* poContext, ArgumentList& coArgs);

Value FReadLine(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Datei-Handle!");
        return oVal;
    }
    pfi->nState = GF::enOK;
    pfi->sMessage = "";
    if (!pfi || !pfi->hFile) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = "Datei ist nicht geoeffnet!";
        return Value("");
    }
    std::string sLine;
    bool ok = ReadLine2String(pfi->hFile, sLine);
    if (!ok) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = strerror(errno);
    }
    if (feof(pfi->hFile)) {
        pfi->nState = GF::enEOF;
        pfi->sMessage = "";
    }
    return Value(sLine);
}

Value FWriteLine(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Datei-Handle!");
        return Value(0);
    }
    pfi->nState = GF::enOK;
    pfi->sMessage = "";
    if (!pfi || !pfi->hFile) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = "Datei ist nicht geoeffnet!";
        return Value(0);
    }
    int rc = fprintf(pfi->hFile, "%s\n", coArgs[1].asString().c_str());
    if (rc < 0) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = strerror(errno);
    }
    return Value((int32_t)(pfi->nState == GF::enOK ? coArgs[1].asString().length() + 1 : 0));
}

static std::string EscapeXML(const std::string& sText)
{
    std::string sTxt(sText);
    CRegExp::Replace(sTxt, "&", "&amp;");
    CRegExp::Replace(sTxt, "<", "&lt;");
    CRegExp::Replace(sTxt, ">", "&gt;");
    return sTxt;
}

/*
static std::string UnescapeXML( const std::string& sText )
{
    std::string sTxt( sText );
    CRegExp::Replace( sTxt, "&lt;","<" );
    CRegExp::Replace( sTxt, "&gt;",">" );
    CRegExp::Replace( sTxt, "$amp;","&" );
    return sTxt;
}
*/

char cBuff = 0;

static char GetChar(FILE* pStream)
{
    char c;
    if (cBuff) {
        c = cBuff;
        cBuff = 0;
        return c;
    }
    return (char)fgetc(pStream);
}

static void UngetChar(char c)
{
    cBuff = c;
}

static std::string ReadXMLTag(FILE* pStream, std::string* pID = 0)
{
    // Tag-Start suchen
    std::string sTag;
    char c;
    while ((c = GetChar(pStream)) != '<' && !ferror(pStream) && !feof(pStream)) {
    }
    do {
        c = GetChar(pStream);
        if (IsAlpha(c) || c == '/')
            sTag += c;
        else
            break;
    } while (!ferror(pStream) && !ferror(pStream) && !feof(pStream));
    if (c == '>')
        UngetChar(c);
    else if (IsSpace(c) && pID) {
        int q = 0;
        *pID = "";
        do {
            c = GetChar(pStream);
            if (c == '\"')
                q++;
            if (q == 1 && c != '\"')
                *pID += c;
        } while (c != '>' && !ferror(pStream) && !feof(pStream));
        if (c == '>')
            UngetChar(c);
    }
    while ((c = GetChar(pStream)) != '>' && !ferror(pStream) && !feof(pStream)) {
    }
    return sTag;
}

static std::string ReadXMLValue(FILE* pStream)
{
    std::string sVal;
    char c;
    do {
        c = GetChar(pStream);
        if (c != '<')
            sVal += c;
    } while (c != '<' && !ferror(pStream) && !feof(pStream));
    if (c == '<')
        UngetChar(c);
    return sVal;
}

static bool ReadValueXML(FILE* pStream, Value& oVal)
{
    static std::string sLastTag;
    std::string sTag = ReadXMLTag(pStream);
    if (!sTag.empty()) {
        switch (sTag[0]) {
            case 'n':
                oVal = Value();
                break;
            case 'e':
                oVal.error(ReadXMLValue(pStream).c_str());
                ReadXMLTag(pStream);
                break;
            case 'i':
                oVal = Value((int32_t)strtol(ReadXMLValue(pStream).c_str(), NULL, 10));
                ReadXMLTag(pStream);
                break;
            case 'f':
                oVal = Value((double)strtod(ReadXMLValue(pStream).c_str(), NULL));
                ReadXMLTag(pStream);
                break;
            case 's':
                oVal = Value(ReadXMLValue(pStream));
                ReadXMLTag(pStream);
                break;
            case 'a': {
                Value vT;
                oVal = Value(VT_VECTOR);
                while (ReadValueXML(pStream, vT)) {
                    oVal.setAt(Value(oVal.size()), vT);
                }
            } break;
            case 'd': {
                std::string sT2, sID;
                Value vT;
                oVal = Value(VT_MAP);
                while (true) {
                    sT2 = ReadXMLTag(pStream, &sID);
                    if (sT2 == "pair") {
                        if (ReadValueXML(pStream, vT))
                            oVal.setAt(Value(sID), vT);
                        ReadXMLTag(pStream);
                    }
                    else if (sT2 == "/dic") {
                        break;
                    }
                }
            } break;
            case '/':
                sLastTag = sTag;
                oVal = Value();
                return false;
        }
    }
    return true;
}

static int32_t WriteValueXML(FILE* pStream, const Value& oVal, int32_t indent = 0)
{
    static std::string sSpace("                                                            ");
    if (indent > 58)
        indent = 58;

    fprintf(pStream, "%s", sSpace.substr(0, (size_t)indent).c_str());

    switch (oVal.cself().getType()) {
        case VT_EMPTY:
            fprintf(pStream, "<non/>\n");
            break;
        case VT_ERROR:
            fprintf(pStream, "<err>%s</err>\n", EscapeXML(oVal.asString()).c_str());
            break;
        case VT_INT:
            fprintf(pStream, "<int>%ld</int>\n", oVal.asLong());
            break;
        case VT_FLOAT:
            fprintf(pStream, "<flt>%.3f</flt>\n", oVal.asReal());
            break;
        case VT_STRING:
            fprintf(pStream, "<str>%s</str>\n", EscapeXML(oVal.asString()).c_str());
            break;
        case VT_VECTOR: {
            fprintf(pStream, "<arr>\n");
            for (int32_t i = 0; i < oVal.size(); i++) {
                WriteValueXML(pStream, oVal.getAt(Value(i)), indent + 1);
            }
            fprintf(pStream, "%s</arr>\n", sSpace.substr(0, (size_t)indent).c_str());
        } break;
        case VT_MAP: {
            fprintf(pStream, "<dic>\n");
            for (int32_t i = 0; i < oVal.size(); i++) {
                fprintf(pStream, "%s<pair id=\"%s\">\n", sSpace.substr(0, (size_t)indent + 1).c_str(), EscapeXML(oVal.getNth(i).asString()).c_str());
                WriteValueXML(pStream, oVal.getAt(oVal.getNth(i)), indent + 2);
                fprintf(pStream, "%s</pair>\n", sSpace.substr(0, (size_t)indent + 1).c_str());
            }
            fprintf(pStream, "%s</dic>\n", sSpace.substr(0, (size_t)indent).c_str());
        } break;
        case VT_REF:
            // TODO: Missing support
            break;
    }
    return 1;
}

Value FReadValue(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Datei-Handle!");
        return oVal;
    }
    pfi->nState = GF::enOK;
    pfi->sMessage = "";
    if (!pfi || !pfi->hFile) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = "Datei ist nicht geoeffnet!";
        return Value("");
    }
    if (!ReadValueXML(pfi->hFile, oVal)) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = "Value konnte nicht aus XML-Strom gelesen werden!";
        oVal.error(pfi->sMessage.c_str());
    }
    if (ferror(pfi->hFile)) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = strerror(errno);
        oVal.error(pfi->sMessage.c_str());
    }
    return oVal;
}

Value FWriteValue(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Datei-Handle!");
        return oVal;
    }
    pfi->nState = GF::enOK;
    pfi->sMessage = "";
    if (!pfi || !pfi->hFile) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = "Datei ist nicht geoeffnet!";
        return Value("");
    }
    int32_t rc = WriteValueXML(pfi->hFile, coArgs[1]);
    if (ferror(pfi->hFile)) {
        pfi->nState = GF::enERROR;
        pfi->sMessage = strerror(errno);
    }
    return Value((int32_t)(pfi->nState == GF::enOK ? rc : 0));
}

Value FStatus(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Datei-Handle!");
        return oVal;
    }
    return Value((int32_t)(pfi->nState));
}

Value FStatusText(Expression* poContext, ArgumentList& coArgs)
{
    Value oVal;
    FileInfo* pfi = GetFileInfo(coArgs[0].asLong());
    if (!pfi) {
        oVal.error("Unbekanntes Datei-Handle!");
        return oVal;
    }
    return Value(pfi->sMessage);
}

static VKommandos _dummy;
static VKommandos* _pcoCmd = &_dummy;

bool ExistUserFunction(const std::string& sName)
{
    CMetaCommand* pMC;
    pMC = g_oScriptBase.FindProc(sName);
    if (!pMC)
        return false;
    if (!pMC->IsFunction())
        return false;
    return true;
}

bool DoUserFunction(const std::string& sName, ArgumentList& coArgs, Value* poVal)
{
    std::vector<CReference*> coRefs;
    Expression::Variables oVars;
    //  VKommandos* pcoCmd;
    CMetaCommand* pMC;
    std::string sPName;
    std::string sAName;
    std::string sStackInfo;
    int nPArgs = 0, argi = 1;
    size_t nTC = 0;

    pMC = g_oScriptBase.FindProc(sName);
    if (!pMC)
        return false;

    sStackInfo = std::string("#func ") + sName;
    if (!pMC->IsFunction()) {
        poVal->error(fmt::format("Die Prozedur '#proc {}' kann nicht als Funktion aufgerufen werden!", sName));
        return false;
    }

    nPArgs = atoi((*pMC)[0].c_str()) - 1;
    coRefs.resize((size_t)nPArgs, 0);

    while (nTC < coArgs.size()) {
        argi++;
        oVars[std::string("#ARG") + ToString((int32_t)(argi - 1))] = coArgs[nTC];
        sStackInfo += std::string(" ") + coArgs[nTC].asString();
        if (argi - 1 <= nPArgs) {
            sAName = (*pMC)[(size_t)argi];
            oVars[sAName[0] == '&' ? sAName.substr(1) : sAName] = coArgs[nTC];
            //              coRefs[argi-2] = m_pvRef; m_pvRef = 0;
        }
        else {
            poVal->error(fmt::format("Ueberzaehliges Argument '{}' fuer Funktion '{}'!", coArgs[nTC].asString().c_str(), sName));
            return false;
        }
        nTC++;
    }
    oVars[std::string("#ARG0")] = Value((int32_t)(argi - 1));
    if (argi - 1 == nPArgs) {
        //          printf( " ; Call to procedure: %s\n", sPName.c_str() );
        //          for( Expression::Variables::iterator evi = oVars.begin(); evi != oVars.end(); evi++ )
        //          {
        //              printf( " ;    %s = %s\n", (*evi).first.c_str(), (*evi).second.asString().c_str() );
        //          }
        // CALL
        oVars[std::string("$RETURN")] = Value();
        CMCI oMCI(nPArgs + 2);
        g_coCallStack.push_back(sStackInfo);
        try {
            pMC->SubBlock(oMCI, oVars, true, 0, *_pcoCmd, nPArgs + 2);
        }
        catch (CReturnException oRX) {
            oVars[std::string("$RETURN")] = oRX.m_oVal;
            if (g_poStepOut == &oMCI)
                CMetaCommand::SetTrace(2);
        }
#ifdef ROCK_SOLID_CATCH
        catch (...) {
            poVal->error(fmt::format("Unerwartete Ausnahmebehandlung in Funktion '{}'!", sName));
            if (g_poStepOut == &oMCI)
                CMetaCommand::SetTrace(2);
            g_coCallStack.pop_back();
            return false;
        }
#endif
        g_coCallStack.pop_back();
        *poVal = oVars[std::string("$RETURN")];
        return true;
        /*
                for( int ih=0; ih<nPArgs; ih++ )
                {
                    if( (*pMC)[ih+2][0]=='&' )
                    {
                        if( coRefs[ih] )
                        {
                            *(coRefs[ih]) = oVars[(*pMC)[ih+2].substr(1)];
                            delete coRefs[ih];
                        }
                        else
                        {
                            (*this)[nPos+ih] = oVars[(*pMC)[ih+2].substr(1)].asString();
                        }
                    }
                }
                if( psCom )
                    *psCom = AsString();
        */
    }
    else {
        if (argi - 1 < nPArgs) {
            poVal->error(fmt::format("Zu wenig Argumente fuer Funktion '{}'!", sName));
            return false;
        }
    }
    return false;
}

static std::string Quotionmarks(std::string& sText)
{
    std::string t;
    char c;
    for (size_t i = 0; i < sText.size(); i++) {
        c = sText[i];
        switch (c) {
                /*
                        case '\\':
                            i++;
                            if( i<sText.size() )
                            {
                                c = sText[i];
                                if( IsDigit( c ) && i+2<sText.size() && IsDigit( sText[i+1] ) && IsDigit( sText[i+2] ) )
                                {
                                    t += (char)( (c-'0')*100 + (sText[i+1]-'0')*10 + (sText[i+2]-'0') );
                                    i += 2;
                                }
                                else
                                {
                                    t += c;
                                }
                            }
                            break;
                */
            case '|':
                t += '\x22';
                break;
            default:
                t += sText[i];
        }
    }
    return t;
}

bool CMetaCommand::m_bForceDeclare = false;

CMetaCommand::CMetaCommand(std::string sCom, int32_t nLine, bool bIsFunc, bool bVRef)
    : m_bIsFunc(bIsFunc)
    , m_bVRef(bVRef)
//, oMCI.m_nLine( nLine )
{
    AddScript(sCom, nLine);

    if (g_cnMetaTokens.empty()) {
        for (int i = 0; MetaTokens[i].name; i++) {
            g_cnMetaTokens[std::string(MetaTokens[i].name)] = MetaTokens[i].value;
        }
    }
}

CMetaCommand::~CMetaCommand()
{
    //  if( m_pvRef )
    //      delete m_pvRef;
}

int CMetaCommand::AddScript(std::string sCom, int32_t nLine)
{
    std::string sStr;
    size_t p;
    int nBC = 0;

    if (sCom.find("$(") != std::string::npos) {
        m_bInplace = true;
    }
    if (!m_coArgs.empty() && m_coArgs[Args()] != ":" && m_coArgs[Args()] != "{") {
        m_coArgs.push_back(std::string(":"));
        m_coLocs.push_back(LOCATION(nLine, 0));
    }

    if (!sCom.empty()) {
        p = sCom.find_first_not_of("\t ");
        if (p == std::string::npos) {
            p = sCom.size();
        }
        sCom.erase(0, p);
    }

    while (!sCom.empty() && sCom[0] != ';') {
        p = 0;
        do {
            p = sCom.find_first_of("\t :;\x27", p);
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

        if (p) {
            if (sCom[0] == ';')
                break;
            sStr = sCom.substr(0, p);
            if (sStr == "else" || sStr == "#else") {
                if (!m_coArgs.empty() && m_coArgs[Args()] == ":") {
                    m_coArgs.pop_back();
                    m_coLocs.pop_back();
                }
            }
            if (sStr == "{") {
                if (!m_coArgs.empty() && m_coArgs[Args()] == ":") {
                    m_coArgs.pop_back();
                    m_coLocs.pop_back();
                }
                nBC++;
            }
            if (sStr == "}") {
                if (!m_coArgs.empty() && m_coArgs[Args()] == ":") {
                    m_coArgs.pop_back();
                    m_coLocs.pop_back();
                }
                nBC--;
            }
            m_coArgs.push_back(sStr);
            m_coLocs.push_back(LOCATION(nLine, 0));
            sCom.erase(0, p);
        }
        if (sCom[0] == ':') {
            m_coArgs.push_back(std::string(":"));
            m_coLocs.push_back(LOCATION(nLine, 0));
            sCom.erase(0, 1);
        }

        if (!sCom.empty()) {
            p = sCom.find_first_not_of("\t ");
            if (p == std::string::npos) {
                p = sCom.size();
            }
            sCom.erase(0, p);
        }
    }

    return nBC;
}

static bool IsVariable(std::string& sStr)
{
    size_t i = 2;
    if (sStr[0] != '$')
        return false;
    if (!IsAlpha(sStr[1]) && !IsUmlaut(sStr[1]))
        return false;
    while (i < sStr.size() && (IsAlNum(sStr[i]) || IsUmlaut(sStr[i]))) {
        i++;
    }
    if (i >= sStr.size())
        return true;
    return false;
}

void CMetaCommand::Skip(CMCI& oMCI)
{
    if (oMCI.m_nTC > (int)Args())
        oMCI.m_sArg = "";
    else {
        oMCI.m_nLine = m_coLocs[(size_t)oMCI.m_nTC].first;
        oMCI.m_sArg = m_coArgs[(size_t)oMCI.m_nTC++];
    }
}

bool CMetaCommand::Parse(CMCI& oMCI, Expression::Variables& oContext, VKommandos& coCmd, bool bAlone, bool bExpand)
{
    int help = 0;
    _pcoCmd = &coCmd;

    if (oMCI.m_pvRef)
        delete oMCI.m_pvRef;
    oMCI.m_pvRef = 0;
    oMCI.m_pVal = 0;
    oMCI.m_vArg = Value();

    if (oMCI.m_nTC > (int)Args())
        oMCI.m_sArg = "";
    else {
        oMCI.m_nLine = m_coLocs[(size_t)oMCI.m_nTC].first;
        oMCI.m_sOArg = m_coArgs[(size_t)oMCI.m_nTC++];
        oMCI.m_sArg = oMCI.m_sOArg;
        if (IsVariable(oMCI.m_sArg)) {
            Expression::Variables::iterator vi;
            std::string sVName;
            sVName = oMCI.m_sArg;

            vi = oContext.find(sVName);
            if (vi != oContext.end()) {
                oMCI.m_pvRef = new CReference((*vi).second);
                oMCI.m_pVal = &(*vi).second;
                if (bExpand) {
                    oMCI.m_sArg = oMCI.m_pvRef->asString();
                    oMCI.m_vArg = oMCI.m_pvRef->self();
                }
            }
            else {
                vi = Expression::globalContext().find(sVName);
                if (vi != Expression::globalContext().end()) {
                    if (oMCI.m_pvRef)
                        delete oMCI.m_pvRef;
                    oMCI.m_pvRef = new CReference((*vi).second);
                    oMCI.m_pVal = &(*vi).second;
                    if (bExpand) {
                        oMCI.m_sArg = oMCI.m_pvRef->asString();
                        oMCI.m_vArg = oMCI.m_pvRef->self();
                    }
                }
                else {
                    //              char Buff[512];
                    //              if( m_nLine>0 )
                    //                  sprintf( Buff, "; %s(%d) : FEHLER: Unbekannte Variable '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, sVName.c_str() );
                    //              else
                    //                  sprintf( Buff, "; FEHLER: Unbekannte Variable '%s'!", sVName.c_str() );
                    //              coCmd.push_back( std::string(Buff) );
                    if (bExpand)
                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Unbekannte Variable '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, sVName.c_str()));
                }
            }
        }
        else if (!strchr("/#{}:", oMCI.m_sArg[0])) {
            //          Value oVal;
            if (bExpand) {
                Expression oExp(oMCI.m_sArg.c_str(), m_sSrcFile.c_str(), oMCI.m_nLine);
                if (!oExp.evaluate(oContext, oMCI.m_sArg.c_str(), &oMCI.m_vArg, &help, m_bForceDeclare, m_bInplace)) {
                    if (oMCI.m_vArg.getType() == VT_ERROR) {
                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Ausdruck ungueltig '%s': %s", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str(), oMCI.m_vArg.asString().c_str()));
                        m_bParseError = true;
                    }
                    else {
                        oMCI.m_sArg = oMCI.m_vArg.asString();
                        if (oExp.getContainer())
                            oMCI.m_pvRef = new CReference(*oExp.getContainer());
                    }
                }
                else {
                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: Ausdruck ungueltig '%s': %s", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str(), oMCI.m_vArg.asString().c_str()));
                    //              coCmd.push_back( std::string(Buff) );
                    //              m_sArg = std::string("?") + m_sArg + std::string("?");
                    m_bParseError = true;
                    return true;
                }
            }
            else {
                Value* pVal = Expression::getObjectReference(oContext, oMCI.m_sArg);
                if (pVal) {
                    oMCI.m_pvRef = new CReference(*pVal);
                    oMCI.m_pVal = pVal;
                }
            }
        }
    }
    if (help) {
        if (bAlone)
            return true;
        //      coCmd.push_back( std::string("; FEHLER: Zuweisung in Anweisung oder Ausdruck verwendet!" ) );
        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Zuweisung in Anweisung oder Ausdruck verwendet!", m_sSrcFile.c_str(), oMCI.m_nLine));
        return false;
    }
    return false;
}

static void TraceLinesFromFile(const std::string& sFileName, int32_t nLine, int32_t nNum = 1)
{
    std::fstream ifs;
    std::istringstream iss;
    std::istream* is = nullptr;
    std::string sLine;
    int32_t nLineNumber = 0;

    if (nLine == 0) {
        CONMSG(("<Zeile nicht verfuegbar>\n"));
        return;
    }

    if (g_pseudoFiles.count(sFileName)) {
        iss.str(g_pseudoFiles[sFileName]);
        is = &iss;
    }
    else {
        ifs.open(sFileName.c_str(), ios::in);
        if (ifs.fail()) {
            CONMSG(("Auf die Datei '%s' kann nicht zugegriffen werden!\n", sFileName.c_str()));
            return;
        }
        is = &ifs;
    }
    while (true) {
        getline(*is, sLine);
        if (is->fail()) {
            CONMSG(("<Zeile nicht verfuegbar>\n"));
            break;
        }
        nLineNumber++;
        if (nLineNumber >= nLine + nNum)
            break;
        if (nLineNumber >= nLine)
            CONMSG(("%s\n", sLine.c_str()));
    }
}

void CMetaCommand::DumpContext(Expression::Variables& oContext)
{
    Expression::Variables::iterator vi;

    for (vi = oContext.begin(); vi != oContext.end(); vi++) {
        if (!((*vi).first.empty())) {
#ifndef _DEBUG
            if ((*vi).first[0] != '#')
#endif
            {
                DumpVariable((*vi).first, (*vi).second);
            }
        }
    }
}

void CMetaCommand::DumpVariable(const std::string& sName, const Value& oVal, bool bExtendedOutput)
{
    const char* pcType;
    switch (oVal.getType()) {
        case VT_INT:
            pcType = "int";
            break;
        case VT_FLOAT:
            pcType = "flt";
            break;
        case VT_STRING:
            pcType = "str";
            break;
        case VT_ERROR:
            pcType = "err";
            break;
        case VT_REF:
            pcType = "ref";
            break;
        case VT_MAP:
            pcType = "dic";
            break;
        case VT_VECTOR:
            pcType = "arr";
            break;
        default:
            pcType = "non";
    }
    CONMSG(("%-16s (%s): %s\n", (sName[0] == '#' ? sName.substr(1).c_str() : sName.c_str()), pcType, oVal.asString().c_str()));
}

COutputTable g_oOT;

bool bFirstTrace = true;

void CMetaCommand::QuicksortHelper1(CMCI& oMCI, VKommandos& coCmd, const std::string& sArrayName, Value* pVector, int32_t l, int32_t r, const std::string& sCmpFunc, const ArgumentList& oArgs)
{
    ArgumentList::const_iterator ai;
    int32_t i, j;
    if (r > l) {
        i = l - 1;
        j = r;
        for (;;) {
            if (sCmpFunc.empty()) {
                while (pVector->getAt(Value(++i)) < pVector->getAt(Value(r)))
                    ;
                while (pVector->getAt(Value(--j)) > pVector->getAt(Value(r)))
                    ;
            }
            else {
                ArgumentList coArgs;
                Value oVErg;
                do {
                    coArgs.clear();
                    coArgs.push_back(Value(sArrayName));
                    coArgs.push_back(Value(++i));
                    coArgs.push_back(Value(r));
                    for (ai = oArgs.begin(); ai != oArgs.end(); ai++)
                        coArgs.push_back(*ai);
                    if (i >= r || i < 0)
                        break;
                    if (!DoUserFunction(sCmpFunc, coArgs, &oVErg)) {
                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Die kleiner-als-Funktion '#func %s $arrayname $i1 $i2' fuer #sort <array> <lessfunc> wurde nicht gefunden!", m_sSrcFile.c_str(), oMCI.m_nLine, sCmpFunc.c_str()));
                        return;
                    }
                } while (i < r && oVErg.asLong());
                do {
                    coArgs.clear();
                    coArgs.push_back(Value(sArrayName));
                    coArgs.push_back(Value(r));
                    coArgs.push_back(Value(--j));
                    for (ai = oArgs.begin(); ai != oArgs.end(); ai++)
                        coArgs.push_back(*ai);
                    if (r >= j || r < 0)
                        break;
                    if (!DoUserFunction(sCmpFunc, coArgs, &oVErg)) {
                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Die kleiner-als-Funktion '#func %s $arrayname $i1 $i2' fuer #sort <array> <lessfunc> wurde nicht gefunden!", m_sSrcFile.c_str(), oMCI.m_nLine, sCmpFunc.c_str()));
                        return;
                    }
                } while (j > r && oVErg.asLong());

                //                while( pVector->getAt( Value( (int32_t)++i ) ) < pVector->getAt( Value( (int32_t)r ) ) ) ;
                //                while( pVector->getAt( Value( (int32_t)--j ) ) > pVector->getAt( Value( (int32_t)r ) ) ) ;
            }
            if (i >= j)
                break;
            pVector->getAt(Value(i)).swap(pVector->getAt(Value(j)));
        }
        pVector->getAt(Value(i)).swap(pVector->getAt(Value(r)));
        QuicksortHelper1(oMCI, coCmd, sArrayName, pVector, l, i - 1, sCmpFunc, oArgs);
        QuicksortHelper1(oMCI, coCmd, sArrayName, pVector, i + 1, r, sCmpFunc, oArgs);
    }
}

int32_t CMetaCommand::Partition(CMCI& oMCI, VKommandos& coCmd, const std::string& sArrayName, Value* pVector, int32_t l, int32_t r, const std::string& sCmpFunc, const ArgumentList& oArgs)
{
    // Partition(A,l,r)
    ArgumentList::const_iterator ai;
    int32_t j, k, p;

    j = l - 1;
    k = r + 1;
    p = l;
    while (true) {
        if (sCmpFunc.empty()) {
            do
                j++;
            while (pVector->getAt(Value(j)) < pVector->getAt(Value(p)));
            do
                k--;
            while (pVector->getAt(Value(k)) > pVector->getAt(Value(p)));
        }
        else {
            ArgumentList coArgs;
            Value oVErg;

            do {
                j++;
                if (j != p) {
                    coArgs.clear();
                    coArgs.push_back(Value(sArrayName));
                    coArgs.push_back(Value(j));
                    coArgs.push_back(Value(p));
                    for (ai = oArgs.begin(); ai != oArgs.end(); ai++)
                        coArgs.push_back(*ai);
                    if (!DoUserFunction(sCmpFunc, coArgs, &oVErg)) {
                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Die kleiner-als-Funktion '#func %s $arrayname $i1 $i2' fuer #sort <array> <lessfunc> wurde nicht gefunden!", m_sSrcFile.c_str(), oMCI.m_nLine, sCmpFunc.c_str()));
                        return -1;
                    }
                }
                else
                    oVErg = 0;
            } while (oVErg.asLong());  // pVector->getAt( Value( j ) ) < pVector->getAt( Value( l ) ) );

            do {
                k--;
                if (k != p) {
                    coArgs.clear();
                    coArgs.push_back(Value(sArrayName));
                    coArgs.push_back(Value(p));
                    coArgs.push_back(Value(k));
                    for (ai = oArgs.begin(); ai != oArgs.end(); ai++)
                        coArgs.push_back(*ai);
                    if (!DoUserFunction(sCmpFunc, coArgs, &oVErg)) {
                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Die kleiner-als-Funktion '#func %s $arrayname $i1 $i2' fuer #sort <array> <lessfunc> wurde nicht gefunden!", m_sSrcFile.c_str(), oMCI.m_nLine, sCmpFunc.c_str()));
                        return -1;
                    }
                }
                else
                    oVErg = 0;
            } while (oVErg.asLong());
            //            do k--; while( pVector->getAt( Value( k ) ) > pVector->getAt( Value( l ) ) );
        }
        if (j < k) {
            if (p == j)
                p = k;
            else if (p == k)
                p = j;
            pVector->getAt(Value(j)).swap(pVector->getAt(Value(k)));
        }
        else
            return k;
    }
}

void CMetaCommand::QuicksortHelper2(CMCI& oMCI, VKommandos& coCmd, const std::string& sArrayName, Value* pVector, int32_t l, int32_t r, const std::string& sCmpFunc, const ArgumentList& oArgs)
{
    // Quicksort(A,l,r)
    int32_t k;

    if (l < r) {
        k = Partition(oMCI, coCmd, sArrayName, pVector, l, r, sCmpFunc, oArgs);
        if (k >= 0) {
            QuicksortHelper2(oMCI, coCmd, sArrayName, pVector, l, k, sCmpFunc, oArgs);
            QuicksortHelper2(oMCI, coCmd, sArrayName, pVector, k + 1, r, sCmpFunc, oArgs);
        }
    }
}

void CMetaCommand::RunScript(CMCI& oMCI, Expression::Variables& oContext, std::string* psCom, VKommandos& coCmd, int nIdx)
{
    /*
        if( !psCom )
        {
            coCmd.push_back( std::string("; FATAL ERROR in script!") );
            return;
        }
    */
    oMCI.m_nTC = nIdx;
    if (!oMCI.m_nTC && (*this)[(size_t)oMCI.m_nTC] == "//")
        oMCI.m_nTC++;

    do {
        do {
            if (g_coBreakConditions.size() && !g_nBreakCondition) {
                Expression oExp("1+1", "<debugger>", 0);
                Value oVal;
                int h;
                for (size_t i = 0; i < g_coBreakConditions.size(); i++) {
                    if (!oExp.evaluate(oContext, g_coBreakConditions[i].c_str(), &oVal, &h, false)) {
                        if (!(!oVal)) {
                            CMetaCommand::SetTrace(2);
                            g_nBreakCondition = (int)(i + 1);
                            break;
                        }
                    }
                }
            }

            if (g_nLimitRuntime) {
                if (!g_nStartTime) {
                    g_nStartTime = time(NULL);
                }
                static int divider = 0;
                if (divider++ > 100) {
                    divider = 0;
                    if (time(NULL) > g_nStartTime + g_nLimitRuntime) {
                        throw CTimeoutException(int32_t(time(NULL) - g_nStartTime));
                    }
                }
            }

            if (g_nTrace && !IsFlag(VF_NOCONSOLE) && (!g_poStepOver || g_poStepOver == &oMCI)) {
                if (g_nTraceSteps) {
                    if (bFirstTrace) {
                        CONMSG(("\n----------------------------------------\n"));
                        bFirstTrace = false;
                    }
                    else
                        CONMSG(("----------------------------------------\n"));
                    CONMSG(("File: %s(%d)\n", m_sSrcFile.c_str(), m_coLocs[(size_t)oMCI.m_nTC].first));
                    TraceLinesFromFile(m_sSrcFile, m_coLocs[(size_t)oMCI.m_nTC].first);
                    if (g_nBreakCondition) {
                        CONMSG(("Ausnahme duch Abbruchbedingung %d: %s\n", g_nBreakCondition, g_coBreakConditions[(size_t)g_nBreakCondition - 1].c_str()));
                        g_nBreakCondition = 0;
                    }
                    DumpContext(oContext);
                }
                if (g_nTraceSteps > 0) {
                    if (!(--g_nTraceSteps)) {
                        g_nTraceSteps = -1;
                        g_nTrace = 2;
                    }
                }
                if (g_nTrace == 2) {
                    std::vector<std::string> coArgs;
                    //                  char Buff[128];

                    g_nTraceSteps = 0;
                    g_poStepOver = 0;
                    g_poStepOut = 0;
                    while (true) {
                        fprintf(stderr, "> ");
                        fflush(stderr);
                        auto nStart = clock();
                        std::string sInput = InputFromConsole();
                        g_nTimeCorrection += clock() - nStart;
                        coArgs.clear();
                        //                        std::string sInput( Buff );
                        CRegExp oRE;
                        int p = 0;
                        //                      oRE.Prepare( "([^ \\t\\n\\r\\f']|'([^']|\\')+')+" );
                        //                      oRE.Prepare( "([^ \\t\\n\\r\\f']|'([^'\\\\]|\\\\')*')+" );
                        oRE.Prepare("([^ \\t\\n\\r\\f']+|'([^'\\\\]|\\\\.)*')+");
                        while (oRE.Find(sInput, p)) {
                            coArgs.push_back(sInput.substr((size_t)oRE.Begin(), (size_t)oRE.Size()));
                            //                        printf( "%s\n", sInput.substr( oRE.Begin(), oRE.Size() ).c_str() );
                            p = oRE.End() + 1;
                        }

                        // split( coArgs, std::string( Buff ), std::string( " \t" ), std::string( "'" ), '\\' );

                        if (coArgs.size()) {
                            if (IsEqual(coArgs[0].c_str(), "b")) {
                                if (coArgs.size() == 1) {
                                    for (size_t i = 0; i < g_coBreakConditions.size(); i++) {
                                        CONMSG(("%2d: %s\n", i + 1, g_coBreakConditions[i].c_str()));
                                    }
                                }
                                else if (coArgs.size() == 2) {
                                    g_coBreakConditions.push_back(coArgs[1]);
                                    CONMSG(("Als %i. Abbruchbedingung gesetzt.\n", g_coBreakConditions.size()));
                                }
                                else {
                                    CONMSG(("Zu viele Parameter!\n"));
                                }
                            }
                            else if (IsEqual(coArgs[0].c_str(), "bd")) {
                                if (coArgs.size() == 2) {
                                    int id = atoi(coArgs[1].c_str()) - 1;
                                    if (id < 0 || size_t(id) > g_coBreakConditions.size()) {
                                        CONMSG(("Ungueltige Nummer fuer Abbruchbedingung!\n"));
                                    }
                                    else {
                                        g_coBreakConditions.erase(g_coBreakConditions.begin() + id);
                                        CONMSG(("Abbruchbedingung aus Liste geloescht.\n"));
                                    }
                                }
                                else {
                                    CONMSG(("Falsche Parameterzahl!\n"));
                                }
                            }
                            else if (IsEqual(coArgs[0].c_str(), "e")) {
                                Expression oExp(std::string("3+3"), "<debugger>", 0);
                                Value oVal;
                                int help;
                                int32_t nTrace = g_nTrace;
                                int32_t nTraceSteps = g_nTraceSteps;

                                g_nTrace = 0;
                                g_nTraceSteps = -1;

                                if (coArgs.size() >= 2) {
                                    if (!oExp.evaluate(oContext, coArgs[1].c_str(), &oVal, &help, false))
                                        DumpVariable("Ergebnis", oVal);
                                    else {
                                        CONMSG(("Fehler in Ausdruck: %s\n", oVal.asString().c_str()));
                                    }
                                }
                                else {
                                    CONMSG(("Ausdruck fehlt!\n"));
                                }
                                g_nTrace = nTrace;
                                g_nTraceSteps = nTraceSteps;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "g")) {
                                DumpContext(Expression::globalContext());
                                break;
                            }
#ifdef _DEBUG
                            else if (IsEqual(coArgs[0].c_str(), "i")) {
                                CONMSG(("Anzahl instanziierter Values: %d\n", Value::ValueCount()));
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "cvp")) {
                                Value::ResetPool();
                                CONMSG(("Valuepool zurueckgesetzt.\n"));
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "dvp")) {
                                CONMSG(("Valuepool seit letztem Reset:\n"));
                                Value::DumpPool();
                                break;
                            }
#endif
                            else if (IsEqual(coArgs[0].c_str(), "l")) {
                                g_nTrace = 0;
                                g_nTraceSteps = -1;
                                g_poStepOut = &oMCI;
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "q")) {
                                g_nTrace = 0;
                                g_nTraceSteps = -1;
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "q!")) {
                                g_nTrace = 0;
                                g_nTraceSteps = -1;
                                g_bNoMoreBreaks = true;
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "r")) {
                                g_nTrace = 1;
                                g_nTraceSteps = -1;
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "v")) {
                                DumpContext(oContext);
                            }
                            else if (IsEqual(coArgs[0].c_str(), "s")) {
                                if (coArgs.size() == 2) {
                                    g_nTraceSteps = atoi(coArgs[1].c_str());
                                }
                                if (g_nTraceSteps <= 0)
                                    g_nTraceSteps = 1;

                                g_nTrace = 1;
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "n")) {
                                if (coArgs.size() == 2) {
                                    g_nTraceSteps = atoi(coArgs[1].c_str());
                                }
                                if (g_nTraceSteps <= 0)
                                    g_nTraceSteps = 1;

                                g_nTrace = 1;
                                g_poStepOver = &oMCI;
                                break;
                            }
                            else if (IsEqual(coArgs[0].c_str(), "w")) {
                                if (g_coCallStack.size()) {
                                    for (size_t i = 0; i < g_coCallStack.size(); i++) {
                                        CONMSG(("%d:  %s\n", i, g_coCallStack[g_coCallStack.size() - i - 1].c_str()));
                                    }
                                }
                            }
                            else if (!strcmp(sInput.c_str(), "?") || IsEqual(sInput, "hilfe") || IsEqual(sInput, "help")) {
                                CONMSG(("Metaskript-Debugger Befehlsliste:\n"));
                                CONMSG(("---------------------------------\n"));
                                CONMSG(("b [exp] Abbruchbedingungen anzeigen oder setzen\n"));
                                CONMSG(("bd n    Abbruchbedingung <n> loeschen\n"));
                                CONMSG(("e exp   Ausdruck <exp> auswerten und Ergebnis anzeigen\n"));
                                CONMSG(("g       Globals-Information auflisten\n"));
#ifdef _DEBUG
                                CONMSG(("i       Interne Informationen\n"));
                                CONMSG(("cvp     Interne Informationen\n"));
                                CONMSG(("dvp     Interne Informationen\n"));
#endif
                                CONMSG(("l       Ohne Trace bis zum Verlassen des Kontextes fortfahren\n"));
                                CONMSG(("n [n]   Einen (bzw. <n>) Schritt(e) ausfuehren, Aufrufe ueberspringen\n"));
                                CONMSG(("q       Debugger beenden und ohne Trace fortfahren\n"));
                                CONMSG(("q!      Debugger beenden und weitere #trace ignorieren\n"));
                                CONMSG(("r       Ausfuehrung mit Trace fortsetzen\n"));
                                CONMSG(("s [n]   Einen (bzw. <n>) Schritt(e) in der Ausfuehrung fortsetzen\n"));
                                CONMSG(("v       Ausgeben des lokalen Kontextes\n"));
                                CONMSG(("w       Ausgeben des Call-Stacks\n"));
                            }
                            else if (!sInput.empty()) {
                                CONMSG(("Unbekannter Befehl! (?=Hilfe)\n"));
                            }
                        }
                    }
                }
            }
        } while (!g_nTraceSteps);

        g_nStepCount++;

        if (Parse(oMCI, oContext, coCmd, true)) {
            // Eine einfach Zuweisung hat stattgefunden
            Parse(oMCI, oContext, coCmd);
        }
        else {
            std::map<std::string, int>::const_iterator mti = g_cnMetaTokens.find(oMCI.m_sArg);
            if (mti != g_cnMetaTokens.end()) {
                switch ((*mti).second) {
                    case GMT_forever: {
                        SubBlock(oMCI, oContext, true, psCom, coCmd);
                    } break;
                    case GMT_after: {
                        int n;
                        if (oMCI.m_nTC <= (int)Args()) {
                            CReference* pRef;
                            size_t nPos = (size_t)oMCI.m_nTC;
                            Parse(oMCI, oContext, coCmd);
                            n = atoi(oMCI.m_sArg.c_str());
                            pRef = oMCI.m_pvRef;
                            oMCI.m_pvRef = 0;
                            if (--n == 0) {
                                coCmd.push_back(std::string("; INFO: #after ist abgelaufen!"));
                                SubBlock(oMCI, oContext, true, psCom, coCmd);
                                //              (*this)[m_nTC] = ";";
                            }
                            else {
                                SubBlock(oMCI, oContext, false, psCom, coCmd);
                            }
                            if (pRef) {
                                pRef->self() = Value((int32_t)(n < 0 ? 0 : n));
                                delete pRef;
                            }
                            else {
                                if (psCom) {
                                    (*this)[nPos] = std::to_string(n < 0 ? 0 : n);
                                    if (n <= 0 && nPos >= 2)
                                        (*this)[nPos - 2] = ";";
                                    *psCom = AsString();
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: In Prozeduren sind fuer #after nur Variable erlaubt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                    //                      coCmd.push_back( std::string("; FEHLER: In Prozeduren sind fuer #after nur Variable erlaubt!") );
                                }
                            }
                        }
                    } break;
                    case GMT_next: {
                        int n;
                        if (oMCI.m_nTC <= (int)Args()) {
                            CReference* pRef;
                            size_t nPos = (size_t)oMCI.m_nTC;
                            Parse(oMCI, oContext, coCmd);
                            n = atoi(oMCI.m_sArg.c_str());
                            pRef = oMCI.m_pvRef;
                            oMCI.m_pvRef = 0;
                            if (n > 0) {
                                n--;
                                SubBlock(oMCI, oContext, true, psCom, coCmd);
                            }
                            else {
                                SubBlock(oMCI, oContext, false, psCom, coCmd);
                                coCmd.push_back(std::string("; INFO: #next ist abgelaufen!"));
                                //              (*this)[0] = ";";
                            }
                            if (pRef) {
                                pRef->self() = Value(int32_t(n));
                                delete pRef;
                            }
                            else {
                                if (psCom) {
                                    (*this)[nPos] = std::to_string(n);
                                    *psCom = AsString();
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: In Prozeduren sind fuer #next nur Variable erlaubt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                    //                      coCmd.push_back( std::string("; FEHLER: In Prozeduren sind fuer #next nur Variable erlaubt!") );
                                }
                            }
                        }
                    } break;
                    case GMT_every: {
                        int n, m;
                        if (oMCI.m_nTC <= (int)Args()) {
                            CReference* pRef;
                            size_t nPos;
                            Parse(oMCI, oContext, coCmd);
                            m = atoi(oMCI.m_sArg.c_str());
                            nPos = (size_t)oMCI.m_nTC;
                            Parse(oMCI, oContext, coCmd);
                            n = atoi(oMCI.m_sArg.c_str());
                            pRef = oMCI.m_pvRef;
                            oMCI.m_pvRef = 0;
                            if (--n == 0) {
                                n = m;
                                SubBlock(oMCI, oContext, true, psCom, coCmd);
                            }
                            else {
                                SubBlock(oMCI, oContext, false, psCom, coCmd);
                            }
                            if (pRef) {
                                pRef->self() = Value(int32_t(n));
                                delete pRef;
                            }
                            else {
                                if (psCom) {
                                    (*this)[nPos] = std::to_string(n);
                                    *psCom = AsString();
                                }
                                else {
                                    //                      coCmd.push_back( std::string("; FEHLER: In Prozeduren sind fuer #every nur Variable erlaubt!") );
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: In Prozeduren sind fuer #every nur Variable erlaubt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                        }
                    } break;
                    case GMT_ifunit: {
                        int32_t en, ei;

                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);

                            en = EinheitenNummer(oMCI.m_sArg);

                            for (ei = 0; ei < (int32_t)g_poCurrentRegion->GetVEinheiten().size(); ei++) {
                                if (en == g_poCurrentRegion->GetVEinheiten()[(size_t)ei]->m_nNummer)
                                    break;
                            }

                            SubBlock(oMCI, oContext, ei < (int32_t)g_poCurrentRegion->GetVEinheiten().size(), psCom, coCmd);
                            if (oMCI.m_sArg == "else" || oMCI.m_sArg == "#else") {
                                SubBlock(oMCI, oContext, !(ei < (int32_t)g_poCurrentRegion->GetVEinheiten().size()), psCom, coCmd);
                            }
                        }
                    } break;
                    case GMT_ifregion: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            bool bVal;
                            Parse(oMCI, oContext, coCmd);
                            if (g_poCurrentRegion) {
                                bVal = (oMCI.m_sArg == g_poCurrentRegion->GetName());
                            }
                            else {
                                bVal = false;
                                ERRMSG(&coCmd, ("%s(%d) : FEHLER: #ifregion ausserhalb eines Regionskontextes benutzt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            }
                            if ((*this)[(size_t)oMCI.m_nTC] == "{") {
                                SubBlock(oMCI, oContext, bVal, psCom, coCmd);
                                if (oMCI.m_sArg == "else" || oMCI.m_sArg == "#else") {
                                    SubBlock(oMCI, oContext, !bVal, psCom, coCmd);
                                }
                            }
                            else {
                                ERRMSG(&coCmd, ("%s(%d) : FEHLER: '{' zu Beginn eines Komandoblocks erwartet!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            }
                        }
                    } break;
                    case GMT_if: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            bool bVal;
                            Parse(oMCI, oContext, coCmd);
                            bVal = !(!oMCI.m_vArg);  //(atoi( oMCI.m_sArg.c_str() )!=0);
                            if ((*this)[(size_t)oMCI.m_nTC] == "{") {
                                SubBlock(oMCI, oContext, bVal, psCom, coCmd);
                                if (oMCI.m_sArg == "else" || oMCI.m_sArg == "#else") {
                                    SubBlock(oMCI, oContext, !bVal, psCom, coCmd);
                                }
                            }
                            else {
                                ERRMSG(&coCmd, ("%s(%d) : FEHLER: '{' zu Beginn eines Komandoblocks erwartet!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            }
                        }
                    } break;
                    case GMT_while: {
                        if (psCom) {
                            //              coCmd.push_back( std::string("; FEHLER: #while ist nur in Prozeduren erlaubt!") );
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #while ist nur in Prozeduren erlaubt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                        if (oMCI.m_nTC <= (int)Args()) {
                            int nLoopPos = oMCI.m_nTC;
                            bool bVal;
                            oMCI.m_nLoop++;
                            do {
                                oMCI.m_nTC = nLoopPos;
                                Parse(oMCI, oContext, coCmd);
                                bVal = (atoi(oMCI.m_sArg.c_str()) != 0);
                                try {
                                    SubBlock(oMCI, oContext, bVal, psCom, coCmd);
                                }
                                catch (CBreakException oBX) {
                                    oMCI.m_nTC = nLoopPos;
                                    Parse(oMCI, oContext, coCmd);
                                    SubBlock(oMCI, oContext, false, psCom, coCmd);
                                    bVal = false;
                                }
                                catch (CContinueException oCX) {
                                    bVal = true;
                                }
                            } while (bVal);
                            oMCI.m_nLoop--;
                        }
                    } break;
                    case GMT_break: {
                        if (!oMCI.m_nLoop) {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #break ausserhalb von Schleifen benutzt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            Parse(oMCI, oContext, coCmd);
                        }
                        else {
                            throw CBreakException();
                        }
                    } break;
                    case GMT_continue: {
                        if (!oMCI.m_nLoop) {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #continue ausserhalb von Schleifen benutzt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            Parse(oMCI, oContext, coCmd);
                        }
                        else {
                            throw CContinueException();
                        }
                    } break;
                    case GMT_assert: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            bool bVal;
                            Parse(oMCI, oContext, coCmd);
                            bVal = !(!oMCI.m_vArg);
                            if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                Parse(oMCI, oContext, coCmd);
                                if (!bVal) {
                                    ERRMSG(&coCmd, ("%s(%d) : #assert fehlgeschlagen: %s", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                                }
                            }
                            else {
                                if (!bVal) {
                                    ERRMSG(&coCmd, ("%s(%d) : #assert fehlgeschlagen!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                            Parse(oMCI, oContext, coCmd);
                        }
                    } break;
                    case GMT_error: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: %s", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                        }
                        else {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: Keine Fehlermeldung fuer #error!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                    } break;
                    case GMT_warning: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);
                            ERRMSG(&coCmd, ("%s(%d) : Warnung: %s", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                        }
                        else {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: Keine Fehlermeldung fuer #warning!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                    } break;
                    case GMT_message: {
                        if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                            Parse(oMCI, oContext, coCmd);
                            coCmd.push_back(std::string("; " + oMCI.m_sArg));
                        }
                        else {
                            coCmd.push_back(std::string(""));
                        }
                        Parse(oMCI, oContext, coCmd);
                    } break;
                    case GMT_debug: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);
                            if (IsEqual(oMCI.m_sArg, "progress")) {
                                Parse(oMCI, oContext, coCmd);
                                if (IsFlag(VF_PROGRESSINFO)) {
                                    g_bForceEOL = true;
                                    COutput::TPrint("console", "\r{}", oMCI.m_sArg.c_str());
                                }
                            }
                            else {
                                if (IsFlag(VF_DEBUGMODE))
                                    COutput::TPrint("debug", "{}\n", oMCI.m_sArg.c_str());
                            }
                            Parse(oMCI, oContext, coCmd);
                        }
                    } break;
                    case GMT_input: {
                        if (IsFlag(VF_RESTRICTED)) {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #input ist im Restricted-Modus deaktiviert!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                        std::string sVarName;
                        Value* pVal = 0;
                        if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                            Parse(oMCI, oContext, coCmd);
                            CONMSG(("\n%s\n", oMCI.m_sArg.c_str()));
                            if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                Parse(oMCI, oContext, coCmd, true, false);
                                sVarName = oMCI.m_sArg.c_str();
                                pVal = oMCI.m_pVal;
                            }
                        }
                        else {
                            if (IsFlag(VF_PROGRESSINFO)) {
                                g_bForceEOL = true;
                                CONMSG(("\r[Weiter mit der Eingabetaste]"));
                            }
                            else {
                                CONMSG(("\n[Weiter mit der Eingabetaste]\n"));
                            }
                        }
                        auto nStart = clock();
                        std::string sInput;
                        if (!IsFlag(VF_NOCONSOLE))
                            sInput = InputFromConsole();
                        g_nTimeCorrection += clock() - nStart;
                        if (!sVarName.empty()) {
                            Value oErg(sInput);
                            if (pVal) {
                                *pVal = oErg;
                            }
                            else if (m_bForceDeclare || !Expression::setValue(oContext, sVarName.c_str(), &oErg, m_bForceDeclare)) {
                                ERRMSG(&coCmd, ("%s(%d) : FEHLER: Undefinierte Variable '%s' als Ziel fuer #input verwendet!", m_sSrcFile.c_str(), oMCI.m_nLine, sVarName.c_str()));
                            }
                        }
                        if (IsFlag(VF_NOCONSOLE)) {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #input bei umgeleiteten Ausgabekanaelen verwendet!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                        Parse(oMCI, oContext, coCmd);
                    } break;
                    case GMT_table: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);
                            if (IsEqual(oMCI.m_sArg, "CLEAR"))
                                g_oOT.Clear();
                            else if (IsEqual(oMCI.m_sArg, "DUMP")) {
                                if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                    Parse(oMCI, oContext, coCmd);
                                    FileInfo* pfi = GetFileInfo(oMCI.m_vArg.asLong());
                                    if (!pfi || !pfi->hFile) {
                                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Ungueltiges Datei-Handle fuer #table DUMP verwendet!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                    }
                                    else {
                                        g_oOT.Output(pfi->hFile);
                                    }
                                }
                                else {
                                    g_oOT.Output(coCmd, "; ");
                                }
                                g_oOT.Clear();
                            }
                            else if (IsEqual(oMCI.m_sArg, "DEBUG")) {
                                if (IsFlag(VF_DEBUGMODE))
                                    g_oOT.Output("debug", "");
                                g_oOT.Clear();
                            }
                            else if (IsEqual(oMCI.m_sArg, "NEXT"))
                                g_oOT.Next();
                            else {
                                COutputTable::FORMAT enFormat = COutputTable::enLEFT;
                                bool bHaveFormat = false;
                                if (IsEqual(oMCI.m_sArg, "LEFT")) {
                                    bHaveFormat = true;
                                    enFormat = COutputTable::enLEFT;
                                }
                                else if (IsEqual(oMCI.m_sArg, "RIGHT")) {
                                    bHaveFormat = true;
                                    enFormat = COutputTable::enRIGHT;
                                }
                                else if (IsEqual(oMCI.m_sArg, "CENTER")) {
                                    bHaveFormat = true;
                                    enFormat = COutputTable::enCENTER;
                                }
                                if (bHaveFormat) {
                                    Parse(oMCI, oContext, coCmd);
                                }
                                switch (oMCI.m_vArg.getType()) {
                                    case VT_INT:
                                        if (bHaveFormat)
                                            g_oOT.Col(oMCI.m_vArg.asLong(), enFormat);
                                        else
                                            g_oOT.Col(oMCI.m_vArg.asLong());
                                        break;
                                    case VT_FLOAT:
                                        if (bHaveFormat)
                                            g_oOT.Col(oMCI.m_vArg.asReal(), 2, enFormat);
                                        else
                                            g_oOT.Col(oMCI.m_vArg.asReal());
                                        break;
                                    case VT_STRING:
                                        if (bHaveFormat)
                                            g_oOT.Col(oMCI.m_vArg.asString(), enFormat);
                                        else
                                            g_oOT.Col(oMCI.m_vArg.asString());
                                        break;
                                    default:
                                        g_oOT.Col("???", enFormat);
                                }
                            }

                            //              coCmd.push_back( std::string( "; " + oMCI.m_sArg ) );
                            Parse(oMCI, oContext, coCmd);
                        }
                    } break;
                    case GMT_config: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            CBlockBase::CFGOBJMODE enMode(CBlockBase::enTABH);
                            std::vector<std::string> coVNames;
                            std::string sFile, sMode, sCaption;
                            bool bMode = true;

                            Parse(oMCI, oContext, coCmd);
                            sCaption = oMCI.m_sArg;

                            Parse(oMCI, oContext, coCmd);
                            sMode = oMCI.m_sArg;
                            if (IsEqual(sMode, "FILE")) {
                                Parse(oMCI, oContext, coCmd);
                                sFile = oMCI.m_sArg;
                                if (IsFlag(VF_RESTRICTED) && sFile.find_first_of("/\\:") != std::string::npos) {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: #config darf im Restricted-Modus nur ohne Pfad benutzt werden!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }

                                Parse(oMCI, oContext, coCmd);
                                sMode = oMCI.m_sArg;
                            }

                            if (IsEqual(sMode, "TABH")) {
                                enMode = CBlockBase::enTABH;
                            }
                            else if (IsEqual(sMode, "TABV")) {
                                enMode = CBlockBase::enTABV;
                            }
                            else if (IsEqual(sMode, "NESTED")) {
                                enMode = CBlockBase::enNEST;
                            }
                            else {
                                bMode = false;
                            }

                            while (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                Parse(oMCI, oContext, coCmd);
                                coVNames.push_back(oMCI.m_sArg);
                            }

                            Parse(oMCI, oContext, coCmd);

                            if (bMode && !sCaption.empty() && (enMode == CBlockBase::enTABH || coVNames.empty())) {
                                g_sSrcFile = m_sSrcFile;
                                g_nSrcLine = oMCI.m_nLine;
                                CBlockBase::ReadConfigObjects(sFile, sCaption, enMode, coVNames);
                                g_nSrcLine = -1;
                            }
                            else {
                                ERRMSG(&coCmd, ("%s(%d) : FEHLER: Falsche Parameter fuer #config <Object> [FILE <Datei>] <Mode> [<Titles>] benutzt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            }
                        }
                    } break;
                    case GMT_call: {
                        if (oMCI.m_nTC <= (int)Args()) {
                            std::vector<CReference*> coRefs;
                            Expression::Variables oVars;
                            CMetaCommand* pMC;
                            std::string sPName;
                            std::string sAName;
                            std::string sStackInfo;
                            int nPArgs = 0, argi = 1;
                            size_t nPos;
                            size_t nRealArgs = 0;
                            bool bVArg = false;
                            bool bWasFunc = false;
                            bool invalidArgs = false;

                            Parse(oMCI, oContext, coCmd);
                            sPName = oMCI.m_sArg;
                            sStackInfo = std::string("#proc ") + sPName;
                            pMC = g_oScriptBase.FindProc(sPName);
                            if (pMC) {
                                if (pMC->IsFunction()) {
                                    pMC = 0;
                                    bWasFunc = true;
                                }
                                else {
                                    nPArgs = atoi((*pMC)[0].c_str());
                                    if (nPArgs < 0) {
                                        nPArgs = -nPArgs;
                                        bVArg = true;
                                    }
                                    nPArgs--;
                                }
                            }

                            nPos = (size_t)oMCI.m_nTC;

                            while (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                argi++;
                                if (Parse(oMCI, oContext, coCmd)) {
                                    invalidArgs = true;
                                }
                                nRealArgs++;
                                sStackInfo += std::string(" ") + oMCI.m_vArg.asString();
                                oVars[std::string("#ARG") + ToString(int32_t(argi - 1))] = oMCI.m_vArg;
                                coRefs.push_back(oMCI.m_pvRef);
                                oMCI.m_pvRef = NULL;
                                if (pMC && argi - 1 <= nPArgs) {
                                    sAName = (*pMC)[(size_t)argi];
                                    oVars[sAName[0] == '&' ? sAName.substr(1) : sAName] = oMCI.m_vArg;
                                    oVars[std::string("#REF") + ToString(int32_t(argi - 1))] = (sAName[0] == '&' ? sAName.substr(1) : sAName);
                                }
                                else if (pMC) {
                                    //                      char Buff[256];
                                    //                      sprintf( Buff, "; FEHLER: Ueberzaehliges Argument '%s' fuer Prozedur '%s'!", oMCI.m_sArg.c_str(), sPName.c_str() );
                                    //                      coCmd.push_back( std::string(Buff) );
                                    if (!bVArg)
                                        ERRMSG(&coCmd, ("%s(%d) : Warnung: Ueberzaehliges Argument '%s' fuer Prozedur '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str(), sPName.c_str()));
                                }
                            }
                            oVars[std::string("#ARG0")] = Value(int32_t(argi - 1));
                            oVars[std::string("#REF0")] = Value(int32_t(nPArgs));
                            Parse(oMCI, oContext, coCmd);
                            if (pMC) {
                                if (argi - 1 < nPArgs) {
                                    //                          char Buff[256];
                                    //                          sprintf( Buff, "; FEHLER: Zu wenig Argumente fuer Prozedur '%s'!", sPName.c_str() );
                                    //                          coCmd.push_back( std::string(Buff) );
                                    if (!bVArg)
                                        ERRMSG(&coCmd, ("%s(%d) : Warnung: Zu wenig Argumente fuer Prozedur '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, sPName.c_str()));
                                    while (argi - 1 < nPArgs) {
                                        sAName = (*pMC)[(size_t)++argi];
                                        oVars[sAName[0] == '&' ? sAName.substr(1) : sAName] = Value(0);
                                        coRefs.push_back(0);
                                    }
                                }
                                //                  printf( " ; Call to procedure: %s\n", sPName.c_str() );
                                //                  for( Expression::Variables::iterator evi = oVars.begin(); evi != oVars.end(); evi++ )
                                //                  {
                                //                      printf( " ;    %s = %s\n", (*evi).first.c_str(), (*evi).second.asString().c_str() );
                                //                  }
                                // CALL
                                //                  int nOldTC = oMCI.m_nTC; // Um Rekursionen zu erm�glichen den TC retten
                                CMCI oNMCI(nPArgs + 2);

                                if (!invalidArgs) {
                                    g_coCallStack.push_back(sStackInfo);

                                    try {
                                        pMC->SubBlock(oNMCI, oVars, true, 0, coCmd, nPArgs + 2);
                                    }
                                    catch (CReturnException oRX) {
                                    }
#ifdef ROCK_SOLID_CATCH
                                    catch (...) {
                                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: Unerwartete Ausnahmebehandlung in Prozedur '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, sPName.c_str()));
                                    }
#endif
                                    g_coCallStack.pop_back();
                                }
                                if (g_poStepOut == &oNMCI)
                                    CMetaCommand::SetTrace(2);

                                //                  oMCI.m_nTC = nOldTC;
                                for (size_t ih = 0; ih < coRefs.size(); ih++) {
                                    if ((int)ih < nPArgs && (*pMC)[ih + 2][0] == '&') {
                                        Expression::Variables::iterator vi = oVars.find((*pMC)[ih + 2].substr(1));
                                        if (vi != oVars.end()) {
                                            if (coRefs[ih]) {
                                                coRefs[ih]->self() = (*vi).second;
                                            }
                                            else {
                                                if (psCom && ih < nRealArgs) {
                                                    if ((*vi).second.getType() == VT_STRING)
                                                        (*this)[nPos + ih] = std::string("'") + (*vi).second.asString() + "'";
                                                    else
                                                        (*this)[nPos + ih] = (*vi).second.asString();
                                                }
                                            }
                                        }
                                    }
                                    else if ((int)ih >= nPArgs && pMC->m_bVRef) {
                                        Expression::Variables::iterator vi = oVars.find(std::string("#ARG") + ToString(int32_t(ih + 1)));
                                        if (vi != oVars.end()) {
                                            if (coRefs[ih]) {
                                                coRefs[ih]->self() = (*vi).second;
                                            }
                                            else {
                                                if (psCom && ih < nRealArgs) {
                                                    if ((*vi).second.getType() == VT_STRING)
                                                        (*this)[nPos + ih] = std::string("'") + (*vi).second.asString() + "'";
                                                    else
                                                        (*this)[nPos + ih] = (*vi).second.asString();
                                                }
                                            }
                                        }
                                    }

                                    if (coRefs[ih])
                                        delete coRefs[ih];
                                }
                                if (psCom)
                                    *psCom = AsString();
                            }
                            else {
                                //                  char Buff[256];
                                //                  sprintf( Buff, "; FEHLER: Unbekannte Prozedur '%s'!", sPName.c_str() );
                                //                  coCmd.push_back( std::string(Buff) );
                                if (bWasFunc) {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: Funktion '#func %s' kann nicht mit #call aufgerufen werden!", m_sSrcFile.c_str(), oMCI.m_nLine, sPName.c_str()));
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: Unbekannte Prozedur '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, sPName.c_str()));
                                }
                            }
                        }
                    } break;
                    case GMT_return: {
                        if (!m_bIsFunc) {
                            if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                ERRMSG(&coCmd, ("%s(%d) : FEHLER: #return <expr> ausserhalb von Funktionen benutzt!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                Parse(oMCI, oContext, coCmd);
                            }
                            else {
                                if (psCom) {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: #return direkt im Zug verwendet!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                                else
                                    throw CReturnException(Value());
                            }
                        }
                        else {
                            Parse(oMCI, oContext, coCmd);
                            throw CReturnException(oMCI.m_vArg);
                        }
                    } break;
                    case GMT_default: {
                        if (g_poCurrentUnit) {
                            for (int32_t ki = 0; ki < (int)g_poCurrentUnit->m_csKommandos.size(); ki++) {
                                if (g_poCurrentUnit->m_csKommandos[ki].asString()[0] != '/' && g_poCurrentUnit->m_csKommandos[ki].asString()[0] != ';' &&
                                    (g_poCurrentUnit->m_csKommandos[ki].asString().size() < 8 ||
                                     (!IsEqual(g_poCurrentUnit->m_csKommandos[ki].asString().substr(0, 8), "reservie") && !IsEqual(g_poCurrentUnit->m_csKommandos[ki].asString().substr(0, 8), "kampfzau")))) {
                                    coCmd.push_back(g_poCurrentUnit->m_csKommandos[ki]);
                                }
                            }
                        }
                        else {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #default ausserhalb eines Einheitenkontext verwendet!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                        Parse(oMCI, oContext, coCmd);
                    } break;
                    case GMT_sort: {
                        if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                            Value* pVal;
                            Parse(oMCI, oContext, coCmd, true, false);
                            pVal = oMCI.m_pVal;
                            if (pVal && pVal->getType() == VT_VECTOR) {
                                std::string sArrayName(oMCI.m_sOArg);
                                ArgumentList oArgs;
                                std::string sFunc;

                                if (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                    Parse(oMCI, oContext, coCmd);
                                    sFunc = oMCI.m_sArg;
                                    while (oMCI.m_nTC <= (int)Args() && (*this)[(size_t)oMCI.m_nTC] != ":" && (*this)[(size_t)oMCI.m_nTC] != "}") {
                                        Parse(oMCI, oContext, coCmd);
                                        oArgs.push_back(oMCI.m_vArg);
                                    }
                                }
                                if (pVal) {
                                    QuicksortHelper2(oMCI, coCmd, sArrayName, pVal, 0, pVal->size() - 1, sFunc, oArgs);
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : #sort kann nur direkte #array-Behaelter sortieren, keine eingebetteten!", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                            else {
                                ERRMSG(&coCmd, ("%s(%d) : #sort <array> <lessfunc> unterstuetzt nur Arrays!", m_sSrcFile.c_str(), oMCI.m_nLine));
                            }
                        }
                        Parse(oMCI, oContext, coCmd);
                    } break;
                    case GMT_trace: {
                        if (IsFlag(VF_RESTRICTED)) {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: #trace ist im Restricted-Modus deaktiviert!", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                        Parse(oMCI, oContext, coCmd);
                        if (!g_bNoMoreBreaks)
                            g_nTrace = atoi(oMCI.m_sArg.c_str());
                        Parse(oMCI, oContext, coCmd);
                    } break;
                    case GMT_notrace: {
                        Parse(oMCI, oContext, coCmd);
                        g_nTrace = 0;
                    } break;
                    case GMT_tag: {
                        std::string sBlock;
                        std::string sTag;
                        Value oVal;
                        std::shared_ptr<CObjectPart> poOA;

                        if (oMCI.m_nTC <= (int)Args()) {
                            oMCI.m_nLine = m_coLocs[(size_t)oMCI.m_nTC].first;
                            oMCI.m_sOArg = m_coArgs[(size_t)oMCI.m_nTC++];
                            oMCI.m_sArg = oMCI.m_sOArg;
                            //                                Parse( oMCI, oContext, coCmd );
                            sBlock = oMCI.m_sArg;
                            poOA = Expression::parseObjectAccess(sBlock, &oContext);
                            sBlock = poOA->label;
                        }
                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);
                            sTag = oMCI.m_sArg;
                        }
                        if (oMCI.m_nTC <= (int)Args()) {
                            Parse(oMCI, oContext, coCmd);
                            oVal = oMCI.m_vArg;
                        }
                        if (!sBlock.empty() && !sTag.empty() && oVal.getType() != VT_EMPTY) {
                            if (IsEqual(sBlock, "REGION")) {
                                if (poOA->index.empty() && g_poCurrentRegion) {
                                    g_poCurrentRegion->SetValue(std::string("!") + sTag, oVal);
                                }
                                else if (poOA->index.size() == 2 && g_poCurrentReport) {
                                    CRegion* pReg = g_poCurrentReport->GetMap()->GetFromECords(poOA->index[0].asLong(), poOA->index[1].asLong(), 0);
                                    if (pReg) {
                                        pReg->SetValue(std::string("!") + sTag, oVal);
                                    }
                                    else {
                                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: #tag fuer nicht in CR enthaltenen REGION-Block aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                    }
                                }
                                else if (poOA->index.size() == 3 && g_poCurrentReport) {
                                    CRegion* pReg = g_poCurrentReport->GetMap()->GetFromECords(poOA->index[0].asLong(), poOA->index[1].asLong(), poOA->index[2].asLong(), 0);
                                    if (pReg && pReg->Map()) {
                                        pReg->SetValue(std::string("!") + sTag, oVal);
                                    }
                                    else {
                                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: #tag fuer nicht in CR enthaltenen REGION-Block aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                    }
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: REGION-Block fuer #tag falsch indiziert oder aus falschem Kontext aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                            else if (IsEqual(sBlock, "EINHEIT")) {
                                if (g_poCurrentUnit && poOA->index.empty()) {
                                    g_poCurrentUnit->SetValue(std::string("!") + sTag, oVal);
                                }
                                else if (g_poCurrentReport && poOA->index.size() == 1) {
                                    CEinheit* pUnit = g_poCurrentReport->SearchUnit(EinheitenNummer(poOA->index[0].asString()), false);
                                    if (pUnit) {
                                        pUnit->SetValue(std::string("!") + sTag, oVal);
                                    }
                                    else {
                                        ERRMSG(&coCmd, ("%s(%d) : FEHLER: #tag fuer nicht in CR enthaltenen EINHEIT-Block aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                    }
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: EINHEIT-Block fuer #tag falsch indiziert oder aus falschem Kontext aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                            else if (IsEqual(sBlock, "BURG") && poOA->index.size() == 1) {
                                int32_t nBNr = (int32_t)strtol(poOA->index[0].asString().c_str(), 0, g_poCurrentReport->BNrBase());
                                CBauwerk* poBuilding = g_poCurrentReport->GetBuilding(nBNr);
                                if (poBuilding) {
                                    poBuilding->SetValue(std::string("!") + sTag, oVal);
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: #tag fuer nicht in CR enthaltenen BURG-Block aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                            else if (IsEqual(sBlock, "SCHIFF") && poOA->index.size() == 1) {
                                int32_t nSNr = (int32_t)strtol(poOA->index[0].asString().c_str(), 0, g_poCurrentReport->BNrBase());
                                CSchiff* poShip = g_poCurrentReport->GetShip(nSNr);
                                if (poShip) {
                                    poShip->SetValue(std::string("!") + sTag, oVal);
                                }
                                else {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: #tag fuer nicht in CR enthaltenen SHIFF-Block aufgerufen", m_sSrcFile.c_str(), oMCI.m_nLine));
                                }
                            }
                        }
                        else {
                            ERRMSG(&coCmd, ("%s(%d) : FEHLER: zu wenig Parameter fuer #tag <BlockName> <TagName> <Expr>", m_sSrcFile.c_str(), oMCI.m_nLine));
                        }
                        Parse(oMCI, oContext, coCmd);
                    } break;
                    case GMT_var: {
                        Expression oExp("3+3", m_sSrcFile.c_str(), oMCI.m_nLine);
                        int bAssign;

                        Parse(oMCI, oContext, coCmd, true, false);
                        while (oMCI.m_sArg != ":" && oMCI.m_sArg != "}") {
                            if (IsIdentifier(oMCI.m_sArg)) {
                                if (!Expression::getLocalRef(oContext, oMCI.m_sArg.c_str())) {
                                    Value oVal;
                                    Expression::setValue(oContext, oMCI.m_sArg.c_str(), &oVal, false);
                                }
                                else {
                                    ERRMSG(0, ("%s(%d) : FEHLER: Im lokalen Kontext existiert bereits ein Bezeichner '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                                }
                            }
                            else {
                                Value oVal;
                                if (oExp.evaluate(oContext, oMCI.m_sArg.c_str(), &oVal, &bAssign, false)) {
                                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: Ausdruck ungueltig '%s': %s", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str(), oVal.asString().c_str()));
                                }
                                if (bAssign) {
                                }
                            }
                            Parse(oMCI, oContext, coCmd, true, false);
                        }
                    } break;
                    case GMT_array: {
                        // Value oVal;

                        Parse(oMCI, oContext, coCmd, true, false);
                        while (oMCI.m_sArg != ":" && oMCI.m_sArg != "}") {
                            if (IsIdentifier(oMCI.m_sArg)) {
                                if (!Expression::getLocalRef(oContext, oMCI.m_sArg.c_str())) {
                                    Value oVal(VT_VECTOR);
                                    Expression::setValue(oContext, oMCI.m_sArg.c_str(), &oVal, false);
                                }
                                else {
                                    ERRMSG(0, ("%s(%d) : FEHLER: Im lokalen Kontext existiert bereits ein Bezeichner '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                                }
                            }
                            else {
                                ERRMSG(0, ("%s(%d) : FEHLER: '%s' ist kein gueltiger Bezeichner!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                            }
                            Parse(oMCI, oContext, coCmd, true, false);
                        }
                    } break;
                    case GMT_dict: {
                        // Value oVal;

                        Parse(oMCI, oContext, coCmd, true, false);
                        while (oMCI.m_sArg != ":" && oMCI.m_sArg != "}") {
                            if (IsIdentifier(oMCI.m_sArg)) {
                                if (!Expression::getLocalRef(oContext, oMCI.m_sArg.c_str())) {
                                    Value oVal(VT_MAP);
                                    Expression::setValue(oContext, oMCI.m_sArg.c_str(), &oVal, false);
                                }
                                else {
                                    ERRMSG(0, ("%s(%d) : FEHLER: Im lokalen Kontext existiert bereits ein Bezeichner '%s'!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                                }
                            }
                            else {
                                ERRMSG(0, ("%s(%d) : FEHLER: '%s' ist kein gueltiger Bezeichner!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                            }
                            Parse(oMCI, oContext, coCmd, true, false);
                        }
                    } break;
                        //                default:
                }
            }
            else if (oMCI.m_sArg[0] == '{') {
                ERRMSG(0, ("%s(%d) : FEHLER: Unerwarteter Beginn eines Befehlsblocks!", m_sSrcFile.c_str(), oMCI.m_nLine));
            }
            else if (oMCI.m_sArg[0] != '#') {
                AddCommand(oMCI, oContext, coCmd);
            }
            else {
                if (oMCI.m_sArg == "#proc" || oMCI.m_sArg == "#func" || oMCI.m_sArg == "#const" || oMCI.m_sArg == "#include") {
                    ERRMSG(0, ("%s(%d) : FEHLER: '%s' Ist nicht innerhalb von Bloecken erlaubt! Vermutlich fehlt davor ein schliessendes '}'.", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                }
                else {
                    ERRMSG(0, ("%s(%d) : FEHLER: '%s' ist kein gueltiger Befehl!", m_sSrcFile.c_str(), oMCI.m_nLine, oMCI.m_sArg.c_str()));
                }
            }
        }

        if (!oMCI.m_sArg.empty() && oMCI.m_sArg != ":" && oMCI.m_sArg != "}") {
            while (oMCI.m_nTC <= (int)Args() && oMCI.m_sArg != ":" && oMCI.m_sArg != "}") {
                oMCI.m_nLine = m_coLocs[(size_t)oMCI.m_nTC].first;
                oMCI.m_sArg = m_coArgs[(size_t)oMCI.m_nTC++];
            }
            {
                if (g_nStepCount != g_nLastErrorStep) {
                    ERRMSG(&coCmd, ("%s(%d) : FEHLER: Fehlendes ':' als Befehlstrenner oder ueberzaehlige Parameter!", m_sSrcFile.c_str(), oMCI.m_nLine));
                }
            }
        }

        if (oMCI.m_sArg == "}") {
            if (!nIdx)
                ERRMSG(&coCmd, ("%s(%d) : FEHLER: Unerwartetes '}' gefunden!", m_sSrcFile.c_str(), oMCI.m_nLine));
            return;
        }
    } while (!oMCI.m_sArg.empty());
}

void CMetaCommand::SubBlock(CMCI& oMCI, Expression::Variables& oContext, bool bCond, std::string* psCom, VKommandos& coCmd, int nIdx)
{
    if (nIdx >= 0)
        oMCI.m_nTC = nIdx;
    Parse(oMCI, oContext, coCmd);
    if (oMCI.m_sArg != "{") {
        ERRMSG(&coCmd, ("%s(%d) : FEHLER: '{' zu Beginn eines Komandoblocks erwartet!", m_sSrcFile.c_str(), oMCI.m_nLine));
        return;
    }

    if (bCond) {
        RunScript(oMCI, oContext, psCom, coCmd, oMCI.m_nTC);
    }
    else {
        //      VKommandos coDummy;
        int nCnt = 1;
        while (!oMCI.m_sArg.empty() && nCnt > 0) {
            Skip(oMCI);
            if (oMCI.m_sArg == "{")
                nCnt++;
            if (oMCI.m_sArg == "}")
                nCnt--;
        }
    }

    if (oMCI.m_sArg != "}") {
        ERRMSG(&coCmd, ("%s(%d) : FEHLER: '}' am Ende eines Komandoblocks erwartet!", m_sSrcFile.c_str(), oMCI.m_nLine));
    }
    else {
        Parse(oMCI, oContext, coCmd);
    }
}

bool CMetaCommand::ProcExists(const std::string& sProc)
{
    CMetaCommand* pMC;
    pMC = g_oScriptBase.FindProc(sProc);
    return (pMC && !pMC->IsFunction());
}

bool CMetaCommand::Call(const std::string& sProc, VKommandos& coCmd)
{
    //  VKommandos coCmd;
    Expression::Variables oVars;
    CMetaCommand* pMC;
    int nPArgs = 0;

    pMC = g_oScriptBase.FindProc(sProc);
    if (pMC) {
        CMCI oMCI(nPArgs + 2);
        nPArgs = atoi((*pMC)[0].c_str());
        if (nPArgs < 0)
            nPArgs = -nPArgs;
        nPArgs--;
        g_coCallStack.push_back(std::string("#proc ") + sProc);
        try {
            pMC->SubBlock(oMCI, oVars, true, 0, coCmd, nPArgs + 2);
        }
        catch (CReturnException oRX) {
        }
#ifdef ROCK_SOLID_CATCH
        catch (...) {
            ERRMSG(&coCmd, ("KeinFile(0) : FEHLER: Unerwartete Ausnahmebehandlung in Prozedur '%s'!", sProc.c_str()));
        }
#endif
        g_coCallStack.pop_back();
        return true;
    }

    return false;
}

void CMetaCommand::AddCommand(CMCI& oMCI, Expression::Variables& oContext, VKommandos& coCmd, bool bMulti)
{
    std::string sCmd;
    int nBC = 0;
    //  int n = nIdx;

    //  Parse( oMCI, oContext, coCmd );
    //  if( oMCI.m_sArg != "{" )
    //  {
    //      coCmd.push_back( std::string("; FEHLER: '{' zu Beginn eines Komandoblocks erwartet!") );
    //      return;
    //  }

    //  Parse( oMCI, oContext, coCmd );
    m_bParseError = false;
    for (; oMCI.m_nTC <= (int)Args(); Parse(oMCI, oContext, coCmd)) {
        if (oMCI.m_sArg.empty() && oMCI.m_vArg.getType() != VT_STRING)
            break;
        if (oMCI.m_sArg == "{")
            nBC++;
        if (oMCI.m_sArg == "}") {
            if (nBC <= 0)
                break;
            nBC--;
        }
        if (nBC <= 0 && oMCI.m_sArg == ":")
            break;
        if (oMCI.m_vArg.getType() == VT_STRING)
            sCmd += Quotionmarks(oMCI.m_sArg);
        else {
            if (oMCI.m_vArg.getType() == VT_FLOAT) {
                oMCI.m_vArg = Value(oMCI.m_vArg.asLong());
                sCmd += oMCI.m_vArg.asString();
            }
            else
                sCmd += oMCI.m_sArg;
        }
        sCmd += ' ';
    }

    if (!m_bParseError) {
        if (!sCmd.empty())
            coCmd.push_back(sCmd);
    }
    else {
        if (!sCmd.empty())
            coCmd.push_back(std::string("; ") + sCmd);
    }
}

CScriptBase::CScriptBase() {}

bool CScriptBase::Import(const std::string& sIFName, const std::string& sText)
{
    if (g_pseudoFiles.count(sIFName)) {
        ERRMSG(0, ("Import of pseudo file with duplicate name '%s'!", sIFName.c_str()));
        return false;
    }
    g_pseudoFiles[sIFName] = sText;
    std::istringstream oIS(sText.c_str());
    return Import(oIS, sIFName);
}

bool CScriptBase::Import(const std::string& sIFName)
{
    std::fstream oIS;
    std::string sFileName = sIFName;

    sFileName = PathedFileName(sFileName);
    oIS.open(sFileName.c_str(), ios::in);

    if (oIS.fail()) {
        ERRMSG(0, ("Auf die Skript-Datei '%s' kann nicht zugegriffen werden!", sFileName.c_str()));
        return false;
    }

    return Import(oIS, sFileName);
}

void CScriptBase::GetLine(std::istream& oIS, std::string& sBuff, int32_t& nLineCounter)
{
    std::string sLine;
    size_t p;
    sBuff = "";
    getline(oIS, sLine);
    if (oIS.fail())
        return;
    while (!sLine.empty() && sLine[sLine.size() - 1] <= 32 && sLine[sLine.size() - 1] > 0)
        sLine.erase(sLine.size() - 1, 1);
    while (!sLine.empty() && sLine[sLine.size() - 1] == '\\') {
        sLine.erase(sLine.size() - 1, 1);
        sBuff += sLine;
        sLine = "";
        nLineCounter++;
        getline(oIS, sLine);
        if (oIS.fail())
            break;
        while (!sLine.empty() && sLine[sLine.size() - 1] <= 32 && sLine[sLine.size() - 1] > 0)
            sLine.erase(sLine.size() - 1, 1);
        p = sLine.find_first_not_of(" \t");
        if (p == std::string::npos) {
            p = sLine.size();
        }
        sLine.erase(0, p);
    }
    sBuff += sLine;
}

bool CScriptBase::Import(std::istream& oIS, const std::string& sFileName)
{
    std::string sLine;
    int32_t nLineNumber = 0;
    std::shared_ptr<CharacterMapper> pMapper(new CharacterMapper("iso-8859-1", "iso-8859-1"));
    Value oVal;
    size_t p;

    while (1) {
        GetLine(oIS, sLine, nLineNumber);
        if (oIS.fail())
            break;
        if (!nLineNumber && sLine.length() >= 3 && sLine.substr(0, 3) == "\xEF\xBB\xBF") {
            pMapper.reset(new CharacterMapper("utf-8", "iso-8859-1"));
            sLine.erase(0, 3);
        }

        p = sLine.find_first_of(';');
        if (p != std::string::npos) {
            sLine.erase(p);
        }
        while (!sLine.empty() && sLine[sLine.size() - 1] <= 32 && sLine[sLine.size() - 1] > 0)
            sLine.erase(sLine.size() - 1, 1);
        nLineNumber++;
        p = sLine.find_first_not_of(" \t");
        if (p == std::string::npos) {
            p = sLine.size();
        }
        sLine.erase(0, p);
        if (IsFlag(VF_FIXENCODINGS)) {
            sLine = mixed_utf8_latin1_to_latin1(sLine);
        }
        if (!sLine.empty()) {
            sLine = pMapper->Map(sLine);
            if (!strncmp("#encoding", sLine.c_str(), 5)) {
                std::string sEnc = sLine.substr(9);
                p = sEnc.find_first_not_of(" \t");
                if (p == std::string::npos) {
                    p = sEnc.size();
                }
                sEnc.erase(0, p);
                p = sEnc.find_last_not_of(" \t");
                if (p != std::string::npos) {
                    sEnc.erase(p + 1);
                }
                if (!CharacterMapper::IsSupported(sEnc) && !(IsEqual(sEnc, "utf-8") || IsEqual(sEnc, "utf8"))) {
                    ERRMSG(0, ("%s(%d) : FEHLER: Encoding '%s' wird nicht unterstuetzt!", sFileName.c_str(), nLineNumber, sEnc.c_str()));
                }
                else
                    pMapper.reset(new CharacterMapper(sEnc.c_str(), "iso-8859-1"));
            }
            else if (!strncmp("#proc", sLine.c_str(), 5)) {
                nLineNumber += AddProc(pMapper.get(), sFileName, nLineNumber, oIS, sLine, false);
            }
            else if (!strncmp("#func", sLine.c_str(), 5)) {
                nLineNumber += AddProc(pMapper.get(), sFileName, nLineNumber, oIS, sLine, true);
            }
            else if (!strncmp("#const", sLine.c_str(), 6)) {
                std::vector<std::string> coArgs;
                CRegExp oRE;
                int pp = 0;
                oRE.Prepare("([^ \\t\\n\\r\\f']|'([^']|\\')+')+");
                while (oRE.Find(sLine, pp)) {
                    coArgs.push_back(sLine.substr((size_t)oRE.Begin(), (size_t)oRE.Size()));
                    pp = oRE.End() + 1;
                }

                if (coArgs.size() != 3) {
                    ERRMSG(0, ("%s(%d) : FEHLER: Falsche Anzahl von Argumenten fuer '#const <name> <value>'!", sFileName.c_str(), nLineNumber));
                }
                else {
                    if (IsIdentifier(coArgs[1], false)) {
                        Expression oExp(coArgs[2], sFileName.c_str(), nLineNumber);
                        Value oValt;
                        int dummy;
                        oExp.evaluate(Expression::globalContext(), coArgs[2].c_str(), &oValt, &dummy, false);
                        Expression::setConstant(coArgs[1].c_str(), &oValt);
                    }
                    else {
                        ERRMSG(0, ("%s(%d) : FEHLER: Ungueltiger Bezeichner '%s' fuer '#const <name> <value>'!", sFileName.c_str(), nLineNumber, coArgs[1].c_str()));
                    }
                }
            }
            else if (!strncmp("#var", sLine.c_str(), 4)) {
                std::string sName = sLine.substr(4);
                do {
                    p = sName.find_first_not_of(" \t");
                    if (p == std::string::npos) {
                        p = sName.size();
                    }
                    sName.erase(0, p);
                    p = sName.find_first_of(" \t");
                    if (p == std::string::npos) {
                        p = sName.size();
                    }
                    if (IsIdentifier(sName.substr(0, p))) {
                        if (!Expression::getGlobalRef(sName.substr(0, p).c_str())) {
                            Expression::setGlobal(sName.substr(0, p).c_str(), &oVal, false);
                        }
                        else {
                            ERRMSG(0, ("%s(%d) : FEHLER: Im globalen Kontext existiert bereits ein Bezeichner '%s'!", sFileName.c_str(), nLineNumber, sName.substr(0, p).c_str()));
                        }
                    }
                    else {
                        ERRMSG(0, ("%s(%d) : FEHLER: '%s' ist kein gueltiger Bezeichner!", sFileName.c_str(), nLineNumber, sName.substr(0, p).c_str()));
                    }
                    sName.erase(0, p);
                } while (!sName.empty());
            }
            else if (!strncmp("#array", sLine.c_str(), 6)) {
                std::string sName = sLine.substr(6);
                do {
                    p = sName.find_first_not_of(" \t");
                    if (p == std::string::npos) {
                        p = sName.size();
                    }
                    sName.erase(0, p);
                    p = sName.find_first_of(" \t");
                    if (p == std::string::npos) {
                        p = sName.size();
                    }
                    if (IsIdentifier(sName.substr(0, p))) {
                        if (!Expression::getGlobalRef(sName.substr(0, p).c_str())) {
                            Value oVect(VT_VECTOR);
                            Expression::setGlobal(sName.substr(0, p).c_str(), &oVect, false);
                        }
                        else {
                            ERRMSG(0, ("%s(%d) : FEHLER: Im globalen Kontext existiert bereits ein Bezeichner '%s'!", sFileName.c_str(), nLineNumber, sName.substr(0, p).c_str()));
                        }
                    }
                    else {
                        ERRMSG(0, ("%s(%d) : FEHLER: '%s' ist kein gueltiger Bezeichner!", sFileName.c_str(), nLineNumber, sName.substr(0, p).c_str()));
                    }
                    sName.erase(0, p);
                } while (!sName.empty());
            }
            else if (!strncmp("#dict", sLine.c_str(), 5)) {
                std::string sName = sLine.substr(5);
                do {
                    p = sName.find_first_not_of(" \t");
                    if (p == std::string::npos) {
                        p = sName.size();
                    }
                    sName.erase(0, p);
                    p = sName.find_first_of(" \t");
                    if (p == std::string::npos) {
                        p = sName.size();
                    }
                    if (IsIdentifier(sName.substr(0, p))) {
                        if (!Expression::getGlobalRef(sName.substr(0, p).c_str())) {
                            Value oMap(VT_MAP);
                            Expression::setGlobal(sName.substr(0, p).c_str(), &oMap, false);
                        }
                        else {
                            ERRMSG(0, ("%s(%d) : FEHLER: Im globalen Kontext existiert bereits ein Bezeichner '%s'!", sFileName.c_str(), nLineNumber, sName.substr(0, p).c_str()));
                        }
                    }
                    else {
                        ERRMSG(0, ("%s(%d) : FEHLER: '%s' ist kein gueltiger Bezeichner!", sFileName.c_str(), nLineNumber, sName.substr(0, p).c_str()));
                    }
                    sName.erase(0, p);
                } while (!sName.empty());
            }
            else if (!strncmp("#include", sLine.c_str(), 8)) {
                std::string sFName = sLine.substr(8);
                p = sFName.find_first_not_of(" \t");
                if (p == std::string::npos) {
                    p = sFName.size();
                }
                sFName.erase(0, p);
                p = sFName.find_last_not_of(" \t");
                if (p != std::string::npos) {
                    sFName.erase(p + 1);
                }

                if (!Import(sFName))
                    return false;
            }
            else if (!strncmp("#", sLine.c_str(), 1)) {
                std::string sBef;
                p = sLine.find_first_of(" \t;");
                if (p != std::string::npos)
                    sBef = sLine.substr(0, p);
                else
                    sBef = sLine;
                ERRMSG(0, ("%s(%d) : FEHLER: Unbekannter oder ausserhalb von Funktionen oder Prozeduren unerlaubter Befehl '%s'!", sFileName.c_str(), nLineNumber, sBef.c_str()));
                return false;
            }
            else if (sLine[0] != ';') {
                ERRMSG(0, ("%s(%d) : FEHLER: Unerwartete Zeichen: %s", sFileName.c_str(), nLineNumber, sLine.c_str()));
                return false;
            }
        }
    }

    //  fprintf( stderr, "\n");
    //  for( COMMANDBASE::iterator i=m_cpoSubs.begin(); i!=m_cpoSubs.end(); i++ )
    //  {
    //      fprintf( stderr, "[%s] %s\n", (*i).first.c_str(), (*i).second->asString().c_str() );
    //  }
    //  fprintf( stderr, "\n");
    return true;
}

CScriptBase::~CScriptBase()
{
    for (COMMANDBASE::iterator i = m_cpoSubs.begin(); i != m_cpoSubs.end(); i++) {
        delete (*i).second;
    }
    m_cpoSubs.clear();
}

int32_t CScriptBase::AddProc(const CharacterMapper* pMapper, const std::string& sFileName, int32_t nLine, std::istream& oIS, std::string& sLine, bool bIsFunc)
{
    CMetaCommand* pMC;
    int32_t nLineNumber = 0;
    int nBC = 0;
    int i;
    size_t p;
    bool bVArg = false;
    bool bVRef = false;

    while (!sLine.empty() && sLine[sLine.size() - 1] <= 32 && sLine[sLine.size() - 1] > 0)
        sLine.erase(sLine.size() - 1, 1);
    p = sLine.find_first_of("'");
    if (p != std::string::npos)
        sLine.erase(p);
    if (sLine.substr(sLine.length() - 3) == "...") {
        if (sLine.substr(sLine.length() - 4) == "&...") {
            if (bIsFunc)
                ERRMSG(0, ("%s(%d) : FEHLER: Funktionen koennen nicht mit Referenzparametern arbeiten!", sFileName.c_str(), nLine));
            else
                bVRef = true;
        }

        if (strncmp("#proc", sLine.c_str(), 5)) {
            ERRMSG(0, ("%s(%d) : FEHLER: Es wurde '...' ohne #proc verwendet!", sFileName.c_str(), nLine));
        }
        if (bVRef)
            sLine.erase(sLine.length() - 4);
        else
            sLine.erase(sLine.length() - 3);
        bVArg = true;
    }
    pMC = new CMetaCommand(sLine, nLine, bIsFunc, bVRef);
    pMC->SetFile(sFileName);
    // Es geht nicht, wenn nach #proc eine Leerzeile kommt!!!
    do {
        do {
            GetLine(oIS, sLine, nLineNumber);
            if (oIS.fail())
                break;
            nLineNumber++;
            p = 0;
            do {
                p = sLine.find_first_of("'\\;", p);
                if (p != std::string::npos) {
                    if (sLine[p] == '\\')
                        p += 2;
                    else if (sLine[p] == '\'') {
                        p = sLine.find_first_of("'", p + 1);
                        if (p != std::string::npos)
                            p++;
                    }
                }
            } while (p != std::string::npos && sLine[p] != ';');

            if (p != std::string::npos) {
                sLine.erase(p);
            }
            while (!sLine.empty() && sLine[sLine.size() - 1] < 32 && sLine[sLine.size() - 1] > 0)
                sLine.erase(sLine.size() - 1, 1);
        } while (sLine.empty());

        if (oIS.fail())
            break;

        p = sLine.find_first_not_of(" \t");
        if (p == std::string::npos) {
            p = sLine.size();
        }
        sLine.erase(0, p);

        sLine = pMapper->Map(sLine);

        if (!sLine.empty() && sLine[0] != ';')
            nBC += pMC->AddScript(sLine, nLine + nLineNumber);
    } while (nBC > 0);

    if (nBC < 0) {
        ERRMSG(0, ("%s(%d) : FEHLER: Ein ueberzaehliges '}' gefunden!", sFileName.c_str(), nLine + nLineNumber));
        delete pMC;
        return nLineNumber;
    }

    i = 2;
    while (i <= (int32_t)pMC->Args()) {
        if (bIsFunc && !(*pMC)[(size_t)i].empty() && (*pMC)[(size_t)i][0] == '&')
            ERRMSG(0, ("%s(%d) : FEHLER: Funktionen koennen nicht mit Referenzparametern arbeiten!", sFileName.c_str(), nLine));
        if ((*pMC)[(size_t)i] == "{")
            break;
        i++;
    }

    (*pMC)[0] = fmt::format("{}", bVArg ? -(i - 1) : i - 1);

    if (pMC->Args() >= 2) {
        COMMANDBASE::iterator cbi = m_cpoSubs.find((*pMC)[1]);
        if (cbi != m_cpoSubs.end()) {
            if ((*cbi).second->GetFile().rfind("<internal", 0)) {
                if ((*cbi).second->GetFile() == sFileName) {
                    ERRMSG(0,
                           ("%s(%d) : Warnung: Mehrfache Definition der %s %s in Datei '%s', die letzte wird verwendet!", sFileName.c_str(), nLine, (*cbi).second->IsFunction() ? "Funktion" : "Prozedur", (*((*cbi).second))[1].c_str(), sFileName.c_str()));
                }
                else {
                    ERRMSG(0, ("%s(%d) : Warnung: Mehrfache Definition der %s %s in Datei '%s' und '%s', die letzte wird verwendet!", sFileName.c_str(), nLine, (*cbi).second->IsFunction() ? "Funktion" : "Prozedur", (*((*cbi).second))[1].c_str(),
                               (*cbi).second->GetFile().c_str(), sFileName.c_str()));
                }
            }
            delete (*cbi).second;
            (*cbi).second = pMC;
        }
        else {
            m_cpoSubs.insert(COMMANDBASE::value_type((*pMC)[1], pMC));
        }
    }
    else
        delete pMC;

    return nLineNumber;
}

CMetaCommand* CScriptBase::FindProc(const std::string& sPName)
{
    COMMANDBASE::iterator cbi = m_cpoSubs.find(sPName);
    if (cbi != m_cpoSubs.end()) {
        return (*cbi).second;
    }
    return 0;
}
