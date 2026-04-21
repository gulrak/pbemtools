/****************************************************************************
 *   $Source: D:\\Development\\Repository/ETools/Vorlage/Zugvorlage.h,v $
 *   $Author: ssh $
 *     $Date: 2003/07/01 09:39:30 $
 * $Revision: 1.1 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Algemeine Utility-Funktionen
 *****************************************************************************
 *
 * $Log: Zugvorlage.h,v $
 * Revision 1.1  2003/07/01 09:39:30  ssh
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/01 09:19:29  ssh
 * Initial recvsing of Source...
 *
 * Revision 1.7  2000/02/24 09:56:48  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.6  1999/11/28 17:38:41  S.Schuemann
 * - Mannigfaltige Änderungen für Vorlage V1.4 beta 9
 *
 * Revision 1.5  1999/11/17 08:59:13  S.Schuemann
 * - support für multiple CRs
 *
 * - vielfache Änderungen für Vorlage 1.4 beta 8
 *
 * Revision 1.4  1999/10/20 02:19:40  S.Schuemann
 * - Die neue Option '-hb' erlaubt es, zu Beginn der Zugvorlage
 *   eine Handelsübersicht, nach Parteien und Produkten einzufügen,
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
 * Revision 1.3  1999/10/18 21:32:20  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.2  1999/09/27 10:31:20  S.Schuemann
 * - Final 1.3
 * - Kampfstatus wird bei Einheiten angezeigt
 * - Schiff und Ablegekueste werden beim Kapitaen angezeigt
 * - Anpassungen durch neue Klasse CReport statt CWorldDB mit
 *   leicht veraenderten Zustaendigkeiten
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/
#pragma once

#include <EBase/Report.h>

#include <cstdint>
#include <fstream>
#include <string>

class CVorlage
{
public:
    CVorlage() {}

    ~CVorlage() {}

    void Vorlage(CReport& oReport, CReport* poRep2, bool bTime = false);
    void RunMetacommands(CReport& oReport);
    void WriteMap(const std::string& sFile);

    static void InitDBS(int32_t nRunde, std::vector<CReport*>& cpoReports);
    static void Islandize(CKarte::IslandQueue& cpoQueue, RegionDB& coRDB);

protected:
    void Regionsvorlage(CRegion* poReg, CRegion* poReg2, CReport* poRep2);
    void ShowInvisibles(CRegion* poReg, CRegion* poReg2, CReport* poRep2);
    void Einheitenvorlage(CEinheit* poUnit, CReport* poRep2);
    void FremdEinheiten(CEinheit* poUnit, CReport* poRep2);
    void BauwerkAusgabe(CBauwerk* pBuilding, CRegion::VEinheiten* poVE = 0);
    void SchiffAusgabe(CSchiff* pSchiff);
    std::string MutateCRBlock(std::fstream& oIS, CBlockBase* poBlockObj, bool utf8);

    int32_t m_nPlayer;
    int32_t m_nUnits;
    CReport* m_poCurrentReport;
    CKarte* m_poKarte;
    CRegion* m_poCurrentRegion;
    CEinheit* m_poCurrentUnit;
    VKommandos m_coInitCmd;
    VKommandos m_coExitCmd;
};
