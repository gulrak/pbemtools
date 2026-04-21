/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/EBase/Report.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:55:52 $
 * $Revision: 1.12 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: ERESSEA-Datenklassen inclusive CR-Parser
 *****************************************************************************
 *
 * $Log: Report.cpp,v $
 * Revision 1.12  2000/02/24 09:55:52  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.11  1999/11/28 17:38:10  S.Schuemann
 * - Mannigfaltige Änderungen für Vorlage V1.4 beta 9
 *
 * Revision 1.10  1999/11/17 08:58:14  S.Schuemann
 * - support für multiple CRs
 *
 * - vielfache Änderungen für Vorlage 1.4 beta 8
 *
 * Revision 1.9  1999/11/08 11:10:45  S.Schuemann
 * - verbessertes Luxusgut-Handling
 * - Korrektur der Kapazitaetsberechnung
 * - Neue Flags
 *
 * Revision 1.8  1999/11/03 10:21:37  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.7  1999/10/26 13:25:43  S.Schuemann
 * - Anpassungen fuer neue Object-Attribute und Vorlage 1.4 b 5
 *
 * Revision 1.6  1999/10/24 07:59:28  S.Schuemann
 * - Korrekturen fuer Troll-Kapazitaeten (Trolle ziehen Wagen)
 *
 * Revision 1.5  1999/10/20 02:22:32  S.Schuemann
 * - Anpassungen der Reportklassen fuer die Features von Vorlage 1.4 b 3
 *
 * Revision 1.4  1999/10/18 21:31:46  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.3  1999/09/29 08:02:56  S.Schuemann
 * - Implementation der Behandlung des Kapitels EINHEITSBOTSCHAFTEN
 *
 * Revision 1.2  1999/09/27 06:22:31  S.Schuemann
 * - CWorldDB heisst jetzt CReport;
 * - Die CR-Felder "Runde" und "Anzahl Personen" werden unterstützt;
 * - CReport::GetValue() fuer das Objekt REPORT implementiert;
 * - Fehler im Kampfstatus behoben;
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#include "Report.h"
#include <algorithm>
#include <cmath>
#include "Expression.h"
#include "ReportStream.h"
#include "Utility.h"
#include "hierarchy.h"
#include "regexp.h"

using namespace std;

MessagePool g_oMessagePool;

std::string g_sConfigFile;

static StringTable g_stringTable;

// CRegion::Einheiten g_cpoEinheiten;
int g_nMaxVersion = 0;
bool g_bIsRealBuildingType = true;

RegionDB g_coRDB;
RegionDB::iterator g_iRDB;
int32_t g_nRDBIndex = -1;

EinheitenDB g_coEDB;
EinheitenDB::iterator g_iEDB;
int32_t g_nEDBIndex = -1;

RegEinheitenDB g_coREDB;

RRegionDB g_coRRegionDB;
REinheitenDB g_coREinheitenDB;

CReport::Reports CReport::m_cpoReports;
CReport::ParteiInfos CReport::g_cpoParteiInfos;
CReport::Gruppen CReport::m_cpoGruppen;

typedef std::map<std::string, int> TAGMAP;

TAGMAP g_coTags;

class AdditionalTag
{
    typedef std::set<std::string> AdditionalTagSet;
    typedef std::map<std::string, AdditionalTagSet> AdditionalTags;

public:
    static void Init();
    static void Add(const std::string& sBlock, const std::string& sTag);
    static bool Check(const std::string& sBlock, const std::string& sTag);

private:
    static AdditionalTags m_coAdditionalTags;
};

AdditionalTag::AdditionalTags AdditionalTag::m_coAdditionalTags;

void AdditionalTag::Init()
{
    static bool bInit = false;
    if (!bInit) {
        CConfigFile oCF(g_sConfigFile);
        std::string sBlock;
        size_t i = 1;
        size_t j;
        while (true) {
            if (!oCF.FetchLine("CRTags", i++, false))
                break;
            sBlock = oCF.GetString(0);
            j = 1;
            while (oCF.IsString(j)) {
                Add(sBlock, oCF.GetString(j++));
            }
        }
    }
}

void AdditionalTag::Add(const std::string& sBlock, const std::string& sTag)
{
    m_coAdditionalTags[Flatten(sBlock)].insert(Flatten(sTag));
}

bool AdditionalTag::Check(const std::string& sBlock, const std::string& sTag)
{
    AdditionalTags::const_iterator ati = m_coAdditionalTags.find(Flatten(sBlock));
    if (ati != m_coAdditionalTags.end()) {
        return (*ati).second.find(Flatten(sTag)) != (*ati).second.end();
    }
    return false;
}

typedef std::map<std::string, CGegenstandsInfo> GEGENSTANDINFO;

GEGENSTANDINFO g_coGInfo;

CGegenstandsInfo& CGegenstandsInfo::Lookup(const std::string& sThing)
{
    static CGegenstandsInfo oUnbekannt;

    if (g_coGInfo.empty()) {
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1;
        while (true) {
            if (!oCF.FetchLine("Things", i++))
                break;

            g_coGInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Name"), Value(oCF.GetString(0)));
            g_coGInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Gewicht"), Value(oCF.GetReal(1)));
            if (oCF.GetReal(2) > 0)
                g_coGInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Kapazit\xE4t"), Value(oCF.GetReal(2)));
            if (oCF.GetString(3).empty())
                g_coGInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Plural"), Value(oCF.GetString(0)));
            else
                g_coGInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Plural"), Value(oCF.GetString(3)));
        }
    }

    map<string, CGegenstandsInfo>::iterator i = g_coGInfo.find(DeUmlaut(sThing));

    if (i == g_coGInfo.end()) {
        if (!oUnbekannt.NumValues())
            oUnbekannt.SetValue(std::string("Name"), Value(std::string("Unbekannt")));
        return oUnbekannt;
    }
    return (*i).second;
}

typedef map<string, CRasse> RASSENINFO;

RASSENINFO g_coRaceInfo;

CRasse& CRasse::Lookup(const std::string& sRace)
{
    static CRasse oUnbekannt;

    if (g_coRaceInfo.empty()) {
        std::vector<std::string> coRaces;
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1;

        if (oCF.FetchLine("Races", i++)) {
            std::string sTRace;
            size_t r = 0;
            while (!(sTRace = oCF.GetString(r++)).empty()) {
                coRaces.push_back(DeUmlaut(sTRace));
            }

            while (true) {
                if (!oCF.FetchLine("Races", i++))
                    break;

                for (r = 1; r < coRaces.size(); r++) {
                    if (std::fabs(oCF.GetReal(r)) > 0.00001 || (!oCF.GetString(r).empty() && isdigit(oCF.GetString(r)[0]))) {
                        if (std::fabs((double)oCF.GetLong(r) - oCF.GetReal(r)) > 0.00001)
                            g_coRaceInfo[coRaces[r]].SetValue(oCF.GetString(0), Value((double)oCF.GetReal(r)));
                        else
                            g_coRaceInfo[coRaces[r]].SetValue(oCF.GetString(0), Value(oCF.GetLong(r)));
                    }
                    else {
                        g_coRaceInfo[coRaces[r]].SetValue(oCF.GetString(0), Value(oCF.GetString(r)));
                    }
                }
            }
        }
    }

    map<string, CRasse>::iterator i = g_coRaceInfo.find(DeUmlaut(sRace));

    if (i == g_coRaceInfo.end()) {
        if (!oUnbekannt.NumValues())
            oUnbekannt.SetValue(std::string("Einzahl"), Value(std::string("Unbekannt")));
        return oUnbekannt;
    }
    return (*i).second;
}

typedef map<string, CBurgInfo> BURGINFO;

BURGINFO g_coBurgInfo;

CBurgInfo& CBurgInfo::Lookup(const std::string& sBurg)
{
    static CBurgInfo oUnbekannt;

    if (g_coBurgInfo.empty()) {
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1;
        while (true) {
            if (!oCF.FetchLine("Castles", i++))
                break;

            g_coBurgInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Name"), Value(oCF.GetString(0)));
            g_coBurgInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Groesse"), Value(oCF.GetLong(1)));
            g_coBurgInfo[DeUmlaut(oCF.GetString(0))].SetValue(std::string("Bonus"), Value(oCF.GetLong(2)));
        }
    }

    map<string, CBurgInfo>::iterator i = g_coBurgInfo.find(DeUmlaut(sBurg));

    if (i == g_coBurgInfo.end()) {
        if (!oUnbekannt.NumValues())
            oUnbekannt.SetValue(std::string("Name"), Value(std::string("Unbekannt")));
        return oUnbekannt;
    }
    return (*i).second;
}

CBurgInfo* CBurgInfo::Lookup(int32_t nSize)
{
    CBurgInfo* pBT = 0;
    int32_t nGroesse = 0;
    for (map<string, CBurgInfo>::iterator bi = g_coBurgInfo.begin(); bi != g_coBurgInfo.end(); ++bi) {
        int32_t groesse = bi->second.GetValue("Groesse").asLong();
        if (groesse && groesse < nSize && groesse > nGroesse)
            pBT = &(bi->second);
    }
    return pBT;
}

typedef map<string, CBlockBase::Ptr> BUILDINGINFO;

BUILDINGINFO g_coBuildingInfo;

CBlockBase::Ptr CBuildingInfo::Lookup(const std::string& sBurg)
{
    static CBlockBase::Ptr poUnbekannt(new CBuildingInfo());

    if (g_coBuildingInfo.empty()) {
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1, j;
        std::shared_ptr<CBlockBase> pBlock;
        std::string sTmp;

        while (true) {
            if (!oCF.FetchLine("Buildings", i++))
                break;
            // pBlock = new CBuildingInfo();
            g_coBuildingInfo[DeUmlaut(oCF.GetString(0))].reset(new CBuildingInfo());
            g_coBuildingInfo[DeUmlaut(oCF.GetString(0))]->SetValue(std::string("Name"), Value(oCF.GetString(0)));
            j = 1;

            sTmp = oCF.GetString(j);
            while (!sTmp.empty() && (isdigit(sTmp[0]) || sTmp[0] == '-' || sTmp[0] == '+')) {
                if (std::fabs((double)oCF.GetLong(j) - oCF.GetReal(j)) > 0.00001)
                    g_coBuildingInfo[DeUmlaut(oCF.GetString(0))]->SetValue(oCF.GetString(j + 1), Value((double)oCF.GetReal(j)));
                else
                    g_coBuildingInfo[DeUmlaut(oCF.GetString(0))]->SetValue(oCF.GetString(j + 1), Value(oCF.GetLong(j)));
                j += 2;
                sTmp = oCF.GetString(j);
            }
            while (!sTmp.empty() && !(isdigit(sTmp[0]) || sTmp[0] == '-' || sTmp[0] == '+')) {
                pBlock.reset(new CBlockBase("BuildingSubObject", oCF.GetString(j++)));
                g_coBuildingInfo[DeUmlaut(oCF.GetString(0))]->AddBlock(pBlock);
                sTmp = oCF.GetString(j);
                while (!sTmp.empty() && (isdigit(sTmp[0]) || sTmp[0] == '-' || sTmp[0] == '+')) {
                    if (std::fabs((double)oCF.GetLong(j) - oCF.GetReal(j)) > 0.00001)
                        pBlock->SetValue(oCF.GetString(j + 1), Value((double)oCF.GetReal(j)));
                    else
                        pBlock->SetValue(oCF.GetString(j + 1), Value(oCF.GetLong(j)));
                    j += 2;
                    sTmp = oCF.GetString(j);
                }
            }
            /*
                        while( !oCF.GetString( j ).empty() && !IsDigit( oCF.GetString( j )[0] ) )
                        {
                            pBlock.reset( new CBlockBase( "BuildingSubObject", oCF.GetString( j++ ) ) );
                            g_coBuildingInfo[DeUmlaut( oCF.GetString( 0 ) )]->AddBlock( pBlock );
                            while( oCF.GetLong( j ) )
                            {
                                pBlock->SetValue( oCF.GetString( j+1 ), Value( oCF.GetLong( j ) ) );
                                j += 2;
                            }
                        }
            */

#ifdef _DEBUG
//            g_coBuildingInfo[DeUmlaut( oCF.GetString( 0 ) )]->Dump();
#endif
        }
    }

    map<string, CBlockBase::Ptr>::iterator i = g_coBuildingInfo.find(DeUmlaut(sBurg));

    if (i == g_coBuildingInfo.end()) {
        if (((CBuildingInfo*)poUnbekannt.get())->NumValues())
            poUnbekannt->SetValue(std::string("Name"), Value(std::string("Unbekannt")));
        return poUnbekannt;
    }
    return (*i).second;
}

/*
CGegenstandsInfo g_coGegenstandsInfos[]=
{
        { "Stein",			"Steine",		60 },
        { "Eisen",			"Eisen",		 5 },
        { "Holz",			"Hoelzer",		 5 },
        { "Pferd",			"Pferde",		50 },
        { "Schwert",		"Schwerter",	 1 },
        { "Speer",			"Speere",		 1 },
        { "Armbrust",		"Armbrueste",	 1 },
        { "Bogen",			"Boegen",		 1 },
        { "Katapult",		"Katapulte",   120 },
        { "Kettenhemd",		"Kettenhemden",	 2 },
        { "Plattenpanzer",	"Plattenpanzer",4 },
        { "Wagen",			"Wagen",		40 },

        { "Oel",			"Oel",			 3 },
        { "Balsam",			"Balsam",		 2 },
        { "Gewuerz",		"Gewuerze",		 2 },
        { "Gew�rz",			"Gew�rze",		 2 },
        { "Juwel",			"Juwelen",		 1 },
        { "Myrrhe",			"Myrrhe",		 2 },
        { "Seide",			"Seide",		 3 },
        { "Weihrauch",		"Weihrauch",	 2 },

        { "Flachwurz",		"Flachwurz",	 0 },
        { "Wuerziger Wagemut", "Wuerziger Wagemut", 0 },
        { "Wuerzige Wagemut", "Wuerziger Wagemut", 0 },
        { "W�rziger Wagemut", "W�rziger Wagemut", 0 },
        { "W�rzige Wagemut", "W�rziger Wagemut", 0 },
        { "Eulenauge",		"Eulenaugen",	 0 },
        { "Gruener Spinnerich", "Gruene Spinneriche", 0 },
        { "Gr�ner Spinnerich", "Gr�ne Spinneriche", 0 },
        { "Blauer Baumringel", "Blaue Baumringel", 0 },
        { "Elfenlieb",		"Elfenlieb",	 0 },
        { "Gurgelkraut",	"Gurgelkraeuter", 0 },
        { "Knotiger Saugwurz","Knotige Saugwurze", 0 },
        { "Blasenmorchel",	"Blasenmorcheln", 0 },
        { "Wasserfinder",	"Wasserfinder", 0 },
        { "Kakteenschwitz",	"Kakteenschwitze", 0 },
        { "Sandfaeule",		"Sandfaeulen",   0 },
        { "Sandf�ule",		"Sandf�ulen",	 0 },
        { "Windbeutel",		"Windbeutel",	 0 },
        { "Fjordwuchs",		"Fjordwuchse",	 0 },
        { "Alraune",		"Alraunen",      0 },
        { "Steinbeisser",	"Steinbeisser", 0 },
        { "Spaltwachs",		"Spaltwachse",	 0 },
        { "Hoehlenglimm",	"Hoehlenglimme",0 },
        { "H�hlenglimm",	"H�hlenglimme",  0 },
        { "Eisblume",		"Eisblumen",	 0 },
        { "Weisser Wueterich","Weisse Wueteriche", 0 },
        { "Wei�er W�terich","Wei�e W�teriche", 0 },
        { "Weisser W�terich","Weisse W�teriche", 0 },
        { "Schneekristall",	"Schneekristalle", 0 },

        { "Siebenmeilentee","Siebenmeilentees",	0 },
        { "Goliathwasser",	"Goliathwasser", 0 },
        { "Wasser des Lebens","Wasser des Lebens", 0 },
        { "Schaffenstrunk",	"Schaffenstraenke", 0 },
        { "Scheusalsbier",	"Scheusalsbiere", 0 },
        { "Duft der Rose",	"Duefte der Rosen", 0 },
    { "Bauernblut",		"Bauernblut", 0 },

        { "Gehirnschmalz",	"Gehirnschmalz", 0 },
        { "Dumpfbackenbrot","Dumpfbackenbrote", 0 },
        { "Stahlpaste",		"Stahlpasten", 0 },
        { "Pferdeglueck",	"Pferdeglueck", 0 },
        { "Pferdegl�ck",	"Pferdegl�ck", 0 },
        { "Berserkerblut",	"Berserkerblut", 0 },
        { "Bauernlieb",		"Bauernlieb", 0 },
        { "Riesengras",		"Riesengraeser", 0 },
        { "Trank der Wahrheit", "Traenke der Wahrheit", 0 },
        { "Faulobstschnaps","Faulobstschnaepse", 0 },
        { "Elixier der Macht","Elixiere der Macht", 0 },
        { "Heiltrank",		"Heiltraenke", 0 },
        { "Amulett der Dunkelheit", "Amulette der Dunkelheit", 0 },
        { "Amulett des Todes","Amulette des Todes", 0 },
        { "Amulett der Heilung","Amulette der Heilung", 0 },
        { "Amulett des wahren Sehens","Amulette des wahren Sehens", 0 },
    { "Mantel der Unverletzlichkeit","Maentel der Unverletzlichkeit", 0 },
    { "Ring der Unsichtbarkeit", "Ringe der Unsichtbarkeit", 0 },
    { "Ring der Macht",	"Ringe der Macht", 0 },
        { "Runenschwert",	"Runenschwerter", 1 },	// stimmt das?
    { "Schildstein",	"Schildsteine", 0 },
    { "Zauberstab des Feuers","Zauberstaebe des Feuers", 0 },
    { "Zauberstab der Blitze","Zauberstaebe der Blitze", 0 },
    { "Zauberstab der Teleportation","Zauberstaebe der Teleportation", 0 },

        { "Drachenkopf",	"Drachenkoepfe", 5 },	// Gewicht unsicher
//	{ "Drachenblut",    "Drachenblut",   1 },
    { "Mallorn",		"Mallorn",		 5 },
    { "Laen",			"Laen",			 2 },
    { "Laenschild",		"Laenschilde",	 0 },
    { "Laenkettenhemd",	"Laenkettenhemden", 1 },
    { "Schild",			"Schilde",		 1 },
    { "Bihaender",		"Bihaender",	 2 },
    { "Bih�nder",		"Bih�nder",		 2 },
    { "Kriegsaxt",		"Kriegsaexte",	 1 },
    { "Elfenbogen",		"Elfenboegen",	 1 },
    { "Laenschwert",	"Laenschwerter", 1 },
    { "Hellebarde",		"Hellebarden",   2 },
    { "Lanze",			"Lanzen",		 2 },
        { 0,0,0 }
};
*/

enum TAGID {
    T_Alias,
    T_Anzahl,
    T_Anderepartei,
    T_Aura,
    T_Auramax,
    T_Baeume,
    T_Bauern,
    T_belagert,
    T_Beschr,
    T_Besitzer,
    T_Belagerer,
    T_bewacht,
    T_Burg,
    T_capacity,
    T_cargo,
    T_Default,
    T_Eisen,
    T_ejcOrdersConfirmed,
    T_folgt,
    T_Groesse,
    T_Gruppe,
    T_herb,
    T_hero,
    T_hp,
    T_Hunger,
    T_Insel,
    T_Kampfstatus,
    T_Kapazitaet,
    T_Kapitaen,
    T_Kueste,
    T_Ladung,
    T_Laen,
    T_Lohn,
    T_Mallorn,
    T_MaxLadung,
    T_Name,
    T_Partei,
    T_Parteitarnung,
    T_Pferde,
    T_Prozent,
    T_privat,
    T_Rekruten,
    T_Richtung,
    T_Runde,
    T_Schaden,
    T_Schiff,
    T_Silber,
    T_Schoesslinge,
    T_Steine,
    T_Strasse,
    T_Tarnung,
    T_temp,
    T_Terrain,
    T_Typ,
    T_typprefix,
    T_unaided,
    T_Unterh,
    T_Unterhalt,
    T_Verkleidung,
    T_Verorkt,
    T_Verraeter,
    T_visibility,
    T_wahrerTyp,
    T_weight,

    T_TNorden,
    T_TSueden,
    T_TOsten,
    T_TWesten,
    T_NNorden,
    T_NSueden,
    T_NOsten,
    T_NWesten,
    T_Parteiname,
    T_maxLuxus
};

#define ADDTAG(t) g_coTags.insert(TAGMAP::value_type(DeUmlaut(#t), T_##t))
#define ADDTAGSZ(n, t) g_coTags.insert(TAGMAP::value_type(n, T_##t))

static void InitTags()
{
    ADDTAG(Alias);
    ADDTAG(Anzahl);
    ADDTAG(Anderepartei);
    ADDTAG(Aura);
    ADDTAG(Auramax);
    ADDTAG(Baeume);
    ADDTAG(Bauern);
    ADDTAG(belagert);
    ADDTAG(Beschr);
    ADDTAG(Besitzer);
    ADDTAG(Belagerer);
    ADDTAG(bewacht);
    ADDTAG(Burg);
    ADDTAG(capacity);
    ADDTAG(cargo);
    ADDTAG(Default);
    ADDTAG(Eisen);
    ADDTAG(ejcOrdersConfirmed);
    ADDTAG(folgt);
    ADDTAG(Groesse);
    ADDTAG(Gruppe);
    ADDTAG(herb);
    ADDTAG(hero);
    ADDTAG(hp);
    ADDTAG(Hunger);
    ADDTAG(Insel);
    ADDTAG(Kampfstatus);
    ADDTAG(Kapazitaet);
    ADDTAG(Kapitaen);
    ADDTAG(Kueste);
    ADDTAG(Ladung);
    ADDTAG(Laen);
    ADDTAG(Lohn);
    ADDTAG(Mallorn);
    ADDTAG(Mallorn);
    ADDTAG(MaxLadung);
    ADDTAG(Name);
    ADDTAG(Partei);
    ADDTAG(Parteitarnung);
    ADDTAG(Pferde);
    ADDTAG(privat);
    ADDTAG(Prozent);
    ADDTAG(Rekruten);
    ADDTAG(Richtung);
    ADDTAG(Runde);
    ADDTAG(Schaden);
    ADDTAG(Schiff);
    ADDTAG(Silber);
    ADDTAG(Schoesslinge);
    ADDTAG(Steine);
    ADDTAG(Strasse);
    ADDTAG(Tarnung);
    ADDTAG(Terrain);
    ADDTAG(temp);
    ADDTAG(Typ);
    ADDTAG(typprefix);
    ADDTAG(unaided);
    ADDTAG(Unterh);
    ADDTAG(Unterhalt);
    ADDTAG(Verkleidung);
    ADDTAG(Verorkt);
    ADDTAG(Verraeter);
    ADDTAG(visibility);
    ADDTAG(wahrerTyp);
    ADDTAG(weight);

    ADDTAGSZ("B\xE4ume", Baeume);
    ADDTAGSZ(
        "Gr\xF6\xDF"
        "e",
        Groesse);
    ADDTAGSZ("Kapazit\xE4t", Kapazitaet);
    ADDTAGSZ("Kapit\xE4n", Kapitaen);
    ADDTAGSZ("K\xFCste", Kueste);
    ADDTAGSZ("Schoe\xDFlinge", Schoesslinge);
    ADDTAGSZ("Sch\xF6sslinge", Schoesslinge);
    ADDTAGSZ("Sch\xF6\xDFlinge", Schoesslinge);
    ADDTAGSZ(
        "Stra\xDF"
        "e",
        Strasse);
    ADDTAGSZ("Verr\xE4ter", Verraeter);

    ADDTAG(TNorden);
    ADDTAG(TSueden);
    ADDTAG(TOsten);
    ADDTAG(TWesten);
    ADDTAG(NNorden);
    ADDTAG(NSueden);
    ADDTAG(NOsten);
    ADDTAG(NWesten);
    ADDTAG(Parteiname);
    ADDTAG(maxLuxus);
}

/////////////////////////////////////////////////////////////////////
//.class: CBlockBase
/////////////////////////////////////////////////////////////////////

CBlockBase::Ptr CBlockBase::g_poConfigObjects;

CBlockBase::CBlockBase(const char* pcType, const std::string& sName, const Value& oKey1, const Value& oKey2, const Value& oKey3)
    : m_pcType(pcType)
    , m_nName(g_stringTable.insert(sName))
    , m_oKey1(oKey1)
    , m_oKey2(oKey2)
    , m_oKey3(oKey3)
{
    (void)m_pcType;
    //    TRACEMSG(( "C: %s\n", m_pcType ));
}

