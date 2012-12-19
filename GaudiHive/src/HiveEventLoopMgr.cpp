#include <algorithm>
#include <tuple>

#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/Incident.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/IAlgManager.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/IEvtSelector.h"
#include "GaudiKernel/IDataManagerSvc.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IConversionSvc.h"
#include "GaudiKernel/AppReturnCode.h"
#include "GaudiKernel/DataSvc.h"

#include "HistogramAgent.h"

// For concurrency
#include "GaudiHive/HiveEventLoopMgr.h"
#include "GaudiKernel/HiveAlgorithmManager.h"
#include "GaudiHive/EventSchedulingState.h"
#include "HiveEventRegistryEntry.h"

#include "tbb/task_scheduler_init.h"
#include "tbb/task.h"
#include "tbb/tick_count.h"

#include "GaudiKernel/EventContext.h"
#include "GaudiKernel/Algorithm.h"

#include <GaudiKernel/GaudiException.h>

#include <pthread.h> // only for the tID!


#include <sys/resource.h>
#include <sys/times.h>

// Instantiation of a static factory class used by clients to create instances of this service
DECLARE_SERVICE_FACTORY(HiveEventLoopMgr)


#define ON_DEBUG if (UNLIKELY(outputLevel() <= MSG::DEBUG))
#define ON_VERBOSE if (UNLIKELY(outputLevel() <= MSG::VERBOSE))

#define DEBMSG ON_DEBUG debug()
#define VERMSG ON_VERBOSE verbose()

/////////////////////////////////////////////////
/// *dirty* place for adding an AlgoTask wrapper
class HiveAlgoTask : public tbb::task {
public:
	HiveAlgoTask(IAlgorithm* algorithm,
			EventSchedulingState* scheduler,
			HiveEventLoopMgr* eventloopmanager): m_algorithm(algorithm),
			m_scheduler(scheduler),
			m_eventloopmanager(eventloopmanager){};
	tbb::task* execute();
	IAlgorithm* m_algorithm;
	EventSchedulingState* m_scheduler;
	HiveEventLoopMgr* m_eventloopmanager;
};

tbb::task* HiveAlgoTask::execute() {
	Algorithm* this_algo = dynamic_cast<Algorithm*>(m_algorithm);
	this_algo->getContext()->m_thread_id = pthread_self();
	m_algorithm->sysExecute();
	m_scheduler->algoFinished();
	// put back the algo into the hive algorithm manager
	m_eventloopmanager->taskFinished(m_algorithm); // TODO do this with index: put index in context?
	return NULL;
}

/////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
// Standard Constructor
//--------------------------------------------------------------------------------------------
HiveEventLoopMgr::HiveEventLoopMgr(const std::string& nam, ISvcLocator* svcLoc)
: MinimalEventLoopMgr(nam, svcLoc)
{
	m_histoDataMgrSvc   = 0;
	m_histoPersSvc      = 0;
	m_evtDataMgrSvc     = 0;
	m_evtDataSvc        = 0;
	m_evtSelector       = 0;
	m_evtContext        = 0;
	m_endEventFired     = true;
	m_total_algos_in_flight=0;
	m_max_parallel = 1;
	m_evts_parallel = 1;
	m_num_threads = 1;
	m_DumpQueues = true;

	// Declare properties
	declareProperty("HistogramPersistency", m_histPersName = "");
	declareProperty("EvtSel", m_evtsel );
	declareProperty("Warnings",m_warnings=true,
			"Set this property to false to suppress warning messages");
	declareProperty("MaxAlgosParallel", m_max_parallel );
	declareProperty("MaxEventsParallel", m_evts_parallel);
	declareProperty("NumThreads", m_num_threads);
	declareProperty("DumpQueues", m_DumpQueues);
	declareProperty("CloneAlgorithms", m_CloneAlgorithms= false);
	declareProperty("AlgosDependencies", m_AlgosDependencies);
}

//--------------------------------------------------------------------------------------------
// Standard Destructor
//--------------------------------------------------------------------------------------------
HiveEventLoopMgr::~HiveEventLoopMgr()   {
	if( m_histoDataMgrSvc ) m_histoDataMgrSvc->release();
	if( m_histoPersSvc ) m_histoPersSvc->release();
	if( m_evtDataMgrSvc ) m_evtDataMgrSvc->release();
	if( m_evtDataSvc ) m_evtDataSvc->release();
	if( m_evtSelector ) m_evtSelector->release();
	if( m_evtContext ) delete m_evtContext;
}

