#include <catch2/catch_all.hpp>
#include <EBase/Value.h>

// -----------------------------------------------------------------------------
// Construction & type tagging
// -----------------------------------------------------------------------------

TEST_CASE("Value construction", "[cvalue]") {
    SECTION("default ctor yields VT_EMPTY") {
        Value v;
        //CHECK(sizeof(Value) == 48); // formerly 64
        CHECK(v.getType() == VT_EMPTY);
    }

    SECTION("int32_t ctor yields VT_INT") {
        Value v(int32_t(42));
        CHECK(v.getType() == VT_INT);
        CHECK(v.asLong() == 42);
        CHECK(v.asReal() == Catch::Approx(42.0));
    }

    SECTION("double ctor yields VT_FLOAT") {
        Value v(3.14);
        CHECK(v.getType() == VT_FLOAT);
        CHECK(v.asReal() == Catch::Approx(3.14));
        CHECK(v.asLong() == 3); // truncated
    }

    SECTION("const char* ctor yields VT_STRING") {
        Value v("hello");
        CHECK(v.getType() == VT_STRING);
        CHECK(v.asString() == "hello");
    }

    SECTION("std::string ctor yields VT_STRING") {
        Value v(std::string("world"));
        CHECK(v.getType() == VT_STRING);
        CHECK(v.asString() == "world");
    }

    SECTION("VT_VECTOR ctor creates empty vector container") {
        Value v(VT_VECTOR);
        CHECK(v.getType() == VT_VECTOR);
        CHECK(v.isContainer());
        CHECK(v.size() == 0);
    }

    SECTION("VT_MAP ctor creates empty map container") {
        Value v(VT_MAP);
        CHECK(v.getType() == VT_MAP);
        CHECK(v.isContainer());
        CHECK(v.size() == 0);
    }

    SECTION("isContainer is false for scalar types") {
        CHECK_FALSE(Value().isContainer());
        CHECK_FALSE(Value(int32_t(1)).isContainer());
        CHECK_FALSE(Value(1.0).isContainer());
        CHECK_FALSE(Value("str").isContainer());
    }
}

TEST_CASE("Value copy construction", "[cvalue]") {
    SECTION("scalar copy is independent") {
        Value a(int32_t(7));
        Value b(a);
        CHECK(b.asLong() == 7);
        b = Value(int32_t(99));
        CHECK(a.asLong() == 7); // a not affected
    }

    SECTION("copy with bRef=true creates VT_REF") {
        Value a(int32_t(5));
        Value ref(a, /*bRef=*/true);
        CHECK(ref.getType() == VT_REF);
        // dereferencing gives the original value
        CHECK(ref.self().asLong() == 5);
    }

    SECTION("copy of VT_REF propagates the same reference") {
        Value target(int32_t(10));
        Value ref(target, true);
        Value ref2(ref); // copy of a ref
        CHECK(ref2.getType() == VT_REF);
        CHECK(ref2.self().asLong() == 10);
    }

    SECTION("vector deep copy is independent") {
        Value arr(VT_VECTOR);
        arr.setAt(Value(int32_t(0)), Value(int32_t(100)));
        Value arr2(arr);
        arr2.setAt(Value(int32_t(0)), Value(int32_t(999)));
        CHECK(arr.getAt(Value(int32_t(0))).asLong() == 100); // original unchanged
    }

    SECTION("map deep copy is independent") {
        Value m(VT_MAP);
        m.setAt(Value("key"), Value(int32_t(1)));
        Value m2(m);
        m2.setAt(Value("key"), Value(int32_t(2)));
        CHECK(m.getAt(Value("key")).asLong() == 1); // original unchanged
    }
}

// -----------------------------------------------------------------------------
// asString conversions
// -----------------------------------------------------------------------------

