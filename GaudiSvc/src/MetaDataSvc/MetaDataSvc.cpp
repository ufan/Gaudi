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
/*
 * MetaDataSvc.cpp
 *
 *  Created on: Mar 24, 2015
 *      Author: Ana Trisovic
 */

// Framework include files
#include "GaudiKernel/IAlgManager.h"
#include "GaudiKernel/IAlgorithm.h"
#include "GaudiKernel/IJobOptionsSvc.h"
#include "GaudiKernel/IProperty.h"
#include "GaudiKernel/IService.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IToolSvc.h"

#include "MetaDataSvc.h"

using Gaudi::MetaDataSvc;

namespace {
  const auto get_name = []( const auto* i ) { return i->name(); };

  struct Identity {
    template <typename T>
    auto operator()( T&& t ) const -> decltype( auto ) {
      return std::forward<T>( t );
    }
  };

  template <typename Iterator, typename Sep, typename Projection = Identity>
  std::string join( Iterator first, Iterator last, Sep sep, Projection proj = {} ) {
    std::string s;
    if ( first != last ) s += proj( *first++ );
    for ( ; first != last; ++first ) {
      s += sep;
      s += proj( *first );
    }
    return s;
  }
  template <typename Container, typename Sep, typename Projection = Identity>
  std::string join( const Container& c, Sep sep, Projection proj = {} ) {
    return join( begin( c ), end( c ), std::move( sep ), std::move( proj ) );
  }
} // namespace

DECLARE_COMPONENT( MetaDataSvc )

StatusCode MetaDataSvc::start() {
  if ( msgLevel( MSG::DEBUG ) ) debug() << "started" << endmsg;
  return collectData();
}

std::map<std::string, std::string> MetaDataSvc::getMetaDataMap() const { return m_metadata; }

StatusCode MetaDataSvc::collectData() {

  // save options for all clients
  {
    auto joSvc = service<IJobOptionsSvc>( "JobOptionsSvc" );
    if ( !joSvc.isValid() ) return StatusCode::FAILURE;
    for ( const auto& c : joSvc->getClients() ) {
      // get options for this client
      const auto props = joSvc->getProperties( c );
      if ( props ) {
        for ( const auto prop : *props ) { m_metadata[c + "." + prop->name()] = prop->toString(); }
      }
    }
  }

  for ( const auto* name : {"ApplicationMgr", "MessageSvc", "NTupleSvc"} ) {
    auto svc = service<IProperty>( name );
    if ( !svc.isValid() ) continue;
    const auto prefix = name + std::string{"."};
    for ( const auto* prop : svc->getProperties() ) { m_metadata[prefix + prop->name()] = prop->toString(); }
  }

  /*
   * TOOLS
   * */
  SmartIF<IToolSvc> tSvc( serviceLocator()->service( "ToolSvc" ) );
  if ( tSvc.isValid() ) { m_metadata["ToolSvc"] = join( tSvc->getInstances( "" ), ", " ); }

  /*
   * SERVICES
   * */
  m_metadata["ISvcLocator.Services"] = join( serviceLocator()->getServices(), ", ", get_name );

  /*
   * ALGORITHMS
   * */
  SmartIF<IAlgManager> algMan( serviceLocator() );
  m_metadata["IAlgManager.Algorithms"] = join( algMan->getAlgorithms(), ", ", get_name );

  /*
   * JOB OPTIONS SERVICE
   * */
  {
    auto joSvc = service<IProperty>( "JobOptionsSvc" );
    if ( !joSvc.isValid() ) return StatusCode::FAILURE;
    for ( const auto* prop : joSvc->getProperties() ) {
      m_metadata["JobOptionsSvc." + prop->name()] = prop->toString();
    }
  }

  if ( msgLevel( MSG::DEBUG ) ) {
    debug() << "Metadata collected:\n";
    for ( const auto& item : m_metadata ) { debug() << item.first << ':' << item.second << '\n'; }
    debug() << endmsg;
  }

  return StatusCode::SUCCESS;
}
