/****************************************************************************
 *   $Source: D:\\Development\\Repository/ETools/EBase/Report.h,v $
 *   $Author: ssh $
 *     $Date: 2003/07/01 09:39:30 $
 * $Revision: 1.1 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: ERESSEA-Datenklassen inclusive CR-Parser
 *****************************************************************************
 *
 * $Log: Report.h,v $
 * Revision 1.1  2003/07/01 09:39:30  ssh
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/01 09:13:41  ssh
 * Initial recvsing of Source...
 *
 * Revision 1.10  2000/02/24 09:55:53  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.9  1999/11/28 17:38:11  S.Schuemann
 * - Mannigfaltige �nderungen f�r Vorlage V1.4 beta 9
 *
 * Revision 1.8  1999/11/17 08:58:15  S.Schuemann
 * - support f�r multiple CRs
 *
 * - vielfache �nderungen f�r Vorlage 1.4 beta 8
 *
 * Revision 1.7  1999/11/08 11:10:45  S.Schuemann
 * - verbessertes Luxusgut-Handling
 * - Korrektur der Kapazitaetsberechnung
 * - Neue Flags
 *
 * Revision 1.6  1999/11/03 10:21:38  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.5  1999/10/26 13:25:43  S.Schuemann
 * - Anpassungen fuer neue Object-Attribute und Vorlage 1.4 b 5
 *
 * Revision 1.4  1999/10/20 02:22:32  S.Schuemann
 * - Anpassungen der Reportklassen fuer die Features von Vorlage 1.4 b 3
 *
 * Revision 1.3  1999/10/18 21:31:46  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.2  1999/09/27 06:22:31  S.Schuemann
 * - CWorldDB heisst jetzt CReport;
 * - Die CR-Felder "Runde" und "Anzahl Personen" werden unterst�tzt;
 * - CReport::GetValue() fuer das Objekt REPORT implementiert;
 * - Fehler im Kampfstatus behoben;
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/
#pragma once

#include <iostream>

#include "Expression.h"
#include "ReportStream.h"
#include "Value.h"
#include "hierarchy.h"

class CReport;
class CRegion;
class CBauwerk;
class CSchiff;
class CGrenze;
class CEinheit;
class CMessage;

typedef std::vector<CMessage*> MessageVector;
typedef std::map<int32_t, MessageVector> MessagePool;
extern MessagePool g_oMessagePool;

class StringTable
{
public:
    typedef std::map<std::string, int> String2Int;
    typedef std::map<int, std::string> Int2String;

    StringTable() { insert(""); }

    int size() const { return int(_s2i.size()); }

    int insert(const std::string& s)
    {
        int i = s2i(s);
        if (i < 0) {
            i = (int)_i2s.size();
            _s2i[s] = i;
            _i2s[i] = s;
        }
        return i;
    }

    bool known(const std::string& s) const { return s2i(s) != -1; }

    int s2i(const std::string& s) const
    {
        String2Int::const_iterator it = _s2i.find(s);
        return it != _s2i.end() ? it->second : -1;
    }

    const std::string& i2s(int i) const
    {
        static std::string nix("<name unknown>");
        Int2String::const_iterator it = _i2s.find(i);
        return it != _i2s.end() ? it->second : nix;
    }

protected:
    Int2String _i2s;
    String2Int _s2i;
};

class CRegionKey
{
public:
    CRegionKey()
        : m_nX(0)
        , m_nY(0)
        , m_nZ(0)
    {
    }

    CRegionKey(int32_t x, int32_t y, int32_t z)
        : m_nX(x)
        , m_nY(y)
        , m_nZ(z)
    {
    }

    CRegionKey(const CRegionKey&) = default;
    CRegionKey& operator=(const CRegionKey&) = default;
    ~CRegionKey() = default;

    bool operator==(const CRegionKey& oRK) const { return m_nX == oRK.m_nX && m_nY == oRK.m_nY && m_nZ == oRK.m_nZ; }

    bool operator!=(const CRegionKey& oRK) const { return !(*this == oRK); }

    bool operator<(const CRegionKey& oRK) const
    {
        if (m_nX < oRK.m_nX)
            return true;
        else if (m_nX > oRK.m_nX)
            return false;
        if (m_nY < oRK.m_nY)
            return true;
        else if (m_nY > oRK.m_nY)
            return false;
        if (m_nZ < oRK.m_nZ)
            return true;
        return false;
    }

    std::string AsString() const { return ToString(m_nX) + ',' + ToString(m_nY) + ',' + ToString(m_nZ); }

protected:
    int32_t m_nX, m_nY, m_nZ;
};

// typedef std::vector<std::string> VKommandos;
typedef CValArray VKommandos;
typedef std::vector<std::string> Effects;

struct rqcmp
{
    bool operator()(const CRegion* pR1, const CRegion* pR2) const;
};

typedef std::set<CRegion*, rqcmp> RegionSet;
typedef std::map<CRegionKey, RegionSet> RegionDB;
typedef std::map<int32_t, CEinheit*> EinheitenDB;
typedef std::map<CRegionKey, EinheitenDB> RegEinheitenDB;
extern RegionDB g_coRDB;
extern RegionDB::iterator g_iRDB;
extern int32_t g_nRDBIndex;
extern EinheitenDB g_coEDB;
extern EinheitenDB::iterator g_iEDB;
extern int32_t g_nEDBIndex;
extern RegEinheitenDB g_coREDB;

typedef std::map<int32_t, RegionDB> RRegionDB;
typedef std::map<int32_t, EinheitenDB> REinheitenDB;
extern RRegionDB g_coRRegionDB;
extern REinheitenDB g_coREinheitenDB;

class CBlockBase
{
public:
    enum CFGOBJMODE { enTABH, enTABV, enNEST };

    typedef std::shared_ptr<CBlockBase> Ptr;
    typedef std::map<int32_t, Value> NamedValues;
    typedef std::map<std::string, Ptr> BlockGroup;
    typedef std::map<int32_t, BlockGroup> NamedSubblocks;
    CBlockBase(const char* pcType, const char* pcName = 0);
    CBlockBase(const char* pcType, const std::string& sName, const Value& oKey1 = Value(), const Value& oKey2 = Value(), const Value& oKey3 = Value());
    CBlockBase(const char* pcType, CReportStream& oRS);
    virtual ~CBlockBase();
    std::string ID() const;
    void LoadSubObject(CReportStream& oRS);
    void SetValue(CReportStream& oRS);
    void SetValue(const std::string& sName, const Value& oVal);

    size_t NumValues() const { return m_coNamedValues.size(); }

    size_t NumSubblocks() const { return m_coSubblocks.size(); }

    bool HasKeys() const { return m_oKey1.getType() != VT_EMPTY || m_oKey2.getType() != VT_EMPTY || m_oKey3.getType() != VT_EMPTY; }

    Value GetKey(int32_t nIdx) const
    {
        switch (nIdx) {
            case 0:
                return m_oKey1;
            case 1:
                return m_oKey2;
            case 2:
                return m_oKey3;
            default:
                return Value();
        }
    }

    Value GetValue(int32_t nIdx, std::string& sName) const;
    Value GetValue(const std::string& sName, const Value& oDefault = Value(int32_t(0))) const;
    Value GetValue(CObjectPart* pOP, const Value& oDefault = Value(int32_t(0))) const;
    void AddBlock(std::shared_ptr<CBlockBase>);
    std::shared_ptr<CBlockBase> GetBlock(const std::string& sName, bool& bClassFound, const Value& oKey1 = Value(), const Value& oKey2 = Value(), const Value& oKey3 = Value());
    void Dump(unsigned int lvl = 0);

    static Value GetValue(std::shared_ptr<CBlockBase> pBlk, const std::string& sOAS, const Value& oDefault = Value(int32_t(0)));
    static Value GetValue(CBlockBase* pBlk, const std::string& sOAS, const Value& oDefault = Value(int32_t(0)));
    static std::string CalcID(int nName, const Value& oKey1 = Value(), const Value& oKey2 = Value(), const Value& oKey3 = Value());
    static std::string CalcID(const std::string& sName, const Value& oKey1 = Value(), const Value& oKey2 = Value(), const Value& oKey3 = Value());
    static void ReadConfigObjects(const std::string& sFile, const std::string& sCap, CFGOBJMODE enCOM, const std::vector<std::string>& coTitles = std::vector<std::string>());
    static Value GetValue(std::shared_ptr<CBlockBase> pBlk, CObjectPart* pOP, const Value& oDefault = Value(int32_t(0)));
    static Value GetValue(CBlockBase* pBlk, CObjectPart* pOP, const Value& oDefault = Value(int32_t(0)));

    static Value GetCfgObjValue(CObjectPart* pOP, const Value& oDefault = Value(int32_t(0))) { return GetValue(g_poConfigObjects, pOP, oDefault); }

    static Value GetCfgObjValue(const std::string& sOAS, const Value& oDefault = Value(int32_t(0))) { return GetValue(g_poConfigObjects, sOAS, oDefault); }

    static void DumpCfgObjs()
    {
        if (g_poConfigObjects.get())
            g_poConfigObjects->Dump();
    }

private:
    const char* m_pcType;
    int m_nName;
    CHierarchyInfo::ptr m_poHInfo;
    Value m_oKey1, m_oKey2, m_oKey3;
    NamedValues m_coNamedValues;
    NamedSubblocks m_coSubblocks;
    static Ptr g_poConfigObjects;
};

class CGegenstandsInfo : public CBlockBase
{
public:
    CGegenstandsInfo()
        : CBlockBase("CGegenstandsInfo")
    {
    }

    virtual ~CGegenstandsInfo() {}

    static CGegenstandsInfo& Lookup(const std::string& sThing);
};

class CRasse : public CBlockBase
{
public:
    CRasse()
        : CBlockBase("CRasse")
    {
    }

    virtual ~CRasse() {}

    static CRasse& Lookup(const std::string& sRace);
};

class CBurgInfo : public CBlockBase
{
public:
    CBurgInfo()
        : CBlockBase("CBurgInfo")
    {
    }

    virtual ~CBurgInfo() {}

    static CBurgInfo& Lookup(const std::string& sBurg);
    static CBurgInfo* Lookup(int32_t nSize);
    static std::vector<CBurgInfo*> m_vpoBInfos;
};

class CBuildingInfo : public CBlockBase
{
public:
    CBuildingInfo()
        : CBlockBase("CBuildingInfo")
    {
    }

    virtual ~CBuildingInfo() {}

    static CBlockBase::Ptr Lookup(const std::string& sBuilding);
};

class CMessage : public CBlockBase
{
public:
    typedef std::shared_ptr<CMessage> Ptr;

    enum RENDERER { NONE, ERESSEA1, ERESSEA2 };

    CMessage(int32_t round);
    CMessage(const std::string& sRendered, int32_t round);
    CMessage(CReportStream& oRS, int32_t round);

    virtual ~CMessage() {}

    std::string Render(CReport* pRep) const;
    void GetCoords(std::string sTag, int32_t& x, int32_t& y, int32_t& z) const;

    /*
        static void SetRule( int32_t nID, const std::string& sRule )
        {
            m_coMessageTypes[nID] = sRule;
        }
    */
    static void ForceRendering(bool bForce) { m_bForceRender = bForce; }

    static void SelectRenderer(RENDERER enRenderer, std::map<int32_t, std::string>* pRules)
    {
        m_enRenderer = enRenderer;
        m_pcoMessageTypes = pRules;
    }

    static int32_t Messages(int32_t round);
    static CMessage* FindMessage(int32_t round, int32_t idx);