CBlockBase::CBlockBase(const char* pcType, const char* pcName)
    : m_pcType(pcType)
    , m_nName(g_stringTable.insert(pcName ? pcName : ""))
{
    //    TRACEMSG(( "C: %s\n", m_pcType ));
}

CBlockBase::CBlockBase(const char* pcType, CReportStream& oRS)
    : m_pcType(pcType)
{
    //    TRACEMSG(( "C: %s\n", m_pcType ));
    std::string sTag;
    m_nName = g_stringTable.insert(oRS.GetValue());
    if (oRS.GetNumDat() > 0) {
        m_oKey1 = Value(oRS.GetDat(0));
    }
    if (oRS.GetNumDat() > 1) {
        m_oKey2 = Value(oRS.GetDat(1));
    }
    if (oRS.GetNumDat() > 2) {
        m_oKey3 = Value(oRS.GetDat(2));
    }
    oRS.Next();
    while (!oRS.EOS()) {
        sTag = oRS.GetComment();
        if (sTag.empty())
            sTag = std::string("@") + ToString((int32_t)m_coNamedValues.size());

        if (oRS.GetType() == CReportStream::enBLOCK) {
            if (CHierarchy::IsChild(pcType, oRS.GetValue())) {
                LoadSubObject(oRS);
            }
            else {
                break;
            }
        }
        else if (!oRS.GetComment().empty()) {
            if (oRS.GetType() == CReportStream::enINTEGER) {
                if (oRS.GetNumDat())
                    SetValue(sTag, Value(oRS.GetDat(0)));
                for (int i = 1; i < oRS.GetNumDat(); i++) {
                    SetValue(oRS.GetComment() + ':' + ToString(i), Value(oRS.GetDat(i)));
                }
            }
            else if (oRS.GetType() == CReportStream::enSTRING) {
                SetValue(sTag, Value(oRS.GetValue()));
            }
        }
        oRS.Next();
    }
}

CBlockBase::~CBlockBase()
{
    //    TRACEMSG(( "D: %s\n", m_pcType ));
}

void CBlockBase::LoadSubObject(CReportStream& oRS)
{
    std::string sName = oRS.GetValue();
    CBlockBase::Ptr pBlk(new CBlockBase(sName.c_str(), oRS));
    if (pBlk.get()) {
        pBlk->m_poHInfo = CHierarchy::Lookup(sName);
    }
    AddBlock(pBlk);
}

std::string CBlockBase::ID() const
{
    return CalcID(m_nName, m_oKey1, m_oKey2, m_oKey3);
}

void CBlockBase::SetValue(const std::string& sName, const Value& oVal)
{
    m_coNamedValues[CStringDB::Str2SID(sName)] = oVal;
}

void CBlockBase::SetValue(CReportStream& oRS)
{
    if (oRS.GetComment().empty())
        return;
    switch (oRS.GetType()) {
        case CReportStream::enINTEGER: {
            SetValue(oRS.GetComment(), Value(oRS.GetDat(0)));
            for (int32_t i = 1; i < oRS.GetNumDat(); i++) {
                SetValue(oRS.GetComment() + ':' + ToString(i), Value(oRS.GetDat(i)));
            }
        } break;
        case CReportStream::enSTRING:
            SetValue(oRS.GetComment(), Value(oRS.GetValue()));
            break;
        default:
            break;
    }
}

Value CBlockBase::GetValue(int32_t nIdx, std::string& sName) const
{
    if (nIdx >= (int32_t)NumValues()) {
        sName = "";
        return Value();
    }
    else {
        int c = 0;
        NamedValues::const_iterator vi;
        for (vi = m_coNamedValues.begin(); c < nIdx && vi != m_coNamedValues.end(); vi++, c++)
            ;
        sName = CStringDB::SID2Str((*vi).first);
        return (*vi).second;
    }
}

Value CBlockBase::GetValue(const std::string& sName, const Value& oDefault) const
{
    CObjectPart oOP;
    oOP.label = sName;
    return GetValue((CBlockBase*)this, &oOP, oDefault);
    /*
        NamedValues::const_iterator i = m_coNamedValues.find( CStringDB::Str2SID( sName ) );
        if( i == m_coNamedValues.end() )
            return oDefault;
        return (*i).second;
    */
}

Value CBlockBase::GetValue(std::shared_ptr<CBlockBase> pBlk, CObjectPart* pOP, const Value& oDefault)
{
    return GetValue(pBlk.get(), pOP, oDefault);
}

Value CBlockBase::GetValue(CBlockBase* pBlk, CObjectPart* pOP, const Value& oDefault)
{
    if (pOP) {
        CBlockBase* pOrgBlk = pBlk;
        CBlockBase* pHBlk;
        Value oVal;
        CObjectPart* pCOP = pOP;
        bool bClassFound = false;
        while (pBlk /*&& pCOP->next*/) {
            if (pCOP->next && IsEqual(pCOP->next->label, "size")) {
                NamedSubblocks::iterator i = pBlk->m_coSubblocks.find(g_stringTable.insert(pCOP->label));
                if (i != pBlk->m_coSubblocks.end()) {
                    if ((*i).second.size() == 1 && !((*i).second.begin()->second->HasKeys())) {
                        return Value((int32_t)((*i).second.begin()->second->NumValues()));
                    }
                    else
                        return Value((int32_t)((*i).second.size()));
                }
                else {
                    return Value(0);
                }
            }
            switch (pCOP->index.size()) {
                case 0:
                    pHBlk = pBlk->GetBlock(pCOP->label, bClassFound).get();
                    break;
                case 1:
                    pHBlk = pBlk->GetBlock(pCOP->label, bClassFound, pCOP->index[0]).get();
                    if (pHBlk && !pHBlk->HasKeys() && !pCOP->next) {
                        return pHBlk->GetValue(std::string("@") + pCOP->index[0].asString());
                    }
                    break;
                case 2:
                    pHBlk = pBlk->GetBlock(pCOP->label, bClassFound, pCOP->index[0], pCOP->index[1]).get();
                    break;
                default:
                    pHBlk = pBlk->GetBlock(pCOP->label, bClassFound, pCOP->index[0], pCOP->index[1], pCOP->index[2]).get();
            }

            if (!pHBlk)
                break;

            pBlk = pHBlk;
            pCOP = pCOP->next;
            if (!pCOP)
                break;
        }
        if (pBlk) {
            if (pCOP) {
                NamedValues::const_iterator i;
                if (pCOP->index.size() && pCOP->index[0].asLong()) {
                    i = pBlk->m_coNamedValues.find(CStringDB::Str2SID(pCOP->label + ':' + ToString(pCOP->index[0].asLong())));
                }
                else {
                    i = pBlk->m_coNamedValues.find(CStringDB::Str2SID(pCOP->label));
                }
                if (i == pBlk->m_coNamedValues.end()) {
                    if (pCOP == pOP && g_poConfigObjects.get() == pOrgBlk && !bClassFound) {
                        Value oErr;
                        std::string sErr(std::string("Zugriff auf nicht existierendes Object '") + pOP->label + "'!");
                        oErr.error(sErr.c_str());
                        return oErr;
                    }
                    oVal = oDefault;
                }
                else
                    oVal = (*i).second;
            }
            else {
                oVal = (oDefault.asLong() == 0) ? Value(1) : oDefault;
            }
        }
        else {
            if (pCOP == pOP && g_poConfigObjects.get() == pOrgBlk && !bClassFound) {
                Value oErr;
                std::string sErr(std::string("Zugriff auf nicht existierendes Object '") + pOP->label + "'!");
                oErr.error(sErr.c_str());
                return oErr;
            }
            oVal = oDefault;
        }
        return oVal;
    }
    else {
        return oDefault;
    }
}

Value CBlockBase::GetValue(std::shared_ptr<CBlockBase> pBlk, const std::string& sOAS, const Value& oDefault)
{
    std::shared_ptr<CObjectPart> pOP = Expression::parseObjectAccess(sOAS);
    if (pOP.get()) {
        Value oVal(GetValue(pBlk, pOP.get(), oDefault));
        return oVal;
    }
    else {
        NamedValues::const_iterator i = pBlk->m_coNamedValues.find(CStringDB::Str2SID(sOAS));
        if (i == pBlk->m_coNamedValues.end())
            return oDefault;
        return (*i).second;
    }
}

Value CBlockBase::GetValue(CBlockBase* pBlk, const std::string& sOAS, const Value& oDefault)
{
    std::shared_ptr<CObjectPart> pOP = Expression::parseObjectAccess(sOAS);
    if (pOP.get()) {
        Value oVal(GetValue(pBlk, pOP.get(), oDefault));
        return oVal;
    }
    else {
        NamedValues::const_iterator i = pBlk->m_coNamedValues.find(CStringDB::Str2SID(sOAS));
        if (i == pBlk->m_coNamedValues.end())
            return oDefault;
        return (*i).second;
    }
}

Value CBlockBase::GetValue(CObjectPart* pOP, const Value& oDefault) const
{
    return GetValue((CBlockBase*)this, pOP, oDefault);
}

void CBlockBase::AddBlock(std::shared_ptr<CBlockBase> pBlock)
{
    NamedSubblocks::iterator i = m_coSubblocks.find(pBlock->m_nName);
    if (i == m_coSubblocks.end()) {
        BlockGroup bg;
        i = m_coSubblocks.insert(NamedSubblocks::value_type(pBlock->m_nName, bg)).first;
    }
    (*i).second[pBlock->ID()] = pBlock;
}

std::shared_ptr<CBlockBase> CBlockBase::GetBlock(const std::string& sName, bool& bClassFound, const Value& oKey1, const Value& oKey2, const Value& oKey3)
{
    int32_t id = g_stringTable.insert(sName);
    // std::clog << "CBlockBase::GetBlock(" << sName << ") : sid = " << id << std::endl;
    NamedSubblocks::iterator i = m_coSubblocks.find(id);
    if (i != m_coSubblocks.end()) {
        bClassFound = true;
        if ((*i).second.size() == 1 && !(*i).second.begin()->second->HasKeys()) {
            if (oKey1.getType() != VT_EMPTY)
                return (*i).second.begin()->second;
        }
        BlockGroup::iterator bi = (*i).second.find(CalcID(id, oKey1, oKey2, oKey3));
        if (bi != (*i).second.end()) {
            return (*bi).second;
        }
    }
    else {
        bClassFound = false;
    }
    return std::shared_ptr<CBlockBase>();
}

std::string CBlockBase::CalcID(const std::string& sName, const Value& oKey1, const Value& oKey2, const Value& oKey3)
{
    return CalcID(g_stringTable.insert(sName), oKey1, oKey2, oKey3);
}

std::string CBlockBase::CalcID(int nName, const Value& oKey1, const Value& oKey2, const Value& oKey3)
{
    return std::to_string(nName) + '/' + Flatten(oKey1.asString()) + '/' + Flatten(oKey2.asString()) + '/' + Flatten(oKey3.asString());
}

void CBlockBase::ReadConfigObjects(const std::string& sFile, const std::string& sCap, CFGOBJMODE enCOM, const std::vector<std::string>& coTitles)
{
    std::string sFileName = PathedFileName(sFile.empty() ? g_sConfigFile : sFile);
    bool bClassFound;
    if (!g_poConfigObjects.get()) {
        g_poConfigObjects.reset(new CBlockBase("CFGObjectBase", std::string("ObjBase")));
    }

    switch (enCOM) {
        case enTABH: {
            CBlockBase::Ptr pBlock;
            CConfigFile oCF(sFileName);
            size_t i = 1;
            while (true) {
                if (!oCF.FetchLine(sCap, i++))
                    break;

                pBlock = g_poConfigObjects->GetBlock(sCap, bClassFound, Value(DeUmlaut(oCF.GetString(0))));
                if (!pBlock.get()) {
                    pBlock.reset(new CBlockBase("ConfigObjectTabH", sCap, Value(DeUmlaut(oCF.GetString(0)))));
                    g_poConfigObjects->AddBlock(pBlock);
                }

                pBlock->SetValue(std::string("Name"), Value(oCF.GetString(0)));

                for (size_t j = 0; j < coTitles.size(); j++) {
                    if (oCF.IsString(j + 1))
                        pBlock->SetValue(coTitles[j], Value(oCF.GetString(j + 1)));
                    else if (std::fabs((double)oCF.GetLong(j + 1) - oCF.GetReal(j + 1)) > 0.00001)
                        pBlock->SetValue(coTitles[j], Value(oCF.GetReal(j + 1)));
                    else
                        pBlock->SetValue(coTitles[j], Value(oCF.GetLong(j + 1)));
                }
            }
        } break;
        case enTABV: {
            std::vector<std::string> coIdx;
            CConfigFile oCF(sFileName);
            size_t i = 1;

            if (oCF.FetchLine(sCap, i++)) {
                std::string sIdx;
                CBlockBase::Ptr pBlock;
                size_t r = 0;

                while (!(sIdx = oCF.GetString(r++)).empty()) {
                    if (!coIdx.empty()) {
                        pBlock = g_poConfigObjects->GetBlock(sCap, bClassFound, Value(DeUmlaut(sIdx)));
                        if (!pBlock.get()) {
                            pBlock = Ptr(new CBlockBase("ConfigObjectTabV", sCap, Value(DeUmlaut(sIdx))));
                            g_poConfigObjects->AddBlock(pBlock);
                        }
                        pBlock->SetValue("Name", Value(sIdx));
                    }
                    coIdx.push_back(DeUmlaut(sIdx));
                }

                while (true) {
                    if (!oCF.FetchLine(sCap, i++))
                        break;

                    for (r = 1; r < coIdx.size(); r++) {
                        pBlock = g_poConfigObjects->GetBlock(sCap, bClassFound, Value(coIdx[r]));

                        if (oCF.IsString(r))
                            pBlock->SetValue(oCF.GetString(0), Value(oCF.GetString(r)));
                        else if (std::fabs((double)oCF.GetLong(r) - oCF.GetReal(r)) > 0.00001)
                            pBlock->SetValue(oCF.GetString(0), Value((double)oCF.GetReal(r)));
                        else
                            pBlock->SetValue(oCF.GetString(0), Value(oCF.GetLong(r)));
                    }
                }
            }
        } break;
        case enNEST: {
            CConfigFile oCF(sFileName);
            CBlockBase::Ptr pBlock, pNBlock;
            std::string sTmp;
            size_t i = 1, j;

            while (true) {
                if (!oCF.FetchLine(sCap, i++))
                    break;
                // pBlock = new CBuildingInfo();
                pBlock = g_poConfigObjects->GetBlock(sCap, bClassFound, Value(DeUmlaut(oCF.GetString(0))));
                if (!pBlock.get()) {
                    pBlock.reset(new CBlockBase("ConfigObjectNest", sCap, Value(DeUmlaut(oCF.GetString(0)))));
                    g_poConfigObjects->AddBlock(pBlock);
                }
                pBlock->SetValue(std::string("Name"), Value(oCF.GetString(0)));

                j = 1;

                sTmp = oCF.GetString(j);
                while (!sTmp.empty() && (isdigit(sTmp[0]) || sTmp[0] == '-' || sTmp[0] == '+')) {
                    if (std::fabs((double)oCF.GetLong(j) - oCF.GetReal(j)) > 0.00001)
                        pBlock->SetValue(oCF.GetString(j + 1), Value((double)oCF.GetReal(j)));
                    else
                        pBlock->SetValue(oCF.GetString(j + 1), Value(oCF.GetLong(j)));
                    j += 2;
                    sTmp = oCF.GetString(j);
                }
                while (!sTmp.empty() && !(isdigit(sTmp[0]) || sTmp[0] == '-' || sTmp[0] == '+')) {
                    pNBlock = pBlock->GetBlock(sTmp, bClassFound);
                    if (!pNBlock.get()) {
                        pNBlock.reset(new CBlockBase("ConfigObjectNestSub", sTmp));
                        pBlock->AddBlock(pNBlock);
                    }
                    sTmp = oCF.GetString(++j);
                    while (!sTmp.empty() && (isdigit(sTmp[0]) || sTmp[0] == '-' || sTmp[0] == '+')) {
                        if (std::fabs((double)oCF.GetLong(j) - oCF.GetReal(j)) > 0.00001)
                            pNBlock->SetValue(oCF.GetString(j + 1), Value((double)oCF.GetReal(j)));
                        else
                            pNBlock->SetValue(oCF.GetString(j + 1), Value(oCF.GetLong(j)));
                        j += 2;
                        sTmp = oCF.GetString(j);
                    }
                }
            }
        } break;
    }
}

void CBlockBase::Dump(unsigned int lvl)
{
    char spc[16];
    snprintf(spc, sizeof(spc), "%%%ds", lvl * 2);
    fprintf(stderr, spc, "");
    fprintf(stderr, "<%s", g_stringTable.i2s(m_nName).c_str());
    if (m_oKey1.getType() != VT_EMPTY) {
        fprintf(stderr, " k1=%c%s%c", 34, m_oKey1.asString().c_str(), 34);
        if (m_oKey2.getType() != VT_EMPTY) {
            fprintf(stderr, " k2=%c%s%c", 34, m_oKey2.asString().c_str(), 34);
            if (m_oKey3.getType() != VT_EMPTY) {
                fprintf(stderr, " k3=%c%s%c", 34, m_oKey3.asString().c_str(), 34);
            }
        }
    }
    fprintf(stderr, ">\n");
    for (NamedValues::iterator ai = m_coNamedValues.begin(); ai != m_coNamedValues.end(); ai++) {
        fprintf(stderr, spc, "");
        fprintf(stderr, "  <%s>%s</%s>\n", CStringDB::SID2Str((*ai).first).c_str(), (*ai).second.asString().c_str(), CStringDB::SID2Str((*ai).first).c_str());
    }
    for (NamedSubblocks::iterator bi = m_coSubblocks.begin(); bi != m_coSubblocks.end(); bi++) {
        for (BlockGroup::iterator bgi = (*bi).second.begin(); bgi != (*bi).second.end(); bgi++) {
            (*bgi).second->Dump(lvl + 1);
        }
    }
    fprintf(stderr, spc, "");
    fprintf(stderr, "</%s>\n", g_stringTable.i2s(m_nName).c_str());
}

/////////////////////////////////////////////////////////////////////
//.class: CMessage
/////////////////////////////////////////////////////////////////////

std::map<int32_t, std::string>* CMessage::m_pcoMessageTypes = 0;
bool CMessage::m_bForceRender = false;
CMessage::RENDERER CMessage::m_enRenderer = CMessage::NONE;

void CMessage::registerMessage(int32_t round)
{
    g_oMessagePool[round].push_back(this);
}

CMessage::CMessage(int32_t round)
    : CBlockBase("CMessage", std::string("MESSAGE"))
{
    registerMessage(round);
}

CMessage::CMessage(const std::string& sRendered, int32_t round)
    : CBlockBase("CMessage", std::string("MESSAGE"))
{
    registerMessage(round);
    SetValue(std::string("rendered"), Value(sRendered));
}

CMessage::CMessage(CReportStream& oRS, int32_t round)
    : CBlockBase("CMessage", oRS)
{
    registerMessage(round);
}

void CMessage::GetCoords(std::string sTag, int32_t& x, int32_t& y, int32_t& z) const
{
    x = 0;
    y = 0;
    z = 0;
    if (GetValue(sTag, Value("")).getType() == VT_INT) {
        x = GetValue(sTag, Value(0)).asLong();
        y = GetValue(sTag + ":1", Value(0)).asLong();
        z = GetValue(sTag + ":2", Value(0)).asLong();
    }
    else {
        std::string sReg = GetValue(sTag, Value("")).asString();
        if (!sReg.empty()) {
            x = atoi(sReg.c_str());
            auto p = sReg.find(',');
            if (p != std::string::npos) {
                y = atoi(sReg.c_str() + p + 1);
                p = sReg.find(',', p + 1);
                if (p != std::string::npos) {
                    z = atoi(sReg.c_str() + p + 1);
                }
                else
                    z = 0;
            }
        }
    }
}

int32_t CMessage::Messages(int32_t round)
{
    MessagePool::const_iterator mpi = g_oMessagePool.find(round);
    if (mpi == g_oMessagePool.end())
        return 0;
    return (int32_t)mpi->second.size();
}

CMessage* CMessage::FindMessage(int32_t round, int32_t idx)
{
    MessagePool::const_iterator mpi = g_oMessagePool.find(round);
    if (mpi == g_oMessagePool.end())
        return 0;
    if (size_t(idx) >= mpi->second.size())
        return 0;
    return mpi->second[size_t(idx)];
}

std::string CMessage::Render(CReport* pRep) const
{
    std::string sMsg;

    if (!m_bForceRender)
        sMsg = GetValue("rendered", Value("")).asString();

    if (sMsg.empty()) {
        switch (m_enRenderer) {
            case ERESSEA1:
                return Eressea1_Render(pRep);
                break;
            case ERESSEA2:
                return Eressea2_Render(pRep);
                break;
            default:
                // TODO: Handle error
                break;
        }
    }

    return sMsg;
}

std::string CMessage::Eressea1_Render(CReport* pRep) const
{
    std::string sMsg;
    int32_t nID = GetValue("type").asLong();
    std::map<int32_t, std::string>::const_iterator i = m_pcoMessageTypes->find(nID);
    std::string sRule, sFunc, sVal;
    std::string::size_type p = 0;
    std::string::size_type e = 0;

    if (i == m_pcoMessageTypes->end()) {
        sMsg = GetValue("rendered", Value("")).asString();
        if (sMsg.empty())
            return std::string("(MSG: kein Messagetyp und kein 'rendered')");
        return sMsg;
    }
    sRule = (*i).second;

    while ((p = sRule.find_first_of('{', e)) != std::string::npos) {
        sMsg += sRule.substr(e, p - e);
        e = sRule.find_first_of('}', p);
        if (e == std::string::npos) {
            ERRMSG(0, ("Fehler: Syntaktischer Fehler in Message-Typ: %s", (*i).second.c_str()));
            break;
        }
        e++;
        sFunc = sRule.substr(p + 1, e - p - 2);
        sVal = sFunc;
        if (!sFunc.empty() && sFunc[0] == '$') {
            auto h = sFunc.find_last_of(' ');
            if (h != std::string::npos) {
                sVal = sFunc.substr(h + 1);
                sFunc.erase(h);
            }

            if (IsEqual(sFunc, "$travel")) {
                switch (GetValue(sVal).asLong()) {
                    case 1:
                        sVal = "reitet";
                        break;
                    case 2:
                        sVal = "wandert";
                        break;
                    default:
                        sVal = "reist";
                }
            }
            else if (IsEqual(sFunc, "$travelthru")) {
                if (!GetValue(sVal, Value("")).asString().empty())
                    sVal = std::string(" Dabei wurde ") + GetValue(sVal).asString() + " durchquert.";
                else
                    sVal = "";
            }
            else if (IsEqual(sFunc, "$of")) {
                if (GetValue("amount").asLong() != GetValue(sVal).asLong())
                    sVal = std::string("von ") + GetValue(sVal).asString() + " ";
                else
                    sVal = "";
                if (nID == 2097 || nID == 771334452) {
                    if (!sVal.empty())
                        sVal += ' ';
                    sVal += "Silber";
                }
            }
            else if (IsEqual(sFunc, "$earn")) {
                switch (GetValue(sVal).asLong()) {
                    case 0:
                        sVal = "Arbeit";
                        break;
                    case 1:
                        sVal = "Unterhaltung";
                        break;
                    case 2:
                        sVal = "Steuereinteiben";
                        break;
                    case 3:
                        sVal = "Handeln";
                        break;
                    case 4:
                        sVal = "Handelssteuern";
                        break;
                    default:
                        sVal =
                            "dunklen Gesch\xE4"
                            "ften";
                }
            }
        }
        else if (IsEqual(sVal, "unit") || IsEqual(sVal, "target")) {
            CEinheit* pU = pRep->SearchUnit(GetValue(sVal).asLong());
            if (pU)
                sVal = pU->Name() + " (" + itoan(GetValue(sVal).asLong(), pRep->ENrBase()) + ")";
            else
                sVal = std::string("Unbekannte Einheit (") + itoan(GetValue(sVal).asLong(), pRep->ENrBase()) + ")";
        }
        else if (IsEqual(sVal, "from") || IsEqual(sVal, "to")) {
            sVal = pRep->Parteiname(GetValue(sVal).asLong()).substr(1) + " (" + itoan(GetValue(sVal).asLong(), pRep->PNrBase()) + ")";
            ;
        }
        else if (IsEqual(sVal, "building")) {
            CBauwerk* pB = pRep->GetBuilding(GetValue(sVal).asLong());
            if (pB)
                sVal = pB->Name() + " (" + itoan(GetValue(sVal).asLong(), pRep->BNrBase()) + ")";
            else
                sVal = "unbekannt";
        }
        else if (IsEqual(sVal, "ship")) {
            CSchiff* pS = pRep->GetShip(GetValue(sVal).asLong());
            if (pS)
                sVal = pS->Name() + " (" + itoan(GetValue(sVal).asLong(), pRep->BNrBase()) + ")";
            else
                sVal = "unbekannt";
        }
        else if (IsEqual(sVal, "region") || IsEqual(sVal, "start") || IsEqual(sVal, "end")) {
            int32_t x, y, z;
            GetCoords(sVal, x, y, z);
            CRegion* pReg = pRep->Karte()->GetFromECords(x, y, z, true);
            if (pReg) {
                sVal = pReg->GetName() + " (" + ToString(x) + "," + ToString(y);
                if (z)
                    sVal += "," + ToString(z);
                sVal += ")";
            }
            else
                sVal = "Unbekante Region";
        }
        else
            sVal = GetValue(sVal).asString();

        sMsg += sVal;
    }
    sMsg += sRule.substr(e);

    return sMsg;
}

