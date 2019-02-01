#include "GaudiExamples/MyTrack.h"

using namespace Gaudi::Examples;

namespace {
  struct InstanceCount {
    long count;
    InstanceCount() : count( 0 ) {}
    ~InstanceCount() {
      if ( count ) std::cout << "Number of MyTrack instances:" << count << std::endl;
    }
  };
} // namespace
static InstanceCount s_instances;

/// Standard constructor
MyTrack::MyTrack() : m_px( 0.0 ), m_py( 0.0 ), m_pz( 0.0 ) {
  // m_event(this);
  // m_originVertex(this);
  // m_decayVertices(this);
  s_instances.count++;
}

/// Standard constructor
MyTrack::MyTrack( float x, float y, float z ) : m_px( x ), m_py( y ), m_pz( z ) {
  // m_event(this);
  // m_originVertex(this);
  // m_decayVertices(this);
  s_instances.count++;
}

MyTrack::MyTrack( const MyTrack& t ) : KeyedObject<int>( t.key() ), m_px( t.m_px ), m_py( t.m_py ), m_pz( t.m_pz ) {
  s_instances.count++;
}

/// Standard Destructor
MyTrack::~MyTrack() { s_instances.count--; }
