#ifdef _WIN32
// Avoid conflicts between windows and the message service.
#define NOMSG
#define NOGDI
#endif

#include "GaudiKernel/Kernel.h"
#include "GaudiKernel/StatusCode.h"
#include "GaudiKernel/Message.h"
#include "GaudiKernel/xtoa.h"
#include "MessageSvc.h"

#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;

// Instantiation of a static factory class used by clients to create
// instances of this service
DECLARE_COMPONENT(MessageSvc)

static const std::string levelNames[MSG::NUM_LEVELS] = { 
   "NIL", "VERBOSE", "DEBUG", "INFO",
   "WARNING", "ERROR", "FATAL", "ALWAYS"
};

// Constructor
MessageSvc::MessageSvc( const std::string& name, ISvcLocator* svcloc )
  : base_class( name, svcloc ) {
  m_defaultStream = &std::cout;
  m_outputLevel   = MSG::NIL;
  declareProperty( "Format",      m_defaultFormat = Message::getDefaultFormat() );
  declareProperty( "timeFormat",  m_defaultTimeFormat = Message::getDefaultTimeFormat() );
  declareProperty( "showStats",   m_stats = false );
  declareProperty( "statLevel",   m_statLevel = 0 );

  // Special properties to control output level of individual sources
  declareProperty( "setVerbose",  m_thresholdProp[MSG::VERBOSE] );
  declareProperty( "setDebug",    m_thresholdProp[MSG::DEBUG] );
  declareProperty( "setInfo",     m_thresholdProp[MSG::INFO] );
  declareProperty( "setWarning",  m_thresholdProp[MSG::WARNING] );
  declareProperty( "setError",    m_thresholdProp[MSG::ERROR] );
  declareProperty( "setFatal",    m_thresholdProp[MSG::FATAL] );
  declareProperty( "setAlways",   m_thresholdProp[MSG::ALWAYS] );

  declareProperty( "useColors",        m_color=false)->declareUpdateHandler(&MessageSvc::initColors, this);

  declareProperty( "fatalColorCode",   m_logColors[MSG::FATAL] );
  declareProperty( "errorColorCode",   m_logColors[MSG::ERROR] );
  declareProperty( "warningColorCode", m_logColors[MSG::WARNING] );
  declareProperty( "infoColorCode",    m_logColors[MSG::INFO] );
  declareProperty( "debugColorCode",   m_logColors[MSG::DEBUG] );
  declareProperty( "verboseColorCode", m_logColors[MSG::VERBOSE] );
  declareProperty( "alwaysColorCode",  m_logColors[MSG::ALWAYS] );

  const int defaultLimit = 500;
  declareProperty( "fatalLimit",    m_msgLimit[MSG::FATAL]   = defaultLimit );
  declareProperty( "errorLimit",    m_msgLimit[MSG::ERROR]   = defaultLimit );
  declareProperty( "warningLimit",  m_msgLimit[MSG::WARNING] = defaultLimit );
  declareProperty( "infoLimit",     m_msgLimit[MSG::INFO]    = defaultLimit );
  declareProperty( "debugLimit",    m_msgLimit[MSG::DEBUG]   = defaultLimit );
  declareProperty( "verboseLimit",  m_msgLimit[MSG::VERBOSE] = defaultLimit );
  declareProperty( "alwaysLimit",   m_msgLimit[MSG::ALWAYS]  = 0 );

  declareProperty( "defaultLimit",  m_msgLimit[MSG::NIL]     = defaultLimit );

  declareProperty( "enableSuppression", m_suppress = false );
  declareProperty( "countInactive", m_inactCount = false )->declareUpdateHandler( &MessageSvc::setupInactCount, this );
#ifndef NDEBUG
  // initialize the MsgStream static flag.
  MsgStream::enableCountInactive(m_inactCount);
#endif

  declareProperty( "loggedStreams",
                   m_loggedStreamsName,
                   "MessageStream sources we want to dump into a logfile" );

  for (int ic=0; ic<MSG::NUM_LEVELS; ++ic) {
    m_logColors[ic].declareUpdateHandler(&MessageSvc::setupColors, this);
    m_msgLimit[ic].declareUpdateHandler(&MessageSvc::setupLimits, this);
    m_thresholdProp[ic].declareUpdateHandler(&MessageSvc::setupThreshold, this);
  }


  for (int i=0; i<MSG::NUM_LEVELS; ++i) {
      m_msgCount[i] = 0;
  }

}