std::string CMessage::Eressea2_Render(CReport* pRep) const
{
    std::string sMsg;
    int32_t nID = GetValue("type").asLong();
    std::map<int32_t, std::string>::const_iterator i = m_pcoMessageTypes->find(nID);
    std::string sRule, sFunc, sVal;

    if (i == m_pcoMessageTypes->end()) {
        sMsg = GetValue("rendered", Value("")).asString();
        if (sMsg.empty())
            return std::string("(MSG: kein Messagetyp und kein 'rendered')");
        return sMsg;
    }
    sRule = (*i).second;

    std::string::const_iterator iRule(sRule.begin());
    return E2R_Parse(iRule, sRule.end());
}

std::string CMessage::E2R_Parse(std::string::const_iterator& iRule, const std::string::const_iterator& iEnd) const
{
    ArgumentList coArgs;
    std::string sErg;
    std::string sArg;

    if (iRule == iEnd) {
        return "";
    }
    else if (*iRule == '"') {
        iRule++;
        if (iRule != iEnd && *iRule != '"') {
            do {
                if (*iRule == ',') {
                    sArg += ',';
                    ++iRule;
                }
                sArg += E2R_Parse(iRule, iEnd);
            } while (iRule != iEnd && *iRule != '"');
        }
        if (iRule != iEnd && *iRule == '"')
            iRule++;
        return sArg;
    }
    else if (*iRule == '$') {
        std::string sID;
        bool ref = false;
        iRule++;
        if (iRule != iEnd && *iRule == '{') {
            iRule++;
            ref = true;
        }
        while (iRule != iEnd && IsAlNum(*iRule)) {
            sID += *iRule++;
        }
        if (ref && iRule != iEnd && *iRule == '}') {
            iRule++;
            return GetValue(sID, Value("")).asString();
        }
        if (iRule != iEnd && *iRule != '(') {
            Value oVal0 = GetValue(sID, Value(""));
            Value oVal1, oVal2;

            sArg = oVal0.asString();
            if (oVal0.getType() == VT_INT) {
                oVal1 = GetValue(sID + ":1", Value());
                oVal2 = GetValue(sID + ":2", Value(0));
                if (oVal1.getType() == VT_INT) {
                    sArg = std::string("REGION[") + ToString(oVal0.asLong()) + ',' + ToString(oVal1.asLong()) + ',' + ToString(oVal2.asLong()) + "]";
                }
            }
            return sArg;
        }
        std::string sParam;
        do {
            // Klammer oder Komma �berspringen
            sParam = "";
            do {
                iRule++;
                while (iRule != iEnd && *iRule == ' ')
                    ++iRule;
                sParam += E2R_Parse(iRule, iEnd);
                while (iRule != iEnd && *iRule == ' ')
                    ++iRule;
            } while (iRule != iEnd && *iRule == '.');
            coArgs.push_back(Value(sParam));
        } while (iRule != iEnd && *iRule == ',');
        if (iRule != iEnd && *iRule != ')') {
            ERRMSG(0, ("Fehler: Fehlende Klammer fuer Funktion '%s' in Renderregel!", std::string(std::string("EMR_") + sID).c_str()));
        }
        else {
            iRule++;
        }
        Value oVErg;
        if (sID == "if" && coArgs.size() == 2) {
            coArgs.push_back(Value(""));
        }
        if (!DoUserFunction(std::string("EMR_") + sID, coArgs, &oVErg)) {
            ERRMSG(0, ("Fehler: Benoetigte Render-Funktion '%s' nicht gefunden!", std::string(std::string("EMR_") + sID).c_str()));
            return std::string("$") + sID + "()";
        }
        return oVErg.asString();
    }
    else {
        while (iRule != iEnd && *iRule != '$' && *iRule != ')' && *iRule != ',' && *iRule != '"') {
            sErg += *iRule++;
        }
    }
    return sErg;
}

/*
std::map<int32_t,std::string> CMessage::m_coMessageTypes;

CMessage::CMessage( CReportStream& oRS )
{
        do
        {
        if( !oRS.GetComment().empty() )
        {
            if( oRS.GetType()==CReportStream::enINTEGER )
            {
                if( oRS.GetNumDat() )
                    m_coParams[oRS.GetComment()] = ToString((int32_t)oRS.GetDat(0));
            }
            else if( oRS.GetType()==CReportStream::enSTRING )
            {
                m_coParams[oRS.GetComment()] = oRS.GetValue();
            }
        }

                oRS.Next();
        }
        while( !oRS.EOS() && oRS.GetType()!=CReportStream::enBLOCK );
}

CMessage::~CMessage()
{
}

std::string CMessage::Render( int32_t nID ) const
{
    std::map<int32_t,std::string>::const_iterator i = m_coMessageTypes.find( nID );
    std::string sMsg,sRule;
    int p = 0;
    int e = 0;

    if( i == m_coMessageTypes.end() )
        return Element( "rendered" ) + "(kein Messagetyp)";
    sRule = (*i).second;

    while( (p=sRule.find_first_of( '{', e )) != std::string::npos )
    {
        e = sRule.find_first_of( '}', p );
        if( e == std::string::npos )
        {
            ERRMSG( 0, ( "Fehler: Syntaktischer Fehler in Message-Typ: %s", (*i).second.c_str() ));
            break;
        }
        sMsg += std::string( "[" ) + Element( sRule.substr( p+1, e-p-1 ) ) + std::string( "]" );
    }
    return sMsg;
}

std::string CMessage::Element( const std::string& sKey ) const
{
    std::map<std::string,std::string>::const_iterator i = m_coParams.find( sKey );
    if( i == m_coParams.end() )
        return "(unbekannt)";
    else
        return (*i).second;
}
*/

/////////////////////////////////////////////////////////////////////
//.class: CKarte
/////////////////////////////////////////////////////////////////////

CKarte::CKarte(CReport* poReport)
    : m_bMap(false)
    , m_nCX(0)
    , m_nCY(0)
    , m_nCursorX(0)
    , m_nCursorY(0)
    , m_nLeft(0)
    , m_nRight(0)
    , m_nTop(0)
    , m_nBottom(0)
    , m_poReport(poReport)
{
}

CKarte::CKarte(CReportStream& oRS, CReport* poReport)
    : m_bMap(false)
    , m_nCX(0)
    , m_nCY(0)
    , m_nCursorX(0)
    , m_nCursorY(0)
    , m_nLeft(0)
    , m_nRight(0)
    , m_nTop(0)
    , m_nBottom(0)
    , m_poReport(poReport)
{
    Import(oRS);
}

void CKarte::Import(CReportStream& oRS, int nRunde)
{
    int32_t nPos = 0;
    CRegion* pReg;
    do {
        if (oRS.GetType() == CReportStream::enBLOCK && (oRS.GetValue() == "REGION" || oRS.GetValue() == "DURCHREISEREGION" || oRS.GetValue() == "SPEZIALREGION")) {
            pReg = new CRegion(oRS, this, nRunde, nPos++);
            Set(pReg);
        }
        else
            oRS.Next();
    } while (!oRS.EOS());
}

void CKarte::Write(CReportStream& oRS)
{
    for (RegionMap::iterator i = m_cpoRegions.begin(); i != m_cpoRegions.end(); i++) {
        (*i).second->Write(oRS);
    }
}

CKarte::~CKarte()
{
    for (RegionMap::iterator i = m_cpoRegions.begin(); i != m_cpoRegions.end(); i++) {
        delete (*i).second;
    }
    m_cpoRegions.clear();
}

int CKarte::m_nWLeft;
int CKarte::m_nWRight;
int CKarte::m_nWTop;
int CKarte::m_nWBottom;
bool CKarte::m_bWMap = false;

void CKarte::Set(CRegion* poRegion)
{
    RegionMap::iterator i;
    CRegionKey oRKey = poRegion->GetKey();
    i = m_cpoRegions.find(oRKey);
    if (i != m_cpoRegions.end()) {
        if (poRegion->GetBlock() < (*i).second->GetBlock()) {
            delete (*i).second;
            (*i).second = poRegion;
            for (std::vector<CRegion*>::iterator vri = m_cpoVRegions.begin(); vri != m_cpoVRegions.end(); vri++) {
                if ((*vri)->GetKey() == oRKey) {
                    (*vri) = poRegion;
                    break;
                }
            }
        }
        else {
            delete poRegion;
            return;
        }
    }
    else {
        if (!m_bMap && (poRegion->GetBlock() < CRegion::enSPEZIALREGION || poRegion->GetBlock() == CRegion::enSCHEMEN)) {
            m_nLeft = poRegion->GetEX() + poRegion->GetEY() / 2;
            m_nRight = poRegion->GetEX() + poRegion->GetEY() / 2;
            m_nTop = poRegion->GetEY();
            m_nBottom = poRegion->GetEY();
            m_bMap = true;
        }
        if (!m_bWMap && (poRegion->GetBlock() < CRegion::enSPEZIALREGION || poRegion->GetBlock() == CRegion::enSCHEMEN)) {
            m_nWLeft = poRegion->GetEX() + poRegion->GetEY() / 2;
            m_nWRight = poRegion->GetEX() + poRegion->GetEY() / 2;
            m_nWTop = poRegion->GetEY();
            m_nWBottom = poRegion->GetEY();
            m_bWMap = true;
        }
        m_cpoRegions.insert(RegionMap::value_type(poRegion->GetKey(), poRegion));
        m_cpoVRegions.push_back(poRegion);
        if (poRegion->GetBlock() < CRegion::enSPEZIALREGION || poRegion->GetBlock() == CRegion::enSCHEMEN) {
            if (poRegion->GetEY() > m_nTop)
                m_nTop = poRegion->GetEY();
            if (poRegion->GetEY() < m_nBottom)
                m_nBottom = poRegion->GetEY();
            if (poRegion->GetEX() + poRegion->GetEY() / 2 < m_nLeft)
                m_nLeft = poRegion->GetEX() + poRegion->GetEY() / 2;
            if (poRegion->GetEX() + poRegion->GetEY() / 2 > m_nRight)
                m_nRight = poRegion->GetEX() + poRegion->GetEY() / 2;

            if (poRegion->GetEY() > m_nWTop)
                m_nWTop = poRegion->GetEY();
            if (poRegion->GetEY() < m_nWBottom)
                m_nWBottom = poRegion->GetEY();
            if (poRegion->GetEX() + poRegion->GetEY() / 2 < m_nWLeft)
                m_nWLeft = poRegion->GetEX() + poRegion->GetEY() / 2;
            if (poRegion->GetEX() + poRegion->GetEY() / 2 > m_nWRight)
                m_nWRight = poRegion->GetEX() + poRegion->GetEY() / 2;
        }
    }
    poRegion->SetMap(this);
}

CRegion* CKarte::GetFromECords(int32_t nX, int32_t nY, int32_t nZ, bool bDeep)
{
    static CRegion oUnknown(std::string(""));
    RegionMap::iterator i;
    i = m_cpoRegions.find(CRegion::CalcKey(nX, nY, nZ));
    if (i == m_cpoRegions.end()) {
        if (bDeep) {
            RegionDB::iterator i2;
            i2 = g_coRDB.find(CRegion::CalcKey(nX, nY, nZ));
            if (i2 != g_coRDB.end())
                return *((*i2).second.begin());
        }
        return &oUnknown;
    }
    return (*i).second;
}

CRegion* CKarte::GetFromDCords(int nX, int nY, int nZ, bool bDeep)
{
    static CRegion oUnknown(std::string(""));
    RegionMap::iterator i;
    i = m_cpoRegions.find(CalcMapKey(nX, nY, nZ));
    if (i == m_cpoRegions.end()) {
        if (bDeep) {
            RegionDB::iterator i2;
            i2 = g_coRDB.find(CalcMapKey(nX, nY, nZ));
            if (i2 != g_coRDB.end())
                return *((*i2).second.begin());
        }
        return &oUnknown;
    }
    return (*i).second;
}

CRegionKey CKarte::CalcMapKey(int nX, int nY, int nZ)
{
    return CRegion::CalcKey(nX - nY / 2 - (nY > 0 && (nY & 1)), nY, nZ);
}

CRegionKey CKarte::GetIsland(int32_t x, int32_t y, int32_t z)
{
    RegionDB::iterator ri = g_coRDB.find(CRegionKey(x, y, z));
    if (ri == g_coRDB.end())
        return CRegionKey(0x7fffffffl, 0x7fffffffl, 0x7fffffffl);
    else
        return (*(*ri).second.begin())->GetIsland();
}

int CKarte::Islandize()
{
    RegionDB::iterator i;
    set<CRegionKey> coInfo;
    bool bChg;
    CRegionKey nMin, t;
    int32_t x, y, z;

    do {
        // printf( "iterating islandizer... (%d)\n", ++ic );
        bChg = false;
        for (i = g_coRDB.begin(); i != g_coRDB.end(); i++) {
            if ((*(*i).second.begin())->IsLand()) {
                x = (*(*i).second.begin())->GetEX();
                y = (*(*i).second.begin())->GetEY();
                z = (*(*i).second.begin())->GetEZ();
                nMin = (*(*i).second.begin())->GetIsland();
                t = GetIsland(x - 1, y + 1, z);
                if (t < nMin)
                    nMin = t;
                t = GetIsland(x, y + 1, z);
                if (t < nMin)
                    nMin = t;
                t = GetIsland(x + 1, y, z);
                if (t < nMin)
                    nMin = t;
                t = GetIsland(x + 1, y - 1, z);
                if (t < nMin)
                    nMin = t;
                t = GetIsland(x, y - 1, z);
                if (t < nMin)
                    nMin = t;
                t = GetIsland(x - 1, y, z);
                if (t < nMin)
                    nMin = t;
                if ((*(*i).second.begin())->GetIsland() != nMin) {
                    (*(*i).second.begin())->SetIsland(nMin);
                    bChg = true;
                }
            }
        }
    } while (bChg);

    for (i = g_coRDB.begin(); i != g_coRDB.end(); i++) {
        // printf(" %8lx ", (*i).second->GetIsland() );
        coInfo.insert((*(*i).second.begin())->GetIsland());
    }
    /*
        do
            {
                    // printf( "iterating islandizer... (%d)\n", ++ic );
                    bChg = false;
                    for( i = m_cpoRegions.begin(); i != m_cpoRegions.end(); i++ )
                    {
                            if( (*i).second->IsLand() )
                            {
                                    nMin = (*i).second->GetIsland();
                                    t = (*i).second->GetNW()->GetIsland(); if( t<nMin ) nMin = t;
                                    t = (*i).second->GetNO()->GetIsland(); if( t<nMin ) nMin = t;
                                    t = (*i).second->GetO()->GetIsland();  if( t<nMin ) nMin = t;
                                    t = (*i).second->GetSO()->GetIsland(); if( t<nMin ) nMin = t;
                                    t = (*i).second->GetSW()->GetIsland(); if( t<nMin ) nMin = t;
                                    t = (*i).second->GetW()->GetIsland();  if( t<nMin ) nMin = t;
                                    if( (*i).second->GetIsland()!= nMin )
                                    {
                                            (*i).second->SetIsland( nMin );
                                            bChg = true;
                                    }
                            }
                    }
            }
            while( bChg );

            for( i = m_cpoRegions.begin(); i != m_cpoRegions.end(); i++ )
            {
                    // printf(" %8lx ", (*i).second->GetIsland() );
                    coInfo.insert( (*i).second->GetIsland() );
            }
    */
    return (int)coInfo.size() - 1;
}

#ifdef RUMBURAK
int CKarte::Islandize(RegionMap& oRDB)
{
    RegionMap::iterator i;
    set<CRegionKey> coInfo;
    bool bChg;
    int nMin, t;
    int ic = 0;

    do {
        // printf( "iterating islandizer... (%d)\n", ++ic );
        bChg = false;
        for (i = oRDB.begin(); i != oRDB.end(); i++) {
            if ((*i).second->IsLand()) {
                nMin = (*i).second->GetIsland();
                /*
                                                t = (*i).second->GetNW()->GetIsland(); if( t<nMin ) nMin = t;
                                                t = (*i).second->GetNO()->GetIsland(); if( t<nMin ) nMin = t;
                                                t = (*i).second->GetO()->GetIsland();  if( t<nMin ) nMin = t;
                                                t = (*i).second->GetSO()->GetIsland(); if( t<nMin ) nMin = t;
                                                t = (*i).second->GetSW()->GetIsland(); if( t<nMin ) nMin = t;
                                                t = (*i).second->GetW()->GetIsland();  if( t<nMin ) nMin = t;
                */
                if ((*i).second->GetIsland() != nMin) {
                    (*i).second->SetIsland(nMin);
                    bChg = true;
                }
            }
        }
    } while (bChg);

    for (i = m_cpoRegions.begin(); i != m_cpoRegions.end(); i++) {
        // printf(" %8lx ", (*i).second->GetIsland() );
        coInfo.insert((*i).second->GetIsland());
    }

    return (int)coInfo.size() - 1;
}
#endif

void CKarte::FillIslandQueue(IslandQueue& cpoQueue)
{
    RegionMap::iterator i;

    for (i = m_cpoRegions.begin(); i != m_cpoRegions.end(); i++) {
        // printf(" %8lx ", (*i).second->GetIsland() );
        if (!(*i).second->GetIslandName().empty())
            cpoQueue.push_back((*i).second);
    }
}

void CKarte::DumpMap(const std::string& sTarget, int nCX, int nCY, int nB, int nH, const char* pcPref)
{
    int h, x, y, z;
    int x1, x2, sx, sy;
    char fmt[16], fmt2[16];
    char c, o;
    int e;
    nB >>= 1;
    nH >>= 1;
    if (nCY & 1)
        nCX++;

    y = nCY + nH + 1;
    h = (y & 1) & !(nCY & 1);

    x1 = (nCX - nB + h - ((y > 0) ? y + 1 : y) / 2);
    x2 = (nCX + nB + h - ((y > 0) ? y + 1 : y) / 2);
    sx = (log10((double)abs(x1)) > log10((double)abs(x2))) ? (int)log10((double)abs(x1)) : (int)log10((double)abs(x2));
    sy = (log10((double)abs(y)) > log10((double)abs(nCY - nH))) ? (int)log10((double)abs(y)) : (int)log10((double)abs(nCY - nH));
    sy++;
    snprintf(fmt, sizeof(fmt), "%%+%dd ", sy + 1);
    snprintf(fmt2, sizeof(fmt2), "%%%ds ", sy + 1);

    if (pcPref)
        COutput::TPrintf(sTarget, "%s ", pcPref);
    COutput::TPrintf(sTarget, "    ");
    if ((y & 1) ^ (nCY & 1))
        COutput::TPrintf(sTarget, " ");
    e = ((y & 1) ^ (nCY & 1)) ? 0 : 1;
    for (x = nCX - nB + h; x <= nCX + nB + h + e; x++)
        COutput::TPrintf(sTarget, "%c ", (x - ((y > 0) ? y + 1 : y) / 2) < 0 ? '-' : ((x - ((y > 0) ? y + 1 : y) / 2) ? '+' : '|'));
    COutput::TPrintf(sTarget, "\n");

    for (z = (int)pow((double)10, (double)sx); z > 0; z /= 10) {
        if (pcPref)
            COutput::TPrintf(sTarget, "%s ", pcPref);
        COutput::TPrintf(sTarget, fmt2, "");
        if ((y & 1) ^ (nCY & 1))
            COutput::TPrintf(sTarget, " ");
        for (x = nCX - nB + h; x <= nCX + nB + h + e; x++)
            COutput::TPrintf(sTarget, "%c ", ((abs(x - ((y > 0) ? y + 1 : y) / 2) / z) % 10) + '0');
        COutput::TPrintf(sTarget, "\n");
    }

    for (--y; y >= nCY - nH; y--) {
        if (pcPref)
            COutput::TPrintf(sTarget, "%s ", pcPref);
        COutput::TPrintf(sTarget, fmt, y);
        if ((y & 1) ^ (nCY & 1))
            COutput::TPrintf(sTarget, " ");
        h = (y & 1) & !(nCY & 1);
        for (x = nCX - nB + h; x <= nCX + nB + h; x++) {
            c = GetFromDCords(x, y, 0, true)->GetRegionChar();
            o = ' ';  // GetFromDCords( x, y, 0, false )->IsOwnUnit()?'!':' ';
            if (c == '/' && (abs(x - ((y > 0) ? y + 1 : y) / 2) % 10))
                c = ' ';
            if (c == ' ' && !(y % 10))
                c = '-';
            COutput::TPrintf(sTarget, "%c%c", c, o);
        }
        if (!((y & 1) ^ (nCY & 1)))
            COutput::TPrintf(sTarget, " ");
        COutput::TPrintf(sTarget, fmt, y);
        COutput::TPrintf(sTarget, "\n");
    }

    h = (y & 1) & !(nCY & 1);
    x1 = (nCX - nB + h - ((y > 0) ? y + 1 : y) / 2);
    x2 = (nCX + nB + h - ((y > 0) ? y + 1 : y) / 2);
    sx = (log10((double)abs(x1)) > log10((double)abs(x2))) ? (int)log10((double)abs(x1)) : (int)log10((double)abs(x2));

    if (pcPref)
        COutput::TPrintf(sTarget, "%s ", pcPref);
    COutput::TPrintf(sTarget, "    ");
    if ((y & 1) ^ (nCY & 1))
        COutput::TPrintf(sTarget, " ");
    e = ((y & 1) ^ (nCY & 1)) ? 0 : 1;
    for (x = nCX - nB + h; x <= nCX + nB + h + e; x++)
        COutput::TPrintf(sTarget, "%c ", (x - ((y > 0) ? y + 1 : y) / 2) < 0 ? '-' : ((x - ((y > 0) ? y + 1 : y) / 2) ? '+' : '|'));
    COutput::TPrintf(sTarget, "\n");

    for (z = (int)pow((double)10, (double)sx); z > 0; z /= 10) {
        if (pcPref)
            COutput::TPrintf(sTarget, "%s ", pcPref);
        COutput::TPrintf(sTarget, fmt2, "");
        if ((y & 1) ^ (nCY & 1))
            COutput::TPrintf(sTarget, " ");
        for (x = nCX - nB + h; x <= nCX + nB + h + e; x++)
            COutput::TPrintf(sTarget, "%c ", ((abs(x - ((y > 0) ? y + 1 : y) / 2) / z) % 10) + '0');
        COutput::TPrintf(sTarget, "\n");
    }
}

void CKarte::DumpFullMap(const std::string& sTarget, const char* pcPref)
{
    int nCX, nCY, nB, nH;

    nCY = (m_nTop + m_nBottom) / 2;
    nCX = ((m_nLeft + (nCY - m_nBottom) / 2) + (m_nRight - (m_nTop - nCY) / 2)) / 2;
    nB = m_nRight - m_nLeft;  //(m_nRight-(m_nTop-nCY)/2) - (m_nLeft+(nCY-m_nBottom)/2);
    nH = m_nTop - m_nBottom;

    DumpMap(sTarget, nCX, nCY, nB + 5, nH + 1, pcPref);
}

void CKarte::DumpWorldMap(const std::string& sTarget, const char* pcPref)
{
    int nCX, nCY, nB, nH;

    nCY = (m_nWTop + m_nWBottom) / 2;
    nCX = ((m_nWLeft + (nCY - m_nWBottom) / 2) + (m_nWRight - (m_nWTop - nCY) / 2)) / 2;
    nB = m_nWRight - m_nWLeft;  //(m_nRight-(m_nTop-nCY)/2) - (m_nLeft+(nCY-m_nBottom)/2);
    nH = m_nWTop - m_nWBottom;

    DumpMap(sTarget, nCX, nCY, nB + 5, nH + 1, pcPref);
}

/////////////////////////////////////////////////////////////////////
//.class: CReport
/////////////////////////////////////////////////////////////////////

std::string CReport::m_sDefaultPassword;
int32_t CReport::m_nMaxRound = 0;

