/***********************************************************************************\
* (c) Copyright 2022 CERN for the benefit of the LHCb and ATLAS collaborations      *
*                                                                                   *
* This software is distributed under the terms of the Apache version 2 licence,     *
* copied verbatim in the file "LICENSE".                                            *
*                                                                                   *
* In applying this licence, CERN does not waive the privileges and immunities       *
* granted to it by virtue of its status as an Intergovernmental Organization        *
* or submit itself to any jurisdiction.                                             *
\***********************************************************************************/
#include <Gaudi/Accumulators.h>
#include <catch2/catch.hpp>

using namespace Gaudi::Accumulators;

namespace {
  struct Algo;
}
template <>
class CommonMessaging<Algo> : public CommonMessagingBase {
  void create_msgStream() const override { m_msgStream.reset( new MsgStream( nullptr, "TestAlgo" ) ); }
};

namespace {
  // Mock code for the test
  struct MonitoringHub : Gaudi::Monitoring::Hub {};
  struct ServiceLocator {
    MonitoringHub& monitoringHub() { return m_monitHub; }
    MonitoringHub  m_monitHub{};
  };
  struct Algo : CommonMessaging<Algo> {
    ServiceLocator* serviceLocator() { return &m_serviceLocator; }
    std::string     name() const { return ""; }
    ServiceLocator  m_serviceLocator{};
  };

  template <typename C>
  struct helper;

  template <typename C>
  struct helper_base {
    using counter_t = C;
    counter_t c;

    void check_data() const {
      nlohmann::json j = c.toJSON();
      CHECK( j["type"].get<std::string>() == c.typeString );
      CHECK( j["empty"].get<bool>() == !c.toBePrinted() );
      static_cast<const helper<C>*>( this )->check_details( j, j["empty"].get<bool>() );
    }
    void check_round_trip() const {
      auto j  = c.toJSON();
      auto c2 = c.fromJSON( j );
      auto j2 = c2.toJSON();
      CHECK( j == j2 );
    }
    void check() {
      check_data();
      check_round_trip();
      static_cast<helper<C>*>( this )->fill();
      check_data();
      check_round_trip();
    }
  };