//#############################################################################


/// Initialize Service
StatusCode MessageSvc::initialize() {
  StatusCode sc;
  sc = Service::initialize();
  if( sc.isFailure() ) return sc;
  // Release pointer to myself done in Service base class
  //if( m_msgsvc.isValid() ) {
  //  m_msgsvc = 0;
  //}
  // Set my own properties
  sc = setProperties();
  if (sc.isFailure()) return sc;

#ifdef _WIN32
  m_color = false;
#endif

  //NOTE: m_colMap is used _before_ it is filled here,
  //      i.e. while it is still empty.
  //      Moving this initialization 'up' by eg. just 
  //      having a 'static const' colMap does not leave
  //      the results invariant...
  m_colMap["black"]  = MSG::BLACK;
  m_colMap["red"]    = MSG::RED;
  m_colMap["green"]  = MSG::GREEN;
  m_colMap["yellow"] = MSG::YELLOW;
  m_colMap["blue"]   = MSG::BLUE;
  m_colMap["purple"] = MSG::PURPLE;
  m_colMap["cyan"]   = MSG::CYAN;
  m_colMap["white"]  = MSG::WHITE;

  // make sure the map of logged stream names is initialized
  setupLogStreams();

  return StatusCode::SUCCESS;
}

//#############################################################################

/// Reinitialize Service
StatusCode MessageSvc::reinitialize() {
  m_state = Gaudi::StateMachine::OFFLINE;
  return initialize();
}

//#############################################################################

void MessageSvc::initColors(Property& /*prop*/) {

  if (m_color) {
     static const std::array<std::pair<MSG::Level,std::vector<std::string>>,3> tbl{
                          { { MSG::FATAL, { { "[94;101;1m" } } },
                            { MSG::ERROR, { { "[97;101;1m" } } },
                            { MSG::WARNING, { { "[93;1m" } } } } };

     for (const auto& p : tbl ) {
        auto &lC = m_logColors[p.first];
        if (lC.value().empty()) {
            lC.set(p.second);
    } else {
            MessageSvc::setupColors( lC );
    }
    }

  } else {

    // reset all color codes;
    for (int ic=0; ic<MSG::NUM_LEVELS; ++ic) {
      m_logColors[ic].set( { } );
    }

  }

}

//#############################################################################

void MessageSvc::setupColors(Property& prop) {

  if (! m_color) return;

  static const std::array<std::pair<const char*,MSG::Level>,7> tbl { 
      { {"fatalColorCode",   MSG::FATAL   },
        {"errorColorCode",   MSG::ERROR   },
        {"warningColorCode", MSG::WARNING },
        {"infoColorCode",    MSG::INFO    },
        {"debugColorCode",   MSG::DEBUG   },
        {"verboseColorCode", MSG::VERBOSE },
        {"alwaysColorCode",  MSG::ALWAYS  } } };

  auto i = std::find_if( std::begin(tbl),std::end(tbl),
                         [&](const std::pair<const char*,MSG::Level>& t) {
                                return prop.name() == t.first;
  } );
  if (i==std::end(tbl)) {
    cout << "ERROR: Unknown message color parameter: " << prop.name()
         << endl;
    return;
  }
  int ic = i->second;

  string code;
  auto itr = m_logColors[ic].value().begin();

  if ( m_logColors[ic].value().size() == 1 ) {

    if (*itr == "") {
      code = "";
    } else if (itr->compare(0,1,"[") == 0) {
      code = "\033" + *itr;
    } else {
      code = "\033[" + colTrans(*itr, 90) + ";1m";
    }

  } else if (m_logColors[ic].value().size() == 2) {
    auto itr2 = itr + 1;

    code =  "\033[" + colTrans(*itr, 90) + ";"
      + colTrans(*itr2, 100) + ";1m";

  }

  m_logColorCodes[ic] = code;

}
//#############################################################################

