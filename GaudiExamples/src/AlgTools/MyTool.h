#ifndef GAUDIEXANMPLES_MYTOOL_H
#define GAUDIEXANMPLES_MYTOOL_H 1

// Include files
#include "GaudiKernel/AlgTool.h"
#include "IMyTool.h"

/** @class MyTool MyTool.h
 *  This is an interface class for a example tool
 *
 *  @author Pere Mato
 *  @date   14/10/2001
 */
class MyTool : public extends<AlgTool, IMyTool>
{
public:
  /// Standard Constructor
  using extends::extends;

  /// IMyTool interface
  virtual const std::string& message() const;
  virtual void doIt();
  /// Overriding initialize and finalize
  virtual StatusCode initialize();
  virtual StatusCode finalize();

protected:
  /// Standard destructor
  virtual ~MyTool();

private:
  /// Properties
  IntegerProperty m_int{this, "Int", 100};
  DoubleProperty m_double{this, "Double", 100.};
  StringProperty m_string{this, "String", "hundred"};
  BooleanProperty m_bool{this, "Bool", true};
};
#endif // GAUDIEXANMPLES_MYTOOL_H
