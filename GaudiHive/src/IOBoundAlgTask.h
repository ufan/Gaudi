#ifndef GAUDIHIVE_IOBOUNDALGTASK_H
#define GAUDIHIVE_IOBOUNDALGTASK_H

// Framework include files
#include "GaudiKernel/IAlgTask.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/EventContext.h"
#include "GaudiKernel/IAlgExecStateSvc.h"

/**@class IOBoundAlgTask IOBoundAlgTask.h GaudiHive/src/IOBoundAlgTask.h
 *
 *  Wrapper around I/O-bound Gaudi-algorithms. It may also cover the accelerator-targeted algorithms.
 *  It must be used to prepare algorithms before sending them to I/O-bound scheduler.
 *
 *  @author  Illya Shapoval
 *  @version 1.0
 */

class IOBoundAlgTask : public IAlgTask {
public:
  IOBoundAlgTask(IAlgorithm* algorithm,
                 unsigned int algoIndex,
                 EventContext* ctx,
                 ISvcLocator* svcLocator,
                 IAlgExecStateSvc* aem):
    m_algorithm(algorithm),
    m_evtCtx(ctx),
    m_algoIndex(algoIndex),
    m_aess(aem),
    m_serviceLocator(svcLocator) {}

    ~IOBoundAlgTask() override {}

    virtual StatusCode execute() override;

private:
  SmartIF<IAlgorithm> m_algorithm;
  EventContext* m_evtCtx;
  const unsigned int m_algoIndex;
  IAlgExecStateSvc *m_aess;
  SmartIF<ISvcLocator> m_serviceLocator;
};

#endif
