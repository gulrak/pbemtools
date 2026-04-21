/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/Vorlage/Expression.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:56:46 $
 * $Revision: 1.10 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Klassen fuer die Ausdrucksauswertung in Metabefehlen
 *****************************************************************************
 *
 * $Log: Expression.cpp,v $
 * Revision 1.10  2000/02/24 09:56:46  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.9  1999/11/28 17:38:41  S.Schuemann
 * - Mannigfaltige Änderungen für Vorlage V1.4 beta 9
 *
 * Revision 1.8  1999/11/08 11:14:45  S.Schuemann
 * - Attribute region.gewinn, region[x,y], unit.bewache,
 *   region.pool.ding
 * - Ecaping in Strings
 * - Vars in Attributzugriffen
 * - Funktionen
 * - Sortierung nach BESCHREIBE PRIVAT
 * - Liste fremder Einheiten (normal/verbose)
 * - Bugfix: Leerzeile nach Kapitänsinfo entfernt
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

#include <cmath>
#include <cstdlib>

#ifdef VAX
#include <descrip.h>
#include <ssdef.h>
#endif

#include "../EBase/Utility.h"
#include "Expression.h"
#include "Report.h"
#include "ReportStream.h"

// #define USEOBJMAP
#define USEFUNCMAP

#define ERR(n)                        \
    {                                 \
        g_nERROR = n;                 \
        g_nERPOS = 0;                 \
        g_sERTOK = _token;            \
        _recurse = 0;                 \
        throw ExpressionException(n); \
    }
#define ERRX(t)                                    \
    {                                              \
        g_nERROR = -1;                             \
        g_nERPOS = 0;                              \
        g_sERTOK = _token;                         \
        _recurse = 0;                              \
        throw ExpressionException(std::string(t)); \
    }
#define ERR2(n, t)                       \
    {                                    \
        g_nERROR = n;                    \
        g_nERPOS = 0;                    \
        g_sERTOK = _token;               \
        _recurse = 0;                    \
        throw ExpressionException(n, t); \
    }

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_E
#define M_E 2.71828182845904523536
#endif

/////////////////////////////////////////////////////////////////////
//.block: Globale Variable (noch)
/////////////////////////////////////////////////////////////////////

int g_nERROR;           // Fehler-Code
std::string g_sERTOK;   // Fehler-Token
int g_nERPOS;           // Fehler-Position
const char* g_pcERANC;  // Hilfspointer zur g_nERPOS-Berechnung

/////////////////////////////////////////////////////////////////////
//.block: Mathematische Funktionen die in Ausdruecken erlaubt sind
/////////////////////////////////////////////////////////////////////

std::map<std::string, Expression::FUNCTION*> g_cpoFunctionMap;
std::map<std::string, Expression::OBJECTS*> g_cpoObjectMap;

Expression::FUNCTION Funcs[] = {
    // Name, Anzahl der Argumente, Funktionspointer
    /*
       { "sin",     1,    sin },
       { "cos",     1,    cos },
       { "tan",     1,    tan },
       { "asin",    1,    asin },
       { "acos",    1,    acos },
       { "atan",    1,    atan },
       { "sinh",    1,    sinh },
       { "cosh",    1,    cosh },
       { "tanh",    1,    tanh },
       { "exp",     1,    exp },
       { "log",     1,    log },
       { "log10",   1,    log10 },
       { "sqrt",    1,    sqrt },
       { "floor",   1,    floor },
       { "ceil",    1,    ceil },
       { "abs",     1,    fabs },
       { "hypot",   2,    hypot },
    */
    {"LocalUnit", 1, false, FGetUnitOfRegion},
    {"and", 2, false, FAnd},
    {"or", 2, false, FOr},
    {"xor", 2, false, FXor},
    {"not", 1, false, FNot},
    {"abs", 1, false, FAbs},
    {"after", 2, false, FAfter},
    {"antoi", 2, false, FAntoi},
    {"before", 2, false, FBefore},
    {"ceil", 1, false, FCeil},
    {"change", 3, false, FChange},
    {"crop", 2, false, FCrop},
    {"equals", 2, false, FEquals},
    {"flatten", 1, false, FFlatten},
    {"float", 1, false, FFloat},
    {"floor", 1, false, FFloor},
    {"int", 1, false, FInt},
    {"isnothing", 1, false, FIsNothing},
    {"itoan", 2, false, FItoan},
    {"length", 1, false, FLength},
    {"match", 2, false, FMatch},
    {"random", 0, false, FRandom},
    {"sign", 1, false, FSign},
    {"sqrt", 1, false, FSqrt},
    {"substr", 3, false, FSubStr},
    {"time", 0, false, FTime},
    {"tolower", 1, false, FToLower},
    {"toupper", 1, false, FToUpper},
    {"typeof", 1, false, FTypeOf},
    {"xname", 2, false, FXName},
    {"_gv_", 4, false, Fgv},
    {"_cv_", 4, false, Fcv},
    {"open", 2, true, FOpen},
    {"close", 1, true, FClose},
    //    { "seek",   2, FSeek },
    //    { "read",   2, FRead },
    //    { "write",  2, FWrite },
    {"readline", 1, true, FReadLine},
    {"writeline", 2, true, FWriteLine},
    {"read", 1, true, FReadValue},
    {"write", 2, true, FWriteValue},
    {"status", 1, true, FStatus},
    {"statustext", 1, true, FStatusText},
    {"system", 1, true, FSystem},
    {"exp", 1, false, FExp},
    {"log", 1, false, FLog},
    {"log10", 1, false, FLog10},
    {nullptr, 0, false, nullptr}};