TEST_CASE("Value::asString", "[cvalue]") {
    SECTION("VT_INT returns decimal string") {
        CHECK(Value(int32_t(42)).asString() == "42");
        CHECK(Value(int32_t(-7)).asString() == "-7");
        CHECK(Value(int32_t(0)).asString() == "0");
    }

    SECTION("VT_FLOAT returns 3-decimal string") {
        CHECK(Value(1.5).asString() == "1.500");
        CHECK(Value(0.0).asString() == "0.000");
        CHECK(Value(-2.0).asString() == "-2.000");
    }

    SECTION("VT_STRING returns the string value") {
        CHECK(Value("hello").asString() == "hello");
        CHECK(Value("").asString() == "");
    }

    SECTION("VT_VECTOR returns '[N Elements]' without literal flag") {
        Value v(VT_VECTOR);
        v.setAt(Value(int32_t(0)), Value(int32_t(1)));
        v.setAt(Value(int32_t(1)), Value(int32_t(2)));
        CHECK(v.asString() == "[2 Elements]");
    }

    SECTION("VT_VECTOR with bForceLiteral returns [e0,e1,...] form") {
        Value v(VT_VECTOR);
        v.setAt(Value(int32_t(0)), Value(int32_t(10)));
        v.setAt(Value(int32_t(1)), Value(int32_t(20)));
        CHECK(v.asString(true) == "[10,20]");
    }

    SECTION("VT_MAP returns '[N Elements]'") {
        Value m(VT_MAP);
        m.setAt(Value("a"), Value(int32_t(1)));
        CHECK(m.asString() == "[1 Elements]");
    }

    SECTION("VT_EMPTY returns empty string") {
        Value v;
        CHECK(v.asString() == "");
    }
}

// -----------------------------------------------------------------------------
// size
// -----------------------------------------------------------------------------

TEST_CASE("Value::size", "[cvalue]") {
    CHECK(Value(VT_VECTOR).size() == 0);
    CHECK(Value(VT_MAP).size() == 0);
    CHECK(Value(std::string("abc")).size() == 3);
    CHECK(Value(std::string("")).size() == 0);
    // Non-string scalars return -1
    CHECK(Value(int32_t(5)).size() == -1);
    CHECK(Value(2.0).size() == -1);
    CHECK(Value().size() == -1); // VT_EMPTY
}

// -----------------------------------------------------------------------------
// Assignment
// -----------------------------------------------------------------------------

TEST_CASE("Value assignment", "[cvalue]") {
    SECTION("self-assignment is safe") {
        Value v(int32_t(3));
        Value* ptr = &v;
        v = *ptr; // use pointer to suppress -Wself-assign-overloaded
        CHECK(v.asLong() == 3);
    }

    SECTION("assign changes type and value") {
        Value v(int32_t(1));
        v = Value("text");
        CHECK(v.getType() == VT_STRING);
        CHECK(v.asString() == "text");
    }

    SECTION("assign changes type and value") {
        Value v(int32_t(1));
        v = Value("text");
        CHECK(v.getType() == VT_STRING);
        CHECK(v.asString() == "text");
    }
}

// -----------------------------------------------------------------------------
// Error state
// -----------------------------------------------------------------------------

TEST_CASE("Value error state", "[cvalue]") {
    SECTION("error() sets VT_ERROR and stores message") {
        Value v(int32_t(1));
        v.error("something went wrong");
        CHECK(v.getType() == VT_ERROR);
        CHECK(v.asString() == "something went wrong");
    }
}

// -----------------------------------------------------------------------------
// Arithmetic operators
// -----------------------------------------------------------------------------

