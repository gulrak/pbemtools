#include <catch2/catch_all.hpp>
#include <EBase/Expression.h>

// Mocking required externs to avoid linking with half of the project
#include <vector>
#include <string>

Value DoPartei(CObjectPart*) { return Value(); }
Value DoUnit(CObjectPart*) { return Value(); }
Value DoShip(CObjectPart*) { return Value(); }
Value DoBuilding(CObjectPart*) { return Value(); }
Value DoRegion(CObjectPart*) { return Value(); }
Value DoGrenze(CObjectPart*) { return Value(); }
Value DoReport(CObjectPart*) { return Value(); }
Value DoThings(CObjectPart*) { return Value(); }
Value DoRaces(CObjectPart*) { return Value(); }
Value DoDB(CObjectPart*) { return Value(); }

Value FGetUnitOfRegion(Expression*, ArgumentList&) { return Value(); }
Value FRandom(Expression*, ArgumentList&) { return Value(); }
Value FEquals(Expression*, ArgumentList& coArgs) {
    if (coArgs.size() >= 2) return Value(coArgs[0] == coArgs[1] ? 1 : 0);
    return Value(0);
}
Value FMatch(Expression*, ArgumentList&) { return Value(); }
Value FBefore(Expression*, ArgumentList&) { return Value(); }
Value FAfter(Expression*, ArgumentList&) { return Value(); }
Value FCrop(Expression*, ArgumentList&) { return Value(); }
Value FChange(Expression*, ArgumentList&) { return Value(); }
Value FSubStr(Expression*, ArgumentList&) { return Value(); }
Value FCeil(Expression*, ArgumentList& coArgs) { return Value(ceil(coArgs[0].asReal())); }
Value FFloor(Expression*, ArgumentList& coArgs) { return Value(floor(coArgs[0].asReal())); }
Value FAbs(Expression*, ArgumentList& coArgs) { return Value(fabs(coArgs[0].asReal())); }
Value FSign(Expression*, ArgumentList&) { return Value(); }
Value FSqrt(Expression*, ArgumentList& coArgs) { return Value(sqrt(coArgs[0].asReal())); }
Value FExp(Expression*, ArgumentList& coArgs) { return Value(exp(coArgs[0].asReal())); }
Value FLog(Expression*, ArgumentList& coArgs) { return Value(log(coArgs[0].asReal())); }
Value FLog10(Expression*, ArgumentList& coArgs) { return Value(log10(coArgs[0].asReal())); }
Value FFloat(Expression*, ArgumentList& coArgs) { return Value(coArgs[0].asReal()); }
Value FInt(Expression*, ArgumentList& coArgs) { return Value(coArgs[0].asLong()); }
Value FIsNothing(Expression*, ArgumentList&) { return Value(); }
Value FItoan(Expression*, ArgumentList&) { return Value(); }
Value FAntoi(Expression*, ArgumentList&) { return Value(); }
Value FXName(Expression*, ArgumentList&) { return Value(); }
Value FLength(Expression*, ArgumentList&) { return Value(); }
Value FFlatten(Expression*, ArgumentList&) { return Value(); }
Value FToLower(Expression*, ArgumentList& coArgs) {
    std::string s = coArgs[0].asString();
    for (auto& c : s) c = tolower(c);
    return Value(s);
}
Value FToUpper(Expression*, ArgumentList& coArgs) {
    std::string s = coArgs[0].asString();
    for (auto& c : s) c = toupper(c);
    return Value(s);
}
Value FTypeOf(Expression*, ArgumentList&) { return Value(); }
Value FTime(Expression*, ArgumentList&) { return Value(); }
Value FAnd(Expression*, ArgumentList& coArgs) { return Value(coArgs[0].asLong() && coArgs[1].asLong() ? 1 : 0); }
Value FOr(Expression*, ArgumentList& coArgs) { return Value(coArgs[0].asLong() || coArgs[1].asLong() ? 1 : 0); }
Value FXor(Expression*, ArgumentList& coArgs) { return Value(coArgs[0].asLong() ^ coArgs[1].asLong() ? 1 : 0); }
Value FNot(Expression*, ArgumentList& coArgs) { return Value(!coArgs[0].asLong() ? 1 : 0); }
Value Fgv(Expression*, ArgumentList&) { return Value(); }
Value Fcv(Expression*, ArgumentList&) { return Value(); }

Value FOpen(Expression*, ArgumentList&) { return Value(); }
Value FClose(Expression*, ArgumentList&) { return Value(); }
Value FReadLine(Expression*, ArgumentList&) { return Value(); }
Value FWriteLine(Expression*, ArgumentList&) { return Value(); }
Value FReadValue(Expression*, ArgumentList&) { return Value(); }
Value FWriteValue(Expression*, ArgumentList&) { return Value(); }
Value FStatus(Expression*, ArgumentList&) { return Value(); }
Value FStatusText(Expression*, ArgumentList&) { return Value(); }
Value FSystem(Expression*, ArgumentList&) { return Value(); }
bool DoUserFunction(const std::string&, ArgumentList&, Value*) { return false; }

