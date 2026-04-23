/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/Vorlage/Zugvorlage.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:56:47 $
 * $Revision: 1.14 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Algemeine Utility-Funktionen
 *****************************************************************************
 *
 * $Log: Zugvorlage.cpp,v $
 * Revision 1.14  2000/02/24 09:56:47  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.13  1999/11/28 17:38:41  S.Schuemann
 * - Mannigfaltige Änderungen für Vorlage V1.4 beta 9
 *
 * Revision 1.12  1999/11/17 08:59:13  S.Schuemann
 * - support für multiple CRs
 *
 * - vielfache Änderungen für Vorlage 1.4 beta 8
 *
 * Revision 1.11  1999/11/08 11:14:46  S.Schuemann
 * - Attribute region.gewinn, region[x,y], unit.bewache,
 *   region.pool.ding
 * - Ecaping in Strings
 * - Vars in Attributzugriffen
 * - Funktionen
 * - Sortierung nach BESCHREIBE PRIVAT
 * - Liste fremder Einheiten (normal/verbose)
 * - Bugfix: Leerzeile nach Kapitänsinfo entfernt
 *
 * Revision 1.10  1999/11/03 10:21:57  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.9  1999/10/28 12:39:58  S.Schuemann
 * - Änderungen für den Linux-Port
 *
 * Revision 1.8  1999/10/26 13:27:02  S.Schuemann
 * - Anpassungen fuer Vorlage 1.4 b 5
 *
 * - Neue Objekt-Attribute
 *
 * - User-Variable und #while
 *
 * Revision 1.7  1999/10/24 08:02:19  S.Schuemann
 * - Anpassungen fuer 1.4 b 4
 * - CReference als Value eingefuehrt
 * - Unterprogramme mit #proc und #call implementiert
 *
 * Revision 1.6  1999/10/20 02:19:39  S.Schuemann
 * - Die neue Option '-hb' erlaubt es, zu Beginn der Zugvorlage
 *   eine Handelsübersicht, nach Parteien und Produkten einzuf�gen,
 *   um den Überblick zu behalten
 *
 * - Die neue Option '-si' erlaubt es, die Regionen nach
 *   Inselzugehörigkeit zu sortieren, statt nach Report-
 *   Reihenfolge, dabei liegen alle Regionen beisammen, die
 *   miteinander Verbunden sind
 *
 * - Die neue Option '-ox ext' leitet die Zugvorlage, wie die
 *   Option '-o filename' in eine Datei um, die aber den selben
 *   Basisnamen wie der Bezugsreport hat, aber die Datei-
 *   erweiterung ext bekommt; Der Report muß die Endung '.cr'
 *   haben (wie ja üblich)
 *
 * - Die neue Option '-pb' zeigt BESCHREIBE-PRIVAT-Inhalte in
 *   der Vorlage an
 *
 * - Bei Schiffen steht nun auch die freie und die theoretische
 *   Kapazität
 *
 * - In den Regionsinfos steht nun der von den Bauern
 *   erwirtschaftete Gewinn, also die Menge, die man maximal
 *   Abschöpfen kann, ohne die Regionsreserven zu gefährden
 *
 * - In der REGION-Zeile steht nun auch noch der Geländetyp
 *
 * Revision 1.5  1999/10/18 21:32:20  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.4  1999/09/29 08:04:24  S.Schuemann
 * - Fehler in der Kapitaensbehandlung fuehrte zum Absturz
 *
 * Revision 1.3  1999/09/27 10:31:20  S.Schuemann
 * - Final 1.3
 * - Kampfstatus wird bei Einheiten angezeigt
 * - Schiff und Ablegekueste werden beim Kapitaen angezeigt
 * - Anpassungen durch neue Klasse CReport statt CWorldDB mit
 *   leicht veraenderten Zustaendigkeiten
 *
 * Revision 1.2  1999/09/20 15:02:33  Steffen
 * - kosmetische Aenderungen am Source;
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#ifdef _MSC_VER

#include <new.h>

#include <new>

_PNH _old_new_handler;

int my_new_handler(size_t)
{
    throw std::bad_alloc();
    return 0;
}

#else
#endif

#include <EBase/Expression.h>
#include <EBase/Report.h>
#include <EBase/ReportStream.h>
#include <EBase/Utility.h>
#include <EBase/Value.h>
#include <EBase/charencoding.h>
#include <EBase/hierarchy.h>
#include <EBase/regexp.h>
#include <pbemtools/version.h>

#include <algorithm>
#include <clocale>
#include <cstring>
#include <ctime>
#include <ios>
#include <sstream>

#include "CRNE.h"
#include "Metascript.h"
#include "Zugvorlage.h"
#include "crashdumphandler.h"

#define VERSIONINFO "Vorlage " PBEMTOOLS_VERSION_STRING_LONG

int g_nLineSize = 100;
int g_nCommandLineSize = g_nLineSize;
int g_nNumErrors = 0;
int g_nNumWarnings = 0;
int32_t g_nPassNum = 0;
int32_t g_nMinPasses = 0;
int32_t g_nTimeCorrection = 0;

// FILE* g_hOut = NULL;
// FILE* g_hErr = NULL;
// FILE* g_hDeb = NULL;

bool g_bFile = false;
bool g_bError = false;
bool g_bDebug = false;
bool g_bWait = false;

extern int32_t g_nContainerLimit;

std::string g_sConfigPathName;
std::string g_sSpiel = "vorlage";
std::string g_sCmdOptions = "Aufruf:";

std::set<std::string> g_csDupDescrFilter;

int32_t g_nOnlyGroup = -1;

std::string GetConfigFileName()
{
    return g_sConfigPathName;
}

void SetConfigFileName(const std::string& sFName)
{
    if (FileExists(sFName))
        g_sConfigPathName = sFName;
}

static void EXIT(int32_t nCode)
{
    if (g_bWait && !IsFlag(VF_NOCONSOLE)) {
        char Buff[32];
        CONMSG(("Fertig, weiter mit der Eingabetaste.\n"));
        auto res = fgets(Buff, 28, stdin);
        if (!res) {
            exit(-1);
        }
    }
#ifdef _MSC_VER
    _set_new_handler(_old_new_handler);
#endif
    exit(nCode);
}

static void VorlageErrorMsg(void* pDat, const char* pszTxt)
{
    CMetaCommand::SetErrMsg(pszTxt);

    if (IsFlag(VF_TRACEONERROR)) {
        CMetaCommand::SetTrace(2);
    }
    if (pDat) {
        VKommandos* poCmd = (VKommandos*)pDat;
        poCmd->push_back(std::string("; ") + std::string(pszTxt));
    }
    if (CRegExp::Match(pszTxt, "(?i)(Fehler|Error):"))
        g_nNumErrors++;
    if (CRegExp::Match(pszTxt, "(?i)(Warnung|Warning):"))
        g_nNumWarnings++;
}

static void MyBreak(void)
{
    CMetaCommand::SetTrace(2);
}

static void WrapOut(const std::string& sPfx, const std::string& sText, size_t nLen, const std::string sFirstPfx = "")
{
    std::string sTxt = sText;
    std::string sFPfx = sFirstPfx;
    int c = 0;
    if (sFirstPfx.empty())
        sFPfx = sPfx;
    do {
        COutput::TPrint("vorlage", "{}{}\n", c ? sPfx : sFPfx, Wrap(sTxt, nLen - (c ? sPfx.size() : sFPfx.size())));
        c++;
    } while (!sTxt.empty());
}

const char* pcJahr[] = {"Dezember", "Januar", "Februar", "Maerz", "April", "Mai", "Juni", "Juli", "August", "September", "Oktober", "November"};

const char* pcJahr2[] = {"Feldsegen", "Nebeltage", "Sturmmond", "Herdfeuer", "Eiswind", "Schneebann", "Bl\xFCtenregen", "Mond der milden Winde", "Sonnenfeuer"};

void CVorlage::InitDBS(int32_t nRunde, std::vector<CReport*>& cpoReports)
{
    CKarte::RegionMap::const_iterator rmi;
    RegionDB::iterator rdbi;
    RegEinheitenDB::iterator redbi;
    EinheitenDB::iterator ui;
    unsigned int nRep, nEIdx;

    for (nRep = 0; nRep < cpoReports.size(); nRep++) {
        //		TRACEMSG(( "Analysiere Report %s\n", cpoReports[nRep]->m_sOrgCRName.c_str() ));
        for (rmi = cpoReports[nRep]->Karte()->Regions().begin(); rmi != cpoReports[nRep]->Karte()->Regions().end(); rmi++) {
            // Region in DB eintragen
            if ((*rmi).second->GetBlock() <= CRegion::enSCHEMEN) {
                rdbi = g_coRDB.find((*rmi).second->GetKey());
                if (rdbi == g_coRDB.end())
                    rdbi = g_coRDB.insert(RegionDB::value_type((*rmi).second->GetKey(), RegionSet())).first;
                (*rdbi).second.insert((*rmi).second);

                // Rundenbasierte Region-DB
                // g_coRRegionDB[nRunde-cpoReports[nRep]->Runde()].insert( (*rmi).second );
                rdbi = g_coRRegionDB[nRunde - cpoReports[nRep]->Runde()].find((*rmi).second->GetKey());
                if (rdbi == g_coRRegionDB[nRunde - cpoReports[nRep]->Runde()].end())
                    rdbi = g_coRRegionDB[nRunde - cpoReports[nRep]->Runde()].insert(RegionDB::value_type((*rmi).second->GetKey(), RegionSet())).first;
                (*rdbi).second.insert((*rmi).second);
            }

            // Einheiten in DB eintragen
            redbi = g_coREDB.find((*rmi).second->GetKey());
            if (redbi == g_coREDB.end()) {
                redbi = g_coREDB.insert(RegEinheitenDB::value_type((*rmi).second->GetKey(), EinheitenDB())).first;
            }
            for (nEIdx = 0; nEIdx < (*rmi).second->GetVEinheiten().size(); nEIdx++) {
                // Eintragung in Region-UnitDB
                // TODO: Check if this is right
                if (true /*|| cpoReports[nRep]->Runde() == nRunde*/) {
                    ui = (*redbi).second.find((*rmi).second->GetVEinheiten()[nEIdx]->Nummer());
                    if (ui == (*redbi).second.end()) {
                        (*redbi).second.insert(EinheitenDB::value_type((*rmi).second->GetVEinheiten()[nEIdx]->Nummer(), (*rmi).second->GetVEinheiten()[nEIdx]));
                        //					COutput::TPrintf( "vorlage", "    Neue Einheit: %s\n", itoa36( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer() ) );
                    }
                    else {
                        if ((*ui).second->GetQuality() < (*rmi).second->GetVEinheiten()[nEIdx]->GetQuality()) {
                            (*ui).second = (*rmi).second->GetVEinheiten()[nEIdx];
                            //						COutput::TPrintf( "vorlage", "    Bessere Info: %s\n", itoa36( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer() ) );
                        }
                    }
                }
                // Eintragung in Global-UnitDB
                ui = g_coEDB.find((*rmi).second->GetVEinheiten()[nEIdx]->Nummer());
                if (ui == g_coEDB.end()) {
                    g_coEDB.insert(EinheitenDB::value_type((*rmi).second->GetVEinheiten()[nEIdx]->Nummer(), (*rmi).second->GetVEinheiten()[nEIdx]));
                }
                else {
                    if ((*ui).second->GetQuality() < (*rmi).second->GetVEinheiten()[nEIdx]->GetQuality()) {
                        (*ui).second = (*rmi).second->GetVEinheiten()[nEIdx];
                    }
                }

                // Rundenbasierte Einheiten-DB
                ui = g_coREinheitenDB[nRunde - cpoReports[nRep]->Runde()].find((*rmi).second->GetVEinheiten()[nEIdx]->Nummer());
                if (ui == g_coREinheitenDB[nRunde - cpoReports[nRep]->Runde()].end()) {
                    g_coREinheitenDB[nRunde - cpoReports[nRep]->Runde()].insert(EinheitenDB::value_type((*rmi).second->GetVEinheiten()[nEIdx]->Nummer(), (*rmi).second->GetVEinheiten()[nEIdx]));
                }
                else {
                    if ((*ui).second->GetQuality() < (*rmi).second->GetVEinheiten()[nEIdx]->GetQuality()) {
                        (*ui).second = (*rmi).second->GetVEinheiten()[nEIdx];
                    }
                }
            }

            /*
                            if( cpoReports[nRep]->Runde() == nRunde )
                            {
            //  			COutput::TPrintf( "vorlage", "  Region %s\n", (*rmi).second->m_sName.c_str() );
                                            // Region in DB eintragen
                            if( (*rmi).second->GetBlock()<=CRegion::enSCHEMEN )
                            {
                                                rdbi = g_coRDB.find( (*rmi).second->GetKey() );
                                                if( rdbi == g_coRDB.end() )
                                                {
                                                        g_coRDB.insert( RegionDB::value_type( (*rmi).second->GetKey(), (*rmi).second ) );
                                                }
                                                else
                                                {
                                    if( (*rdbi).second->m_nRunde != nRunde && (*rmi).second->m_nRunde == nRunde )
                                        (*rdbi).second = (*rmi).second;
                                                        else if( (*rdbi).second->GetQuality() < (*rmi).second->GetQuality() )
                                                                (*rdbi).second = (*rmi).second;
                                                }
                            }
                                            // Einheiten in DB eintragen
                                            redbi = g_coREDB.find( (*rmi).second->GetKey() );
                                            if( redbi == g_coREDB.end() )
                                            {
                                                    redbi = g_coREDB.insert( RegEinheitenDB::value_type( (*rmi).second->GetKey(), EinheitenDB() ) ).first;
                                            }
                                            for( nEIdx = 0; nEIdx < (*rmi).second->GetVEinheiten().size(); nEIdx++ )
                                            {
                                                    // Eintragung in Region-UnitDB
                                                    ui = (*redbi).second.find( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer() );
                                                    if( ui == (*redbi).second.end() )
                                                    {
                                                            (*redbi).second.insert( EinheitenDB::value_type( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer(), (*rmi).second->GetVEinheiten()[nEIdx] ) );
                    //					COutput::TPrintf( "vorlage", "    Neue Einheit: %s\n", itoa36( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer() ) );
                                                    }
                                                    else
                                                    {
                                                            if( (*ui).second->GetQuality() < (*rmi).second->GetVEinheiten()[nEIdx]->GetQuality() )
                                                            {
                                                                    (*ui).second = (*rmi).second->GetVEinheiten()[nEIdx];
                    //						COutput::TPrintf( "vorlage", "    Bessere Info: %s\n", itoa36( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer() ) );
                                                            }
                                                    }

                                                    // Eintragung in Global-UnitDB
                                                    ui = g_coEDB.find( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer() );
                                                    if( ui == g_coEDB.end() )
                                                    {
                                                            g_coEDB.insert( EinheitenDB::value_type( (*rmi).second->GetVEinheiten()[nEIdx]->Nummer(), (*rmi).second->GetVEinheiten()[nEIdx] ) );
                                                    }
                                                    else
                                                    {
                                                            if( (*ui).second->GetQuality() < (*rmi).second->GetVEinheiten()[nEIdx]->GetQuality() )
                                                            {
                                                                    (*ui).second = (*rmi).second->GetVEinheiten()[nEIdx];
                                                            }
                                                    }
                                            }
                                    }
                        else
                        {
                                        // Region in DB eintragen
                                        rdbi = g_coRDB.find( (*rmi).second->GetKey() );
                                        if( rdbi == g_coRDB.end() )
                                        {
                                                g_coRDB.insert( RegionDB::value_type( (*rmi).second->GetKey(), (*rmi).second ) );
                                        }
                                        else
                                        {
                                                if( (*rdbi).second->GetQuality() < (*rmi).second->GetQuality() )
                                                        (*rdbi).second = (*rmi).second;
                                        }
                        }
            */
        }
    }
}

void CVorlage::Islandize(CKarte::IslandQueue& cpoQueue, RegionDB& coRDB)
{
    RegionDB::iterator rdbi;
    CRegion* poReg;
    CRegion* poR2;
    int32_t x, y, z;

    while (!cpoQueue.empty()) {
        poReg = cpoQueue.front();
        /*
                if( poReg && poReg->GetValue("herb").asString().size()>2 )
                {
                    rdbi = coRDB.find( poReg->GetKey() );
                                if( rdbi != coRDB.end() )
                                {
                                        (*rdbi).second->SetValue( "herb", poReg->GetValue("herb") );
                                }
                }
        */
        if (poReg && !poReg->GetIslandName().empty()) {
            //			fprintf( stderr, "Region: %s\n", poReg->m_sName.c_str() );

            rdbi = coRDB.find(poReg->GetKey());
            if (rdbi != coRDB.end()) {
                (*(*rdbi).second.begin())->SetIslandName(poReg->GetIslandName());
            }

            x = poReg->GetEX();
            y = poReg->GetEY();
            z = poReg->GetEZ();

            // Nordosten
            rdbi = coRDB.find(CRegion::CalcKey(x, y + 1, z));
            if (rdbi != coRDB.end()) {
                poR2 = *(*rdbi).second.begin();
                if (poR2->GetIslandName().empty() && poR2->IsLand()) {
                    poR2->SetIslandName(poReg->GetIslandName());
                    cpoQueue.push_back(poR2);
                }
            }
            // Osten
            rdbi = coRDB.find(CRegion::CalcKey(x + 1, y, z));
            if (rdbi != coRDB.end()) {
                poR2 = *(*rdbi).second.begin();
                if (poR2->GetIslandName().empty() && poR2->IsLand()) {
                    poR2->SetIslandName(poReg->GetIslandName());
                    cpoQueue.push_back(poR2);
                }
            }
            // Suedosten
            rdbi = coRDB.find(CRegion::CalcKey(x + 1, y - 1, z));
            if (rdbi != coRDB.end()) {
                poR2 = *(*rdbi).second.begin();
                if (poR2->GetIslandName().empty() && poR2->IsLand()) {
                    poR2->SetIslandName(poReg->GetIslandName());
                    cpoQueue.push_back(poR2);
                }
            }
            // Suedwesten
            rdbi = coRDB.find(CRegion::CalcKey(x, y - 1, z));
            if (rdbi != coRDB.end()) {
                poR2 = *(*rdbi).second.begin();
                if (poR2->GetIslandName().empty() && poR2->IsLand()) {
                    poR2->SetIslandName(poReg->GetIslandName());
                    cpoQueue.push_back(poR2);
                }
            }
            // Westen
            rdbi = coRDB.find(CRegion::CalcKey(x - 1, y, z));
            if (rdbi != coRDB.end()) {
                poR2 = *(*rdbi).second.begin();
                if (poR2->GetIslandName().empty() && poR2->IsLand()) {
                    poR2->SetIslandName(poReg->GetIslandName());
                    cpoQueue.push_back(poR2);
                }
            }
            // Nordwesten
            rdbi = coRDB.find(CRegion::CalcKey(x - 1, y + 1, z));
            if (rdbi != coRDB.end()) {
                poR2 = *(*rdbi).second.begin();
                if (poR2->GetIslandName().empty() && poR2->IsLand()) {
                    poR2->SetIslandName(poReg->GetIslandName());
                    cpoQueue.push_back(poR2);
                }
            }
        }
        cpoQueue.pop_front();
    }
}

#define PIPRINT         \
    g_bForceEOL = true; \
    COutput::Target("console")->Printf

