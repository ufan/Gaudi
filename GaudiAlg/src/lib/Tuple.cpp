// $Id: Tuple.cpp,v 1.2 2005/01/18 15:51:53 mato Exp $
// ============================================================================
// GaudiAlg
// ============================================================================
#include "GaudiAlg/Tuple.h"
#include "GaudiAlg/TupleObj.h"
// ============================================================================

// ============================================================================
/** @file Tuple.cpp
 *
 *  Implementation file for class : Tuple
 *
 *  @date 2002-10-30
 *  @author Vanya Belyaev Ivan.Belyaev@itep.ru
 */
// ============================================================================

// ============================================================================
/** Standard constructor
 *  @param tuple pointer to "real" tuple object
 */
// ============================================================================
Tuples::Tuple::Tuple( Tuples::TupleObj* tuple )
  : m_tuple( tuple )
{
  if( m_tuple ) { m_tuple -> addRef () ; }
}
// ============================================================================

// ============================================================================
/** copy constructor
 */
// ============================================================================
Tuples::Tuple::Tuple( const Tuples::Tuple&    tuple )
  : m_tuple ( tuple.m_tuple )
{
  if( m_tuple ) { m_tuple ->addRef() ; }
}
// ============================================================================

// ============================================================================
/** assignment  operator
 *  Tuples could be assigned in a safe way
 *  @param tuple tuple to be assigned
 */
// ============================================================================
Tuples::Tuple& Tuples::Tuple::operator=( const Tuples::Tuple& tuple )
{
  // self assigenment
  if( &tuple == this ) { return *this; }
  // temporary variable
  auto  tmp = tuple.m_tuple ;
  // increase reference count
  if( tmp  ) { tmp     -> addRef  () ; }
  // decrease reference count
  if( m_tuple ) { m_tuple -> release () ; }
  // assign
  m_tuple = tmp ;
  //
  return *this ;
}
// ============================================================================

// ============================================================================
// destructor
// ============================================================================
Tuples::Tuple::~Tuple()
{
  if( m_tuple ) { m_tuple->release() ; }
}
// ============================================================================

// ============================================================================
// The END
// ============================================================================
