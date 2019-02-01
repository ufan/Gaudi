#ifndef GAUDIEXAMPLES_PARTPROPEXA_H
#define GAUDIEXAMPLES_PARTPROPEXA_H 1

#include "GaudiKernel/Algorithm.h"
#include "HepPDT/CommonParticleData.hh"
#include "HepPDT/ProcessUnknownID.hh"

class IPartPropSvc;

class PartPropExa : public Algorithm {

public:
  using Algorithm::Algorithm;
  StatusCode initialize() override;
  StatusCode execute() override;

private:
  IPartPropSvc* m_pps;
};

namespace HepPDT {
  class TestUnknownID : public ProcessUnknownID {
  public:
    TestUnknownID() = default;

    CommonParticleData* processUnknownID( ParticleID, const ParticleDataTable& pdt ) override;
  };
} // namespace HepPDT

#endif // GAUDIEXAMPLES_PARTPROPEXA_H
