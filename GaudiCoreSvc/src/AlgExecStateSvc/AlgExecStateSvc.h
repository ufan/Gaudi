#ifndef GAUDICORESVC_ALGEXECSTATESVC_H
#define GAUDICORESVC_ALGEXECSTATESVC_H 1

#include "GaudiKernel/IAlgExecStateSvc.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/Service.h"

#include <map>
#include <mutex>
#include <vector>

/** @class AlgExecStateSvc
  * @brief A service that keeps track of the execution state of Algorithm
  *
  */
class AlgExecStateSvc : public extends<Service, IAlgExecStateSvc>
{
public:
  using extends::extends;

  typedef IAlgExecStateSvc::AlgStateMap_t AlgStateMap_t;

  const AlgExecState& algExecState( const Gaudi::StringKey& algName, const EventContext& ctx ) const override;
  const AlgExecState& algExecState( IAlgorithm* iAlg, const EventContext& ctx ) const override;
  AlgExecState& algExecState( IAlgorithm* iAlg, const EventContext& ctx ) override;
  const AlgStateMap_t& algExecStates( const EventContext& ctx ) const override;

  void reset( const EventContext& ctx ) override;

  void addAlg( IAlgorithm* iAlg ) override;
  void addAlg( const Gaudi::StringKey& algName ) override;

  const EventStatus::Status& eventStatus( const EventContext& ctx ) const override;

  void setEventStatus( const EventStatus::Status& sc, const EventContext& ctx ) override;

  void updateEventStatus( const bool& b, const EventContext& ctx ) override;

  unsigned int algErrorCount( const IAlgorithm* iAlg ) const override;
  void resetErrorCount( const IAlgorithm* iAlg ) override;
  unsigned int incrementErrorCount( const IAlgorithm* iAlg ) override;

  void dump( std::ostringstream& ost, const EventContext& ctx ) const override;

private:
  // one vector entry per event slot
  std::vector<AlgStateMap_t> m_algStates;

  std::vector<EventStatus::Status> m_eventStatus;
  std::vector<Gaudi::StringKey>    m_preInitAlgs;

  std::map<Gaudi::StringKey, std::atomic<unsigned int>> m_errorCount;

  void           init();
  void           checkInit() const;
  std::once_flag m_initFlag;
  bool           m_isInit = false;

  std::mutex m_mut;
};

#endif
