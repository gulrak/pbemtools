#include <catch2/catch_all.hpp>
#include <EBase/ReportStream.h>

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "test_utils.h"

namespace {

std::string normalizeLineEndings(std::string text)
{
    std::string normalized;
    normalized.reserve(text.size());

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\r') {
            if (i + 1 < text.size() && text[i + 1] == '\n') {
                ++i;
            }
            normalized.push_back('\n');
        }
        else {
            normalized.push_back(text[i]);
        }
    }

    return normalized;
}

}  // namespace

TEST_CASE("CReportStream parses block lines", "[reportstream]")
{
    auto path = writeTempTestFile("reportstream_block", "REGION 12 -3 0 ; known region\n");

    CReportStream stream(path);

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enBLOCK);
    CHECK(stream.GetValue() == "REGION");
    CHECK(stream.Data() == std::vector<int32_t>{12, -3, 0});
    CHECK(stream.GetComment() == " known region");
    CHECK(stream.GetLine() == 1);
}

TEST_CASE("CReportStream parses integer data lines", "[reportstream]")
{
    auto path = writeTempTestFile("reportstream_integer", "10 -20, 30 ; pool\n");

    CReportStream stream(path);

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enINTEGER);
    CHECK(stream.Data() == std::vector<int32_t>{10, -20, 30});
    CHECK(stream.GetComment() == " pool");
}

TEST_CASE("CReportStream parses and deescapes string lines", "[reportstream]")
{
    auto path = writeTempTestFile("reportstream_string", "\"quoted \\\"name\\\"\";Name\n");

    CReportStream stream(path);

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enSTRING);
    CHECK(stream.GetValue() == "quoted \"name\"");
    CHECK(stream.GetComment() == "Name");
}

TEST_CASE("CReportStream skips empty lines and supports Unget", "[reportstream]")
{
    auto path = writeTempTestFile("reportstream_unget", "\n\nVERSION 66\n\"Eressea\";Spiel\n");

    CReportStream stream(path);

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enBLOCK);
    CHECK(stream.GetValue() == "VERSION");
    CHECK(stream.GetDat(0) == 66);
    CHECK(stream.GetLine() == 2);

    REQUIRE(stream.Unget());
    REQUIRE_FALSE(stream.Unget());

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enBLOCK);
    CHECK(stream.GetValue() == "VERSION");
    CHECK(stream.GetDat(0) == 66);

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enSTRING);
    CHECK(stream.GetValue() == "Eressea");
    CHECK(stream.GetComment() == "Spiel");
}

TEST_CASE("CReportStream strips an initial UTF-8 BOM", "[reportstream]")
{
    auto path = writeTempTestFile("reportstream_bom", std::string("\xEF\xBB\xBF") + "VERSION 66\n");

    CReportStream stream(path);

    REQUIRE(stream.Next());
    CHECK(stream.GetType() == CReportStream::enBLOCK);
    CHECK(stream.GetValue() == "VERSION");
    CHECK(stream.GetDat(0) == 66);
}

TEST_CASE("CReportStream write helpers produce CR lines", "[reportstream]")
{
    auto path = writeTempTestFile("reportstream_write_helpers", "");
    {
        CReportStream stream(path, false);
        stream.WriteBlock("REGION", "known", 1, -2, 0);
        stream.WriteLine("Cadiz", "Name");
        stream.WriteLine(5600, "Bauern");
        stream.WriteLine(10, 20, "Koordinaten");
    }

    std::ifstream in(path, std::ios::binary);
    std::string contents((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    CHECK(normalizeLineEndings(contents) == "REGION 1 -2 0;known\n\"Cadiz\";Name\n5600;Bauern\n10 20;Koordinaten\n");
}
