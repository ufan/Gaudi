#include "FileReadTool.h"
#include <fstream>

DECLARE_COMPONENT( FileReadTool )

std::unique_ptr<std::istream> FileReadTool::open( const std::string& url ) {
  // remove the optional "file://" from the beginning of the url
  std::string path;
  if ( url.compare( 0, 7, "file://" ) == 0 ) {
    path = url.substr( 7 );
  } else {
    path = url;
  }
  return std::make_unique<std::ifstream>( path );
}

const std::vector<std::string>& FileReadTool::protocols() const {
  /// Vector of supported protocols.
  static const std::vector<std::string> s_protocols = {{"file"}};
  return s_protocols;
}
