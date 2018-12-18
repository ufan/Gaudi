// ============================================================================
#ifndef GAUDIKERNEL_ICHRONOSTATSVC_H
#define GAUDIKERNEL_ICHRONOSTATSVC_H
// ============================================================================
// Include files
// ============================================================================
// STD & STL
// ============================================================================
#include <iostream>
#include <string>
// ============================================================================
// GaudiKernel
// ============================================================================
#include "GaudiKernel/IChronoSvc.h"
#include "GaudiKernel/IService.h"
#include "GaudiKernel/IStatSvc.h"
// ============================================================================
/** @class IChronoStatSvc IChronoStatSvc.h GaudiKernel/IChronoStatSvc.h
 *
 *  The IChronoStatSvc is the interface implemented by the ChronoStatService.
 *  This interface is used by any algorithm or services wanting to study
 *  its own performance and CPU consumption and some statistical computation
 *
 *  @see IChronoSvc
 *  @see IStatSvc
 *
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 *         (inspired by Mauro VILLA's codes used within ARTE framework )
 *
 *  @date December 1, 1999
 *  @date 2007-07-08
 */
class GAUDI_API IChronoStatSvc : virtual public extend_interfaces<IService, IChronoSvc, IStatSvc>
{
public:
  /// InterfaceID
  DeclareInterfaceID( IChronoStatSvc, 6, 0 );
};
// ============================================================================
// The END
// ============================================================================
#endif //  GAUDIKERNEL_ICHRONOSTATSVC_H