protected:
    void registerMessage(int32_t round);
    std::string Eressea1_Render(CReport* pRep) const;

    std::string Eressea2_Render(CReport* pRep) const;
    std::string E2R_Parse(std::string::const_iterator& iRule, const std::string::const_iterator& iEnd) const;

    static bool m_bForceRender;
    static RENDERER m_enRenderer;
    static std::map<int32_t, std::string>* m_pcoMessageTypes;
};

/*
class CMessage
{
public:
        CMessage( CReportStream& oRS );
    virtual ~CMessage();
    std::string Render( int32_t nID ) const;
    std::string Element( const std::string& sKey ) const;

    static void SetRule( int32_t nID, const std::string& sRule )
    {
        m_coMessageTypes[nID] = sRule;
    }

protected:
    std::map<std::string,std::string> m_coParams;
    static std::map<int32_t,std::string> m_coMessageTypes;
};
*/

class CKarte
{
public:
    friend class CRegion;

    typedef std::map<CRegionKey, CRegion*> RegionMap;
    typedef std::list<CRegion*> IslandQueue;

    CKarte(CReport* poReport);
    CKarte(CReportStream& oRS, CReport* poReport);
    virtual ~CKarte();

    void Import(CReportStream& oRS, int nRunde = 0);
    void Write(CReportStream& oRS);
    void Set(CRegion* poRegion);
    CRegion* GetFromECords(int32_t nX, int32_t nY, int32_t nZ, bool bDeep = false);
    CRegion* GetFromDCords(int32_t nX, int32_t nY, int32_t nZ, bool bDeep = false);

    int32_t GetCX() const { return m_nCX; }

    int32_t GetCY() const { return m_nCY; }

    void SetCenter(int nCX, int nCY)
    {
        m_nCX = nCX;
        m_nCY = nCY;
    }

    int32_t GetCursorX() const { return m_nCursorX; }

    int32_t GetCursorY() const { return m_nCursorY; }

    int32_t GetCursorEX() const { return m_nCursorX - m_nCursorY / 2 - ((m_nCursorY > 0) && (m_nCursorY & 1) ? 1 : 0); }

    int32_t GetCursorEY() const { return m_nCursorY; }

    void SetCursor(int32_t nCursorX, int32_t nCursorY)
    {
        m_nCursorX = nCursorX;
        m_nCursorY = nCursorY;
    }

    CReport* Report() { return m_poReport; }

    RegionMap& Regions() { return m_cpoRegions; }

    std::vector<CRegion*>& VRegions() { return m_cpoVRegions; }