//--------------------------------------------------------------------------------------------
// implementation of IAppMgrUI::initialize
//--------------------------------------------------------------------------------------------
StatusCode HiveEventLoopMgr::initialize()    {
	// Initialize the base class
	StatusCode sc = MinimalEventLoopMgr::initialize();
	if( !sc.isSuccess() ) {
		DEBMSG << "Error Initializing base class MinimalEventLoopMgr." << endmsg;
		return sc;
	}

	find_dependencies();

	// Setup access to event data services
	m_evtDataMgrSvc = serviceLocator()->service("EventDataSvc");
	if( !m_evtDataMgrSvc.isValid() )  {
		fatal() << "Error retrieving EventDataSvc interface IDataManagerSvc." << endmsg;
		return StatusCode::FAILURE;
	}
	m_evtDataSvc = serviceLocator()->service("EventDataSvc");
	if( !m_evtDataSvc.isValid() )  {
		fatal() << "Error retrieving EventDataSvc interface IDataProviderSvc." << endmsg;
		return StatusCode::FAILURE;
	}
  m_whiteboard = serviceLocator()->service("EventDataSvc");
	if( !m_evtDataSvc.isValid() )  {
		fatal() << "Error retrieving EventDataSvc interface IHiveWhiteBoard." << endmsg;
		return StatusCode::FAILURE;
	}
  m_whiteboard->setNumberOfStores(m_evts_parallel).ignore();

	// Obtain the IProperty of the ApplicationMgr
	m_appMgrProperty = serviceLocator();
	if ( ! m_appMgrProperty.isValid() )   {
		fatal() << "IProperty interface not found in ApplicationMgr." << endmsg;
		return StatusCode::FAILURE;
	}

	// We do not expect a Event Selector necessarily being declared
	setProperty(m_appMgrProperty->getProperty("EvtSel")).ignore();

	if( m_evtsel != "NONE" || m_evtsel.length() == 0) {
		m_evtSelector = serviceLocator()->service("EventSelector");
		if( m_evtSelector.isValid() ) {
			// Setup Event Selector
			sc=m_evtSelector->createContext(m_evtContext);
			if( !sc.isSuccess() )   {
				fatal() << "Can not create the event selector Context." << endmsg;
				return sc;
			}
		}
		else {
			fatal() << "EventSelector not found." << endmsg;
			return sc;
		}
	}
	else {
		m_evtSelector = 0;
		m_evtContext = 0;
		if ( m_warnings ) {
			warning() << "Unable to locate service \"EventSelector\" " << endmsg;
			warning() << "No events will be processed from external input." << endmsg;
		}
	}

	// Setup access to histogramming services
	m_histoDataMgrSvc = serviceLocator()->service("HistogramDataSvc");
	if( !m_histoDataMgrSvc.isValid() )  {
		fatal() << "Error retrieving HistogramDataSvc." << endmsg;
		return sc;
	}
	// Setup histogram persistency
	m_histoPersSvc = serviceLocator()->service("HistogramPersistencySvc");
	if( !m_histoPersSvc.isValid() ) {
		warning() << "Histograms cannot not be saved - though required." << endmsg;
		return sc;
	}

	// Setup tbb task scheduler
	// TODO: shouldn't be in this case
	// One more for the current thread
	m_tbb_scheduler_init = new tbb::task_scheduler_init(m_num_threads+1);

	return StatusCode::SUCCESS;
}
//--------------------------------------------------------------------------------------------
// implementation of IService::reinitialize
//--------------------------------------------------------------------------------------------
StatusCode HiveEventLoopMgr::reinitialize() {

	// Initialize the base class
	StatusCode sc = MinimalEventLoopMgr::reinitialize();
	if( !sc.isSuccess() ) {
		DEBMSG << "Error Initializing base class MinimalEventLoopMgr." << endmsg;
		return sc;
	}

	// Check to see whether a new Event Selector has been specified
	setProperty(m_appMgrProperty->getProperty("EvtSel"));
	if( m_evtsel != "NONE" || m_evtsel.length() == 0) {
		SmartIF<IService> theSvc(serviceLocator()->service("EventSelector"));
		SmartIF<IEvtSelector> theEvtSel(theSvc);
		if( theEvtSel.isValid() && ( theEvtSel.get() != m_evtSelector.get() ) ) {
			// Setup Event Selector
			if ( m_evtSelector.get() && m_evtContext ) {
				// Need to release context before switching to new event selector
				m_evtSelector->releaseContext(m_evtContext);
				m_evtContext = 0;
			}
			m_evtSelector = theEvtSel;
			if (theSvc->FSMState() == Gaudi::StateMachine::INITIALIZED) {
				sc = theSvc->reinitialize();
				if( !sc.isSuccess() ) {
					error() << "Failure Reinitializing EventSelector "
							<< theSvc->name( ) << endmsg;
					return sc;
				}
			}
			else {
				sc = theSvc->sysInitialize();
				if( !sc.isSuccess() ) {
					error() << "Failure Initializing EventSelector "
							<< theSvc->name( ) << endmsg;
					return sc;
				}
			}
			sc = m_evtSelector->createContext(m_evtContext);
			if( !sc.isSuccess() ) {
				error() << "Can not create Context " << theSvc->name( ) << endmsg;
				return sc;
			}
			info() << "EventSelector service changed to "
					<< theSvc->name( ) << endmsg;
		}
		else if ( m_evtSelector.isValid() ) {
			if ( m_evtContext ) {
				m_evtSelector->releaseContext(m_evtContext);
				m_evtContext = 0;
			}
			sc = m_evtSelector->createContext(m_evtContext);
			if( !sc.isSuccess() ) {
				error() << "Can not create Context " << theSvc->name( ) << endmsg;
				return sc;
			}
		}
	}
	else if ( m_evtSelector.isValid() && m_evtContext ) {
		m_evtSelector->releaseContext(m_evtContext);
		m_evtSelector = 0;
		m_evtContext = 0;
	}
	return StatusCode::SUCCESS;
}


