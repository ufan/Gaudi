#include "ReEntAlg.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/MsgStream.h"

#include <chrono>
#include <thread>

DECLARE_COMPONENT( ReEntAlg )

///////////////////////////////////////////////////////////////////////////

ReEntAlg::ReEntAlg( const std::string& name, ISvcLocator* pSvcLocator ) : Gaudi::Algorithm( name, pSvcLocator ) {}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode ReEntAlg::initialize() {
  auto sc = Gaudi::Algorithm::initialize();
  if ( !sc ) return sc;
  info() << "initialize()" << endmsg;
  return sc;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode ReEntAlg::execute( const EventContext& ctx ) const {
  // wait a little bit to make sure the printouts are in a stable order
  std::this_thread::sleep_for( std::chrono::milliseconds( 50 * ctx.slot() ) );

  info() << "execute(): context: (" << ctx << ") index: " << index() << " cardinality: " << cardinality() << endmsg;

  std::chrono::milliseconds dt{m_sleep + ctx.slot() * 500};
  std::this_thread::sleep_for( dt );

  info() << "... done in " << dt.count() << " ms for " << ctx << endmsg;
  std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

  return StatusCode::SUCCESS;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

StatusCode ReEntAlg::finalize() {
  info() << "finalize()" << endmsg;
  return Gaudi::Algorithm::finalize();
}