    int Islandize();
    void FillIslandQueue(IslandQueue& cpoQueue);

    void DumpMap(const std::string& sTarget, int32_t nCX, int32_t nCY, int32_t nB, int32_t nH, const char* pcPref = NULL);
    void DumpFullMap(const std::string& sTarget, const char* pcPref = NULL);
    void DumpWorldMap(const std::string& sTarget, const char* pcPref = NULL);

    static CRegionKey GetIsland(int32_t x, int32_t y, int32_t z);

protected:
    CRegionKey CalcMapKey(int32_t nX, int32_t nY, int32_t nZ);
    bool m_bMap;
    int32_t m_nCX;
    int32_t m_nCY;
    int32_t m_nCursorX;
    int32_t m_nCursorY;
    int32_t m_nLeft, m_nRight;
    int32_t m_nTop, m_nBottom;
    CReport* m_poReport;
    std::vector<CRegion*> m_cpoVRegions;
    RegionMap m_cpoRegions;
    static int32_t m_nWLeft, m_nWRight;
    static int32_t m_nWTop, m_nWBottom;
    static bool m_bWMap;
};

class CParteihandel
{
public:
    typedef std::map<std::string, int> Produkte;
    Produkte m_coProdukte;
};

class CTradeInfo
{
public:
    CTradeInfo(int32_t nUnit, int nBuilding, int nX, int nY, int nZ, int nSilber)
        : m_nUnit(nUnit)
        , m_nBuilding(nBuilding)
        , m_nX(nX)
        , m_nY(nY)
        , m_nZ(nZ)
        , m_nSilber(nSilber)
    {
    }

    int32_t m_nUnit;
    int m_nBuilding;
    int m_nX, m_nY, m_nZ;
    int m_nSilber;
};

class CPartei : public CBlockBase
{
public:
    CPartei(int32_t nPartei)
        : CBlockBase("CPartei", "PARTEI", Value(nPartei))
    {
    }

    virtual ~CPartei() {}

    void SetAllianz(int32_t nPartei, int32_t nStatus) { m_cnAllianzen[nPartei] = nStatus; }

    int32_t GetAllianz(int32_t nPartei) const
    {
        std::map<int32_t, int32_t>::const_iterator i = m_cnAllianzen.find(nPartei);
        if (i != m_cnAllianzen.end())
            return (*i).second;
        return 0;
    }

protected:
    std::map<int32_t, int32_t> m_cnAllianzen;
};

class CGruppe : public CBlockBase
{
public:
    CGruppe(int32_t nGruppe)
        : CBlockBase("CGruppe", "GRUPPE", Value(nGruppe))
    {
    }

    virtual ~CGruppe() {}

    void SetAllianz(int32_t nPartei, int32_t nStatus) { m_cnAllianzen[nPartei] = nStatus; }

    int32_t GetAllianz(int32_t nPartei) const
    {
        std::map<int32_t, int32_t>::const_iterator i = m_cnAllianzen.find(nPartei);
        if (i != m_cnAllianzen.end())
            return (*i).second;
        return 0;
    }

protected:
    std::map<int32_t, int32_t> m_cnAllianzen;
};

class CReport : public CBlockBase
{
public:
    friend class CRegion;
    friend class CVorlage;

    typedef std::map<size_t, CMessage::Ptr> Messages;
    typedef std::pair<int, std::string> Nachricht;
    typedef std::map<int32_t, std::string> Parteien;
    typedef std::map<int32_t, CPartei::Ptr> ParteiInfos;
    typedef std::map<int32_t, CGruppe::Ptr> Gruppen;
    typedef std::list<Nachricht> Nachrichten;
    typedef std::map<int32_t, CEinheit*> Einheiten;
    typedef std::map<int32_t, CBauwerk*> Bauwerke;
    typedef std::map<int32_t, CSchiff*> Schiffe;
    typedef std::list<CReport*> Reports;
    typedef std::map<int32_t, CParteihandel*> Handelspartner;
    typedef std::list<CTradeInfo> TradeInfos;
    typedef std::vector<std::string> Optionen;

    enum MSGTYPE { MT_UNKNOWN, MT_EREIGNISSE, MT_HANDEL, MT_EINKOMMEN };

    CReport(const std::string& sFName = "");
    ~CReport();

    bool IsUtf8() const { return m_bUTF8; }

    bool IsValid() const { return m_bIsValid; }

    void Import(const std::string& sFName);
    void Write(const std::string& sFName);

    CKarte* GetMap() { return m_poMap; }

    int32_t Version() const { return m_nVersion; }

    int32_t Partei() const { return m_nPartei; }

    int32_t Runde() const { return m_nRunde; }

    CMessage::RENDERER MessageRenderer() const { return m_enMsgRenderer; }

    std::map<int32_t, std::string>* MessageRules() { return &m_coMessageTypes; }

    std::map<int32_t, std::string>* MessageSections() { return &m_coMessageSections; }

    void SetMessageRenderer(CMessage::RENDERER enRenderer) { m_enMsgRenderer = enRenderer; }

    void SetMessageRule(int32_t nID, const std::string& sRule) { m_coMessageTypes[nID] = sRule; }

    void SetMessageSection(int32_t nID, const std::string& sSection) { m_coMessageSections[nID] = sSection; }

    int Zeitalter() const { return m_nZeitalter; }

    int32_t Jahr() const
    {
        if (Zeitalter() == 1)
            return (Runde() - 1) / 12 + 1;
        else
            return (Runde() - 184) / 27 + 1;
    }

    int32_t Monat() const
    {
        if (Zeitalter() == 1)
            return (Runde()) % 12 ? (Runde()) % 12 : 12;
        else
            return ((Runde() - 184) / 3) % 9 ? ((Runde() - 184) / 3) % 9 : 9;
    }

    int32_t Woche() const
    {
        if (Zeitalter() == 1)
            return 1;
        else
            return (Runde() - 184) % 3 + 1;
    }

    int ENrBase() const { return m_nENrBase; }

    int PNrBase() const { return m_nPNrBase; }

    int BNrBase() const { return m_nBNrBase; }

    int32_t Rekrutierungskosten() const { return m_nRekrutierungskosten; }

    std::string FileName() const { return m_sOrgCRName; }

    std::string Spiel() const { return m_sSpiel; }

    bool HasSpiel() const { return m_bHasSpiel; }

    std::string Konfiguration() const { return m_sKonfiguration; }

    std::string Parteiname(int32_t nPNr = -2) const
    {
        switch (nPNr) {
            case -2:
                return m_sParteiname;
            case -1:
                return std::string("-parteigetarnt");
            case 0:
                return std::string("-Monster");
        }
        Parteien::const_iterator pi = m_coParteien.find(nPNr);
        return pi == m_coParteien.end() ? std::string("-unbekannt") : (*pi).second;
    }

    std::string Passwort() const { return m_sPasswort; }

    std::string Gruppe(int32_t nID)
    {
        CGruppe::Ptr pG = GetGruppe(nID);
        std::string sGruppe;
        if (pG.get())
            sGruppe = pG->GetValue("name", Value("")).asString();
        return sGruppe;
    }

    CKarte* Karte() { return m_poMap; }

