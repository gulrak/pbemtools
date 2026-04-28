#include <catch2/catch_all.hpp>
#include <EBase/Report.h>

#include <string>

namespace {

std::string fixturePath(const std::string& fileName)
{
    return std::string(EBASE_TEST_FIXTURE_DIR) + "/" + fileName;
}

}  // namespace

TEST_CASE("CReport parses synthesized party 49 regions fixture", "[report]")
{
    CReport report(fixturePath("test-v69.cr"));

    REQUIRE(report.IsValid());
    CHECK(report.IsUtf8());
    CHECK(report.Version() == 69);
    CHECK(report.Runde() == 803);
    CHECK(report.Partei() == 49);
    CHECK(report.PNrBase() == 36);
    CHECK(report.BNrBase() == 36);
    CHECK(report.Spiel() == "Eressea");
    CHECK(report.Konfiguration() == "Standard");
    CHECK(report.Parteiname() == "Volk der Gulrowinger");
    CHECK(report.Rekrutierungskosten() == 110);
    CHECK(report.GetMap()->Regions().size() == 2);
    CHECK(report.GEinheiten().size() == 7);

    CRegion* eadrax = report.GetMap()->GetFromECords(3, 2, 0);
    REQUIRE(eadrax != nullptr);
    CHECK(eadrax->GetName() == "Eadrax");
    CHECK(eadrax->GetEX() == 3);
    CHECK(eadrax->GetEY() == 2);
    CHECK(eadrax->GetEZ() == 0);
    CHECK(eadrax->GetBauern() == 3228);
    CHECK(eadrax->GetPferde() == 186);
    CHECK(eadrax->GetSilber() == 262211);
    CHECK(eadrax->GetUnterhalt() == 13110);
    CHECK(eadrax->GetRekruten() == 80);
    CHECK(eadrax->GetLohn() == 14);
    CHECK(eadrax->NumFrontiers() == 3);
    CHECK(eadrax->NumBuildings() == 3);

    REQUIRE(eadrax->GetResource("Silber") != nullptr);
    CHECK(eadrax->GetResource("Silber")->GetValue("number").asLong() == 262211);
    REQUIRE(eadrax->GetResource("Eisen") != nullptr);
    CHECK(eadrax->GetResource("Eisen")->GetValue("skill").asLong() == 11);
    CHECK(eadrax->GetResource("Eisen")->GetValue("number").asLong() == 46);

    REQUIRE(eadrax->GetBuilding(1004152) != nullptr);
    CHECK(eadrax->GetBuilding(1004152)->Name() == "Heiliger Tempel vom Buch");
    REQUIRE(eadrax->GetBuilding(876379) != nullptr);
    CHECK(eadrax->GetBuilding(876379)->Besitzer() == 258005);

    REQUIRE(eadrax->GetVEinheiten().size() == 3);
    CEinheit* unit = eadrax->GetVEinheiten().front();
    CHECK(unit->Nummer() == 1594019);
    CHECK(unit->Name() == "Zwerg");
    CHECK(unit->Partei() == 49);
    CHECK(unit->Anzahl() == 5);
    CHECK(unit->Typ() == "Zwerge");
    CHECK(unit->Bauwerk() == 1004152);
    REQUIRE(unit->Talents().size() == 1);
    CHECK(unit->Talents().front().m_sTyp == "Wahrnehmung");
    CHECK(unit->Talents().front().m_nStufe == 16);
    REQUIRE(unit->Things().size() == 12);
    CHECK(unit->Things().front().first == "Balsam");
    CHECK(unit->Things().front().second == 77);

    CRegion* antares = report.GetMap()->GetFromECords(3, 4, 0);
    REQUIRE(antares != nullptr);
    CHECK(antares->GetName() == "Antares");
    CHECK(DeUmlaut(antares->GetRegionTypeName()) == "wueste");
    CHECK(antares->NumBuildings() == 1);
    REQUIRE(antares->GetVEinheiten().size() == 4);
    CHECK(antares->GetVEinheiten().front()->Nummer() == 229801);
    CHECK(antares->GetVEinheiten().front()->Things().size() == 7);
}