void CVorlage::RunMetacommands(CReport& oReport)
{
    CRegion* poReg = nullptr;
    const CRegion::VEinheiten* poVE = nullptr;
    CEinheit* poUnit = nullptr;
    int32_t nUnitCnt = 0;
    bool bDoneReg = false;
    char pcPass[16];
    //	Expression::clearAllVars();

    m_nPlayer = oReport.Partei();
    m_poKarte = oReport.Karte();
    g_poCurrentReport = &oReport;
    g_poKarte = m_poKarte;

    if (g_nMinPasses)
        snprintf(pcPass, sizeof(pcPass), "Pass %ld: ", g_nPassNum);
    else
        pcPass[0] = 0;

    if (IsFlag(VF_PROGRESSINFO)) {
        if (g_nPassNum <= 1)
            TRACEMSG(("\n"));
        PIPRINT("\r%sEinheiten: %3d%% - OnInit", pcPass, nUnitCnt * 100 / m_nUnits);
    }
    Value vCurrentMeta(-1);
    Expression::setGlobal("$CURRENTMETA", &vCurrentMeta);

    CMetaCommand::Call(std::string("OnInit"), m_coInitCmd);

    //	for( CKarte::RegionMap::const_iterator rmi = m_poKarte->Regions().begin(); rmi != m_poKarte->Regions().end(); rmi++ )
    for (RegionDB::iterator rmi = g_coRDB.begin(); rmi != g_coRDB.end(); rmi++) {
        //		cpoRegions.push_back((*rmi).second);
        //		poReg = (*rmi).second;
        poReg = *((*rmi).second.begin());
        /*
                if( poReg->DeepGetValue("visibility").getType() == VT_STRING )
                {
                    TRACEMSG(( "%s/%s\n", poReg->GetName().c_str(), poReg->GetRegionTypeName().c_str() ));
                }
        */
        vCurrentMeta = Value(-1);
        Expression::setGlobal("$CURRENTMETA", &vCurrentMeta);

        m_poCurrentRegion = poReg;
        g_poCurrentRegion = poReg;
        if (poReg->Map() == m_poKarte || poReg->DeepGetValue("visibility").getType() == VT_STRING) {
            poVE = &poReg->GetVEinheiten();
            bDoneReg = false;
            if (IsFlag(VF_RUNALLVISIBLEREGIONS)) {
                if (IsFlag(VF_PROGRESSINFO)) {
                    if (poReg->GetEZ()) {
                        PIPRINT("\r%sEinheiten: %3d%% - OnRegion(%d,%d,%d)", pcPass, nUnitCnt * 100 / m_nUnits, poReg->GetEX(), poReg->GetEY(), poReg->GetEZ());
                    }
                    else {
                        PIPRINT("\r%sEinheiten: %3d%% - OnRegion(%d,%d)", pcPass, nUnitCnt * 100 / m_nUnits, poReg->GetEX(), poReg->GetEY());
                    }
                }
                CMetaCommand::Call(std::string("OnRegion"), poReg->GetKommandos());
                if (poReg->GetVBauwerke()) {
                    for (CRegion::VBauwerke::const_iterator bi = poReg->GetVBauwerke()->begin(); bi != poReg->GetVBauwerke()->end(); ++bi) {
                        g_poCurrentBuilding = *bi;
                        CMetaCommand::Call(std::string("OnBuilding"), (*bi)->GetKommandos());
                        g_poCurrentBuilding = 0;
                    }
                }
                if (poReg->GetVSchiffe()) {
                    for (CRegion::VSchiffe::const_iterator si = poReg->GetVSchiffe()->begin(); si != poReg->GetVSchiffe()->end(); ++si) {
                        g_poCurrentShip = *si;
                        CMetaCommand::Call(std::string("OnShip"), (*si)->GetKommandos());
                        g_poCurrentShip = 0;
                    }
                }
                if (IsFlag(VF_PROGRESSINFO)) {
                    PIPRINT("\r                                                                      ");
                }
                bDoneReg = true;
            }
            for (unsigned i = 0; i < poVE->size(); i++) {
                if (poVE->operator[](i)->Partei() == m_nPlayer && !poVE->operator[](i)->GetValue("Verraeter")) {
                    if (!bDoneReg) {
                        if (IsFlag(VF_PROGRESSINFO)) {
                            if (poReg->GetEZ()) {
                                PIPRINT("\r%sEinheiten: %3d%% - OnRegion(%d,%d,%d)", pcPass, nUnitCnt * 100 / m_nUnits, poReg->GetEX(), poReg->GetEY(), poReg->GetEZ());
                            }
                            else {
                                PIPRINT("\r%sEinheiten: %3d%% - OnRegion(%d,%d)", pcPass, nUnitCnt * 100 / m_nUnits, poReg->GetEX(), poReg->GetEY());
                            }
                        }
                        CMetaCommand::Call(std::string("OnRegion"), poReg->GetKommandos());
                        if (poReg->GetVBauwerke()) {
                            for (CRegion::VBauwerke::const_iterator bi = poReg->GetVBauwerke()->begin(); bi != poReg->GetVBauwerke()->end(); ++bi) {
                                g_poCurrentBuilding = *bi;
                                CMetaCommand::Call(std::string("OnBuilding"), (*bi)->GetKommandos());
                                g_poCurrentBuilding = 0;
                            }
                        }
                        if (poReg->GetVSchiffe()) {
                            for (CRegion::VSchiffe::const_iterator si = poReg->GetVSchiffe()->begin(); si != poReg->GetVSchiffe()->end(); ++si) {
                                g_poCurrentShip = *si;
                                CMetaCommand::Call(std::string("OnShip"), (*si)->GetKommandos());
                                g_poCurrentShip = 0;
                            }
                        }
                        if (IsFlag(VF_PROGRESSINFO)) {
                            PIPRINT("\r                                                                      ");
                        }
                        bDoneReg = true;
                    }

                    poUnit = poVE->operator[](i);
                    m_poCurrentUnit = poUnit;
                    g_poCurrentUnit = poUnit;

                    if (IsFlag(VF_PROGRESSINFO)) {
                        PIPRINT("\r%sEinheiten: %3d%% - OnUnit(%s)      ", pcPass, nUnitCnt * 100 / m_nUnits, itoan(poUnit->m_nNummer, g_poCurrentReport->ENrBase()));
                    }

                    CMetaCommand::Call(std::string("OnUnit"), poUnit->m_csMetaOut);

                    if (Expression::getGlobal("$EXECINLINE").asLong()) {
                        if (IsFlag(VF_PROGRESSINFO)) {
                            PIPRINT("\r%sEinheiten: %3d%% - [%s]            ", pcPass, nUnitCnt * 100 / m_nUnits, itoan(poUnit->m_nNummer, g_poCurrentReport->ENrBase()));
                        }

                        if (IsFlag(VF_PRIVATMETA)) {
                            // Metabefehle in privaten Beschreibungen
                            if (!poUnit->m_sPrivat.empty()) {
                                std::string sNewBP;
                                std::string sTemp;
                                int c = 0;
                                //						bMetas = true;
                                Expression::Variables oContext;
                                CMetaCommand oMC(poUnit->m_sPrivat, 0);
                                oMC.SetFile(g_poCurrentReport->FileName());
                                //						oMC.SetLine( 0 );
                                CMCI oMCI(0);
                                oMC.RunScript(oMCI, oContext, &poUnit->m_sPrivat, poUnit->m_csMetaOut);
                                sNewBP = std::string("BESCHREIBE PRIVAT %c%s%c\n") + '"' + poUnit->m_sPrivat + '"';
                                do {
                                    sTemp = std::string(c ? "               " : "") + Wrap(sNewBP, size_t(g_nLineSize - (c ? 16 : 1)));
                                    if (!sNewBP.empty() && oReport.Version() >= 35 && IsEqual(oReport.Spiel(), "Eressea"))
                                        sTemp += " \\";
                                    poUnit->m_csMetaOut.push_back(sTemp);
                                    c++;
                                } while (!sNewBP.empty());

                                //						poUnit->m_csMetaOut.push_back( std::string( Buff ) );
                            }
                        }
                        else {
                            // Metabefehle in persistenten Kommentaren
                            std::string oldcmd;
                            std::string newcmd;
                            size_t realidx = 0;
                            for (size_t j = 0; j < poUnit->m_csKommandos.size(); j++) {
                                oldcmd = poUnit->m_csKommandos[(int32_t)j].asString();
                                if (IsMetaCommand(oldcmd)) {
                                    //							bMetas = true;
                                    Expression::Variables oContext;
                                    newcmd = oldcmd;
                                    CMetaCommand oMC(oldcmd, poUnit->m_cnKomLines[j]);
                                    oMC.SetFile(g_poCurrentReport->FileName());
                                    CMCI oMCI(0);
                                    oMCI.m_nLine = poUnit->m_cnKomLines[j];
                                    //							oMC.SetLine( poUnit->m_cnKomLines[j] );
                                    if (poUnit->m_csMetaOut.size() > realidx && oldcmd == poUnit->m_csMetaOut[(int32_t)realidx].asString()) {
                                        vCurrentMeta = Value(int32_t(realidx));
                                        Expression::setGlobal("$CURRENTMETA", &vCurrentMeta);
                                    }
                                    else {
                                        vCurrentMeta = Value(-1);
                                        Expression::setGlobal("$CURRENTMETA", &vCurrentMeta);
                                        //                                        ERRMSG( poUnit->m_csMetaOut, ( "%s(%d) : Warnung: Konflikt zwischen manueller Aenderung von 'UNIT.OUTPUT[%d]' und Referenzparametern!", g_poCurrentReport->FileName(),
                                        //                                        poUnit->m_cnKomLines[j], sName.substr( 0, p ).c_str(), realidx ) );
                                    }
                                    oMC.RunScript(oMCI, oContext, &newcmd, poUnit->m_csMetaOut);
                                    vCurrentMeta = Value(-1);
                                    Expression::setGlobal("$CURRENTMETA", &vCurrentMeta);
                                    if (IsFlag(VF_FULLCOMMANDOUTPUT)) {
                                        if (!IsEqual(newcmd.c_str(), oldcmd.c_str())) {
                                            if (poUnit->m_csMetaOut.size() > realidx && oldcmd == poUnit->m_csMetaOut[(int32_t)realidx].asString()) {
                                                poUnit->m_csMetaOut[(int32_t)realidx] = Value(newcmd);
                                            }
                                            else {
                                                ERRMSG((void*)&(poUnit->m_csMetaOut),
                                                       ("%s(%d) : Warnung: Konflikt zwischen manueller Aenderung von 'UNIT.OUTPUT[%d]' und Referenzparametern!", g_poCurrentReport->FileName().c_str(), poUnit->m_cnKomLines[j], realidx));
                                            }
                                        }
                                    }
                                    else {
                                        poUnit->m_csKommandos[(int32_t)j] = Value(newcmd);
                                    }
                                }
                                if (!oldcmd.empty() && oldcmd[0] == '/')
                                    realidx++;
                            }
                        }
                    }

                    if (IsFlag(VF_PROGRESSINFO)) {
                        PIPRINT("\r%sEinheiten: %3d%% - EndUnit(%s)     ", pcPass, nUnitCnt * 100 / m_nUnits, itoan(poUnit->m_nNummer, g_poCurrentReport->ENrBase()));
                        nUnitCnt++;
                    }
                    CMetaCommand::Call(std::string("EndUnit"), poUnit->m_csMetaOut);
                }
            }
        }

        if (bDoneReg) {
            if (IsFlag(VF_PROGRESSINFO)) {
                if (poReg->GetEZ()) {
                    PIPRINT("\r%sEinheiten: %3d%% - EndRegion(%d,%d,%d)", pcPass, nUnitCnt * 100 / m_nUnits, poReg->GetEX(), poReg->GetEY(), poReg->GetEZ());
                }
                else {
                    PIPRINT("\r%sEinheiten: %3d%% - EndRegion(%d,%d)", pcPass, nUnitCnt * 100 / m_nUnits, poReg->GetEX(), poReg->GetEY());
                }
            }
            CMetaCommand::Call(std::string("EndRegion"), poReg->GetEndKommandos());
            if (IsFlag(VF_PROGRESSINFO)) {
                PIPRINT("\r                                                                      ");
            }
        }
    }

    if (IsFlag(VF_PROGRESSINFO)) {
        PIPRINT("\r%sEinheiten: %3d%% - OnExit", pcPass, nUnitCnt * 100 / m_nUnits);
    }
    CMetaCommand::Call(std::string("OnExit"), m_coExitCmd);

    if (IsFlag(VF_PROGRESSINFO)) {
        PIPRINT("\r%sEinheiten: %3d%% \n", pcPass, nUnitCnt * 100 / m_nUnits);
    }
}

std::string CVorlage::MutateCRBlock(std::fstream& oIS, CBlockBase* poBlockObj, bool utf8)
{
    std::string sLine;
    std::string sTag, sTagCR;
    std::set<std::string> coUserTags;
    Value oVal;
    bool custom = false;

    while (true) {
        getline(oIS, sLine);
        if (oIS.fail())
            break;

        if (!sLine.empty() && IsAlpha(sLine[0]))
            break;

        while (!sLine.empty() && sLine[sLine.size() - 1] < 32 && sLine[sLine.size() - 1] > 0)
            sLine.erase(sLine.size() - 1, 1);

        custom = false;
        if (poBlockObj && poBlockObj->NumValues()) {
            std::string::size_type p = sLine.find_last_of(';');
            if (p != std::string::npos) {
                // TODO: Real UTF8-Handling
                sTag = sTagCR = sLine.substr(p + 1);
                if (utf8)
                    Utf8toIso885915(sTag);
                oVal = poBlockObj->CBlockBase::GetValue(std::string("!") + sTag, Value());
                coUserTags.insert(std::string("!") + sTag);
                if (oVal.getType() != VT_EMPTY) {
                    custom = true;
                }
            }
        }

        if (custom) {
            if (oVal.getType() == VT_STRING) {
                std::string strVal = Escape(oVal.asString());
                COutput::TPrint("vorlage", "\"{}\";{}\n", utf8 ? iso885915ToUtf8(strVal) : strVal, sTagCR);
            }
            else {
                COutput::TPrint("vorlage", "{};{}\n", oVal.asLong(), sTagCR);
            }
        }
        else {
            COutput::TPrint("vorlage", "{}\n", sLine);
        }
    }

    if (poBlockObj) {
        std::string sName;
        Value oValt;
        for (int32_t k = 0; k < (int32_t)poBlockObj->NumValues(); k++) {
            oValt = poBlockObj->CBlockBase::GetValue(k, sName);
            if (!sName.empty() && sName[0] == '!') {
                if (coUserTags.find(sName) == coUserTags.end()) {
                    if (utf8)
                        iso885915ToUtf8(sName);
                    if (oValt.getType() == VT_STRING) {
                        std::string strVal = Escape(oValt.asString());
                        COutput::TPrint("vorlage", "\"{}\";{}\n", utf8 ? iso885915ToUtf8(strVal) : strVal, sName.substr(1));
                    }
                    else {
                        COutput::TPrint("vorlage", "{};{}\n", oValt.asLong(), sName.substr(1));
                    }
                }
            }
        }
    }

    return sLine;
}