    Einheiten& GEinheiten() { return m_cpoGEinheiten; }

    CEinheit* SearchUnit(int32_t nENr, bool bDeep = true);
    Value GetValue(const std::string& sKey);

    void AddBuilding(int32_t nID, CBauwerk* pB) { m_cpoBauwerke[nID] = pB; }

    CBauwerk* GetBuilding(int32_t nID)
    {
        Bauwerke::iterator bi = m_cpoBauwerke.find(nID);
        return bi == m_cpoBauwerke.end() ? 0 : (*bi).second;
    }

    void AddShip(int32_t nID, CSchiff* pS) { m_cpoSchiffe[nID] = pS; }

    CSchiff* GetShip(int32_t nID)
    {
        Schiffe::iterator si = m_cpoSchiffe.find(nID);
        return si == m_cpoSchiffe.end() ? 0 : (*si).second;
    }

    size_t NumMessage() const { return m_cpoMessages.size(); }

    void AddMessage(CMessage::Ptr pMsg) { m_cpoMessages[m_cpoMessages.size()] = pMsg; }

    CMessage::Ptr GetMessage(size_t nID)
    {
        Messages::iterator mi = m_cpoMessages.find(nID);
        if (mi != m_cpoMessages.end()) {
            return (*mi).second;
        }
        else {
            return CMessage::Ptr();
        }
    }

    size_t NumNachrichten() const { return m_csNachrichten.size(); }

    std::string GetNachricht(int32_t nID)
    {
        Nachrichten::iterator ni;
        int32_t nCnt = 0;
        for (ni = m_csNachrichten.begin(); nCnt < nID && ni != m_csNachrichten.end(); ni++, nCnt++)
            ;
        return nCnt == nID ? (*ni).second : std::string("");
    }

    void CalculateStatistics();

    CPartei::Ptr GetLocalParteiInfo(int32_t nPartei)
    {
        ParteiInfos::iterator pi = m_cpoLocalParteiInfos.find(nPartei);
        if (pi == m_cpoLocalParteiInfos.end())
            return CPartei::Ptr();
        else
            return (*pi).second;
    }

    size_t GetParteiNum() { return m_cpoLocalParteiInfos.size(); }

    static CPartei::Ptr GetGlobalParteiInfo(int32_t nPartei)
    {
        ParteiInfos::iterator pi = g_cpoParteiInfos.find(nPartei);
        if (pi == g_cpoParteiInfos.end())
            return CPartei::Ptr();
        else
            return (*pi).second;
    }

    CPartei::Ptr GetNthParteiInfo(int32_t nIndex)
    {
        ParteiInfos::iterator pi = m_cpoLocalParteiInfos.begin();
        while (pi != m_cpoLocalParteiInfos.end() && nIndex) {
            nIndex--;
            pi++;
        }
        if (pi == m_cpoLocalParteiInfos.end())
            return CPartei::Ptr();
        else
            return (*pi).second;
    }

    static size_t GetGruppenNum() { return m_cpoGruppen.size(); }

    static CGruppe::Ptr GetGruppe(int32_t nGruppe)
    {
        Gruppen::iterator gi = m_cpoGruppen.find(nGruppe);
        if (gi == m_cpoGruppen.end())
            return CGruppe::Ptr();
        else
            return (*gi).second;
    }

    static CGruppe::Ptr GetNthGruppe(int32_t nIndex)
    {
        Gruppen::iterator gi = m_cpoGruppen.begin();
        while (gi != m_cpoGruppen.end() && nIndex) {
            nIndex--;
            gi++;
        }
        if (gi == m_cpoGruppen.end())
            return CGruppe::Ptr();
        else
            return (*gi).second;
    }

    static int32_t GetGroupIdByName(const std::string& sName);

    Optionen& Options() { return m_csOptionen; }

    static void SetDefaultPassword(const std::string& sPass) { m_sDefaultPassword = sPass; }

protected:
    void StatistikEreignisse(const std::string& sMsg);
    void StatistikHandel(const std::string& sMsg);
    void StatistikProduktion(const std::string& sMsg);
    void StatistikEinkommen(const std::string& sMsg);
    void InsertHandel(int32_t nPNr, int32_t nAmount, const std::string& sProduct);
    int32_t PNrFromENr(int32_t nENr);

protected:
    bool m_bIsValid;
    bool m_bHasIslandTags;
    bool m_bHasSpiel;
    bool m_bUTF8;
    CKarte* m_poMap;
    std::string m_sOrgCRName;
    int32_t m_nVersion;
    std::string m_sSpiel;
    std::string m_sKonfiguration;
    std::string m_sPasswort;
    std::string m_sParteiname;
    int m_nENrBase, m_nPNrBase, m_nBNrBase;
    int32_t m_nRunde;
    int m_nZeitalter;
    int32_t m_nPartei;
    int32_t m_nRekrutierungskosten;
    int32_t m_nPersonen;
    int32_t m_nPunkte;
    int32_t m_nPunkteschnitt;
    int32_t m_nEinkommen;
    int32_t m_nAusgaben;
    int32_t m_nMsgEinkommen;
    int32_t m_nMsgAusgaben;
    CMessage::RENDERER m_enMsgRenderer;
    Nachrichten m_csNachrichten;
    Einheiten m_cpoGEinheiten;
    Handelspartner m_cpoHPartner;
    Parteien m_coParteien;
    ParteiInfos m_cpoLocalParteiInfos;
    TradeInfos m_coTradeInfos;
    Messages m_cpoMessages;
    Bauwerke m_cpoBauwerke;
    Schiffe m_cpoSchiffe;
    Optionen m_csOptionen;
    std::map<int32_t, std::string> m_coMessageTypes;
    std::map<int32_t, std::string> m_coMessageSections;
    static std::string m_sDefaultPassword;
    static Reports m_cpoReports;
    static ParteiInfos g_cpoParteiInfos;
    static Gruppen m_cpoGruppen;
    static int32_t m_nMaxRound;
};

class CResource : public CBlockBase
{
public:
    CResource(CReportStream& oRS)
        : CBlockBase("CResource", oRS)
    {
    }

    virtual ~CResource() {}
};

class CRegionSorter
{
public:
    CRegionSorter(int nFlags)
        : m_nFlags(nFlags)
    {
    }

    virtual ~CRegionSorter() {}

    bool operator()(CRegion* pR1, CRegion* pR2) const;

protected:
    int m_nFlags;
};

class DummyRegion : public CBlockBase
{
    struct CTerrainType
    {
        std::string m_sName;
        char m_cMCY, m_cMCN;
        int32_t m_nMaxWork;
        bool m_bLand, m_bEisen, m_bLaen;

        CTerrainType(const std::string& sName, char cMCY, char cMCN, int nMaxWork, bool bLand, bool bEisen, bool bLaen)
            : m_sName(sName)
            , m_cMCY(cMCY)
            , m_cMCN(cMCN)
            , m_nMaxWork(nMaxWork)
            , m_bLand(bLand)
            , m_bEisen(bEisen)
            , m_bLaen(bLaen)
        {
        }

        CTerrainType(const CTerrainType&) = default;