TEST_CASE("Value arithmetic", "[cvalue]") {
    SECTION("int + int") {
        Value a(int32_t(3)), b(int32_t(4));
        Value r = a + b;
        CHECK(r.getType() == VT_INT);
        CHECK(r.asLong() == 7);
    }

    SECTION("float + float") {
        Value a(1.5), b(2.5);
        Value r = a + b;
        CHECK(r.getType() == VT_FLOAT);
        CHECK(r.asReal() == Catch::Approx(4.0));
    }

    SECTION("float + int promotes to float arithmetic") {
        Value a(1.5), b(int32_t(2));
        Value r = a + b;
        CHECK(r.getType() == VT_FLOAT);
        CHECK(r.asReal() == Catch::Approx(3.5));
    }

    SECTION("string + string concatenates") {
        Value a("foo"), b("bar");
        Value r = a + b;
        CHECK(r.getType() == VT_STRING);
        CHECK(r.asString() == "foobar");
    }

    SECTION("string + int appends formatted integer") {
        Value a("num="), b(int32_t(42));
        Value r = a + b;
        CHECK(r.getType() == VT_STRING);
        CHECK(r.asString() == "num=42");
    }

    SECTION("int + string is an error") {
        Value a(int32_t(1)), b("x");
        Value r = a + b;
        CHECK(r.getType() == VT_ERROR);
    }

    SECTION("int - int") {
        Value r = Value(int32_t(10)) - Value(int32_t(3));
        CHECK(r.getType() == VT_INT);
        CHECK(r.asLong() == 7);
    }

    SECTION("int * int") {
        Value r = Value(int32_t(6)) * Value(int32_t(7));
        CHECK(r.getType() == VT_INT);
        CHECK(r.asLong() == 42);
    }

    SECTION("int / int") {
        Value r = Value(int32_t(10)) / Value(int32_t(4));
        CHECK(r.getType() == VT_INT);
        CHECK(r.asLong() == 2); // integer division
    }

    SECTION("division by zero is an error") {
        Value r = Value(int32_t(1)) / Value(int32_t(0));
        CHECK(r.getType() == VT_ERROR);
    }

    SECTION("int % int") {
        Value r = Value(int32_t(10)) % Value(int32_t(3));
        CHECK(r.asLong() == 1);
    }

    SECTION("modulo by zero is an error") {
        Value r = Value(int32_t(5)) % Value(int32_t(0));
        CHECK(r.getType() == VT_ERROR);
    }

    SECTION("unary minus on int") {
        Value r = -Value(int32_t(5));
        CHECK(r.asLong() == -5);
    }

    SECTION("unary minus on float") {
        Value r = -Value(2.5);
        CHECK(r.asReal() == Catch::Approx(-2.5));
    }

    SECTION("pow") {
        Value r = Value(int32_t(2)).pow(Value(int32_t(10)));
        CHECK(r.asLong() == 1024);
    }

    SECTION("error propagates through arithmetic") {
        Value err;
        err.error("bad");
        Value r = Value(int32_t(1)) + err;
        CHECK(r.getType() == VT_ERROR);
    }

    SECTION("VT_EMPTY in arithmetic is an error") {
        Value r = Value() + Value(int32_t(1));
        CHECK(r.getType() == VT_ERROR);
    }
}

// -----------------------------------------------------------------------------
// Comparison operators
// -----------------------------------------------------------------------------

TEST_CASE("Value comparisons", "[cvalue]") {
    SECTION("int equality") {
        CHECK(Value(int32_t(3)) == Value(int32_t(3)));
        CHECK_FALSE(Value(int32_t(3)) == Value(int32_t(4)));
        CHECK(Value(int32_t(3)) != Value(int32_t(4)));
    }

    SECTION("float equality uses epsilon") {
        CHECK(Value(1.0) == Value(1.0));
        CHECK_FALSE(Value(1.0) == Value(2.0));
    }

    SECTION("int vs float compares numerically") {
        CHECK(Value(int32_t(3)) == Value(3.0));
    }

    SECTION("string equality") {
        CHECK(Value("abc") == Value("abc"));
        CHECK_FALSE(Value("abc") == Value("def"));
    }

    SECTION("ordering integers") {
        CHECK(Value(int32_t(1)) < Value(int32_t(2)));
        CHECK(Value(int32_t(2)) > Value(int32_t(1)));
        CHECK(Value(int32_t(1)) <= Value(int32_t(1)));
        CHECK(Value(int32_t(1)) >= Value(int32_t(1)));
    }

    SECTION("ordering strings is lexicographic") {
        CHECK(Value("apple") < Value("banana"));
        CHECK(Value("z") > Value("a"));
    }

    SECTION("VT_EMPTY comparisons are always false") {
        Value empty;
        CHECK_FALSE(empty == Value(int32_t(0)));
        CHECK_FALSE(empty < Value(int32_t(1)));
    }

    SECTION("VT_ERROR comparisons are always false") {
        Value err;
        err.error("e");
        CHECK_FALSE(err == Value(int32_t(0)));
    }
}

// -----------------------------------------------------------------------------
// Logical operators
// -----------------------------------------------------------------------------

TEST_CASE("Value logical operators", "[cvalue]") {
    SECTION("operator! on zero int is true") {
        CHECK(!Value(int32_t(0)));
        CHECK_FALSE(!Value(int32_t(1)));
        CHECK_FALSE(!Value(int32_t(-1)));
    }

    SECTION("operator! on empty string is true") {
        CHECK(!Value(""));
        CHECK_FALSE(!Value("x"));
    }

    SECTION("operator! on VT_EMPTY is false") {
        CHECK_FALSE(!Value());
    }

    SECTION("operator&& short-circuits on false first arg") {
        // Catch2 can't decompose Value::operator&&/||; extract to bool first
        CHECK_FALSE(bool(Value(int32_t(0)) && Value(int32_t(1))));
        CHECK(bool(Value(int32_t(1)) && Value(int32_t(1))));
    }

    SECTION("operator|| returns true if either is non-zero") {
        CHECK(bool(Value(int32_t(0)) || Value(int32_t(1))));
        CHECK_FALSE(bool(Value(int32_t(0)) || Value(int32_t(0))));
    }

    SECTION("VT_EMPTY in logical ops is always false") {
        CHECK_FALSE(bool(Value() && Value(int32_t(1))));
        CHECK_FALSE(bool(Value() || Value(int32_t(1))));
    }
}

