/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/Vorlage/Expression.h,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:56:47 $
 * $Revision: 1.9 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Klassen fuer die Ausdrucksauswertung in Metabefehlen
 *****************************************************************************
 *
 * $Log: Expression.h,v $
 * Revision 1.9  2000/02/24 09:56:47  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.8  1999/11/28 17:38:41  S.Schuemann
 * - Mannigfaltige Änderungen für Vorlage V1.4 beta 9
 *
 * Revision 1.7  1999/11/03 10:21:56  S.Schuemann
 * - Anpassungen an Vorlage 1.4 beta 7
 *
 * Revision 1.6  1999/10/28 12:39:58  S.Schuemann
 * - Änderungen für den Linux-Port
 *
 * Revision 1.5  1999/10/26 13:27:01  S.Schuemann
 * - Anpassungen fuer Vorlage 1.4 b 5
 *
 * - Neue Objekt-Attribute
 *
 * - User-Variable und #while
 *
 * Revision 1.4  1999/10/24 08:02:18  S.Schuemann
 * - Anpassungen fuer 1.4 b 4
 * - CReference als Value eingefuehrt
 * - Unterprogramme mit #proc und #call implementiert
 *
 * Revision 1.3  1999/10/18 21:32:19  S.Schuemann
 * - Diverse Aenderungen, fuer die Versionen 1.3.1, 1.3.2, 1.3.3 sowie 1.4 b 1 und 1.4 b 2
 *
 * Revision 1.2  1999/09/27 10:26:07  S.Schuemann
 * - Neues Objekt REPORT eingetragen
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#pragma once

#include <EBase/Value.h>

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

// Token types produced by the lexer (nextToken).
enum class TokenType { NONE = 0, VARNAME, OBJECT, IDENTIFIER, DELIMITER, NUMBER, FLOATNUMBER, UNITNUMBER, STRING };

class ExpressionException
{
public:
    ExpressionException(int n, const std::string& txt = "")
        : num(n)
        , text(txt)
    {
    }

    ExpressionException(const std::string& sTxt)
        : num(-1)
        , text(sTxt)
    {
    }

    ~ExpressionException() {}

    int num;
    std::string text;
};

using ArgumentList = std::vector<Value>;

class Expression
{
public:
    enum RC { E_OK, E_SYNTAX, E_UNBALAN, E_DIVZERO, E_UNKNOWN, E_BADFUNC, E_NUMARGS, E_NOARG, E_EMPTY, E_VALUE, E_RECOVL, E_BADINDEX, E_INTERNAL, E_FLOAT, E_RESTRICTED };

    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept { return std::hash<std::string_view>{}(sv); }
    };
    typedef std::unordered_map<std::string, Value, StringHash, std::equal_to<>> Variables;

    typedef struct
    {
        const char* name;                                            // Functionsname
        size_t args;                                                 // Anzahl der Parameter
        bool risc;                                                   // true wenn verboten bei --restricted
        Value (*func)(Expression* poContext, ArgumentList& coArgs);  // Zeiger auf die Funktion
    } FUNCTION;

    typedef struct
    {
        const char* name;                    // Functionsname
        Value (*func)(CObjectPart* poPart);  // Zeiger auf die Funktion
    } OBJECTS;

    Expression(std::string sExpr, const char* pcFile, int32_t nLine);

    int evaluate(Variables& oContext, const char* e, Value* result, int* a, bool bForce, bool bExpectInplace = true);

    Value* getContainer() const { return _container; }

    bool isContainer(const char* name, Value** pvalue);
    bool getValue(const char* name, Value* value);
    Value* getValueRef(const char* name);
    bool setValue(const char* name, const Value* value, bool bForce = false);
    static bool setValue(Variables& oContext, const char* name, const Value* value, bool bForce = false);
    static bool setGlobal(const char* name, const Value* value, bool bForce = false);
    static void setConstant(const char* name, const Value* value);
    static Value getGlobal(const char* name);
    static Value* getGlobalRef(const char* name);
    static Value* getLocalRef(Expression::Variables& oContext, const char* name);

    static void clearAllVars();
    static bool clearVar(const char* name);

    static Variables& globalContext() { return _vars; }

    static std::shared_ptr<CObjectPart> parseObjectAccess(const std::string& sObjAcc, Variables* poContext = nullptr);
    static Value* getObjectReference(Expression::Variables& oContext, const std::string& sObj);

    enum Precedence {
        PREC_NONE = 0,
        PREC_LOGICAL = 10,         // Level 1a: &, |, &&, ||
        PREC_COMPARISON = 20,      // Level 1b: <, >, <=, >=, ==, !=
        PREC_ADDITIVE = 30,        // Level 2: +, -
        PREC_MULTIPLICATIVE = 40,  // Level 3: *, /, %
        PREC_EXPONENT = 50,        // Level 4: ^
        PREC_UNARY = 60            // Level 5: unary -, !
    };

    enum Associativity { ASSOC_NONE, ASSOC_LEFT, ASSOC_RIGHT };

    //static Precedence getPrecedence(const std::string& token);
    //static Associativity getAssociativity(const std::string& token);
    static bool inAssignment() { return _inAssign; }