CReport::CReport(const std::string& sFName)
    : CBlockBase("CReport")
    , m_bIsValid(false)
    , m_bHasIslandTags(false)
    , m_bHasSpiel(false)
    , m_bUTF8(false)
    , m_poMap(0)
    , m_sOrgCRName(sFName)
    , m_nVersion(36)
    , m_sSpiel("Eressea")
    , m_sKonfiguration("Standard")
    , m_nENrBase(10)
    , m_nPNrBase(10)
    , m_nBNrBase(10)
    , m_nRunde(0)
    , m_nZeitalter(1)
    , m_nPartei(-1)
    , m_nRekrutierungskosten(0)
    , m_nPersonen(0)
    , m_nPunkte(-1)
    , m_nPunkteschnitt(-1)
    , m_nEinkommen(0)
    , m_nAusgaben(0)
    , m_nMsgEinkommen(0)
    , m_nMsgAusgaben(0)
{
    g_sConfigFile = GetConfigFileName();

    m_poMap = new CKarte(this);

    if (g_coTags.empty())
        InitTags();

    m_coParteien[0] = std::string("-Monster");
    m_coParteien[-1] = std::string("-parteigetarnt");

    if (!sFName.empty()) {
        Import(sFName);
    }

    m_cpoReports.push_back(this);
}

CReport::~CReport()
{
    m_cpoReports.remove(this);
    for (Handelspartner::iterator i = m_cpoHPartner.begin(); i != m_cpoHPartner.end(); i++) {
        delete (*i).second;
    }
    m_cpoHPartner.clear();
    delete m_poMap;
}

void CReport::Import(const std::string& sFName)
{
    CReportStream oRS(sFName);
    CPartei::Ptr pPartei;
    CGruppe::Ptr pGruppe;
    // int nLetztePartei = -1;
    // int nRunde = 0;
    int nParteiPhase = 0;
    int32_t nRepPartei = 0;
    int32_t nIgnoredPartei = 0;
    int32_t battle_x = 0, battle_y = 0, battle_z = 0;
    bool inBattle = false;
    bool bFirstRegion = true;
    int nBlocks = 0;

    oRS.Next();

    if (oRS.EOS())
        return;

    do {
        if (oRS.GetType() == CReportStream::enBLOCK) {
            if (nBlocks == 1)
                AdditionalTag::Init();
            nBlocks++;
            if (oRS.GetType() == CReportStream::enBLOCK && !IsEqual(oRS.GetValue(), "MESSAGE")) {
                inBattle = false;
            }
        }
        if (oRS.GetType() == CReportStream::enBLOCK && IsEqual(oRS.GetValue(), "VERSION")) {
            m_nVersion = oRS.GetDat(0);

            if (m_nVersion < 20 && g_nMaxVersion + 1 < m_nVersion) {
                m_sSpiel = "Empiria";
                m_bHasSpiel = true;
            }

            if (m_nVersion >= 29)
                g_coFlags.insert(VF_HEXMAP);

            if (m_nVersion >= 32) {
                g_coFlags.insert(VF_BASE36);
                m_nENrBase = 36;
            }

            if (m_nVersion >= 49) {
                g_coFlags.insert(VF_FULLBASE36);
                m_nENrBase = 36;
                m_nPNrBase = 36;
                m_nBNrBase = 36;
            }

            if (m_nVersion >= 57) {
                g_coFlags.insert(VF_NEWERESSEASTATI);
            }

            if (m_nVersion < 50 && g_nMaxVersion + 1 < m_nVersion)
                g_bIsRealBuildingType = false;

            if (m_nVersion >= 59)
                g_coFlags.insert(VF_RESOURCEBLOCKS);

            if (m_nVersion > g_nMaxVersion)
                g_nMaxVersion = m_nVersion;

            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "charset")) {
            std::string enc = oRS.GetValue();
            if (IsEqual(enc, "utf-8") || IsEqual(enc, "utf8")) {
                m_bUTF8 = true;
                oRS.Utf8Mode(true);
            }
            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Spiel")) {
            std::string::size_type p;
            m_sSpiel = oRS.GetValue();
            m_bHasSpiel = true;
            if (!IsEqual(m_sSpiel, "eressea")) {
                g_coFlags.erase(VF_FULLBASE36);
                m_nPNrBase = 10;
                m_nBNrBase = 10;
            }
            p = g_sConfigFile.find_last_of("\\/:");
            if (p == std::string::npos)
                p = 0;

            std::string sSpiel = m_sSpiel;
            for (auto& c : sSpiel)
                c = (char)tolower(c);

#ifdef _WIN32
            g_sConfigFile = g_sConfigFile.substr(0, p + 1) + Flatten(sSpiel) + std::string(".cfg");
#else
            g_sConfigFile = g_sConfigFile.substr(0, p + 1) + std::string(".") + Flatten(sSpiel) + std::string("rc");
#endif
            SetConfigFileName(g_sConfigFile);
            AdditionalTag::Init();
            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "noskillpoints")) {
            if (oRS.GetDat(0) && !IsFlag(VF_SHOWEMULATEDDAYS))
                g_coFlags.insert(VF_NOSKILLPOINTS);
            SetValue(oRS);
            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Konfiguration")) {
            m_sKonfiguration = oRS.GetValue();
            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Basis")) {
            m_nENrBase = oRS.GetDat(0);
            if (oRS.GetDat(0) != 36)
                g_coFlags.erase(VF_BASE36);
            m_nENrBase = oRS.GetDat(0);
            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Koordinaten")) {
            if (!IsEqual(oRS.GetValue().c_str(), "Hex"))
                g_coFlags.erase(VF_HEXMAP);
            oRS.Next();
        }
        else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Runde")) {
            m_nRunde = oRS.GetDat(0);
            oRS.Next();
            if (m_nMaxRound < m_nRunde)
                m_nMaxRound = m_nRunde;
            if (m_nRunde >= 208 && IsEqual(m_sSpiel, "eressea")) {
                g_coFlags.insert(VF_FULLBASE36);
                m_nENrBase = 36;
                m_nPNrBase = 36;
                m_nBNrBase = 36;
            }
        }
        else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Zeitalter")) {
            m_nZeitalter = oRS.GetDat(0);
            oRS.Next();
        }
        //		else if( oRS.GetType()==CReportStream::enBLOCK && IsEqual( oRS.GetValue(), "PARTEI" ) && m_nPartei<0 )
        //		{
        //			m_nPartei = oRS.GetDat(0); oRS.Next();
        //		}
        else if (oRS.GetType() == CReportStream::enBLOCK && IsEqual(oRS.GetValue(), "OPTIONEN")) {
            oRS.Next();
            while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK)) {
                if (oRS.GetType() == CReportStream::enINTEGER && oRS.GetDat(0)) {
                    m_csOptionen.push_back(std::string("+") + oRS.GetComment());
                }
                else {
                    m_csOptionen.push_back(std::string("-") + oRS.GetComment());
                }
                oRS.Next();
            }
        }
        else if (oRS.GetType() == CReportStream::enBLOCK &&
                 (oRS.GetValue() == "MELDUNGEN" || oRS.GetValue() == "EREIGNISSE" || oRS.GetValue() == "EINKOMMEN" || oRS.GetValue() == "HANDEL" || oRS.GetValue() == "PRODUKTION" || oRS.GetValue() == "BEWEGUNGEN")) {
            std::string sMsg;
            int enMT = MT_UNKNOWN;

            if (oRS.GetValue() == "EREIGNISSE")
                enMT = MT_EREIGNISSE;

            if (oRS.GetValue() == "HANDEL")
                enMT = MT_HANDEL;

            if (oRS.GetValue() == "EINKOMMEN")
                enMT = MT_EINKOMMEN;

            oRS.Next();
            while (!oRS.EOS()) {
                if (oRS.GetType() == CReportStream::enSTRING) {
                    sMsg = oRS.GetValue();
                    if (!sMsg.empty() && sMsg[sMsg.size() - 1] != '.' && sMsg[sMsg.size() - 1] != '!') {
                        oRS.Next();
                        if (oRS.GetType() == CReportStream::enSTRING) {
                            sMsg += ' ';
                            sMsg += oRS.GetValue();
                        }
                    }
                    m_csNachrichten.push_back(Nachricht(enMT, sMsg));
                    if (oRS.GetType() != CReportStream::enSTRING)
                        break;
                }
                else
                    break;
                oRS.Next();
            }
        }
        else if (oRS.GetType() == CReportStream::enBLOCK && oRS.GetValue() == "MESSAGE") {
            std::string sMsg;
            bool bRegion = false;
            int nType = 0;
            int nFrom = 0;
            // int nTo = 0;
            int nAmount = 0;
            int32_t x = 0, y = 0, z = 0;
            int32_t nUnit = -1;
            int nBuilding = -1;
            bool bDrop = false;
            CMessage::Ptr pMsg(new CMessage(oRS, m_nRunde));
            if (inBattle && pMsg->GetValue("region", Value("")).asString().empty()) {
                pMsg->SetValue("region", Value(battle_x));
                pMsg->SetValue("region:1", Value(battle_y));
                pMsg->SetValue("region:2", Value(battle_z));
            }
            if (m_nRunde < 227) {
                switch (pMsg->GetValue("type").asLong()) {
                    case 9386:
                        nFrom = pMsg->GetValue("from").asLong();
                        // nTo     = pMsg->GetValue( "to" ).asLong();
                        nAmount = pMsg->GetValue("amount").asLong();
                        break;
                    case 2097:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = pMsg->GetValue("amount").asLong();
                        break;
                    case 24543:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = -pMsg->GetValue("cost").asLong();
                        break;
                    case 364:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = -pMsg->GetValue("money").asLong();
                }
            }
            else if (m_nRunde == 227) {
                switch (pMsg->GetValue("type").asLong()) {
                    case 1682429624:
                        nFrom = pMsg->GetValue("from").asLong();
                        // nTo     = pMsg->GetValue( "to" ).asLong();
                        nAmount = pMsg->GetValue("amount").asLong();
                        break;
                    case -1376149197:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = pMsg->GetValue("amount").asLong();
                        break;
                    case 443066738:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = -pMsg->GetValue("cost").asLong();
                        break;
                    case 170076:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = -pMsg->GetValue("money").asLong();
                }
            }
            else if (m_nRunde >= 227) {
                switch (pMsg->GetValue("type").asLong()) {
                    case 1682429624L:
                        nFrom = pMsg->GetValue("from").asLong();
                        // nTo     = pMsg->GetValue( "to" ).asLong();
                        nAmount = pMsg->GetValue("amount").asLong();
                        break;
                    case 771334452L:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = pMsg->GetValue("amount").asLong();
                        break;
                    case 443066738L:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = -pMsg->GetValue("cost").asLong();
                        break;
                    case 170076L:
                        nFrom = -1;  // nTo = Partei();
                        nAmount = -pMsg->GetValue("money").asLong();
                }
            }

            nUnit = pMsg->GetValue("unit").asLong();
            nBuilding = pMsg->GetValue("building").asLong();

            Value oVRegion = pMsg->GetValue("region", Value());
            if (oVRegion.getType() == VT_STRING) {
                std::string sReg = pMsg->GetValue("region").asString();
                if (!sReg.empty()) {
                    x = atoi(sReg.c_str());
                    auto p = sReg.find(',');
                    if (p != std::string::npos) {
                        y = atoi(sReg.c_str() + p + 1);
                        p = sReg.find(',', p + 1);
                        if (p != std::string::npos) {
                            z = atoi(sReg.c_str() + p + 1);
                        }
                        else
                            z = 0;
                        bRegion = true;
                    }
                }
            }
            else if (oVRegion.getType() == VT_INT) {
                x = oVRegion.asLong();
                y = pMsg->GetValue("region:1").asLong();
                z = pMsg->GetValue("region:2").asLong();
                bRegion = true;
            }
            m_cpoMessages.insert(std::make_pair(m_cpoMessages.size(), pMsg));
            if (!bDrop && (nAmount || (nBuilding > 0 && ((m_nRunde < 227 && nType == 7835) || (m_nRunde == 227 && nType == -1376149197L) || (m_nRunde > 227 && nType == 761324692L))))) {
                if (nFrom == Partei())
                    nAmount = -nAmount;
                if (nAmount < 0) {
                    m_nMsgAusgaben -= nAmount;
                }
                else {
                    m_nMsgEinkommen += nAmount;
                }
                if (!bRegion)
                    z = -1;
                m_coTradeInfos.push_back(CTradeInfo(nUnit, nBuilding, x, y, z, nAmount));
            }
        }
        else if (oRS.GetType() == CReportStream::enBLOCK && (oRS.GetValue() == "ADRESSEN" || oRS.GetValue() == "ALLIIERTE")) {
            std::string sParteiname;
            int32_t nP = -1;
            const char* ac = "-";

            CPartei::Ptr pP;

            if (oRS.GetValue() == "ALLIIERTE") {
                ac = "+";
            }

            oRS.Next();
            while (!oRS.EOS() && oRS.GetType() != CReportStream::enBLOCK) {
                if (pP.get())
                    pP->SetValue(oRS);

                if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Partei")) {
                    ParteiInfos::iterator pi = m_cpoLocalParteiInfos.find(oRS.GetDat(0));
                    if (pi == m_cpoLocalParteiInfos.end()) {
                        pP.reset(new CPartei(oRS.GetDat(0)));
                        pP->SetValue("Nummer", Value((int32_t)oRS.GetDat(0)));
                        m_cpoLocalParteiInfos.insert(ParteiInfos::value_type(oRS.GetDat(0), pP));
                    }
                    else
                        pP = (*pi).second;

                    nP = oRS.GetDat(0);
                    oRS.Next();
                }
                else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Parteiname")) {
                    sParteiname = oRS.GetValue();
                    oRS.Next();
                }
                else {
                    oRS.Next();
                }
                if (nP >= 0 && !sParteiname.empty()) {
                    if (m_coParteien[nP].empty() || m_coParteien[nP][0] == '-')
                        m_coParteien[nP] = std::string(ac) + sParteiname;
                    nP = -1;
                    sParteiname = "";
                }
            }
        }
        else if (oRS.GetType() == CReportStream::enBLOCK && (oRS.GetValue() == "ALLIANZ" || oRS.GetValue() == "PARTEI")) {
            // std::string sPass;
            // std::string sPName;
            int32_t nRekrutierungskosten = 0;
            int32_t nPersonen = 0;
            int32_t nPunkte = 0;
            int32_t nPunkteschnitt = 0;
            int32_t nRunde = m_nRunde;
            int32_t nPNum = 0;
            std::string sParteiname;
            std::string sPasswort;
            int32_t nP = -1;
            const char* ac = "-";
            bool bPartei = false;
            CPartei::Ptr pP;

            ParteiInfos::iterator pi = m_cpoLocalParteiInfos.find(oRS.GetDat(0));
            if (pi == m_cpoLocalParteiInfos.end()) {
                nPNum = oRS.GetDat(0);
                pP.reset(new CPartei(nPNum));
                pP->SetValue("Nummer", Value(nPNum));
                pP->SetValue("Runde", Value(m_nRunde));
                m_cpoLocalParteiInfos.insert(ParteiInfos::value_type(oRS.GetDat(0), pP));
            }
            else
                pP = (*pi).second;

            ParteiInfos::iterator pig = g_cpoParteiInfos.find(oRS.GetDat(0));
            if (pig == g_cpoParteiInfos.end()) {
                g_cpoParteiInfos.insert(ParteiInfos::value_type(oRS.GetDat(0), pP));
            }
            else {
                if (pig->second->GetValue("Runde") < m_nRunde) {
                    pig->second = pP;
                }
            }

            if (oRS.GetValue() == "ALLIANZ") {
                ac = "+";
                nP = oRS.GetDat(0);
            }
            else if (oRS.GetValue() == "PARTEI") {
                pGruppe.reset();
                pPartei = pP;
                nP = oRS.GetDat(0);
                if (nParteiPhase == 1)
                    nParteiPhase = 2;
                bPartei = true;
                // nLetztePartei = oRS.GetDat(0);
            }

            oRS.Next();
            while (!oRS.EOS() && oRS.GetType() != CReportStream::enBLOCK) {
                if (pP.get() && pPartei.get())
                    pP->SetValue(oRS);
                if (m_nRunde >= m_nMaxRound && oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "status")) {
                    if (pPartei.get()) {
                        ((CPartei*)pPartei.get())->SetAllianz(nP, oRS.GetDat(0));
                    }
                    else if (pGruppe.get()) {
                        ((CGruppe*)pGruppe.get())->SetAllianz(nP, oRS.GetDat(0));
                    }
                }
                if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Rekrutierungskosten")) {
                    nRekrutierungskosten = oRS.GetDat(0);
                }
                else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Anzahl Personen")) {
                    nPersonen = oRS.GetDat(0);
                }
                else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Passwort")) {
                    ac = "+";
                    sPasswort = oRS.GetValue();
                    if (!nRepPartei) {
                        nRepPartei = nPNum;
                        nParteiPhase = 1;
                    }
                    else {
                        if (nPNum != nRepPartei) {
                            if (nPNum != nIgnoredPartei) {
                                ERRMSG(0, ("Line %d, Warnung: Mehr als eine authorisierte Partei! Ignoriere %s", oRS.GetLine(), itoan(nP, PNrBase())));
                                nIgnoredPartei = nPNum;
                            }
                            nParteiPhase++;
                        }
                    }
                }
                else if (m_nPunkte <= 0 && oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Punkte")) {
                    nPunkte = oRS.GetDat(0);
                    if (!nRepPartei) {
                        nRepPartei = nPNum;
                        nParteiPhase = 1;
                    }
                    else {
                        if (nPNum != nRepPartei) {
                            if (nPNum != nIgnoredPartei) {
                                ERRMSG(0, ("Line %d, Warnung: Mehr als eine authorisierte Partei! Ignoriere %s", oRS.GetLine(), itoan(nP, PNrBase())));
                                nIgnoredPartei = nPNum;
                            }
                            nParteiPhase++;
                        }
                    }
                }
                else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Punktedurchschnitt")) {
                    nPunkteschnitt = oRS.GetDat(0);
                    if (!nRepPartei) {
                        nRepPartei = nPNum;
                        nParteiPhase = 1;
                    }
                    else {
                        if (nPNum != nRepPartei) {
                            if (nPNum != nIgnoredPartei) {
                                ERRMSG(0, ("Line %d, Warnung: Mehr als eine authorisierte Partei! Ignoriere %s", oRS.GetLine(), itoan(nP, PNrBase())));
                                nIgnoredPartei = nPNum;
                            }
                            nParteiPhase++;
                        }
                    }
                }
                else if (oRS.GetType() == CReportStream::enINTEGER && IsEqual(oRS.GetComment(), "Runde")) {
                    nRunde = m_nRunde ? m_nRunde : oRS.GetDat(0);
                }
                else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Parteiname")) {
                    sParteiname = oRS.GetValue();
                }
                else if (oRS.GetType() == CReportStream::enSTRING && IsEqual(oRS.GetComment(), "Magiegebiet")) {
                    if (!nRepPartei) {
                        nRepPartei = nPNum;
                        nParteiPhase = 1;
                    }
                    else {
                        if (nPNum != nRepPartei) {
                            if (nPNum != nIgnoredPartei) {
                                //                                ERRMSG( 0, ( "Line %d, Warnung: Mehr als eine authorisierte Partei! Ignoriere %s", oRS.GetLine(), itoan(nP,PNrBase()) ));
                                nIgnoredPartei = nPNum;
                            }
                            nParteiPhase++;
                        }
                    }
                }
                oRS.Next();
            }
            if (bPartei && nParteiPhase == 1) {
                m_nPartei = nP;
                m_sParteiname = sParteiname;
                m_nRekrutierungskosten = nRekrutierungskosten;
                m_nPersonen = nPersonen;
                m_sPasswort = sPasswort;
                m_nPunkte = nPunkte;
                m_nPunkteschnitt = nPunkteschnitt;
                m_nRunde = nRunde;
            }
            if (nP >= 0 && !sParteiname.empty()) {
                if (m_coParteien[nP].empty() || m_coParteien[nP][0] == '-')
                    m_coParteien[nP] = std::string(ac) + sParteiname;
                nP = -1;
            }
        }
        else if (oRS.GetType() == CReportStream::enBLOCK && oRS.GetValue() == "GRUPPE") {
            Gruppen::iterator gi = m_cpoGruppen.find(oRS.GetDat(0));
            CGruppe::Ptr pG;
            int32_t nID = oRS.GetDat(0);
            if (gi == m_cpoGruppen.end()) {
                pG.reset(new CPartei(nID));
                pG->SetValue("Nummer", Value(nID));
                m_cpoGruppen.insert(Gruppen::value_type(nID, pG));
            }
            else
                pG = (*gi).second;

            pPartei.reset();
            pGruppe = pG;

            oRS.Next();
            while (!oRS.EOS() && oRS.GetType() != CReportStream::enBLOCK) {
                if (pG.get())
                    pG->SetValue(oRS);
                oRS.Next();
            }
        }
        else if (oRS.GetType() == CReportStream::enBLOCK && (oRS.GetValue() == "REGION" || oRS.GetValue() == "DURCHREISEREGION" || oRS.GetValue() == "SPEZIALREGION")) {
            if (bFirstRegion) {
                CMessage::Ptr pMsg(new CMessage(m_nRunde));
                pMsg->SetValue("type", Value(-2));
                m_cpoMessages[m_cpoMessages.size()] = pMsg;
                bFirstRegion = false;
            }

            m_poMap->Import(oRS, m_nRunde);
        }
        else if (oRS.GetValue() == "BATTLE") {
            inBattle = true;
            battle_x = oRS.GetDat(0);
            battle_y = oRS.GetDat(1);
            battle_z = oRS.GetDat(2);
            CMessage::Ptr pMsg(new CMessage(m_nRunde));
            pMsg->SetValue("region", Value(battle_x));
            pMsg->SetValue("region:1", Value(battle_y));
            pMsg->SetValue("region:2", Value(battle_z));
            pMsg->SetValue("type", Value(-1));
            m_cpoMessages[m_cpoMessages.size()] = pMsg;
            oRS.Next();
            while (!oRS.EOS() && (oRS.GetType() != CReportStream::enBLOCK || oRS.GetValue() != "MESSAGE"))
                oRS.Next();
        }
        else if (oRS.GetValue() == "ZAUBER" || oRS.GetValue() == "TRAENKE") {
            oRS.Next();
            while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK))
                oRS.Next();
        }
        else if (oRS.GetValue() == "BATTLESPEC") {
            oRS.Next();
            while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK))
                oRS.Next();
        }
        else {
            if (oRS.GetType() != CReportStream::enBLOCK) {
                SetValue(oRS);
            }
            else {
                if (CHierarchy::IsChild("VERSION", oRS.GetValue())) {
                    LoadSubObject(oRS);
                }
            }
            oRS.Next();
        }
    } while (!oRS.EOS());

    CRegion::Einheiten::iterator ui;
    int32_t en1, en2, en3, en4;
    std::string::size_type pos;
    bool bUsed;

    if (MessageRenderer() == CMessage::ERESSEA2) {
        SetMessageRule(-1, "\"In $region($region) fand ein Kampf statt.\"");
        SetMessageRule(-2, "\"Ende der globalen Nachrichten.\"");
    }
    else {
        SetMessageRule(-1, "In {region} fand ein Kampf statt.");
        SetMessageRule(-2, "Ende der globalen Nachrichten.");
    }
    SetMessageSection(-1, "battle");
    SetMessageSection(-2, "dummy");

    for (Messages::const_iterator mi = m_cpoMessages.begin(); mi != m_cpoMessages.end(); mi++) {
        bUsed = false;
        // TRACEMSG(( "%s\n", ((CMessage*)((*mi).second.get()))->Render( this ).c_str() ));
        en1 = (*mi).second->GetValue("unit", Value("")).asLong();
        en2 = (*mi).second->GetValue("target", Value("")).asLong();
        en3 = (*mi).second->GetValue("teacher", Value("")).asLong();
        en4 = (*mi).second->GetValue("student", Value("")).asLong();
        if (en1) {
            ui = m_cpoGEinheiten.find(en1);
            if (ui != m_cpoGEinheiten.end() && (*ui).second->Partei() == Partei()) {
                (*ui).second->AddMessage((*mi).second);
                bUsed = true;
            }
        }
        if (en2) {
            ui = m_cpoGEinheiten.find(en2);
            if (ui != m_cpoGEinheiten.end() && (*ui).second->Partei() == Partei()) {
                (*ui).second->AddMessage((*mi).second);
                bUsed = true;
            }
        }
        if (en3) {
            ui = m_cpoGEinheiten.find(en3);
            if (ui != m_cpoGEinheiten.end() && (*ui).second->Partei() == Partei()) {
                (*ui).second->AddMessage((*mi).second);
                bUsed = true;
            }
        }
        if (en4) {
            ui = m_cpoGEinheiten.find(en4);
            if (ui != m_cpoGEinheiten.end() && (*ui).second->Partei() == Partei()) {
                (*ui).second->AddMessage((*mi).second);
                bUsed = true;
            }
        }
        if (!bUsed) {
            if (!(*mi).second->GetValue("region", Value("")).asString().empty()) {
                int32_t x, y, z;
                ((CMessage*)((*mi).second.get()))->GetCoords(/*(*mi).second->GetValue(*/ "region" /*, Value( "" ) ).asString()*/, x, y, z);
                CRegion* pReg = GetMap()->GetFromECords(x, y, z);
                if (pReg) {
                    pReg->AddMessage((*mi).second);
                    bUsed = true;
                }
            }
            if (!(*mi).second->GetValue("start", Value("")).asString().empty()) {
                int32_t x, y, z;
                ((CMessage*)((*mi).second.get()))->GetCoords(/*(*mi).second->GetValue(*/ "start" /*, Value( "" ) ).asString()*/, x, y, z);
                CRegion* pReg = GetMap()->GetFromECords(x, y, z);
                if (pReg) {
                    pReg->AddMessage((*mi).second);
                    bUsed = true;
                }
            }
            if (!(*mi).second->GetValue("end", Value("")).asString().empty()) {
                int32_t x, y, z;
                ((CMessage*)((*mi).second.get()))->GetCoords(/*(*mi).second->GetValue( */ "end" /*, Value( "" ) ).asString()*/, x, y, z);
                CRegion* pReg = GetMap()->GetFromECords(x, y, z);
                if (pReg) {
                    pReg->AddMessage((*mi).second);
                    bUsed = true;
                }
            }
        }
        if (bUsed) {
            (*mi).second->SetValue("_used", Value(1));
        }
    }

    for (Nachrichten::const_iterator ni = m_csNachrichten.begin(); ni != m_csNachrichten.end(); ni++) {
        //		switch( (*ni).first )
        //		{
        //		case MT_HANDEL: StatistikHandel( (*ni).second ); break;
        //		}

        pos = 0;
        bUsed = false;
        en1 = FindNextENum((*ni).second, pos);
        if (en1 > 0) {
            ui = m_cpoGEinheiten.find(en1);
            if (ui != m_cpoGEinheiten.end() && (*ui).second->Partei() == Partei()) {
                (*ui).second->AddMessage((*ni).second);
                bUsed = true;
            }

            en2 = FindNextENum((*ni).second, pos);
            if (en2 > 0 && en2 != en1) {
                ui = m_cpoGEinheiten.find(en2);
                if (ui != m_cpoGEinheiten.end() && (*ui).second->Partei() == Partei()) {
                    (*ui).second->AddMessage((*ni).second);
                    bUsed = true;
                }
            }
        }

        if (!bUsed) {
            int x, y, z;
            pos = 0;
            if (FindNextRegion((*ni).second, pos, x, y, z)) {
                CRegion* pReg = GetMap()->GetFromECords(x, y, z);
                if (pReg)
                    pReg->AddMessage((*ni).second);
            }
        }
    }

    CConfigFile oCF(g_sConfigFile);
    std::string sTxt;
    size_t i = 1;
    while (oCF.FetchLine("Options", i++, false)) {
        sTxt = oCF.GetString(0);
        if (CRegExp::Match(sTxt, "EBase\\s*=\\s*")) {
            CRegExp::Replace(sTxt, "EBase\\s*=\\s*", "");
            int b = atoi(sTxt.c_str());
            if (b >= 2 && b <= 36)
                m_nENrBase = b;
        }
        else if (CRegExp::Match(sTxt, "PBase\\s*=\\s*")) {
            CRegExp::Replace(sTxt, "PBase\\s*=\\s*", "");
            int b = atoi(sTxt.c_str());
            if (b >= 2 && b <= 36)
                m_nPNrBase = b;
        }
        else if (CRegExp::Match(sTxt, "BBase\\s*=\\s*")) {
            CRegExp::Replace(sTxt, "BBase\\s*=\\s*", "");
            int b = atoi(sTxt.c_str());
            if (b >= 2 && b <= 36)
                m_nBNrBase = b;
        }
        else if (m_sPasswort.empty() && m_sDefaultPassword.empty() && CRegExp::Match(sTxt, std::string("Passwor[dt](") + itoan(m_nPartei, m_nPNrBase) + ")?\\s*=\\s*")) {
            CRegExp::Replace(sTxt, std::string("Passwor[dt](") + itoan(m_nPartei, m_nPNrBase) + ")?\\s*=\\s*", "");
            if (sTxt.length() > 1 && sTxt[0] == 34)
                sTxt = sTxt.substr(1, sTxt.length() - 2);
            m_sPasswort = sTxt;
        }
    }

    if (m_sPasswort.empty()) {
        m_sPasswort = m_sDefaultPassword;
    }

    m_bIsValid = true;
}