        const CTerrainType& operator=(const CTerrainType& oTT)
        {
            m_sName = oTT.m_sName;
            m_cMCY = oTT.m_cMCY;
            m_cMCN = oTT.m_cMCN;
            m_nMaxWork = oTT.m_nMaxWork;
            m_bLand = oTT.m_bLand;
            m_bEisen = oTT.m_bEisen;
            m_bLaen = oTT.m_bLaen;
            return *this;
        }
    };

    typedef std::map<std::string, CTerrainType> TerrainTypes;
    typedef std::map<int32_t, CBauwerk*> Bauwerke;
    typedef std::vector<CBauwerk*> VBauwerke;
    typedef std::map<int32_t, CSchiff*> Schiffe;
    typedef std::vector<CSchiff*> VSchiffe;
    typedef std::map<int32_t, CEinheit*> Einheiten;
    typedef std::vector<CEinheit*> VEinheiten;
    typedef std::map<int32_t, CGrenze*> Grenzen;
    typedef std::vector<CGrenze*> VGrenzen;
    typedef std::map<std::string, CResource*> Resourcen;
    typedef std::vector<CResource*> VResourcen;
    typedef std::vector<std::string> Durchreisen;
    typedef std::list<std::string> Botschaften;
    typedef std::list<CMessage::Ptr> Messages;
    typedef std::pair<std::string, int32_t> Luxusgut;
    typedef std::vector<Luxusgut> Luxusgueter;
    typedef std::map<std::string, int> Materialpool;
    typedef std::map<std::string, double> ResourceImpacts;

    enum REGIONBLOCKS { enREGION, enDURCHREISEREGION, enSPEZIALREGION, enSCHEMEN, enUNKNOWN };

    CRegionKey m_nInsel;
    std::string m_sInsel;
    std::string m_sName;
    std::string m_sBeschreibung;
    Luxusgueter m_coLuxusgueter;
    std::unique_ptr<Bauwerke> m_pcpoBauwerke;
    std::unique_ptr<VBauwerke> m_pcpoVBauwerke;
    std::unique_ptr<Schiffe> m_pcpoSchiffe;
    std::unique_ptr<VSchiffe> m_pcpoVSchiffe;
    Grenzen m_cpoGrenzen;
    VGrenzen m_cpoVGrenzen;
    Resourcen m_cpoResourcen;
    VResourcen m_cpoVResourcen;
    Einheiten m_cpoEinheiten;
    VEinheiten m_cpoVEinheiten;
    Durchreisen m_coDurchreisen;
    Durchreisen m_coDurchschiffungen;
    Botschaften m_coBotschaften;
    VKommandos m_coKommandos;
    VKommandos m_coEndKommandos;
    Effects m_coEffects;
    Messages m_cpoMessages;
};

class CRegion : public CBlockBase
{
public:
    struct CTerrainType
    {
        std::string m_sName;
        char m_cMCY, m_cMCN;
        int32_t m_nMaxWork;
        bool m_bLand, m_bEisen, m_bLaen;

        CTerrainType(const std::string& sName, char cMCY, char cMCN, int nMaxWork, bool bLand, bool bEisen, bool bLaen)
            : m_sName(sName)
            , m_cMCY(cMCY)
            , m_cMCN(cMCN)
            , m_nMaxWork(nMaxWork)
            , m_bLand(bLand)
            , m_bEisen(bEisen)
            , m_bLaen(bLaen)
        {
        }

        CTerrainType(const CTerrainType&) = default;

        const CTerrainType& operator=(const CTerrainType& oTT)
        {
            m_sName = oTT.m_sName;
            m_cMCY = oTT.m_cMCY;
            m_cMCN = oTT.m_cMCN;
            m_nMaxWork = oTT.m_nMaxWork;
            m_bLand = oTT.m_bLand;
            m_bEisen = oTT.m_bEisen;
            m_bLaen = oTT.m_bLaen;
            return *this;
        }
    };

    friend class CRegionSorter;

    typedef std::map<std::string, CTerrainType> TerrainTypes;
    typedef std::map<int32_t, CBauwerk*> Bauwerke;
    typedef std::vector<CBauwerk*> VBauwerke;
    typedef std::map<int32_t, CSchiff*> Schiffe;
    typedef std::vector<CSchiff*> VSchiffe;
    typedef std::map<int32_t, CEinheit*> Einheiten;
    typedef std::vector<CEinheit*> VEinheiten;
    typedef std::map<int32_t, CGrenze*> Grenzen;
    typedef std::vector<CGrenze*> VGrenzen;
    typedef std::map<std::string, CResource*> Resourcen;
    typedef std::vector<CResource*> VResourcen;
    typedef std::vector<std::string> Durchreisen;
    typedef std::list<std::string> Botschaften;
    typedef std::list<CMessage::Ptr> Messages;
    typedef std::pair<std::string, int32_t> Luxusgut;
    typedef std::vector<Luxusgut> Luxusgueter;
    typedef std::map<std::string, int32_t> Materialpool;
    typedef std::map<std::string, double> ResourceImpacts;

    enum REGIONBLOCKS { enREGION, enDURCHREISEREGION, enSPEZIALREGION, enSCHEMEN, enUNKNOWN };

    CRegion(const std::string& sType, int32_t nX = 0, int32_t nY = 0, int32_t nZ = 0, int nRunde = 0);
    CRegion(CReportStream& oRS, CKarte* poMap, int nRund = 0, int32_t nPos = 0);
    ~CRegion();

    int GetQuality() const;

    REGIONBLOCKS GetBlock() const { return m_enBlock; }

    CKarte* Map() const { return m_poMap; }

    int Runde() const
    {
        if (m_nRunde)
            return m_nRunde;
        return 0;
    }

    void SetMap(CKarte* poMap) { m_poMap = poMap; }

    void Write(CReportStream& oRS);
    void Vorlage(int32_t nPlayer);

    std::string GetName() const { return m_sName; }

    std::string Beschr() const { return m_sBeschreibung; }

    Value GetValue(const std::string& sKey) const;
    Value DeepGetValue(const std::string& sKey);
    int32_t SilverOf(int32_t nPlayer) const;
    int32_t PersonsOf(int32_t nPlayer, bool realPersons = false) const;
    bool IsGroup(int32_t nGroupID) const;

    bool IsOwnUnit() const { return m_bOwnUnit; }

    int32_t GetEX() const { return m_nX; }

    int32_t GetEY() const { return m_nY; }

    int32_t GetEZ() const { return m_nZ; }

    int32_t GetLohn() const { return m_nLohn; }

    int32_t GetBauern() const { return m_nBauern; }

    int32_t GetSilber() const { return m_nSilber; }

    int32_t GetUnterhalt() const { return m_nUnterhalt; }

    int32_t GetRekruten() const { return m_nRekruten; }

    int32_t GetPferde() const { return m_nPferde; }

    int32_t GetBaeume() const { return m_nBaeume; }

    int32_t GetEisen() const { return m_nEisen; }

    int32_t GetLaen() const { return m_nLaen; }

    bool isMallorn() const { return m_nMallorn != 0; }

