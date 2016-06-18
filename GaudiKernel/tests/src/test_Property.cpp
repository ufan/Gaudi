#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_PropertyMgr
#include <boost/test/unit_test.hpp>

#include "GaudiKernel/PropertyMgr.h"
#include "GaudiKernel/GaudiException.h"

namespace {
  const std::string emptyName{};
  /// Helper to allow instantiation of PropertyMgr.
  struct AnonymousPropertyMgr: public implements<IProperty, INamedInterface>,
                               public PropertyMgr {
    const std::string& name() const override { return emptyName; }
  };
}

BOOST_AUTO_TEST_CASE( value_props_constructors )
{
  {
    StringProperty p;
    BOOST_CHECK(p.value() == "");
    BOOST_CHECK(p.name() == "");
    BOOST_CHECK(p.documentation() == "");
  }
  {
    StringProperty p("abc");
    BOOST_CHECK(p.value() == "abc");
    BOOST_CHECK(p.name() == "");
    BOOST_CHECK(p.documentation() == "");
  }
  {
    StringProperty p("abc", "xyz");
    BOOST_CHECK(p.value() == "xyz");
    BOOST_CHECK(p.name() == "abc");
    BOOST_CHECK(p.documentation() == "");
  }
  {
    StringProperty p("abc", "xyz", "doc");
    BOOST_CHECK(p.value() == "xyz");
    BOOST_CHECK(p.name() == "abc");
    BOOST_CHECK(p.documentation() == "doc");
  }
  {
    IntegerProperty p;
    BOOST_CHECK(p.value() == 0);
    BOOST_CHECK(p.name() == "");
    BOOST_CHECK(p.documentation() == "");
  }
  {
    IntegerProperty p(123);
    BOOST_CHECK(p.value() == 123);
    BOOST_CHECK(p.name() == "");
    BOOST_CHECK(p.documentation() == "");
  }
  {
    IntegerProperty p("abc", 456);
    BOOST_CHECK(p.value() == 456);
    BOOST_CHECK(p.name() == "abc");
    BOOST_CHECK(p.documentation() == "");
  }
  {
    IntegerProperty p("abc", 456, "doc");
    BOOST_CHECK(p.value() == 456);
    BOOST_CHECK(p.name() == "abc");
    BOOST_CHECK(p.documentation() == "doc");
  }
}

BOOST_AUTO_TEST_CASE( string_conversion )
{
  {
    StringProperty  p1{"p1", ""};
    BOOST_CHECK(p1.value() == "");
    p1 = "abc";
    BOOST_CHECK(p1.value() == "abc");
    BOOST_CHECK(p1.fromString("xyz"));
    BOOST_CHECK(p1.value() == "xyz");
    BOOST_CHECK(p1.toString() == "xyz");
  }
  {
    IntegerProperty p2{"p2", 10};
    BOOST_CHECK(p2.value() == 10);
    p2 = 20;
    BOOST_CHECK(p2.value() == 20);
    BOOST_CHECK(p2.fromString("123"));
    BOOST_CHECK(p2.value() == 123);
    BOOST_CHECK(p2.toString() == "123");
  }
  {
    BooleanProperty p3{"p3", true};
    BOOST_CHECK(p3.value() == true);
    p3 = false;
    BOOST_CHECK(p3.value() == false);
    BOOST_CHECK(p3.fromString("true"));
    BOOST_CHECK(p3.value() == true);
    BOOST_CHECK(p3.toString() == "True");
  }
  {
    IntegerProperty dst{0};
    StringProperty  src{"321"};
    BOOST_CHECK(dst.value() == 0);
    BOOST_CHECK(dst.assign(src));
    BOOST_CHECK(dst.value() == 321);
    dst = 100;
    BOOST_CHECK(dst.value() == 100);
    BOOST_CHECK(dst.load(src));
    BOOST_CHECK(src.value() == "100");
  }
  {
    // string property as from options
    StringProperty opt{"\"NONE\""};
    StringProperty p{};
    BOOST_CHECK(opt.load(p));
    BOOST_CHECK(p.value() == "NONE");
  }
  {
    // string property as from options
    StringProperty opt{"\"NONE\""};
    std::string dst;
    StringPropertyRef p{"test", dst};
    BOOST_CHECK(opt.load(p));
    BOOST_CHECK(p.value() == "NONE");
    BOOST_CHECK(dst == "NONE");
  }
}

template<class T>
const T& convert_to(const T& v) {return v;}

BOOST_AUTO_TEST_CASE( implicit_conversion )
{
  {
    IntegerProperty p(123);
    BOOST_CHECK(convert_to<int>(p) == 123);
    BOOST_CHECK(convert_to<long>(p) == 123);
    BOOST_CHECK(convert_to<double>(p) == 123);
  }
  {
    BooleanProperty p(true);
    BOOST_CHECK(convert_to<int>(p) == 1);
    BOOST_CHECK(convert_to<bool>(p) == true);
  }
  {
    BooleanProperty p(false);
    BOOST_CHECK(convert_to<int>(p) == 0);
    BOOST_CHECK(convert_to<bool>(p) == false);
  }
}