int32_t CReport::GetGroupIdByName(const std::string& sName)
{
    Gruppen::iterator gi = m_cpoGruppen.begin();
    while (gi != m_cpoGruppen.end()) {
        if (CRegExp::Match((*gi).second->GetValue("name").asString(), sName)) {
            return (*gi).first;
        }
        gi++;
    }
    return -1;
}

void CReport::Write(const std::string& sFName)
{
    CReportStream oRS(sFName, false);

    oRS.WriteBlock("VERSION", "Version des Computer Reports", 10029);
    oRS.WriteLine("Standard", "Konfiguration");
    m_poMap->Write(oRS);
}

Value CReport::GetValue(const std::string& sKey)
{
    if (IsEqual(sKey.c_str(), "Runde"))
        return Value(int32_t(m_nRunde));
    if (IsEqual(sKey.c_str(), "Partei"))
        return Value(itoan(m_nPartei, PNrBase()));
    if (IsEqual(sKey.c_str(), "Rekrutierungskosten"))
        return Value(int32_t(m_nRekrutierungskosten));
    if (IsEqual(sKey.c_str(), "Personen"))
        return Value(int32_t(m_nPersonen));
    if (IsEqual(sKey.c_str(), "Spiel"))
        return Value(m_sSpiel);
    return CBlockBase::GetValue(sKey, Value(0));
}

CEinheit* CReport::SearchUnit(int32_t nENr, bool bDeep)
{
    CRegion::Einheiten::iterator ui;
    Reports::iterator ri;
    ui = m_cpoGEinheiten.find(nENr);
    if (ui != m_cpoGEinheiten.end())
        return (*ui).second;
    else {
        if (!bDeep)
            return 0;
        for (ri = m_cpoReports.begin(); ri != m_cpoReports.end(); ri++) {
            //			if( (*ri)->Runde() == Runde() )
            //			{
            ui = (*ri)->m_cpoGEinheiten.find(nENr);
            if (ui != (*ri)->m_cpoGEinheiten.end())
                return (*ui).second;
            //			}
        }
    }
    return 0;
}

int32_t CReport::PNrFromENr(int32_t nENr)
{
    CEinheit* pE;

    pE = SearchUnit(nENr);
    if (pE) {
        return pE->Partei();
    }
    return -1;
}