    bool isVerorkt() const { return m_bVerorkt; }

    int GetVerkauf() const { return m_nVerkauf; }

    int GetBonus() const;

    void hasOwnUnit(bool flag) { m_bOwnUnit = flag; }

    CRegionKey GetKey() const { return CalcKey(m_nX, m_nY, m_nZ); }

    CRegionKey GetIsland() const { return m_nInsel; }

    CRegionKey GetSortIsland() const { return CKarte::GetIsland(m_nX, m_nY, m_nZ); }

    void SetIsland(CRegionKey nInsel) { m_nInsel = nInsel; }

    void SetIslandName(const std::string& name) { m_idInsel = CStringDB::Str2SID(name); }

    const std::string& GetIslandName() const { return CStringDB::SID2Str(m_idInsel); }

    char GetRegionChar() const;
    const std::string& GetRegionTypeName() const;
    int32_t GetRegionKap() const;
    int32_t CalcJobs() const;
    int32_t CalcProfit() const;

    int32_t GetEinkommen() const { return m_nEinkommen; }

    int32_t GetAusgaben() const { return m_nAusgaben; }

    void SetEinkommen(int32_t i) { m_nEinkommen = i; }

    void SetAusgaben(int32_t i) { m_nAusgaben = i; }

    const Durchreisen& GetDurchreisen() const { return m_coDurchreisen; }

    const Durchreisen& GetDurchschiffungen() const { return m_coDurchschiffungen; }

    VKommandos& GetKommandos() { return m_coKommandos; }

    VKommandos& GetEndKommandos() { return m_coEndKommandos; }

    VResourcen& GetResourcen() { return m_cpoVResourcen; }

    Luxusgueter& GetLuxusgueter() { return m_coLuxusgueter; }

    CResource* GetResource(const std::string& sName) const
    {
        Resourcen::const_iterator ri = m_cpoResourcen.find(Flatten(sName));
        if (ri != m_cpoResourcen.end())
            return (*ri).second;
        return 0;
        /*
        for( int i=0; i<m_cpoVResourcen.size(); i++ )
        {
            if( IsEqual( m_cpoVResourcen[i]->GetValue( "type" ).asString(), sName.c_str() ) )
                return m_cpoVResourcen[i];
        }

        return 0;
        */
    }

    size_t NumFrontiers() const { return m_cpoGrenzen.size(); }

    CGrenze* GetFrontier(int32_t nID)
    {
        Grenzen::iterator gi = m_cpoGrenzen.find(nID);
        if (gi != m_cpoGrenzen.end())
            return (*gi).second;
        else
            return 0;
    }

    size_t NumBuildings() const { return m_pcpoBauwerke ? m_pcpoBauwerke->size() : 0; }

    CBauwerk* GetBuilding(int32_t nID)
    {
        if (!m_pcpoBauwerke)
            return 0;
        Bauwerke::iterator bi = m_pcpoBauwerke->find(nID);
        if (bi != m_pcpoBauwerke->end())
            return (*bi).second;
        else
            return 0;
    }

    size_t NumShips() const { return m_pcpoSchiffe ? m_pcpoSchiffe->size() : 0; }

    CSchiff* GetShip(int32_t nID)
    {
        if (!m_pcpoSchiffe)
            return 0;
        Schiffe::iterator si = m_pcpoSchiffe->find(nID);
        if (si != m_pcpoSchiffe->end())
            return (*si).second;
        else
            return 0;
    }

    bool IsLand() const { return m_poTerrain->m_bLand; }

    const CTerrainType* GetTerrain() const { return m_poTerrain; }

    const CTerrainType* FindTerrain(const std::string& sType);

    VEinheiten& GetVEinheiten() { return m_cpoVEinheiten; }

    VBauwerke* GetVBauwerke() { return m_pcpoVBauwerke.get(); }

    VSchiffe* GetVSchiffe() { return m_pcpoVSchiffe.get(); }

    const VGrenzen* GetVGrenzen() { return &m_cpoVGrenzen; }

    const Botschaften& GetBotschaften() const { return m_coBotschaften; }

    void AddMaterialpool(int32_t nPartei, Materialpool& coPool, bool bSearchable = true);

    void AddEinkommen(int32_t nSilber) { m_nEinkommen += nSilber; }

    void AddAusgaben(int32_t nSilber) { m_nAusgaben += nSilber; }

    void AddMessage(const std::string& sTxt) { m_coBotschaften.push_back(sTxt); }

    void AddMessage(CMessage::Ptr pMsg) { m_cpoMessages.push_back(pMsg); }

    size_t NumMessage() const { return m_cpoMessages.size(); }

    CMessage::Ptr GetMessage(int32_t nID)
    {
        Messages::iterator mi = m_cpoMessages.begin();
        while (mi != m_cpoMessages.end() && nID) {
            nID--;
            mi++;
        }
        if (mi != m_cpoMessages.end()) {
            return (*mi);
        }
        else {
            return CMessage::Ptr();
        }
    }

    const Effects& GetEffects() const { return m_coEffects; }

    CRegion* GetNW() { return m_poMap->GetFromECords(m_nX - 1, m_nY + 1, m_nZ); }

    CRegion* GetN() { return m_poMap->GetFromECords(m_nX, m_nY + 1, m_nZ); }

    CRegion* GetNO() { return m_poMap->GetFromECords(m_nX, m_nY + 1, m_nZ); }

    CRegion* GetO() { return m_poMap->GetFromECords(m_nX + 1, m_nY, m_nZ); }

    CRegion* GetSO() { return m_poMap->GetFromECords(m_nX + 1, m_nY - 1, m_nZ); }

    CRegion* GetS() { return m_poMap->GetFromECords(m_nX, m_nY - 1, m_nZ); }

    CRegion* GetSW() { return m_poMap->GetFromECords(m_nX, m_nY - 1, m_nZ); }

    CRegion* GetW() { return m_poMap->GetFromECords(m_nX - 1, m_nY, m_nZ); }

    CRegionKey CalcRelKey(int32_t nDX, int32_t nDY) const { return CalcKey(nDX + m_nX, nDY + m_nY, m_nZ); }

    static CRegionKey CalcKey(int32_t nX, int32_t nY, int32_t nZ) { return CRegionKey(nX, nY, nZ); }

    static int32_t CurrentPlayer() { return m_nCurrentPlayer; }

    static void SetCurrentPlayer(int32_t nP) { m_nCurrentPlayer = nP; }

    static int CurrentRound() { return m_nCurrentRound; }

    static void SetCurrentRound(int32_t nR) { m_nCurrentRound = nR; }