void CVorlage::Vorlage(CReport& oReport, CReport* poRep2, bool bTime)
{
    std::vector<CRegion*> cpoRegions;
    RegionDB::iterator rdbi;
    CRegion* poReg2;
    //	CRegion* poRQ = 0;
    //	FILE* hHold = 0;
    clock_t nStart;
    int32_t nUnits = 0, nPersons = 0, nNeededFood = 0;
    int ic;

    if (IsFlag(VF_CROUTPUT) || IsFlag(VF_SUPPRESSTURNOUTPUT)) {
        COutput::RenameTarget("vorlage", "vorlage_backup");
#ifdef _WIN32
        COutput::SetTarget("vorlage", new COutput("nul"));
#else
        COutput::SetTarget("vorlage", new COutput("/dev/null"));
#endif
    }

    COutput::SetFilter("vorlage", "OutputLineFilter");

    g_nTimeCorrection = 0;
    g_poCurrentReport = &oReport;
    m_poCurrentReport = &oReport;
    CMessage::SelectRenderer(oReport.MessageRenderer(), oReport.MessageRules());
    oReport.CalculateStatistics();
    if (poRep2) {
        poRep2->CalculateStatistics();
    }
    m_nPlayer = oReport.Partei();
    for (CReport::Einheiten::iterator ei = oReport.GEinheiten().begin(); ei != oReport.GEinheiten().end(); ei++) {
        if ((*ei).second->Partei() == m_nPlayer) {
            nUnits++;
            nPersons += (*ei).second->Anzahl();
            if (0 != CRasse::Lookup((*ei).second->RealType()).GetValue(std::string("Unterhalt")).asLong()) {
                nNeededFood += CRasse::Lookup((*ei).second->RealType()).GetValue(std::string("Unterhalt")).asLong() * (*ei).second->Anzahl();
            }
        }
    }
    m_nUnits = nUnits;

    COutput::TPrint("vorlage", "{} {} \"{}\"\n", IsEqual(g_poCurrentReport->m_sSpiel, "eressea") ? "ERESSEA" : "PARTEI", itoan(oReport.Partei(), oReport.PNrBase()), oReport.Passwort());
    if (IsEqual(g_poCurrentReport->m_sSpiel, "eressea") || IsEqual(g_poCurrentReport->m_sSpiel, "empiria") || IsEqual(g_poCurrentReport->m_sSpiel, "vinyambar i") || IsEqual(g_poCurrentReport->m_sSpiel, "vinyambar ii"))
        COutput::TPrint("vorlage", "\n ; ECHECK -l -w4 -r{}\n", oReport.Rekrutierungskosten());
    COutput::TPrint("vorlage", "\n ; {}, (C) 1999-2026 by S.Schuemann\n ; [{} {}]\n", VERSIONINFO, __DATE__, __TIME__);
    WrapOut(" ;         ", g_sCmdOptions, (size_t)g_nLineSize, " ; ");
    if (poRep2 && oReport.Version() != poRep2->Version()) {
        std::ostringstream out;
        out << "Achtung: CR-Versionswechsel von V" << poRep2->Version() << " auf V" << oReport.Version() << "!";
        auto msg = out.str();
        if (bTime)
            TRACEMSG(("%s\n", msg.c_str()));
        COutput::TPrint("vorlage", " ; {}\n", msg);
    }

    if (oReport.Zeitalter() == 1)
        COutput::TPrint("vorlage", "\n ; Zugvorlage aus Report Runde {}  ({} {})\n", oReport.Runde(), pcJahr[(oReport.Runde()) % 12], (oReport.Runde() - 1) / 12 + 1);
    else
        COutput::TPrint("vorlage", "\n ; Zugvorlage aus Report Runde {}  ({}. Woche, {}, {})\n", oReport.Runde(), (oReport.Runde() - 184) % 3 + 1, pcJahr2[((oReport.Runde() - 184) / 3) % 9], (oReport.Runde() - 184) / 27 + 1);

    CPartei::Ptr parteiInfo = oReport.GetLocalParteiInfo(oReport.Partei());
    CPartei::Ptr lastParteiInfo;
    if (poRep2)
        lastParteiInfo = poRep2->GetLocalParteiInfo(poRep2->Partei());

    int32_t maxHeroes = 0;
    if (parteiInfo)
        maxHeroes = parteiInfo->GetValue("max_heroes").asLong();
    int32_t heroes = 0;
    if (parteiInfo)
        heroes = parteiInfo->GetValue("heroes").asLong();

    if (oReport.m_nPunkte > 0 && oReport.m_nPunkteschnitt > 0) {
        if (!poRep2) {
            if (maxHeroes > 0)
                COutput::TPrint("vorlage", " ; Personen: {}, Einheiten: {}, Helden: {}/{}\n", nPersons, nUnits, heroes, maxHeroes);
            else
                COutput::TPrint("vorlage", " ; Personen: {}, Einheiten: {}\n", nPersons, nUnits);
            COutput::TPrint("vorlage", " ; Punkte: {} ({:.2f}% des Durchschnitts)\n", oReport.m_nPunkte, (double)oReport.m_nPunkte * 100.0 / oReport.m_nPunkteschnitt);
        }
        else {
            int32_t nLastUnits = 0, nLastPersons = 0;
            for (CReport::Einheiten::iterator ei = poRep2->GEinheiten().begin(); ei != poRep2->GEinheiten().end(); ei++) {
                if ((*ei).second->Partei() == m_nPlayer) {
                    nLastUnits++;
                    nLastPersons += (*ei).second->Anzahl();
                    //                  if( !IsEqual( (*ei).second->Typ(), "Untote" ) )
                    //    			        nEater += (*ei).second->Anzahl();
                }
            }

            int32_t lastMaxHeroes = 0;
            if (lastParteiInfo)
                lastMaxHeroes = lastParteiInfo->GetValue("max_heroes").asLong();
            int32_t lastHeroes = 0;
            if (lastParteiInfo)
                lastHeroes = lastParteiInfo->GetValue("heroes").asLong();
            if (maxHeroes > 0)
                COutput::TPrint("vorlage", " ; Personen: {} ({:+}), Einheiten: {} ({:+}), Helden: {}/{} ({:+}/{:+})\n", nPersons, nPersons - nLastPersons, nUnits, nUnits - nLastUnits, heroes, maxHeroes, heroes - lastHeroes, maxHeroes - lastMaxHeroes);
            else
                COutput::TPrint("vorlage", " ; Personen: {} ({:+}), Einheiten: {} ({:+})\n", nPersons, nPersons - nLastPersons, nUnits, nUnits - nLastUnits);

            if (poRep2->m_nPunkte)
                COutput::TPrint("vorlage", " ; Punkte: {} ({:.2f}% des Durchschnitts, {:+.2f}%)\n", oReport.m_nPunkte, (double)oReport.m_nPunkte * 100.0 / oReport.m_nPunkteschnitt,
                                (double)oReport.m_nPunkte * 100.0 / oReport.m_nPunkteschnitt - (double)poRep2->m_nPunkte * 100.0 / poRep2->m_nPunkteschnitt);
            else
                COutput::TPrint("vorlage", " ; Punkte: {} ({:.2f}% des Durchschnitts)\n", oReport.m_nPunkte, (double)oReport.m_nPunkte * 100.0 / oReport.m_nPunkteschnitt);
        }
    }

    //	Expression::clearAllVars();

    m_poKarte = oReport.Karte();
    g_poKarte = m_poKarte;

    //	if( IsFlag( VF_SORTISLANDS ) )
    //	{
    if (bTime)
        TRACEMSG((" ["));
    nStart = clock();
    ic = m_poKarte->Islandize();
    if (bTime) {
        TRACEMSG(("%d Inseln (%1.2f sek.)]\n", ic, float(clock() - nStart) / CLOCKS_PER_SEC));
    }
    //	}

    Value oPassNumVal, oMinPassesVal, oExecInline, oGame;

    m_coInitCmd.clear();
    m_coExitCmd.clear();

    oGame = Value(g_poCurrentReport->m_sSpiel);
    Expression::setConstant("GAME", &oGame);

    oExecInline = Value(1);
    Expression::setGlobal("$EXECINLINE", &oExecInline);

    oPassNumVal = Value(1);
    Expression::setGlobal("$PASSNUM", &oPassNumVal);

    if (g_nMinPasses) {
        g_nPassNum = 1;
        oPassNumVal = g_nPassNum;
        oMinPassesVal = g_nMinPasses;
        Expression::setGlobal("$PASSNUM", &oPassNumVal);
        Expression::setGlobal("$MINPASSES", &oMinPassesVal);
    }

    bool bLoop = true;
    int32_t nPassNum;

    do {
        RunMetacommands(oReport);

        oExecInline = Value(0);
        Expression::setGlobal("$EXECINLINE", &oExecInline);

        if (g_nMinPasses) {
            nPassNum = Expression::getGlobal("$PASSNUM").asLong();
            if (nPassNum != g_nPassNum) {
                g_nPassNum = nPassNum;
                oPassNumVal = g_nPassNum;
                Expression::setGlobal("$PASSNUM", &oPassNumVal);
            }
            else if (nPassNum < g_nMinPasses) {
                g_nPassNum++;
                oPassNumVal = g_nPassNum;
                Expression::setGlobal("$PASSNUM", &oPassNumVal);
            }
            else {
                bLoop = false;
            }
        }
        else {
            bLoop = false;
        }
    } while (bLoop);

    if (!m_coInitCmd.empty()) {
        for (int32_t i = 0; i < (int32_t)m_coInitCmd.size(); i++) {
            if (m_coInitCmd[i].empty()) {
                COutput::TWrite("vorlage", "\n");
            }
            else {
                if (m_coInitCmd[i].asString()[0] == ';')
                    WrapOut(" ; ", m_coInitCmd[i].asString().substr(2), (size_t)g_nLineSize);
            }
        }
    }

    if (IsFlag(VF_SHOWHANDEL)) {
        COutput::TWrite("vorlage", "\n ; Wirtschaftsbilanz:\n");
        COutput::TPrint("vorlage", " ; Gesamteinkommen:{:9} Silber\n", oReport.m_nEinkommen);
        COutput::TPrint("vorlage", " ; Gesamtausgaben: {:9} Silber {}\n", oReport.m_nAusgaben + nNeededFood /*Eater*10*/, oReport.Version() > 40 ? "" : "(z.Zt. ohne kostenpfl. Talente)");
        int64_t nVermoegen = 0;
        for (CKarte::RegionMap::const_iterator rmi = m_poKarte->Regions().begin(); rmi != m_poKarte->Regions().end(); rmi++) {
            nVermoegen += (*rmi).second->SilverOf(oReport.Partei());
        }
        COutput::TPrint("vorlage", " ; Gesamtverm\xF6gen: {:9} Silber\n", nVermoegen);

        if (oReport.m_cpoHPartner.size())
            COutput::TWrite("vorlage", "\n ; Warenaustausch:\n");

        for (CReport::Handelspartner::const_iterator rhi = oReport.m_cpoHPartner.begin(); rhi != oReport.m_cpoHPartner.end(); rhi++) {
            std::ostringstream out;
            out << oReport.Parteiname((*rhi).first).c_str() + 1 << "(" << itoan((*rhi).first, oReport.PNrBase()) << "): ";
            for (CParteihandel::Produkte::const_iterator ppi = (*rhi).second->m_coProdukte.begin(); ppi != (*rhi).second->m_coProdukte.end(); ppi++) {
                if (ppi != (*rhi).second->m_coProdukte.begin()) {
                    out << ", ";
                }
                out << std::showpos << (*ppi).second << std::noshowpos << " " << (*ppi).first;
            }
            WrapOut(" ;        ", getAndReset(out), (size_t)g_nLineSize, " ; ");
        }
    }

    for (CKarte::RegionMap::const_iterator rmi = m_poKarte->Regions().begin(); rmi != m_poKarte->Regions().end(); rmi++) {
        rdbi = g_coRDB.find((*rmi).second->GetKey());
        if (rdbi != g_coRDB.end()) {
            if ((*(*rdbi).second.begin())->GetIslandName().empty())
                (*rmi).second->SetIslandName((*(*rdbi).second.begin())->GetIslandName());
        }
        cpoRegions.push_back((*rmi).second);
    }
    std::stable_sort(cpoRegions.begin(), cpoRegions.end(), CRegionSorter(IsFlag(VF_SORTISLANDS)));

    if (IsFlag(VF_SHOWKOMPKARTE)) {
        COutput::TWrite("vorlage", "\n ; Uebersichtskarte:\n");
        oReport.Karte()->DumpFullMap("vorlage", " ; ");
    }

    if (IsFlag(VF_SHOWWORLDKARTE)) {
        COutput::TWrite("vorlage", "\n ; Karte der bekannten Welt:\n");
        oReport.Karte()->DumpWorldMap("vorlage", " ; ");
    }

    //	for( CKarte::RegionMap::const_iterator i = m_poKarte->Regions().begin(); i != m_poKarte->Regions().end(); i++ )
    for (size_t i = 0; i < cpoRegions.size(); i++) {
        poReg2 = 0;
        if (poRep2) {
            CKarte::RegionMap::const_iterator ri;
            ri = poRep2->Karte()->Regions().find(cpoRegions[i] /*.second*/->GetKey());
            if (ri != poRep2->Karte()->Regions().end()) {
                poReg2 = (*ri).second;
                if (!poReg2->PersonsOf(m_nPlayer) || !poReg2->IsLand())
                    poReg2 = 0;
            }
        }
        if (g_nOnlyGroup < 0 || cpoRegions[i]->IsGroup(g_nOnlyGroup)) {
            if (cpoRegions[i]->PersonsOf(m_nPlayer))
                Regionsvorlage(cpoRegions[i] /*.second*/, poReg2, poRep2);
        }
    }

    COutput::TWrite("vorlage", "\n NAECHSTER\n\n");

    if (!m_coExitCmd.empty()) {
        for (int32_t i = 0; i < (int32_t)m_coExitCmd.size(); i++) {
            if (m_coExitCmd[i].empty()) {
                COutput::TWrite("vorlage", "\n");
            }
            else {
                if (m_coExitCmd[i].asString()[0] == ';')
                    WrapOut(" ; ", m_coExitCmd[i].asString().substr(2), (size_t)g_nLineSize);
            }
        }
    }

    COutput::SetFilter("vorlage", "");

    if (IsFlag(VF_CROUTPUT)) {
        CEinheit* poUnit = 0;
        std::fstream oIS;
        std::string sLine;
        std::string sCmd;
        int32_t nENr = -1;
        // int32_t nPNr = 0;
        bool bGotLine = false;
        bool bKonfiguration = false;
        // size_t i;

        COutput::RenameTarget("vorlage_backup", "vorlage");

        // TODO: Real UTF8-Handling
        oIS.open(oReport.m_sOrgCRName.c_str(), ios::in);

        if (oIS.fail()) {
            ERRMSG(0, ("Auf die Report-Datei kann nicht zugegriffen werden!"));
            EXIT(1);
        }

        while (1) {
            if (!bGotLine) {
                getline(oIS, sLine);
                if (oIS.fail())
                    break;
            }
            else
                bGotLine = false;

            while (!sLine.empty() && sLine[sLine.size() - 1] < 32 && sLine[sLine.size() - 1] > 0)
                sLine.erase(sLine.size() - 1, 1);

            if (!bKonfiguration && sLine.length() > 15 && CRegExp::Match(sLine, "^(\"[^\"]*\"\\s*;\\s*(?i:Konfiguration)|[A-Z]+)")) {
                bKonfiguration = true;
                if (CRegExp::Match(sLine, "^\"[^\"]*\"\\s*;\\s*(?i:Konfiguration)")) {
                    COutput::TWrite("vorlage", "\"Vorlage\";Konfiguration\n");
                }
                else {
                    COutput::TWrite("vorlage", "\"Vorlage\";Konfiguration\n");
                    COutput::TPrint("vorlage", "{}\n", sLine);
                }
            }
            else {
                COutput::TPrint("vorlage", "{}\n", sLine);
            }

            if (!strncmp(sLine.c_str(), "REGION ", 7)) {
                static char buff[128];
                CRegion* pReg;
                char* sptr;
                int32_t x = 0, y = 0, z = 0;
                strncpy(buff, sLine.c_str() + 7, 127);
                buff[127] = 0;
                sptr = buff;
                x = (int32_t)strtol(sptr, &sptr, 10);
                if (*sptr)
                    sptr++;
                y = (int32_t)strtol(sptr, &sptr, 10);
                if (*sptr)
                    sptr++;
                z = (int32_t)strtol(sptr, &sptr, 10);
                pReg = oReport.Karte()->GetFromECords(x, y, z);
                sLine = MutateCRBlock(oIS, pReg, oReport.m_bUTF8);
                bGotLine = true;
            }
            else if (!strncmp(sLine.c_str(), "EINHEIT ", 8)) {
                nENr = (int32_t)strtol(sLine.substr(8).c_str(), NULL, 10);
                poUnit = oReport.SearchUnit(nENr, false);
                //				if( poUnit && poUnit->Partei() != oReport.Partei() )
                //					poUnit = 0;

                sLine = MutateCRBlock(oIS, poUnit, oReport.m_bUTF8);
                bGotLine = true;
            }
            else if (!strncmp(sLine.c_str(), "SCHIFF ", 7)) {
                int32_t nShipNr = (int32_t)strtol(sLine.substr(7).c_str(), NULL, 10);
                CSchiff* poShip = oReport.GetShip(nShipNr);
                sLine = MutateCRBlock(oIS, poShip, oReport.m_bUTF8);
                bGotLine = true;
            }
            else if (!strncmp(sLine.c_str(), "BURG ", 5)) {
                int32_t nBurgNr = (int32_t)strtol(sLine.substr(5).c_str(), NULL, 10);
                CBauwerk* poBuilding = oReport.GetBuilding(nBurgNr);
                sLine = MutateCRBlock(oIS, poBuilding, oReport.m_bUTF8);
                bGotLine = true;
            }
            else if (poUnit && !strncmp(sLine.c_str(), "COMMANDS", 8)) {
                // TODO: Real UTF8-Handling
                while (true) {
                    getline(oIS, sLine);
                    if (oIS.fail() || sLine[0] != '\x22')
                        break;
                }

                if (!IsFlag(VF_FULLCOMMANDOUTPUT)) {
                    for (int32_t i = 0; i < (int32_t)poUnit->m_csKommandos.size(); i++) {
                        std::string strVal = Escape(poUnit->m_csKommandos[i].asString(), true);
                        if (!poUnit->m_csKommandos[i].empty())
                            COutput::TPrint("vorlage", "\"{}\"\n", oReport.m_bUTF8 ? iso885915ToUtf8(strVal) : strVal);
                    }
                }

                for (int32_t i = 0; i < (int32_t)poUnit->m_csMetaOut.size(); i++) {
                    bool bAZ = false;
                    sCmd = poUnit->m_csMetaOut[i].asString();
                    for (unsigned c = 0; c < sCmd.size(); c++) {
                        if (sCmd[c] == 34) {
                            bAZ = !bAZ;
                            sCmd[c] = ' ';
                        }
                        else if (bAZ && sCmd[c] < 33 && sCmd[c] > 0)
                            sCmd[c] = '~';

                        while (!sCmd.empty() && sCmd[sCmd.size() - 1] < 33 && sCmd[sCmd.size() - 1] > 0)
                            sCmd.erase(sCmd.size() - 1, 1);
                    }
                    std::string strVal = Escape(sCmd, true);
                    COutput::TPrint("vorlage", "\"{}\"\n", oReport.m_bUTF8 ? iso885915ToUtf8(strVal) : strVal);
                }

                if (poUnit->m_csMetaOut.empty() && (poUnit->m_csKommandos.empty() || IsFlag(VF_FULLCOMMANDOUTPUT))) {
                    COutput::TWrite("vorlage", "\"\"\n");
                }

                bGotLine = true;
                poUnit = 0;
            }
        }
    }
}

void CVorlage::WriteMap(const std::string& sFile)
{
    if (m_poCurrentReport && m_poKarte) {
        std::fstream oIS, oOS;
        std::string sLine;
        std::string sCmd;

        oIS.open(m_poCurrentReport->m_sOrgCRName.c_str(), ios::in);

        if (oIS.fail()) {
            ERRMSG(0, ("Auf die Report-Datei kann nicht zugegriffen werden!"));
            EXIT(1);
        }

        // TODO: Real UTF8-Handling
        oOS.open(sFile.c_str(), ios::out);
        if (oOS.fail()) {
            ERRMSG(0, ("Karte kann nicht geschrieben werden!"));
            EXIT(1);
        }

        // Copy head from current report
        while (1) {
            // TODO: Real UTF8-Handling
            getline(oIS, sLine);
            if (oIS.fail())
                break;

            while (!sLine.empty() && sLine[sLine.size() - 1] < 32 && sLine[sLine.size() - 1] > 0)
                sLine.erase(sLine.size() - 1, 1);

            if (!sLine.empty()) {
                if (IsAlpha(sLine[0]) && sLine[0] != 'V' && sLine[0] != '\xef')
                    break;
                if (!CRegExp::Match(sLine, "Passwort"))
                    oOS << sLine << std::endl;
                //			        COutput::TPrintf( "vorlage", "%s\n", sLine.c_str() );
            }
        }
        bool utf8 = m_poCurrentReport->m_bUTF8;
        for (RegionDB::iterator rmi = g_coRDB.begin(); rmi != g_coRDB.end(); rmi++) {
            if ((*(*rmi).second.begin())->GetEZ()) {
                oOS << "REGION " << (*(*rmi).second.begin())->GetEX() << " " << (*(*rmi).second.begin())->GetEY() << " " << (*(*rmi).second.begin())->GetEZ() << std::endl;
            }
            else {
                oOS << "REGION " << (*(*rmi).second.begin())->GetEX() << " " << (*(*rmi).second.begin())->GetEY() << std::endl;
            }
            if (IsFlag(VF_EXPORTWITHROUND)) {
                oOS << (*(*rmi).second.begin())->Runde() << ";Runde" << std::endl;
            }
            if ((*(*rmi).second.begin())->IsLand()) {
                oOS << "\"" << iso2utf8((*(*rmi).second.begin())->GetName(), utf8) << "\";Name" << std::endl;
            }
            oOS << "\"" << iso2utf8((*(*rmi).second.begin())->GetRegionTypeName(), utf8) << "\";Terrain" << std::endl;
            if (!(*(*rmi).second.begin())->GetIslandName().empty()) {
                oOS << "\"" << iso2utf8((*(*rmi).second.begin())->GetIslandName(), utf8) << "\";Insel" << std::endl;
            }
            if ((*(*rmi).second.begin())->GetValue("herb").asString().length() > 3) {
                oOS << "\"" << iso2utf8((*(*rmi).second.begin())->GetValue("herb").asString(), utf8) << "\";herb" << std::endl;
            }
        }
    }
}

