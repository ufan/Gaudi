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
#ifndef GAUDISVC_PERSISTENCYSVC_RUNRECORDSTREAM_H
#define GAUDISVC_PERSISTENCYSVC_RUNRECORDSTREAM_H

// Required for inheritance
#include "OutputStream.h"

/** @class RunRecordStream RunRecordStream.h
 * Extension of OutputStream to write run records after last event
 *
 * @author  M.Frank
 * @version 1.0
 */
class RunRecordStream : public OutputStream {
public:
  /// Standard algorithm Constructor
  RunRecordStream( const std::string& nam, ISvcLocator* svc ) : OutputStream( nam, svc ) {}
  /// Runrecords do not get written for each event: Event processing hence dummy....
  StatusCode execute() override { return StatusCode::SUCCESS; }
  /// Algorithm overload: finalization
  StatusCode finalize() override;
};

#endif // GAUDISVC_PERSISTENCYSVC_OUPUTFSRSTREAM_H
