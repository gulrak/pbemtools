/****************************************************************************
 *   $Source: D:\\Development\\Repository/ETools/EBase/Value.h,v $
 *   $Author: ssh $
 *     $Date: 2003/07/01 09:39:30 $
 * $Revision: 1.1 $
 *    $State: Exp $
 * Copyright: (c) Copyright 1999 by S.Schuemann
 *   Project: Eressea-Tools
 *     Zweck: Klassen fuer die Ausdrucksauswertung in Metabefehlen
 *****************************************************************************
 * $Log: Value.h,v $
 * Revision 1.1  2003/07/01 09:39:30  ssh
 * *** empty log message ***
 *
 * Revision 1.1  2003/07/01 09:13:41  ssh
 * Initial recvsing of Source...
 *
 * Revision 1.3  2000/02/24 09:56:47  S.Schuemann
 * Diverse Aenderungen auf dem Pfad zur Vorlage V1.4 beta 10c
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
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

using namespace std;

class CObjectPart;
class Value;
class CValArray;

// template<class T>  f(T t) {...};
#ifdef THIS
#undef THIS
#endif

enum ValueType { VT_EMPTY, VT_ERROR, VT_INT, VT_FLOAT, VT_STRING, VT_REF, VT_VECTOR, VT_MAP };

// Internal storage types for the variant.
// Kept in a namespace to avoid polluting the global scope.
namespace detail {
struct ErrorState
{
    std::string message;
};

struct RefPtr
{
    Value* ptr;
};
}  // namespace detail

class Value
{
public:
    using TYPE = ValueType;

    // The variant alternatives are ordered to match the ValueType enum values,
    // so that _data.index() maps directly to ValueType.
    using Data = std::variant<std::monostate,                                // VT_EMPTY  (0)
                              detail::ErrorState,                            // VT_ERROR  (1)
                              int32_t,                                       // VT_INT    (2)
                              double,                                        // VT_FLOAT  (3)
                              std::string,                                   // VT_STRING (4)
                              detail::RefPtr,                                // VT_REF    (5)
                              std::unique_ptr<std::vector<Value>>,           // VT_VECTOR (6)
                              std::unique_ptr<std::map<std::string, Value>>  // VT_MAP    (7)
                              >;

    Value();
    Value(const Value& oVal, bool bRef = false);
    Value(Value&& oVal) noexcept;
    Value(double fVal);
    Value(int32_t nVal);
    explicit Value(const char* pcVal);
    Value(std::string sVal);
    Value(TYPE enType);
    explicit Value(const CValArray& arr);
    ~Value();

    ValueType getType() const noexcept;

    bool isContainer() const { return cself().getType() == VT_VECTOR || cself().getType() == VT_MAP; }

    bool isProtectedType() const { return cself()._protectType; }

    void error(string sMsg);
    int32_t asLong() const noexcept;
    double asReal() const noexcept;
    std::string asString(bool bForceLiteral = false) const;

    const char* c_str() const
    {
        static std::string sErg;
        sErg = asString();
        return sErg.c_str();
    }

    bool empty() const { return size() == 0; }

    Value& self()
    {
        if (auto* r = std::get_if<detail::RefPtr>(&_data))
            return *(r->ptr);
        return *this;
    }

    const Value& cself() const
    {
        if (const auto* r = std::get_if<detail::RefPtr>(&_data))
            return *(r->ptr);
        return *this;
    }

    const Value& operator=(const Value& oVal);

    bool setAt(const Value& oKey, const Value& oVal = Value());
    Value& getAt(const Value& oKey) const;
    Value getNth(int32_t nIdx) const;
    int32_t size() const;
    void remove(const Value& oKey);
    void clear();

    void swap(Value& oVal);

    Value operator-() const;
    Value operator+(const Value& oVal) const;
    Value operator-(const Value& oVal) const;
    Value operator*(const Value& oVal) const;
    Value operator/(const Value& oVal) const;
    Value operator%(const Value& oVal) const;

    Value pow(const Value& oVal) const;

    bool operator==(const Value& oVal) const;

    bool operator!=(const Value& oVal) const { return !(*this == oVal); }

    bool operator<(const Value& oVal) const;
    bool operator>(const Value& oVal) const;
    bool operator<=(const Value& oVal) const;
    bool operator>=(const Value& oVal) const;
    bool operator&&(const Value& oVal) const;
    bool operator||(const Value& oVal) const;
    bool operator!() const;

    static int32_t valueCount() { return _cnt; }
#ifdef _DEBUG
    static void resetPool()
    {
        _valPool.clear();
        _idCnt = 0;
    }

    static void dumpPool();
#endif

protected:
    Data _data;
    bool _protectType = false;
#ifdef _DEBUG
    int32_t _id = 0;
#endif

    static int32_t _cnt;
#ifdef _DEBUG
    static int32_t _idCnt;
    static std::map<int32_t, Value*> _valPool;
#endif
};

// Non-owning reference to a Value. Standalone class — does not inherit Value.
class CReference
{
public:
    CReference()
        : _ptr(nullptr)
    {
    }

    explicit CReference(Value& oVal)
        : _ptr(&oVal.self())
    {
    }

    void set(Value& oVal) { _ptr = &oVal.self(); }

    bool isValid() const { return _ptr != nullptr; }

    Value& self() { return *_ptr; }

    const Value& cself() const { return *_ptr; }

    // Convenience forwarders so callers don't need to go through self()
    std::string asString() const { return _ptr ? _ptr->asString() : std::string(); }

    ValueType getType() const { return _ptr ? _ptr->getType() : VT_EMPTY; }

private:
    Value* _ptr;
};

// Resizable array of Value. Standalone class — does not inherit Value.
// Used as VKommandos throughout the project.
class CValArray
{
public:
    typedef std::vector<Value>::iterator iterator;
    typedef std::vector<Value>::const_iterator const_iterator;

    CValArray()
        : _changed(false)
    {
    }

    iterator begin() { return _data.begin(); }

    const_iterator begin() const { return _data.begin(); }

    iterator end() { return _data.end(); }

    const_iterator end() const { return _data.end(); }

    size_t size() const { return _data.size(); }

    bool empty() const { return _data.empty(); }

    void clear() { _data.clear(); }

    Value& operator[](int32_t pos) { return _data[size_t(pos)]; }

    const Value& operator[](int32_t pos) const { return _data[size_t(pos)]; }

    void push_back(const Value& oVal)
    {
        _data.push_back(oVal);
        _changed = true;
    }

    bool changed() const { return _changed; }

    void changed(bool chg) { _changed = chg; }

private:
    std::vector<Value> _data;
    bool _changed;
};

class CObjectPart
{
public:
    typedef std::vector<Value> IndexField;

    CObjectPart()
        : next(nullptr)
        , bracket(0)
//        , assign(false)
    {
    }

    ~CObjectPart() { delete next; }

    std::string label;
    IndexField index;
    CObjectPart* next;
    char bracket;
//    bool assign;  // true when this object is the LHS of an assignment
};