/////////////////////////////////////////////////////////////
// Regionsvorlage erstellen
void CVorlage::Regionsvorlage(CRegion* poReg, CRegion* poReg2, CReport* poRep2)
{
    std::map<int32_t, int32_t> coPersonen;
    std::map<int32_t, int32_t> coPersonen2;
    std::map<int32_t, int32_t> coWaffen;
    std::set<CBauwerk*> coBauwerke;
    std::set<CSchiff*> coSchiffe;
    std::set<int32_t> bewachendeParteien;
    RegionDB::iterator rdbi;
    std::ostringstream out;
    CRegion* poRQ = 0;
    CRegion::Materialpool coMPool, coMPool2;
    int32_t nOrt = -1;
    bool bRegHead = false;
    bool bDiff = poReg2 && abs(poReg2->GetQuality() % 100) >= 5;

    g_csDupDescrFilter.clear();

    CRegion::VEinheiten* poVE = &poReg->GetVEinheiten();
    m_poCurrentRegion = poReg;
    g_poCurrentRegion = poReg;

    if (poReg->GetVBauwerke()) {
        coBauwerke.insert(poReg->GetVBauwerke()->begin(), poReg->GetVBauwerke()->end());
    }
    if (poReg->GetVSchiffe()) {
        coSchiffe.insert(poReg->GetVSchiffe()->begin(), poReg->GetVSchiffe()->end());
    }
    rdbi = g_coRDB.find(poReg->GetKey());
    if (rdbi != g_coRDB.end()) {
        poRQ = *(*rdbi).second.begin();
    }
    if (!poRQ) {
        poRQ = poReg;
    }
    if (IsFlag(VF_SORTBURGEN) || IsFlag(VF_SORTTALENTE) || IsFlag(VF_SORTPRIVAT)) {
        std::stable_sort(poReg->GetVEinheiten().begin(), poReg->GetVEinheiten().end(), CEinheitenSorter(0));
    }
    for (size_t i = 0; i < poVE->size(); i++) {
        if (coPersonen.find(poVE->operator[](i)->Partei()) == coPersonen.end()) {
            coPersonen[poVE->operator[](i)->Partei()] = 0;
            coWaffen[poVE->operator[](i)->Partei()] = 0;
        }
        coPersonen[poVE->operator[](i)->Partei()] += poVE->operator[](i)->Anzahl();
        if (poVE->operator[](i)->m_nBewacht) {
            bewachendeParteien.insert(poVE->operator[](i)->Partei());
        }
        if (true /* poVE->operator[](i)->Partei() == m_nPlayer */) {
            std::string sInsel;
            if (!poRQ->GetIslandName().empty() || !IsFlag(VF_SHOWVERBOSEINFO)) {
                sInsel = std::string("[") + poRQ->GetIslandName() + std::string("]");
            }
            else {
                sInsel = "";  // poReg->GetIsland().asString();
            }
            if (!bRegHead) {
                if (IsFlag(VF_SHOWVERBOSEINFO)) {
                    COutput::TWrite("vorlage", "\n; --------------------------------------------------------------\n\n");
                }
                else {
                    COutput::TWrite("vorlage", "\n");
                }
                if (CMetaCommand::ProcExists("CreateRegionHeader")) {
                    VKommandos coOutput;
                    CMetaCommand::Call("CreateRegionHeader", coOutput);
                    if (!coOutput.empty()) {
                        for (size_t j = 0; j < coOutput.size(); j++) {
                            COutput::TPrint("vorlage", " {}\n", coOutput[(int32_t)j].c_str());
                        }
                    }
                }
                else {
                    if (IsEqual(g_poCurrentReport->m_sSpiel, "Verdanon")) {
                        if (!IsFlag(VF_SHOWVERBOSEINFO)) {
                            if (poReg->GetEZ()) {
                                COutput::TPrint("vorlage", " ; {} ({},{},{})\n", poReg->GetName().empty() ? poReg->GetRegionTypeName() : poReg->GetName(), poReg->GetEX(), poReg->GetEY(), poReg->GetEZ());
                            }
                            else {
                                COutput::TPrint("vorlage", " ; {} ({},{})\n", poReg->GetName().empty() ? poReg->GetRegionTypeName() : poReg->GetName(), poReg->GetEX(), poReg->GetEY());
                            }
                        }
                        else if (poReg->GetBlock() == CRegion::enSPEZIALREGION) {
                            COutput::TPrint("vorlage", " ; Astralebene ({}, {} Personen, {}$ Silber)\n", poReg->GetRegionTypeName(), poReg->PersonsOf(m_nPlayer, true), poReg->SilverOf(m_nPlayer));
                        }
                        else if (poReg->GetEZ()) {
                            COutput::TPrint("vorlage", " ; {} ({},{},{}) ({}, {} Personen, {}$ Silber) {}\n", poReg->GetName(), poReg->GetEX(), poReg->GetEY(), poReg->GetEZ(), poReg->GetRegionTypeName(), poReg->PersonsOf(m_nPlayer, true),
                                            poReg->SilverOf(m_nPlayer), sInsel);
                        }
                        else {
                            COutput::TPrint("vorlage", " ; {} ({},{}) ({}, {} Personen, {}$ Silber) {}\n", poReg->GetName(), poReg->GetEX(), poReg->GetEY(), poReg->GetRegionTypeName(), poReg->PersonsOf(m_nPlayer, true),
                                            poReg->SilverOf(m_nPlayer), sInsel);
                        }
                    }
                    else {
                        if (!IsFlag(VF_SHOWVERBOSEINFO)) {
                            if (poReg->GetEZ()) {
                                COutput::TPrint("vorlage", " REGION {},{},{} ; {}\n", poReg->GetEX(), poReg->GetEY(), poReg->GetEZ(), poReg->GetName().empty() ? poReg->GetRegionTypeName() : poReg->GetName());
                            }
                            else {
                                COutput::TPrint("vorlage", " REGION {},{} ; {}\n", poReg->GetEX(), poReg->GetEY(), poReg->GetName().empty() ? poReg->GetRegionTypeName() : poReg->GetName());
                            }
                        }
                        else if (poReg->GetBlock() == CRegion::enSPEZIALREGION) {
                            COutput::TPrint("vorlage", " REGION; Astralebene ({}, {} Personen, {}$ Silber)\n", poReg->GetRegionTypeName(), poReg->PersonsOf(m_nPlayer, true), poReg->SilverOf(m_nPlayer));
                        }
                        else if (poReg->GetEZ()) {
                            COutput::TPrint("vorlage", " REGION {},{},{} ; {} ({}, {} Personen, {}$ Silber) {}\n", poReg->GetEX(), poReg->GetEY(), poReg->GetEZ(), poReg->GetName(), poReg->GetRegionTypeName(),
                                            poReg->PersonsOf(m_nPlayer, true), poReg->SilverOf(m_nPlayer), sInsel);
                        }
                        else {
                            COutput::TPrint("vorlage", " REGION {},{} ; {} ({}, {} Personen, {}$ Silber) {}\n", poReg->GetEX(), poReg->GetEY(), poReg->GetName(), poReg->GetRegionTypeName(), poReg->PersonsOf(m_nPlayer, true),
                                            poReg->SilverOf(m_nPlayer), sInsel);
                        }
                        COutput::TPrint("vorlage", " ; ECheck Lohn {}\n", poReg->GetLohn() ? poReg->GetLohn() : 10);
                    }
                    if (poReg->GetBlock() != CRegion::enSPEZIALREGION && IsFlag(VF_SHOWMINIKARTE)) {
                        static const std::set<std::string> explicitResources{"Bauern", "Silber", "Unterhalt", "Rekruten", "Pferde", "Gewinn", "Pl. frei"};
                        COutputTable OT;
                        std::string sT;

                        if (IsFlag(VF_HEXMAP)) {
                            sT = " ";
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() - 1, poReg->GetEY() + 1, poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX(), poReg->GetEY() + 1, poReg->GetEZ())->GetRegionChar();
                        }
                        else {
                            sT = " ";
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() - 1, poReg->GetEY() - 1, poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX(), poReg->GetEY() - 1, poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() + 1, poReg->GetEY() - 1, poReg->GetEZ())->GetRegionChar();
                        }
                        OT.Col(sT);
                        OT.Col("|").Col("Bauern:").Col(poReg->GetBauern());
                        if (bDiff) {
                            OT.Col(ToStringS(poReg->GetBauern() - poReg2->GetBauern()), COutputTable::enRIGHT);
                        }
                        OT.Col("|").Col("Silber:").Col(poReg->GetSilber());
                        if (bDiff) {
                            OT.Col(ToStringS(poReg->GetSilber() - poReg2->GetSilber()), COutputTable::enRIGHT);
                        }
                        OT.Col("|").Col("Unterhalt:").Col(poReg->GetUnterhalt());
                        if (bDiff) {
                            OT.Col(ToStringS(poReg->GetUnterhalt() - poReg2->GetUnterhalt()), COutputTable::enRIGHT);
                        }
                        OT.Col("|").Next();
                        if (IsFlag(VF_HEXMAP)) {
                            sT = poReg->Map()->GetFromECords(poReg->GetEX() - 1, poReg->GetEY(), poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() + 1, poReg->GetEY(), poReg->GetEZ())->GetRegionChar();
                        }
                        else {
                            sT = " ";
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() - 1, poReg->GetEY(), poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() + 1, poReg->GetEY(), poReg->GetEZ())->GetRegionChar();
                        }
                        OT.Col(sT + ' ');
                        if (IsFlag(VF_RESOURCEBLOCKS)) {
                            OT.Col("|").Col("Rekruten:").Col(poReg->GetRekruten());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->GetRekruten() - poReg2->GetRekruten()), COutputTable::enRIGHT);
                            }
                            OT.Col("|").Col("Pferde:").Col(poReg->GetPferde());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->GetPferde() - poReg2->GetPferde()), COutputTable::enRIGHT);
                            }
                            OT.Col("|").Col("Gewinn:").Col(poReg->CalcProfit());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->CalcProfit() - poReg2->CalcProfit()), COutputTable::enRIGHT);
                            }
                        }
                        else {
                            OT.Col("|").Col("Rekruten:").Col(poReg->GetRekruten());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->GetRekruten() - poReg2->GetRekruten()), COutputTable::enRIGHT);
                            }
                            OT.Col("|").Col("Eisen:").Col(poReg->GetEisen() == -2 ? std::string("?") : (poReg->GetEisen() == -1 ? std::string("-") : ToString(poReg->GetEisen())), COutputTable::enRIGHT);
                            if (bDiff) {
                                OT.Col(poReg->GetEisen() >= 0 && poReg2->GetEisen() >= 0 ? ToStringS(poReg->GetEisen() - poReg2->GetEisen()) : std::string(""), COutputTable::enRIGHT);
                            }
                            OT.Col("|").Col("Gewinn:").Col(poReg->CalcProfit());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->CalcProfit() - poReg2->CalcProfit()), COutputTable::enRIGHT);
                            }
                        }
                        OT.Col("|").Next();
                        if (IsFlag(VF_HEXMAP)) {
                            sT = " ";
                            sT += poReg->Map()->GetFromECords(poReg->GetEX(), poReg->GetEY() - 1, poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() + 1, poReg->GetEY() - 1, poReg->GetEZ())->GetRegionChar();
                        }
                        else {
                            sT = " ";
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() - 1, poReg->GetEY() + 1, poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX(), poReg->GetEY() + 1, poReg->GetEZ())->GetRegionChar();
                            sT += ' ';
                            sT += poReg->Map()->GetFromECords(poReg->GetEX() + 1, poReg->GetEY() + 1, poReg->GetEZ())->GetRegionChar();
                        }
                        OT.Col(sT);
                        if (IsFlag(VF_RESOURCEBLOCKS)) {
                            int nRCnt = 0;
                            OT.Col("|").Col("Pl. frei:").Col(poReg->CalcJobs() - poReg->GetBauern());
                            if (bDiff) {
                                OT.Col(ToStringS((poReg->CalcJobs() - poReg->GetBauern()) - (poReg2->CalcJobs() - poReg2->GetBauern())), COutputTable::enRIGHT);
                            }
                            nRCnt++;
                            if (!poReg->GetResource("Baeume") && !poReg->GetResource("Mallorn") && (poReg->GetValue("Baeume").asLong() || (bDiff && !poReg2->GetResource("Baeume") && !poReg2->GetResource("Mallorn") && poReg2->GetBaeume()))) {
                                OT.Col("|").Col(poReg->isMallorn() ? "Mallorn:" : "Baeume:").Col(poReg->GetBaeume());
                                if (bDiff) {
                                    OT.Col(ToStringS(poReg->GetBaeume() - poReg2->GetBaeume()), COutputTable::enRIGHT);
                                }
                                nRCnt++;
                            }
                            if (!poReg->GetResource("Schoesslinge") && !poReg->GetResource("Mallornschoesslinge") &&
                                (poReg->GetValue("Schoesslinge").asLong() || (bDiff && !poReg2->GetResource("Schoesslinge") && !poReg2->GetResource("Mallornschoesslinge") && poReg2->GetValue("Schoesslinge").asLong()))) {
                                OT.Col("|").Col("Sch\xF6\xDFlinge:").Col(poReg->GetValue("Schoesslinge").asLong());
                                if (bDiff) {
                                    OT.Col(ToStringS(poReg->GetValue("Schoesslinge").asLong() - poReg2->GetValue("Schoesslinge").asLong()), COutputTable::enRIGHT);
                                }
                                nRCnt++;
                            }
                            if (!poReg->GetResource("Steine") && (poReg->GetValue("Steine").asLong() || (bDiff && !poReg2->GetResource("Steine") && poReg2->GetValue("Steine").asLong()))) {
                                if (nRCnt && nRCnt % 3 == 0) {
                                    OT.Col("|").Next();
                                    OT.Col(" ");
                                }
                                OT.Col("|").Col("Steine:").Col(poReg->GetValue("Steine").asLong());
                                if (bDiff) {
                                    OT.Col(ToStringS(poReg->GetValue("Steine").asLong() - poReg2->GetValue("Steine").asLong()), COutputTable::enRIGHT);
                                }
                                nRCnt++;
                            }
                            //                            if( poReg->GetResourcen().size()>1 )
                            {
                                std::string sName;
                                std::set<std::string> coResourceSet;
                                CResource* pRes;
                                CResource* pRes1 = 0;
                                CResource* pRes2 = 0;
                                int32_t nNum1, nNum2, nSkill1;  //, nSkill2;
                                                                //                                OT.Col("|").Col(" ").Col(" "); if( bDiff ) OT.Col(" ");
                                for (unsigned j = 0; j < poReg->GetResourcen().size(); j++) {
                                    coResourceSet.insert(std::string(poReg->GetResourcen()[j]->GetValue("skill").asLong() ? "1" : "0") + Flatten(poReg->GetResourcen()[j]->GetValue("type").asString()));
                                }
                                if (bDiff) {
                                    for (unsigned j = 0; j < poReg2->GetResourcen().size(); j++) {
                                        coResourceSet.insert(std::string(poReg2->GetResourcen()[j]->GetValue("skill").asLong() ? "1" : "0") + Flatten(poReg2->GetResourcen()[j]->GetValue("type").asString()));
                                    }
                                }

                                for (std::set<std::string>::const_iterator rsi = coResourceSet.begin(); rsi != coResourceSet.end(); rsi++) {
                                    pRes1 = poReg->GetResource((*rsi).substr(1));
                                    if (bDiff) {
                                        pRes2 = poReg2->GetResource((*rsi).substr(1));
                                    }
                                    pRes = pRes1 ? pRes1 : pRes2;
                                    sName = pRes->GetValue("type").asString();
                                    if (!explicitResources.count(sName)) {
                                        if (nRCnt && nRCnt % 3 == 0) {
                                            OT.Col("|").Next();
                                            OT.Col(" ");
                                        }
                                        nNum1 = pRes1 ? pRes1->GetValue("number").asLong() : 0;
                                        nSkill1 = pRes1 ? pRes1->GetValue("skill").asLong() : 0;
                                        nNum2 = pRes2 ? pRes2->GetValue("number").asLong() : 0;
                                        //                                        nSkill2 = pRes2 ? pRes2->GetValue( "skill" ).asLong() : 0;
                                        OT.Col("|").Col(sName + (nSkill1 ? "(" + ToString(nSkill1) + "):" : ":")).Col(nNum1);
                                        if (bDiff) {
                                            if (pRes2) {
                                                OT.Col(nNum2 ? ToStringS(nNum1 - nNum2) : std::string(""), COutputTable::enRIGHT);
                                            }
                                            else
                                                OT.Col(" ");
                                        }
                                        nRCnt++;
                                    }
                                }
                                while (nRCnt % 3) {
                                    OT.Col("|").Col(" ").Col(" ");
                                    if (bDiff) {
                                        OT.Col(" ");
                                    }
                                    nRCnt++;
                                }
                            }
                        }
                        else {
                            OT.Col("|").Col("Pferde:").Col(poReg->GetPferde());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->GetPferde() - poReg2->GetPferde()), COutputTable::enRIGHT);
                            }
                            OT.Col("|").Col("Laen:").Col(poReg->GetLaen() == -2 ? std::string("?") : (poReg->GetLaen() == -1 ? std::string("-") : ToString(poReg->GetLaen())), COutputTable::enRIGHT);
                            if (bDiff) {
                                OT.Col(poReg->GetLaen() >= 0 ? ToStringS(poReg->GetLaen() - poReg2->GetLaen()) : std::string(""), COutputTable::enRIGHT);
                            }
                            OT.Col("|").Col(poReg->isMallorn() ? "Mallorn:" : "Baeume:").Col(poReg->GetBaeume());
                            if (bDiff) {
                                OT.Col(ToStringS(poReg->GetBaeume() - poReg2->GetBaeume()), COutputTable::enRIGHT);
                            }
                        }
                        OT.Col("|").Next();

                        if (IsFlag(VF_SHOWLUXUS)) {
                            // static char Delta[32];
                            // static char Fmt[8];
                            int colcnt = 0;
                            for (size_t preis = 0; preis < poReg->GetLuxusgueter().size(); preis++) {
                                if (poReg->GetVerkauf() != (int32_t)preis) {
                                    if (!(colcnt % 3)) {
                                        OT.Col("      ");
                                    }
                                    OT.Col("|").Col(poReg->GetLuxusgueter()[preis].first + ":").Col(poReg->GetLuxusgueter()[preis].second);
                                    if (bDiff) {
                                        OT.Col((poReg2->GetLuxusgueter().size() > preis ? ToStringS(poReg->GetLuxusgueter()[preis].second - poReg2->GetLuxusgueter()[preis].second) : " "), COutputTable::enRIGHT);
                                    }
                                    if (colcnt && !((colcnt + 1) % 3)) {
                                        OT.Col("|").Next();
                                    }
                                    colcnt++;
                                }
                            }
                            //						if( poReg->GetLuxusgueter().size() ) COutput::TPrintf( "vorlage", "\n" );
                        }

                        OT.Output("vorlage", std::string(" ; "));

                        if (IsFlag(VF_SHOWLUXUS) || IsFlag(VF_SHOWLPROD)) {
                            static char Delta[32];
                            if (poReg->GetVerkauf() >= 0 && size_t(poReg->GetVerkauf()) < poReg->GetLuxusgueter().size()) {
                                COutput::TWrite("vorlage", " ; Prod.: ");
                                if (bDiff) {
                                    snprintf(Delta, sizeof(Delta), "%+5ld",
                                             (poReg2->GetLuxusgueter().size() > size_t(poReg2->GetVerkauf())) ? poReg->GetLuxusgueter()[size_t(poReg->GetVerkauf())].second - poReg2->GetLuxusgueter()[size_t(poReg2->GetVerkauf())].second : 0);
                                }
                                else {
                                    Delta[0] = 0;
                                }
                                COutput::TPrint("vorlage", "{:<10}{:4}{}    max. handelbar: {}\n", poReg->GetLuxusgueter()[size_t(poReg->GetVerkauf())].first + ":", poReg->GetLuxusgueter()[size_t(poReg->GetVerkauf())].second, Delta,
                                               poReg->GetBauern() / 100);
                            }
                        }
                        if (IsFlag(VF_SHOWMINIKARTE)) {
                            if (poRQ->DeepGetValue("herb").asString().size() > 2) {
                                COutput::TPrint("vorlage", " ; Kraut: {}\n", poRQ->DeepGetValue("herb").asString());
                            }
                        }

                        if (poReg->isVerorkt())
                            COutput::TWrite("vorlage", " ; Die Region ist verorkt!\n");

                        if (poReg->GetVGrenzen() && !poReg->GetVGrenzen()->empty()) {
                            std::ostringstream os;
                            for (size_t j = 0; j < poReg->GetVGrenzen()->size(); j++) {
                                std::string sGrenze;
                                switch ((*(poReg->GetVGrenzen()))[j] -> Richtung()) {
                                    case 0:
                                        sGrenze = "Nordwesten";
                                        break;
                                    case 1:
                                        sGrenze = "Nordosten";
                                        break;
                                    case 2:
                                        sGrenze = "Osten";
                                        break;
                                    case 3:
                                        sGrenze = "Suedosten";
                                        break;
                                    case 4:
                                        sGrenze = "Suedwesten";
                                        break;
                                    case 5:
                                        sGrenze = "Westen";
                                        break;
                                    default:
                                        sGrenze = "unbekannter Richtung";
                                        break;
                                }
                                os << " ; " << (*(poReg->GetVGrenzen()))[j] -> Typ() << " (" << (*(poReg->GetVGrenzen()))[j] -> Prozent() << "%) in " << sGrenze;
                                COutput::TPrint("vorlage", "{}\n", getAndReset(os));
                            }
                        }
                    }

                    if (IsFlag(VF_SHOWBESCHREIBUNG) && !(poReg->Beschr().empty())) {
                        WrapOut(" ; ", poReg->Beschr(), (size_t)g_nLineSize);
                    }
                    if (IsFlag(VF_SHOWHANDEL)) {
                        int32_t nKosten = 0;
                        for (size_t j = 0; j < poVE->size(); j++) {
                            if (poVE->operator[](j)->Partei() == m_nPlayer && !poVE->operator[](j)->m_nVerraeter) {
                                nKosten += CRasse::Lookup(poVE->operator[](j)->Typ()).GetValue("Unterhalt").asLong() * poVE->operator[](j)->Anzahl();
                            }
                        }
                        if (poReg->GetEinkommen() > 0) {
                            COutput::TPrint("vorlage", " ; Regionseinnahmen:{:6} Silber\n", poReg->GetEinkommen());
                        }
                        if (poReg->GetAusgaben() > 0) {
                            COutput::TPrint("vorlage", " ; Regionsausgaben: {:6} Silber\n", poReg->GetAusgaben() + nKosten);
                        }
                        else if (nKosten) {
                            COutput::TPrint("vorlage", " ; Nahrungskosten:  {:6} Silber\n", nKosten);
                        }
                    }
                }

                if (IsFlag(VF_SHOWMATPOOL)) {
                    std::string sMat;
                    poReg->AddMaterialpool(m_nPlayer, coMPool);
                    for (CRegion::Materialpool::iterator mi = coMPool.begin(); mi != coMPool.end(); mi++) {
                        if (mi != coMPool.begin()) {
                            out << ", ";
                        }
                        sMat = (*mi).first;
                        if (!sMat.empty()) {
                            size_t p = 0;
                            sMat[0] = (char)toupper(sMat[0]);
                            while ((p = sMat.find_first_of(' ', p)) != std::string::npos) {
                                if (++p < sMat.size()) {
                                    sMat[p] = (char)toupper(sMat[p]);
                                }
                            }
                        }
                        out << (*mi).second << " " << sMat;
                    }
                    if (!coMPool.empty()) {
                        WrapOut(" ;               ", std::string("Materialpool: ") + getAndReset(out), (size_t)g_nLineSize, " ; ");
                    }
                }

                if (!poReg->GetEffects().empty() && IsFlag(VF_SHOWVERBOSEINFO)) {
                    if (!bewachendeParteien.empty()) {
                        std::string bewachend;
                        for (auto pnr : bewachendeParteien) {
                            bewachend += (bewachend.empty() ? "" : ", ") + g_poCurrentReport->Parteiname(pnr).substr(1) + " (" + itoan(pnr, g_poCurrentReport->PNrBase()) + ")";
                        }
                        WrapOut(" ; ", "Bewacht von: " + bewachend, (size_t)g_nLineSize);
                    }
                    for (size_t j = 0; j < poReg->GetEffects().size(); j++) {
                        WrapOut(" ; ", poReg->GetEffects()[j], (size_t)g_nLineSize);
                    }
                }

                if (IsFlag(VF_SHOWMESSAGES)) {
                    for (CRegion::Botschaften::const_iterator bi = poReg->GetBotschaften().begin(); bi != poReg->GetBotschaften().end(); bi++) {
                        WrapOut(" ;   ", (*bi), (size_t)g_nLineSize, " ; > ");
                        //						COutput::TPrintf( "vorlage", " ; > %s\n", (*bi).c_str() );
                    }
                    for (int32_t j = 0; j < (int32_t)poReg->NumMessage(); ++j) {
                        auto pMsg = poReg->GetMessage(j);
                        auto type = pMsg->GetValue("type", Value(0));
                        if (type == -1 || !(IsFlag(VF_NOBATTLEMESSAGES) && IsEqual((*(g_poCurrentReport->MessageSections()))[pMsg->GetValue("type", Value(0)).asLong()], "battle"))) {
                            WrapOut(" ;   ", pMsg->Render(g_poCurrentReport), (size_t)g_nLineSize, " ; > ");
                        }
                    }
                    CRegion::Durchreisen::const_iterator di;
                    for (di = poReg->GetDurchreisen().begin(); di != poReg->GetDurchreisen().end(); di++) {
                        COutput::TPrint("vorlage", " ; Durchgereist:  {}\n", *di);
                    }
                    for (di = poReg->GetDurchschiffungen().begin(); di != poReg->GetDurchschiffungen().end(); di++) {
                        COutput::TPrint("vorlage", " ; Durchgesegelt: {}\n", *di);
                    }
                }

                if (!poReg->GetKommandos().empty()) {
                    for (int32_t j = 0; j < (int32_t)poReg->GetKommandos().size(); j++) {
                        if (poReg->GetKommandos()[j].empty()) {
                            COutput::TWrite("vorlage", "\n");
                        }
                        else {
                            if (poReg->GetKommandos()[j].asString()[0] == ';') {
                                WrapOut(" ; ", poReg->GetKommandos()[j].asString().substr(2), (size_t)g_nLineSize);
                            }
                        }
                    }
                }
                if (!IsFlag(VF_SHOWVERBOSEINFO)) {
                    COutput::TWrite("vorlage", "\n");
                }
                if (poReg->PersonsOf(m_nPlayer) - poReg->PersonsOf(m_nPlayer, true) > 0) {
                    WrapOut(" ; ", std::string("In dieser Region sind Einheiten als (") + itoan(m_nPlayer, g_poCurrentReport->PNrBase()) + ") getarnt!", (size_t)g_nLineSize);
                }

                bRegHead = true;
            }

            if (!IsFlag(VF_SUPPRESSUNITS)) {
                if (IsFlag(VF_SORTBURGEN) && nOrt != poReg->GetVEinheiten()[i]->Aufenthaltsort()) {
                    nOrt = poReg->GetVEinheiten()[i]->Aufenthaltsort();
                    COutput::TWrite("vorlage", "\n ; -   -   -   -   -   -   -   -   -   -   -   -\n");
                    if (!nOrt) {
                        COutput::TWrite("vorlage", " ; Auf freiem Feld:\n");
                    }
                    else if (nOrt > 0x10000000) {
                        if (poReg->GetShip(nOrt - 0x10000000)) {
                            coSchiffe.erase(poReg->GetShip(nOrt - 0x10000000));
                            SchiffAusgabe(poReg->GetShip(nOrt - 0x10000000));
                        }
                    }
                    else if (nOrt < 0x10000000) {
                        if (poReg->GetBuilding(nOrt)) {
                            coBauwerke.erase(poReg->GetBuilding(nOrt));
                            BauwerkAusgabe(poReg->GetBuilding(nOrt), poVE);
                        }
                    }
                }

                /////////////////////////////////////////////////////////////
                // Einheitenvorlage erstellen
                if (poVE->operator[](i)->Partei() == m_nPlayer && !(poVE->operator[](i)->m_nVerraeter) && (g_nOnlyGroup < 0 || poVE->operator[](i)->m_nGruppe == g_nOnlyGroup)) {
                    Einheitenvorlage(poVE->operator[](i), poRep2);
                }
                else if (IsFlag(VF_SORTFOREIGN) || (poVE->operator[](i)->Partei() == m_nPlayer && !(poVE->operator[](i)->m_nVerraeter))) {
                    FremdEinheiten(poVE->operator[](i), poRep2);
                }
            }
        }
    }

    if (IsFlag(VF_SORTFOREIGN)) {
        ShowInvisibles(poReg, poReg2, poRep2);
    }

    if (IsFlag(VF_SORTBURGEN)) {
        while (!coBauwerke.empty()) {
            COutput::TWrite("vorlage", "\n ; -   -   -   -   -   -   -   -   -   -   -   -\n");
            BauwerkAusgabe(*(coBauwerke.begin()));
            coBauwerke.erase(coBauwerke.begin());
        }

        while (!coSchiffe.empty()) {
            COutput::TWrite("vorlage", "\n ; -   -   -   -   -   -   -   -   -   -   -   -\n");
            SchiffAusgabe(*(coSchiffe.begin()));
            coSchiffe.erase(coSchiffe.begin());
        }
    }

    if (!IsFlag(VF_SUPPRESSUNITS) && bRegHead && !IsFlag(VF_SORTFOREIGN) && (IsFlag(VF_SHOWUNITS) || IsFlag(VF_SHOWUNITSVERBOSE))) {
        CEinheit* poHU;
        CEinheit* poLU;
        bool bHead = false;
        for (size_t i = 0; i < poVE->size(); i++) {
            poHU = poVE->operator[](i);
            poLU = poRep2 ? poRep2->SearchUnit(poHU->Nummer(), false) : 0;
            if ((!IsFlag(VF_SHOWUNITSNEW) || !poLU || (poLU && poLU->Region()->GetKey() != poHU->Region()->GetKey())) && (poHU->Partei() != m_nPlayer)) {
                if (!bHead) {
                    COutput::TWrite("vorlage", "\n ; -   -   -   -   -   -   -   -   -   -   -   -\n");
                    if (IsFlag(VF_SHOWUNITSNEW)) {
                        COutput::TWrite("vorlage", " ; Neue fremde Einheiten:\n");
                    }
                    else {
                        COutput::TWrite("vorlage", " ; Fremde Einheiten:\n");
                    }
                    bHead = true;
                }
                FremdEinheiten(poVE->operator[](i), poRep2);
            }
        }
    }

    if (!IsFlag(VF_SUPPRESSUNITS) && bRegHead && !IsFlag(VF_SORTFOREIGN) && (IsFlag(VF_SHOWUNITS) || IsFlag(VF_SHOWUNITSVERBOSE))) {
        ShowInvisibles(poReg, poReg2, poRep2);
    }

    if (bRegHead && IsFlag(VF_SHOWTRIBEOVERVIEW) && coPersonen.size() > 1) {
        COutput::TWrite("vorlage", "\n ; -   -   -   -   -   -   -   -   -   -   -   -\n");
        COutput::TWrite("vorlage", " ; Partei\xFC" "bersicht:\n");
        std::string sPfx;

        if (poReg2 && IsFlag(VF_SHOWTDIFF)) {
            CRegion::VEinheiten* poVEt = &poReg2->GetVEinheiten();
            for (size_t i = 0; i < poVEt->size(); i++) {
                if (coPersonen2.find(poVEt->operator[](i)->Partei()) == coPersonen2.end()) {
                    coPersonen2[poVEt->operator[](i)->Partei()] = 0;
                }

                coPersonen2[poVEt->operator[](i)->Partei()] += poVEt->operator[](i)->Anzahl();
            }
        }

        for (std::map<int32_t, int32_t>::iterator ppi = coPersonen.begin(); ppi != coPersonen.end(); ppi++) {
            if ((*ppi).first != m_nPlayer) {
                std::map<int32_t, int32_t>::iterator ppi2 = coPersonen2.find((*ppi).first);

                COutput::TWrite("vorlage", "\n");
                out.str("");
                out.clear();

                std::string sPName = g_poCurrentReport->m_coParteien[(*ppi).first];
                if (sPName.empty()) {
                    sPName = "#";
                }

                sPfx = "  ; ";
                sPfx += (*ppi).first ? sPName[0] : '-';

                if ((*ppi).first >= 0) {
                    out << " ";
                    out << sPName.substr(1);
                    out << " (";
                    out << itoan((*ppi).first, g_poCurrentReport->PNrBase());
                    out << ")";
                }
                else {
                    out << " parteigetarnt";
                }

                if (IsFlag(VF_SHOWTDIFF) && ppi2 != coPersonen2.end() && (*ppi).second != (*ppi2).second) {
                    out << ", " << (*ppi).second << "(" << std::showpos << ((*ppi).second - (*ppi2).second) << std::noshowpos << ((*ppi).second == 1 ? ") Person" : ") Personen");
                }
                else {
                    out << ", " << (*ppi).second << ((*ppi).second == 1 ? " Person" : " Personen");
                }

                if (IsFlag(VF_SHOWMATPOOL)) {
                    CRegion::Materialpool::iterator mi2;
                    std::string sMat;
                    coMPool.clear();
                    poReg->AddMaterialpool((*ppi).first, coMPool);
                    int32_t nMat2 = 0;
                    if (ppi2 != coPersonen2.end()) {
                        coMPool2.clear();
                        poReg2->AddMaterialpool((*ppi2).first, coMPool2);
                    }
                    for (CRegion::Materialpool::iterator mi = coMPool.begin(); mi != coMPool.end(); mi++) {
                        if (ppi2 != coPersonen2.end()) {
                            mi2 = coMPool2.find((*mi).first);
                            if (mi2 != coMPool2.end()) {
                                nMat2 = (*mi2).second;
                            }
                            else {
                                nMat2 = 0;
                            }
                        }
                        if (mi != coMPool.begin()) {
                            out << ", ";
                        }
                        else {
                            out << ", hat: ";
                        }
                        sMat = (*mi).first;
                        if (!sMat.empty()) {
                            sMat[0] = (char)toupper(sMat[0]);
                        }
                        if (IsFlag(VF_SHOWTDIFF) && (*mi).second != nMat2) {
                            out << (*mi).second << "(" << std::showpos << ((*mi).second - nMat2) << std::noshowpos << ") " << sMat;
                        }
                        else {
                            out << (*mi).second << " " << sMat;
                        }
                    }
                }

                WrapOut("  ;   ", getAndReset(out), (size_t)g_nLineSize, sPfx);
            }
        }
    }
    if (bRegHead && !poReg->GetEndKommandos().empty()) {
        for (int32_t i = 0; i < (int32_t)poReg->GetEndKommandos().size(); i++) {
            if (poReg->GetEndKommandos()[i].empty()) {
                COutput::TWrite("vorlage", "\n");
            }
            else {
                if (poReg->GetEndKommandos()[i].asString()[0] == ';') {
                    WrapOut(" ; ", poReg->GetEndKommandos()[i].asString().substr(2), (size_t)g_nLineSize);
                }
            }
        }
    }
}