// -----------------------------------------------------------------------------
// Vector container
// -----------------------------------------------------------------------------

TEST_CASE("Value VT_VECTOR operations", "[cvalue]") {
    Value v(VT_VECTOR);

    SECTION("append with consecutive integer keys") {
        v.setAt(Value(int32_t(0)), Value(int32_t(10)));
        v.setAt(Value(int32_t(1)), Value(int32_t(20)));
        v.setAt(Value(int32_t(2)), Value(int32_t(30)));
        CHECK(v.size() == 3);
        CHECK(v.getAt(Value(int32_t(0))).asLong() == 10);
        CHECK(v.getAt(Value(int32_t(1))).asLong() == 20);
        CHECK(v.getAt(Value(int32_t(2))).asLong() == 30);
    }

    SECTION("replace existing element") {
        v.setAt(Value(int32_t(0)), Value(int32_t(1)));
        v.setAt(Value(int32_t(0)), Value(int32_t(99)));
        CHECK(v.size() == 1);
        CHECK(v.getAt(Value(int32_t(0))).asLong() == 99);
    }

    SECTION("getNth returns the index value (not the element)") {
        v.setAt(Value(int32_t(0)), Value("a"));
        v.setAt(Value(int32_t(1)), Value("b"));
        // getNth for VT_VECTOR returns the index as Value(int), not the element
        CHECK(v.getNth(0).asLong() == 0);
        CHECK(v.getNth(1).asLong() == 1);
        // Out-of-range returns VT_EMPTY
        CHECK(v.getNth(5).getType() == VT_EMPTY);
    }

    SECTION("remove erases by index") {
        v.setAt(Value(int32_t(0)), Value(int32_t(1)));
        v.setAt(Value(int32_t(1)), Value(int32_t(2)));
        v.remove(Value(int32_t(0)));
        CHECK(v.size() == 1);
    }

    SECTION("clear empties the vector") {
        v.setAt(Value(int32_t(0)), Value(int32_t(1)));
        v.clear();
        CHECK(v.size() == 0);
    }

    SECTION("out-of-range key (non-consecutive) fails setAt") {
        bool ok = v.setAt(Value(int32_t(5)), Value(int32_t(1))); // gap
        CHECK_FALSE(ok);
        CHECK(v.size() == 0);
    }

    SECTION("getAt out-of-range returns VT_EMPTY") {
        v.setAt(Value(int32_t(0)), Value(int32_t(42)));
        Value& r = v.getAt(Value(int32_t(99)));
        CHECK(r.getType() == VT_EMPTY);
    }
}

// -----------------------------------------------------------------------------
// Map container
// -----------------------------------------------------------------------------

TEST_CASE("Value VT_MAP operations", "[cvalue]") {
    Value m(VT_MAP);

    SECTION("insert and retrieve by string key") {
        m.setAt(Value("name"), Value("Alice"));
        m.setAt(Value("age"), Value(int32_t(30)));
        CHECK(m.size() == 2);
        CHECK(m.getAt(Value("name")).asString() == "Alice");
        CHECK(m.getAt(Value("age")).asLong() == 30);
    }

    SECTION("setAt with VT_EMPTY value erases the key") {
        m.setAt(Value("x"), Value(int32_t(1)));
        m.setAt(Value("x"), Value()); // VT_EMPTY = erase
        CHECK(m.size() == 0);
        CHECK(m.getAt(Value("x")).getType() == VT_EMPTY);
    }

    SECTION("remove erases by key") {
        m.setAt(Value("a"), Value(int32_t(1)));
        m.remove(Value("a"));
        CHECK(m.size() == 0);
    }

    SECTION("clear empties the map") {
        m.setAt(Value("a"), Value(int32_t(1)));
        m.setAt(Value("b"), Value(int32_t(2)));
        m.clear();
        CHECK(m.size() == 0);
    }

    SECTION("getNth returns the nth key") {
        m.setAt(Value("alpha"), Value(int32_t(1)));
        // Map is ordered: "alpha" is at index 0
        Value key = m.getNth(0);
        CHECK(key.getType() == VT_STRING);
        CHECK(key.asString() == "alpha");
    }

    SECTION("getNth out of range returns VT_EMPTY") {
        CHECK(m.getNth(0).getType() == VT_EMPTY);
    }

    SECTION("getAt missing key returns VT_EMPTY") {
        CHECK(m.getAt(Value("no-such-key")).getType() == VT_EMPTY);
    }

    SECTION("integer key is converted to string for map") {
        m.setAt(Value(int32_t(1)), Value("one"));
        CHECK(m.getAt(Value("1")).asString() == "one");
    }
}

