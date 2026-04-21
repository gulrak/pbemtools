/****************************************************************************
 *   $Source: f:\\SourceArchive/EresseaTools/Vorlage/Value.cpp,v $
 *   $Author: S.Schuemann $
 *     $Date: 2000/02/24 09:56:47 $
 * $Revision: 1.4 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Klassen fuer die Ausdrucksauswertung in Metabefehlen
 *****************************************************************************
 * $Log: Value.cpp,v $
 * Revision 1.4  2000/02/24 09:56:47  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
 *
 * Revision 1.3  1999/11/28 17:38:41  S.Schuemann
 * - Mannigfaltige Aenderungen fuer Vorlage V1.4 beta 9
 *
 * Revision 1.2  1999/10/24 08:02:18  S.Schuemann
 * - Anpassungen fuer 1.4 b 4
 * - CReference als Value eingefuehrt
 * - Unterprogramme mit #proc und #call implementiert
 *
 * Revision 1.1.1.1  1999/09/20 14:55:45  Steffen
 * - Initial CVS-checkin;
 * - Basierend auf dem Stand von Vorlage V1.3b6 gesaeubert und aufgeteilt;
 * - Fehler in Kapazitaetsberechnung behoben;
 *
 *****************************************************************************/

#include "Value.h"
#include <fmt/format.h>
#include <cmath>
#include "Utility.h"

#define EPSILON 0.00001
#define ROUND(f) (f < 0 ? f - EPSILON : f + EPSILON)
#define TRUNC(f) ((int32_t)(f < 0 ? f - EPSILON : f + EPSILON))

int32_t Value::_cnt = 0;
int32_t g_nContainerLimit = 0x7fffffffL;

#ifdef _DEBUG

int32_t Value::_idCnt = 0;
std::map<int32_t, Value*> Value::_valPool;

#ifdef _VALDEBUG

#define REGVALUE  \
    _id = _idCnt; \
    _valPool[_idCnt++] = this
#define UNREGVALUE _valPool.erase(_id)

#else

#define REGVALUE
#define UNREGVALUE

#endif

void Value::dumpPool()
{
    for (auto i = _valPool.begin(); i != _valPool.end(); i++) {
        TRACEMSG(("%5d - %s\n", (*i).first, (*i).second->asString().c_str()));
    }
}

#else

#define REGVALUE
#define UNREGVALUE

#endif

#define MAX_FLOAT 1e50
#define MAX_FLOAT_LENGTH 64

inline double rangeCheck(double val)
{
    if (fabs(val) > MAX_FLOAT) {
        return val < 0 ? -MAX_FLOAT : MAX_FLOAT;
    }
    return val;
}

// Helper: deep-copy a Data variant. unique_ptr alternatives are deep-copied;
// all other alternatives are value-copied. The variant's own copy constructor
// is deleted (because of unique_ptr members), so we use std::visit.
static Value::Data copyData(const Value::Data& src)
{
    return std::visit(
        [](const auto& v) -> Value::Data {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<std::vector<Value>>>) {
                return std::make_unique<std::vector<Value>>(*v);
            }
            else if constexpr (std::is_same_v<T, std::unique_ptr<std::map<std::string, Value>>>) {
                return std::make_unique<std::map<std::string, Value>>(*v);
            }
            else {
                return v;  // copy monostate, ErrorState, int32_t, double, string, RefPtr
            }
        },
        src);
}

// -----------------------------------------------------------------------------
// Constructors / destructor
// -----------------------------------------------------------------------------

Value::Value()
    : _data(std::monostate{})
    , _protectType(false)
{
    REGVALUE;
    _cnt++;
}

Value::Value(const Value& oVal, bool bRef)
    : _protectType(false)
{
    REGVALUE;
    if (bRef) {
        _data = detail::RefPtr{const_cast<Value*>(&oVal.cself())};
    }
    else if (std::holds_alternative<detail::RefPtr>(oVal._data)) {
        // Copy of a ref = ref with the same target (not a deep copy).
        // Assign the RefPtr value directly (not the whole variant) to avoid
        // triggering the deleted variant copy-assignment operator.
        _data = std::get<detail::RefPtr>(oVal._data);
    }
    else {
        _data = copyData(oVal.cself()._data);
    }
    _cnt++;
}

Value::Value(Value&& oVal) noexcept
    : _data(std::move(oVal._data))
    , _protectType(oVal._protectType)
{
    REGVALUE;
    oVal._data = std::monostate{};
    _cnt++;
}