void CVorlage::ShowInvisibles(CRegion* poReg, CRegion* poReg2, CReport* poRep2)
{
    EinheitenDB* poUnits;
    EinheitenDB::iterator ui;
    CEinheit* poHU;
    bool bHead = false;

    poUnits = &g_coREDB[poReg->GetKey()];

    for (ui = poUnits->begin(); ui != poUnits->end(); ui++) {
        if ((*ui).second->Partei() != m_nPlayer) {
            poHU = g_poCurrentReport->SearchUnit((*ui).second->Nummer(), false);
            if (!poHU) {
                EinheitenDB::iterator edbi;
                edbi = g_coREinheitenDB[0].find((*ui).second->Nummer());
                if (edbi != g_coREinheitenDB[0].end()) {
                    const CEinheit* pUT = (*edbi).second;
                    if (pUT) {
                        const CRegion* pRH = (*edbi).second->Region();
                        if (pRH && pRH->GetKey() == poReg->GetKey()) {
                            if (!bHead) {
                                COutput::TWrite("vorlage", "\n ; -   -   -   -   -   -   -   -   -   -   -   -\n");
                                COutput::TWrite("vorlage", " ; Unsichtbare oder getarnte fremde Einheiten:\n");
                                bHead = true;
                            }
                            FremdEinheiten((*ui).second, poRep2);
                        }
                    }
                }
            }
        }
    }
}

void CVorlage::BauwerkAusgabe(CBauwerk* pBuilding, CRegion::VEinheiten* poVE)
{
    int nPers = 0;
    int nUnits = 0;
    if (poVE) {
        for (size_t j = 0; j < poVE->size(); j++) {
            if (pBuilding->Nummer() == poVE->operator[](j)->Aufenthaltsort()) {
                nPers += poVE->operator[](j)->Anzahl();
                ++nUnits;
            }
        }
    }
    int32_t nKap = CBlockBase::GetValue(CBuildingInfo::Lookup(pBuilding->XTyp()), "kapazitaet").asLong();
    int32_t nCountUnits = CBlockBase::GetValue(CBuildingInfo::Lookup(pBuilding->XTyp()), "einheiten").asLong();
    COutput::TPrint("vorlage", " ; In {} '{}' ({}) [{}/{}{}]:\n", pBuilding->XTyp(), pBuilding->Name(), itoan(pBuilding->Nummer(), g_poCurrentReport->BNrBase()), nCountUnits > 0 ? nUnits : nPers, nKap ? nKap : pBuilding->Groesse(),
                   nKap && nKap != pBuilding->Groesse() ? "/" + std::to_string(pBuilding->Groesse()) : "");

    if (IsFlag(VF_SHOWBESCHREIBUNG) && !(pBuilding->Beschreibung().empty())) {
        WrapOut(" ; ", pBuilding->Beschreibung(), (size_t)g_nLineSize);
    }

    if (pBuilding->m_nBelagerer) {
        COutput::TPrint("vorlage", " ; Belagert von: {}\n", pBuilding->m_nBelagerer);
    }
    if (!pBuilding->m_coEffects.empty() && IsFlag(VF_SHOWVERBOSEINFO)) {
        for (size_t i = 0; i < pBuilding->m_coEffects.size(); i++) {
            WrapOut(" ; ", pBuilding->m_coEffects[i], (size_t)g_nLineSize);
        }
    }

    if (!pBuilding->GetKommandos().empty()) {
        for (int32_t i = 0; i < (int32_t)pBuilding->GetKommandos().size(); i++) {
            if (pBuilding->GetKommandos()[i].empty()) {
                COutput::TWrite("vorlage", "\n");
            }
            else {
                if (pBuilding->GetKommandos()[i].asString()[0] == ';') {
                    WrapOut(" ; ", pBuilding->GetKommandos()[i].asString().substr(2), (size_t)g_nLineSize);
                }
            }
        }
    }
}

void CVorlage::SchiffAusgabe(CSchiff* pSchiff)
{
    int32_t count = pSchiff->Anzahl();
    std::string anzahl = count == 1 ? "" : ", Anzahl " + std::to_string(count) + ",";
    // static int dbgcount = 0;
    if (pSchiff->MaxHolz() && pSchiff->MaxHolz() * count != pSchiff->Holz()) {
        COutput::TPrint("vorlage", " ; An Bord von {} '{}' ({}){} ({}/0) im Bau ({}/{}):\n", pSchiff->Typ(), pSchiff->Name(), itoan(pSchiff->Nummer(), g_poCurrentReport->BNrBase()), anzahl, pSchiff->Ladung(), pSchiff->Holz(),
                        pSchiff->MaxHolz() * count);
    }
    else {
        COutput::TPrint("vorlage", " ; An Bord von {} '{}' ({}){} Kap: {}GE/{}GE({}%):\n", pSchiff->Typ(), pSchiff->Name(), itoan(pSchiff->Nummer(), g_poCurrentReport->BNrBase()), anzahl, pSchiff->MaxLadung() - pSchiff->Ladung(),
                        pSchiff->MaxLadung(), pSchiff->Schaden());
    }
    if (IsFlag(VF_SHOWBESCHREIBUNG) && !(pSchiff->Beschreibung().empty())) {
        WrapOut(" ; ", pSchiff->Beschreibung(), (size_t)g_nLineSize);
    }

    if (!pSchiff->m_coEffects.empty() && IsFlag(VF_SHOWVERBOSEINFO)) {
        for (size_t i = 0; i < pSchiff->m_coEffects.size(); i++) {
            WrapOut(" ; ", pSchiff->m_coEffects[i], (size_t)g_nLineSize);
        }
    }

    if (!pSchiff->GetKommandos().empty()) {
        for (int32_t i = 0; i < (int32_t)pSchiff->GetKommandos().size(); i++) {
            if (pSchiff->GetKommandos()[i].empty()) {
                COutput::TWrite("vorlage", "\n");
            }
            else {
                if (pSchiff->GetKommandos()[i].asString()[0] == ';') {
                    WrapOut(" ; ", pSchiff->GetKommandos()[i].asString().substr(2), (size_t)g_nLineSize);
                }
            }
        }
    }
}

