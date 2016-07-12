// ============================================================================
// Local:
// ============================================================================
#include "JobOptionsSvc.h"
#include "Analyzer.h"
#include "Messages.h"
#include "Catalog.h"
#include "Units.h"
#include "PragmaOptions.h"
#include "Node.h"
#include "PythonConfig.h"
// ============================================================================
// Gaudi:
// ============================================================================
#include "GaudiKernel/IProperty.h"
#include "GaudiKernel/Property.h"

#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/System.h"

#include <set>
// ============================================================================
DECLARE_COMPONENT(JobOptionsSvc)
// ============================================================================
// Namespace aliases:
// ============================================================================
namespace gp = Gaudi::Parsers;
// ============================================================================
JobOptionsSvc::JobOptionsSvc(const std::string& name,ISvcLocator* svc):
  base_class(name,svc)
{
  if ( System::isEnvSet("JOBOPTSEARCHPATH") )
    m_dir_search_path = System::getEnv( "JOBOPTSEARCHPATH" );
  if ( System::isEnvSet("JOBOPTSDUMPFILE") )
    m_dump = System::getEnv( "JOBOPTSDUMPFILE" );
}
// ============================================================================
StatusCode JobOptionsSvc::initialize()
{
  // Call base class initializer
  StatusCode sc = Service::initialize();
  // Read the job options if needed
  if (sc) {
    if (m_source_type == "NONE") {
      return sc;
    } else if (m_source_type == "PYTHON") {
      PythonConfig conf(this);
      return conf.evaluateConfig(m_source_path, m_pythonParams, m_pythonAction);
    } else {
      return readOptions( m_source_path , m_dir_search_path);
    }
  }
  return sc;
}

// ============================================================================
StatusCode JobOptionsSvc::addPropertyToCatalogue( const std::string& client, const Property& property )
{
  // helper class to compare pointers to string using the strings
  struct PtrCmp {
    bool operator()( const std::unique_ptr<std::string>& a, const std::unique_ptr<std::string>& b ) const {
      return *a < *b;
    }
  };
  // storage for property names
  static std::set<std::unique_ptr<std::string>, PtrCmp> all_names;
  // reference to the name string, used to initialize Property name (boost::string_ref)
  const std::string& name =
      **( all_names.insert( std::unique_ptr<std::string>{new std::string{property.name()}} ).first );
  std::unique_ptr<Property> p{new StringProperty( name, "" )};
  return property.load( *p ) ? m_svc_catalog.addProperty( client, p.release() ) : StatusCode::FAILURE;
}
// ============================================================================
StatusCode
JobOptionsSvc::removePropertyFromCatalogue
( const std::string& client,
  const std::string& name )
{
  return m_svc_catalog.removeProperty(client,name);
}
// ============================================================================
const JobOptionsSvc::PropertiesT*
JobOptionsSvc::getProperties( const std::string& client) const
{
  return m_svc_catalog.getProperties(client);
}
// ============================================================================
StatusCode JobOptionsSvc::setMyProperties( const std::string& client,
                                           IProperty* myInt )
{
  const auto* props = m_svc_catalog.getProperties(client);
  if ( !props ){ return StatusCode::SUCCESS; }

  bool fail = false;
  for ( const auto& cur : *props )
  {
    StatusCode sc = myInt->setProperty ( *cur ) ;
    if ( sc.isFailure() )
    {
      error()
        << "Unable to set the property '" << cur->name() << "'"
        <<                        " of '" << client         << "'. "
        << "Check option and algorithm names, type and bounds."
        << endmsg;
      fail = true;
    }
  }
  return fail ? StatusCode::FAILURE : StatusCode::SUCCESS ;
}

/// Get the list of clients
std::vector<std::string> JobOptionsSvc::getClients() const {
  return m_svc_catalog.getClients();
}


void JobOptionsSvc::dump (const std::string& file,
                          const gp::Catalog& catalog) const {
  std::ofstream out( file , std::ios_base::out | std::ios_base::trunc );
  if ( !out ) {
    error() << "Unable to open dump-file \""+file+"\"" << endmsg ;
    return ;                                                   // RETURN
  }
  info() << "Properties are dumped into \""+file+"\"" << endmsg ;
  // perform the actual dump:
  out << catalog;
}

void JobOptionsSvc::fillServiceCatalog(const gp::Catalog& catalog) {
  for (const auto&  client : catalog) {
    for (const auto& current : client.second ) {
      addPropertyToCatalogue ( client.first ,
                               StringProperty{ current.NameInClient(),
                                               current.ValueAsString() } );
    }
  }
}

StatusCode JobOptionsSvc::readOptions ( const std::string& file,
                                        const std::string& path) {
    std::string search_path = path;
    if ( search_path.empty() && !m_dir_search_path.empty() )
    { search_path =  m_dir_search_path ; }
    //
    if (msgLevel(MSG::DEBUG))
      debug() << "Reading options from the file "
              << "'" << file << "'" << endmsg;
    gp::Messages messages(msgStream());
    gp::Catalog catalog;
    gp::Units units;
    gp::PragmaOptions pragma;
    gp::Node ast;
    StatusCode sc = gp::ReadOptions(file, path, &messages, &catalog, &units,
                                    &pragma, &ast);

    // --------------------------------------------------------------------------
    if ( sc.isSuccess() )
    {
      if (pragma.IsPrintOptions()) {
        info() << "Print options" << std::endl << catalog
               << endmsg;
      }
      if (pragma.IsPrintTree()) {
        info() << "Print tree:" << std::endl << ast.ToString()
               << endmsg;
      }
      if (pragma.HasDumpFile()) dump(pragma.dumpFile(), catalog);
      info() << "Job options successfully read in from " << file << endmsg;
      fillServiceCatalog(catalog);
    } else {
      fatal() << "Job options errors."<< endmsg;
    }
    // ----------------------------------------------------------------------------
    return sc;
}