Value::Value(double fVal)
    : _data(rangeCheck(fVal))
    , _protectType(false)
{
    REGVALUE;
    _cnt++;
}

Value::Value(int32_t nVal)
    : _data(nVal)
    , _protectType(false)
{
    REGVALUE;
    _cnt++;
}

Value::Value(const char* pcVal)
    : _data(std::string(pcVal))
    , _protectType(false)
{
    REGVALUE;
    _cnt++;
}

Value::Value(std::string sVal)
    : _data(std::move(sVal))
    , _protectType(false)
{
    REGVALUE;
    _cnt++;
}

Value::Value(TYPE enType)
    : _protectType(false)
{
    REGVALUE;
    switch (enType) {
        case VT_VECTOR:
            _data = std::make_unique<std::vector<Value>>();
            break;
        case VT_MAP:
            _data = std::make_unique<std::map<std::string, Value>>();
            break;
        case VT_ERROR:
            _data = detail::ErrorState{};
            break;
        case VT_REF:
            _data = detail::RefPtr{nullptr};
            break;
        default:
            _data = std::monostate{};  // VT_EMPTY and others
            break;
    }
    _cnt++;
}

Value::Value(const CValArray& arr)
    : _protectType(false)
{
    REGVALUE;
    auto vec = std::make_unique<std::vector<Value>>(arr.begin(), arr.end());
    _data = std::move(vec);
    _cnt++;
}

Value::~Value()
{
    UNREGVALUE;
    _cnt--;
}

// -----------------------------------------------------------------------------
// Type query
// -----------------------------------------------------------------------------

ValueType Value::getType() const noexcept
{
    static constexpr ValueType types[] = {VT_EMPTY, VT_ERROR, VT_INT, VT_FLOAT, VT_STRING, VT_REF, VT_VECTOR, VT_MAP};
    return types[_data.index()];
}

// -----------------------------------------------------------------------------
// Value accessors
// -----------------------------------------------------------------------------

int32_t Value::asLong() const noexcept
{
    const Value& t = cself();
    if (const auto* v = std::get_if<int32_t>(&t._data))
        return *v;
    if (const auto* v = std::get_if<double>(&t._data))
        return static_cast<int32_t>(*v);
    return 0;
}

double Value::asReal() const noexcept
{
    const Value& t = cself();
    if (const auto* v = std::get_if<double>(&t._data))
        return *v;
    if (const auto* v = std::get_if<int32_t>(&t._data))
        return static_cast<double>(*v);
    return 0.0;
}

void Value::error(const char* pcMsg)
{
    self()._data = detail::ErrorState{pcMsg};
}

std::string Value::asString(bool bForceLiteral) const
{
    const Value& t = cself();
    const TYPE type = t.getType();

    if (type == VT_INT) {
        return std::to_string(std::get<int32_t>(t._data));
    }
    if (type == VT_FLOAT) {
        const double f = std::get<double>(t._data);
        if (fabs(f) >= MAX_FLOAT)
            return f < 0 ? std::string("-1.#INF") : std::string("1.#INF");
        return fmt::format("{:.3f}", ROUND(f));
    }
    if (type == VT_ERROR) {
        return std::get<detail::ErrorState>(t._data).message;
    }
    if (type == VT_MAP) {
        return fmt::format("[{} Elements]", t.size());
    }
    if (type == VT_VECTOR) {
        if (bForceLiteral) {
            std::string sErg("[");
            const int32_t n = t.size();
            for (int32_t i = 0; i < n; i++) {
                sErg += t.getAt(Value(i)).asString(true);
                sErg += (i < n - 1) ? "," : "]";
            }
            return sErg;
        }
        return fmt::format("[{} Elements]", t.size());
    }
    if (type == VT_STRING) {
        return std::get<std::string>(t._data);
    }
    // VT_EMPTY, VT_REF (shouldn't reach REF via cself)
    return {};
}

// -----------------------------------------------------------------------------
// Assignment
// -----------------------------------------------------------------------------

const Value& Value::operator=(const Value& oVal)
{
    static Value oErr;

    if (&oVal == this)
        return *this;

    if (_protectType && cself().getType() != oVal.cself().getType()) {
        oErr.error("Zuweisung an inkompatible Variable!");
        return oErr;
    }

    self()._data = copyData(oVal.cself()._data);
    return *this;
}