// -----------------------------------------------------------------------------
// CReference
// -----------------------------------------------------------------------------

TEST_CASE("CReference", "[cvalue]") {
    SECTION("default CReference is invalid") {
        CReference ref;
        // Standalone CReference has no type tag of its own; null ref reports VT_EMPTY
        CHECK(ref.getType() == VT_EMPTY);
        CHECK_FALSE(ref.isValid());
    }

    SECTION("CReference pointing to a value dereferences through self()") {
        Value target(int32_t(42));
        CReference ref(target);
        CHECK(ref.isValid());
        CHECK(ref.self().asLong() == 42);
    }

    SECTION("modifying the target is visible through the reference") {
        Value target(int32_t(1));
        CReference ref(target);
        target = Value(int32_t(99));
        CHECK(ref.self().asLong() == 99);
    }

    SECTION("set() rebinds the reference") {
        Value a(int32_t(1)), b(int32_t(2));
        CReference ref(a);
        ref.set(b);
        CHECK(ref.self().asLong() == 2);
    }
}

// -----------------------------------------------------------------------------
// CValArray
// -----------------------------------------------------------------------------

TEST_CASE("CValArray", "[cvalue]") {
    CValArray arr;

    SECTION("starts empty") {
        CHECK(arr.empty());
        CHECK(arr.size() == 0);
    }

    SECTION("push_back appends elements accessible by index") {
        arr.push_back(Value(int32_t(10)));
        arr.push_back(Value(int32_t(20)));
        arr.push_back(Value(int32_t(30)));
        CHECK(arr.size() == 3);
        CHECK(arr[0].asLong() == 10);
        CHECK(arr[1].asLong() == 20);
        CHECK(arr[2].asLong() == 30);
    }

    SECTION("begin/end iterators work") {
        arr.push_back(Value(int32_t(1)));
        arr.push_back(Value(int32_t(2)));
        int32_t sum = 0;
        for (auto& v : arr) sum += v.asLong();
        CHECK(sum == 3);
    }

    SECTION("clear empties the array") {
        arr.push_back(Value(int32_t(1)));
        arr.clear();
        CHECK(arr.empty());
    }

    SECTION("changed() flag tracks push_back") {
        arr.changed(false);
        arr.push_back(Value(int32_t(1)));
        CHECK(arr.changed());
    }

    SECTION("copy-assigns correctly between CValArrays") {
        CValArray other;
        other.push_back(Value(int32_t(42)));
        arr = other;
        CHECK(arr.size() == 1);
        CHECK(arr[0].asLong() == 42);
    }
}

// -----------------------------------------------------------------------------
// swap
// -----------------------------------------------------------------------------

TEST_CASE("Value::swap", "[cvalue]") {
    SECTION("swaps scalar values") {
        Value a(int32_t(1)), b(int32_t(2));
        a.swap(b);
        CHECK(a.asLong() == 2);
        CHECK(b.asLong() == 1);
    }

    SECTION("swaps different types") {
        Value a(int32_t(42)), b("hello");
        a.swap(b);
        CHECK(a.getType() == VT_STRING);
        CHECK(a.asString() == "hello");
        CHECK(b.getType() == VT_INT);
        CHECK(b.asLong() == 42);
    }
}

// -----------------------------------------------------------------------------
// valueCount tracking
// -----------------------------------------------------------------------------

TEST_CASE("Value::valueCount", "[cvalue]") {
    int32_t before = Value::valueCount();
    {
        Value a(int32_t(1));
        Value b(int32_t(2));
        CHECK(Value::valueCount() == before + 2);
    }
    CHECK(Value::valueCount() == before);
}
