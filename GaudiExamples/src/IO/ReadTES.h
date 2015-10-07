#ifndef POOLIO_READTES_H 
#define POOLIO_READTES_H 1

// Include files

#include <string>
#include <vector>

// from Gaudi
#include "GaudiAlg/GaudiAlgorithm.h"


/** @class ReadTES ReadTES.h
 *  
 *
 *  @author Marco Cattaneo
 *  @date   2008-11-03
 */
class ReadTES : public GaudiAlgorithm {
public: 
  /// Standard constructor
  ReadTES( const std::string& name, ISvcLocator* pSvcLocator );

  virtual ~ReadTES( ); ///< Destructor

  virtual StatusCode initialize();    ///< Algorithm initialization
  virtual StatusCode execute   ();    ///< Algorithm execution

protected:

private:
  std::vector<std::string> m_locations; ///< Objects to read from TES
};
#endif // POOLIO_READTES_H
