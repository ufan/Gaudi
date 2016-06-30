#ifndef GAUDISVC_MTEVENTLOOPMGR_H
#define GAUDISVC_MTEVENTLOOPMGR_H 1

// Framework include files
#include "GaudiKernel/IEvtSelector.h"
#include "GaudiKernel/MinimalEventLoopMgr.h"

// Forward declarations
class IIncidentSvc;
class IDataManagerSvc;
class IDataProviderSvc;

/** Class definition of MTEventLoopMgr.
    This is the default processing manager of the application manager.
    This object handles the minimal requirements needed by the
    application manager. It also handles the default user
    configuration setup for standard event processing.

    <UL>
    <LI> handling of the incidents when starting event processing
    <LI> handling of the event loop
    </UL>

    <B>History:</B>
    <PRE>
    +---------+----------------------------------------------+---------+
    |    Date |                 Comment                      | Who     |
    +---------+----------------------------------------------+---------+
    |13/12/00 | Initial version                              | M.Frank |
    +---------+----------------------------------------------+---------+
    </PRE>
   @author Markus Frank
   @version 1.0
*/
class MTEventLoopMgr : public MinimalEventLoopMgr   {
protected:
  StringProperty  m_histPersName {this, "HistogramPersistency",  {},  "Name of the Hist Pers type"};
  StringProperty  m_evtsel {this, "EvtSel",  {},  "Event selector"};

  /// Reference to the indicent service
  IIncidentSvc*     m_incidentSvc = nullptr;
  /// Reference to the Event Data Service's IDataManagerSvc interface
  IDataManagerSvc*  m_evtDataMgrSvc = nullptr;
  /// Reference to the Event Data Service's IDataProviderSvc interface
  IDataProviderSvc* m_evtDataSvc = nullptr;
  /// Reference to the Event Selector
  IEvtSelector*     m_evtSelector = nullptr;
  /// Event Iterator
  IEvtSelector::Context* m_evtCtxt = nullptr;
  /// Reference to the Histogram Data Service
  IDataManagerSvc*  m_histoDataMgrSvc = nullptr;
  /// Reference to the Histogram Persistency Service
  IConversionSvc*   m_histoPersSvc = nullptr;
  /// Property interface of ApplicationMgr
  IProperty*        m_appMgrProperty = nullptr;


public:
  /// Standard Constructor
  using MinimalEventLoopMgr::MinimalEventLoopMgr;
  /// Standard Destructor
  virtual ~MTEventLoopMgr();
  /// Create event address using event selector
  StatusCode getEventRoot(IOpaqueAddress*& refpAddr);

  /// implementation of IService::initalize
  virtual StatusCode initialize();
  /// implementation of IService::reinitalize
  virtual StatusCode reinitialize();
  /// implementation of IService::finalize
  virtual StatusCode finalize();
  /// implementation of IService::nextEvent
  virtual StatusCode nextEvent(int maxevt);
  /// implementation of IEventProcessor::executeEvent(void* par)
  //  virtual StatusCode executeEvent(void* par);
  /// implementation of IEventProcessor::executeRun()
  //  virtual StatusCode executeRun(int maxevt);

private:
  int m_total_nevt  = 0;
};
#endif // GAUDISVC_MTEVENTLOOPMGR_H
