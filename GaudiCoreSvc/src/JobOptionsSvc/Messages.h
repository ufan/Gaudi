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
#ifndef JOBOPTIONSVC_MESSAGES_H_
#define JOBOPTIONSVC_MESSAGES_H_
// ============================================================================
// STD & STL
// ============================================================================
#include <iostream>
#include <string>
#include <vector>
// ============================================================================
// Boost
// ============================================================================

// ============================================================================
// Gaudi:
// ============================================================================
#include "GaudiKernel/MsgStream.h"
#include "Position.h"
// ============================================================================
namespace Gaudi {
  namespace Parsers {
    class Messages final {
    public:
      Messages( MsgStream& stream ) : stream_( stream ) {}
      // Messages(const MsgStream& stream):stream_(stream){}
      void AddInfo( const std::string& info ) { AddMessage( MSG::INFO, info ); }

      void AddWarning( const std::string& warning ) { AddMessage( MSG::WARNING, warning ); }

      void AddError( const std::string& error ) { AddMessage( MSG::ERROR, error ); }

      void AddInfo( const Position& pos, const std::string& info ) { AddMessage( MSG::INFO, pos, info ); }

      void AddWarning( const Position& pos, const std::string& warning ) { AddMessage( MSG::WARNING, pos, warning ); }

      void AddError( const Position& pos, const std::string& error ) { AddMessage( MSG::ERROR, pos, error ); }

    private:
      void AddMessage( MSG::Level level, const std::string& message );

      void AddMessage( MSG::Level level, const Position& pos, const std::string& message );

    private:
      MsgStream& stream_;
      /// Name of last printed filename.
      std::string m_currentFilename;
    };

    // ============================================================================

    // ============================================================================
  } // namespace Parsers
} // namespace Gaudi
// ============================================================================

#endif // JOBOPTIONSVC_MESSAGES_H_
