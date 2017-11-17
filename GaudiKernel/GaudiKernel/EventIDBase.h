#ifndef GAUDIKERNEL_EVENTIDBASE_H
#define GAUDIKERNEL_EVENTIDBASE_H 1

/**
 * @file EventIDBase.h
 *
 * @brief This class provides a unique identification for each event,
 * in terms of run/event number and/or a time stamp.
 *
 * @author RD Schaffer <R.D.Schaffer@cern.ch>
 * @author Paolo Calafiura <pcalafiura@lbl.gov>
 * @author Charles Leggett
 *
 */

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <tuple>

#include "GaudiKernel/compose.h"
namespace details
{
  template <typename lambda>
  struct arg_helper : public arg_helper<decltype( &lambda::operator() )> {
  };
  template <typename T, typename Ret, typename Arg>
  struct arg_helper<Ret ( T::* )( Arg ) const> {
    using type = Arg;
  };

  // given a unary lambda whose argument is of type Arg_t,
  // argument_t<lambda> will be equal to Arg_t
  template <typename lambda>
  using argument_t = typename arg_helper<lambda>::type;

  template <typename Fun>
  auto add_deref( Fun f )
  {
    return compose( f, [=]( auto*... p ) { return f( *p... ); } );
  }

  template <typename Proj, typename Cmp = std::greater<>>
  auto make_cmp( Proj p, Cmp cmp = {} )
  {
    static_assert( std::is_reference<argument_t<Proj>>::value, "must be a reference" );
    static_assert( std::is_const<std::remove_reference_t<argument_t<Proj>>>::value, "must be const" );
    return [=]( argument_t<Proj> lhs, argument_t<Proj> rhs ) { return cmp( p( lhs ), p( rhs ) ); };
  }
}

/**
 * @class EventIDBase
 *
 * @brief This class provides a unique identification for each event,
 * in terms of run/event number and/or a time stamp.
 *
 */

class EventIDBase
{
public:
  typedef unsigned int number_type;
  typedef uint64_t event_number_t;

  static const number_type UNDEFNUM;
  static const event_number_t UNDEFEVT;

public:
  /// \name structors
  //@{
  EventIDBase(){};
  EventIDBase( number_type run_number, event_number_t event_number, number_type time_stamp = UNDEFNUM,
               number_type time_stamp_ns_offset = 0, number_type lumi_block = UNDEFNUM,
               number_type bunch_crossing_id = 0 );
  // Use default copy constructor.
  virtual ~EventIDBase();
  //@}

  /// run number - 32 bit unsigned
  number_type run_number() const { return m_run_number; }

  /// event number - 64 bit unsigned
  event_number_t event_number() const { return m_event_number; }

  /// time stamp - posix time in seconds from 1970, 32 bit unsigned
  number_type time_stamp() const { return m_time_stamp; }

  /// time stamp ns - ns time offset for time_stamp, 32 bit unsigned
  number_type time_stamp_ns_offset() const { return m_time_stamp_ns_offset; }

  /// luminosity block identifier, 32 bit unsigned
  number_type lumi_block() const { return m_lumi_block; }

  /// bunch crossing ID,  32 bit unsigned
  number_type bunch_crossing_id() const { return m_bunch_crossing_id; }

  /// set run number
  void set_run_number( number_type runNumber )
  {
    m_run_number = runNumber;
    if ( m_event_number != UNDEFEVT ) setRE();
    if ( m_lumi_block != UNDEFNUM ) setRL();
  }

  /// set event number
  void set_event_number( event_number_t eventNumber )
  {
    m_event_number = eventNumber;
    if ( m_run_number != UNDEFNUM ) setRE();
    if ( m_lumi_block != UNDEFNUM ) setLE();
  }

  /// set time stamp
  void set_time_stamp( number_type timeStamp )
  {
    m_time_stamp = timeStamp;
    setTS();
  }

  /// set time stamp in ns
  void set_time_stamp_ns_offset( number_type timeStampNs ) { m_time_stamp_ns_offset = timeStampNs; }

  /// set luminosity block identifier
  void set_lumi_block( number_type lumiBlock )
  {
    m_lumi_block = lumiBlock;
    if ( m_run_number != UNDEFNUM ) setRL();
    if ( m_event_number != UNDEFEVT ) setLE();
  }

  /// set bunch crossing ID
  void set_bunch_crossing_id( number_type bcid ) { m_bunch_crossing_id = bcid; }

  /// Comparison operators
  friend bool operator<( const EventIDBase& lhs, const EventIDBase& rhs );
  friend bool operator>( const EventIDBase& lhs, const EventIDBase& rhs );
  friend bool operator==( const EventIDBase& lhs, const EventIDBase& rhs );
  friend bool operator!=( const EventIDBase& lhs, const EventIDBase& rhs );
  friend bool operator<=( const EventIDBase& lhs, const EventIDBase& rhs );
  friend bool operator>=( const EventIDBase& lhs, const EventIDBase& rhs );