  template <>
  struct helper<Counter<>> : helper_base<Counter<>> {
    void fill() { c += 10; }
    void check_details( const nlohmann::json& j, bool empty ) const {
      unsigned long expected = empty ? 0 : 10;
      CHECK( j["nEntries"].get<unsigned long>() == expected );
    }
  };
  template <>
  struct helper<AveragingCounter<>> : helper_base<AveragingCounter<>> {
    void fill() {
      c += 10;
      c += 20;
      c += 30;
    }
    void check_details( const nlohmann::json& j, bool empty ) const {
      if ( empty ) {
        CHECK( j["nEntries"].get<unsigned long>() == 0 );
        CHECK( j["sum"].get<double>() == 0 );
        CHECK( j["mean"].get<double>() == 0 );
      } else {
        CHECK( j["nEntries"].get<unsigned long>() == 3 );
        CHECK( j["sum"].get<double>() == 60 );
        CHECK( j["mean"].get<double>() == 20 );
      }
    }
  };
  template <>
  struct helper<SigmaCounter<>> : helper_base<SigmaCounter<>> {
    void fill() {
      c += 10;
      c += 20;
      c += 30;
    }
    void check_details( const nlohmann::json& j, bool empty ) const {
      if ( empty ) {
        CHECK( j["nEntries"].get<unsigned long>() == 0 );
        CHECK( j["sum"].get<double>() == 0 );
        CHECK( j["mean"].get<double>() == 0 );
        CHECK( j["sum2"].get<double>() == 0 );
        CHECK( j["standard_deviation"].get<double>() == 0 );
      } else {
        CHECK( j["nEntries"].get<unsigned long>() == 3 );
        CHECK( j["sum"].get<double>() == 60 );
        CHECK( j["mean"].get<double>() == 20 );
        CHECK( j["sum2"].get<double>() == 1400 );
        CHECK( j["standard_deviation"].get<double>() == Approx( 8.164966 ) );
      }
    }
  };
  template <>
  struct helper<StatCounter<>> : helper_base<StatCounter<>> {
    void fill() {
      c += 10;
      c += 20;
      c += 30;
    }
    void check_details( const nlohmann::json& j, bool empty ) const {
      if ( empty ) {
        CHECK( j["nEntries"].get<unsigned long>() == 0 );
        CHECK( j["sum"].get<double>() == 0 );
        CHECK( j["mean"].get<double>() == 0 );
        CHECK( j["sum2"].get<double>() == 0 );
        CHECK( j["standard_deviation"].get<double>() == 0 );
        // CHECK( j["min"].get<double>() == 0 );
        // CHECK( j["max"].get<double>() == 0 );
      } else {
        CHECK( j["nEntries"].get<unsigned long>() == 3 );
        CHECK( j["sum"].get<double>() == 60 );
        CHECK( j["mean"].get<double>() == 20 );
        CHECK( j["sum2"].get<double>() == 1400 );
        CHECK( j["standard_deviation"].get<double>() == Approx( 8.164966 ) );
        CHECK( j["min"].get<double>() == 10 );
        CHECK( j["max"].get<double>() == 30 );
      }
    }
  };
  template <>
  struct helper<BinomialCounter<>> : helper_base<BinomialCounter<>> {
    void fill() {
      c += true;
      c += false;
      c += true;
      c += { 3, 7 };
    }
    void check_details( const nlohmann::json& j, bool empty ) const {
      if ( empty ) {
        CHECK( j["nEntries"].get<unsigned long>() == 0 );
        CHECK( j["nTrueEntries"].get<double>() == 0 );
        CHECK( j["nFalseEntries"].get<double>() == 0 );
        // CHECK( j["efficiency"].get<double>() == 0 );
        // CHECK( j["efficiencyErr"].get<double>() == 0 );
      } else {
        CHECK( j["nEntries"].get<unsigned long>() == 10 );
        CHECK( j["nTrueEntries"].get<double>() == 5 );
        CHECK( j["nFalseEntries"].get<double>() == 5 );
        CHECK( j["efficiency"].get<double>() == 0.5 );
        CHECK( j["efficiencyErr"].get<double>() == Approx( 0.158114 ) );
      }
    }
  };
  template <MSG::Level level>
  struct helper<MsgCounter<level>> {
    Algo alg;

    MsgCounter<level> c{ &alg, "my message" };

    void fill() {
      ++c;
      ++c;
      ++c;
    }

    void check_data() const {
      nlohmann::json j = c.toJSON();
      CHECK( j["type"].get<std::string>() == c.typeString );
      CHECK( j["empty"].get<bool>() == !c.toBePrinted() );
      CHECK( j["level"].get<MSG::Level>() == MSG::DEBUG );
      CHECK( j["msg"].get<std::string>() == "my message" );
      if ( j["empty"].get<bool>() ) {
        CHECK( j["nEntries"].get<unsigned long>() == 0 );
      } else {
        CHECK( j["nEntries"].get<unsigned long>() == 3 );
      }
    }

    void check_round_trip() const {
      auto j  = c.toJSON();
      auto c2 = c.fromJSON( j );
      auto j2 = c2.toJSON();
      CHECK( j == j2 );
    }

    void check() {
      check_data();
      check_round_trip();
      fill();
      check_data();
      check_round_trip();
    }
  };
} // namespace

TEMPLATE_TEST_CASE( "counter serialization", "", Counter<>, AveragingCounter<>, SigmaCounter<>, StatCounter<>,
                    BinomialCounter<>, MsgCounter<MSG::DEBUG> ) {
  helper<TestType>{}.check();
}
