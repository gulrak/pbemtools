#include <catch2/catch_all.hpp>
#include <EBase/charencoding.h>
#include <string>
#include <iostream>
#include <map>

TEST_CASE("charencoding iso-8859-1 to utf-8")
{
    CharacterMapper enc("iso-8859-1", "utf-8");
    CHECK(enc.Map("\xE4\xF6\xFC\xDF") == "äöüß");
}