std::string GetConfigFileName() { return ""; }
void SetConfigFileName(const std::string&) {}
bool ExistUserFunction(const std::string&) { return false; }

TEST_CASE("Expression basics", "[cexpression]") {
    Expression::Variables vars;
    Value result;

    SECTION("Simple arithmetic") {
        std::string exprStr = "1 + 2 * 3";
        Expression expr(exprStr, __FILE__, __LINE__);
        int a;
        expr.evaluate(vars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asLong() == 7);
    }

    SECTION("Parentheses") {
        std::string exprStr = "(1 + 2) * 3";
        Expression expr(exprStr, __FILE__, __LINE__);
        int a;
        expr.evaluate(vars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asLong() == 9);
    }

    SECTION("Floating point") {
        std::string exprStr = "3.5 * 2";
        Expression expr(exprStr, __FILE__, __LINE__);
        int a;
        expr.evaluate(vars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asReal() == Catch::Approx(7.0));
    }

    SECTION("Global variables") {
        Expression::Variables localVars;
        Value val(42);

        std::string exprStr = "X + 8";
        Expression expr(exprStr, __FILE__, __LINE__);
        expr.setGlobal("X", &val);

        int a;
        expr.evaluate(localVars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asLong() == 50);

        Expression::clearAllVars();
    }

    SECTION("Local variables") {
        // Local variables need a leading $ sign.
        Expression::Variables localVars;
        Value val(42);

        std::string exprStr = "$X + 8";
        Expression expr(exprStr, __FILE__, __LINE__);
        localVars.insert(std::make_pair("$X", val));

        int a;
        expr.evaluate(localVars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asLong() == 50);

        Expression::clearAllVars();
    }

    SECTION("Strings") {
        std::string exprStr = "'Hello ' + 'World'";
        Expression expr(exprStr, __FILE__, __LINE__);
        int a;
        expr.evaluate(vars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asString() == "Hello World");
    }

    SECTION("Arithmetic precedence") {
        Expression::Variables v;
        int a;
        auto eval = [&](std::string s) {
            Expression e(s, __FILE__, __LINE__);
            Value r;
            e.evaluate(v, s.c_str(), &r, &a, false, false);
            return r;
        };

        // Level 2 (+, -) vs Level 3 (*, /, %)
        CHECK(eval("1 + 2 * 3").asLong() == 7);
        CHECK(eval("10 - 4 / 2").asLong() == 8);
        CHECK(eval("10 % 3 + 1").asLong() == 2);

        // Level 3 vs Level 4 (^)
        CHECK(eval("2 * 3 ^ 2").asLong() == 18); // 2 * (3^2)
        CHECK(eval("2 ^ 3 * 2").asLong() == 16); // (2^3) * 2

        // Level 4 (^) vs Level 5 (unary -, !)
        // In this parser, Level 4 calls Level 5 for the base, so unary has HIGHER precedence.
        CHECK(eval("-2 ^ 2").asLong() == 4);   // (-2)^2 = 4

        // Let's verify unary precedence
        CHECK(eval("!0 + 1").asLong() == 2); // (!0) + 1 = 1 + 1 = 2
    }

    SECTION("Unary operators") {
        Expression::Variables v;
        int a;
        auto eval = [&](std::string s) {
            Expression e(s, __FILE__, __LINE__);
            Value r;
            e.evaluate(v, s.c_str(), &r, &a, false, false);
            return r;
        };

        CHECK(eval("-5").asLong() == -5);
        // NOTE: The current parser does NOT support multiple unary operators like --5
        // Level 5 only checks for ONE operator before calling Level 6.
        // CHECK(eval("--5").asLong() == 5);

        CHECK(eval("!1").asLong() == 0);
        CHECK(eval("!0").asLong() == 1);

        // String negation (special case in Level 5)
        CHECK(eval("!'abc'").asString() == "!abc");
    }

    SECTION("Logical operators") {
        Expression::Variables v;
        int a;
        auto eval = [&](std::string s) {
            Expression e(s, __FILE__, __LINE__);
            Value r;
            e.evaluate(v, s.c_str(), &r, &a, false, false);
            return r.asLong();
        };

        // Bitwise/Logical & and | (Level 1a)
        CHECK(eval("1 & 0") == 0);
        CHECK(eval("1 | 0") == 1);

        // Precedence: & and | have SAME precedence in Level 1a
        CHECK(eval("0 | 1 & 0") == 0);
        CHECK(eval("1 | 0 & 0") == 0); // (1 | 0) & 0 = 1 & 0 = 0

        // Standard logical operators (&&, ||) also supported by Level 1a
        CHECK(eval("1 && 0") == 0);
        CHECK(eval("1 || 0") == 1);
        CHECK(eval("0 || 1 && 0") == 0);
    }

    SECTION("Comparison operators") {
        Expression::Variables v;
        int a;
        auto eval = [&](std::string s) {
            Expression e(s, __FILE__, __LINE__);
            Value r;
            e.evaluate(v, s.c_str(), &r, &a, false, false);
            return r.asLong();
        };

        // Level 1b: <, >, <=, >=, ==, !=
        CHECK(eval("1 == 1") == 1);
        CHECK(eval("1 == 2") == 0);
        CHECK(eval("1 != 2") == 1);
        CHECK(eval("1 < 2") == 1);
        CHECK(eval("2 < 1") == 0);
        CHECK(eval("1 <= 1") == 1);
        CHECK(eval("2 <= 1") == 0);
        CHECK(eval("2 > 1") == 1);
        CHECK(eval("1 > 2") == 0);
        CHECK(eval("2 >= 2") == 1);
        CHECK(eval("1 >= 2") == 0);

        // Precedence: Comparisons higher than &
        CHECK(eval("1 == 1 & 0 == 1") == 0);
        CHECK(eval("1 == 1 | 0 == 1") == 1);
    }

    SECTION("Complex precedence example") {
        Expression::Variables v;
        int a;
        auto eval = [&](std::string s) {
            Expression e(s, __FILE__, __LINE__);
            Value r;
            e.evaluate(v, s.c_str(), &r, &a, false, false);
            return r;
        };

        // 1 + 2 * 3 ^ 2 == 19 & 1
        // 1 + 2 * 9 == 19 & 1
        // 1 + 18 == 19 & 1
        // 19 == 19 & 1
        // 1 & 1 -> 1
        CHECK(eval("1 + 2 * 3 ^ 2 == 19 & 1").asLong() == 1);
    }

    SECTION("Metadata for Shunting-Yard") {
        // Precedence
        CHECK(Expression::getPrecedence("^") == Expression::PREC_EXPONENT);
        CHECK(Expression::getPrecedence("*") == Expression::PREC_MULTIPLICATIVE);
        CHECK(Expression::getPrecedence("+") == Expression::PREC_ADDITIVE);
        CHECK(Expression::getPrecedence("==") == Expression::PREC_COMPARISON);
        CHECK(Expression::getPrecedence("&&") == Expression::PREC_LOGICAL);
        CHECK(Expression::getPrecedence("!") == Expression::PREC_UNARY);

        // Relative precedence
        CHECK(Expression::getPrecedence("^") > Expression::getPrecedence("*"));
        CHECK(Expression::getPrecedence("*") > Expression::getPrecedence("+"));
        CHECK(Expression::getPrecedence("+") > Expression::getPrecedence("=="));
        CHECK(Expression::getPrecedence("==") > Expression::getPrecedence("&&"));

        // Associativity
        CHECK(Expression::getAssociativity("+") == Expression::ASSOC_LEFT);
        CHECK(Expression::getAssociativity("*") == Expression::ASSOC_LEFT);
        CHECK(Expression::getAssociativity("^") == Expression::ASSOC_RIGHT);
        CHECK(Expression::getAssociativity("!") == Expression::ASSOC_RIGHT);
    }
}

TEST_CASE("Expression functions", "[cexpression]") {
    Expression::Variables vars;
    Value result;

    SECTION("abs") {
        std::string exprStr = "abs(-5)";
        Expression expr(exprStr, __FILE__, __LINE__);
        int a;
        expr.evaluate(vars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asLong() == 5);
    }

    SECTION("sqrt") {
        std::string exprStr = "sqrt(16)";
        Expression expr(exprStr, __FILE__, __LINE__);
        int a;
        expr.evaluate(vars, exprStr.c_str(), &result, &a, false, false);
        CHECK(result.asReal() == Catch::Approx(4.0));
    }

    SECTION("tolower/toupper") {
        std::string exprStr1 = "tolower('AbC')";
        Expression expr1(exprStr1, __FILE__, __LINE__);
        int a;
        expr1.evaluate(vars, exprStr1.c_str(), &result, &a, false, false);
        CHECK(result.asString() == "abc");

        std::string exprStr2 = "toupper('AbC')";
        Expression expr2(exprStr2, __FILE__, __LINE__);
        expr2.evaluate(vars, exprStr2.c_str(), &result, &a, false, false);
        CHECK(result.asString() == "ABC");
    }
}
