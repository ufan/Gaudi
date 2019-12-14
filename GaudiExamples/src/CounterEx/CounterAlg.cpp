/***********************************************************************************\
* (c) Copyright 1998-2019 CERN for the benefit of the LHCb and ATLAS collaborations *
*                                                                                   *
* This software is distributed under the terms of the Apache version 2 licence,     *
* copied verbatim in the file "LICENSE".                                            *
*                                                                                   *
* In applying this licence, CERN does not waive the privileges and immunities       *
* granted to it by virtue of its status as an Intergovernmental Organization        *
* or submit itself to any jurisdiction.                                             *
\***********************************************************************************/
// ============================================================================
// Include files
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/RndmGenerators.h"
// ============================================================================
// GaudiAlg
// ============================================================================
#include "GaudiAlg/GaudiAlgorithm.h"
// ============================================================================

// ============================================================================
/** @file
 *  Simple exmaple of usage of different "counters"
 *
 *  @see GaudiAlgorithm
 *  @see StatEntity
 *
 *  @author Vanya BELYAEV Ivan.Belyaev@lapp.in2p3.fr
 *  @date 2008-08-06
 */
// ============================================================================

// ============================================================================
/** @class CounterAlg
 *
 *  Simple algorithm whioch inllustartes the usage
 *  of different "counters"
 *
 *  @see GaudiAlgorithm
 *  @see StatEntity
 *
 *  @author Vanya BELYAEV Ivan.Belyaev@lapp.in2p3.fr
 *  @date 2008-08-06
 */
// ============================================================================
class CounterAlg : public GaudiAlgorithm {
public:
  /** the only one essential method
   *  @return status code
   */
  StatusCode execute() override;

  /** standard contructor
   *  @param name algorithm istance name
   *  @param pSvc pointer to Service Locator
   */
  CounterAlg( const std::string& name, ISvcLocator* pSvc ) : GaudiAlgorithm( name, pSvc ) {
    setProperty( "StatPrint", "true" ).ignore();
  }

  // copy constructor is disabled
  CounterAlg( const CounterAlg& ) = delete;
  // assignement operator is disabled
  CounterAlg& operator=( const CounterAlg& ) = delete;
};
// ============================================================================

// ============================================================================
DECLARE_COMPONENT( CounterAlg )
// ============================================================================

// ============================================================================
/** the only one essential method
 *  @return status code
 */
// ============================================================================
StatusCode CounterAlg::execute() {

  // count overall number of executions:
  ++counter( "executed" );

  Rndm::Numbers gauss( randSvc(), Rndm::Gauss( 0.0, 1.0 ) );
  Rndm::Numbers poisson( randSvc(), Rndm::Poisson( 5.0 ) );

  // 'accumulate' gauss
  const double value = gauss();

  counter( "gauss" ) += value;
  counter( "g2" ) += value * value;

  ( 0 < value ) ? ++counter( "Gpos" ) : ++counter( "Gneg" );

  StatEntity& stat1 = counter( "NG" );
  StatEntity& stat2 = counter( "G" );

  const int num = (int)poisson();
  for ( int i = 0; i < num; ++i ) {
    stat1++;
    stat2 += gauss();
  }

  // assignement
  counter( "assign" ) = value;

  // counter of efficiency
  counter( "eff" ) += ( 0 < value );

  // print the statistics every 1000 events
  const StatEntity& executed = counter( "executed" );
  const int         print    = (int)executed.flag();
  if ( 0 == print % 1000 ) {
    info() << " Event number " << print << endmsg;
    printStat();
    info() << " Efficiency (binomial counter: \"eff\"): (" << counter( "eff" ).eff() * 100.0 << " +- "
           << counter( "eff" ).effErr() * 100.0 << ")%" << endmsg;
  }

  return StatusCode::SUCCESS;
}
// ============================================================================