void MessageSvc::setupLimits(Property& prop) {
  // Just report problems in the settings of the limits and unknown limit parameters
  if (prop.name() == "alwaysLimit") {
    IntegerProperty *p = dynamic_cast<IntegerProperty*>(&prop);
    if (p && p->value() != 0) {
      cout << "MessageSvc ERROR: cannot suppress ALWAYS messages" << endl;
      p->setValue(0);
    }
  } else if (prop.name() == "defaultLimit") {
    for (int i = MSG::VERBOSE; i< MSG::NUM_LEVELS; ++i) {
      if (i != MSG::ALWAYS) {
        m_msgLimit[i] = m_msgLimit[MSG::NIL].value();
      }
    }
  } else if (prop.name() != "fatalLimit" &&
             prop.name() != "errorLimit" &&
             prop.name() != "warningLimit" &&
             prop.name() == "infoLimit" &&
             prop.name() == "debugLimit" &&
             prop.name() == "verboseLimit") {
    cout << "MessageSvc ERROR: Unknown message limit parameter: "
         << prop.name() << endl;
    return;
  }
}
//#############################################################################

void MessageSvc::setupThreshold(Property& prop) {

  static const std::array<std::pair<const char*,MSG::Level>,7> tbl{ 
      { { "setFatal",   MSG::FATAL   },
        { "setError",   MSG::ERROR   },
        { "setWarning", MSG::WARNING },
        { "setInfo",    MSG::INFO    },
        { "setDebug",   MSG::DEBUG   },
        { "setVerbose", MSG::VERBOSE },
        { "setAlways",  MSG::ALWAYS  } } };

  auto i = std::find_if( std::begin(tbl),std::end(tbl),
                         [&](const std::pair<const char*,MSG::Level>& t) {
                                return prop.name() == t.first;
  } );
  if (i==std::end(tbl)) {
    cerr << "MessageSvc ERROR: Unknown message threshold parameter: "
         << prop.name() << endl;
    return;
  }
  int ic = i->second;

  StringArrayProperty *sap = dynamic_cast<StringArrayProperty*>( &prop);
  if (!sap) {
    std::cerr << "could not dcast " << prop.name()
              << " to a StringArrayProperty (which it should be!)" << endl;
  } else {
    for ( auto& i : sap->value() ) setOutputLevel( i, ic );
  }

}

//#############################################################################

#ifdef NDEBUG
void MessageSvc::setupInactCount(Property&) {}
#else
void MessageSvc::setupInactCount(Property&prop) {
  if (prop.name() == "countInactive") {
    BooleanProperty *p = dynamic_cast<BooleanProperty*>(&prop);
    if (p)
      MsgStream::enableCountInactive(p->value());
  }
}
#endif


