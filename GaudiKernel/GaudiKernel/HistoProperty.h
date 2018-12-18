#ifndef GAUDIKERNEL_HISTOPROPERTY_H
#define GAUDIKERNEL_HISTOPROPERTY_H 1
// ============================================================================
// Include files
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/HistoDef.h"
#include "GaudiKernel/Property.h"
namespace Gaudi
{
  namespace Details
  {
    namespace Property
    {
      // ============================================================================
      /** Check if the value is OK
       *  it is a nesessary template specialisation to avoid the compilation error
       *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
       *  @date 2007-09-18
       */
      template <>
      inline void BoundedVerifier<Gaudi::Histo1DDef>::operator()( const Gaudi::Histo1DDef& value ) const
      {
        if ( !value.ok() || ( m_hasLowerBound && ( value < m_lowerBound ) ) ||
             ( m_hasUpperBound && ( m_upperBound < value ) ) )
          throw std::out_of_range( "value " + Gaudi::Utils::toString( value ) + " outside range" );
      }
      // ============================================================================
      /** Check if the value is OK
       *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
       *  @date 2007-09-18
       */
      template <>
      inline void NullVerifier::operator()<Gaudi::Histo1DDef>( const Gaudi::Histo1DDef& value ) const
      {
        if ( !value.ok() ) throw std::invalid_argument( "bad value " + Gaudi::Utils::toString( value ) );
      }
      // ============================================================================
    }
  }
} // end of namespace Gaudi::Details::Property
// ============================================================================
/// the actual type of "histogram property"
typedef Gaudi::Property<Gaudi::Histo1DDef> Histo1DProperty;
// ============================================================================
/// the actual type of "histogram property reference"
typedef Gaudi::Property<Gaudi::Histo1DDef&> Histo1DPropertyRef;
// ============================================================================

// ============================================================================
// The END
// ============================================================================
#endif // GAUDIKERNEL_HISTOPROPERTY_H