void CReport::CalculateStatistics()
{
    CRegion* pReg;
    CEinheit* pE;
    int32_t nP1, nP2;

    m_nEinkommen = m_nMsgEinkommen;
    m_nAusgaben = m_nMsgAusgaben;

    for (Messages::const_iterator mi = m_cpoMessages.begin(); mi != m_cpoMessages.end(); mi++) {
        if (m_nRunde < 227) {
            switch ((*mi).second->GetValue("type").asLong()) {
                case 581:
                    nP1 = PNrFromENr((*mi).second->GetValue("unit").asLong());
                    nP2 = PNrFromENr((*mi).second->GetValue("target").asLong());
                    if (nP1 != nP2) {
                        if (nP1 == m_nPartei)
                            InsertHandel(nP2, -(*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource").asString());
                        else
                            InsertHandel(nP1, (*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource").asString());
                    }
                    break;
                case 9386:
                    nP1 = (*mi).second->GetValue("from").asLong();
                    nP2 = (*mi).second->GetValue("to").asLong();
                    if (nP1 != nP2) {
                        if (nP1 == m_nPartei)
                            InsertHandel(nP2, -(*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource", Value("Silber")).asString());
                        else
                            InsertHandel(nP1, (*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource", Value("Silber")).asString());
                    }
                    break;
            }
        }
        else if (m_nRunde >= 227) {
            switch ((*mi).second->GetValue("type").asLong()) {
                case 5281483:
                case 1235024123:
                    nP1 = PNrFromENr((*mi).second->GetValue("unit").asLong());
                    nP2 = PNrFromENr((*mi).second->GetValue("target").asLong());
                    if (nP1 != nP2) {
                        if (nP1 == m_nPartei)
                            InsertHandel(nP2, -(*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource").asString());
                        else
                            InsertHandel(nP1, (*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource").asString());
                    }
                    break;
                case 1682429624:
                    nP1 = (*mi).second->GetValue("from").asLong();
                    nP2 = (*mi).second->GetValue("to").asLong();
                    if (nP1 != nP2) {
                        if (nP1 == m_nPartei)
                            InsertHandel(nP2, -(*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource", Value("Silber")).asString());
                        else
                            InsertHandel(nP1, (*mi).second->GetValue("amount").asLong(), (*mi).second->GetValue("resource", Value("Silber")).asString());
                    }
                    break;
            }
        }
    }

    for (Nachrichten::const_iterator ni = m_csNachrichten.begin(); ni != m_csNachrichten.end(); ni++) {
        switch ((*ni).first) {
            case MT_EREIGNISSE:
                StatistikEreignisse((*ni).second);
                break;
            case MT_HANDEL:
                StatistikHandel((*ni).second);
                break;
            case MT_EINKOMMEN:
                StatistikEinkommen((*ni).second);
                break;
        }
    }

    int nSilber = 0;

    for (TradeInfos::const_iterator ti = m_coTradeInfos.begin(); ti != m_coTradeInfos.end(); ti++) {
        pReg = nullptr;
        if ((*ti).m_nZ != -1) {
            pReg = GetMap()->GetFromECords((*ti).m_nX, (*ti).m_nY, (*ti).m_nZ);
        }
        else if ((*ti).m_nUnit > 0) {
            pE = SearchUnit((*ti).m_nUnit);
            if (pE) {
                pReg = (CRegion*)pE->Region();
            }
        }

        nSilber = (*ti).m_nSilber;
        if ((*ti).m_nBuilding > 0 && pReg) {
            CBauwerk* pB = pReg->GetBuilding((*ti).m_nBuilding);
            if (pB)
                nSilber = -(pB->Unterhalt());
        }

        if (pReg) {
            if (nSilber > 0)
                pReg->AddEinkommen(nSilber);
            else
                pReg->AddAusgaben(-nSilber);
        }
    }

    if (ExistUserFunction("CalcRegionIncome") || ExistUserFunction("CalcRegionExpenses")) {
        Value oVErg;
        ArgumentList coArgs;
        for (CKarte::RegionMap::const_iterator mi = m_poMap->Regions().begin(); mi != m_poMap->Regions().end(); mi++) {
            g_poCurrentRegion = (*mi).second;
            if (DoUserFunction(std::string("CalcRegionIncome"), coArgs, &oVErg)) {
                g_poCurrentRegion->SetEinkommen(oVErg.asLong());
            }
            if (DoUserFunction(std::string("CalcRegionExpenses"), coArgs, &oVErg)) {
                g_poCurrentRegion->SetAusgaben(oVErg.asLong());
            }
        }
    }
}

void CReport::StatistikEreignisse(const std::string& sMsg)
{
    CEinheit* pE;
    CRegion* pReg = 0;
    char* pStr;
    int32_t e1;
    std::string::size_type pos, pos2 = 0;
    // int32_t p1 = -1;
    int32_t cnt;

    pos = sMsg.find(") bezahlt ");
    if (pos == std::string::npos) {
        //        StatistikHandel( sMsg );
        return;
    }
    else
        pos += 10;

    e1 = FindNextENum(sMsg, pos2);
    if (e1 > 0) {
        pE = SearchUnit(e1);
        if (pE) {
            // p1 = pE->Partei();
            pReg = (CRegion*)pE->Region();
        }
    }

    if (sMsg[pos] == '$')
        pos++;

    cnt = (int32_t)strtol(sMsg.c_str() + pos, &pStr, 10);
    if (!*pStr)
        return;

    m_nAusgaben += cnt;
    if (pReg)
        pReg->AddAusgaben(cnt);
}

void CReport::InsertHandel(int32_t nPNr, int32_t nAmount, const std::string& sProduct)
{
    Handelspartner::iterator hi;
    CParteihandel::Produkte::iterator pi;
    hi = m_cpoHPartner.find(nPNr);
    if (hi == m_cpoHPartner.end()) {
        hi = m_cpoHPartner.insert(Handelspartner::value_type(nPNr, new CParteihandel)).first;
    }
    pi = (*hi).second->m_coProdukte.find(sProduct);
    if (pi == (*hi).second->m_coProdukte.end()) {
        (*hi).second->m_coProdukte.insert(CParteihandel::Produkte::value_type(sProduct, nAmount));
    }
    else {
        (*pi).second += nAmount;
    }
}

void CReport::StatistikHandel(const std::string& sMsg)
{
    CEinheit* pE;
    CRegion* pReg = 0;
    char* pStr;
    int32_t e1 = -1, e2 = -1;
    std::string::size_type pos = 0, pos2;
    int32_t p1 = -1, p2 = -1, ph;
    int32_t cnt;
    bool bKauf = false;

    if (sMsg.empty() || sMsg[0] == 'F')
        return;

    e1 = FindNextENum(sMsg, pos);
    if (e1 > 0) {
        pE = SearchUnit(e1, true);
        if (pE) {
            p1 = pE->Partei();
            pReg = (CRegion*)pE->Region();
        }

        e2 = FindNextENum(sMsg, pos);
        if (e2 > 0) {
            pE = SearchUnit(e2, true);
            if (pE)
                p2 = pE->Partei();
        }
    }

    if (p1 == p2) {
        return;
    }

    pos = sMsg.find(") gibt ");
    if (pos == std::string::npos) {
        pos = sMsg.find(") zahlte ");
        if (pos == std::string::npos) {
            pos = sMsg.find(") kauft f\xFCr ");
            if (pos == std::string::npos) {
                pos = sMsg.find(") kauft fuer ");
                if (pos == std::string::npos) {
                    return;
                }
                else {
                    pos += 13;
                    bKauf = true;
                }
            }
            else {
                pos += 12;
                bKauf = true;
            }
        }
        else
            pos += 9;
    }
    else
        pos += 7;

    cnt = (int32_t)strtol(sMsg.c_str() + pos, &pStr, 10);
    if (!*pStr)
        return;

    if (bKauf) {
        if (pReg) {
            pReg->AddAusgaben(cnt);
        }
        m_nAusgaben += cnt;
        return;
    }

    pos = (size_t)(pStr - sMsg.c_str() + 1);
    pos2 = sMsg.find(" an ", pos);
    if (pos2 == std::string::npos)
        return;

    if (p1 == m_nPartei) {
        cnt = -cnt;
        ph = p2;
    }
    else {
        ph = p1;
    }

    InsertHandel(ph, cnt, sMsg.substr(pos, pos2 - pos));

    /*
            hi = m_cpoHPartner.find( ph );
            if( hi == m_cpoHPartner.end() )
            {
                    hi = m_cpoHPartner.insert( Handelspartner::value_type( ph, new CParteihandel ) ).first;
            }
            pi = (*hi).second->m_coProdukte.find( sMsg.substr( pos, pos2-pos ) );
            if( pi == (*hi).second->m_coProdukte.end() )
            {
                    (*hi).second->m_coProdukte.insert( CParteihandel::Produkte::value_type( sMsg.substr( pos, pos2-pos ), cnt ) );
            }
            else
            {
                    (*pi).second += cnt;
            }
    */
    //	printf( " P%d -> P%d: %d %s\n", p1, p2, cnt, sMsg.substr( pos, pos2-pos ).c_str() );
}

void CReport::StatistikProduktion(const std::string& sMsg) {}

void CReport::StatistikEinkommen(const std::string& sMsg)
{
    CEinheit* pE;
    CRegion* pReg = 0;
    char* pStr;
    int32_t e1 = -1;
    std::string::size_type pos = 0, pos2 = 0;
    // int32_t p1 = -1;
    int32_t cnt;

    pos = sMsg.find(") verdient ");
    if (pos == std::string::npos) {
        pos = sMsg.find(") treibt ");
        if (pos == std::string::npos)
            return;
        else
            pos += 9;
    }
    else
        pos += 11;

    e1 = FindNextENum(sMsg, pos2);
    if (e1 > 0) {
        pE = SearchUnit(e1);
        if (pE) {
            // p1 = pE->Partei();
            pReg = (CRegion*)pE->Region();
        }
    }

    if (sMsg[pos] == '$')
        pos++;

    if (!isdigit(sMsg[pos])) {
        pos = sMsg.find("Silber");
        if (pos != std::string::npos) {
            pos--;
            while (pos > 1 && isdigit(sMsg[pos - 1]))
                pos--;
        }
    }

    cnt = (int32_t)strtol(sMsg.c_str() + pos, &pStr, 10);
    if (!*pStr)
        return;

    m_nEinkommen += cnt;
    if (pReg)
        pReg->AddEinkommen(cnt);
}

/////////////////////////////////////////////////////////////////////
//.class: CRegionSorter
/////////////////////////////////////////////////////////////////////

bool CRegionSorter::operator()(CRegion* pR1, CRegion* pR2) const
{
    static int32_t noName = CStringDB::Str2SID("");
    if (m_nFlags) {
        if (!(pR1->m_idInsel == noName || pR1->m_idInsel == noName) && pR1->GetSortIsland() == pR2->GetSortIsland()) {
            if (pR1->m_idInsel == pR2->m_idInsel)
                return pR1->m_nPos < pR2->m_nPos;
            else
                return CStringDB::SID2Str(pR1->m_idInsel) < CStringDB::SID2Str(pR2->m_idInsel);
        }
        else
            return pR1->GetSortIsland() < pR2->GetSortIsland();
    }
    else
        return pR1->m_nPos < pR2->m_nPos;
}

bool rqcmp::operator()(const CRegion* pR1, const CRegion* pR2) const
{
    return pR1->GetQuality() > pR2->GetQuality();
}

/////////////////////////////////////////////////////////////////////
//.class: CRegion
/////////////////////////////////////////////////////////////////////

const char* g_pcLuxusName[] = {"Balsam", "Gewuerz", "Juwel", "Myrrhe", "Oel", "Seide", "Weihrauch"};

CRegion::TerrainTypes CRegion::m_coTerrains;
CRegion::ResourceImpacts CRegion::m_coRImpacts;
int32_t CRegion::m_nCurrentPlayer = -2;
int32_t CRegion::m_nCurrentRound = -2;
int32_t CRegion::m_nMoveX = 0;
int32_t CRegion::m_nMoveY = 0;

CRegion::CRegion(const std::string& sType, int32_t nX, int32_t nY, int32_t nZ, int nRunde)
    : CBlockBase("CRegion")
    , m_poMap(NULL)
    , m_nInsel(0x7fffffffl, 0x7fffffffl, 0x7fffffffl)
    , m_idInsel(CStringDB::Str2SID(""))
    , m_enBlock(enUNKNOWN)
    , m_nRunde(nRunde)
    , m_nPos(-1)
    , m_nX(nX)
    , m_nY(nY)
    , m_nZ(nZ)
    , m_nPartei(0)
    , m_bVerorkt(false)
    , m_poTerrain(0)
    , m_nBauern(-1)
    , m_nPferde(0)
    , m_nBaeume(0)
    , m_nMallorn(0)
    , m_nEisen(-2)
    , m_nLaen(-2)
    , m_nSilber(-1)
    , m_nUnterhalt(-1)
    , m_nRekruten(-1)
    , m_nLohn(-1)
    , m_nStrasse(-1)
    , m_nVerkauf(-1)
    , m_nMaxBurg(0)
    , m_bOwnUnit(false)
    , m_nEinkommen(0)
    , m_nAusgaben(0)
{
    m_poTerrain = FindTerrain(sType);

    if (m_poTerrain->m_bLand)
        m_nInsel = GetKey();
    else {
        m_nBauern = 0;
        m_nBaeume = 0;
        m_nPferde = 0;
        m_nBaeume = 0;
        m_nMallorn = 0;
        m_nEisen = 0;
        m_nLaen = 0;
        m_nSilber = 0;
        m_nUnterhalt = 0;
        m_nRekruten = 0;
    }
}

CRegion::CRegion(CReportStream& oRS, CKarte* poMap, int nRunde, int32_t nPos)
    : CBlockBase("CRegion")
    , m_poMap(poMap)
    , m_nInsel(0x7fffffffl, 0x7fffffffl, 0x7fffffffl)
    , m_idInsel(CStringDB::Str2SID(""))
    , m_enBlock(enUNKNOWN)
    , m_nRunde(nRunde)
    , m_nPos(nPos)
    , m_nX(0)
    , m_nY(0)
    , m_nZ(0)
    , m_nPartei(0)
    , m_bVerorkt(false)
    , m_poTerrain(0)
    , m_nBauern(0)
    , m_nPferde(0)
    , m_nBaeume(0)
    , m_nMallorn(0)
    , m_nEisen(-2)
    , m_nLaen(-2)
    , m_nSilber(0)
    , m_nUnterhalt(0)
    , m_nRekruten(0)
    , m_nLohn(0)
    , m_nStrasse(0)
    , m_nVerkauf(-1)
    , m_nMaxBurg(0)
    , m_nBonus(0)
    , m_bOwnUnit(false)
    , m_nEinkommen(0)
    , m_nAusgaben(0)
{
    int32_t h;
    std::string sTerrain;

    if (IsEqual(oRS.GetValue(), "REGION")) {
        m_enBlock = enREGION;
    }
    else if (IsEqual(oRS.GetValue(), "DURCHREISEREGION")) {
        m_enBlock = enDURCHREISEREGION;
    }
    else if (IsEqual(oRS.GetValue(), "SCHEMEN")) {
        m_enBlock = enSCHEMEN;
    }
    else if (IsEqual(oRS.GetValue(), "SPEZIALREGION")) {
        m_enBlock = enSPEZIALREGION;
    }
    m_nX = oRS.GetDat(0) + m_nMoveX;
    m_nY = oRS.GetDat(1) + m_nMoveY;
    m_nZ = oRS.GetDat(2);

    oRS.Next();
    do {
        if (oRS.GetType() != CReportStream::enBLOCK) {
            TAGMAP::iterator ti = g_coTags.find(DeUmlaut(oRS.GetComment()));
            if (ti != g_coTags.end()) {
                switch ((*ti).second) {
                    case T_Runde:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nRunde = oRS.GetDat(0);
                        break;
                    case T_Partei:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nPartei = oRS.GetDat(0);
                        break;
                    case T_Name:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sName = oRS.GetValue();
                        break;
                    case T_Terrain:
                        if (oRS.GetType() == CReportStream::enSTRING) {
                            m_poTerrain = FindTerrain(oRS.GetValue());
                            if (!m_poTerrain->m_bEisen)
                                m_nEisen = -1;
                            if (!m_poTerrain->m_bLaen)
                                m_nLaen = -1;
                            /*
                                                                                    switch( oRS.GetValue()[0] )
                                                                                    {
                                                                                    case 'E': m_enType = enEBENE; break;
                                                                                    case 'O': m_enType = enOZEAN; break;
                                                                                    case 'S': m_enType = enSUMPF; break;
                                                                                    case 'W':
                                                                                                    if( IsEqual( oRS.GetValue().c_str(),"Wueste") )
                                                                                                            m_enType = enWUESTE;
                                                                                                    else
                                                                                                            m_enType = enWALD;
                                                                                                    break;
                                                                                    case 'H': m_enType = enHOCHLAND; break;
                                                                                    case 'G': m_enType = enGLETSCHER; break;
                                                                                    case 'B': m_enType = enBERG; break;
                                                                                    case 'f':
                                                                                    case 'F': m_enType = enFEUERWAND; break;
                                                                                    }
                            */
                        }
                        break;
                    case T_Bauern:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBauern = oRS.GetDat(0);
                        break;
                    case T_Beschr:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sBeschreibung = oRS.GetValue();
                        break;
                    case T_Insel:
                        if (oRS.GetType() == CReportStream::enSTRING) {
                            m_idInsel = CStringDB::Str2SID(oRS.GetValue());
                            Map()->Report()->m_bHasIslandTags = true;
                        }
                        else if (oRS.GetType() == CReportStream::enINTEGER) {
                            m_idInsel = CStringDB::Str2SID(CBlockBase::GetValue(Map()->Report(), std::string("island[") + ToString(int32_t(oRS.GetDat(0))) + "].name").asString());
                            Map()->Report()->m_bHasIslandTags = true;
                        }
                        break;
                    case T_Pferde:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nPferde = oRS.GetDat(0);
                        break;
                    case T_Baeume:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBaeume = oRS.GetDat(0);
                        break;
                    case T_Mallorn:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nMallorn = oRS.GetDat(0);
                        break;
                    case T_Eisen:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nEisen = oRS.GetDat(0);
                        break;
                    case T_Laen:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nLaen = oRS.GetDat(0);
                        break;
                    case T_Silber:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nSilber = oRS.GetDat(0);
                        break;
                    case T_Unterh:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nUnterhalt = oRS.GetDat(0);
                        break;
                    case T_Rekruten:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nRekruten = oRS.GetDat(0);
                        break;
                    case T_Lohn:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nLohn = oRS.GetDat(0);
                        break;
                    case T_Strasse:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nStrasse = oRS.GetDat(0);
                        break;
                    case T_TNorden:
                    case T_TSueden:
                    case T_TOsten:
                    case T_TWesten:
                        if (oRS.GetType() == CReportStream::enSTRING) {
                            sTerrain = oRS.GetValue();
                            /*
                                                                                    switch( oRS.GetValue()[0] )
                                                                                    {
                                                                                    case 'E': enTerrain = enEBENE; break;
                                                                                    case 'O': enTerrain = enOZEAN; break;
                                                                                    case 'S': enTerrain = enSUMPF; break;
                                                                                    case 'W':
                                                                                                    if( IsEqual( oRS.GetValue().c_str(),"Wueste") )
                                                                                                            enTerrain = enWUESTE;
                                                                                                    else
                                                                                                            enTerrain = enWALD;
                                                                                                    break;
                                                                                    case 'H': enTerrain = enHOCHLAND; break;
                                                                                    case 'G': enTerrain = enGLETSCHER; break;
                                                                                    case 'B': enTerrain = enBERG; break;
                                                                                    case 'F': enTerrain = enFEUERWAND; break;
                                                                                    }
                            */
                        }
                        break;
                    case T_NNorden:
                        if (m_poMap->GetFromECords(m_nX, m_nY - 1, m_nZ)->m_poTerrain->m_sName.empty())
                            m_poMap->Set(new CRegion(sTerrain, m_nX, m_nY - 1, m_nZ));
                        break;
                    case T_NSueden:
                        if (m_poMap->GetFromECords(m_nX, m_nY + 1, m_nZ)->m_poTerrain->m_sName.empty())
                            m_poMap->Set(new CRegion(sTerrain, m_nX, m_nY + 1, m_nZ));
                        break;
                    case T_NOsten:
                        if (m_poMap->GetFromECords(m_nX + 1, m_nY, m_nZ)->m_poTerrain->m_sName.empty())
                            m_poMap->Set(new CRegion(sTerrain, m_nX + 1, m_nY, m_nZ));
                        break;
                    case T_NWesten:
                        if (m_poMap->GetFromECords(m_nX - 1, m_nY, m_nZ)->m_poTerrain->m_sName.empty())
                            m_poMap->Set(new CRegion(sTerrain, m_nX - 1, m_nY, m_nZ));
                        break;
                    case T_maxLuxus:
                        break;
                    case T_Verorkt:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_bVerorkt = (oRS.GetDat(0) != 0);
                        break;
                    case T_herb:
                        SetValue(oRS);
                        Map()->Report()->m_bHasIslandTags = true;
                        break;
                    case T_Schoesslinge:
                    case T_Steine:
                    case T_visibility:
                        SetValue(oRS);
                        break;
                    default:
                        if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("REGION", oRS.GetComment()))
                            ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                        SetValue(oRS);
                }
            }
            else {
                if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("REGION", oRS.GetComment()))
                    ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                SetValue(oRS);
            }
            oRS.Next();
        }
        else {
            if (m_enBlock == enSCHEMEN || oRS.GetValue() == "REGION" || oRS.GetValue() == "DURCHREISEREGION" || oRS.GetValue() == "SPEZIALREGION") {
                break;
            }
            else if (oRS.GetValue() == "RESOURCE") {
                CResource* pR;
                //				int32_t h = oRS.GetDat(0);
                pR = new CResource(oRS);
                m_cpoResourcen.insert(Resourcen::value_type(Flatten(pR->GetValue("type").asString()), pR));
                m_cpoVResourcen.push_back(pR);
            }
            else if (oRS.GetValue() == "MESSAGETYPES") {
                m_poMap->Report()->SetMessageRenderer(CMessage::ERESSEA1);
                oRS.Next();
                while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK)) {
                    if (oRS.GetType() == CReportStream::enSTRING)
                        m_poMap->Report()->SetMessageRule(atoi(oRS.GetComment().c_str()), oRS.GetValue());
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "MESSAGETYPE") {
                int32_t nID = oRS.GetDat(0);
                m_poMap->Report()->SetMessageRenderer(CMessage::ERESSEA2);
                oRS.Next();
                while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK)) {
                    if (oRS.GetType() == CReportStream::enSTRING) {
                        if (IsEqual(oRS.GetComment(), "text"))
                            m_poMap->Report()->SetMessageRule(nID, oRS.GetValue());
                        else if (IsEqual(oRS.GetComment(), "section"))
                            m_poMap->Report()->SetMessageSection(nID, oRS.GetValue());
                    }
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "TRANSLATION") {
                oRS.Next();
                while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK)) {
                    if (oRS.GetType() == CReportStream::enSTRING) {
                        CTranslationDB::m_coI2L[oRS.GetComment()] = oRS.GetValue();
                        CTranslationDB::m_coL2I[oRS.GetValue()] = oRS.GetComment();
                    }
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "PREISE") {
                Luxusgut m_oLG;
                int preis = 0;
                while (!oRS.EOS()) {
                    oRS.Next();
                    if (oRS.GetType() == CReportStream::enBLOCK)
                        break;
                    if (oRS.GetDat(0) < 0)
                        m_nVerkauf = preis;
                    m_oLG.first = oRS.GetComment();
                    m_oLG.second = oRS.GetDat(0);
                    m_coLuxusgueter.push_back(m_oLG);
                    preis++;
                }
                // oRS.Next();
            }
            else if (oRS.GetValue() == "SCHEMEN") {
                poMap->Set(new CRegion(oRS, poMap, nRunde, nPos));
            }
            else if (oRS.GetValue() == "REGIONSBOTSCHAFTEN" || oRS.GetValue() == "UMGEBUNG" || oRS.GetValue() == "REGIONSKOMMENTAR" || oRS.GetValue() == "REGIONSEREIGNISSE") {
                // REGIONSBOTSCHAFTEN werden zur Zeit noch nicht gespeichert
                CBlockBase::Ptr pBlk(new CBlockBase(oRS.GetValue().c_str(), oRS.GetValue().c_str()));
                AddBlock(pBlk);
                int cnt = 0;

                oRS.Next();
                while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK)) {
                    if (oRS.GetType() == CReportStream::enSTRING) {
                        m_coBotschaften.push_back(oRS.GetValue());
                        pBlk->SetValue(std::string("@") + ToString(int32_t(cnt++)), oRS.GetValue());
                    }
                    else
                        break;
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "BURG") {
                CBauwerk* pB;
                h = oRS.GetDat(0);
                pB = new CBauwerk(oRS, nRunde);
                if (IsEqual(pB->Typ().c_str(), "Burg") || CBurgInfo::Lookup(pB->Typ()).GetValue("Groesse").asLong()) {
                    if (m_nMaxBurg < pB->Groesse()) {
                        m_nMaxBurg = pB->Groesse();
                        m_nBonus = CBurgInfo::Lookup(pB->Typ()).GetValue("Bonus").asLong();
                    }
                }
                if (!m_pcpoBauwerke)
                    m_pcpoBauwerke.reset(new Bauwerke());
                m_pcpoBauwerke->insert(Bauwerke::value_type(h, pB));
                if (!m_pcpoVBauwerke)
                    m_pcpoVBauwerke.reset(new VBauwerke());
                m_pcpoVBauwerke->push_back(pB);
                if (Map() && Map()->Report())
                    Map()->Report()->AddBuilding(h, pB);
            }
            else if (oRS.GetValue() == "SCHIFF") {
                CSchiff* pS;
                h = oRS.GetDat(0);
                if (!m_pcpoSchiffe)
                    m_pcpoSchiffe.reset(new Schiffe());
                m_pcpoSchiffe->insert(Schiffe::value_type(h, pS = new CSchiff(oRS, nRunde)));
                if (!m_pcpoVSchiffe)
                    m_pcpoVSchiffe.reset(new VSchiffe());
                m_pcpoVSchiffe->push_back(pS);
                if (Map() && Map()->Report())
                    Map()->Report()->AddShip(h, pS);
            }
            else if (oRS.GetValue() == "GRENZE") {
                CGrenze* pG;
                //				h = oRS.GetDat(0);
                pG = new CGrenze(oRS);
                m_cpoGrenzen.insert(Grenzen::value_type(pG->Richtung(), pG));
                m_cpoVGrenzen.push_back(pG);
            }
            else if (oRS.GetValue() == "EFFECTS") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enSTRING) {
                    m_coEffects.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "EINHEIT") {
                CEinheit* pE;
                h = oRS.GetDat(0);
                m_cpoEinheiten.insert(Einheiten::value_type(h, pE = new CEinheit(oRS, this)));
                m_cpoVEinheiten.push_back(pE);
                m_poMap->m_poReport->m_cpoGEinheiten.insert(Einheiten::value_type(h, pE));

                if (pE->Partei() == m_poMap->m_poReport->Partei() && pE->Bauwerk()) {
                    CBauwerk* pBW = GetBuilding(pE->Bauwerk());
                    if (pBW && (pBW->Besitzer() == pE->Nummer())) {
                        m_nAusgaben += pBW->Unterhalt();
                    }
                }
            }
            else if (oRS.GetValue() == "DURCHREISE") {
                oRS.Next();
                do {
                    if (oRS.GetType() == CReportStream::enSTRING)
                        m_coDurchreisen.push_back(oRS.GetValue());
                    else
                        break;
                    oRS.Next();
                } while (!oRS.EOS() && oRS.GetType() == CReportStream::enSTRING);
            }
            else if (oRS.GetValue() == "DURCHSCHIFFUNG") {
                oRS.Next();
                do {
                    if (oRS.GetType() == CReportStream::enSTRING)
                        m_coDurchschiffungen.push_back(oRS.GetValue());
                    else
                        break;
                    oRS.Next();
                } while (!oRS.EOS() && oRS.GetType() == CReportStream::enSTRING);
            }
            else if (oRS.GetValue() == "BATTLESPEC") {
                oRS.Next();
                while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK))
                    oRS.Next();
            }
            else if (oRS.GetValue() == "MESSAGE") {
                CMessage::Ptr pMsg(new CMessage(oRS, m_nRunde));
                //                m_cpoMessages.push_back( pMsg );
                Map()->Report()->AddMessage(pMsg);
                if (IsEqual(Map()->Report()->m_sSpiel, "eressea") && pMsg->GetValue("type").asLong() == 1638122429)
                    m_bVerorkt = true;
                pMsg->SetValue(std::string("localmsg"), Value(1));
                pMsg->SetValue(std::string("region"), Value(int32_t(GetEX())));
                pMsg->SetValue(std::string("region:1"), Value(int32_t(GetEY())));
                if (GetEZ())
                    pMsg->SetValue(std::string("region:2"), Value(int32_t(GetEZ())));
                /*
                                oRS.Next();
                                                while( !oRS.EOS() && !oRS.GetType()==CReportStream::enBLOCK )
                                                {
                                                        if( oRS.GetType()==CReportStream::enSTRING && IsEqual( oRS.GetComment(), "rendered" ) )
                                                        {
                                                                m_coBotschaften.push_back( oRS.GetValue() );
                                                        }
                                                        oRS.Next();
                                                }
                */
            }
            else {
                if (CHierarchy::IsChild("REGION", oRS.GetValue())) {
                    LoadSubObject(oRS);
                }
                else {
                    ERRMSG(0, ("Line %d, Warnung: Unbekannter Block: %s", oRS.GetLine(), oRS.GetValue().c_str()));
                    oRS.Next();
                    while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK))
                        oRS.Next();
                }
            }
        }
    } while (!oRS.EOS());

    for (Einheiten::iterator ei = m_cpoEinheiten.begin(); ei != m_cpoEinheiten.end(); ei++) {
        if ((*ei).second->Schiff() && GetShip((*ei).second->Schiff()) && !GetShip((*ei).second->Schiff())->CRKap()) {
            GetShip((*ei).second->Schiff())->AddWeight((int32_t)((*ei).second->Gewicht() + 0.9901));
        }
    }

    //	int nPers = 0;
    CBauwerk* poBW;
    CSchiff* poSH;

    for (size_t j = 0; j < m_cpoVEinheiten.size(); j++) {
        if (m_cpoVEinheiten[j]->m_nBauwerk) {
            poBW = GetBuilding(m_cpoVEinheiten[j]->m_nBauwerk);
            if (poBW) {
                m_cpoVEinheiten[j]->m_nPlace = poBW->Insassen() + 1;
                poBW->AddInsassen(m_cpoVEinheiten[j]->Anzahl());
            }
        }
        else if (m_cpoVEinheiten[j]->m_nSchiff) {
            poSH = GetShip(m_cpoVEinheiten[j]->m_nSchiff);
            if (poSH) {
                m_cpoVEinheiten[j]->m_nPlace = poSH->Insassen() + 1;
                poSH->AddInsassen(m_cpoVEinheiten[j]->Anzahl());
            }
        }
    }

    if (!m_poTerrain)
        m_poTerrain = FindTerrain("");

    if (m_poTerrain->m_bLand)
        m_nInsel = GetKey();
    else {
        m_nBauern = 0;
        m_nBaeume = 0;
        m_nPferde = 0;
        m_nBaeume = 0;
        m_nMallorn = 0;
        m_nEisen = 0;
        m_nLaen = 0;
        m_nSilber = 0;
        m_nUnterhalt = 0;
        m_nRekruten = 0;
    }
}

CRegion::~CRegion()
{
    if (m_pcpoBauwerke) {
        for (Bauwerke::iterator bi = m_pcpoBauwerke->begin(); bi != m_pcpoBauwerke->end(); bi++) {
            delete (*bi).second;
        }
        m_pcpoBauwerke->clear();
    }

    if (m_pcpoSchiffe) {
        for (Schiffe::iterator si = m_pcpoSchiffe->begin(); si != m_pcpoSchiffe->end(); si++) {
            delete (*si).second;
        }
        m_pcpoSchiffe->clear();
    }

    for (Einheiten::iterator ei = m_cpoEinheiten.begin(); ei != m_cpoEinheiten.end(); ei++) {
        delete (*ei).second;
    }
    m_cpoEinheiten.clear();

    for (Grenzen::iterator gi = m_cpoGrenzen.begin(); gi != m_cpoGrenzen.end(); gi++) {
        delete (*gi).second;
    }
    m_cpoGrenzen.clear();

    for (Resourcen::iterator ri = m_cpoResourcen.begin(); ri != m_cpoResourcen.end(); ri++) {
        delete (*ri).second;
    }
    m_cpoResourcen.clear();
}

// 0: Nur Typ
// 2: Der Name ist bekannt
// 5: Einheiten sind hier
// 8: Report enth�lt Eisen-Info
// 10: Report enth�lt Laen-Info
int CRegion::GetQuality() const
{
    int q = (Runde() - CurrentRound()) * 100;
    if (m_nLaen >= 0 && !m_cpoEinheiten.empty())
        return q + 10;
    if (m_nEisen >= 0 && !m_cpoEinheiten.empty())
        return q + 8;
    if (!m_cpoEinheiten.empty())
        return q + 5;
    if (!m_sName.empty())
        return q + 2;
    return q;
}

void CRegion::Write(CReportStream& oRS)
{
    if (m_poTerrain->m_sName.empty())
        return;
    if (m_nZ)
        oRS.WriteBlock("REGION", "", m_nX, m_nY, m_nZ);
    else
        oRS.WriteBlock("REGION", "", m_nX, m_nY);
    if (m_nRunde)
        oRS.WriteLine(m_nRunde, "Runde");
    if (m_nPartei)
        oRS.WriteLine(m_nPartei, "Partei");
    if (!m_sName.empty())
        oRS.WriteLine(m_sName, "Name");
    oRS.WriteLine(m_poTerrain->m_sName, "Terrain");
    if (m_nBauern >= 0)
        oRS.WriteLine(m_nBauern, "Bauern");
    if (!m_sBeschreibung.empty())
        oRS.WriteLine(m_sBeschreibung, "Beschr");
    if (m_nPferde >= 0)
        oRS.WriteLine(m_nPferde, "Pferde");
    if (m_nBaeume >= 0)
        oRS.WriteLine(m_nBaeume, "Baeume");
    if (m_nMallorn >= 0)
        oRS.WriteLine(m_nMallorn, "Mallorn");
    if (m_nEisen > -2)
        oRS.WriteLine(m_nEisen, "Eisen");
    if (m_nLaen > -2)
        oRS.WriteLine(m_nLaen, "Laen");
    if (m_nSilber >= 0)
        oRS.WriteLine(m_nSilber, "Silber");
    if (m_nUnterhalt >= 0)
        oRS.WriteLine(m_nUnterhalt, "Unterh");
    if (m_nRekruten >= 0)
        oRS.WriteLine(m_nRekruten, "Rekruten");
    if (m_nLohn >= 0)
        oRS.WriteLine(m_nLohn, "Lohn");
    if (m_nVerkauf >= 0) {
        oRS.WriteBlock("PREISE");
        for (size_t preis = 0; preis < m_coLuxusgueter.size(); preis++) {
            oRS.WriteLine(m_coLuxusgueter[preis].second, m_coLuxusgueter[preis].first);
        }
    }

    if (m_pcpoBauwerke) {
        for (Bauwerke::iterator bi = m_pcpoBauwerke->begin(); bi != m_pcpoBauwerke->end(); bi++) {
            (*bi).second->Write(oRS);
        }
    }

    if (m_pcpoSchiffe) {
        for (Schiffe::iterator si = m_pcpoSchiffe->begin(); si != m_pcpoSchiffe->end(); si++) {
            (*si).second->Write(oRS);
        }
    }

    for (Einheiten::iterator ei = m_cpoEinheiten.begin(); ei != m_cpoEinheiten.end(); ei++) {
        (*ei).second->Write(oRS);
    }
}

int32_t CRegion::SilverOf(int32_t nPlayer) const
{
    int32_t nSilver = 0;

    for (size_t i = 0; i < m_cpoVEinheiten.size(); i++) {
        if (m_cpoVEinheiten[i]->Partei() == nPlayer) {
            nSilver += m_cpoVEinheiten[i]->Silber();
        }
    }
    return nSilver;
}

int32_t CRegion::PersonsOf(int32_t nPlayer, bool realPersons) const
{
    int32_t nAnzahl = 0;

    for (size_t i = 0; i < m_cpoVEinheiten.size(); i++) {
        if (m_cpoVEinheiten[i]->Partei() == nPlayer && (!realPersons || !m_cpoVEinheiten[i]->m_nVerraeter)) {
            nAnzahl += m_cpoVEinheiten[i]->Anzahl();
        }
    }
    return nAnzahl;
}

bool CRegion::IsGroup(int32_t nGroupID) const
{
    for (size_t i = 0; i < m_cpoVEinheiten.size(); i++) {
        if (m_cpoVEinheiten[i]->GruppenID() == nGroupID) {
            return true;
        }
    }
    return false;
}

int CRegion::GetBonus() const
{
    return m_nBonus;
    //	if( m_nMaxBurg<2 ) return 0;
    //	if( m_nMaxBurg<10 ) return 1;
    //	if( m_nMaxBurg<50 ) return 2;
    //	if( m_nMaxBurg<250 ) return 3;
    //	if( m_nMaxBurg<1250 ) return 4;
    //	return 5;
}

int32_t CRegion::CalcJobs() const
{
    int32_t nJobs = GetRegionKap();
    int32_t nRes;

    if (m_coRImpacts.empty()) {
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1;
        while (true) {
            if (!oCF.FetchLine("Resources", i++, false))
                break;
            m_coRImpacts[DeUmlaut(oCF.GetString(0))] = oCF.GetReal(1);
        }
        m_coRImpacts[DeUmlaut("unknown")] = 0.0;
    }

    if (m_coRImpacts.size() > 1) {
        bool bBaum = false;
        for (ResourceImpacts::const_iterator ri = m_coRImpacts.begin(); ri != m_coRImpacts.end(); ri++) {
            if (IsEqual((*ri).first, "Mallorn") || IsEqual((*ri).first, "Baeume")) {
                if (!bBaum) {
                    bBaum = true;
                    nRes = GetValue((*ri).first).asLong();
                    if (nRes > 0) {
                        nJobs -= (int32_t)(((*ri).second) * nRes);
                    }
                }
            }
            else {
                nRes = GetValue((*ri).first).asLong();
                if (nRes > 0) {
                    nJobs -= (int32_t)(((*ri).second) * nRes);
                }
            }
        }
    }
    else {
        return nJobs - GetValue("Baeume").asLong() * 8 - GetValue("Schoesslinge").asLong() * 4;
    }
    return nJobs;
}

int32_t CRegion::CalcProfit() const
{
    if (m_nBauern > 0) {
        int32_t nArbeit = CalcJobs();  // GetRegionKap() - GetValue( "Baeume" ).asLong()*8 - GetValue( "Schoesslinge" ).asLong()*4;
                                       /*
                                                       if( nArbeit < 0)
                                                       {
                                                               int i = 1123; i++;
                                                       }
                                       */
        return (nArbeit < m_nBauern ? nArbeit : m_nBauern) * (GetBonus() + 11) - m_nBauern * 10;
    }
    return 0;
}

char CRegion::GetRegionChar() const
{
    //	static char RCHARS[]="/.ESDHBGWF";

    if (m_enBlock == enSCHEMEN || m_enBlock == enSPEZIALREGION) {
        if (IsEqual(m_sName, "Ozean"))
            return '.';
        else
            return '?';
    }
    return m_bOwnUnit ? m_poTerrain->m_cMCY : m_poTerrain->m_cMCN;
}

const std::string& CRegion::GetRegionTypeName() const
{
    //	static char* RTNAMES[]={
    //		"Unbekannt","Ozean","Ebene","Sumpf","Wueste","Hochebene","Berg","Gletscher","Wald","Feuerwand"
    //	};
    static std::string sSchemen("Schemen");
    static std::string sSpezial("Nebel");
    if (m_enBlock == enSCHEMEN) {
        return sSchemen;
    }
    if (m_enBlock == enSPEZIALREGION) {
        return sSpezial;
    }
    return m_poTerrain->m_sName;
}

int32_t CRegion::GetRegionKap() const
{
    //	static int32_t RTKAP[]={
    //		0,0,10000,2000,500,4000,1000,100,10000,0
    //	};
    return m_poTerrain->m_nMaxWork;
}

Value CRegion::GetValue(const std::string& sKey) const
{
    char sTmp[2];
    if (IsEqual(sKey.c_str(), "X"))
        return Value(int32_t(m_nX));
    if (IsEqual(sKey.c_str(), "Y"))
        return Value(int32_t(m_nY));
    if (IsEqual(sKey.c_str(), "Z"))
        return Value(int32_t(m_nZ));
    if (IsEqual(sKey.c_str(), "Char")) {
        sTmp[0] = GetRegionChar();
        sTmp[1] = 0;
        return Value(std::string(sTmp));
    }
    if (IsEqual(sKey.c_str(), "Baeume") && GetValue("Mallorn").asLong() > 1) {
        return GetValue("Mallorn");
    }
    if (IsEqual(sKey.c_str(), "Mallorn") && m_nMallorn == 1) {
        return m_nBaeume;
    }
    CResource* pR = GetResource(sKey);
    if (pR) {
        return Value(pR->GetValue("number").asLong());
    }
    if (IsEqual(sKey.c_str(), "Bauern")) {
        return Value(int32_t(m_nBauern));
    }
    if (IsEqual(sKey.c_str(), "Pferde")) {
        return Value(int32_t(m_nPferde));
    }
    if (IsEqual(sKey.c_str(), "Baeume")) {
        return Value(int32_t(m_nBaeume));
    }
    if (IsEqual(sKey.c_str(), "Mallorn")) {
        return Value(int32_t(m_nMallorn));
    }
    if (IsEqual(sKey.c_str(), "Eisen")) {
        return Value(int32_t(m_nEisen));
    }
    if (IsEqual(sKey.c_str(), "Insel")) {
        return Value(CStringDB::SID2Str(m_idInsel));
    }
    if (IsEqual(sKey.c_str(), "Silber")) {
        return Value(int32_t(m_nSilber));
    }
    if (IsEqual(sKey.c_str(), "Unterhalt")) {
        return Value(int32_t(m_nUnterhalt));
    }
    if (IsEqual(sKey.c_str(), "Rekruten")) {
        return Value(int32_t(m_nRekruten));
    }
    if (IsEqual(sKey.c_str(), "Lohn")) {
        return Value(int32_t(m_nLohn));
    }
    if (IsEqual(sKey.c_str(), "Laen")) {
        return Value(int32_t(m_nLaen));
    }
    if (IsEqual(sKey.c_str(), "Strasse")) {
        return Value(int32_t(m_nStrasse));
    }
    if (IsEqual(sKey.c_str(), "Einnahmen")) {
        return Value(int32_t(GetEinkommen()));
    }
    if (IsEqual(sKey.c_str(), "Ausgaben")) {
        return Value(int32_t(GetAusgaben()));
    }
    if (IsEqual(sKey.c_str(), "verorkt")) {
        return Value(int32_t(m_bVerorkt ? 1 : 0));
    }
    if (IsEqual(sKey.c_str(), "arbeitsplaetze")) {
        return Value(int32_t(CalcJobs()));
    }
    return CBlockBase::GetValue(sKey, Value());
}

Value CRegion::DeepGetValue(const std::string& sKey)
{
    Value oVal;
    oVal = GetValue(sKey);
    if (oVal.getType() == VT_EMPTY) {
        RegionDB::iterator rdbi;
        rdbi = g_coRDB.find(GetKey());
        if (rdbi != g_coRDB.end()) {
            RegionSet::iterator rsi = (*rdbi).second.begin();
            while (rsi != (*rdbi).second.end()) {
                oVal = (*rsi)->GetValue(sKey);
                if (oVal.getType() != VT_EMPTY)
                    break;
                rsi++;
            }
            if (oVal.getType() == VT_EMPTY) {
                oVal = Value(0);
            }
        }
    }
    return oVal;
}

const CRegion::CTerrainType* CRegion::FindTerrain(const std::string& sType)
{
    static CTerrainType oUnknown("", '/', '/', 0, false, false, false);
    TerrainTypes::const_iterator ti;

    if (!m_coTerrains.size()) {
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1;
        while (true) {
            if (!oCF.FetchLine("Terrains", i++))
                break;
            m_coTerrains.insert(TerrainTypes::value_type(DeUmlaut(oCF.GetString(0)), CTerrainType(oCF.GetString(0), oCF.GetString(1)[0], oCF.GetString(2)[0], oCF.GetLong(3), oCF.GetLong(4) != 0, oCF.GetLong(5) != 0, oCF.GetLong(6) != 0)));
        }
    }
    ti = m_coTerrains.find(DeUmlaut(sType));
    if (ti == m_coTerrains.end()) {
        if (!sType.empty()) {
            std::string sTemp = Flatten(sType) + "abcdefghijklmnopqrstuvwxyz";
            std::string::const_iterator si = sTemp.begin();
            char rc;
            while (si != sTemp.end()) {
                ti = m_coTerrains.begin();
                while (ti != m_coTerrains.end()) {
                    if (tolower((*ti).second.m_cMCY) == (*si) || tolower((*ti).second.m_cMCN) == (*si))
                        break;
                    ti++;
                }
                if (ti == m_coTerrains.end())
                    break;
                si++;
            }
            if (si == sTemp.end())
                rc = '/';
            else
                rc = (*si);

            ERRMSG(0, ("Warnung: Unbekannter Regionstyp, simuliere Config-Eintrag: \x22%s\x22, \x22%c\x22, \x22%c\x22,  0,  1,  1,  1", sType.c_str(), toupper(rc), rc));
            return &(m_coTerrains.insert(TerrainTypes::value_type(DeUmlaut(sType), CTerrainType(sType, (char)toupper(rc), rc, 0, true, true, true))).first->second);
        }
        return &oUnknown;
    }
    else
        return &(*ti).second;
}

void CRegion::AddMaterialpool(int32_t nPartei, Materialpool& coPool, bool bSearchable)
{
    for (size_t i = 0; i < m_cpoVEinheiten.size(); i++) {
        if (m_cpoVEinheiten[i]->Partei() == nPartei) {
            m_cpoVEinheiten[i]->AddMaterialpool(coPool, bSearchable);
        }
    }
}

/////////////////////////////////////////////////////////////////////
//.class: CBauwerk
/////////////////////////////////////////////////////////////////////

CBauwerk::BuildingTypes CBauwerk::m_coBuildings;

CBauwerk::CBauwerk(CReportStream& oRS, int nRunde)
    : CBlockBase("CBauwerk")
    , m_nGroesse(0)
    , m_nBesitzer(0)
    , m_nPartei(0)
    , m_nUnterhalt(0)
    , m_nBelagerer(0)
    , m_nInsassen(0)
{
    m_nNummer = oRS.GetDat(0);
    oRS.Next();

    SetValue("runde", Value(int32_t(nRunde)));

    do {
        if (oRS.GetType() != CReportStream::enBLOCK) {
            TAGMAP::iterator ti = g_coTags.find(DeUmlaut(oRS.GetComment()));
            if (ti != g_coTags.end()) {
                switch ((*ti).second) {
                    case T_Typ:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sTyp = oRS.GetValue();
                        break;
                    case T_Name:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sName = oRS.GetValue();
                        break;
                    case T_Beschr:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sBeschreibung = oRS.GetValue();
                        break;
                    case T_Groesse:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nGroesse = oRS.GetDat(0);
                        break;
                    case T_Besitzer:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBesitzer = oRS.GetDat(0);
                        break;
                    case T_Partei:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nPartei = oRS.GetDat(0);
                        break;
                    case T_Unterhalt:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nUnterhalt = oRS.GetDat(0);
                        break;
                    case T_Belagerer:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBelagerer = oRS.GetDat(0);
                        break;
                    case T_wahrerTyp:
                        SetValue(oRS);
                        break;
                    default:
                        if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("BURG", oRS.GetComment()))
                            ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                        SetValue(oRS);
                }
            }
            else {
                if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("BURG", oRS.GetComment()))
                    ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                SetValue(oRS);
            }
            oRS.Next();
        }
        else {
            if (oRS.GetValue() == "EFFECTS") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enSTRING) {
                    m_coEffects.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else {
                if (CHierarchy::IsChild("BURG", oRS.GetValue())) {
                    LoadSubObject(oRS);
                }
                else {
                    break;
                }
            }
        }
    } while (!oRS.EOS());

    if (!m_nUnterhalt) {
        m_nUnterhalt = CBlockBase::GetValue(CBuildingInfo::Lookup(Typ()), "unterhalt.silber").asLong();
        if (m_nUnterhalt < 0)
            m_nUnterhalt = m_nGroesse * (-m_nUnterhalt);
    }
}

CBauwerk::~CBauwerk() {}

void CBauwerk::Write(CReportStream& oRS)
{
    oRS.WriteBlock("BURG", "", m_nNummer);
    if (!m_sTyp.empty())
        oRS.WriteLine(m_sTyp, "Typ");
    if (!m_sName.empty())
        oRS.WriteLine(m_sName, "Name");
    if (!m_sBeschreibung.empty())
        oRS.WriteLine(m_sBeschreibung, "Beschr");
    if (m_nGroesse)
        oRS.WriteLine(m_nGroesse, "Groesse");
    if (m_nBesitzer)
        oRS.WriteLine(m_nBesitzer, "Besitzer");
    if (m_nPartei)
        oRS.WriteLine(m_nPartei, "Partei");
    if (m_nUnterhalt)
        oRS.WriteLine(m_nUnterhalt, "Unterhalt");
    if (m_nBelagerer)
        oRS.WriteLine(m_nBelagerer, "Belagerer");
}

std::string CBauwerk::XTyp() const
{
    if (IsEqual(m_sTyp.c_str(), "Burg") && !g_bIsRealBuildingType) {
        CBurgInfo* pBT = CBurgInfo::Lookup(m_nGroesse);
        if (!pBT) {
            if (m_nGroesse == 1)
                return std::string("Grundmauern");
            if (m_nGroesse < 10)
                return std::string("Befestigung");
            if (m_nGroesse < 50)
                return std::string("Turm");
            if (m_nGroesse < 250)
                return std::string("Burg");
            if (m_nGroesse < 1250)
                return std::string("Festung");
            return std::string("Zitadelle");
        }
        else {
            return pBT->GetValue("Name").asString();
        }
    }
    return m_sTyp;
}

/////////////////////////////////////////////////////////////////////
//.class: CSchiff
/////////////////////////////////////////////////////////////////////
CSchiff::ShipTypes CSchiff::m_coShips;

CSchiff::CSchiff(CReportStream& oRS, int nRunde)
    : CBlockBase("CSchiff")
    , m_nAnzahl(1)
    , m_nSchaden(0)
    , m_nProzent(0)
    , m_nKapitaen(0)
    , m_nPartei(0)
    , m_nLadung(0)
    , m_nMaxLadung(0)
    , m_nKueste(-1)
    , m_bCRKap(false)
    , m_nInsassen(0)
{
    m_nNummer = oRS.GetDat(0);
    oRS.Next();

    SetValue("runde", Value(int32_t(nRunde)));

    do {
        if (oRS.GetType() != CReportStream::enBLOCK) {
            TAGMAP::iterator ti = g_coTags.find(DeUmlaut(oRS.GetComment()));
            if (ti != g_coTags.end()) {
                switch ((*ti).second) {
                    case T_Name:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sName = oRS.GetValue();
                        break;
                    case T_Beschr:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sBeschreibung = oRS.GetValue();
                        break;
                    case T_Typ:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sTyp = oRS.GetValue();
                        break;
                    case T_Anzahl:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nAnzahl = oRS.GetDat(0);
                        break;
                    case T_Prozent:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nProzent = oRS.GetDat(0);
                        break;
                    case T_Schaden:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nSchaden = oRS.GetDat(0);
                        break;
                    case T_Kapitaen:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nKapitaen = oRS.GetDat(0);
                        break;
                    case T_Partei:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nPartei = oRS.GetDat(0);
                        break;
                    case T_Ladung:
                        if (oRS.GetType() == CReportStream::enINTEGER) {
                            m_nLadung = oRS.GetDat(0);
                            m_bCRKap = true;
                        }
                        break;
                    case T_MaxLadung:
                        if (oRS.GetType() == CReportStream::enINTEGER) {
                            m_nMaxLadung = oRS.GetDat(0);
                            m_bCRKap = true;
                        }
                        break;
                    case T_Kueste:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nKueste = oRS.GetDat(0);
                        break;
                    case T_cargo:
                        if (oRS.GetType() == CReportStream::enINTEGER) {
                            m_nLadung = (oRS.GetDat(0) + 99) / 100;
                            m_bCRKap = true;
                        }
                        SetValue(oRS);
                        break;
                    case T_capacity:
                        if (oRS.GetType() == CReportStream::enINTEGER) {
                            m_nMaxLadung = (oRS.GetDat(0) + 99) / 100;
                            m_bCRKap = true;
                        }
                        SetValue(oRS);
                        break;
                    case T_Groesse:
                    case T_wahrerTyp:
                        SetValue(oRS);
                        break;
                    default:
                        if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("SCHIFF", oRS.GetComment()))
                            ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                        SetValue(oRS);
                }
            }
            else {
                if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("SCHIFF", oRS.GetComment()))
                    ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                SetValue(oRS);
            }
            oRS.Next();
        }
        else {
            if (oRS.GetValue() == "EFFECTS") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enSTRING) {
                    m_coEffects.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else {
                if (CHierarchy::IsChild("SCHIFF", oRS.GetValue())) {
                    LoadSubObject(oRS);
                }
                else {
                    break;
                }
            }
        }
    } while (!oRS.EOS());

    if (!m_bCRKap) {
        m_nLadung = 0;
    }
}