void CVorlage::Einheitenvorlage(CEinheit* poUnit, CReport* poRep2)
{
    //	VKommandos coMetaErg;
    CEinheit* poLU;
    std::ostringstream out;
    const char* pcKampf;
    // bool bMetas = false;
    bool bKommando = false;
    size_t i;

    m_poCurrentUnit = poUnit;
    g_poCurrentUnit = poUnit;

    poLU = poRep2 ? poRep2->SearchUnit(poUnit->Nummer(), false) : 0;

    if (poUnit->m_nSchiff) {
        out << "," << ((m_poCurrentRegion->GetShip(poUnit->m_nSchiff) && m_poCurrentRegion->GetShip(poUnit->m_nSchiff)->Kapitaen() == poUnit->m_nNummer) ? 'S' : 's') << ":" << itoan(poUnit->m_nSchiff, g_poCurrentReport->BNrBase());
    }
    if (IsEqual(poUnit->m_sTyp.c_str(), "Untote") || (0 == CRasse::Lookup(poUnit->RealType()).GetValue(std::string("Unterhalt")).asLong() && 0 != CRasse::Lookup(poUnit->RealType()).GetValue(std::string("Gewicht")).asLong())) {
        out << ",I";
    }
    if (poUnit->m_nBauwerk) {
        CBauwerk* pBW = m_poCurrentRegion->GetBuilding(poUnit->m_nBauwerk);
        int nI = pBW ? pBW->Groesse() : 0;
        if (pBW) {
            int32_t nKap = CBlockBase::GetValue(CBuildingInfo::Lookup(pBW->XTyp()), "kapazitaet").asLong();
            if (nKap) {
                nI = nKap;
            }
        }
        if (pBW && (pBW->m_nBesitzer == poUnit->m_nNummer || (!pBW->m_nBesitzer && poUnit->m_nPlace == 1))) {
            bKommando = true;
        }
        if (bKommando && pBW->Unterhalt()) {
            out << ",U" << pBW->Unterhalt();
        }
        if (IsFlag(VF_SHOWVERBOSEINFO)) {
            out << "," << (bKommando ? 'B' : 'b') << ":" << itoan(poUnit->m_nBauwerk, g_poCurrentReport->BNrBase()) << "(" << poUnit->m_nPlace << "/" << nI << ")";
        }
    }

    static bool bReadKampfStati = false;
    static std::map<int, std::string> csKampfStatiMap;
    if (!bReadKampfStati) {
        bReadKampfStati = true;
        CConfigFile oCF(GetConfigFileName());
        size_t j = 1;
        while (true) {
            if (!oCF.FetchLine("Kampfstatus", j++, false)) {
                break;
            }
            csKampfStatiMap[oCF.GetLong(1)] = oCF.GetString(0);
        }
    }
    if (!csKampfStatiMap.empty() && csKampfStatiMap.find(poUnit->m_nKampfStatus) != csKampfStatiMap.end()) {
        pcKampf = csKampfStatiMap[poUnit->m_nKampfStatus].c_str();
    }
    else {
        if (IsFlag(VF_NEWERESSEASTATI)) {
            switch (poUnit->m_nKampfStatus) {
                case 0:
                    pcKampf = "aggressiv";
                    break;
                case 1:
                    pcKampf = "vorne";
                    break;
                case 2:
                    pcKampf = "hinten";
                    break;
                case 3:
                    pcKampf = "defensiv";
                    break;
                case 4:
                    pcKampf = "k\xE4mpft nicht";
                    break;
                case 5:
                    pcKampf = "flieht";
                    break;
                default:
                    pcKampf = "unbekannt";
            }
        }
        else {
            switch (poUnit->m_nKampfStatus) {
                case 0:
                    pcKampf = "vorne";
                    break;
                case 1:
                    pcKampf = "hinten";
                    break;
                case 2:
                    pcKampf = "k\xE4mpft nicht";
                    break;
                case 3:
                    pcKampf = "flieht";
                    break;
                default:
                    pcKampf = "unbekannt";
            }
        }
    }

    if (CMetaCommand::ProcExists("CreateUnitHeader")) {
        VKommandos coOutput;
        CMetaCommand::Call("CreateUnitHeader", coOutput);
        if (!coOutput.empty()) {
            for (int32_t j = 0; j < (int32_t)coOutput.size(); j++) {
                COutput::TPrint("vorlage", "  {}\n", coOutput[j].c_str());
            }
        }
    }
    else {
        //	if( IsFlag( VF_BASE36 ) )
        if (IsFlag(VF_SHOWVERBOSEINFO)) {
            COutput::TPrint("vorlage", "\n  EINHEIT {};  {} [{},{}${}] {}{}{}{}{}{}\n", itoan(poUnit->m_nNummer, g_poCurrentReport->ENrBase()), poUnit->m_sName, poUnit->m_nAnzahl, poUnit->m_nSilber, getAndReset(out),
                            !poUnit->WahrerTyp().empty() ? poUnit->Typ() + ", " : "", poUnit->m_nParteitarnung ? "parteigetarnt, " : "", poUnit->m_nBewacht ? "bewacht, " : "",
                            poUnit->m_shp.empty() ? std::string{} : poUnit->m_shp + ", ", pcKampf, poUnit->m_nHunger ? ", hungert" : "");
            if (poUnit->m_nVerkleidung) {
                COutput::TPrint("vorlage", "  ; Verkleidet als {} ({})\n", poUnit->Region()->Map()->Report()->Parteiname(poUnit->m_nVerkleidung).substr(1), itoan(poUnit->m_nVerkleidung, g_poCurrentReport->PNrBase()));
            }
            if (poUnit->m_nVerraeter) {
                COutput::TWrite("vorlage", "  ; VERR\xC4TER!\n");
            }
            //	else
            //	    COutput::TPrintf( "vorlage", "\n  EINHEIT %6d;  %s [%d,%d$%s] %s%s%s%s%s%s\n", poUnit->m_nNummer, poUnit->m_sName.c_str(), poUnit->m_nAnzahl, poUnit->m_nSilber, Buff, (!poUnit->m_sWahrerTyp.empty())?poUnit->m_sTyp.c_str():"",
            // poUnit->m_nParteitarnung?"parteigetarnt, ":"", poUnit->m_nBewacht?"bewacht, ":"", poUnit->m_shp.empty()?"":std::string( poUnit->m_shp + ", " ).c_str(), pcKampf, (poUnit->m_nHunger)?", hungert":"" );
        }
        else {
            COutput::TPrint("vorlage", "  EINHEIT {};  {} [{},{}${}]\n", itoan(poUnit->m_nNummer, g_poCurrentReport->ENrBase()), poUnit->m_sName, poUnit->m_nAnzahl, poUnit->m_nSilber, getAndReset(out));
        }
        if (!poUnit->Gruppe().empty() && IsFlag(VF_SHOWVERBOSEINFO)) {
            WrapOut("  ; In Gruppe: ", poUnit->Gruppe(), (size_t)g_nLineSize);
        }
        if (poUnit->CBlockBase::GetValue("hero").asLong() && IsFlag(VF_SHOWVERBOSEINFO)) {
            COutput::TWrite("vorlage", "  ; Heldenstatus!\n");
        }

        if (poUnit->GetValue("unaided", "").asLong()) {
            COutput::TWrite("vorlage", "  ; Bekommt im Kampf keine Hilfe!\n");
        }

        if (IsFlag(VF_SHOWBESCHREIBUNG) && !(poUnit->Beschreibung().empty())) {
            std::string sDescr = poUnit->m_sBeschreibung;
            if (IsFlag(VF_STRIPDUPLICATEDESCR)) {
                if (sDescr.length() > 60 && g_csDupDescrFilter.insert(sDescr).second) {
                    sDescr = sDescr.substr(0, 57) + " ...";
                }
            }
            WrapOut("  ; ", sDescr, (size_t)g_nLineSize);
        }

        if (IsFlag(VF_SHOWLASTEN)) {
            poUnit->Kapazitaeten("vorlage");
        }
        if (poUnit->m_nSchiff && m_poCurrentRegion->GetShip(poUnit->m_nSchiff) && m_poCurrentRegion->GetShip(poUnit->m_nSchiff)->Kapitaen() == poUnit->m_nNummer && IsFlag(VF_SHOWVERBOSEINFO)) {
            std::string sKueste = "auf offener See";
            // NW = 0, NO = 1, O = 2, SO = 3, SW =4, W =5
            if (m_poCurrentRegion->GetTerrain()->m_bLand) {
                sKueste = ", auslaufen nach ";
                switch (m_poCurrentRegion->GetShip(poUnit->m_nSchiff)->Kueste()) {
                    case 0:
                        sKueste += "Nordwesten";
                        break;
                    case 1:
                        sKueste += "Nordosten";
                        break;
                    case 2:
                        sKueste += "Osten";
                        break;
                    case 3:
                        sKueste += "Suedosten";
                        break;
                    case 4:
                        sKueste += "Suedwesten";
                        break;
                    case 5:
                        sKueste += "Westen";
                        break;
                    default:
                        sKueste = ", Ablegerichtung beliebig";
                        break;
                }
            }
            out << "Kapitaen von " << m_poCurrentRegion->GetShip(poUnit->m_nSchiff)->Name() << " (" << itoan(m_poCurrentRegion->GetShip(poUnit->m_nSchiff)->Nummer(), g_poCurrentReport->BNrBase()) << ") " << sKueste;
            WrapOut("  ; ", getAndReset(out), (size_t)g_nLineSize);
        }

        if (IsFlag(VF_SHOWPRIVAT) && !poUnit->m_sPrivat.empty()) {
            WrapOut("  ; ", std::string("Privat: ") + poUnit->m_sPrivat, (size_t)g_nLineSize);
        }

        if (!poUnit->m_coEffects.empty() && IsFlag(VF_SHOWVERBOSEINFO)) {
            for (size_t j = 0; j < poUnit->m_coEffects.size(); j++) {
                WrapOut("  ; ", poUnit->m_coEffects[j], (size_t)g_nLineSize);
            }
        }

        if (IsFlag(VF_SHOWMESSAGES)) {
            for (CEinheit::Botschaften::const_iterator bi = poUnit->m_coBotschaften.begin(); bi != poUnit->m_coBotschaften.end(); bi++) {
                WrapOut("  ;   ", (*bi), (size_t)g_nLineSize, "  ; ");
                //			COutput::TPrintf( "vorlage", "  ; %s\n", (*bi).c_str() );
            }
            for (CEinheit::Messages::const_iterator mi = poUnit->m_cpoMessages.begin(); mi != poUnit->m_cpoMessages.end(); mi++) {
                WrapOut("  ;   ", ((CMessage*)((*mi).get()))->Render(g_poCurrentReport), (size_t)g_nLineSize, "  ; > ");
            }
        }

        if (IsFlag(VF_SHOWTALENTE)) {
            if (poUnit->m_coSprueche.size()) {
                out << "Spr\xFC"
                       "che: ";
                for (i = 0; i < poUnit->m_coSprueche.size(); i++) {
                    if (i) {
                        out << ", ";
                    }
                    out << poUnit->m_coSprueche[i];
                }
                WrapOut("  ; ", getAndReset(out), (size_t)g_nLineSize);
            }
            if (poUnit->m_cpoKampfzauber.size()) {
                out << "Kampfzauber: ";
                for (i = 0; i < poUnit->m_cpoKampfzauber.size(); i++) {
                    if (i) {
                        out << ", ";
                    }
                    out << poUnit->m_cpoKampfzauber[i]->GetValue("name").asString() << " " << poUnit->m_cpoKampfzauber[i]->GetValue("level").asLong();
                }
                WrapOut("  ; ", getAndReset(out), (size_t)g_nLineSize);
            }

            for (i = 0; i < poUnit->m_coTalente.size(); i++) {
                if (i) {
                    out << ", ";
                }
                if (poUnit->m_nTarnung >= 0 && IsEqual(poUnit->m_coTalente[i].m_sTyp.c_str(), "Tarnung")) {
                    if (IsFlag(VF_NOSKILLPOINTS)) {
                        if (IsFlag(VF_SHOWTDIFF) && poLU && (poUnit->m_coTalente[i].m_nStufe - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Stufe").asLong())) {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_nTarnung << "/" << poUnit->m_coTalente[i].m_nStufe << " (" << std::showpos
                                << (poUnit->m_coTalente[i].m_nStufe - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Stufe").asLong()) << std::noshowpos << ")";
                        }
                        else {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_nTarnung << "/" << poUnit->m_coTalente[i].m_nStufe;
                        }
                    }
                    else {
                        if (IsFlag(VF_SHOWTDIFF) && poLU && (poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Tage").asLong() / poLU->m_nAnzahl)) {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_nTarnung << "/" << poUnit->m_coTalente[i].m_nStufe << " [" << poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl << "(" << std::showpos
                                << (poLU ? poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Tage").asLong() / poLU->m_nAnzahl : 0) << std::noshowpos << ")]";
                        }
                        else {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_nTarnung << "/" << poUnit->m_coTalente[i].m_nStufe << " [" << poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl << "]";
                        }
                    }
                }
                else {
                    if (IsFlag(VF_NOSKILLPOINTS)) {
                        if (IsFlag(VF_SHOWTDIFF) && poLU && (poUnit->m_coTalente[i].m_nStufe - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Stufe").asLong())) {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_coTalente[i].m_nStufe << " (" << std::showpos << (poUnit->m_coTalente[i].m_nStufe - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Stufe").asLong()) << std::noshowpos
                                << ")";
                        }
                        else {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_coTalente[i].m_nStufe;
                        }
                    }
                    else {
                        if (IsFlag(VF_SHOWTDIFF) && poLU && (poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Tage").asLong() / poLU->m_nAnzahl)) {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_coTalente[i].m_nStufe << " [" << poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl << "(" << std::showpos
                                << (poLU ? poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl - poLU->GetValue(poUnit->m_coTalente[i].m_sTyp, "Tage").asLong() / poLU->m_nAnzahl : 0) << std::noshowpos << ")]";
                        }
                        else {
                            out << poUnit->m_coTalente[i].m_sTyp << " " << poUnit->m_coTalente[i].m_nStufe << " [" << poUnit->m_coTalente[i].m_nTage / poUnit->m_nAnzahl << "]";
                        }
                    }
                }
                if (IsEqual(poUnit->m_coTalente[i].m_sTyp.c_str(), "Magie") && poUnit->m_nAuramax > 0) {
                    if (IsFlag(VF_SHOWTDIFF) && poLU && (poUnit->m_nAura - poLU->m_nAura)) {
                        out << ", Aura " << poUnit->m_nAura << "(" << std::showpos << (poUnit->m_nAura - poLU->m_nAura) << std::noshowpos << ")";
                    }
                    else {
                        out << ", Aura " << poUnit->m_nAura;
                    }
                    if (IsFlag(VF_SHOWTDIFF) && poLU && (poUnit->m_nAuramax - poLU->m_nAuramax)) {
                        out << " [" << poUnit->m_nAuramax << "(" << std::showpos << (poUnit->m_nAuramax - poLU->m_nAuramax) << std::noshowpos << ")]";
                    }
                    else {
                        out << " [" << poUnit->m_nAuramax << "]";
                    }
                }
            }
            if (i) {
                WrapOut("  ; ", getAndReset(out), (size_t)g_nLineSize);
            }
        }
        if (IsFlag(VF_SHOWGEGENSTAENDE)) {
            bool bSome = false;
            for (i = 0; i < poUnit->m_coGegenstaende.size(); i++) {
                if (!IsEqual(poUnit->m_coGegenstaende[i].first, "Silber")) {
                    if (bSome) {
                        out << ", ";
                    }
                    out << poUnit->m_coGegenstaende[i].second << " " << poUnit->m_coGegenstaende[i].first.c_str();
                    bSome = true;
                }
            }
            if (bSome) {
                WrapOut("  ; ", getAndReset(out), (size_t)g_nLineSize);
            }
        }
    }

    if (!IsFlag(VF_FULLCOMMANDOUTPUT)) {
        for (int32_t j = 0; j < (int32_t)poUnit->m_csKommandos.size(); j++) {
            if (poUnit->m_csMetaOut.empty() || IsFlag(VF_DONTKILLCOMMANDS)) {
                COutput::TPrint("vorlage", "    {}\n", poUnit->m_csKommandos[j].c_str());
            }
            else {
                auto p = poUnit->m_csKommandos[j].asString().find_first_not_of(" \t");
                if (poUnit->m_csKommandos[j].empty() || (p != std::string::npos && (poUnit->m_csKommandos[j].asString()[p] == '/' || poUnit->m_csKommandos[j].asString()[p] == ';'))) {
                    COutput::TPrint("vorlage", "    {}\n", poUnit->m_csKommandos[j].c_str());
                }
                else {
                    poUnit->m_csKommandos[j] = Value("");
                }
            }
        }
    }

    for (int32_t j = 0; j < (int32_t)poUnit->m_csMetaOut.size(); j++) {
        if (!poUnit->m_csMetaOut[j].empty() && poUnit->m_csMetaOut[j].asString()[0] == ';') {
            WrapOut("    ; ", poUnit->m_csMetaOut[j].c_str(), (size_t)g_nLineSize, "    ");
        }
        else if (!poUnit->m_csMetaOut[j].empty() && poUnit->m_csMetaOut[j].asString()[0] == '/') {
            COutput::TPrint("vorlage", "    {}\n", poUnit->m_csMetaOut[j].c_str());
        }
        else {
            std::string sLine = poUnit->m_csMetaOut[j].c_str();
            std::string sPref;
            size_t nLSize = (size_t)g_nCommandLineSize - 4;
            while (sLine.length() > nLSize) {
                if (sLine[nLSize - 2] == ' ') {
                    COutput::TPrint("vorlage", "    {}{}\\\n", sPref, sLine.substr(0, nLSize - 1));
                    sLine.erase(0, nLSize - 1);
                }
                else {
                    COutput::TPrint("vorlage", "    {}{}\\\n", sPref, sLine.substr(0, nLSize - 2));
                    sLine.erase(0, nLSize - 2);
                }
                if (sPref.empty()) {
                    nLSize -= 4;
                    sPref = "    ";
                }
            }
            COutput::TPrint("vorlage", "    {}{}\n", sPref, sLine);
        }
        //        COutput::TPrintf( "vorlage", "    %s\n", poUnit->m_csMetaOut[i].c_str() );
    }
    if (IsFlag(VF_FULLCOMMANDOUTPUT) && !poUnit->GetMetaOut().changed()) {
        // Emulation des Verhaltens ohne --fullcom f�r Einheiten ohne Ergebnisse durch das Script
        for (int32_t j = 0; j < (int32_t)poUnit->m_csKommandos.size(); j++) {
            auto p = poUnit->m_csKommandos[j].asString().find_first_not_of(" \t");
            if (!poUnit->m_csKommandos[j].empty() && p != std::string::npos && poUnit->m_csKommandos[j].asString()[p] != '/') {
                COutput::TPrint("vorlage", "    {}\n", poUnit->m_csKommandos[j].c_str());
            }
        }
    }
}

void CVorlage::FremdEinheiten(CEinheit* poUnit, CReport* poRep2)
{
    CEinheit* poHU;
    CEinheit* poLU;
    std::ostringstream out;
    std::string sPfx;

    poHU = poUnit;
    poLU = poRep2 ? poRep2->SearchUnit(poUnit->Nummer(), false) : 0;

    if (!IsFlag(VF_SHOWUNITSNEW) || !poLU || (poLU && poLU->Region()->GetKey() != poHU->Region()->GetKey()) || (poLU && poHU->Partei() != poLU->Partei())) {
        COutput::TWrite("vorlage", "\n");

        std::string sPName = g_poCurrentReport->m_coParteien[poUnit->m_nPartei];
        if (sPName.empty()) {
            sPName = "#";
        }

        sPfx = "  ; ";
        sPfx += poUnit->m_nPartei ? sPName[0] : '-';

        poHU = g_poCurrentReport->SearchUnit(poUnit->Nummer(), false);
        if (!poHU) {
            sPfx += "!";
        }
        else {
            if (!poUnit->GetQuality() || poUnit->m_nVerraeter || (poUnit->GetQuality() > poHU->GetQuality() && (sPName[0] != '+' || !poHU->GetQuality()))) {
                sPfx += "!";
            }
            else {
                sPfx += " ";
            }
        }

        // Name und Einheitennummer
        out << poUnit->m_sName << " (" << itoan(poUnit->m_nNummer, g_poCurrentReport->ENrBase()) << ")";

        // Partei
        if (poUnit->m_nPartei >= 0) {
            out << ", ";
            if (poUnit->m_nVerraeter) {
                out << "getarnt als ";
            }
            out << sPName.substr(1) << " (" << itoan(poUnit->m_nPartei, g_poCurrentReport->PNrBase()) << ")";
        }
        else {
            out << ", parteigetarnt";
        }
        if (poUnit->m_nVerraeter) {
            out << ", Verr\xE4ter";
        }
        if (!poUnit->Gruppe().empty()) {
            out << ", Mitglied in " << poUnit->Gruppe();
        }
        // Anzahl und Typ
        if (!poUnit->m_sTyp.empty()) {
            std::string sTyp = poUnit->m_sWahrerTyp.empty() ? poUnit->PrefixedTyp() : poUnit->PrefixedTyp(true);
            std::string sITyp = poUnit->m_sWahrerTyp.empty() ? poUnit->Typ() : poUnit->WahrerTyp();
            if (poUnit->m_nAnzahl == 1 && sTyp.size()) {
                if (sITyp[sITyp.size() - 1] == 'n' && sITyp[0] != 'K') {
                    sTyp.erase(sTyp.size() - 2);
                }
                else {
                    sTyp.erase(sTyp.size() - 1);
                }
            }
            out << ", " << poUnit->m_nAnzahl << " " << sTyp;
        }

        // Aus Richtung
        if (poRep2) {
            if (poLU && (poLU->Region()->GetKey() != poUnit->Region()->GetKey())) {
                char b[32];
                out << ", aus " << (poLU->Region()->GetName().empty() && poLU->Region()->GetTerrain() ? poLU->Region()->GetTerrain()->m_sName : poLU->Region()->GetName()) << " (" << poLU->Region()->GetEX() << "," << poLU->Region()->GetEY() << ")";
            }
        }

        static bool bReadSilverMask = true;
        static std::map<int32_t, std::string> coSilverMasks;
        if (bReadSilverMask) {
            bReadSilverMask = false;
            CConfigFile oCF(GetConfigFileName());
            size_t i = 1;
            while (true) {
                if (!oCF.FetchLine("SilverMasks", i++, false)) {
                    break;
                }
                coSilverMasks[oCF.GetLong(1)] = oCF.GetString(0);
            }
        }

        // Verbose-Infos
        if (IsFlag(VF_SHOWUNITSVERBOSE)) {
            if (poUnit->m_nSilber || !poUnit->m_coGegenstaende.empty()) {
                out << ", hat: ";
            }
            if (poUnit->m_nSilber) {
                if (poUnit->GetQuality() == 10) {
                    out << poUnit->m_nSilber << " Silber";
                }
                else {
                    if (coSilverMasks.empty()) {
                        if (poUnit->m_nSilber >= 5000) {
                            out << " Silberkassette";
                        }
                        else {
                            out << " Silberbeutel";
                        }
                    }
                    else {
                        for (std::map<int32_t, std::string>::const_iterator i = coSilverMasks.begin(); i != coSilverMasks.end(); i++) {
                            if (poUnit->m_nSilber >= (*i).first) {
                                out << (poUnit->m_nSilber / (*i).first) << (*i).second;
                                break;
                            }
                        }
                    }
                }
            }

            // Gegenstaende
            bool bSome = false;
            for (size_t gi = 0; gi < poUnit->m_coGegenstaende.size(); gi++) {
                if (!IsEqual(poUnit->m_coGegenstaende[gi].first, "Silber")) {
                    if (bSome || poUnit->m_nSilber) {
                        out << ", ";
                    }
                    out << poUnit->m_coGegenstaende[gi].second << " " << poUnit->m_coGegenstaende[gi].first;
                    bSome = true;
                }
            }

            // Talente
            for (size_t ti = 0; ti < poUnit->m_coTalente.size(); ti++) {
                if (!ti) {
                    out << ", Talente: ";
                }
                else {
                    out << ", ";
                }
                out << poUnit->m_coTalente[ti].m_sTyp << " " << poUnit->m_coTalente[ti].m_nStufe << "[" << (poUnit->m_coTalente[ti].m_nTage / poUnit->m_nAnzahl) << "]";
            }

            // Beschreibung
            if (!poUnit->m_sBeschreibung.empty()) {
                if (IsFlag(VF_STRIPDUPLICATEDESCR)) {
                    if (poUnit->m_sBeschreibung.length() < 60 || !g_csDupDescrFilter.insert(poUnit->m_sBeschreibung).second) {
                        out << "; " << poUnit->m_sBeschreibung;
                    }
                    else {
                        out << "; " << poUnit->m_sBeschreibung.substr(0, 57) << " ...";
                    }
                }
                else {
                    out << "; " << poUnit->m_sBeschreibung;
                }
            }
        }

        // Ausgabe
        WrapOut("  ;   ", getAndReset(out), (size_t)g_nLineSize, sPfx);
    }
}

static bool OpenOXFile(const std::string& sRName, std::string& sOXName, bool bForce = false)
{
    std::string sName;

    if (sOXName.empty()) {
        return false;
    }

    sName = sRName;
    if (sName.length() > 3 && IsEqual(sName.substr(sName.length() - 3).c_str(), ".cr")) {
        sName.erase(sName.length() - 2);
        if (sOXName[0] == '.') {
            sOXName.erase(0, 1);
        }
        sName += sOXName;

        if (!bForce && FileExists(sName)) {
            ERRMSG(0, ("FEHLER: Datei '%s' existiert bereits!\n", sName.c_str()));
        }
        else {
            COutput::SetTarget("vorlage", new COutput(sName));
            //			g_hOut = fopen( sName.c_str(), "w" );
            if (!COutput::Target("vorlage")->IsOkay()) {
                ERRMSG(0, ("FEHLER: Datei '%s' konnte nicht fuer die Ausgabe ge\xF6"
                           "ffnet werden!\n",
                           sName.c_str()));
                return false;
            }
            return true;
        }
    }
    return false;
}

extern "C" {
static void ExitHandler()
{
    //    CRNENode::ClearRules();
    CHierarchy::Free();
    Expression::clearAllVars();
    COutput::CloseTargets();
    CloseAllOpenFiles();
}
}

/////////////////////////////////////////////////////////////////////
//.block: main
/////////////////////////////////////////////////////////////////////
// -f -k -t -g -n -l -uv -sb -st -ox zv -i beispiel.vms
int main(int argc, char* argv[])
{
    //    CrashDumpHandler oCDH( std::string( VERSIONINFO ) + "(Build " + ToString( (int32_t)BUILDNUMBER ) + ")" );
    CKarte::IslandQueue cpoQueue;
    std::vector<std::string> coArgs;
    std::vector<std::string> coScripts;
    std::vector<std::string> coReports;
    std::vector<CReport*> cpoReports;
    std::vector<int32_t> coPlayers;
    std::string sPlayer;
    std::string sMapOutName;
    std::string sConfigPath;
    clock_t nStart;
    clock_t nTStart;
    std::string sOName;
    std::string sOXName;
    // std::string sOPName;
    std::string sGroupName;
    bool bForce = false;
    bool bTime = true;
    // bool bCROut = false;
    bool bLocalHeader = true;
    int32_t nPlayer = -1;
    int nTX, nTY;
    unsigned int i = 1;
    size_t j;
    // int p;

#ifdef _MSC_VER
    _old_new_handler = _set_new_handler(my_new_handler);
#endif

    atexit(ExitHandler);

    setvbuf(stdout, 0, _IONBF, 0);
    setvbuf(stderr, 0, _IONBF, 0);

    COutput::SetTarget("stdout", new COutput(stdout));
    COutput::SetTarget("stderr", new COutput(stderr));
    COutput::SetTarget("vorlage", new COutput(stdout));
    COutput::SetTarget("debug", new COutput(stderr));
    COutput::SetTarget("trace", new COutput(stderr));
    if (IsConsole(stdout))
        COutput::SetTarget("console", new COutput(stdout));
    else if (IsConsole(stderr))
        COutput::SetTarget("console", new COutput(stderr));
    else {
        COutput::SetTarget("console", new COutput(stderr));
        g_coFlags.insert(VF_NOCONSOLE);
    }

    //	printf("sizeof:\nCExpression::Variables: %d\nstd::vector<CReference*>: %d\nCMCI: %d\n",
    //		   sizeof(Expression::Variables), sizeof(std::vector<CReference*>), sizeof(CMCI) );
    /*
        printf("sizeof(CRegion)=%d\n", sizeof(CRegion));
        printf("sizeof(DummyRegion)=%d\n", sizeof(DummyRegion));
        printf("sizeof(CBlockBase)=%d\n", sizeof(CBlockBase));
        printf("sizeof(Value)=%d\n", sizeof(Value));
    */
    g_pfErrorFunc = VorlageErrorMsg;

    g_oScriptBase.Import("<internal:1>", "#func EMR_building $BNr\n{\n$BNr=itoan(antoi($BNr,10),36)\n#return building[$BNr].name+' ('+$BNr+')'\n}\n");
    g_oScriptBase.Import("<internal:2>", "#func EMR_eq $Val1 $Val2\n{\n#return $Val1==$Val2\n}\n");
    g_oScriptBase.Import("<internal:3>", "#func EMR_faction $PNr\n{\n$PNr=itoan(antoi($PNr,10),36)\n#return partei[$PNr].parteiname+' ('+$PNr+')'\n}\n");
    g_oScriptBase.Import("<internal:4>", "#func EMR_if $CC $Val1 $Val2\n{\n#if $CC=='0'\n{\n#return $Val2\n}\n#else\n{\n#return $Val1\n}\n}\n");
    g_oScriptBase.Import("<internal:5>", "#func EMR_int $Val\n{\n#return $Val\n}\n");
    g_oScriptBase.Import("<internal:6>", "#func EMR_isnull $Val\n{\n#return !$Val||$Val=='-1'\n}\n");
    g_oScriptBase.Import("<internal:7>",
                         "#func EMR_region $RegInfo\n{\n#if $($RegInfo).z\n{\n#return $($RegInfo).name+' ('+$($RegInfo).x+','+$($RegInfo).y+','+$($RegInfo).z+')'\n}\n#else\n{\n#return $($RegInfo).name+' ('+$($RegInfo).x+','+$($RegInfo).y+')'\n}\n}\n");
    g_oScriptBase.Import("<internal:8>", "#func EMR_resource $Resource $Wanted\n{\n#return $Resource\n}");
    g_oScriptBase.Import("<internal:9>", "#func EMR_skill $Skill\n{\n#return $Skill\n}\n");
    g_oScriptBase.Import("<internal:10>", "#func EMR_order $Order\n{\n#return $Order\n}\n");
    g_oScriptBase.Import("<internal:11>", "#func EMR_unit $ENr\n{\n$ENr=itoan(antoi($ENr,10),36)\n#return unit[$ENr].name+' ('+$ENr+')'\n}\n");
    g_oScriptBase.Import("<internal:12>", "#func EMR_strlen $String\n{\n#return length($String)\n}\n");

    Value oBuild(int32_t(PBEMTOOLS_BUILD_NUMBER_EMU));
    Value oVersion{std::string(PBEMTOOLS_VERSION_STRING_SHORT)};
    Expression::setConstant("BUILDNUMBER", &oBuild);
    Expression::setConstant("TOOLVERSION", &oVersion);

    if (clock() == (clock_t)-1) {
        TRACEMSG(("Keine Zeitmessung m\xF6glich!\n"));
        bTime = false;
    }
    nTStart = clock();

    setlocale(LC_CTYPE, "German");
    SetBreakHandler(MyBreak);

    char* pOpts = getenv("VORLAGEOPTIONS");
    if (pOpts) {
        split(coArgs, std::string(pOpts), std::string(" \t"), std::string("\0x22'"), '\\');
    }

    for (j = 1; j < (size_t)argc; j++) {
        if (strlen(argv[j]) && argv[j][0] == '@' && strcmp(argv[j - 1], "-o")) {
            std::fstream oIS;
            std::string sLine;
            oIS.open(&argv[j][1], ios::in);

            if (oIS.fail()) {
                ERRMSG(0, ("FEHLER: Steuerdatei-Datei '%s' konnte nicht ge\xF6"
                           "ffnet werden!\n",
                           &argv[j][1]));
            }
            else {
                while (true) {
                    getline(oIS, sLine);
                    if (oIS.fail()) {
                        break;
                    }
                    while (!sLine.empty() && sLine[sLine.size() - 1] < 32 && sLine[sLine.size() - 1] > 0) {
                        sLine.erase(sLine.size() - 1, 1);
                    }
                    split(coArgs, sLine, std::string(" \t"), std::string("\0x22'"), '\\');
                }
            }
        }
        else if (!strcmp(argv[j], "--cfgpath")) {
            j++;
            if (j < (size_t)argc) {
                sConfigPath = argv[j];
            }
        }
        else
            coArgs.push_back(std::string(argv[j]));
    }

    i = 0;

//	g_sConfigPathName = argv[0];
#ifdef _WIN32
    if (sConfigPath.empty()) {
        g_sConfigPathName = GetExecutablePathname();
    }
    else {
        g_sConfigPathName = sConfigPath;
        if (sConfigPath[sConfigPath.length() - 1] != '/' && sConfigPath[sConfigPath.length() - 1] != '\\')
            g_sConfigPathName += "/";
    }
    auto p = g_sConfigPathName.find_last_of("\\/:");
    if (p == std::string::npos) {
        g_sConfigPathName = "";
    }
    else {
        g_sConfigPathName.erase(p + 1);
    }
    g_sConfigPathName += g_sSpiel + std::string(".cfg");
#else
    if (sConfigPath.empty()) {
        g_sConfigPathName = getenv("HOME");
    }
    else {
        g_sConfigPathName = sConfigPath;
    }
    if (!g_sConfigPathName.empty() || g_sConfigPathName[g_sConfigPathName.size() - 1] != '/') {
        g_sConfigPathName += "/";
    }
    g_sConfigPathName += std::string(".") + g_sSpiel + std::string("rc");
#endif

    for (j = 0; j < coArgs.size(); j++) {
        if (!strcmp("-o", coArgs[j].c_str()) || !strcmp("-ox", coArgs[j].c_str())) {
            COutput::SetTarget("stderr", new COutput(stdout));
        }
        else if (!strcmp("-q", coArgs[i].c_str())) {
            bTime = false;
        }
        else if (!strcmp("-v", coArgs[i].c_str())) {
            bLocalHeader = false;
        }
    }

    for (j = 0; j < coArgs.size(); j++) {
        if (!strcmp("-e", coArgs[j].c_str())) {
            if (++j >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer die Fehlerausgabe mit Option '-e'!\n"));
                EXIT(1);
            }
            COutput::SetTarget("stderr", new COutput(coArgs[j], true));
            //			g_hErr = fopen( coArgs[i].c_str(), "w" );
            if (!COutput::Target("stderr")->IsOkay()) {
                ERRMSG(0, ("FEHLER: Datei '%s' konnte nicht fuer die Ausgabe geoeffnet werden!\n", coArgs[j].c_str()));
                EXIT(1);
            }
            g_bError = true;
        }
    }

    if (bTime && bLocalHeader) {
        INFOMSG(("\n%s [Build %d],\n(C) Copyright 1999-2026 by S.Schuemann\n", VERSIONINFO, PBEMTOOLS_BUILD_NUMBER_EMU));
    }

    for (size_t aix = 0; aix < coArgs.size(); aix++) {
        g_sCmdOptions += std::string(" ") + coArgs[aix];
    }

    g_coFlags.insert(VF_SHOWVERBOSEINFO);
    bool explicitCommandLineSize = false;
    while (i < coArgs.size()) {
        if (!strcmp("-sb", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SORTBURGEN);
        }
        else if (!strcmp("-sp", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SORTPRIVAT);
        }
        else if (!strcmp("-st", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SORTTALENTE);
        }
        else if (!strcmp("-sk", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SORTKOMMANDO);
        }
        else if (!strcmp("-si", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SORTISLANDS);
        }
        else if (!strcmp("-cr", coArgs[i].c_str())) {
            g_coFlags.insert(VF_CROUTPUT);
        }
        else if (!strcmp("-t", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWTALENTE);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-td", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWTALENTE);
            g_coFlags.insert(VF_SHOWTDIFF);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-ts", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWEMULATEDDAYS);
        }
        else if (!strcmp("-d", coArgs[i].c_str())) {
            g_coFlags.insert(VF_DEBUGMODE);
        }
        else if (!strcmp("-f", coArgs[i].c_str())) {
            bForce = true;
        }
        else if (!strcmp("-fd", coArgs[i].c_str())) {
            CMetaCommand::ForceDeclares(true);
            g_coFlags.insert(VF_FORCEDECLARES);
        }
        else if (!strcmp("-fr", coArgs[i].c_str())) {
            CMessage::ForceRendering(true);
        }
        else if (!strcmp("--fullcom", coArgs[i].c_str())) {
            g_coFlags.insert(VF_FULLCOMMANDOUTPUT);
        }
        else if (!strcmp("--fullregs", coArgs[i].c_str())) {
            g_coFlags.insert(VF_RUNALLVISIBLEREGIONS);
        }
        else if (!strcmp("-g", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWGEGENSTAENDE);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-b", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWBESCHREIBUNG);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-hb", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWHANDEL);
        }
        else if (!strcmp("-k", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWMINIKARTE);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-kl", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWMINIKARTE);
            g_coFlags.insert(VF_SHOWLUXUS);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-klp", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWMINIKARTE);
            g_coFlags.insert(VF_SHOWLUXUS);
            g_coFlags.insert(VF_SHOWLPROD);
            g_coFlags.insert(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-kk", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWKOMPKARTE);
        }
        else if (!strcmp("--kkall", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWWORLDKARTE);
        }
        else if (!strcmp("-l", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWLASTEN);
        }
        else if (!strcmp("-m", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWMATPOOL);
        }
        else if (!strcmp("-more", coArgs[i].c_str())) {
            g_coFlags.insert(VF_PAGER);
        }
        else if (!strcmp("-mi", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Anzahl der Durchlaeufe fehlt fuer die Option '-mi'!\n"));
                EXIT(1);
            }
            g_nMinPasses = atoi(coArgs[i].c_str());
            if (g_nMinPasses < 0 || g_nMinPasses > 10) {
                ERRMSG(0, ("FEHLER: Falscher Wert fuer die Anzahl der Durchlaeufe (0-10)!\n"));
                EXIT(1);
            }
        }
        else if (!strcmp("-map", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer die Option '-map'!\n"));
                EXIT(1);
            }
            sMapOutName = coArgs[i];
        }
        else if (!strcmp("--mapr", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer die Option '--mapr'!\n"));
                EXIT(1);
            }
            sMapOutName = coArgs[i];
            g_coFlags.insert(VF_EXPORTWITHROUND);
        }
        else if (!strcmp("-n", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWMESSAGES);
        }
        else if (!strcmp("--no-battle-messages", coArgs[i].c_str())) {
            g_coFlags.insert(VF_NOBATTLEMESSAGES);
        }
        else if (!strcmp("-nrzv", coArgs[i].c_str())) {
            g_coFlags.erase(VF_SHOWVERBOSEINFO);
        }
        else if (!strcmp("-nv", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SUPPRESSTURNOUTPUT);
        }
        else if (!strcmp("-q", coArgs[i].c_str())) {
            bTime = false;
        }
        else if (!strcmp("-v", coArgs[i].c_str())) {
            TRACEMSG(("\n%s,\n(C) Copyright 1999-2019 by S.Schuemann\n", VERSIONINFO));
            exit(0);
        }
        else if (!strcmp("-p", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Parteinummer fuer die Option '-p'!\n"));
                EXIT(1);
            }
            sPlayer = coArgs[i];
            nPlayer = 0;
        }
        else if (!strcmp("-gr", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Gruppenname fuer die Option '-gr'!\n"));
                EXIT(1);
            }
            sGroupName = coArgs[i];
        }
        else if (!strcmp("-cfg", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Spielname fuer die Option '-cfg'!\n"));
                EXIT(1);
            }
            g_sSpiel = coArgs[i];
        }
        else if (!strcmp("--output-encoding", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Encoding fuer die Option '--output-encoding'!\n"));
                EXIT(1);
            }
            if (!CharacterMapper::IsSupported(coArgs[i])) {
                ERRMSG(0, ("FEHLER: Encoding '%s' wird nicht unterstuetzt!\n", coArgs[i].c_str()));
                EXIT(1);
            }
            COutput::SetEncoding(coArgs[i]);
        }
        else if (!strcmp("-et", coArgs[i].c_str())) {
            g_coFlags.insert(VF_TRACEONERROR);
        }
        else if (!strcmp("-pb", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWPRIVAT);
        }
        else if (!strcmp("-pi", coArgs[i].c_str())) {
            g_coFlags.insert(VF_PROGRESSINFO);
        }
        else if (!strcmp("-pm", coArgs[i].c_str())) {
            g_coFlags.insert(VF_PRIVATMETA);
        }
        else if (!strcmp("-u", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWUNITS);
        }
        else if (!strcmp("-uv", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWUNITSVERBOSE);
        }
        else if (!strcmp("-us", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SORTFOREIGN);
        }
        else if (!strcmp("-up", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWTRIBEOVERVIEW);
        }
        else if (!strcmp("-un", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SHOWUNITSNEW);
        }
        else if (!strcmp("-uq", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SUPPRESSUNITS);
        }
        else if (!strcmp("-rc", coArgs[i].c_str())) {
            g_coFlags.insert(VF_DONTKILLCOMMANDS);
        }
        else if (!strcmp("-wait", coArgs[i].c_str())) {
            g_bWait = true;
        }
        else if (!strcmp("-ws", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SUPPRESSKEYWARN);
        }
        else if (!strcmp("-wall", coArgs[i].c_str())) {
            g_coFlags.insert(VF_NOWARNINGS);
        }
        else if (!strcmp("-wfirst", coArgs[i].c_str())) {
            g_coFlags.insert(VF_SUPPRESSMULTIERRORS);
        }
        else if (!strcmp("--pedantic", coArgs[i].c_str())) {
            g_coFlags.insert(VF_DIAGPEDANTIC);
        }
        else if (!strcmp("--version2", coArgs[i].c_str())) {
            g_coFlags.insert(VF_VERSION2WARNING);
        }
        else if (!strcmp("--strip-duplicate-descr", coArgs[i].c_str())) {
            g_coFlags.insert(VF_STRIPDUPLICATEDESCR);
        }
        else if (!strcmp("--restricted", coArgs[i].c_str())) {
            g_coFlags.insert(VF_RESTRICTED);
        }
        else if (!strcmp("--fix-encodings", coArgs[i].c_str())) {
            g_coFlags.insert(VF_FIXENCODINGS);
        }
        else if (!strcmp("-w", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Zeilenlaenge fuer die Option '-w'!\n"));
                EXIT(1);
            }
            g_nLineSize = atoi(coArgs[i].c_str());
            if (g_nLineSize < 40) {
                ERRMSG(0, ("FEHLER: Zeilenlaenge darf nicht kleiner als 40 sein!\n"));
                EXIT(1);
            }
        }
        else if (!strcmp("-wc", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Zeilenlaenge fuer die Option '-wc'!\n"));
                EXIT(1);
            }
            g_nCommandLineSize = atoi(coArgs[i].c_str());
            explicitCommandLineSize = true;
            if (g_nCommandLineSize < 40) {
                ERRMSG(0, ("FEHLER: Zeilenlaenge darf nicht kleiner als 40 sein!\n"));
                EXIT(1);
            }
        }
        else if (!strcmp("-cl", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Groessenangabe fuer die Option '-cl'!\n"));
                EXIT(1);
            }
            g_nContainerLimit = atoi(coArgs[i].c_str());
            if (g_nLineSize < 16) {
                ERRMSG(0, ("FEHLER: Behaelterlimit darf nicht kleiner als 16 sein!\n"));
                EXIT(1);
            }
        }
        else if (!strcmp("--rseed", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Initialisierungswert fuer die Option '--rseed'!\n"));
                EXIT(1);
            }
            Random((int32_t)strtol(coArgs[i].c_str(), NULL, 0));
        }
        else if (!strcmp("--limit-runtime", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Zeit fuer die Option '--limit-runtime'!\n"));
                EXIT(1);
            }
            g_nLimitRuntime = (int32_t)strtol(coArgs[i].c_str(), NULL, 0);
        }
        else if (!strcmp("-e", coArgs[i].c_str())) {
            ++i;
            /*
                        if( ++i>=coArgs.size() )
                        {
                                ERRMSG( 0, ( "FEHLER: Kein Dateiname fuer die Fehlerausgabe mit Option '-e'!\n" ));
                                EXIT( 1 );
                        }
            COutput::SetTarget( "stderr", new COutput( coArgs[i], true ) );
//			g_hErr = fopen( coArgs[i].c_str(), "w" );
            if( !COutput::Target( "stderr" )->IsOkay() )
                        {
                                ERRMSG( 0, ( "FEHLER: Datei '%s' konnte nicht fuer die Ausgabe geoeffnet werden!\n", coArgs[i].c_str() ));
                                EXIT( 1 );
                        }
                        g_bError = true;
            */
        }
        else if (!strcmp("-do", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer die Debugausgabe mit Option '-do'!\n"));
                EXIT(1);
            }
            COutput::SetTarget("debug", new COutput(coArgs[i], true));
            //			g_hErr = fopen( coArgs[i].c_str(), "w" );
            if (!COutput::Target("debug")->IsOkay()) {
                ERRMSG(0, ("FEHLER: Datei '%s' konnte nicht fuer die Ausgabe geoeffnet werden!\n", coArgs[i].c_str()));
                EXIT(1);
            }
        }
        else if (!strcmp("-to", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer die Traceausgabe mit Option '-to'!\n"));
                EXIT(1);
            }
            COutput::SetTarget("trace", new COutput(coArgs[i], true));
            //			g_hErr = fopen( coArgs[i].c_str(), "w" );
            if (!COutput::Target("trace")->IsOkay()) {
                ERRMSG(0, ("FEHLER: Datei '%s' konnte nicht fuer die Ausgabe geoeffnet werden!\n", coArgs[i].c_str()));
                EXIT(1);
            }
        }
        else if (!strcmp("-pw", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Passwort fuer die Option '-pw'!\n"));
                EXIT(1);
            }
            CReport::SetDefaultPassword(coArgs[i].c_str());
        }
        else if (!strcmp("-o", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer die Ausgabe mit Option '-o'!\n"));
                EXIT(1);
            }

            sOName = coArgs[i];
        }
        else if (!strcmp("-ox", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Dateierweiterung fuer die Ausgabe mit Option '-ox'!\n"));
                EXIT(1);
            }
            sOXName = coArgs[i].c_str();
        }
        /*
                        else if( !strcmp( "-op", coArgs[i].c_str() ) )
                        {
                                if( ++i>=coArgs.size() )
                                {
                                        ERRMSG( 0, ( "FEHLER: Keine Dateierweiterung fuer die Ausgabe mit Option '-ox'!\n" ));
                                        EXIT( 1 );
                                }

                                sOPName = coArgs[i].c_str();
                        }
        */
        else if (!strcmp("-i", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer Import!\n"));
                EXIT(1);
            }
            coScripts.push_back(coArgs[i]);
        }
        else if (!strcmp("-xn", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer den Namensregeln!\n"));
                EXIT(1);
            }
            nStart = clock();
            if (bTime) {
                TRACEMSG(("lese '%s'...", coArgs[i].c_str()));
            }
            try {
                CRNENode::ReadRules(coArgs[i]);
            }
            catch (CRNEException e) {
                ERRMSG(0, ("FEHLER: Syntaxfehler in Namensregeln: %s\n", e.why().c_str()));
                EXIT(1);
            }
            if (bTime) {
                TRACEMSG((" (%1.2f sek.)\n", float(clock() - nStart) / CLOCKS_PER_SEC));
            }
        }
        else if (!strcmp("-insel", coArgs[i].c_str())) {
            int ic;
            //-kk -k -t -g -n -l -sb -st 491403.cr 491404.cr >test.out 568
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Kein Dateiname fuer den Inseltest!\n"));
                EXIT(1);
            }

            nStart = clock();
            if (bTime) {
                TRACEMSG(("lese '%s'...", coArgs[i].c_str()));
            }
            CReport oR(coArgs[i].c_str());
            if (bTime) {
                TRACEMSG((" (%1.2f sek.)\n", float(clock() - nStart) / CLOCKS_PER_SEC));
            }
            oR.Karte()->DumpFullMap("stdout", "");
            if (bTime) {
                TRACEMSG(("ermittle Inseln..."));
            }
            nStart = clock();
            ic = oR.Karte()->Islandize();
            if (bTime) {
                TRACEMSG((" %d gefunden. (%1.2f sek.)\n", ic, float(clock() - nStart) / CLOCKS_PER_SEC));
            }
            EXIT(1);
        }
        else if (!strcmp("-mr", coArgs[i].c_str())) {
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Keine Koordinatenoffsets fuer die Option '-mr'!\n"));
                EXIT(1);
            }
            nTX = atoi(coArgs[i].c_str());
            if (++i >= coArgs.size()) {
                ERRMSG(0, ("FEHLER: Nur ein Koordinatenoffset fuer die Option '-mr'!\n"));
                EXIT(1);
            }
            nTY = atoi(coArgs[i].c_str());
            CRegion::SetMoveOffset(nTX, nTY);
            if (i >= coArgs.size()) {
                EXIT(1);
            }
        }
        else if (*(coArgs[i].c_str()) == '-') {
            ERRMSG(0, ("FEHLER: Unbekannte Option '%s'!\n", coArgs[i].c_str()));
            EXIT(1);
        }
        else {
            break;
        }
        i++;
    }

    if (!explicitCommandLineSize) {
        g_nCommandLineSize = g_nLineSize;
    }

    if (i < coArgs.size()) {
        CReport* poRep1;
        CReport* poRep2;
        int nActRound = 0;
        int nPrevRound;
        CVorlage oV;
        CReport* pNewRep;
        std::string sName;
        std::string sSpiel;
        while (i < coArgs.size()) {
            while (!strcmp("-mr", coArgs[i].c_str())) {
                if (++i >= coArgs.size()) {
                    ERRMSG(0, ("FEHLER: Keine Koordinatenoffsets fuer die Option '-mr'!\n"));
                    EXIT(1);
                }
                nTX = atoi(coArgs[i].c_str());
                if (++i >= coArgs.size()) {
                    ERRMSG(0, ("FEHLER: Nur ein Koordinatenoffset fuer die Option '-mr'!\n"));
                    EXIT(1);
                }
                nTY = atoi(coArgs[i].c_str());
                CRegion::SetMoveOffset(nTX, nTY);
                if (++i >= coArgs.size()) {
                    EXIT(1);
                }
            }
            if (bTime) {
                TRACEMSG(("lese '%s'...", coArgs[i].c_str()));
            }
            sName = coArgs[i++].c_str();
            nStart = clock();
            pNewRep = new CReport(sName);
            if (sSpiel.empty() && pNewRep->HasSpiel()) {
                sSpiel = pNewRep->Spiel();
            }
            if (pNewRep->HasSpiel() && sSpiel != pNewRep->Spiel()) {
                ERRMSG(0, ("Warnung: Wiederspruechliche Spielangaben in den CRs!\n"));
            }
            if (pNewRep->IsValid()) {
                for (j = 0; j < cpoReports.size(); j++) {
                    if (pNewRep->Partei() == cpoReports[j]->Partei()) {
                        break;
                    }
                }
                if (j == cpoReports.size()) {
                    if (pNewRep) {
                        coPlayers.push_back(pNewRep->Partei());
                    }
                    if (bTime) {
                        TRACEMSG((" (%1.2f sek.), Partei %s.\n", float(clock() - nStart) / CLOCKS_PER_SEC, itoan(pNewRep->Partei(), pNewRep->PNrBase())));
                    }
                }
                else {
                    if (bTime) {
                        TRACEMSG((" (%1.2f sek.), wieder Partei %s.\n", float(clock() - nStart) / CLOCKS_PER_SEC, itoan(pNewRep->Partei(), pNewRep->PNrBase())));
                    }
                }
                cpoReports.push_back(pNewRep);
                pNewRep->Karte()->FillIslandQueue(cpoQueue);
            }
            else {
                TRACEMSG(("Fehler beim Einlesen!\n"));
                delete pNewRep;
            }
        }

        for (i = 0; i < cpoReports.size(); i++) {
            if (nActRound < cpoReports[i]->Runde()) {
                nActRound = cpoReports[i]->Runde();
            }
        }

        if (!nActRound || !coPlayers.size()) {
            ERRMSG(0, ("FEHLER: Kein Vorlagentauglicher Basisreport gefunden!\n"));
            EXIT(1);
        }

        if (!coScripts.empty()) {
            bool bOk;
            for (std::vector<std::string>::iterator si = coScripts.begin(); si != coScripts.end(); si++) {
                nStart = clock();
                if (bTime) {
                    TRACEMSG(("importiere '%s'...", (*si).c_str()));
                }
                bOk = g_oScriptBase.Import((*si));
                if (bTime) {
                    TRACEMSG((" (%1.2f sek.)\n", float(clock() - nStart) / CLOCKS_PER_SEC));
                }
                if (!bOk) {
                    EXIT(1);
                }
            }
        }

        if (bTime) {
            TRACEMSG(("Aktuelle Runde: %d\n", nActRound));
        }
        nStart = clock();
        if (bTime) {
            TRACEMSG(("Regions- und Einheitendatenbanken werden aufgebaut..."));
        }
        CVorlage::InitDBS(nActRound, cpoReports);

        if (bTime) {
            TRACEMSG((" (%1.2f sek.)\n", float(clock() - nStart) / CLOCKS_PER_SEC));
        }
        if (!cpoQueue.empty()) {
            if (bTime) {
                TRACEMSG(("werte %d Insel Tags aus...", cpoQueue.size()));
            }
            nStart = clock();

            CVorlage::Islandize(cpoQueue, g_coRDB);

            if (bTime) {
                TRACEMSG((" (%1.2f sek.)\n", float(clock() - nStart) / CLOCKS_PER_SEC));
            }
        }

        nStart = clock();
        if (bTime) {
            TRACEMSG(("erzeuge Zugvorlage(n)..."));
        }
        CRegion::SetCurrentRound(nActRound);

        bool bFoundActivePlayer = false;

        try {
            for (j = 0; j < coPlayers.size(); j++) {
                if (coPlayers[j] > 0) {
                    poRep1 = 0;
                    poRep2 = 0;
                    nPrevRound = 0;

                    for (i = 0; i < cpoReports.size(); i++) {
                        if (!poRep1 && cpoReports[i]->Partei() == coPlayers[j] && cpoReports[i]->Runde() == nActRound) {
                            poRep1 = cpoReports[i];
                        }
                        if (cpoReports[i]->Partei() == coPlayers[j] && cpoReports[i]->Runde() > nPrevRound && cpoReports[i]->Runde() < nActRound) {
                            nPrevRound = cpoReports[i]->Runde();
                            poRep2 = cpoReports[i];
                        }
                    }

                    if (!sPlayer.empty() && poRep1) {
                        nPlayer = (int32_t)strtol(sPlayer.c_str(), 0, poRep1->PNrBase());
                    }
                    if (poRep1 && (nPlayer < 0 || poRep1->Partei() == nPlayer)) {
                        CRegion::SetCurrentPlayer(nPlayer);
                        bFoundActivePlayer = true;
                        if (!sGroupName.empty()) {
                            g_nOnlyGroup = CReport::GetGroupIdByName(sGroupName);
                        }
                        else {
                            g_nOnlyGroup = -1;
                        }
                        if (sOName.empty()) {
                            if (OpenOXFile(poRep1->FileName(), sOXName, bForce)) {
                                oV.Vorlage(*poRep1, poRep2, bTime);
                                //							fclose( g_hOut ); g_hOut = 0;
                            }
                            else {
                                if (sOXName.empty()) {
                                    oV.Vorlage(*poRep1, poRep2, bTime);
                                }
                            }
                        }
                        else {
                            if (!sOName.empty()) {
                                std::string sOPName = sOName;
                                CRegExp::Replace(sOPName, "@p", ToString(poRep1->Partei()));
                                CRegExp::Replace(sOPName, "@P", itoan(poRep1->Partei(), poRep1->PNrBase()));
                                CRegExp::Replace(sOPName, "@r", ToString(poRep1->Runde()));
                                CRegExp::Replace(sOPName, "@j", poRep1->Jahr() < 10 ? (std::string("0") + ToString(poRep1->Jahr())) : ToString(poRep1->Jahr()));
                                CRegExp::Replace(sOPName, "@m", poRep1->Zeitalter() == 2 ? ToString(poRep1->Monat()) : (poRep1->Monat() < 10 ? (std::string("0") + ToString(poRep1->Monat())) : ToString(poRep1->Monat())));
                                CRegExp::Replace(sOPName, "@w", ToString(poRep1->Woche()));
                                COutput::SetTarget("vorlage", new COutput(sOPName));
                                if (!COutput::Target("vorlage")->IsOkay()) {
                                    ERRMSG(0, ("FEHLER: Datei '%s' konnte nicht fuer die Ausgabe geoeffnet werden!\n", sOPName.c_str()));
                                }
                                else {
                                    oV.Vorlage(*poRep1, poRep2, bTime);
                                }
                            }
                        }
                    }
                }
            }
        }
        catch (CTimeoutException& ex) {
            ERRMSG(0, ("FEHLER: Abbruch durch zu lange Skriptlaufzeit nach %d Sekunden!\n", ex._runtime));
        }

        if (bTime) {
            TRACEMSG(("(%1.2f sek.)\n", float(clock() - (unsigned)g_nTimeCorrection - nStart) / CLOCKS_PER_SEC));
        }
        if (!bFoundActivePlayer) {
            if (!sPlayer.empty()) {
                ERRMSG(0, ("FEHLER: Konnte keine Befehle fuer Partei '%s' finden, habe nichts ausgefuehrt.", sPlayer.c_str()));
            }
            else {
                ERRMSG(0, ("FEHLER: Konnte keine Befehle finden, habe nichts ausgefuehrt."));
            }
        }

        if (!sMapOutName.empty()) {
            nStart = clock();
            if (bTime) {
                TRACEMSG(("schreibe Karten-CR..."));
            }
            oV.WriteMap(sMapOutName);
            if (bTime) {
                TRACEMSG((" (%1.2f sek.)\n", float(clock() - nStart) / CLOCKS_PER_SEC));
            }
        }

        for (i = 0; i < cpoReports.size(); i++) {
            delete cpoReports[i];
        }

        if (bTime) {
            if (g_nTimeCorrection) {
                TRACEMSG(("Zeit ueber alles (%1.2f sek. + %1.2f sek. fuer Eingaben)\n", float(clock() - (unsigned)g_nTimeCorrection - nTStart) / CLOCKS_PER_SEC, float(g_nTimeCorrection) / CLOCKS_PER_SEC));
            }
            else {
                TRACEMSG(("Zeit ueber alles (%1.2f sek.)\n", float(clock() - nTStart) / CLOCKS_PER_SEC));
            }
        }

        if (g_nNumErrors && !g_nNumWarnings) {
            TRACEMSG(("Es trat%s %d Fehler auf!\n", g_nNumErrors == 1 ? "" : "en", g_nNumErrors));
        }
        else if (!g_nNumErrors && g_nNumWarnings) {
            TRACEMSG(("Es traten %d Warnungen auf!\n", g_nNumWarnings));
        }
        else if (g_nNumErrors && g_nNumWarnings) {
            TRACEMSG(("Es traten %d Fehler und %d Warnungen auf!\n", g_nNumErrors, g_nNumWarnings));
        }
    }
    else {
        COutput* pTarget = COutput::Target("stdout");
        if (g_bError) {
            pTarget = COutput::Target("stderr");
        }
        //                01234567890123456789012345678901234567890123456789012345678901234567890123456789
        pTarget->Printf("\nAufruf: VORLAGE [Optionen] [CR-Datei1] { [CR-Datei2] {...} } { [> Vorlagendatei] }\n\n");
        pTarget->Write("    -b        Beschreibungen der Einheiten mit in die Vorlage uebernehmen\n");
        pTarget->Write("    -cfg s    Gibt den Basisnamen der Konfigurationsdatei an\n");
        pTarget->Write("   --cfgpath p Pfad in dem die Konfig-Dateien gesucht werden\n");
        pTarget->Write("    -cr       Ausgabe der Metabefehlsauswertung als CR, statt in eine Vorlage\n");
        pTarget->Write("    -cl n     Limitierung der Behaeltergroesse auf n, Warnung wenn groesser\n");
        pTarget->Write("    -d        Debug-Mode aktivieren\n");
        pTarget->Write("    -do f     Die Debugausgaben von Vorlage erfolgen in die Datei f\n");
        pTarget->Write("    -e f      Die Fehlermeldungen von Vorlage erfolgen in die Datei f\n");
        pTarget->Write("    -et       Bei Skript-Fehlern in den Debugger springen\n");
        pTarget->Write("    -f        Forciert das Ueberschreiben bestehender Zugdateien\n");
        pTarget->Write("    -fd       Fordert das Deklarieren von Variablen vor erstem Gebrauch\n");
        pTarget->Write("    -fr       Erzwingt das Rendern der Messages durch Vorlage (nicht empfohlen)\n");
        pTarget->Write("   --fullcom  Unit-Output enthaelt auch die persistenten Kommentare\n");
        pTarget->Write("   --fullregs Alle Regionen des aktuellen CRs oder mit 'visibility' in OnRegion\n");
        pTarget->Write("              und EndRegion beruecksichtigen\n");
        pTarget->Write("   --fix-encodings\n");
        pTarget->Write("              Versucht, UTF-8 und Latin1-Mischungen in Latin1 zu konvertieren\n");
        pTarget->Write("    -g        Gegnstandsliste in Einheitenkommentar\n");
        pTarget->Write("    -gr n     Zugvorlage nur fuer Gruppe n erzeugen (kein Einfluss bei -cr)\n");
        pTarget->Write("    -hb       Parteihandelsbilanz zu Beginn der Vorlage\n");
        pTarget->Write("    -i f      Datei f als Skriptdatei einlesen; Die Option kann mehrfach\n");
        pTarget->Write("              verwendet werden\n");
        pTarget->Write("    -k        Minikarte der Nachbarregionen und Regionsinfos\n");
        pTarget->Write("    -kl       Wie -k, aber mit Luxusgutpreisen\n");
        pTarget->Write("    -klp      Wie -k, aber mit angebotenem Luxusgut\n");
        pTarget->Write("    -kk       Komplettkarte der Ausdehnung des Basis-Reports im Kopf der Vorlage\n");
        pTarget->Write("   --kkall    Komplettkarte ohne Beachtung des Basis-Reports, also alles\n");
        pTarget->Write("    -l        Gewichtsuebersicht in Einheitenkommentar\n");
        pTarget->Write("   --limit-runtime t\n");
        pTarget->Write("              Skriptlaufzeit auf t Sekunden beschraenken\n");
        pTarget->Write("    -m        Materialpool anzeigen\n");
        pTarget->Write("    -mi n     Multipass-Interpretation n-fach mit Durchlaufnummer in $PASSNUM\n");
        pTarget->Write("    -map f    Karten-CR exportieren\n");
        pTarget->Write("   --mapr f   Karten-CR incl. Runden-Infos exportieren\n");
        pTarget->Write("    -more     Bei Bildschirmausgaben nach jedem Bildschirm anhalten\n");
        pTarget->Write("    -mr dx dy Bei nachfolgenden CRs dx und dy zu den Koordinaten addieren\n");
        pTarget->Write("    -n        Regionsspezifische Nachrichten in Regionsinfo uebernehmen\n");
        pTarget->Write("   --no-battle-messages\n");
        pTarget->Write("              Unterdrueckt detailierte Kampf-Nachrichten in den Redionen\n");
        pTarget->Write("    -nrzv     Zugvorlage wie im Anhang des NR\n");
        pTarget->Write("    -nv       Ausgabe der Zugvorlage unterdruecken\n");
        pTarget->Write("    -o f      Die Ausgabe der Vorlage erfolgt nicht auf stdout, sondern\n");
        pTarget->Write("              in die Datei mit dem Namen f\n");
        pTarget->Write("    -ox x     Wie -o, aber es wird nur eine Dateierweiterung angegeben,\n");
        pTarget->Write("              die mit dem Dateinamen des Reports den neuen Namen ergibt.\n");
        pTarget->Write("   --output-encoding e\n");
        pTarget->Write("              Ermoeglicht die Angabe eines Zeichensatzes fuer die Ausagedateien\n");
        pTarget->Write("    -p n      Legt fest, fuer welchen Spieler die Vorlage erstellt wird,\n");
        pTarget->Write("              wenn CRs von verschiedenen Spielern uebergeben werden\n");
        pTarget->Write("    -pb       Zeigt BESCHREIBE-PRIVAT-Inhalte in der Vorlage an\n");
        pTarget->Write("   --pedantic Mehr Fehlermeldungen zur Fehlersuche in Skripten\n");
        pTarget->Write("    -pi       Fortschrittsanzeige bei der Abarbeitung der Einheiten\n");
        pTarget->Write("    -pm       Metabefehle in privaten Beschreibungen statt persistenten\n");
        pTarget->Write("              Kommentaren suchen\n");
        pTarget->Write("    -pw p     Passwort w fuer Zugvorlage, wenn keines in CR gefunden\n");
        pTarget->Write("    -q        Unterdrueckt die Ausgabe der Zeitmessungen\n");
        pTarget->Write("    -rc       Befehle aus CR nicht durch Metabefehl-Ergebnisse ueberschreiben\n");
        pTarget->Write("    -sb       Einheiten-Sortierung u. Gruppierung nach Bauwerken (auch Schiffe)\n");
        pTarget->Write("    -si       Sortierung der Regionen nach Inseln\n");
        pTarget->Write("    -sk       Komandofuehrende Einheiten falls Sortierung immer an den Anfang\n");
        pTarget->Write("    -sp       Einheiten-Sortierung nach Privat-Beschreibung\n");
        pTarget->Write("    -st       Einheiten-Sortierung nach Talenten\n");
        pTarget->Write("              Diese drei Sortierungen koennen kombiniert werden;\n");
        pTarget->Write("              Es wird dann innerhalb der Bauwerkegruppen nach\n");
        pTarget->Write("              Talenten sortiert\n");
        pTarget->Write("   --restricted\n");
        pTarget->Write("              Dateifunktionen und Serveruntaugliche Befehle werden blockiert\n");
        pTarget->Write("   --rseed n  Initialisiert den Zufallsgenerator mit dem angegebenen Wert\n");
        pTarget->Write("   --strip-duplicate-descr\n");
        pTarget->Write("              Doppelte Einheiten-Beschreibungen in einer Region abkuerzen\n");
        pTarget->Write("    -t        Talentliste in Einheitenkommentar\n");
        pTarget->Write("    -ts       Quantisierte Talentpunkte-Anzeige fuer aktuelle Eressea-Zuege\n");
        pTarget->Write("    -td       Talentliste in Einheitenkommentar mit Aenderungen wenn moeglich\n");
        pTarget->Write("    -u        Fremde Einheiten am Ende der Einheiten einer Region auflisten\n");
        pTarget->Write("    -uv       Wie -u, aber mit Angabe von Beschreibung und Guetern\n");
        pTarget->Write("    -up       Uebersicht ueber die Parteien in der Region einfuegen\n");
        pTarget->Write("    -us       Fremde Einheiten nicht am Ende auflisten sondern einsortieren\n");
        pTarget->Write("    -un       fremde Einheiten werden mit -u o. -uv nur angezeigt, wenn sie\n");
        pTarget->Write("              im vorreport nicht in derselben Region waren (oder die Region\n");
        pTarget->Write("              neu im Report ist)\n");
        pTarget->Write("    -uq       Die Ausgabe jeglicher Einheitenbloecke wird unterdrueckt.\n");
        pTarget->Write("    -v        Ausgabe der Versionsinfo von Vorlage und Ende\n");
        pTarget->Write("   --version2 Warnungen bei Features die ab V2.0 nicht unterstuetzt werden\n");
        pTarget->Write("    -w l      Nachrichten und Info-Zeilen auf l Zeichen umbrechen (default: 100)\n");
        pTarget->Write("    -wc l     Befehle auf l Zeichen umbrechen (default: -w l)\n");
        pTarget->Write("    -ws       Warnungen ueber unbekannte Feldkennungen unterdruecken\n");
        pTarget->Write("    -wall     Alle Warnungen unterdruecken\n");
        pTarget->Write("    -wait     Nach der Ausfuehrung auf Druck auf die Eingabetaste warten\n");
        pTarget->Write("    -wfirst   Warnungen und Fehlermeldungen nur beim ersten auftreten melden\n");
        pTarget->Write("    -xn f     Das Regelfile f fuer den Namensgenerator verwenden\n");
    }

    TRACEMSG(("\n"));

    EXIT(g_returnCode);

    return 0;
}