    static void SetMoveOffset(int32_t nX, int32_t nY)
    {
        m_nMoveX = nX;
        m_nMoveY = nY;
    }

protected:
    CKarte* m_poMap;
    CRegionKey m_nInsel;
    int32_t m_idInsel;
    REGIONBLOCKS m_enBlock;
    int32_t m_nRunde;
    int32_t m_nPos;
    int32_t m_nX;
    int32_t m_nY;
    int32_t m_nZ;
    int32_t m_nPartei;
    std::string m_sName;
    bool m_bVerorkt;
    const CTerrainType* m_poTerrain;
    //	std::string m_sType;
    int32_t m_nBauern;
    std::string m_sBeschreibung;
    int32_t m_nPferde;
    int32_t m_nBaeume;
    int32_t m_nMallorn;
    int32_t m_nEisen;
    int32_t m_nLaen;
    int32_t m_nSilber;
    int32_t m_nUnterhalt;
    int32_t m_nRekruten;
    int32_t m_nLohn;
    int32_t m_nStrasse;
    //	int32_t m_nPreise[7];
    //	std::string m_sLuxusgut[7];
    Luxusgueter m_coLuxusgueter;
    int32_t m_nVerkauf;
    int32_t m_nMaxBurg;
    int32_t m_nBonus;
    bool m_bOwnUnit;
    int32_t m_nEinkommen;
    int32_t m_nAusgaben;
    std::unique_ptr<Bauwerke> m_pcpoBauwerke;
    std::unique_ptr<VBauwerke> m_pcpoVBauwerke;
    std::unique_ptr<Schiffe> m_pcpoSchiffe;
    std::unique_ptr<VSchiffe> m_pcpoVSchiffe;
    Grenzen m_cpoGrenzen;
    VGrenzen m_cpoVGrenzen;
    Resourcen m_cpoResourcen;
    VResourcen m_cpoVResourcen;
    Einheiten m_cpoEinheiten;
    VEinheiten m_cpoVEinheiten;
    Durchreisen m_coDurchreisen;
    Durchreisen m_coDurchschiffungen;
    Botschaften m_coBotschaften;
    VKommandos m_coKommandos;
    VKommandos m_coEndKommandos;
    Effects m_coEffects;
    Messages m_cpoMessages;
    static int32_t m_nCurrentPlayer;
    static int32_t m_nCurrentRound;
    static int32_t m_nMoveX;
    static int32_t m_nMoveY;
    static TerrainTypes m_coTerrains;
    static ResourceImpacts m_coRImpacts;
};

class CBauwerk : public CBlockBase
{
public:
    struct CBuildingType
    {
        std::string m_sName;
        int32_t m_nUSilber;

        CBuildingType(const std::string& sName, int32_t nUSilber)
            : m_sName(sName)
            , m_nUSilber(nUSilber)
        {
        }

        CBuildingType(const CBuildingType&) = default;

        const CBuildingType& operator=(const CBuildingType& oBT)
        {
            m_sName = oBT.m_sName;
            m_nUSilber = oBT.m_nUSilber;
            return *this;
        }
    };

    friend class CVorlage;

    typedef std::map<std::string, CBuildingType> BuildingTypes;

    CBauwerk(CReportStream& oRS, int32_t nRunde);
    ~CBauwerk();
    void Write(CReportStream& oRS);

    std::string Typ() const { return m_sTyp; }

    std::string XTyp() const;

    std::string Name() const { return m_sName; }

    std::string Beschreibung() const { return m_sBeschreibung; }

    int32_t Nummer() const { return m_nNummer; }

    int32_t Besitzer() const { return m_nBesitzer; }

    int32_t Belagerer() const { return m_nBelagerer; }

    int32_t Groesse() const { return m_nGroesse; }

    int32_t Insassen() const { return m_nInsassen; }

    int32_t Unterhalt() const { return m_nUnterhalt; }

    void AddInsassen(int32_t nAnz) { m_nInsassen += nAnz; }

    const Effects& GetEffects() const { return m_coEffects; }

    VKommandos& GetKommandos() { return m_coKommandos; }

protected:
    int32_t m_nNummer;
    std::string m_sTyp;
    std::string m_sName;
    std::string m_sBeschreibung;
    int32_t m_nGroesse;
    int32_t m_nBesitzer;
    int32_t m_nPartei;
    int32_t m_nUnterhalt;
    int32_t m_nBelagerer;
    int32_t m_nInsassen;
    Effects m_coEffects;
    VKommandos m_coKommandos;
    static BuildingTypes m_coBuildings;
};

class CSchiff : public CBlockBase
{
public:
    struct CShipType
    {
        std::string m_sName;
        int32_t m_nKap;
        int32_t m_nHolz;

        CShipType(const std::string& sName, int32_t nKap, int32_t nHolz)
            : m_sName(sName)
            , m_nKap(nKap)
            , m_nHolz(nHolz)
        {
        }

        CShipType(const CShipType&) = default;

        const CShipType& operator=(const CShipType& oBT)
        {
            m_sName = oBT.m_sName;
            m_nKap = oBT.m_nKap;
            m_nHolz = oBT.m_nHolz;
            return *this;
        }
    };

    friend class CVorlage;

    typedef std::map<std::string, CShipType> ShipTypes;

    CSchiff(CReportStream& oRS, int32_t nRunde);
    ~CSchiff();
    void Write(CReportStream& oRS);

    std::string Typ() const { return m_sTyp; }

    std::string Name() const { return m_sName; }

    std::string Beschreibung() const { return m_sBeschreibung; }

    int32_t Nummer() const { return m_nNummer; }

    int32_t Kapitaen() const { return m_nKapitaen; }

    int32_t Kueste() const { return m_nKueste; }

    int32_t Anzahl() const { return m_nAnzahl; }

    int32_t Schaden() const { return m_nSchaden; }

    int32_t Prozent() const { return m_nProzent; }

    int32_t Ladung() const { return m_nLadung; }

    int32_t MaxLadung() const
    {
        if (m_bCRKap)
            return m_nMaxLadung;
        else
            return Kapazitaet();
    }

    int32_t Kapazitaet() const;
    int32_t MaxHolz() const;
    int32_t Holz() const;

    bool CRKap() const { return m_bCRKap; }

    int32_t Insassen() const { return m_nInsassen; }

    void AddWeight(int32_t nWeight) { m_nLadung += nWeight; }

    void AddInsassen(int32_t nAnz) { m_nInsassen += nAnz; }

    const Effects& GetEffects() const { return m_coEffects; }

    VKommandos& GetKommandos() { return m_coKommandos; }

    static const CShipType* FindShip(const std::string& sType);

protected:
    int32_t m_nNummer;
    std::string m_sName;
    std::string m_sBeschreibung;
    std::string m_sTyp;
    int32_t m_nAnzahl;
    int32_t m_nSchaden;
    int32_t m_nProzent;
    int32_t m_nKapitaen;
    int32_t m_nPartei;
    int32_t m_nLadung;
    int32_t m_nMaxLadung;
    int32_t m_nKueste;
    bool m_bCRKap;
    int32_t m_nInsassen;
    Effects m_coEffects;
    VKommandos m_coKommandos;
    static ShipTypes m_coShips;
};

class CTalent
{
public:
    CTalent(const std::string& sTyp, int32_t nTage, int32_t nStufe, int32_t nAddon = 0)
        : m_sTyp(sTyp)
        , m_nTage(nTage)
        , m_nStufe(nStufe)
        , m_nAddon(nAddon)
    {
    }

    std::string m_sTyp;
    int32_t m_nTage;
    int32_t m_nStufe;
    int32_t m_nAddon;
};