CSchiff::~CSchiff() {}

void CSchiff::Write(CReportStream& oRS)
{
    oRS.WriteBlock("SCHIFF", "", m_nNummer);
    if (!m_sName.empty())
        oRS.WriteLine(m_sName, "Name");
    if (!m_sBeschreibung.empty())
        oRS.WriteLine(m_sBeschreibung, "Beschr");
    if (!m_sTyp.empty())
        oRS.WriteLine(m_sTyp, "Typ");
    if (m_nProzent)
        oRS.WriteLine(m_nProzent, "Prozent");
    if (m_nSchaden)
        oRS.WriteLine(m_nSchaden, "Schaden");
    if (m_nKapitaen)
        oRS.WriteLine(m_nKapitaen, "Kapitaen");
    if (m_nPartei)
        oRS.WriteLine(m_nPartei, "Partei");
    if (m_nLadung)
        oRS.WriteLine(m_nLadung, "Ladung");
    if (m_nMaxLadung)
        oRS.WriteLine(m_nMaxLadung, "MaxLadung");
    if (m_nKueste >= 0 && m_nKueste <= 5)
        oRS.WriteLine(m_nKueste, "Kueste");
}

int32_t CSchiff::Kapazitaet() const
{
    int32_t nKap = 0;
    if (m_sTyp.empty())
        return 0;
    const CShipType* pST = FindShip(m_sTyp);
    if (pST) {
        nKap = pST->m_nKap;
    }
    else {
        switch (toupper(m_sTyp[0])) {
            case 'B':
                nKap = 50;
                break;
            case 'L':
                nKap = 500;
                break;
            case 'D':
                nKap = 1000;
                break;
            case 'K':
                nKap = 3000;
                break;
            case 'T':
                nKap = 2000;
                break;
            case 'G':
                nKap = 20000;
                break;
        }
    }
    nKap = int32_t(int64_t(nKap * m_nAnzahl) * (100 - m_nSchaden) / 100);
    return nKap;
}

int32_t CSchiff::MaxHolz() const
{
    if (m_sTyp.empty())
        return 0;
    const CShipType* pST = FindShip(m_sTyp);
    return pST->m_nHolz;
}

int32_t CSchiff::Holz() const
{
    return GetValue("groesse").asLong();
}

const CSchiff::CShipType* CSchiff::FindShip(const std::string& sType)
{
    static CShipType oUnknown("", 0, 0);
    ShipTypes::const_iterator ti;
    std::string sName;
    int32_t nKap, nHolz;

    if (!m_coShips.size()) {
        CConfigFile oCF(g_sConfigFile);
        size_t i = 1;
        while (true) {
            if (!oCF.FetchLine("Ships", i++))
                break;
            sName = oCF.GetString(0);
            nKap = oCF.GetLong(1);
            nHolz = oCF.GetLong(5);
            m_coShips.insert(ShipTypes::value_type(DeUmlaut(sName), CShipType(sName, nKap, nHolz)));
        }
    }
    ti = m_coShips.find(DeUmlaut(sType));
    if (ti == m_coShips.end())
        return &oUnknown;
    else
        return &(*ti).second;
}

/////////////////////////////////////////////////////////////////////
//.class: CGrenze
/////////////////////////////////////////////////////////////////////

CGrenze::CGrenze(CReportStream& oRS)
    : CBlockBase("CGrenze")
    , m_nNummer(-1)
    , m_nRichtung(-1)
    , m_nProzent(0)
{
    oRS.Next();

    do {
        if (oRS.GetType() != CReportStream::enBLOCK) {
            //	while( oRS.GetType()!=CReportStream::enBLOCK )
            //	{
            TAGMAP::iterator ti = g_coTags.find(DeUmlaut(oRS.GetComment()));
            if (ti != g_coTags.end()) {
                switch ((*ti).second) {
                    case T_Typ:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sTyp = oRS.GetValue();
                        break;
                    case T_Prozent:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nProzent = oRS.GetDat(0);
                        break;
                    case T_Richtung:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nRichtung = oRS.GetDat(0);
                        break;
                    default:
                        if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("GRENZE", oRS.GetComment()))
                            ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                        SetValue(oRS);
                }
            }
            else {
                if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("GRENZE", oRS.GetComment()))
                    ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                SetValue(oRS);
            }
            oRS.Next();
        }
        else {
            if (oRS.GetValue() == "EFFECTS") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enSTRING) {
                    m_coEffects.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else {
                if (CHierarchy::IsChild("GRENZE", oRS.GetValue())) {
                    LoadSubObject(oRS);
                }
                else {
                    break;
                }
            }
        }
    } while (!oRS.EOS());
}

CGrenze::~CGrenze() {}

void CGrenze::Write(CReportStream& oRS)
{
    if (m_nNummer < 0)
        return;
    oRS.WriteBlock("GRENZE", "", m_nNummer);
    if (!m_sTyp.empty())
        oRS.WriteLine(m_sTyp, "typ");
    if (m_nRichtung)
        oRS.WriteLine(m_nRichtung, "richtung");
    if (m_nProzent)
        oRS.WriteLine(m_nProzent, "prozent");
}

/////////////////////////////////////////////////////////////////////
//.class: CKampfzauber
/////////////////////////////////////////////////////////////////////

CKampfzauber::CKampfzauber(CReportStream& oRS)
    : CBlockBase("KAMPFZAUBER", oRS)
{
}

CKampfzauber::~CKampfzauber() {}

/////////////////////////////////////////////////////////////////////
//.class: CTalentSorter
/////////////////////////////////////////////////////////////////////

bool CTalentSorter::operator()(const CTalent& oT1, const CTalent& oT2) const
{
    return oT1.m_nStufe > oT2.m_nStufe;
}

/////////////////////////////////////////////////////////////////////
//.class: CEinheitenSorter
/////////////////////////////////////////////////////////////////////

bool CEinheitenSorter::operator()(CEinheit* pE1, CEinheit* pE2) const
{
    if (IsFlag(VF_SORTBURGEN)) {
        if (IsFlag(VF_SORTKOMMANDO) && pE1->m_nPlace && (pE1->Aufenthaltsort() == pE2->Aufenthaltsort())) {
            if (pE1->m_nPlace == 1)
                return true;
            if (pE2->m_nPlace == 1)
                return false;
        }
        if (IsFlag(VF_SORTPRIVAT) && (pE1->Aufenthaltsort() == pE2->Aufenthaltsort())) {
            if (pE1->m_sPrivat < pE2->m_sPrivat)
                return true;
            if (pE1->m_sPrivat > pE2->m_sPrivat)
                return false;
        }
        if (IsFlag(VF_SORTTALENTE) && (pE1->Aufenthaltsort() == pE2->Aufenthaltsort())) {
            if (pE1->Talents().size() == 0)
                return false;
            if (pE2->Talents().size() == 0)
                return true;
            return pE1->Talents()[0].m_sTyp < pE2->Talents()[0].m_sTyp;
        }
        return (pE1->Aufenthaltsort() ? pE1->Aufenthaltsort() : 0x10000000) < (pE2->Aufenthaltsort() ? pE2->Aufenthaltsort() : 0x10000000);
    }
    else {
        if (IsFlag(VF_SORTKOMMANDO) && pE1->m_nPlace && (pE1->Aufenthaltsort() == pE2->Aufenthaltsort())) {
            if (pE1->m_nPlace == 1)
                return true;
            if (pE2->m_nPlace == 1)
                return false;
        }
        if (IsFlag(VF_SORTPRIVAT)) {
            if (pE1->m_sPrivat < pE2->m_sPrivat)
                return true;
            if (pE1->m_sPrivat > pE2->m_sPrivat)
                return false;
        }
        if (pE1->Talents().size() == 0)
            return false;
        if (pE2->Talents().size() == 0)
            return true;
        return pE1->Talents()[0].m_sTyp < pE2->Talents()[0].m_sTyp;
    }
}

/////////////////////////////////////////////////////////////////////
//.class: CEinheit
/////////////////////////////////////////////////////////////////////

CEinheit::CEinheit(CReportStream& oRS, CRegion* poRegion)
    : CBlockBase("CEinheit")
    , m_poRegion(poRegion)
    , m_nPlace(0)
    , m_nNummer(0)
    , m_nTemp(0)
    , m_nAlias(0)
    , m_nPartei(-1)
    , m_nVerkleidung(0)
    , m_nAnzahl(1)
    , m_nBauwerk(0)
    , m_nSchiff(0)
    , m_nSilber(0)
    , m_nGruppe(0)
    , m_nKampfStatus(0)
    , m_nBewacht(0)
    , m_nBelagert(0)
    , m_nParteitarnung(0)
    , m_nTarnung(-1)
    , m_nAura(-1)
    , m_nAuramax(-1)
    , m_nHunger(0)
    , m_nVerraeter(0)
    , m_fKapReiten(0.0)
    , m_fFKapReiten(0.0)
    , m_nRHO(-1)
    , m_fKapGehen(0.0)
    , m_fFKapGehen(0.0)
    , m_nGHO(0)
{
    m_nNummer = oRS.GetDat(0);
    oRS.Next();

    do {
        if (oRS.GetType() != CReportStream::enBLOCK) {
            TAGMAP::iterator ti = g_coTags.find(DeUmlaut(oRS.GetComment()));
            if (ti != g_coTags.end()) {
                switch ((*ti).second) {
                    case T_temp:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nTemp = oRS.GetDat(0);
                        break;
                    case T_Name:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sName = oRS.GetValue();
                        break;
                    case T_Beschr:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sBeschreibung = oRS.GetValue();
                        break;
                    case T_Partei:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nPartei = oRS.GetDat(0);
                        break;
                    case T_Anzahl:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nAnzahl = oRS.GetDat(0);
                        break;
                    case T_Typ:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sTyp = oRS.GetValue();
                        break;
                    case T_wahrerTyp:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sWahrerTyp = oRS.GetValue();
                        break;
                    case T_Burg:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBauwerk = oRS.GetDat(0);
                        break;
                    case T_Schiff:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nSchiff = oRS.GetDat(0);
                        break;
                    case T_Silber:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nSilber = oRS.GetDat(0);
                        break;
                    case T_Kampfstatus:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nKampfStatus = oRS.GetDat(0);
                        break;
                    case T_Default:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sDefault = oRS.GetValue();
                        break;
                    case T_Gruppe:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nGruppe = oRS.GetDat(0);
                        break;
                    case T_bewacht:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBewacht = oRS.GetDat(0);
                        break;
                    case T_belagert:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nBelagert = oRS.GetDat(0);
                        break;
                    case T_Parteitarnung:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nParteitarnung = oRS.GetDat(0);
                        break;
                    case T_Tarnung:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nTarnung = oRS.GetDat(0);
                        break;
                    case T_Aura:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nAura = oRS.GetDat(0);
                        break;
                    case T_Auramax:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nAuramax = oRS.GetDat(0);
                        break;
                    case T_Hunger:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nHunger = oRS.GetDat(0);
                        break;
                    case T_Alias:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nAlias = oRS.GetDat(0);
                        break;
                    case T_hp:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_shp = oRS.GetValue();
                        break;
                    case T_privat:
                        if (oRS.GetType() == CReportStream::enSTRING)
                            m_sPrivat = oRS.GetValue();
                        break;
                    case T_Anderepartei:
                    case T_Verkleidung:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nVerkleidung = oRS.GetDat(0);
                        break;
                    case T_Verraeter:
                        if (oRS.GetType() == CReportStream::enINTEGER)
                            m_nVerraeter = oRS.GetDat(0);
                        break;
                    case T_Parteiname:
                    case T_Kapazitaet:
                    case T_Ladung:
                        //                        SetValue( oRS );
                        break;
                    case T_typprefix:
                    case T_unaided:
                    case T_ejcOrdersConfirmed:
                    case T_folgt:
                    case T_hero:
                    case T_weight:
                        SetValue(oRS);
                        break;
                    default:
                        if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("EINHEIT", oRS.GetComment()))
                            ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                        SetValue(oRS);
                }
            }
            else {
                if (!IsFlag(VF_SUPPRESSKEYWARN) && !AdditionalTag::Check("EINHEIT", oRS.GetComment()))
                    ERRMSG(0, ("Line %d, Warnung: Unbekannte Feldkennung: %s", oRS.GetLine(), oRS.GetComment().c_str()));
                SetValue(oRS);
            }
            oRS.Next();
        }
        else {
            /*if( oRS.GetValue()=="EINHEIT" ||
    oRS.GetValue()=="REGION" ||
    oRS.GetValue()=="DURCHREISEREGION" ||
    oRS.GetValue()=="SCHEMEN" ||
    oRS.GetValue()=="SPEZIALREGION" ||
    oRS.GetValue()=="MESSAGETYPE" ||
    oRS.GetValue()=="MESSAGETYPES" ||
    oRS.GetValue()=="TRANSLATION" )
            {
                    break;
            }
            else*/
            if (oRS.GetValue() == "COMMANDS") {
                oRS.Next();
                std::string cmd;
                while (oRS.GetType() == CReportStream::enSTRING) {
                    cmd = oRS.GetValue();
                    m_csKommandos.push_back(oRS.GetValue());
                    if (IsFlag(VF_FULLCOMMANDOUTPUT) && !cmd.empty() && cmd[0] == '/')
                        m_csMetaOut.push_back(oRS.GetValue());
                    m_cnKomLines.push_back(oRS.GetLine());
                    oRS.Next();
                }
                m_csMetaOut.changed(false);
            }
            else if (oRS.GetValue() == "EFFECTS") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enSTRING) {
                    m_coEffects.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "TALENTE") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enINTEGER) {
                    m_coTalente.push_back(CTalent(oRS.GetComment(), oRS.GetDat(0), oRS.GetDat(1), oRS.GetDat(2)));
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "GEGENSTAENDE" || oRS.GetValue() == "GEGENST\xC4NDE") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enINTEGER) {
                    if (!m_nSilber && IsEqual(oRS.GetComment(), "Silber"))
                        m_nSilber = oRS.GetDat(0);
                    m_coGegenstaende.push_back(CGegenstand(oRS.GetComment(), oRS.GetDat(0)));
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "SPRUECHE" || oRS.GetValue() ==
                                                         "SPR\xDC"
                                                         "CHE") {
                oRS.Next();
                while (!oRS.EOS() && !(oRS.GetType() == CReportStream::enBLOCK)) {
                    m_coSprueche.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "EINHEITSBOTSCHAFTEN") {
                oRS.Next();
                while (oRS.GetType() == CReportStream::enSTRING) {
                    m_coBotschaften.push_back(oRS.GetValue());
                    oRS.Next();
                }
            }
            else if (oRS.GetValue() == "KAMPFZAUBER") {
                m_cpoKampfzauber.push_back(CKampfzauber::Ptr(new CKampfzauber(oRS)));
            }
            else {
                if (CHierarchy::IsChild("EINHEIT", oRS.GetValue())) {
                    LoadSubObject(oRS);
                }
                else {
                    break;
                }
                /*
                                ERRMSG( 0, ( "Line %d, Warnung: Unbekannter Block: %s", oRS.GetLine(), oRS.GetValue().c_str() ));
                                oRS.Next();
                                while( !oRS.EOS() && !oRS.GetType()==CReportStream::enBLOCK ) oRS.Next();
                */
            }
        }
    } while (!oRS.EOS());

    if (m_csKommandos.empty() && !m_sDefault.empty())
        m_csKommandos.push_back(m_sDefault);
    if (m_nPartei && m_nPartei == m_poRegion->Map()->Report()->Partei())
        m_poRegion->hasOwnUnit(true);

    std::stable_sort(m_coTalente.begin(), m_coTalente.end(), CTalentSorter(0));
}

