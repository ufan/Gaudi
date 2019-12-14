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
#ifndef AIDATUPLES_AIDATupleAlgorithmWrite_H
#define AIDATUPLES_AIDATupleAlgorithmWrite_H 1

// Include files
#include "AIDA/ITuple.h"
#include "GaudiKernel/Algorithm.h"

using namespace AIDA;

// Forward declarations
class AIDATupleSvc;

class AIDATupleAlgorithmWrite : public Algorithm {

public:
  // Constructor of this form must be provided
  AIDATupleAlgorithmWrite( const std::string& name, ISvcLocator* pSvcLocator );

  // Three mandatory member functions of any algorithm
  StatusCode initialize();
  StatusCode execute();
  StatusCode finalize();

private:
  AIDA::ITuple* tuple;
};

#endif // AIDATUPLES_AIDATupleAlgorithmWrite_H