class CGrenze : public CBlockBase
{
public:
    friend class CVorlage;

    CGrenze(CReportStream& oRS);
    ~CGrenze();
    void Write(CReportStream& oRS);

    std::string Typ() const { return m_sTyp; }

    int32_t Richtung() const { return m_nRichtung; }

    int32_t Prozent() const { return m_nProzent; }

    const Effects& GetEffects() const { return m_coEffects; }

protected:
    int32_t m_nNummer;
    std::string m_sTyp;
    int32_t m_nRichtung;
    int32_t m_nProzent;
    Effects m_coEffects;
};

class CTalentSorter
{
public:
    CTalentSorter(int32_t nFlags)
        : m_nFlags(nFlags)
    {
    }

    virtual ~CTalentSorter() {}

    bool operator()(const CTalent& oT1, const CTalent& oT2) const;

protected:
    int32_t m_nFlags;
};

class CEinheitenSorter
{
public:
    CEinheitenSorter(int32_t nFlags)
        : m_nFlags(nFlags)
    {
    }

    virtual ~CEinheitenSorter() {}

    bool operator()(CEinheit* pE1, CEinheit* pE2) const;

protected:
    int32_t m_nFlags;
};

class CKampfzauber : public CBlockBase
{
public:
    CKampfzauber(CReportStream& oRS);
    ~CKampfzauber();
};

class CEinheit : public CBlockBase
{
public:
    friend class CEinheitenSorter;
    friend class CMetaCommand;
    friend class CVorlage;
    friend class CRegion;
    typedef std::vector<int32_t> KommandoZeilen;
    typedef std::vector<CTalent> Talente;
    typedef std::pair<std::string, int32_t> CGegenstand;
    typedef std::vector<CGegenstand> Gegenstaende;
    typedef std::vector<std::string> Botschaften;
    typedef std::vector<std::string> Sprueche;
    typedef std::list<CMessage::Ptr> Messages;
    typedef std::vector<CKampfzauber::Ptr> Kampfzauber;

    CEinheit(CReportStream& oRS, CRegion* poRegion);
    ~CEinheit();
    void Write(CReportStream& oRS);
    int32_t GetQuality() const;
    using CBlockBase::GetValue;
    Value GetValue(const std::string& sKey, const std::string& sKey2) const;

    std::string Typ() const { return m_sTyp; }

    std::string WahrerTyp() const { return m_sWahrerTyp; }

    std::string RealType() const { return m_sWahrerTyp.empty() ? m_sTyp : m_sWahrerTyp; }

    std::string PrefixedTyp(bool bWahr = false) const;

    std::string Name() const { return m_sName; }

    std::string Beschreibung() const { return m_sBeschreibung; }

    int32_t Runde() const { return m_poRegion ? m_poRegion->Runde() : 0; }

    int32_t GruppenID() const { return m_nGruppe; }

    std::string Gruppe() const
    {
        if (m_nGruppe > 0 && m_poRegion && m_poRegion->Map()) {
            CGruppe::Ptr pG = m_poRegion->Map()->Report()->GetGruppe(m_nGruppe);
            std::string sGruppe;
            if (pG.get())
                sGruppe = pG->GetValue("name", Value("")).asString();
            return sGruppe;
        }
        return "";
    }

    CGruppe::Ptr GetGruppe() const
    {
        CGruppe::Ptr pG;
        if (m_nGruppe > 0 && m_poRegion && m_poRegion->Map()) {
            pG = m_poRegion->Map()->Report()->GetGruppe(m_nGruppe);
        }
        return pG;
    }

    int32_t Nummer() const { return m_nNummer; }

    int32_t Partei() const { return m_nPartei; }

    int32_t Silber() const { return m_nSilber; }

    int32_t Anzahl() const { return m_nAnzahl; }

    int32_t Schiff() const { return m_nSchiff; }

    int32_t Bauwerk() const { return m_nBauwerk; }

    int32_t Aufenthaltsort() const { return m_nSchiff ? m_nSchiff + 0x10000000 : m_nBauwerk; }

    const CRegion* Region() const { return m_poRegion; }

    double Gewicht() const;

    std::string HP() const { return m_shp; }

    void CalcKapazitaeten(double& fKapReiten, double& fFKapReiten, int32_t& nRHO, double& fKapGehen, double& fFKapGehen, int32_t& nGHO) const;
    void Kapazitaeten(const std::string& sTarget) const;

    const Talente& Talents() const { return m_coTalente; }

    const Gegenstaende& Things() const { return m_coGegenstaende; }

    const Kampfzauber& CSpells() const { return m_cpoKampfzauber; }

    const Effects& GetEffects() const { return m_coEffects; }

    void Vorlage(CRegion* poReg);

    void AddMessage(std::string sMsg) { m_coBotschaften.push_back(sMsg); }

    void AddMessage(CMessage::Ptr pMsg) { m_cpoMessages.push_back(pMsg); }

    void AddMaterialpool(CRegion::Materialpool& coPool, bool bSearchable);

    CEinheit* GlobalUnit(int32_t nENr);

    bool HasMetas() const;

    VKommandos& GetKommandos() { return m_csKommandos; }

    VKommandos& GetMetaOut() { return m_csMetaOut; }

    Botschaften& GetBotschaften() { return m_coBotschaften; }

protected:
    CRegion* m_poRegion;
    int32_t m_nPlace;
    int32_t m_nNummer;
    int32_t m_nTemp;
    int32_t m_nAlias;
    std::string m_sName;
    std::string m_sBeschreibung;
    int32_t m_nPartei;
    int32_t m_nVerkleidung;
    int32_t m_nAnzahl;
    std::string m_sTyp;
    std::string m_sWahrerTyp;
    int32_t m_nBauwerk;
    int32_t m_nSchiff;
    int32_t m_nSilber;
    int32_t m_nGruppe;
    int32_t m_nKampfStatus;
    int32_t m_nBewacht;
    int32_t m_nBelagert;
    int32_t m_nParteitarnung;
    int32_t m_nTarnung;
    int32_t m_nAura;
    int32_t m_nAuramax;
    int32_t m_nHunger;
    int32_t m_nVerraeter;
    mutable double m_fKapReiten;
    mutable double m_fFKapReiten;
    mutable int32_t m_nRHO;
    mutable double m_fKapGehen;
    mutable double m_fFKapGehen;
    mutable int32_t m_nGHO;
    std::string m_sDefault;
    std::string m_sPrivat;
    std::string m_shp;
    KommandoZeilen m_cnKomLines;
    VKommandos m_csKommandos;
    VKommandos m_csMetaOut;
    Talente m_coTalente;
    Gegenstaende m_coGegenstaende;
    Botschaften m_coBotschaften;
    Sprueche m_coSprueche;
    Effects m_coEffects;
    Messages m_cpoMessages;
    Kampfzauber m_cpoKampfzauber;
};

extern CRegion* g_poCurrentRegion;
extern std::string GetConfigFileName();
extern void SetConfigFileName(const std::string& sFName);
extern bool ExistUserFunction(const std::string& sName);
extern bool DoUserFunction(const std::string& sName, ArgumentList& coArgs, Value* poVal);