//--------------------------------------------------------------------------------------------
// implementation of IService::stop
//--------------------------------------------------------------------------------------------
StatusCode HiveEventLoopMgr::stop()    {
	if ( ! m_endEventFired ) {
		// Fire pending EndEvent incident
		m_incidentSvc->fireIncident(Incident(name(),IncidentType::EndEvent));
		m_endEventFired = true;
	}
	return MinimalEventLoopMgr::stop();
}

//--------------------------------------------------------------------------------------------
// implementation of IAppMgrUI::finalize
//--------------------------------------------------------------------------------------------
StatusCode HiveEventLoopMgr::finalize()    {
	StatusCode sc;

	// Finalize base class
	sc = MinimalEventLoopMgr::finalize();
	if (! sc.isSuccess()) {
		error() << "Error finalizing base class" << endmsg;
		return sc;
	}

	// Save Histograms Now
	if ( m_histoPersSvc != 0 )    {
		HistogramAgent agent;
		sc = m_histoDataMgrSvc->traverseTree( &agent );
		if( sc.isSuccess() )   {
			IDataSelector* objects = agent.selectedObjects();
			// skip /stat entry!
			if ( objects->size() > 0 )    {
				IDataSelector::iterator i;
				for ( i = objects->begin(); i != objects->end(); i++ )    {
					IOpaqueAddress* pAddr = 0;
					StatusCode iret = m_histoPersSvc->createRep(*i, pAddr);
					if ( iret.isSuccess() )     {
						(*i)->registry()->setAddress(pAddr);
					}
					else  {
						sc = iret;
					}
				}
				for ( i = objects->begin(); i != objects->end(); i++ )    {
					IRegistry* reg = (*i)->registry();
					StatusCode iret = m_histoPersSvc->fillRepRefs(reg->address(), *i);
					if ( !iret.isSuccess() )    {
						sc = iret;
					}
				}
			}
			if ( sc.isSuccess() )    {
				info() << "Histograms converted successfully according to request." << endmsg;
			}
			else  {
				error() << "Error while saving Histograms." << endmsg;
			}
		}
		else {
			error() << "Error while traversing Histogram data store" << endmsg;
		}
	}

	// Release event selector context
	if ( m_evtSelector && m_evtContext )   {
		m_evtSelector->releaseContext(m_evtContext).ignore();
		m_evtContext = 0;
	}

	// Release all interfaces...
	m_histoDataMgrSvc = 0;
	m_histoPersSvc    = 0;

	m_evtSelector     = 0;
	m_evtDataSvc      = 0;
	m_evtDataMgrSvc   = 0;

	delete m_tbb_scheduler_init;

	return StatusCode::SUCCESS;
}