  bool isRunEvent() const;
  bool isTimeStamp() const;
  bool isLumiEvent() const;
  bool isRunLumi() const;
  bool isValid() const;

  /// Extraction operators
  friend std::ostream& operator<<( std::ostream& os, const EventIDBase& rhs );

  static auto SortByTimeStamp()
  {
    return details::add_deref( details::make_cmp(
        []( const EventIDBase& e ) { return std::tie( e.m_time_stamp, e.m_time_stamp_ns_offset ); } ) );
  };

  static auto SortByRunEvent()
  {
    return details::add_deref(
        details::make_cmp( []( const EventIDBase& e ) { return std::tie( e.m_run_number, e.m_event_number ); } ) );
  };

  static auto SortByLumiEvent()
  {
    return details::add_deref(
        details::make_cmp( []( const EventIDBase& e ) { return std::tie( e.m_lumi_block, e.m_event_number ); } ) );
  };

  static auto SortByRunLumi()
  {
    return details::add_deref(
        details::make_cmp( []( const EventIDBase& e ) { return std::tie( e.m_run_number, e.m_lumi_block ); } ) );
  };

private:
  enum Type { Invalid = 0, RunEvent = 1 << 1, TimeStamp = 1 << 2, LumiEvent = 1 << 3, RunLumi = 1 << 4 };

  unsigned m_type{Invalid};

  void setRE() { m_type = m_type | RunEvent; }
  void setTS() { m_type = m_type | TimeStamp; }
  void setLE() { m_type = m_type | LumiEvent; }
  void setRL() { m_type = m_type | RunLumi; }

  /// run number
  number_type m_run_number{UNDEFNUM};

  /// event number
  event_number_t m_event_number{UNDEFEVT};

  /// posix time in seconds since 1970/01/01
  number_type m_time_stamp{UNDEFNUM};

  /// time stamp ns - ns time offset for time_stamp, 32 bit unsigned
  number_type m_time_stamp_ns_offset{UNDEFNUM};

  /// luminosity block number:
  /// the number which uniquely tags a luminosity block within a run
  number_type m_lumi_block{UNDEFNUM};

  /// bunch crossing ID,  32 bit unsigned
  number_type m_bunch_crossing_id{UNDEFNUM};
};

inline bool operator<( const EventIDBase& lhs, const EventIDBase& rhs )
{
  // first try ordering by timestamp if both are non-zero
  // then try ordering by run/lumi/event
  // this assumes that both EventIDBase have the same set of values defined.

  if ( lhs.isTimeStamp() && rhs.isTimeStamp() ) {
    return lhs.m_time_stamp < rhs.m_time_stamp;
  } else {
    return std::tie( lhs.m_run_number, lhs.m_lumi_block, lhs.m_event_number ) <
           std::tie( rhs.m_run_number, rhs.m_lumi_block, rhs.m_event_number );
  }
}

inline bool operator==( const EventIDBase& lhs, const EventIDBase& rhs )
{
  // We assume that equality via run/event/lumi numbers is sufficient
  return ( lhs.m_run_number == rhs.m_run_number && lhs.m_event_number == rhs.m_event_number &&
           lhs.m_lumi_block == rhs.m_lumi_block );
}
inline bool operator>( const EventIDBase& lhs, const EventIDBase& rhs ) { return rhs < lhs; }
inline bool operator!=( const EventIDBase& lhs, const EventIDBase& rhs ) { return !( lhs == rhs ); }
inline bool operator<=( const EventIDBase& lhs, const EventIDBase& rhs ) { return !( lhs > rhs ); }
inline bool operator>=( const EventIDBase& lhs, const EventIDBase& rhs ) { return !( lhs < rhs ); }

inline std::ostream& operator<<( std::ostream& os, const EventIDBase& rhs )
{
  if ( rhs.m_type == EventIDBase::Invalid ) return os << "[INVALID]";

  const char* separator = "";
  os << "[";
  if ( rhs.m_run_number != EventIDBase::UNDEFNUM ) {
    os << rhs.m_run_number;
    separator = ",";
  }

  if ( rhs.m_event_number != EventIDBase::UNDEFEVT ) {
    os << separator << rhs.m_event_number;
    separator = ",";
  }

  if ( rhs.isTimeStamp() ) {
    os << separator << "t:" << rhs.m_time_stamp;
    if ( rhs.m_time_stamp_ns_offset != 0 ) {
      os << "." << std::setfill( '0' ) << std::setw( 9 ) << rhs.m_time_stamp_ns_offset;
    }
    separator = ",";
  }

  if ( rhs.isLumiEvent() || rhs.isRunLumi() ) {
    os << separator << "l:" << rhs.m_lumi_block;
    separator = ",";
  }

  if ( rhs.m_bunch_crossing_id != 0 ) {
    os << separator << "b:" << rhs.m_bunch_crossing_id;
  }
  os << "]";
  return os;
}

#endif // EVENTINFO_EVENTID_H