protected:
    char peekChar(bool bInString = false, int nOffset = 0);
    char popChar(bool bInString = false);
    char expandInplace(int nOffset, bool bInString);

    void nextToken(bool bRecursion = false);                  // lexer: advance to next token
    void parseObject(bool bGetRef = false);                   // parse a variable/object-path LHS
    int parseAssignment(Value* r);                            // handle assignment (=)
    void evalExpr(Value* r);                                  // Shunting-Yard binary-expression evaluator
    void parseUnary(Value* r);                                // handle prefix unary operators (-, +, !)
    void parsePrimary(Value* r, CReference* pRef = nullptr);  // literals, variables, function calls, (…), […]

protected:
    std::string _exp;
    std::string _expWork;
    std::string::const_iterator _isExp;

    int _pos;
    bool _force;
    bool _expectInplace;
    int _recurse;
    TokenType _type;
    std::string _token;  // current token text (replaces the old malloc'd char* buffer)
    int32_t _line;
    const char* _file;
    std::shared_ptr<CObjectPart> _object;
    Value _objVal;
    Value* _container = nullptr;
    Value* _valueRef = nullptr;
    Variables* _context = nullptr;

    static inline bool _inAssign = false;  // true while evaluating the RHS of an assignment

    static constexpr int MAX_RECURSION_DEPTH = 64;
    static Variables _vars;
    static Variables _consts;
};

class CAScriptObject
{
public:
    virtual const CAScriptObject& operator[](const Value& oVal) = 0;
    virtual const Value& getValue(ArgumentList) = 0;
};

extern Value DoPartei(CObjectPart* poPart);
extern Value DoUnit(CObjectPart* poPart);
extern Value DoShip(CObjectPart* poPart);
extern Value DoBuilding(CObjectPart* poPart);
extern Value DoRegion(CObjectPart* poPart);
extern Value DoGrenze(CObjectPart* poPart);
extern Value DoReport(CObjectPart* poPart);
extern Value DoThings(CObjectPart* poPart);
extern Value DoRaces(CObjectPart* poPart);
extern Value DoDB(CObjectPart* poPart);

extern Value FGetUnitOfRegion(Expression* poContext, ArgumentList& coArgs);
extern Value FRandom(Expression* poContext, ArgumentList& coArgs);
extern Value FEquals(Expression* poContext, ArgumentList& coArgs);
extern Value FMatch(Expression* poContext, ArgumentList& coArgs);
extern Value FBefore(Expression* poContext, ArgumentList& coArgs);
extern Value FAfter(Expression* poContext, ArgumentList& coArgs);
extern Value FCrop(Expression* poContext, ArgumentList& coArgs);
extern Value FChange(Expression* poContext, ArgumentList& coArgs);
extern Value FSubStr(Expression* poContext, ArgumentList& coArgs);
extern Value FCeil(Expression* poContext, ArgumentList& coArgs);
extern Value FFloor(Expression* poContext, ArgumentList& coArgs);
extern Value FAbs(Expression* poContext, ArgumentList& coArgs);
extern Value FSign(Expression* poContext, ArgumentList& coArgs);
extern Value FSqrt(Expression* poContext, ArgumentList& coArgs);
extern Value FExp(Expression* poContext, ArgumentList& coArgs);
extern Value FLog(Expression* poContext, ArgumentList& coArgs);
extern Value FLog10(Expression* poContext, ArgumentList& coArgs);
extern Value FFloat(Expression* poContext, ArgumentList& coArgs);
extern Value FInt(Expression* poContext, ArgumentList& coArgs);
extern Value FIsNothing(Expression* poContext, ArgumentList& coArgs);
extern Value FItoan(Expression* poContext, ArgumentList& coArgs);
extern Value FAntoi(Expression* poContext, ArgumentList& coArgs);
extern Value FXName(Expression* poContext, ArgumentList& coArgs);
extern Value FLength(Expression* poContext, ArgumentList& coArgs);
extern Value FFlatten(Expression* poContext, ArgumentList& coArgs);
extern Value FToLower(Expression* poContext, ArgumentList& coArgs);
extern Value FToUpper(Expression* poContext, ArgumentList& coArgs);
extern Value FTypeOf(Expression* poContext, ArgumentList& coArgs);
extern Value FTime(Expression* poContext, ArgumentList& coArgs);
extern Value FAnd(Expression* poContext, ArgumentList& coArgs);
extern Value FOr(Expression* poContext, ArgumentList& coArgs);
extern Value FXor(Expression* poContext, ArgumentList& coArgs);
extern Value FNot(Expression* poContext, ArgumentList& coArgs);
extern Value Fgv(Expression* poContext, ArgumentList& coArgs);
extern Value Fcv(Expression* poContext, ArgumentList& coArgs);

extern Value FOpen(Expression* poContext, ArgumentList& coArgs);
extern Value FClose(Expression* poContext, ArgumentList& coArgs);
// extern Value FSeek( Expression* poContext, ArgumentList& coArgs );
// extern Value FRead( Expression* poContext, ArgumentList& coArgs );
// extern Value FWrite( Expression* poContext, ArgumentList& coArgs );
extern Value FReadLine(Expression* poContext, ArgumentList& coArgs);
extern Value FWriteLine(Expression* poContext, ArgumentList& coArgs);
extern Value FReadValue(Expression* poContext, ArgumentList& coArgs);
extern Value FWriteValue(Expression* poContext, ArgumentList& coArgs);
extern Value FStatus(Expression* poContext, ArgumentList& coArgs);
extern Value FStatusText(Expression* poContext, ArgumentList& coArgs);

extern Value FSystem(Expression* poContext, ArgumentList& coArgs);

extern bool DoUserFunction(const std::string& sName, ArgumentList& coArgs, Value* poVal);
