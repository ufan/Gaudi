// ============================================================================
// Include files
// ============================================================================
// STD & STL
// ============================================================================
#include <cmath>
#include <cstdio>
#include <iostream>
// ============================================================================
// Gaudi
// ============================================================================
#include "GaudiKernel/SystemOfUnits.h"
#include "GaudiMath/GaudiMath.h"
// ============================================================================
// CLHEP
// ============================================================================
#include "CLHEP/GenericFunctions/Sin.hh"
// ============================================================================
// Handle CLHEP 2.0.x move to CLHEP namespace
namespace CLHEP
{
}
using namespace CLHEP;

// ============================================================================
/** @file
 *
 *  Test file for the class NumericalDerivative
 *
 *  @date 2003-08-31
 *  @author Vanya  BELYAEV Ivan.Belyaev@itep.ru
 */
// ============================================================================

int main()
{

  std::cout << " Test for numerical differentiation of Genfun::Sin(x) " << std::endl;

  const GaudiMath::Function&   mysin = Genfun::Sin();
  const GaudiMath::Derivative& prim1 = GaudiMath::Derivative( mysin, 0 );
  const GaudiMath::Function&   prim  = prim1;

  for ( double x = -90 * Gaudi::Units::degree; x <= 180 * Gaudi::Units::degree; x += 10 * Gaudi::Units::degree ) {
    double value = prim( x );
    double error = prim1.error();
    printf( " x=%8.3f deg; Sin'=%+.19f; ActualErr=%+.19f; EstimatedErr=%+.19f\n", x / Gaudi::Units::degree, value,
            value - cos( x ), error );
  }
}

// ============================================================================
// The END
// ============================================================================