// -----------------------------------------------------------------------------
// Container operations
// -----------------------------------------------------------------------------

bool Value::setAt(const Value& oKey, const Value& oVal)
{
    const TYPE enType = cself().getType();
    const TYPE enKType = oKey.cself().getType();

    if (enKType == VT_VECTOR || enKType == VT_MAP)
        return false;

    if (enType == VT_MAP) {
        auto& map = *std::get<std::unique_ptr<std::map<std::string, Value>>>(cself()._data);
        if (oVal.cself().getType() == VT_EMPTY) {
            map.erase(oKey.cself().asString());
        }
        else {
            map[oKey.cself().asString()] = oVal;
            if (map.size() == size_t(g_nContainerLimit)) {
                ERRMSG(0, ("Warnung: Ueberschreiten des Behaelterlimits!"));
            }
        }
        return true;
    }
    if (enType == VT_VECTOR) {
        if (enKType != VT_INT && enKType != VT_FLOAT) {
            ERRMSG(0, ("FEHLER: Nichtnumerischer Wert als Index fuer Array benutzt!"));
        }
        auto& vec = *std::get<std::unique_ptr<std::vector<Value>>>(cself()._data);
        const size_t nIdx = size_t(oKey.cself().asLong());
        if (nIdx > vec.size())
            return false;
        if (nIdx == vec.size()) {
            vec.push_back(oVal);
            if (vec.size() == size_t(g_nContainerLimit)) {
                ERRMSG(0, ("Warnung: Ueberschreiten des Behaelterlimits!"));
            }
        }
        else {
            vec[nIdx] = oVal;
        }
        return true;
    }
    return false;
}

Value& Value::getAt(const Value& oKey) const
{
    static Value oEmpty;
    static Value oErr;

    const TYPE enType = cself().getType();
    const TYPE enKType = oKey.cself().getType();

    if (enType == VT_ERROR) {
        oErr = cself();
        return oErr;
    }
    if (enKType == VT_ERROR) {
        oErr = oKey.cself();
        return oErr;
    }
    if (enKType == VT_VECTOR || enKType == VT_MAP) {
        oErr.error("Behaelter als Index/Schluessel verwendet.");
        return oErr;
    }

    if (enType == VT_VECTOR) {
        if (enKType != VT_INT && enKType != VT_FLOAT) {
            ERRMSG(0, ("FEHLER: Nichtnumerischer Wert als Index fuer Array benutzt!"));
        }
        const auto& vec = *std::get<std::unique_ptr<std::vector<Value>>>(cself()._data);
        const size_t i = size_t(oKey.cself().asLong());
        if (i < vec.size())
            return const_cast<Value&>(vec[i]);
        // Out-of-range: original code fell through to return oEmpty (not oErr)
    }
    if (enType == VT_MAP) {
        auto& map = *std::get<std::unique_ptr<std::map<std::string, Value>>>(cself()._data);
        auto it = map.find(oKey.cself().asString());
        if (it != map.end())
            return it->second;
    }
    return oEmpty;
}

Value Value::getNth(int32_t nIdx) const
{
    if (cself().getType() == VT_ERROR)
        return Value();
    if (nIdx >= 0) {
        if (cself().getType() == VT_VECTOR) {
            const auto& vec = *std::get<std::unique_ptr<std::vector<Value>>>(cself()._data);
            if (size_t(nIdx) < vec.size())
                return Value(nIdx);
        }
        else if (cself().getType() == VT_MAP) {
            const auto& map = *std::get<std::unique_ptr<std::map<std::string, Value>>>(cself()._data);
            auto it = map.begin();
            int cnt = 0;
            while (it != map.end() && cnt < nIdx) {
                ++it;
                ++cnt;
            }
            if (it != map.end())
                return Value(it->first);
        }
    }
    return Value();
}

void Value::remove(const Value& oKey)
{
    if (cself().getType() == VT_MAP) {
        auto& map = *std::get<std::unique_ptr<std::map<std::string, Value>>>(cself()._data);
        map.erase(oKey.cself().asString());
    }
    else if (cself().getType() == VT_VECTOR) {
        if (oKey.cself().getType() != VT_INT && oKey.cself().getType() != VT_FLOAT) {
            ERRMSG(0, ("FEHLER: Nichtnumerischer Wert als Index fuer Array benutzt!"));
        }
        auto& vec = *std::get<std::unique_ptr<std::vector<Value>>>(cself()._data);
        const size_t nIdx = size_t(oKey.cself().asLong());
        if (nIdx < vec.size())
            vec.erase(vec.begin() + int32_t(nIdx));
    }
}