CEinheit::~CEinheit() {}

void CEinheit::Write(CReportStream& oRS)
{
    oRS.WriteBlock("EINHEIT", "", m_nNummer);
    if (!m_sName.empty())
        oRS.WriteLine(m_sName, "Name");
    if (!m_sBeschreibung.empty())
        oRS.WriteLine(m_sBeschreibung, "Beschr");
    if (m_nPartei >= 0)
        oRS.WriteLine(m_nPartei, "Partei");
    if (m_nAnzahl != 1)
        oRS.WriteLine(m_nAnzahl, "Anzahl");
    if (!m_sTyp.empty())
        oRS.WriteLine(m_sTyp, "Typ");
    if (!m_sWahrerTyp.empty())
        oRS.WriteLine(m_sTyp, "wahrerTyp");
    if (m_nBauwerk)
        oRS.WriteLine(m_nBauwerk, "Burg");
    if (m_nSchiff)
        oRS.WriteLine(m_nSchiff, "Schiff");
    if (m_nSilber)
        oRS.WriteLine(m_nSilber, "Silber");
    if (m_nBelagert)
        oRS.WriteLine(m_nBelagert, "belagert");
    if (m_nBewacht)
        oRS.WriteLine(m_nBewacht, "bewacht");
    else if (m_nKampfStatus >= 0)
        oRS.WriteLine(m_nKampfStatus, "Kampfstatus");
    if (m_nParteitarnung)
        oRS.WriteLine(m_nParteitarnung, "Parteitarnung");
    if (m_nTarnung >= 0)
        oRS.WriteLine(m_nTarnung, "Tarnung");

    if (!m_csKommandos.empty()) {
        oRS.WriteBlock("COMMANDS");
        for (VKommandos::iterator ki = m_csKommandos.begin(); ki != m_csKommandos.end(); ki++) {
            if (!(*ki).asString().empty())
                oRS.WriteLine((*ki).asString());
        }
    }

    if (!m_coTalente.empty()) {
        oRS.WriteBlock("TALENTE");
        for (Talente::iterator ti = m_coTalente.begin(); ti != m_coTalente.end(); ti++) {
            oRS.WriteLine((*ti).m_nTage, (*ti).m_nStufe, (*ti).m_sTyp);
        }
    }

    if (!m_coGegenstaende.empty()) {
        oRS.WriteBlock("GEGENSTAENDE");
        for (Gegenstaende::iterator gi = m_coGegenstaende.begin(); gi != m_coGegenstaende.end(); gi++) {
            oRS.WriteLine((*gi).second, (*gi).first);
        }
    }
}

// 0: Nur Name und Nummer
// 1: Name, Nummer, Partei
// 2: Name, Nummer, Partei, Talente
// 3: Name, Nummer, Partei, Talentwerte
// 10: Parteiinfo aus Report der Partei
int CEinheit::GetQuality() const
{
    int q = (Region()->Runde() - CRegion::CurrentRound()) * 100;
    if (m_poRegion->Map()->Report()->Partei() == m_nPartei)
        return q + 10;
    if (m_coTalente.size())
        return q + 2;
    if (m_nPartei >= 0)
        return q + 1;
    return q;
}

/*
inline double RundeGewicht( double w )
{
    if( w>0 )
        return double(int64_t((w+0.0005)*1000))/1000.0f;
    else
        return double(int64_t((w-0.0005)*1000))/1000.0f;
}
*/

void CEinheit::CalcKapazitaeten(double& fKapReiten, double& fFKapReiten, int32_t& nRHO, double& fKapGehen, double& fFKapGehen, int32_t& nGHO) const
{
    if (m_nRHO >= 0) {
        fKapReiten = m_fKapReiten;
        fFKapReiten = m_fFKapReiten;
        nRHO = m_nRHO;
        fKapGehen = m_fKapGehen;
        fKapGehen = m_fFKapGehen;
        nGHO = m_nGHO;
    }

    if (ExistUserFunction("CalcUnitCapacities")) {
        Value oVErg;
        ArgumentList coArgs;
        coArgs.push_back(Value(itoan(m_nNummer, m_poRegion->Map()->Report()->ENrBase())));
        if (DoUserFunction(std::string("CalcUnitCapacities"), coArgs, &oVErg)) {
            if (oVErg.getType() != VT_VECTOR || oVErg.size() != 6) {
                ERRMSG(0, ("FEHLER: '#func CalcUnitCapacities' muss ein Array mit sechs Werten liefern!"));
            }
            else {
                fKapReiten = oVErg.getAt(Value(0)).asReal();
                fFKapReiten = oVErg.getAt(Value(1)).asReal();
                nRHO = oVErg.getAt(Value(2)).asLong();
                fKapGehen = oVErg.getAt(Value(3)).asReal();
                fFKapGehen = oVErg.getAt(Value(4)).asReal();
                nGHO = oVErg.getAt(Value(5)).asLong();

                m_fKapReiten = fKapReiten;
                m_fFKapReiten = fFKapReiten;
                m_nRHO = nRHO;
                m_fKapGehen = fKapGehen;
                m_fKapGehen = fFKapGehen;
                m_nGHO = nGHO;
                return;
            }
        }
    }

    double fPersGew;
    double fPersKap;
    double fWagenGewicht;
    double fWagenKapazitaet;
    double fPferdGewicht;
    double fPferdKapazitaet;
    fPersGew = CRasse::Lookup(RealType()).GetValue(std::string("Gewicht")).asReal();
    if (fPersGew < 0.001)
        fPersGew = RealType() == "Trolle" ? 20.0 : 10.0;
    fPersKap = CRasse::Lookup(RealType()).GetValue(std::string("Kapazit\xE4t")).asReal();
    if (fPersKap < 0.001)
        fPersKap = RealType() == "Trolle" ? 10.8 : 5.4;
    fWagenGewicht = CGegenstandsInfo::Lookup("Wagen").GetValue("Gewicht").asReal();
    if (fWagenGewicht < 0.001)
        fWagenGewicht = 40.0;
    fWagenKapazitaet = CGegenstandsInfo::Lookup("Wagen").GetValue("Kapazit\xE4t").asReal();
    if (fWagenKapazitaet < 0.001)
        fWagenKapazitaet = 140.0;
    fPferdGewicht = CGegenstandsInfo::Lookup("Pferd").GetValue("Gewicht").asReal();
    if (fPferdGewicht < 0.001)
        fPferdGewicht = 50.0;
    fPferdKapazitaet = CGegenstandsInfo::Lookup("Pferd").GetValue("Kapazit\xE4t").asReal();
    if (fPferdKapazitaet < 0.001)
        fPferdKapazitaet = 20.0;

    bool bVerdanon = IsEqual(m_poRegion->Map()->Report()->Spiel(), "Verdanon");
    int nWagen = ((CEinheit*)this)->GetValue(std::string("Wagen"), std::string("")).asLong();
    int nPferde = ((CEinheit*)this)->GetValue(std::string("Pferd"), std::string("")).asLong();
    int nTReiten = ((CEinheit*)this)->GetValue(std::string("Reiten"), std::string("Stufe")).asLong();
    int nReiten = nTReiten * m_nAnzahl;

    int nPferdePerWagen = int(fWagenGewicht / fPferdKapazitaet);
    int nErlPferdeReiten = nReiten * 2;
    int nErlPferdeGehen = nReiten * (bVerdanon ? 3 : 4) + m_nAnzahl;
    int nErlWagenGehen = nReiten * 2;
    int nPferdGezogeneWagen;
    int nTrollGezogeneWagen;
    int nZuvielWagen;
    int nLaufendePersonen;
    double fTransportgut;
    double fKapazitaet, fFreieKapaz;
    double fGewicht = Gewicht();

    fKapReiten = 0.0;
    fFKapReiten = 0.0;
    nRHO = 0;
    fKapGehen = 0.0;
    fFKapGehen = 0.0;
    nGHO = 0;

    //	if( m_poRegion->Map()->Report()->Version()<20 || IsEqual( m_poRegion->Map()->Report()->Spiel(), "Ermpiria" ) )
    //		fPersKap = (RealType()=="Trolle"?10.0:5.0);

    //	if( bVerdanon )
    //		fPersKap = 6.0;

    if (nErlPferdeReiten > nPferde)
        nErlPferdeReiten = nPferde;
    if (nErlPferdeGehen > nPferde)
        nErlPferdeGehen = nPferde;

    fTransportgut = fGewicht - fWagenGewicht * nWagen - fPferdGewicht * nPferde - fPersGew * m_nAnzahl;

    //  RK: %1.1f/%1.1f  GK:
    if (!nPferde || !nErlPferdeReiten) {
        fKapReiten = 0.0;
        nRHO = nPferde;
    }
    else {
        // Reitkapazitaet ermitteln
        if (!nPferdePerWagen) {
            nPferdGezogeneWagen = 0;
        }
        else {
            nPferdGezogeneWagen = nErlPferdeReiten / nPferdePerWagen;  // erlaubte wagen
        }
        if (nPferdGezogeneWagen > nWagen) {
            nPferdGezogeneWagen = nWagen;  // wenn wagenmaximum nicht ausgeschoepft wird
        }
        if (nWagen > nPferdGezogeneWagen) {
            nZuvielWagen = nWagen - nPferdGezogeneWagen;  // nicht gezogene wagen
        }
        else {
            nZuvielWagen = 0;
        }
        fKapazitaet = fWagenKapazitaet * nPferdGezogeneWagen + fPferdKapazitaet * (nErlPferdeReiten - nPferdePerWagen * nPferdGezogeneWagen);  // die wagen sowie die restpferde
        fFreieKapaz = fKapazitaet - fPersGew * m_nAnzahl - fTransportgut - fWagenGewicht * nZuvielWagen;                                       // alles rauf auf wagen und Pferde

        fKapReiten = fKapazitaet;
        fFKapReiten = fFreieKapaz;
        if (nPferde > nErlPferdeReiten) {
            nRHO = nPferde - nErlPferdeReiten;
        }
        else {
            nRHO = 0;
        }
    }

    nTrollGezogeneWagen = 0;
    nLaufendePersonen = m_nAnzahl;
    if (!nPferdePerWagen) {
        nPferdGezogeneWagen = 0;
    }
    else {
        nPferdGezogeneWagen = nErlPferdeGehen / nPferdePerWagen;  // maximal moegliche wagen
    }
    if (nPferdGezogeneWagen > nErlWagenGehen) {
        nPferdGezogeneWagen = nErlWagenGehen;  // es werden nur erlaubte gezogen
    }
    if (nPferdGezogeneWagen > nWagen) {
        nPferdGezogeneWagen = nWagen;  // wenn wagenmaximum nicht ausgeschoepft wird
    }
    if (nWagen > nPferdGezogeneWagen) {
        nZuvielWagen = nWagen - nPferdGezogeneWagen;  // nicht gezogene wagen
        if (CRasse::Lookup(RealType()).GetValue("Wagenzug").asLong()) {
            int h = nLaufendePersonen - (nErlPferdeGehen + nTReiten) / (nTReiten + 1);
            int hh = CRasse::Lookup(RealType()).GetValue("Wagenzug").asLong();
            h = h / hh;
            if (h > 0) {
                if (h > nZuvielWagen) {
                    h = nZuvielWagen;
                }
                nZuvielWagen -= h;
                nLaufendePersonen -= h * CRasse::Lookup(RealType()).GetValue("Wagenzug").asLong();
                nTrollGezogeneWagen = h;
            }
        }
    }
    else {
        nZuvielWagen = 0;
    }
    fKapazitaet = fWagenKapazitaet * (nPferdGezogeneWagen + nTrollGezogeneWagen) + fPferdKapazitaet * (nErlPferdeGehen - nPferdePerWagen * nPferdGezogeneWagen) + fPersKap * nLaufendePersonen;  // die wagen sowie die restpferde und die Leute tragen mit
    fFreieKapaz = fKapazitaet - fTransportgut - fWagenGewicht * nZuvielWagen;                                                                                                                    // alles rauf auf wagen und Pferde

    fKapGehen = fKapazitaet;
    fFKapGehen = fFreieKapaz;
    nGHO = nPferde > nErlPferdeGehen ? nPferde - nErlPferdeGehen : 0;

    m_fKapReiten = fKapReiten;
    m_fFKapReiten = fFKapReiten;
    m_nRHO = nRHO;
    m_fKapGehen = fKapGehen;
    m_fKapGehen = fFKapGehen;
    m_nGHO = nGHO;
}

// Gesammtgewicht, Reitkapazität/frei, Gehkapazität/frei, Ladung
void CEinheit::Kapazitaeten(const std::string& sTarget) const
{
    double fKapReiten, fFKapReiten, fKapGehen, fFKapGehen;
    int32_t nRHO, nGHO;

    COutput::TPrintf(sTarget, "  ; Gew: %sGE", ToString(Gewicht()).c_str());

    CalcKapazitaeten(fKapReiten, fFKapReiten, nRHO, fKapGehen, fFKapGehen, nGHO);

    if (!nRHO && ((CEinheit*)this)->GetValue(std::string("Pferd"), std::string("")).asLong()) {
        COutput::TPrintf(sTarget, "  Reiten: %sGE/%sGE", ToString(fFKapReiten).c_str(), ToString(fKapReiten).c_str());
    }
    if (nGHO) {
        COutput::TPrintf(sTarget, "  (%d Pferd%s zuviel!)", nGHO, (nGHO > 1) ? "e" : "");
    }
    else {
        COutput::TPrintf(sTarget, "  Gehen: %sGE/%sGE", ToString(fFKapGehen).c_str(), ToString(fKapGehen).c_str());
    }
    COutput::TPrintf(sTarget, "\n");
}

double CEinheit::Gewicht() const
{
    Gegenstaende::const_iterator gi;
    GEGENSTANDINFO::const_iterator gii;
    double fGew = 0.0;
    // int nPferde = 0, nWagen = 0;

    fGew = CRasse::Lookup(RealType()).GetValue(std::string("Gewicht")).asReal();
    if (fGew < 0.001)
        fGew = (RealType() == "Trolle" ? 20.0 : 10.0);
    fGew *= m_nAnzahl;
    for (gi = m_coGegenstaende.begin(); gi != m_coGegenstaende.end(); gi++) {
        if (!IsEqual((*gi).first, "Silber")) {
            fGew += CGegenstandsInfo::Lookup((*gi).first).GetValue(std::string("Gewicht")).asReal() * gi->second;
        }
    }
    //	if( m_poRegion->Map()->Report()->Version()>20 || !IsEqual( m_poRegion->Map()->Report()->Spiel(), "Empiria" ) )
    fGew += CGegenstandsInfo::Lookup("Silber").GetValue(std::string("Gewicht")).asReal() * m_nSilber;
    return fGew;
}

Value CEinheit::GetValue(const std::string& sKey, const std::string& sKey2) const
{
    if (IsEqual(sKey.c_str(), "Anzahl")) {
        return Value(int32_t(m_nAnzahl));
    }
    if (IsEqual(sKey.c_str(), "Aura")) {
        return Value(int32_t(m_nAura));
    }
    if (IsEqual(sKey.c_str(), "Auramax")) {
        return Value(int32_t(m_nAuramax));
    }
    if (IsEqual(sKey.c_str(), "Hunger")) {
        return Value(int32_t(m_nHunger));
    }
    if (IsEqual(sKey.c_str(), "Silber")) {
        return Value(int32_t(m_nSilber));
    }
    if (IsEqual(sKey.c_str(), "Partei")) {
        return Value(itoan(m_nPartei, m_poRegion->Map()->Report()->PNrBase()));
    }
    if (IsEqual(sKey.c_str(), "Bauwerk")) {
        if (m_nBauwerk) {
            return Value(itoan(m_nBauwerk, m_poRegion->Map()->Report()->BNrBase()));
        }
        return Value(0);
    }
    if (IsEqual(sKey.c_str(), "Position")) {
        return Value(int32_t(m_nPlace));
    }
    if (IsEqual(sKey.c_str(), "Schiff")) {
        if (m_nSchiff) {
            return Value(itoan(m_nSchiff, m_poRegion->Map()->Report()->BNrBase()));
        }
        return Value(0);
    }
    if (IsEqual(sKey.c_str(), "Bewacht")) {
        return Value(int32_t(m_nBewacht));
    }
    if (IsEqual(sKey.c_str(), "Kampfstatus")) {
        return Value(int32_t(m_nKampfStatus));
    }
    if (IsEqual(sKey.c_str(), "Parteitarnung")) {
        return Value(int32_t(m_nParteitarnung));
    }
    if (IsEqual(sKey.c_str(), "Verraeter")) {
        return Value(int32_t(m_nVerraeter));
    }
    if (IsEqual(sKey.c_str(), "Anderepartei")) {
        return Value(itoan(m_nVerkleidung, m_poRegion->Map()->Report()->PNrBase()));
    }
    if (IsEqual(sKey.c_str(), "Verkleidung")) {
        return Value(itoan(m_nVerkleidung, m_poRegion->Map()->Report()->PNrBase()));
    }
    if (IsEqual(sKey.c_str(), "X")) {
        return Value(int32_t(m_poRegion->GetEX()));
    }
    if (IsEqual(sKey.c_str(), "Y")) {
        return Value(int32_t(m_poRegion->GetEY()));
    }
    if (IsEqual(sKey.c_str(), "privat")) {
        return Value(m_sPrivat);
    }
    if (IsEqual(sKey.c_str(), "beschr")) {
        return Value(m_sBeschreibung);
    }
    if (IsEqual(sKey.c_str(), "parteiname")) {
        //        abort();
        return Value(m_poRegion->Map()->Report()->Parteiname(m_nPartei).substr(1));
    }
    if (IsEqual(sKey.c_str(), "status")) {
        return Value(m_poRegion->Map()->Report()->Parteiname(m_nPartei).substr(0, 1));
    }
    if (IsEqual(sKey.c_str(), "Tarnung") && sKey2.empty()) {
        return Value(int32_t(m_nTarnung));
    }
    if (IsEqual(sKey.c_str(), "temp")) {
        if (m_nTemp) {
            return Value(itoan(m_nTemp, m_poRegion->Map()->Report()->ENrBase()));
        }
        return Value(0);
    }
    if (IsEqual(sKey.c_str(), "alias")) {
        if (m_nAlias) {
            return Value(itoan(m_nAlias, m_poRegion->Map()->Report()->ENrBase()));
        }
        return Value(0);
    }
    if (IsEqual(sKey.c_str(), "hasmetas")) {
        return Value(HasMetas() ? 1 : 0);
    }

    for (Talente::const_iterator ti = m_coTalente.begin(); ti != m_coTalente.end(); ti++) {
        if (IsEqual(sKey.c_str(), (*ti).m_sTyp.c_str())) {
            if (IsEqual(sKey2.c_str(), "Tage") || IsEqual(sKey2.c_str(), "punkte")) {
                if (IsFlag(VF_NOSKILLPOINTS)) {
                    int32_t nBonus = CRasse::Lookup(RealType()).GetValue(sKey).asLong();
                    int32_t nStufe = int32_t((*ti).m_nStufe);
                    return (nStufe - nBonus) * (nStufe - nBonus + 1) * 15 * Anzahl();
                }
                else {
                    return Value(int32_t((*ti).m_nTage));
                }
            }
            if (IsEqual(sKey2.c_str(), "Stufe")) {
                return Value(int32_t((*ti).m_nStufe));
            }
            if (IsFlag(VF_NOSKILLPOINTS) && IsEqual(sKey2.c_str(), "Mod")) {
                return Value(int32_t((*ti).m_nTage));
            }
        }
    }

    for (Gegenstaende::const_iterator gi = m_coGegenstaende.begin(); gi != m_coGegenstaende.end(); gi++) {
        if (IsEqual(sKey.c_str(), (*gi).first.c_str())) {
            return Value(int32_t((*gi).second));
        }
    }

    return CBlockBase::GetValue(sKey, Value(0));
}

void CEinheit::AddMaterialpool(CRegion::Materialpool& coPool, bool bSearchable)
{
    CRegion::Materialpool::iterator mi;

    for (Gegenstaende::iterator gi = m_coGegenstaende.begin(); gi != m_coGegenstaende.end(); gi++) {
        if (bSearchable)
            mi = coPool.find(DeUmlaut((*gi).first));
        else
            mi = coPool.find((*gi).first);
        if (mi == coPool.end()) {
            if (bSearchable)
                coPool.insert(CRegion::Materialpool::value_type(DeUmlaut((*gi).first), (*gi).second));
            else
                coPool.insert(CRegion::Materialpool::value_type((*gi).first, (*gi).second));
        }
        else
            (*mi).second += (*gi).second;
    }
}

CEinheit* CEinheit::GlobalUnit(int32_t nENr)
{
    CReport::Einheiten::iterator i;
    i = m_poRegion->Map()->Report()->GEinheiten().find(nENr);
    if (i != m_poRegion->Map()->Report()->GEinheiten().end())
        return (*i).second;
    return 0;
}

bool CEinheit::HasMetas() const
{
    VKommandos::const_iterator ki;
    for (ki = m_csKommandos.begin(); ki != m_csKommandos.end(); ki++)
        if (CRegExp::Match((*ki).asString(), "//\\s+#"))
            return true;
    return false;
}

std::string CEinheit::PrefixedTyp(bool bWahr) const
{
    std::string sPrefix;
    if (!CBlockBase::GetValue("typprefix", Value("")).asString().empty()) {
        sPrefix = CBlockBase::GetValue("typprefix", Value("")).asString();
    }
    if (sPrefix.empty() && m_nGruppe >= 0 && m_poRegion && m_poRegion->Map()) {
        CGruppe::Ptr pG = m_poRegion->Map()->Report()->GetGruppe(m_nGruppe);
        Value oVal(0);
        if (pG.get())
            sPrefix = pG->GetValue("typprefix", Value("")).asString();
    }
    if (sPrefix.empty() && m_poRegion && m_poRegion->Map()) {
        CPartei::Ptr pP = m_poRegion->Map()->Report()->GetLocalParteiInfo(m_nPartei);
        Value oVal(0);
        if (pP.get())
            sPrefix = pP->GetValue("typprefix", Value("")).asString();
    }
    if (sPrefix.empty()) {
        return bWahr ? WahrerTyp() : Typ();
    }
    return sPrefix + Flatten(bWahr ? WahrerTyp() : Typ());
}
