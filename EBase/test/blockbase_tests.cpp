#include <catch2/catch_all.hpp>
#include <EBase/Report.h>

#include <memory>
#include <string>

TEST_CASE("CBlockBase stores and enumerates named values", "[blockbase]")
{
    CBlockBase block("TestBlock", "REGION");

    block.SetValue("Name", Value("Cadiz"));
    block.SetValue("Bauern", Value(int32_t(5600)));

    CHECK(block.NumValues() == 2);
    CHECK(block.GetValue("Name").asString() == "Cadiz");
    CHECK(block.GetValue("Bauern").asLong() == 5600);
    CHECK(block.GetValue("missing", Value("fallback")).asString() == "fallback");

    std::string firstName;
    Value firstValue = block.GetValue(0, firstName);
    CHECK_FALSE(firstName.empty());
    CHECK(firstValue.getType() != VT_EMPTY);
}

TEST_CASE("CBlockBase finds keyed subblocks through object access", "[blockbase]")
{
    CBlockBase root("Root", "REPORT");
    auto region = std::make_shared<CBlockBase>("Region", "REGION", Value(int32_t(-16)), Value(int32_t(47)));

    region->SetValue("Name", Value("Cadiz"));
    region->SetValue("Lohn", Value(int32_t(14)));
    root.AddBlock(region);

    CHECK(root.NumSubblocks() == 1);
    CHECK(CBlockBase::GetValue(&root, "REGION[-16,47].Name").asString() == "Cadiz");
    CHECK(CBlockBase::GetValue(&root, "REGION[-16,47].Lohn").asLong() == 14);
    CHECK(CBlockBase::GetValue(&root, "REGION[-16,46].Name", Value("unknown")).asString() == "unknown");
    CHECK(CBlockBase::GetValue(&root, "REGION.size").asLong() == 1);
}