Expression::OBJECTS Objects[] = {
    // Objectname, Indizierbar?, Funktionspointer
    {"region", DoRegion},     {"grenze", DoGrenze}, {"unit", DoUnit},     {"einheit", DoUnit},  {"partei", DoPartei}, {"ship", DoShip}, {"schiff", DoShip},
    {"building", DoBuilding}, {"burg", DoBuilding}, {"report", DoReport}, {"things", DoThings}, {"races", DoRaces},   {"db", DoDB},     {nullptr, nullptr}};

Expression::Variables Expression::_vars;
Expression::Variables Expression::_consts;

const char* g_apcErrorText[] = {"Ok.", "Syntaktischer Fehler.", "Klammerung Fehlerhaft.", "Division durch Null.", "Zugriff auf unbekannte Variable.",
                                //	"Zu viele Variable definiert.",
                                "Unbekannte Funktion.", "Falsche Anzahl von Parametern.", "Argument fehlt.", "Leerer Ausdruck.", "Value Fehler.", "Ueberlauf der Rekursionstiefe.", "Index oder Key ungueltig.", "Interner Fehler in Datenstruktur.",
                                "Fehlerhafte Fliesskommazahl.", "Gesperrte Funktion im Restricted-Modus.", 0};

// -----------------------------------------------------------------------------
//.class: Expression
// -----------------------------------------------------------------------------
Expression::Expression(std::string sExpr, const char* pcFile, int32_t nLine)
    : _pos(0)
    , _force(false)
    , _expectInplace(true)
    , _recurse(0)
    , _type(TokenType::NONE)
    , _line(nLine)
    , _file(pcFile)
{
    int i;
    _exp = sExpr;
    _expWork = sExpr;
    _isExp = _expWork.begin();

    if (g_cpoFunctionMap.empty()) {
        for (i = 0; Funcs[i].name; i++) {
            g_cpoFunctionMap[std::string(Funcs[i].name)] = &(Funcs[i]);
        }
        for (i = 0; Objects[i].name; i++) {
            g_cpoObjectMap[Flatten(Objects[i].name)] = &(Objects[i]);
        }
    }
}

char Expression::peekChar(bool bInString, int nOffset)
{
    if (!_expectInplace) {
        return _pos + nOffset >= (int)_expWork.length() ? 0 : *(_isExp + nOffset);
    }
    return expandInplace(nOffset, bInString);
}

char Expression::popChar(bool bInString)
{
    char c;
    if (!_expectInplace) {
        c = _pos++ >= (int)_expWork.length() ? 0 : *_isExp++;
    }
    else {
        c = expandInplace(-1, bInString);
        _pos++;
        _isExp++;
    }
    return c;
}

char Expression::expandInplace(int nOffset, bool bInString)
{
    int32_t nPos = _pos;
    std::string::const_iterator isExp = _isExp;
    if (nOffset > 0) {
        nPos += nOffset;
        isExp += nOffset;
    }
    if (!bInString) {
        while (nPos + 2 < (int32_t)_expWork.length() && *isExp == '$' && *(isExp + 1) == '(') {
            if (IsFlag(VF_VERSION2WARNING)) {
                ERRMSG(0, ("%s(%d) : Warnung: Inplace wird ab Version 2.0 nicht mehr unterstuetzt!\n", _file, _line));
            }
            std::string sExp(_expWork.substr(static_cast<size_t>(nPos + 2)));
            Expression oExp(sExp, _file, _line);
            Value oVal;
            int dummy, rc;
            rc = oExp.evaluate(*_context, sExp.c_str(), &oVal, &dummy, _force);
            if (rc != E_OK)
                ERR(rc);
            if (oExp._pos + nPos + 1 >= (int32_t)_expWork.length() || _expWork[static_cast<size_t>(oExp._pos + nPos + 1)] != ')')
                ERR(E_SYNTAX);
            _expWork.replace((size_t)nPos, (size_t)oExp._pos + 2, oVal.asString());
            _isExp = _expWork.begin() + _pos;
            isExp = _isExp + nOffset;
        }
    }
    if (nPos >= (int)_expWork.length())
        return 0;
    return *isExp;
}

void Expression::clearAllVars()
{
    _vars.clear();
    _consts.clear();
}

bool Expression::clearVar(const char* name)
{
    // false wenn Var nicht gefunden
    return _vars.erase(std::string(name)) > 0;
}

bool Expression::isContainer(const char* name, Value** pvalue)
{
    Variables::iterator i;

    if (pvalue) {
        *pvalue = 0;
    }

    i = _context->find(std::string(name));
    if (i != _context->end()) {
        if ((*i).second.getType() == VT_MAP || (*i).second.getType() == VT_VECTOR) {
            if (pvalue)
                *pvalue = &(*i).second;
            return true;
        }
        return false;
    }
    i = _vars.find(std::string(name));
    if (i != _vars.end()) {
        if ((*i).second.getType() == VT_MAP || (*i).second.getType() == VT_VECTOR) {
            if (pvalue)
                *pvalue = &(*i).second;
            return true;
        }
        return false;
    }
    return false;
}

