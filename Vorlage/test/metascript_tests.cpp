#include <catch2/catch_all.hpp>

#include <EBase/Expression.h>
#include <EBase/Utility.h>
#include <Vorlage/Metascript.h>

#include <cstdint>
#include <string>

int32_t g_nTimeCorrection = 0;

std::string GetConfigFileName()
{
    return METASCRIPT_TEST_CONFIG_FILE;
}

void SetConfigFileName(const std::string&)
{
}

namespace {

struct MetaScriptFixture {
    MetaScriptFixture()
    {
        Expression::clearAllVars();
        g_coFlags.insert(VF_NOCONSOLE);
        CMetaCommand::SetTrace(0);
        g_nLimitRuntime = 0;
        g_nTimeCorrection = 0;
    }
};

VKommandos runInline(const std::string& script)
{
    CMetaCommand command(script, 1);
    command.SetFile("<test:inline>");

    CMCI mci(0);
    Expression::Variables vars;
    VKommandos output;
    std::string persisted = script;

    command.RunScript(mci, vars, &persisted, output);
    return output;
}

std::string commandAt(const VKommandos& commands, int32_t index)
{
    return commands[index].asString();
}

}  // namespace

TEST_CASE_METHOD(MetaScriptFixture, "CMetaCommand executes interpreter-only conditionals", "[metascript]")
{
    VKommandos output = runInline("#if 1 { #message 'yes' } #else { #message 'no' }");

    REQUIRE(output.size() == 1);
    CHECK(commandAt(output, 0) == "; yes");
}

TEST_CASE_METHOD(MetaScriptFixture, "CScriptBase imports procedures without a Vorlage instance", "[metascript]")
{
    CScriptBase scripts;

    REQUIRE(scripts.Import("<test:local-import>",
                           R"(
#proc LocalImported $text
{
    #message $text
}
)"));

    CMetaCommand* proc = scripts.FindProc("LocalImported");
    REQUIRE(proc != nullptr);
    CHECK_FALSE(proc->IsFunction());
    CHECK(proc->Args() >= 5);
}

TEST_CASE_METHOD(MetaScriptFixture, "global script base calls imported procedures", "[metascript]")
{
    REQUIRE(g_oScriptBase.Import("<test:global-procedure>",
                                 R"(
#proc GlobalProcedure
{
    #message 'from proc'
}
)"));

    VKommandos output;
    REQUIRE(CMetaCommand::Call("GlobalProcedure", output));

    REQUIRE(output.size() == 1);
    CHECK(commandAt(output, 0) == "; from proc");
}

TEST_CASE_METHOD(MetaScriptFixture, "metascript functions are available to expressions", "[metascript]")
{
    REQUIRE(g_oScriptBase.Import("<test:function-call>",
                                 R"(
#func AddValues $left $right
{
    #return $left+$right
}
)"));

    ArgumentList args;
    args.push_back(Value(2));
    args.push_back(Value(5));

    Value result;
    REQUIRE(DoUserFunction("AddValues", args, &result));
    CHECK(result.asLong() == 7);
}

TEST_CASE_METHOD(MetaScriptFixture, "procedures can use local variables and loops", "[metascript]")
{
    REQUIRE(g_oScriptBase.Import("<test:loop-procedure>",
                                 R"(
#proc LoopProcedure
{
    #var $i
    $i=0
    #while $i<3
    {
        #message $i
        $i=$i+1
    }
}
)"));

    VKommandos output;
    REQUIRE(CMetaCommand::Call("LoopProcedure", output));

    REQUIRE(output.size() == 3);
    CHECK(commandAt(output, 0) == "; 0");
    CHECK(commandAt(output, 1) == "; 1");
    CHECK(commandAt(output, 2) == "; 2");
}