//--------------------------------------------------------------------------------------------
// implementation of executeEvent(void* par)
//--------------------------------------------------------------------------------------------
StatusCode HiveEventLoopMgr::executeEvent(void* /*par*/)    {

	// Fire BeginEvent "Incident"
	m_incidentSvc->fireIncident(Incident(name(),IncidentType::BeginEvent));
	// An incident may schedule a stop, in which case is better to exit before the actual execution.
	if ( m_scheduledStop ) {
		always() << "Terminating event processing loop due to a stop scheduled by an incident listener" << endmsg;
		return StatusCode::SUCCESS;
	}

	// Execute Algorithms
	m_incidentSvc->fireIncident(Incident(name(), IncidentType::BeginProcessing));

	// Prepare the event context for concurrency


	// Call the resetExecuted() method of ALL "known" algorithms
	// (before we were reseting only the topalgs)
	SmartIF<IAlgManager> algMan(serviceLocator());
	if (LIKELY(algMan.isValid())) {
		const ListAlgPtrs& allAlgs = algMan->getAlgorithms() ;
		for( ListAlgPtrs::const_iterator ialg = allAlgs.begin() ; allAlgs.end() != ialg ; ++ialg ) {
			if (LIKELY(0 != *ialg)) (*ialg)->resetExecuted();
		}
	}

	// Some debug
	if(m_DumpQueues){
		HiveAlgorithmManager* hivealgman = dynamic_cast<HiveAlgorithmManager*> (algMan.get());
		if (hivealgman) hivealgman->dump();
	}

	bool eventfailed = false;//run_parallel();

	// ensure that the abortEvent flag is cleared before the next event
	if (UNLIKELY(m_abortEvent)) {
		DEBMSG << "AbortEvent incident fired by " << m_abortEventSource << endmsg;
		m_abortEvent = false;
	}

	// Call the execute() method of all output streams
	for (ListAlg::iterator ito = m_outStreamList.begin(); ito != m_outStreamList.end(); ito++ ) {
		(*ito)->resetExecuted();
		StatusCode sc;
		sc = (*ito)->sysExecute();
		if (UNLIKELY(!sc.isSuccess())) {
			warning() << "Execution of output stream " << (*ito)->name() << " failed" << endmsg;
			eventfailed = true;
		}
	}

	m_incidentSvc->fireIncident(Incident(name(), IncidentType::EndProcessing));

	// Check if there was an error processing current event
	if (UNLIKELY(eventfailed)){
		error() << "Error processing event loop." << endmsg;
		return StatusCode(StatusCode::FAILURE,true);
	}
	return StatusCode(StatusCode::SUCCESS,true);

}

//--------------------------------------------------------------------------------------------
// implementation of IEventProcessing::executeRun
//--------------------------------------------------------------------------------------------
StatusCode HiveEventLoopMgr::executeRun( int maxevt )    {
	StatusCode  sc;
	// initialize the base class
	sc = MinimalEventLoopMgr::executeRun(maxevt);
	return sc;
}

//--------------------------------------------------------------------------------------------
// implementation of IAppMgrUI::nextEvent
//--------------------------------------------------------------------------------------------
// Here the loop on the events takes place.
// This is also the natural place to put the preparation of the algorithms
// contexts, which contain the event specific data.