void Value::clear()
{
    if (cself().getType() == VT_MAP) {
        std::get<std::unique_ptr<std::map<std::string, Value>>>(cself()._data)->clear();
    }
    if (cself().getType() == VT_VECTOR) {
        std::get<std::unique_ptr<std::vector<Value>>>(cself()._data)->clear();
    }
}

int32_t Value::size() const
{
    const TYPE type = cself().getType();
    if (type == VT_MAP)
        return int32_t(std::get<std::unique_ptr<std::map<std::string, Value>>>(cself()._data)->size());
    if (type == VT_VECTOR)
        return int32_t(std::get<std::unique_ptr<std::vector<Value>>>(cself()._data)->size());
    if (type == VT_STRING)
        return int32_t(std::get<std::string>(cself()._data).size());
    return -1;
}

void Value::swap(Value& oVal)
{
    const TYPE ta = cself().getType();
    const TYPE tb = oVal.cself().getType();
    const bool aHasPtr = (ta == VT_REF || ta == VT_VECTOR || ta == VT_MAP);
    const bool bHasPtr = (tb == VT_REF || tb == VT_VECTOR || tb == VT_MAP);

    if (aHasPtr && bHasPtr) {
        std::swap(self()._data, oVal.self()._data);
    }
    else {
        Value oTemp(oVal);
        oVal = *this;
        *this = oTemp;
    }
}

// -----------------------------------------------------------------------------
// Arithmetic operators
// -----------------------------------------------------------------------------