//#############################################################################
/// Finalize Service
StatusCode MessageSvc::finalize() {

  m_suppress = false;

  {
    std::ostringstream os;

    if (m_stats) {
      os << "Summarizing all message counts" << endl;
    } else {
      os << "Listing sources of suppressed message: " << endl;
    }

    os << "=====================================================" << endl;
    os << " Message Source              |   Level |    Count" << endl;
    os << "-----------------------------+---------+-------------" << endl;


    bool found(false);

    for (auto itr=m_sourceMap.begin(); itr!=m_sourceMap.end(); ++itr) {
      for (unsigned int ic = 0; ic < MSG::NUM_LEVELS; ++ic) {
        if ( (itr->second.msg[ic] >= m_msgLimit[ic] && m_msgLimit[ic] != 0 ) ||
            (m_stats && itr->second.msg[ic] > 0 && ic >= m_statLevel.value()) ) {
          os << " ";
          os.width(28);
          os.setf(ios_base::left,ios_base::adjustfield);
          os << itr->first;

          os << "|";

          os.width(8);
          os.setf(ios_base::right,ios_base::adjustfield);
          os << levelNames[ic];

          os << " |";

          os.width(9);
          os << itr->second.msg[ic];

          os << endl;

          found = true;
        }
      }
    }
    os << "=====================================================" << endl;

    if (found || m_stats) {
      cout << os.str();
    }
  }

#ifndef NDEBUG
  if (m_inactCount.value()) {

    std::ostringstream os;
    os << "Listing sources of Unprotected and Unseen messages\n";

    bool found(false);

    unsigned int ml(0);
    for (const auto& itr : m_inactiveMap) {
      for (unsigned int ic = 0; ic < MSG::NUM_LEVELS; ++ic) {
        if (itr.second.msg[ic] != 0 && itr.first.length() > ml) {
          ml = itr.first.length(); 
        }
      }
    }

    for (unsigned int i=0; i<ml+25; ++i) os << "=";

    os << std::endl << " ";
    os.width(ml+2);
    os.setf(ios_base::left,ios_base::adjustfield);
    os << "Message Source";
    os.width(1);
    os << "|   Level |    Count" << endl;

    for (unsigned int i=0; i<ml+3; ++i) {
      os << "-";
    }
    os << "+---------+-----------" << endl;


    for (auto itr=m_inactiveMap.begin(); itr!=m_inactiveMap.end(); ++itr) {
      for (unsigned int ic = 0; ic < MSG::NUM_LEVELS; ++ic) {
	if (itr->second.msg[ic] != 0) {
	  os << " ";
	  os.width(ml+2);
	  os.setf(ios_base::left,ios_base::adjustfield);
	  os << itr->first;

	  os << "|";

	  os.width(8);
	  os.setf(ios_base::right,ios_base::adjustfield);
	  os << levelNames[ic];

	  os << " |";

	  os.width(9);
	  os << itr->second.msg[ic];

	  os << endl;

	  found = true;
	}
      }
    }
    for (unsigned int i=0; i<ml+25; ++i) {
      os << "=";
    }
    os << endl;

    if (found) {
      cout << os.str();
    }
  }
#endif

  return StatusCode::SUCCESS;
}