Value* Expression::getValueRef(const char* name)
{
    Variables::iterator i;

    i = _context->find(std::string(name));
    if (i != _context->end()) {
        return &(*i).second;
    }
    i = _vars.find(std::string(name));
    if (i != _vars.end()) {
        return &(*i).second;
    }
    else {
        return 0;
    }
}

bool Expression::getValue(const char* name, Value* value)
{
    Variables::iterator i;

    i = _context->find(std::string(name));
    if (i != _context->end()) {
        *value = (*i).second;
        return true;
    }
    i = _vars.find(std::string(name));
    if (i != _vars.end()) {
        *value = (*i).second;
        return true;
    }
    else {
        char Buff[512];
        snprintf(Buff, sizeof(Buff), "Undeklarierte Variable '%s' verwendet.", name);
        value->error(Buff);
        //		*value = Value( name );
        return false;
    }
}

bool Expression::setValue(const char* name, const Value* value, bool bForce)
{
    Variables::iterator i;
    if (_context) {
        i = _context->find(std::string(name));
        if (i != _context->end()) {
            (*i).second = *value;
        }
        else {
            i = _vars.find(std::string(name));
            if (i != _vars.end()) {
                (*i).second = *value;
            }
            else {
                if (!bForce)
                    _context->insert(Variables::value_type(std::string(name), *value));
                else {
                    //			char Buff[512];
                    //			sprintf( Buff, "Zuweisung an undefinierte Variable '%s'.", name );
                    //			value->error( Buff );
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

bool Expression::setValue(Variables& oContext, const char* name, const Value* value, bool bForce)
{
    Variables::iterator i;
    i = oContext.find(std::string(name));
    if (i != oContext.end()) {
        (*i).second = *value;
    }
    else {
        i = _vars.find(std::string(name));
        if (i != _vars.end()) {
            (*i).second = *value;
        }
        else {
            if (!bForce)
                oContext.insert(Variables::value_type(std::string(name), *value));
            else {
                //			char Buff[512];
                //			sprintf( Buff, "Zuweisung an undefinierte Variable '%s'.", name );
                //			value->error( Buff );
                return false;
            }
        }
    }
    return true;
}

bool Expression::setGlobal(const char* name, const Value* value, bool bForce)
{
    Variables::iterator i;
    i = _vars.find(std::string(name));
    if (i != _vars.end()) {
        (*i).second = *value;
    }
    else {
        if (!bForce)
            _vars.insert(Variables::value_type(std::string(name), *value));
        else {
            //			char Buff[512];
            //			sprintf( Buff, "Zuweisung an undefinierte Variable '%s'.", name );
            //			value->error( Buff );
            return false;
        }
    }
    return true;
}

void Expression::setConstant(const char* name, const Value* value)
{
    Variables::iterator i;
    i = _consts.find(std::string(name));
    if (i != _consts.end()) {
        (*i).second = *value;
    }
    else {
        _consts.insert(Variables::value_type(std::string(name), *value));
    }
}

Value Expression::getGlobal(const char* name)
{
    Variables::iterator i;
    i = _vars.find(std::string(name));
    if (i != _vars.end()) {
        return (*i).second;
    }
    i = _consts.find(std::string(name));
    if (i != _consts.end()) {
        return (*i).second;
    }
    return Value();
}

Value* Expression::getGlobalRef(const char* name)
{
    Variables::iterator i;
    i = _vars.find(std::string(name));
    if (i != _vars.end()) {
        return &(*i).second;
    }
    i = _consts.find(std::string(name));
    if (i != _consts.end()) {
        return &(*i).second;
    }
    return 0;
}

Value* Expression::getLocalRef(Expression::Variables& oContext, const char* name)
{
    Variables::iterator i;
    i = oContext.find(std::string(name));
    if (i != oContext.end()) {
        return &(*i).second;
    }
    return 0;
}

void Expression::nextToken(bool bRecursion)
{
    if (++_recurse > MAX_RECURSION_DEPTH) {
        ERR(E_RECOVL);
    }

    _type = TokenType::NONE;
    _token.clear();
    _container = nullptr;

    while (IsSpace(peekChar()))
        popChar();
    auto c = peekChar();
    if (!c) {
        _type = TokenType::DELIMITER;
    }
    else if (c == '\x27') {
        _type = TokenType::STRING;
        popChar();
        while (peekChar(true) != '\x27') {
            if (!peekChar(true)) {
                ERR(E_SYNTAX);
            }
            if (peekChar(true) == '\\') {
                popChar(true);
                if (peekChar(true) && !IsDigit(peekChar(true))) {
                    _token += popChar(true);
                }
                else {
                    c = (peekChar(true) - '0');
                    popChar(true);
                    for (int i = 0; i < 2 && peekChar(true) && IsDigit(peekChar(true)); i++) {
                        c = (c * 10) + (popChar(true) - '0');
                    }
                    _token += c;
                }
            }
            else {
                _token += peekChar(true);
                popChar(true);
            }
        }
        if (peekChar(true) != '\x27')
            ERR(E_SYNTAX);
        popChar(true);
    }
    else if (IsDelim(c)) {
        _type = TokenType::DELIMITER;
        _token += popChar();
        if ((_token[0] == '=' || _token[0] == '!' || _token[0] == '<' || _token[0] == '>') && (peekChar() == '=')) {
            _token += popChar();
        }
        else if ((_token[0] == '&' || _token[0] == '|') && (peekChar() == _token[0])) {
            _token += popChar();
        }
    }
    else if (c == '$') {
        if (!bRecursion)
            parseObject();
        else {
            _type = TokenType::VARNAME;
            _token += popChar();
            c = peekChar();
            if (c == '`' || c == (char)0xb4 || c == '&' || c == '+') {
                _token += popChar();
            }
            else {
                while ((IsAlpha(peekChar()) || IsDigit(peekChar()) || IsUmlaut(peekChar()))) {
                    _token += popChar();
                }
            }
        }
    }
    else if (IsDigit(c)) {
        _type = TokenType::NUMBER;
        while (IsDigit(peekChar()) || peekChar() == '.') {
            c = popChar();
            if (c == '.') {
                if (_type == TokenType::FLOATNUMBER) {
                    ERR(E_FLOAT);
                }
                _type = TokenType::FLOATNUMBER;
            }
            _token += c;
        }
        if (isalpha(peekChar())) {
            _type = TokenType::UNITNUMBER;
            while (IsAlpha(peekChar()) || IsDigit(peekChar())) {
                _token += popChar();
            }
        }
    }
    else if (IsAlpha(c) || IsUmlaut(c)) {
        if (!bRecursion)
            parseObject();
        else {
            _type = TokenType::IDENTIFIER;
            while (IsAlpha(peekChar()) || IsUmlaut(peekChar())) {
                _token += popChar();
            }
            if (IsDigit(peekChar())) {
                _type = TokenType::UNITNUMBER;
                while (IsAlpha(peekChar()) || IsDigit(peekChar())) {
                    _token += popChar();
                }
            }
        }
    }
    else if (c) {
        _token += popChar();
        ERR2(E_SYNTAX, "Invalides Zeichen: " + to_hex(c));
    }
    while (IsSpace(peekChar()))
        popChar();

    _recurse--;
}

void Expression::parseObject(bool bGetRef)
{
    CObjectPart* pCP;
    Value oVal;
    Value oHVal;
    Value* poContainer = nullptr;
    std::shared_ptr<CObjectPart> pObj(new CObjectPart());
    bool bVarName = false;
    bool bUndefined = false;
    std::string sToken;

    _valueRef = nullptr;
    pCP = pObj.get();
    nextToken(true);
    if (_type == TokenType::VARNAME) {
        sToken = _token;
        bVarName = true;
        if (sToken == "$RETURN" && IsFlag(VF_VERSION2WARNING)) {
            ERRMSG(0, ("%s(%d) : Warnung: Statt $RETURN wird ab Version 2.0 nur noch #return unterstuetzt!\n", _file, _line));
        }
        if (isContainer(_token.c_str(), &poContainer)) {
            pCP->label = _token;
        }
        else if (!bGetRef) {
            if (getValue(_token.c_str(), &oVal)) {
                pCP->label = _token;
                oHVal = oVal;
            }
            else {
                if (_force)
                    bUndefined = true;
            }
        }
        else if (bGetRef) {
            _valueRef = getValueRef(_token.c_str());
            if (!_valueRef && _force)
                bUndefined = true;
        }
        else {
            pCP->label = _token;
            oHVal = Value(_token);
        }
    }
    else {
        pCP->label = _token;
        oHVal = Value(_token);
    }

    while (peekChar() == '[' || peekChar() == '.') {
        nextToken(true);
        if (_token[0] == '[') {
            pCP->bracket = _token[0];
            if (peekChar() == pCP->bracket)
                ERR(E_NOARG);
            do {
                nextToken();
                if (_token[0] == ',')
                    ERR(E_NOARG);
                evalExpr(&oVal);
                if (oVal.getType() == VT_MAP || oVal.getType() == VT_VECTOR)
                    ERR(E_BADINDEX);
                pCP->index.push_back(oVal);
                if (poContainer && pCP->index.size() > 1)
                    ERR(E_NUMARGS);
            } while (_token[0] == ',');
            if (_token[0] != ']')
                ERR(E_UNBALAN);
            // Sind wir in oberster Ebene in einem Container?
            if (poContainer && pCP == pObj.get() && pCP->index.size() == 1 && (bGetRef || peekChar() == '[' || peekChar() == '.' || peekChar() == '(')) {
                const Value* poHelp;
                poHelp = &(poContainer->getAt(oVal));
                if (poHelp->getType() == VT_MAP || poHelp->getType() == VT_VECTOR) {
                    poContainer = const_cast<Value*>(poHelp);
                    pCP->index.clear();
                }
                else {
                    ERR(E_BADINDEX);
                }
            }
        }
        else if (_token[0] == '.') {
            pCP->next = new CObjectPart();
            pCP = pCP->next;
            nextToken(true);
            if (_token[0] == '(') {
                nextToken();
                if (_token[0] == ')')
                    ERR(E_NOARG);
                evalExpr(&oVal);
                pCP->label = oVal.asString();
                if (_token[0] != ')')
                    ERR(E_UNBALAN);
            }
            else if (_type == TokenType::VARNAME) {
                if (getValue(_token.c_str(), &oVal)) {
                    pCP->label = oVal.asString();
                }
                else {
                    ERR(E_UNKNOWN);
                }
            }
            else
                pCP->label = _token;
        }
        else {
            break;
        }
    }

    if (!sToken.empty()) {
        _token = sToken;
    }

    if (poContainer)
        _valueRef = poContainer;
    _container = poContainer;
    _object = pObj;
    _objVal = oHVal;
    _type = bUndefined ? TokenType::VARNAME : TokenType::OBJECT;

    if (!bVarName)
        _valueRef = nullptr;
}

// Zuweisung
int Expression::parseAssignment(Value* r)
{
    Value* poContainer;

    std::string sT;

    if (peekChar() == '=' && peekChar(false, 1) != '=') {
        if (_type != TokenType::OBJECT) {
            if (_type != TokenType::VARNAME)
                r->error("Zuweisung an einen Wert statt einer Referenz.");
            else {
                std::string err;
                err = std::string("Undeklarierte Variable '") + _token + std::string("' verwendet.");
                r->error(err.c_str());
            }
            ERR(E_SYNTAX);
        }

        poContainer = _container;
        std::shared_ptr<CObjectPart> poObject(_object);
        CReference oRef;
        Value oVal;
        sT = _token;
        _assign = true;
        parsePrimary(&oVal, &oRef);
        _assign = false;
        nextToken();
        if (_token.empty() && _type == TokenType::DELIMITER) {
            if (poContainer /*_objVal.getType()==VT_MAP || _objVal.getType()==VT_VECTOR*/) {
                if (poObject->index.empty()) {
                    poContainer->clear();
                }
                else if (poObject->index.size() == 1) {
                    poContainer->remove(poObject->index[0]);
                }
                else {
                    char Buff[512];
                    snprintf(Buff, sizeof(Buff), "Falsche Indizierung im Behaelter '%s'.", poObject->label.c_str());
                    r->error(Buff);
                    ERR(E_BADINDEX);
                }
            }
            else {
                Value oV;
                if (!setValue(sT.c_str(), &oV, _force)) {
                    char Buff[512];
                    snprintf(Buff, sizeof(Buff), "Zuweisung an undefinierte Variable '%s'.", sT.c_str());
                    r->error(Buff);
                    ERR(E_UNKNOWN);
                }
            }
            return 1;
        }
        evalExpr(r);
        if (poContainer) {
            if (poObject->index.size() == 1) {
                if (!poContainer->setAt(poObject->index[0], *r)) {
                    char Buff[512];
                    snprintf(Buff, sizeof(Buff), "Falsche Indizierung im Behaelter '%s'.", poObject->label.c_str());
                    r->error(Buff);
                    ERR(E_BADINDEX);
                }
            }
            else {
                if (poContainer->getType() == r->getType()) {
                    *poContainer = *r;
                }
                else if (poObject->index.size()) {
                    char Buff[512];
                    snprintf(Buff, sizeof(Buff), "Falsche Indizierung im Behaelter '%s'.", poObject->label.c_str());
                    r->error(Buff);
                    ERR(E_BADINDEX);
                }
                else {
                    r->error("Inkompatible Typen in Zuweisung.");
                    ERR(E_VALUE);
                }
            }
        }
        else if (_context && poObject->label == "ARG") {
            if (poObject->index.size() != 1)
                ERR(E_BADINDEX);
            int idx = poObject->index[0].asLong();
            Value oVal2, oRef0;

            getValue("#ARG0", &oVal2);
            getValue("#REF0", &oRef0);

            if (idx < 0 || idx >= oVal2.asLong())
                ERR(E_BADINDEX);
            if (idx < oRef0.asLong()) {
                if (getValue(std::string(std::string("#REF") + ToString((int32_t)(idx + 1))).c_str(), &oVal2)) {
                    if (!setValue(oVal2.asString().c_str(), r))
                        ERR(E_INTERNAL);
                }
                else
                    ERR(E_INTERNAL);
            }
            else {
                if (!setValue(std::string(std::string("#ARG") + ToString((int32_t)(idx + 1))).c_str(), r))
                    ERR(E_INTERNAL);
            }
        }
        else if (sT[0] == '$' && (!poObject->index.empty() || poObject->next)) {
            char Buff[512];
            snprintf(Buff, sizeof(Buff), "Zuweisung ueber Behaelter/Objekt-Zugriff an Variable '%s'.", sT.c_str());
            r->error(Buff);
            ERR(E_BADINDEX);
        }
        else if (sT[0] == '$') {
            if (!setValue(sT.c_str(), r, _force)) {
                char Buff[512];
                snprintf(Buff, sizeof(Buff), "Zuweisung an undefinierte Variable '%s'.", sT.c_str());
                r->error(Buff);
                ERR(E_UNKNOWN);
            }
        }
        else if (oRef.isValid()) {
            oRef.self() = *r;
        }
        else {
            char Buff[512];
            snprintf(Buff, sizeof(Buff), "Fehlerhafte Zuweisung, '%s' ist kein gueltiges Zuweisungsziel.", sT.c_str());
            r->error(Buff);
            ERR(E_SYNTAX);
        }
        return 1;
    }
    evalExpr(r);
    return 0;
}

// Shunting-Yard: evaluates binary-operator expressions.
// parseUnary is called for each operand (it handles unary operators and calls parsePrimary).
void Expression::evalExpr(Value* r)
{
    std::vector<Value> valStack;
    std::vector<std::string> opStack;

    // Apply the top operator in opStack to the top two values in valStack.
    auto applyTop = [&]() {
        std::string op = opStack.back();
        opStack.pop_back();
        // Copy-construct before pop_back() to avoid dangling-reference UB with unique_ptr members.
        Value rhs = valStack.back();
        valStack.pop_back();
        Value lhs = valStack.back();
        valStack.pop_back();
        Value res;

        if (op == "+")
            res = lhs + rhs;
        else if (op == "-")
            res = lhs - rhs;
        else if (op == "*")
            res = lhs * rhs;
        else if (op == "/") {
            res = lhs / rhs;
            if (res.getType() == VT_ERROR)
                throw ExpressionException(res.asString());
        }
        else if (op == "%") {
            res = lhs % rhs;
            if (res.getType() == VT_ERROR)
                throw ExpressionException(res.asString());
        }
        else if (op == "^")
            res = lhs.pow(rhs);
        else if (op == "<")
            res = Value(lhs < rhs ? 1 : 0);
        else if (op == ">")
            res = Value(lhs > rhs ? 1 : 0);
        else if (op == "<=")
            res = Value(lhs <= rhs ? 1 : 0);
        else if (op == ">=")
            res = Value(lhs >= rhs ? 1 : 0);
        else if (op == "==")
            res = Value(lhs == rhs ? 1 : 0);
        else if (op == "!=")
            res = Value(lhs == rhs ? 0 : 1);
        else if (op[0] == '&')  // & or &&
            res = Value((lhs && rhs) ? 1 : 0);
        else if (op[0] == '|')  // | or ||
            res = Value((lhs || rhs) ? 1 : 0);
        else {
            ERR(E_SYNTAX);
        }

        valStack.push_back(res);
    };

    // Returns true when the current token is a binary operator.
    // Guards TokenType::STRING so string-valued tokens are never misread as operators.
    auto isBinOp = [&]() -> bool {
        if (_type == TokenType::STRING)
            return false;
        if (_token.empty())
            return false;
        if (_token[0] == '&' || _token[0] == '|')
            return true;  // & && | ||
        if (_token[0] == '<' || _token[0] == '>')
            return true;  // < <= > >=
        if (_token[0] == '=' && _token[1] == '=')
            return true;  // == (not single =)
        if (_token[0] == '!' && _token[1] == '=')
            return true;  // != (not unary !)
        if (_token[0] == '+' || _token[0] == '-')
            return true;
        if (_token[0] == '*' || _token[0] == '/' || _token[0] == '%')
            return true;
        if (_token[0] == '^')
            return true;
        return false;
    };

    // Parse first primary expression (parseUnary handles unary ops and calls parsePrimary).
    Value first;
    parseUnary(&first);
    valStack.push_back(first);

    // Main Shunting-Yard loop.
    while (isBinOp()) {
        std::string op = _token;

        // Emit legacy warning for single-char & or | (use && / || instead).
        if (IsFlag(VF_VERSION2WARNING) && op.length() == 1 && (op[0] == '&' || op[0] == '|')) {
            ERRMSG(0, ("%s(%d) : Warnung: Ab Version 2.0 wird statt & bzw. | nur noch && bzw. || akzeptiert!\n", _file, _line));
        }

        int prec = static_cast<int>(getPrecedence(op));
        bool rightAssoc = (getAssociativity(op) == ASSOC_RIGHT);

        // Pop operators with higher precedence, or equal precedence when left-associative.
        while (!opStack.empty()) {
            int topPrec = static_cast<int>(getPrecedence(opStack.back()));
            if (topPrec > prec || (topPrec == prec && !rightAssoc))
                applyTop();
            else
                break;
        }

        opStack.push_back(op);
        nextToken();

        // Parse next primary expression.
        Value next;
        parseUnary(&next);
        valStack.push_back(next);
    }

    // Drain remaining operators.
    while (!opStack.empty())
        applyTop();

    *r = valStack.back();
}

// Vorzeichen und Logisches Nicht
void Expression::parseUnary(Value* r)
{
    char o = 0;

    if (_type != TokenType::STRING && (_token[0] == '+' || _token[0] == '-' || _token[0] == '!') && _token.size() == 1) {
        o = _token[0];
        nextToken();
    }
    parsePrimary(r);
    if (o == '-') {
        *r = -*r;
    }
    else if (o == '!') {
        if (r->getType() == VT_STRING) {
            *r = "!" + r->asString();
        }
        else {
            *r = Value(!*r ? 1 : 0);
        }
    }
}

// Variable / Funktionen / Objekte / Klammerungen
void Expression::parsePrimary(Value* r, CReference* pRef)
{
    int i;
    ArgumentList a;

    if (_type != TokenType::STRING && _token[0] == '(') {
        nextToken();
        if (_token[0] == ')')
            ERR(E_NOARG);
        parseAssignment(r);
        if (_token[0] != ')')
            ERR(E_UNBALAN);
        nextToken();
    }
    else if (_type != TokenType::STRING && _token[0] == '[') {
        nextToken();
        Value oArray(VT_VECTOR);
        Value oVal;
        while (_token[0] != ']' || _type != TokenType::DELIMITER) {
            parseAssignment(&oVal);
            oArray.setAt(Value((int32_t)oArray.size()), oVal);
            if (_token[0] == ',' && _type == TokenType::DELIMITER)
                nextToken();
            else if (_token[0] != ']' || _type != TokenType::DELIMITER)
                ERR(E_SYNTAX);
        }
        *r = oArray;
        nextToken();
    }
    else {
        if (_type == TokenType::NUMBER || _type == TokenType::FLOATNUMBER) {
            double fTemp = atof(_token.c_str());
            if (_type == TokenType::NUMBER)
                *r = Value((int32_t)fTemp);
            else
                *r = Value(fTemp);
            nextToken();
        }
        else if (_type == TokenType::STRING) {
            *r = Value(_token);
            nextToken();
        }
        else if (_type == TokenType::OBJECT) {
            if (_container) {
                if (_object->index.size() == 1) {
                    *r = _container->getAt(_object->index[0]);
                    if (r->getType() == VT_ERROR) {
                        ERR(E_VALUE);
                    }
                }
                else if (_object->next && IsEqual(_object->next->label, "size") && !_object->next->next) {
                    *r = _container->size();
                }
                else if (peekChar() == '(') {
                    Value* poContainer = _container;
                    _container = nullptr;
                    nextToken();
                    Value oIdx;
                    nextToken();
                    evalExpr(&oIdx);
                    if (_token[0] != ')' || (oIdx.asLong() < 0 && oIdx.asLong() > poContainer->size())) {
                        char Buff[512];
                        snprintf(Buff, sizeof(Buff), "Falsche Indizierung im Behaelter '%s'.", _object->label.c_str());
                        r->error(Buff);
                        ERR(E_SYNTAX);
                    }
                    *r = poContainer->getNth(oIdx.asLong());
                }
                else if (_object->index.empty()) {
                    *r = *_container;
                }
                else {
                    char Buff[512];
                    snprintf(Buff, sizeof(Buff), "Falsche Indizierung im Behaelter '%s'.", _object->label.c_str());
                    r->error(Buff);
                    ERR(E_SYNTAX);
                }
                nextToken();
                return;
            }
            else if (_object->label == "ARG") {
                if (_object->next) {
                    if (_object->index.empty() && IsEqual(_object->next->label, "SIZE")) {
                        if (!getValue("#ARG0", r))
                            *r = Value(0);
                        nextToken();
                        return;
                    }
                    else {
                        ERR(E_BADINDEX);
                    }
                }
                else {
                    if (_object->index.size() != 1)
                        ERR(E_BADINDEX);
                    int idx = _object->index[0].asLong();
                    Value oVal, oRef0;

                    getValue("#ARG0", &oVal);
                    getValue("#REF0", &oRef0);

                    if (idx < 0 || idx >= oVal.asLong())
                        ERR(E_BADINDEX);
                    if (idx < oRef0.asLong()) {
                        if (getValue(std::string(std::string("#REF") + ToString((int32_t)(idx + 1))).c_str(), &oVal)) {
                            if (!getValue(oVal.asString().c_str(), r))
                                ERR(E_INTERNAL);
                        }
                        else
                            ERR(E_INTERNAL);
                    }
                    else {
                        if (!getValue(std::string(std::string("#ARG") + ToString((int32_t)(idx + 1))).c_str(), r))
                            ERR(E_INTERNAL);
                    }
                    nextToken();
                    return;
                }
            }
            Value oConst(getGlobal(_object->label.c_str()));
            if (oConst.getType() != VT_EMPTY) {
                *r = oConst;
                nextToken();
                return;
            }

            if (!_object->index.empty() || _object->next) {
                // Propagate assignment context to the object handler.
                _object->assign = _assign;
#ifdef USEOBJMAP
                std::map<std::string, Expression::OBJECTS*>::const_iterator oi = g_cpoObjectMap.find(Flatten(_object->label));
                if (oi != g_cpoObjectMap.end()) {
                    if (!_object->next && _object->index.empty())
                        *r = _objVal;
                    else {
                        if (pRef) {
                            Value oR((*oi).second->func(_object.get()));
                            if (oR.getType() == VT_REF)
                                pRef->set(oR);
                            *r = oR;
                        }
                        else {
                            *r = (*oi).second->func(_object.get());
                        }
                    }
                    if (r->getType() == VT_ERROR)
                        throw ExpressionException(r->asString());
                    nextToken();
                    return;
                }
#else
                for (i = 0; Objects[i].name; i++) {
                    if (IsEqual(_object->label, Objects[i].name)) {
                        if (!_object->next && _object->index.empty())
                            *r = _objVal;
                        else {
                            if (pRef) {
                                Value oR(Objects[i].func(_object.get()));
                                if (oR.getType() == VT_REF)
                                    pRef->set(oR);
                                *r = oR;
                            }
                            else {
                                *r = Objects[i].func(_object.get());
                            }
                        }
                        if (r->getType() == VT_ERROR)
                            throw ExpressionException(r->asString());
                        nextToken();
                        return;
                    }
                }
#endif
            }

            if (!_object->label.empty() && _object->label[0] == '$' && (!_object->index.empty() || _object->next)) {
                ERR(E_BADINDEX);
            }

            if (peekChar() == '(') {
                std::string sFName = _token;
                nextToken();
                a.clear();
                do {
                    nextToken();
                    if (a.size() && _type == TokenType::DELIMITER && (_token[0] == ')' || _token[0] == ','))
                        ERR(E_NOARG);
                    if (_type != TokenType::DELIMITER || _token[0] != ')') {
                        a.push_back(Value());
                        parseAssignment(&a[a.size() - 1]);
                        if (a[a.size() - 1].getType() == VT_ERROR)
                            throw ExpressionException(a[a.size() - 1].asString());
                    }
                } while (_token[0] == ',');
                nextToken();

                if (!DoUserFunction(sFName, a, r)) {
#ifdef USEFUNCMAP
                    std::map<std::string, Expression::FUNCTION*>::const_iterator fi = g_cpoFunctionMap.find(sFName);
                    if (fi != g_cpoFunctionMap.end()) {
                        if (a.size() != (*fi).second->args) {
                            ERR(E_NUMARGS);
                        }
                        if ((*fi).second->risc && IsFlag(VF_RESTRICTED)) {
                            ERR(E_RESTRICTED);
                        }
                        *r = (*fi).second->func(this, a);
                        if (r->getType() == VT_ERROR)
                            throw ExpressionException(r->asString());
                    }
                    else {
                        ERRX(std::string("Unbekannte Funktion: ") + sFName);
                    }
#else
                    for (i = 0; Funcs[i].name; i++) {
                        if (IsEqual(sFName.c_str(), Funcs[i].name)) {
                            if (a.size() != Funcs[i].args) {
                                ERR(E_NUMARGS);
                            }
                            if (Funcs[i].risc && IsFlag(VF_RESTRICTED)) {
                                ERR(E_RESTRICTED);
                            }
                            *r = Funcs[i].func(this, a);
                            if (r->getType() == VT_ERROR)
                                throw ExpressionException(r->asString());
                            return;
                        }
                    }
                    if (!Funcs[i].name) {
                        ERRX(std::string("Unbekannte Funktion: ") + sFName);
                    }
#endif
                }
            }
            else {
                if (!_object->next && _object->index.empty())
                    *r = _objVal;
                else {
                    *r = CBlockBase::GetCfgObjValue(_object.get());
                    if (r->getType() == VT_ERROR)
                        throw ExpressionException(r->asString());
                }
                nextToken();
            }
        }
        else if (_type == TokenType::VARNAME) {
            if (!getValue(_token.c_str(), r))
                ERR(E_UNKNOWN);
            nextToken();
        }
        else if (_type == TokenType::UNITNUMBER || _type == TokenType::IDENTIFIER) {
            *r = Value(_token);
            nextToken();
        }
        else
            ERR(E_SYNTAX);
    }
}

int Expression::evaluate(Variables& oContext, const char* e, Value* result, int* a, bool bForce, bool bExpectInplace)
{
    _context = &oContext;
    _force = bForce;
    _expectInplace = bExpectInplace;

    try {
        _exp = e;
        _expWork = e;
        _pos = 0;
        _isExp = _expWork.begin();
        g_pcERANC = e;
        *result = Value();
        nextToken();
        if (_token.empty() && _type != TokenType::STRING)
            ERR(E_EMPTY);
        _assign = false;
        *a = parseAssignment(result);
        _assign = false;
        if (result->getType() == VT_ERROR) {
            result->error(result->asString().c_str());
            return E_VALUE;
        }
        if (_type != TokenType::NONE && _type != TokenType::DELIMITER)
            ERR(E_SYNTAX);
    }
    catch (ExpressionException E) {
        _assign = false;
        if (result->getType() != VT_ERROR) {
            if (E.num < 0)
                result->error(E.text.c_str());
            else
                result->error((g_apcErrorText[E.num] + std::string(" ") + E.text).c_str());
        }
        return (E.num);
        /*
                        const char* str = e;
                        while( *str )
                        {
                                if( !IsAlNum(*str) ) break;
                                str++;
                        }
                        if( *str )
                        {
                                result->error( g_apcErrorText[E.num] );
                                return( E.num );
                        }
                        *result = Value( e );
        */
    }
    return E_OK;
}

std::shared_ptr<CObjectPart> Expression::parseObjectAccess(const std::string& sObjAcc /*, int nRecurse, Value** poContainer*/, Variables* poContext)
{
    Variables oContext;
    Value oVal;
    Expression oExp(sObjAcc, "<internal>", 0);
    oExp._recurse = 0;  // nRecurse;
    oExp._context = poContext ? poContext : &oContext;
    oExp.nextToken();
    /*
    if (poContainer)
        *poContainer = oExp._container;
    */
    return oExp._object;
}

Value* Expression::getObjectReference(Expression::Variables& oContext, const std::string& sObj)
{
    Expression oExp(sObj, "<internal>", 0);
    oExp._recurse = 0;
    oExp._context = &oContext;
    oExp._valueRef = 0;
    oExp.parseObject(true);
    return oExp._valueRef;
}

Expression::Precedence Expression::getPrecedence(const std::string& token)
{
    if (token == "&" || token == "|" || token == "&&" || token == "||")
        return PREC_LOGICAL;
    if (token == "<" || token == ">" || token == "<=" || token == ">=" || token == "==" || token == "!=")
        return PREC_COMPARISON;
    if (token == "+" || token == "-")
        return PREC_ADDITIVE;
    if (token == "*" || token == "/" || token == "%")
        return PREC_MULTIPLICATIVE;
    if (token == "^")
        return PREC_EXPONENT;
    if (token == "!" || token == "UNARY_MINUS")
        return PREC_UNARY;
    return PREC_NONE;
}

Expression::Associativity Expression::getAssociativity(const std::string& token)
{
    if (token == "^")
        return ASSOC_RIGHT;
    if (token == "!" || token == "UNARY_MINUS")
        return ASSOC_RIGHT;
    return ASSOC_LEFT;
}