StatusCode HiveEventLoopMgr::nextEvent(int maxevt)   {
	// Collapse executeEvent and run_parallel in the same method
	// TODO _very_ sporty on conditions and checks!!

	auto start_time = tbb::tick_count::now();
	auto secsFromStart = [&start_time]()->double{
		return (tbb::tick_count::now()-start_time).seconds();
	};

	typedef std::tuple<EventContext*,EventSchedulingState*> contextSchedState_tuple;
	typedef DataSvcHelpers::RegistryEntry regEntry;

	MsgStream log(msgSvc(), name());


	// Reset the application return code.
	Gaudi::setAppReturnCode(m_appMgrProperty, Gaudi::ReturnCode::Success, true).ignore();

	// Lambda to check if an event has finished
	auto has_finished = [] // acquire nothing
	                     (contextSchedState_tuple evtContext_evtstate) // argument is a tuple
	                     { return std::get<1>(evtContext_evtstate)->hasFinished();}; // true if finished

	// Useful for the Logs
	always() << "Running with "
			<< m_evts_parallel << " parallel events, "
			<< m_max_parallel << " max concurrent algorithms, "
			<< m_num_threads << " threads."
			<< endmsg;

	int n_processed_events = 0;
  StatusCode sc;
  
	// Create the root event into which we put all events
	//*PM* StatusCode sc = m_evtDataMgrSvc->setRoot ("/Event", new DataObject());
	//*PM* if( !sc.isSuccess() )  {
	//*PM* 	warning() << "Error declaring event root DataObject" << endmsg;
	//*PM* }

	// Get the algorithm Manager
	SmartIF<IAlgManager> algMan(serviceLocator());
	HiveAlgorithmManager* hivealgman = dynamic_cast<HiveAlgorithmManager*> (algMan.get());

	// Retrieve the event (even outside the loop?)
	//*PM* DataObject*       pObject = 0;
	//*PM* m_evtDataSvc->retrieveObject("/Event",pObject);
	//*PM* regEntry* rootRegistry = dynamic_cast<regEntry*>(pObject->registry()); //TODO: an interface in the evtDataSvc for it would come handy

	// Events in flight
	std::list<contextSchedState_tuple> events_in_flight;

	// Loop until no more evts are there
	while (maxevt == -1 ? true : n_processed_events<maxevt){// TODO Fix the condition in case of -1

		const unsigned int n_events_in_flight = events_in_flight.size();

		const unsigned int n_evts_to_process = maxevt - n_processed_events - n_events_in_flight;


		unsigned int n_acquirable_events = m_evts_parallel - n_events_in_flight ;
		if (n_acquirable_events > n_evts_to_process)
			n_acquirable_events = n_evts_to_process;

		log << MSG::INFO << "Evts in flight: " <<  n_events_in_flight << endmsg;
		log << MSG::INFO << "Evts processed: " <<  n_processed_events<< endmsg;
		log << MSG::INFO << "Evts parallel: " << m_evts_parallel << endmsg;
		log << MSG::INFO << "Acquirable Events are " << n_acquirable_events << endmsg;

		// Initialisation section ------------------------------------------------

		// Loop on events to be initialised
		for (unsigned int offset=0; offset< n_acquirable_events; ++offset){

			EventContext* evtContext(new EventContext);
			const int evt_num =  n_processed_events + offset + n_events_in_flight;
			evtContext->m_evt_num = evt_num;

			//*PM* std::string evtname ( "Evt_" + std::to_string(evt_num) );
			//*PM* Hive::HiveEventRegistryEntry* evt_registry = new Hive::HiveEventRegistryEntry(evtname,rootRegistry);
			//*PM* rootRegistry->add(evt_registry);
			//*PM* evtContext->m_registry = evt_registry;

      evtContext->m_evt_slot = m_whiteboard->allocateStore(evt_num);
      m_whiteboard->selectStore(evtContext->m_evt_slot).ignore();
      sc = m_evtDataMgrSvc->setRoot ("/Event", new DataObject());
      if( !sc.isSuccess() )  {
       	warning() << "Error declaring event root DataObject" << endmsg;
      }

			EventSchedulingState* event_state = new EventSchedulingState(m_topAlgList.size());
			events_in_flight.push_back(std::make_tuple(evtContext,event_state));

			always()  << "Started event " << evt_num << " at " << secsFromStart() << endmsg;

		}// End initialisation loop on acquired events

		// End initialisation session --------------------------------------------

		// Scheduling session ----------------------------------------------------
		auto in_flight_end = events_in_flight.end();
		auto in_flight_begin = events_in_flight.begin();
		// loop until at least one evt finished
		while (in_flight_end == find_if(in_flight_begin, in_flight_end ,has_finished)){
			bool no_algo_can_run = true;
			for (auto& evtContext_evtstate : events_in_flight){ // loop on evts

				EventContext* event_Context = std::get<0>(evtContext_evtstate);
				EventSchedulingState* event_state = std::get<1>(evtContext_evtstate);

				for (unsigned int algo_counter=0; algo_counter<m_topAlgList.size(); algo_counter++) { // loop on algos
					// check whether all requirements/dependencies for the algorithm are fulfilled...
					const state_type& algo_requirements = m_all_requirements[algo_counter];
					state_type dependencies_missing = (event_state->state() & algo_requirements) ^ algo_requirements;

					// ...and whether the algorithm was already started and if it can be started
					if ( (dependencies_missing == 0) &&
							(event_state->hasStarted(algo_counter) ) == false &&
							(m_total_algos_in_flight < m_max_parallel )) {

						no_algo_can_run = false;

						// Pick the algorithm if available and if not and requested create one
						IAlgorithm* ialgo=NULL;
						if(hivealgman->acquireAlgorithm(algo_counter,ialgo,m_CloneAlgorithms)){
							log << MSG::INFO << "Launching algo " << algo_counter<< endmsg;
							// Attach context to the algo
							Algorithm* algo = dynamic_cast<Algorithm*> (ialgo);
							algo->setContext(event_Context);

							tbb::task* t = new( tbb::task::allocate_root() ) HiveAlgoTask(ialgo, event_state, this);
							tbb::task::enqueue( *t);

							event_state->algoStarts(algo_counter);
							++m_total_algos_in_flight;

							log << MSG::INFO << "Algos in flight: " <<  m_total_algos_in_flight << endmsg;
						}
					} // End scheduling if block

				}// end loop on algo indices

				// Check whether we are in a stall situation
				if (no_algo_can_run && // nothing can run
						m_total_algos_in_flight==0 && // nothing is running
						//events_in_flight.size() != 0 && // at least one event is in flight
						in_flight_end == find_if(in_flight_begin, in_flight_end ,has_finished)){ // none finished

					std::cout << "Going to check in detail\n";
					/* Additional condition since the test above is not enough since not atomic!!
					Indeed it could happen that:
					o No algo could run
					o At least one evt is in flight
					o One algo finished, the m_total_algos_in_flight is now 0, but some input is available!
					It would be much better to use an atomic condition, maybe via a transaction.
					 */
					bool stalled=true;
					for (auto& event :events_in_flight){
						EventSchedulingState* this_event_state = std::get<1>(event);
						unsigned int algo_counter=0;
						for (auto& algo_requirements:m_all_requirements ) { // loop on algos
							// check whether all requirements/dependencies for the algorithm are fulfilled and it did not run already...
							state_type dependencies_missing = (this_event_state->state() & algo_requirements) ^ algo_requirements;
							if (dependencies_missing == 0 and not this_event_state->hasStarted(algo_counter)){
								// Some algo can still be scheduled.
								stalled=false;
								break;
							}
						}
					}
					if (stalled){
						std::string error("No algorithm is in flight and no algorithm can be scheduled on the events being processed.");
						fatal() << error <<std::endl;
						for (auto& event : events_in_flight){
							// Show what ran
							unsigned int algo_counter=0;
							EventContext* this_event_Context = std::get<0>(event);
							EventSchedulingState* this_event_state = std::get<1>(event);
							fatal() << "Algorithms that ran for event " << this_event_Context->m_evt_num << std::endl;

							for (auto& algo : m_topAlgList){
								bool has_started = this_event_state->hasStarted(algo_counter);
								if (has_started)
									fatal() << " o " << algo->name() << " could run" << std::endl;
								else
									fatal() << " o " << algo->name() << " could NOT run" << std::endl;
								algo_counter++;
							} // End of internal loop on algos

						} // End Internal Loop on Events in flight
						fatal() << endmsg;


						throw GaudiException (error,"Hive",false);
					} // End of block executed at stall
				} // End stall check

				// update the event state with what has been put into the DataSvc
				//*PM* bool queue_full(false);
				//*PM* std::string product_name;

				//*PM* Hive::HiveEventRegistryEntry* hiveregistryentry= dynamic_cast<Hive::HiveEventRegistryEntry*>(event_Context->m_registry);
				//*PM* tbb::concurrent_queue<std::string>& new_products = hiveregistryentry->new_products();
				//*PM* do {
				//*PM* 	queue_full = new_products.try_pop(product_name);
				//*PM* 	if (queue_full && m_product_indices.count( product_name ) == 1) { // only products with dependencies upon need to be announced to other algos
				//*PM* 		event_state->update_state(m_product_indices[product_name]);
				//*PM* 	}
				//*PM* } while (queue_full);
        std::vector<std::string> new_products;
        m_whiteboard->selectStore(event_Context->m_evt_slot).ignore();
        sc = m_whiteboard->getNewDataObjects(new_products);
        if( !sc.isSuccess() )  {
          warning() << "Error getting recent new products (since last time called)" << endmsg;
        }
        for (std::vector<std::string>::iterator i = new_products.begin(); i != new_products.end(); i++) {
          if (m_product_indices.count( *i ) == 1) { // only products with dependencies upon need to be announced to other algos
            event_state->update_state(m_product_indices[*i]);
          }
        }
			}// end loop on evts in flight
		}// end loop until at least one evt in flight finished

		// Remove from the in flight events the finished ones
		std::list<contextSchedState_tuple>::iterator it=events_in_flight.begin();

		while (it!=events_in_flight.end()){
			// Now proceed to deletion
			if (std::get<1>(*it)->hasFinished()){
				const unsigned int evt_num = std::get<0>(*it)->m_evt_num;
        const unsigned int evt_slot = std::get<0>(*it)->m_evt_slot;
				log << MSG::INFO << "Event "<< evt_num << " finished. Events in fight are "
						<< events_in_flight.size() << ". Processed events are "
						<<  n_processed_events << endmsg;
				always() << "Event "<< evt_num << " finished. now is " <<  secsFromStart() << endmsg;

				// Calculate min and max event num
				unsigned int min_event_num=0xFFFFFFFF;
				unsigned int max_event_num=0;

				for (auto& evtContext_evtstate : events_in_flight){
					const unsigned int evt_num = std::get<0>(evtContext_evtstate)->m_evt_num;
					// Update min and max for backlog calculation
					if (evt_num > max_event_num) max_event_num=evt_num;
					if (evt_num < min_event_num) min_event_num=evt_num;
				}
				unsigned int evt_backlog=max_event_num-min_event_num;
				always() << "Event backlog (max= " << max_event_num << ", min= "
						<< min_event_num<<" ) = " << evt_backlog << endmsg;
        
        sc = m_whiteboard->clearStore(evt_slot);
        if( !sc.isSuccess() )  {
          warning() << "Clear of Event data store failed" << endmsg;
        }
        else {
          info() << "Cleared store " << evt_slot << endmsg;
        }
        m_whiteboard->freeStore(evt_slot).ignore();

				delete std::get<0>(*it);
				delete std::get<1>(*it);
				it=events_in_flight.erase(it) ;

				n_processed_events++;

				if(m_DumpQueues && hivealgman) hivealgman->dump();
			} else{
				++it;
			}
		}
		// End scheduling session ------------------------------------------------

	} // End while loop on events


	always() << "---> Loop Finished (seconds): " << secsFromStart() <<endmsg;

	if(m_DumpQueues && hivealgman) hivealgman->dump();

	return StatusCode::SUCCESS;

}

