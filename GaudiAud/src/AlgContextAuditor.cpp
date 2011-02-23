// $Id: AlgContextAuditor.cpp,v 1.4 2007/11/13 12:53:54 marcocle Exp $
// ============================================================================
// CVS tag $Name:  $, version $Revision: 1.4 $
// ============================================================================
// $Log: AlgContextAuditor.cpp,v $
// Revision 1.4  2007/11/13 12:53:54  marcocle
// Charles Leggett
//  - bug #28570.
//    Modified AlgContextAuditor to avoid that it passes a null pointer to
//    AlgContextSvc. It happens if the AuditorSvc is auditing objects that inherit
//    from INamedInterface, but not from IAlgorithm (e.g. services).
//
// Revision 1.3  2007/05/24 13:49:20  hmd
// ( Vanya Belyaev) patch #1171. The enhancement of existing Algorithm Context Service
//    is the primary goal of the proposed patch. The existing
//    AlgContextSvc is not safe with respect to e.g. Data-On-Demand
//    service or to operations with subalgorithms. The patched service
//    essentially implements the queue of executing algorithms, thus the
//    problems are eliminiated. In addition the enriched interface
//    provides the access to the whole queue of executing algorithms.
//
// ============================================================================
// Incldue files
// ============================================================================
// STD & STL
// ============================================================================
#include <cassert>
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/IAlgContextSvc.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/AudFactory.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/MsgStream.h"
// ============================================================================
// local
// ============================================================================
#include "AlgContextAuditor.h"
// ============================================================================
/** @file
 *  Implementation file for class AlgContexAuditor
 *  @author M. Shapiro, LBNL
 *  @author modified by Vanya BELYAEV ibelyaev@physics.syr.edu
 */
// ============================================================================
namespace
{
  /** make a safe cast using "smart interface"
   *  @see  INamedInterface
   *  @see  IAlgorithm
   *  @see   SmartIF
   *  @see   IInterface::queryInterface
   *  @param  i pointer to INamedInterface
   *  @return pointer to IAlgorithm
   */
  inline IAlgorithm* toAlg ( IInterface* ni )
  {
    if ( 0 == ni ) { return 0 ; }
    SmartIF<IAlgorithm> alg ( ni ) ;
    return alg ;
  }
}
// ============================================================================
// mandatory auditor fatcory, needed for instantiation
// ============================================================================
DECLARE_AUDITOR_FACTORY(AlgContextAuditor)
// ============================================================================
// standard constructor @see Auditor
// ============================================================================
AlgContextAuditor::AlgContextAuditor
( const std::string& name ,
  ISvcLocator*       pSvc )
  : Auditor( name , pSvc )
  , m_svc   ( 0    )
{}
// ============================================================================
// destructor
// ============================================================================
AlgContextAuditor::~AlgContextAuditor() {}
// ============================================================================
// standard initialization, see @IAuditor
// ============================================================================
StatusCode AlgContextAuditor::initialize()
{
  // initialize the base class
  StatusCode sc = Auditor::initialize() ;
  if ( sc.isFailure() ) { return sc ; }                           // RETURN
  if ( 0 != m_svc ) { m_svc -> release() ; m_svc = 0 ; }
  sc = Auditor::service ( "AlgContextSvc" , m_svc , true ) ;
  if ( sc.isFailure() )
  {
    MsgStream log ( msgSvc() , name() ) ;
    log << MSG::ERROR << "Unable to locate 'AlgContextSvc'" << sc << endmsg ;
    m_svc = 0 ;
    return sc ;  // RETURN
  }
  if ( 0 == m_svc     )
  {
    MsgStream log ( msgSvc() , name() ) ;
    log << MSG::ERROR << "Invalid pointer to IAlgContextSvc" << endmsg ;
    return StatusCode::FAILURE ;           // RETURN
  }
  return StatusCode::SUCCESS ;
}
// ============================================================================
// standard finalization, see @IAuditor
// ============================================================================
StatusCode AlgContextAuditor::finalize ()
{
  if ( 0 != m_svc ) { m_svc-> release() ; m_svc = 0 ; }
  // finalize the base class
  return Auditor::finalize () ;
}
// ============================================================================
void AlgContextAuditor::beforeInitialize ( INamedInterface*  a ) {
  if ( 0 != m_svc ) {
    IAlgorithm* alg = toAlg(a);
    if (alg != 0) m_svc -> setCurrentAlg ( alg ).ignore() ;
  }
}
// ============================================================================
void AlgContextAuditor::afterInitialize  ( INamedInterface*  a ) {
  if ( 0 != m_svc ) {
    IAlgorithm* alg = toAlg(a);
    if (alg != 0) m_svc -> unSetCurrentAlg ( alg ).ignore() ;
  }
}
// ============================================================================
void AlgContextAuditor::beforeFinalize   ( INamedInterface*  a ) {
  if ( 0 != m_svc ) {
    IAlgorithm* alg = toAlg(a);
    if (alg != 0) m_svc -> setCurrentAlg ( alg ).ignore() ;
  }
}
// ============================================================================
void AlgContextAuditor::afterFinalize    ( INamedInterface*  a ) {
  if ( 0 != m_svc ) {
    IAlgorithm* alg = toAlg(a);
    if (alg != 0) m_svc -> unSetCurrentAlg ( alg ).ignore() ;
  }
}
// ============================================================================
void AlgContextAuditor::beforeExecute    ( INamedInterface*  a ) {
  if ( 0 != m_svc ) {
    IAlgorithm* alg = toAlg(a);
    if (alg != 0) m_svc -> setCurrentAlg ( alg ).ignore() ;
  }
}
// ============================================================================
void AlgContextAuditor::afterExecute     ( INamedInterface*  a       ,
                                           const StatusCode& /* s */ ) {
  if ( 0 != m_svc ) {
    IAlgorithm* alg = toAlg(a);
    if (alg != 0) m_svc -> unSetCurrentAlg ( alg ).ignore() ;
  }
}
// ============================================================================

// ============================================================================
// The END
// ============================================================================