//#############################################################################
std::string MessageSvc::colTrans(std::string col, int offset) {
  auto itr = m_colMap.find(col);
  int icol = offset + ( (itr != m_colMap.end()) ? itr->second : 8 );
  return std::to_string( icol );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: reportMessage
// Purpose: dispatches a message to the relevant streams.
// ---------------------------------------------------------------------------
//

void MessageSvc::reportMessage( const Message& msg, int outputLevel )    {
  std::unique_lock<std::recursive_mutex> lock(m_reportMutex);

  int key = msg.getType();

  m_msgCount[key] ++;

  const Message *cmsg = &msg;

  // processing logged streams
  if ( !m_loggedStreams.empty() ) {
    auto iLog = m_loggedStreams.find( msg.getSource() );
    if ( m_loggedStreams.end() != iLog ) {
      (*iLog->second) << *cmsg << std::endl;
    }
  }

  if ( m_suppress.value() || m_stats.value() ) {

    // Increase the counter of 'key' type of messages for the source and
    // get the new value.
    const int nmsg = ++(m_sourceMap[msg.getSource()].msg[key]);

    if (m_suppress.value()) {

      if ( m_msgLimit[key] != 0 ) {
        if (nmsg == m_msgLimit[key]) {
          std::string txt = levelNames[key] + " message limit ("
              + std::to_string( m_msgLimit[key].value() )
              + ") reached for " + msg.getSource() + ". Suppressing further output.";
          cmsg = new Message(msg.getSource(), MSG::WARNING, txt);
          cmsg->setFormat(msg.getFormat());
        } else if (nmsg > m_msgLimit[key]) {
          return;
        }
      }
    }

  }

  auto first = m_streamMap.lower_bound( key );
  if ( first != m_streamMap.end() ) {
    auto last = m_streamMap.upper_bound( key );
    while( first != last ) {
      std::ostream& stream = *( (*first).second.second );
      stream << *cmsg << std::endl;
      first++;
    }
  }
  else if ( key >= outputLevel )   {
    msg.setFormat(m_defaultFormat);
    msg.setTimeFormat(m_defaultTimeFormat);
    if (!m_color) {
      (*m_defaultStream) << *cmsg << std::endl << std::flush;
    } else {
      (*m_defaultStream) <<  m_logColorCodes[key] << *cmsg << "\033[m"
                         << std::endl << std::flush;
    }
  }

  if (cmsg != &msg) { delete cmsg; }

}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: reportMessage
// Purpose: dispatches a message to the relevant streams.
// ---------------------------------------------------------------------------
//
void MessageSvc::reportMessage( const Message& msg )    {
  reportMessage(msg, outputLevel(msg.getSource()));
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: reportMessage
// Purpose: dispatches a message to the relevant streams.
// ---------------------------------------------------------------------------
//
void MessageSvc::reportMessage (const char* source,
                                int type,
                                const char* message) {
  Message msg( source, type, message);
  reportMessage( msg );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: reportMessage
// Purpose: dispatches a message to the relevant streams.
// ---------------------------------------------------------------------------
//
void MessageSvc::reportMessage (const std::string& source,
                                int type,
                                const std::string& message) {
  Message msg( source, type, message);
  reportMessage( msg );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: sendMessage
// Purpose: finds a message for a given status code and dispatches it.
// ---------------------------------------------------------------------------
//

void MessageSvc::reportMessage (const StatusCode& key,
                                const std::string& source)
{
  std::unique_lock<std::recursive_mutex> lock(m_messageMapMutex);

  auto first = m_messageMap.lower_bound( key );
  if ( first != m_messageMap.end() ) {
    auto last = m_messageMap.upper_bound( key );
    while( first != last ) {
      Message msg = (*first).second;
      msg.setSource( source );
      Message stat_code1( source, msg.getType(), "Status Code " + std::to_string(key.getCode()) );
      reportMessage( stat_code1 );
      reportMessage( msg );
      first++;
    }
  }
  else {
    Message mesg = m_defaultMessage;
    mesg.setSource( source );
    Message stat_code2( source,  mesg.getType(), "Status Code " + std::to_string( key.getCode() ) );
    reportMessage( stat_code2 );
    reportMessage( mesg );
  }
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: insertStream
// Purpose: inserts a stream for a message type.
// ---------------------------------------------------------------------------
//

void MessageSvc::insertStream (int key,
                               const std::string& name,
                               std::ostream *stream)
{
  m_streamMap.insert( { key, NamedStream(name,stream) } );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseStream
// Purpose: erases all the streams for all the message types.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseStream()
{
  m_streamMap.clear();
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseStream
// Purpose: erases all the streams for a message type.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseStream( int message_type )
{
  m_streamMap.erase( message_type );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseStream
// Purpose: erases one stream for a message type.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseStream( int key, std::ostream* stream )   {
  if ( stream )    {
    bool changed = true;
    while( changed ) {
      changed = false;
      auto first = m_streamMap.lower_bound( key );
      auto last = m_streamMap.upper_bound( key );
      auto i = std::find_if( first, last, 
                             [&](StreamMap::const_reference j) { return j.second.second == stream; } );
      if ( i != last ) {
          m_streamMap.erase( i );
          changed = true;
      }
    }
  }
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseStream
// Purpose: erases one stream for all message types.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseStream( std::ostream* stream )    {
  if ( stream )    {
    bool changed = true;
    while( changed ) {
      changed = false;
      auto i = std::find_if( m_streamMap.begin(), m_streamMap.end(),
                             [&](StreamMap::const_reference j) { return j.second.second == stream; } );
      if ( i != m_streamMap.end() ) {
          m_streamMap.erase( i );
          changed = true;
      }
    }
  }
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: insertMessage
// Purpose: inserts a message for a status code.
// ---------------------------------------------------------------------------
//

void MessageSvc::insertMessage( const StatusCode& key, const Message& msg )
{
  std::unique_lock<std::recursive_mutex> lock(m_messageMapMutex);
  m_messageMap.insert( { key, msg } );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseMessage
// Purpose: erases all the messages for all the status codes.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseMessage()
{
  std::unique_lock<std::recursive_mutex> lock(m_messageMapMutex);
  m_messageMap.clear();
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseMessage
// Purpose: erases all the messages for a status code.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseMessage( const StatusCode& key )
{
  std::unique_lock<std::recursive_mutex> lock(m_messageMapMutex);
  m_messageMap.erase( key );
}

//#############################################################################
// ---------------------------------------------------------------------------
// Routine: eraseMessage
// Purpose: erases one message for a status code.
// ---------------------------------------------------------------------------
//

void MessageSvc::eraseMessage( const StatusCode& key, const Message& msg )
{
  std::unique_lock<std::recursive_mutex> lock(m_messageMapMutex);

  bool changed = true;
  while( changed ) {
    changed = false;
    auto first = m_messageMap.lower_bound( key );
    auto last = m_messageMap.upper_bound( key );
    auto i = std::find_if( first, last, [&](MessageMap::const_reference j) { return j.second==msg; } );
    if ( i!=last ) {
        m_messageMap.erase( i );
        changed = true;
    }
  }
}

// ---------------------------------------------------------------------------
int MessageSvc::outputLevel()   const {
// ---------------------------------------------------------------------------
  return m_outputLevel;
}

// ---------------------------------------------------------------------------
int MessageSvc::outputLevel( const std::string& source )   const {
// ---------------------------------------------------------------------------
  std::unique_lock<std::recursive_mutex> lock(m_thresholdMapMutex);
  auto it = m_thresholdMap.find( source );
  return ( it != m_thresholdMap.end() ) ?  it->second : m_outputLevel.value();
}

// ---------------------------------------------------------------------------
void MessageSvc::setOutputLevel(int new_level)    {
// ---------------------------------------------------------------------------
  m_outputLevel = new_level;
}

// ---------------------------------------------------------------------------
void MessageSvc::setOutputLevel(const std::string& source, int level)    {
// ---------------------------------------------------------------------------
  std::unique_lock<std::recursive_mutex> lock(m_thresholdMapMutex);

  /*
  auto p = m_thresholdMap.insert(ThresholdMap::value_type( source, level) );
  if( p.second == false ) {
    // Already esisting an output level for that source. Erase and enter it again
    m_thresholdMap.erase ( p.first );
    m_thresholdMap.insert(ThresholdMap::value_type( source, level) );
  }
  */
  m_thresholdMap[source] = level;
}

// ---------------------------------------------------------------------------
std::string MessageSvc::getLogColor(int logLevel) const   {
// ---------------------------------------------------------------------------
  return (logLevel < MSG::NUM_LEVELS) ? m_logColorCodes[logLevel] : "" ;
}

// ---------------------------------------------------------------------------
int MessageSvc::messageCount( MSG::Level level) const   {

  return m_msgCount[level];

}

// ---------------------------------------------------------------------------
void
MessageSvc::incrInactiveCount(MSG::Level level, const std::string& source) {

  ++(m_inactiveMap[source].msg[level]);

}

// ---------------------------------------------------------------------------
void MessageSvc::setupLogStreams()
{
  // reset state
  m_loggedStreams.clear();

  for ( auto& iProp : m_loggedStreamsName ) {

    std::set<std::string> outFileNames;
    for ( auto& jProp : m_loggedStreamsName ) {
      if ( jProp.first != iProp.first ) {
        outFileNames.insert( jProp.second );
      }
    }
    tee( iProp.first, iProp.second, outFileNames );

  }//> loop over property entries
}

// ---------------------------------------------------------------------------
void MessageSvc::tee( const std::string& sourceName,
                      const std::string& outFileName,
                      const std::set<std::string>& outFileNames )
{
  const std::ios_base::openmode openMode = std::ios_base::out |
                                           std::ios_base::trunc;

  auto iStream = m_loggedStreams.find( sourceName );
  if ( iStream != std::end(m_loggedStreams) ) {
    m_loggedStreams.erase( iStream );
  }

  // before creating a new ofstream, make sure there is no already existing
  // one with the same file name...
  for (auto&  iStream : m_loggedStreams ) {
    if ( outFileNames.find( outFileName ) != outFileNames.end() ) {
      m_loggedStreams[sourceName] = m_loggedStreams[iStream.first];
      return;
    }
  }

  auto out = std::make_shared<std::ofstream>( outFileName.c_str(), openMode );
  if ( out->good() ) {
    m_loggedStreams[sourceName] = std::move(out);
  } else { 
    out->close();
  }

}