Value Value::operator+(const Value& oVal) const
{
    Value oE(*this);

    if (oVal.cself().getType() == VT_ERROR)
        oE.error(std::get<detail::ErrorState>(oVal.cself()._data).message.c_str());
    if (cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE lhs = cself().getType();
    const TYPE rhs = oVal.cself().getType();

    switch (rhs) {
        case VT_INT:
            switch (lhs) {
                case VT_INT:
                    std::get<int32_t>(oE.self()._data) += std::get<int32_t>(oVal.cself()._data);
                    break;
                case VT_FLOAT: {
                    double f = rangeCheck(std::get<double>(oE.self()._data) + std::get<int32_t>(oVal.cself()._data));
                    oE.self()._data = f;
                } break;
                case VT_STRING:
                    std::get<std::string>(oE.self()._data) += fmt::format("{}", std::get<int32_t>(oVal.cself()._data));
                    break;
                default:
                    break;
            }
            break;
        case VT_FLOAT:
            switch (lhs) {
                case VT_INT:
                    std::get<int32_t>(oE.self()._data) += TRUNC(std::get<double>(oVal.cself()._data));
                    break;
                case VT_FLOAT: {
                    double f = rangeCheck(std::get<double>(oE.self()._data) + std::get<double>(oVal.cself()._data));
                    oE.self()._data = f;
                } break;
                case VT_STRING:
                    std::get<std::string>(oE.self()._data) += oVal.asString();
                    break;
                default:
                    break;
            }
            break;
        case VT_STRING:
            switch (lhs) {
                case VT_INT:
                case VT_FLOAT:
                    oE.error("String kann nicht in numerischen Wert gewandelt werden.");
                    break;
                case VT_STRING:
                    std::get<std::string>(oE.self()._data) += std::get<std::string>(oVal.cself()._data);
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    return oE;
}

Value Value::operator-(const Value& oVal) const
{
    Value oE(*this);

    if (oVal.cself().getType() == VT_ERROR)
        oE.error(std::get<detail::ErrorState>(oVal.cself()._data).message.c_str());
    if (cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE lhs = cself().getType();
    const TYPE rhs = oVal.cself().getType();

    switch (rhs) {
        case VT_INT:
            switch (lhs) {
                case VT_INT:
                    std::get<int32_t>(oE.self()._data) -= std::get<int32_t>(oVal.cself()._data);
                    break;
                case VT_FLOAT: {
                    double f = std::get<double>(oE.self()._data) - std::get<int32_t>(oVal.cself()._data);
                    oE.self()._data = f;
                } break;
                case VT_STRING:
                    // Preserved original (copy-paste) behaviour: string - int appends the int
                    std::get<std::string>(oE.self()._data) += fmt::format("{}", std::get<int32_t>(oVal.cself()._data));
                    break;
                default:
                    break;
            }
            break;
        case VT_FLOAT:
            switch (lhs) {
                case VT_INT:
                    std::get<int32_t>(oE.self()._data) -= TRUNC(std::get<double>(oVal.cself()._data));
                    break;
                case VT_FLOAT: {
                    double f = std::get<double>(oE.self()._data) - std::get<double>(oVal.cself()._data);
                    oE.self()._data = f;
                } break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Subtraktionen verwendet werden.");
                    break;
                default:
                    break;
            }
            break;
        case VT_STRING:
            oE.error("Strings koennen nicht in Subtraktionen verwendet werden.");
            break;
        default:
            break;
    }
    return oE;
}

Value Value::operator*(const Value& oVal) const
{
    Value oE(*this);

    if (oVal.cself().getType() == VT_ERROR)
        oE.error(std::get<detail::ErrorState>(oVal.cself()._data).message.c_str());
    if (cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE lhs = cself().getType();
    const TYPE rhs = oVal.cself().getType();

    switch (rhs) {
        case VT_INT:
            switch (lhs) {
                case VT_INT:
                    std::get<int32_t>(oE.self()._data) *= std::get<int32_t>(oVal.cself()._data);
                    break;
                case VT_FLOAT: {
                    double f = rangeCheck(std::get<double>(oE.self()._data) * std::get<int32_t>(oVal.cself()._data));
                    oE.self()._data = f;
                } break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Multiplikationen verwendet werden.");
                    break;
                default:
                    break;
            }
            break;
        case VT_FLOAT:
            switch (lhs) {
                case VT_INT:
                    std::get<int32_t>(oE.self()._data) *= TRUNC(std::get<double>(oVal.cself()._data));
                    break;
                case VT_FLOAT: {
                    double f = rangeCheck(std::get<double>(oE.self()._data) * std::get<double>(oVal.cself()._data));
                    oE.self()._data = f;
                } break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Multiplikationen verwendet werden.");
                    break;
                default:
                    break;
            }
            break;
        case VT_STRING:
            oE.error("Strings koennen nicht in Multiplikationen verwendet werden.");
            break;
        default:
            break;
    }
    return oE;
}

Value Value::operator/(const Value& oVal) const
{
    Value oE(*this);

    if (oVal.cself().getType() == VT_ERROR)
        oE.error(std::get<detail::ErrorState>(oVal.cself()._data).message.c_str());
    if (cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE lhs = cself().getType();
    const TYPE rhs = oVal.cself().getType();

    switch (rhs) {
        case VT_INT: {
            const int32_t divisor = std::get<int32_t>(oVal.cself()._data);
            switch (lhs) {
                case VT_INT:
                    if (!divisor)
                        oE.error("Division durch Null.");
                    else
                        std::get<int32_t>(oE.self()._data) /= divisor;
                    break;
                case VT_FLOAT:
                    if (!divisor)
                        oE.error("Division durch Null.");
                    else {
                        double f = rangeCheck(std::get<double>(oE.self()._data) / divisor);
                        oE.self()._data = f;
                    }
                    break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Divisionen verwendet werden.");
                    break;
                default:
                    break;
            }
        } break;
        case VT_FLOAT: {
            const double divisor = std::get<double>(oVal.cself()._data);
            switch (lhs) {
                case VT_INT:
                    if (fabs(divisor) < EPSILON)
                        oE.error("Division durch Null.");
                    else
                        std::get<int32_t>(oE.self()._data) /= TRUNC(divisor);
                    break;
                case VT_FLOAT:
                    if (fabs(divisor) < EPSILON)
                        oE.error("Division durch Null.");
                    else {
                        double f = rangeCheck(std::get<double>(oE.self()._data) / divisor);
                        oE.self()._data = f;
                    }
                    break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Divisionen verwendet werden.");
                    break;
                default:
                    break;
            }
        } break;
        case VT_STRING:
            oE.error("Strings koennen nicht in Divisionen verwendet werden.");
            break;
        default:
            break;
    }
    return oE;
}

Value Value::operator%(const Value& oVal) const
{
    Value oE(*this);

    if (oVal.cself().getType() == VT_ERROR)
        oE.error(std::get<detail::ErrorState>(oVal.cself()._data).message.c_str());
    if (cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE lhs = cself().getType();
    const TYPE rhs = oVal.cself().getType();

    switch (rhs) {
        case VT_INT: {
            const int32_t divisor = std::get<int32_t>(oVal.cself()._data);
            switch (lhs) {
                case VT_INT:
                    if (!divisor)
                        oE.error("Division durch Null.");
                    else
                        std::get<int32_t>(oE.self()._data) %= divisor;
                    break;
                case VT_FLOAT:
                    if (!divisor)
                        oE.error("Division durch Null.");
                    else {
                        double f = double(int32_t(std::get<double>(oE.self()._data)) % divisor);
                        oE.self()._data = f;
                    }
                    break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Divisionen verwendet werden.");
                    break;
                default:
                    break;
            }
        } break;
        case VT_FLOAT: {
            const double divisor = std::get<double>(oVal.cself()._data);
            switch (lhs) {
                case VT_INT:
                    if (fabs(divisor) < EPSILON)
                        oE.error("Division durch Null.");
                    else
                        std::get<int32_t>(oE.self()._data) %= TRUNC(divisor);
                    break;
                case VT_FLOAT:
                    if (fabs(divisor) < EPSILON)
                        oE.error("Division durch Null.");
                    else {
                        double f = double(int32_t(std::get<double>(oE.self()._data)) % TRUNC(divisor));
                        oE.self()._data = f;
                    }
                    break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Modulooperationen verwendet werden.");
                    break;
                default:
                    break;
            }
        } break;
        case VT_STRING:
            oE.error("Strings koennen nicht in Modulooperation verwendet werden.");
            break;
        default:
            break;
    }
    return oE;
}

Value Value::operator-() const
{
    Value oE(*this);

    if (cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE type = cself().getType();
    if (type == VT_INT) {
        std::get<int32_t>(oE.self()._data) = -std::get<int32_t>(cself()._data);
    }
    else if (type == VT_FLOAT) {
        std::get<double>(oE.self()._data) = -std::get<double>(cself()._data);
    }
    else {
        oE.error("Negierung eines Strings nicht moeglich.");
    }
    return oE;
}

Value Value::pow(const Value& oVal) const
{
    Value oE(*this);

    if (oVal.cself().getType() == VT_ERROR)
        oE.error(std::get<detail::ErrorState>(oVal.cself()._data).message.c_str());
    if (cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        oE.error("Operation mit typlosem Operanden.");
    if (oE.self().getType() == VT_ERROR)
        return oE;

    const TYPE lhs = cself().getType();
    const TYPE rhs = oVal.cself().getType();

    switch (rhs) {
        case VT_INT: {
            const int32_t exp = std::get<int32_t>(oVal.cself()._data);
            switch (lhs) {
                case VT_INT:
                    oE.self()._data = int32_t(::pow(double(std::get<int32_t>(cself()._data)), exp));
                    break;
                case VT_FLOAT:
                    oE.self()._data = rangeCheck(::pow(std::get<double>(cself()._data), exp));
                    break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Multiplikationen verwendet werden.");
                    break;
                default:
                    break;
            }
        } break;
        case VT_FLOAT: {
            const double exp = std::get<double>(oVal.cself()._data);
            switch (lhs) {
                case VT_INT:
                    oE.self()._data = int32_t(::pow(double(std::get<int32_t>(cself()._data)), TRUNC(exp)));
                    break;
                case VT_FLOAT:
                    oE.self()._data = rangeCheck(::pow(std::get<double>(cself()._data), exp));
                    break;
                case VT_STRING:
                    oE.error("Strings koennen nicht in Multiplikationen verwendet werden.");
                    break;
                default:
                    break;
            }
        } break;
        case VT_STRING:
            oE.error("Strings koennen nicht in Multiplikationen verwendet werden.");
            break;
        default:
            break;
    }
    return oE;
}

// -----------------------------------------------------------------------------
// Comparison operators
// -----------------------------------------------------------------------------

static bool AlmostEqualRelativeOrAbsolute(double A, double B, double maxRelativeError = 0.000001, double maxAbsoluteError = 0.000001)
{
    if (fabs(A - B) < maxAbsoluteError)
        return true;
    double relativeError;
    if (fabs(B) > fabs(A))
        relativeError = fabs((A - B) / B);
    else
        relativeError = fabs((A - B) / A);
    return relativeError <= maxRelativeError;
}

bool Value::operator==(const Value& oVal) const
{
    const TYPE lt = cself().getType();
    const TYPE rt = oVal.cself().getType();

    if (lt == VT_ERROR || rt == VT_ERROR || lt == VT_EMPTY || rt == VT_EMPTY)
        return false;

    if (lt == VT_STRING || rt == VT_STRING)
        return cself().asString() == oVal.cself().asString();

    if ((lt == VT_INT || lt == VT_FLOAT) && (rt == VT_INT || rt == VT_FLOAT))
        return AlmostEqualRelativeOrAbsolute(cself().asReal(), oVal.cself().asReal());

    return false;
}

bool Value::operator<(const Value& oVal) const
{
    const TYPE lt = cself().getType();
    const TYPE rt = oVal.cself().getType();

    if (lt == VT_ERROR || rt == VT_ERROR || lt == VT_EMPTY || rt == VT_EMPTY)
        return false;

    if (lt == VT_STRING || rt == VT_STRING)
        return cself().asString() < oVal.cself().asString();

    if ((lt == VT_INT || lt == VT_FLOAT) && (rt == VT_INT || rt == VT_FLOAT))
        return cself().asReal() < oVal.cself().asReal();

    return false;
}

bool Value::operator>(const Value& oVal) const
{
    const TYPE lt = cself().getType();
    const TYPE rt = oVal.cself().getType();

    if (lt == VT_ERROR || rt == VT_ERROR || lt == VT_EMPTY || rt == VT_EMPTY)
        return false;

    if (lt == VT_STRING || rt == VT_STRING)
        return cself().asString() > oVal.cself().asString();

    if ((lt == VT_INT || lt == VT_FLOAT) && (rt == VT_INT || rt == VT_FLOAT))
        return cself().asReal() > oVal.cself().asReal();

    return false;
}

bool Value::operator<=(const Value& oVal) const
{
    const TYPE lt = cself().getType();
    const TYPE rt = oVal.cself().getType();

    if (lt == VT_ERROR || rt == VT_ERROR || lt == VT_EMPTY || rt == VT_EMPTY)
        return false;

    if (lt == VT_STRING || rt == VT_STRING)
        return cself().asString() <= oVal.cself().asString();

    if ((lt == VT_INT || lt == VT_FLOAT) && (rt == VT_INT || rt == VT_FLOAT))
        return cself().asReal() <= oVal.cself().asReal();

    return false;
}

bool Value::operator>=(const Value& oVal) const
{
    const TYPE lt = cself().getType();
    const TYPE rt = oVal.cself().getType();

    if (lt == VT_ERROR || rt == VT_ERROR || lt == VT_EMPTY || rt == VT_EMPTY)
        return false;

    if (lt == VT_STRING || rt == VT_STRING)
        return cself().asString() >= oVal.cself().asString();

    if ((lt == VT_INT || lt == VT_FLOAT) && (rt == VT_INT || rt == VT_FLOAT))
        return cself().asReal() >= oVal.cself().asReal();

    return false;
}

// -----------------------------------------------------------------------------
// Logical operators
// -----------------------------------------------------------------------------

bool Value::operator&&(const Value& oVal) const
{
    if (cself().getType() == VT_ERROR || oVal.cself().getType() == VT_ERROR || cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        return false;

    const int32_t a = (cself().getType() == VT_STRING) ? !std::get<std::string>(cself()._data).empty() : asLong();
    const int32_t b = (oVal.cself().getType() == VT_STRING) ? !std::get<std::string>(oVal.cself()._data).empty() : oVal.asLong();
    return a && b;
}

bool Value::operator||(const Value& oVal) const
{
    if (cself().getType() == VT_ERROR || oVal.cself().getType() == VT_ERROR || cself().getType() == VT_EMPTY || oVal.cself().getType() == VT_EMPTY)
        return false;

    const int32_t a = (cself().getType() == VT_STRING) ? !std::get<std::string>(cself()._data).empty() : asLong();
    const int32_t b = (oVal.cself().getType() == VT_STRING) ? !std::get<std::string>(oVal.cself()._data).empty() : oVal.asLong();
    return a || b;
}

bool Value::operator!() const
{
    if (cself().getType() == VT_ERROR || cself().getType() == VT_EMPTY)
        return false;

    if (cself().getType() == VT_STRING)
        return std::get<std::string>(cself()._data).empty();

    return asLong() == 0;
}