/// Create event address using event selector
StatusCode HiveEventLoopMgr::getEventRoot(IOpaqueAddress*& refpAddr)  {
	refpAddr = 0;
	StatusCode sc = m_evtSelector->next(*m_evtContext);
	if ( !sc.isSuccess() )  {
		return sc;
	}
	// Create root address and assign address to data service
	sc = m_evtSelector->createAddress(*m_evtContext,refpAddr);
	if( !sc.isSuccess() )  {
		sc = m_evtSelector->next(*m_evtContext);
		if ( sc.isSuccess() )  {
			sc = m_evtSelector->createAddress(*m_evtContext,refpAddr);
			if ( !sc.isSuccess() )  {
				warning() << "Error creating IOpaqueAddress." << endmsg;
			}
		}
	}
	return sc;
}


// Here because temporary
#include <iostream>


/// Compute dependencies between the algorithms
void
HiveEventLoopMgr::find_dependencies() {

	//	/**
	//	 * This is not very simple, but here you have the reasons:
	//	 * o We input the inputs and outputs as "vectors" in the config as PROPERTIES
	//	 * o The modules store this as vector of strings
	//	 * o We need to massage them:(
	//	 * We opt for testing the scheduler and then properly change the interfaces.
	//	 */
	//	auto tokenize_gaudi_string_vector =
	//			[] (std::string s) -> const std::vector<std::string> {
	//		for (const char c: {'\'',']','['})
	//			replace(s.begin(), s.end(), c, ' ');
	//		// remove spaces
	//		s.erase(remove_if(s.begin(), s.end(), isspace), s.end());
	//		// replace commas with spaces
	//		replace(s.begin(), s.end(), ',', ' ');
	//		// tokenize
	//		std::vector<std::string> tokens;
	//		std::stringstream os(s);
	//		std::string tmp;
	//		while( os >> tmp )
	//			tokens.push_back(tmp);
	//		return tokens;
	//	};
	//
	//	auto get_algo_collections =
	//			[tokenize_gaudi_string_vector] (IAlgorithm* algo, const std::string & type) -> const std::vector<std::string> {
	//		// This is how you can get the properties from the Ialgo and not the algo!
	//		SmartIF<IProperty> algo_properties(algo);
	//		return tokenize_gaudi_string_vector (algo_properties->getProperty(type).toString());
	//	};
	//
	//	// the lambdas above will disappear -----------------
	//
	//

	//	// to be replaced!!
	//	// Let's loop through all algos and their required inputs
	//	unsigned int algo_counter(0);
	//	unsigned int input_counter(0);
	//	for (IAlgorithm* algo: m_topAlgList) {
	//		const std::vector<std::string>& inputs = get_algo_collections(algo,"Inputs");
	//		state_type requirements(0);
	//		for (const std::string& input: inputs){
	//			std::pair<std::map<std::string,unsigned int>::iterator,bool> ret;
	//			ret = m_product_indices.insert(std::pair<std::string, unsigned int>("/Event/"+input,input_counter));
	//			// insert successful means == wasn't known before. So increment counter
	//			if (ret.second==true) {
	//				++input_counter;
	//			};
	//			// in any case the return value holds the proper product index
	//			requirements[ret.first->second] = true;
	//		}
	//		all_requirements[algo_counter] = requirements;
	//		++algo_counter;
	//	}
	//	m_numberOfAlgos = algo_counter;
	//	m_all_requirements = all_requirements;

	const unsigned int n_algos = m_topAlgList.size();
	std::vector<state_type> all_requirements(n_algos);

	unsigned int algo_counter=0;
	unsigned int input_counter=0;
	// loop on the dependencies
	for (const auto& algoDependencies : m_AlgosDependencies){ // loop on algo dependencies lists
		state_type requirements(0);

		for (const auto& dependency : algoDependencies){ // loop on dependencies
			auto ret = m_product_indices.insert(std::pair<std::string, unsigned int>("/Event/"+dependency,input_counter));
			// insert successful means == wasn't known before. So increment counter
			if (ret.second==true) ++input_counter;
			// in any case the return value holds the proper product index
			requirements[ret.first->second] = true;
		}// end loop on single dependencies

		all_requirements[algo_counter] = requirements;
		++algo_counter;
	} // end loop on algo dependencies lists

	m_numberOfAlgos = algo_counter;
	m_all_requirements = all_requirements;


}

//------------------------------------------------------------------------------

void HiveEventLoopMgr::taskFinished(IAlgorithm*& algo){
	SmartIF<IAlgManager> algMan(serviceLocator());
	HiveAlgorithmManager* hivealgman = dynamic_cast<HiveAlgorithmManager*> (algMan.get());

	hivealgman->releaseAlgorithm(algo->name(),algo);

	--m_total_algos_in_flight;

	MsgStream log(msgSvc(), name());
	log << MSG::DEBUG << "[taskFinished] Algos in flight: " <<  m_total_algos_in_flight << endmsg;
}
