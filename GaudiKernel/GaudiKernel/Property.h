#ifndef GAUDIKERNEL_PROPERTY_H
#define GAUDIKERNEL_PROPERTY_H
// ============================================================================
// STD & STL
// ============================================================================
#include <string>
#include <stdexcept>
#include <typeinfo>
// ============================================================================
// Application C++ Class Headers
// ============================================================================
#include "GaudiKernel/Kernel.h"
#include "GaudiKernel/PropertyVerifier.h"
#include "GaudiKernel/Parsers.h"
#include "GaudiKernel/ToStream.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/IProperty.h"
// ============================================================================

// ============================================================================
class Property   ;
class PropertyMgr;
// ============================================================================

// ============================================================================
/// The output operator for friendly printout
// ============================================================================
GAUDI_API std::ostream& operator<<(std::ostream& stream, const Property& prop);
// ============================================================================
/** @class Property Property.h GaudiKernel/Property.h
 *
 *  Property base class allowing Property* collections to be "homogeneous"
 *
 * @author Paul Maley
 * @author CTDay
 * @author Vanya BELYAEV ibelyaev@physics.syr.edu
 * @author Marco Clemencic
 */
class GAUDI_API Property
{
private:
  // the default constructor is disabled
  Property() = delete;
public:
  /// property name
  const std::string&    name      () const { return m_name             ; }
  /// property documentation
  const std::string&    documentation() const { return m_documentation; }
  /// property type-info
  const std::type_info* type_info () const { return m_typeinfo         ; }
  /// property type
  std::string           type      () const { return m_typeinfo->name() ; }
  ///  export the property value to the destination
  virtual bool load   (       Property& dest   ) const = 0 ;
  /// import the property value form the source
  virtual bool assign ( const Property& source )       = 0 ;
public:
  /// value  -> string
  virtual std::string  toString   ()  const = 0 ;
  /// value  -> stream
  virtual void toStream(std::ostream& out) const = 0;
  /// string -> value
  virtual StatusCode   fromString ( const std::string& value ) = 0 ;
public:
  /// get a reference to the readCallBack
  const std::function<void(Property&)>&  readCallBack() const { return m_readCallBack; }
  /// get a reference to the updateCallBack
  const std::function<void(Property&)>& updateCallBack() const { return m_updateCallBack; }

  /// set new callback for reading
  virtual Property&  declareReadHandler   ( std::function<void(Property&)> fun ) ;
  /// set new callback for update
  virtual Property&  declareUpdateHandler ( std::function<void(Property&)> fun ) ;

  template< class HT >
  inline Property& declareReadHandler( void ( HT::* MF ) ( Property& ) , HT* instance )
  { return declareReadHandler( [=](Property& p) { (instance->*MF)(p); } ) ; }

  template< class HT >
  inline Property& declareUpdateHandler( void ( HT::* MF ) ( Property& ) , HT* instance )
  { return declareUpdateHandler ( [=](Property& p) { (instance->*MF)(p); } ); }

  /// use the call-back function at reading
  virtual void useReadHandler   () const ;
  /// use the call-back function at update
  virtual bool useUpdateHandler ()       ;
public:
  /// virtual destructor
  virtual ~Property() = default;
  /// clone: "virtual constructor"
  virtual Property*          clone     () const = 0 ;
  /// set the new value for the property name
  void setName ( std::string value ) { m_name = std::move(value) ; }
  /// set the documentation string
  void setDocumentation( std::string documentation ) {
    m_documentation = std::move(documentation); }
  /// the printout of the property value
  virtual std::ostream& fillStream ( std::ostream& ) const ;
protected:
  /// constructor from the property name and the type
  Property
  ( const std::type_info& type      ,
    std::string    name = "" ) ;
  /// constructor from the property name and the type
  Property
  ( std::string    name      ,
    const std::type_info& type      ) ;
  /// copy constructor
  Property           ( const Property& ) = default;
  /// assignment operator
  Property& operator=( const Property& ) = default;
private:
  // property name
  std::string              m_name           ;
  // property doc string
  std::string              m_documentation;
  // property type
  const std::type_info*    m_typeinfo       ;
protected:
  // Declare this property to a PropertyMgr instance.
  void declareTo(PropertyMgr* owner);
  // call back functor for reading
  mutable std::function<void(Property&)> m_readCallBack;
  // call back functor for update
  std::function<void(Property&)> m_updateCallBack;
};
// ============================================================================
/** @class PropertyWithValue
 *  Helper intermediate class which
 *  represent partly implemented property
 *  with value of concrete type
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 *  @date 2006-02-27
 */
// ============================================================================
template <class TYPE>
class PropertyWithValue : public Property
{
public:
  // ==========================================================================
  /// Hosted type
  using ValueType = TYPE;
  /// the type-traits for properties
  using Traits = Gaudi::Utils::PropertyTypeTraits<ValueType>;
  /// the actual storage type
  using PVal = typename Traits::PVal;
  // ==========================================================================
protected:
  // ==========================================================================
  /// the constructor with property name and value
  inline PropertyWithValue
  ( std::string        name  ,
    PVal               value ,
    const bool         owner )
    : Property(typeid(TYPE), std::move(name))
    , m_value(value)
    , m_own(owner)
  {}
  /// copy constructor (don't let the compiler generate a buggy one)
  inline PropertyWithValue ( const PropertyWithValue& right )
    : Property( right         )
    , m_value ( Traits::copy ( right.value() , right.m_own ) )
    , m_own   ( right.m_own   )
  {}
  /// copy constructor from any other type
  template <class OTHER>
  inline PropertyWithValue ( const PropertyWithValue<OTHER>& right )
    : Property( right         )
    , m_value ( Traits::copy ( right.value() , right.m_own ) )
    , m_own   ( right.m_own   )
  {}
  /// virtual destructor
  ~PropertyWithValue() override {
    Traits::dele ( m_value , m_own ) ;
  }
  /// assignment operator
  PropertyWithValue& operator=( const ValueType& value ) {
    if ( !setValue ( value ) )
      throw std::out_of_range( "Value not verified" ) ;
    return *this ;
  }
  // assignment operator (don't let the compiler generate a buggy one)
  PropertyWithValue& operator=( const PropertyWithValue& right ) {
    // assign the base class
    Property::operator=( right ) ;
    // assign the value
    PropertyWithValue<ValueType>::operator=( right.value() ) ;
    return *this ;
  }
  // assignment operator
  template <class OTHER>
  PropertyWithValue& operator=( const PropertyWithValue<OTHER>& right )
  {
    // assign the base class
    Property::operator=( right ) ;
    // assign the value
    PropertyWithValue<ValueType>::operator=( right.value() ) ;
    return *this ;
  }
  // ==========================================================================
public:
  // ==========================================================================
  /// implicit conversion to the type
  operator const ValueType&      () const { return value() ;}
  operator ValueType&      ()  { return value() ;}
  /// explicit conversion
  inline const ValueType& value() const { useReadHandler() ; return *m_value ; }
  inline ValueType& value() { useReadHandler() ; return *m_value ; }

  /// @name Helpers for easy use of string and vector properties.
  /// @{
  /// They are instantiated only if they are implemented in the wrapped class.
  template<class T=const ValueType>
  inline decltype(std::declval<T>().size()) size() const { return value().size(); }
  template<class T=const ValueType>
  inline decltype(std::declval<T>().empty()) empty() const { return value().empty(); }
  template<class T=const ValueType>
  inline decltype(std::declval<T>().begin()) begin() const { return value().begin(); }
  template<class T=const ValueType>
  inline decltype(std::declval<T>().end()) end() const { return value().end(); }
  template<class T=ValueType>
  inline decltype(std::declval<T>().begin()) begin() { return value().begin(); }
  template<class T=ValueType>
  inline decltype(std::declval<T>().end()) end() { return value().end(); }
  template<class T=const ValueType>
  inline decltype(std::declval<T>()[typename T::key_type{}])
    operator[] (const typename T::key_type & key) const { return value()[key]; }
  template<class T=ValueType>
  inline decltype(std::declval<T>()[typename T::key_type{}])
    operator[] (const typename T::key_type & key) { return value()[key]; }
  /// @}
  // ==========================================================================
public:
  // ==========================================================================
  /// NB: abstract : to be implemented when verifier is available
  virtual bool setValue ( const ValueType&     value  )  = 0  ;
  /// get the value from another property
  bool assign   ( const Property& source )       override {
    // 1) Is the property of "the same" type?
    const PropertyWithValue<TYPE>* p =
      dynamic_cast<const PropertyWithValue<TYPE>*>       ( &source ) ;
    if ( p ) { return setValue ( p->value() ) ; }       // RETURN
    // 2) Else use the string representation
    return this->fromString( source.toString() ).isSuccess() ;
  }
  /// set value for another property
  bool load     (       Property& dest   ) const override {
    // delegate to the 'opposite' method ;
    return dest.assign( *this ) ;
  }
  /// string -> value
  StatusCode fromString ( const std::string& source )  override {
    ValueType tmp ;
    StatusCode sc = Gaudi::Parsers::parse ( tmp , source ) ;
    if ( sc.isFailure() ) { return sc ; }
    return setValue ( tmp ) ? StatusCode::SUCCESS : StatusCode::FAILURE ;
  }
  /// value  -> string
  std::string  toString   () const  override {
    useReadHandler();
    return Gaudi::Utils::toString( *m_value ) ;
  }
  /// value  -> stream
  void  toStream (std::ostream& out) const  override {
    useReadHandler();
    Gaudi::Utils::toStream( *m_value, out ) ;
  }
  // ==========================================================================
protected:
  // ==========================================================================
  /// set the value
  inline void  i_set ( const ValueType& value ) {
    Traits::assign(*m_value, value);
  }
  /// get the value
  inline PVal  i_get () const {
    return m_value;
  }
  // ==========================================================================
private:
  // ==========================================================================
  /// the actual property value
  PVal m_value ;                                   // the actual property value
  /// owner of the storage
  bool  m_own  ;                                   //      owner of the storage
  // ==========================================================================
};
// ============================================================================
/// full specializations for std::string
// ============================================================================
template <>
inline std::string
PropertyWithValue<std::string>::toString () const
{
  useReadHandler();
  return this->value() ;
}
// ============================================================================
template <>
inline bool PropertyWithValue<std::string>::assign ( const Property& source )
{ return this->fromString( source.toString() ).isSuccess() ; }
// ============================================================================

// ============================================================================
/** @class PropertyWithVerifier
 *  Helper intermediate class which
 *  represent partly implemented property
 *  with value of concrete type and concrete verifier
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 *  @date 2006-02-27
 */
// ============================================================================
template<class TYPE,class VERIFIER>
class PropertyWithVerifier
  : public PropertyWithValue<TYPE>
{
public:
  using ValueType = typename PropertyWithValue<TYPE>::ValueType;
  using Traits = typename PropertyWithValue<TYPE>::Traits;
  using VerifierType = VERIFIER;
protected:
  // ==========================================================================
  /// the constructor with property name and value
  PropertyWithVerifier
  ( std::string                                           name     ,
    typename Gaudi::Utils::PropertyTypeTraits<TYPE>::PVal value    ,
    const bool                                            owner    ,
    const VerifierType&                                   verifier )
    : PropertyWithValue<TYPE> ( std::move(name) , value , owner )
    , m_verifier ( verifier )
  {}
  /// virtual destructor
  ~PropertyWithVerifier() override = default;
  // ==========================================================================
public:
  // ==========================================================================
  inline       VerifierType& verifier()       { return m_verifier ; }
  inline const VerifierType& verifier() const { return m_verifier ; }
  /// update the value of the property/check the verifier
  bool set( const ValueType& value ) {
    /// use verifier!
    if ( !m_verifier.isValid( &value ) ) { return false ; }
    /// update the value
    this->i_set( value ) ;
    /// invoke the update handler
    return this->useUpdateHandler() ;
  }
  /// implementation of PropertyWithValue::setValue
  bool setValue( const TYPE& value ) override { return set( value ) ; }
  /// templated assignment
  template <class OTHER,class OTHERVERIFIER>
  PropertyWithVerifier& operator=
  ( const PropertyWithVerifier<OTHER,OTHERVERIFIER>& right ) {
    PropertyWithValue<TYPE>::operator=(right) ;
    return *this ;
  }
  /// templated assignment
  template <class OTHER>
  PropertyWithVerifier& operator=( const PropertyWithValue<OTHER>& right ) {
    PropertyWithValue<TYPE>::operator=(right) ;
    return *this ;
  }
  /// assignment
  PropertyWithVerifier& operator=( const TYPE& right ) {
    PropertyWithValue<TYPE>::operator=( right ) ;
    return *this ;
  }
  // ==========================================================================
private:
  // ==========================================================================
  /// the default & copy constructors are deleted
  PropertyWithVerifier() = delete;
  PropertyWithVerifier( const  PropertyWithVerifier& right ) = delete;
  // ==========================================================================
private:
  // ==========================================================================
  /// the verifier itself
  VerifierType m_verifier ;                                  // the verifier itself
  // ==========================================================================
} ;
// ============================================================================
/** @class SimpleProperty Property.h GaudiKernel/Property.h
 *
 *  SimpleProperty concrete class which implements the full
 *  Property interface
 *
 *  @author Paul Maley
 *  @author CTDay
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 */
// ============================================================================
template <class TYPE,class VERIFIER = BoundedVerifier<TYPE> >
class SimpleProperty
  : public PropertyWithVerifier<TYPE,VERIFIER>
{
public:
  using ValueType = typename PropertyWithVerifier<TYPE,VERIFIER>::ValueType;
  using Traits = typename PropertyWithVerifier<TYPE,VERIFIER>::Traits;
  using VerifierType = typename PropertyWithVerifier<TYPE,VERIFIER>::VerifierType;
public:
  // ==========================================================================
  /// "Almost default" constructor from verifier
  SimpleProperty
  ( VerifierType       verifier = VerifierType() )
    : PropertyWithVerifier<TYPE,VERIFIER>("", Traits::new_(), true, verifier)
  {}
   /// The constructor from the value and verifier (ATLAS needs it!)
  SimpleProperty
  ( const ValueType&        value                 ,
    VerifierType       verifier = VerifierType() )
    : PropertyWithVerifier<TYPE,VERIFIER>("", Traits::new_(value), true, verifier)
  {}
  /// The constructor from the name, value and verifier
  SimpleProperty
  ( std::string        name                  ,
    const ValueType&        value                 ,
    std::string        doc = ""              ,
    VerifierType       verifier = VerifierType() )
    : PropertyWithVerifier<TYPE,VERIFIER>
      ( std::move(name) , Traits::new_(value) , true , verifier )
  {
    this->setDocumentation(std::move(doc));
  }
  /// The constructor from the name, value and verifier
  SimpleProperty
  ( PropertyMgr*       owner                 ,
    std::string        name                  ,
    const ValueType&        value = ValueType{}        ,
    std::string        doc = ""              ,
    VerifierType       verifier = VerifierType() )
    : SimpleProperty( std::move(name), value, std::move(doc), verifier )
  {
    this->declareTo(owner);
  }
  /// constructor from other property type
  template <class OTHER>
  SimpleProperty ( const PropertyWithValue<OTHER>& right )
    : PropertyWithVerifier<TYPE,VERIFIER>
      ( right.name() , Traits::new_( right.value() ) , true , VERIFIER() )
  {}
  /// copy constructor (must be!)
  SimpleProperty ( const SimpleProperty& right )
    : PropertyWithVerifier<TYPE,VERIFIER>
      ( right.name() , Traits::new_( right.value() ) , true , right.verifier() )
  {}
  /// virtual Destructor
  ~SimpleProperty() override = default;
  /// implementation of Property::clone
  SimpleProperty* clone() const override { return new SimpleProperty(*this) ; }
  /// assignment form the value
  SimpleProperty& operator=( const TYPE& value ) {
    PropertyWithVerifier<TYPE,VERIFIER>::operator=( value );
    return *this ;
  }
  /// assignment form the other property type
  template <class OTHER>
  SimpleProperty& operator=( const PropertyWithValue<OTHER>& right ) {
    PropertyWithVerifier<TYPE,VERIFIER>::operator=( right );
    return *this ;
  }
  // ==========================================================================
};
// ============================================================================

// ============================================================================
/** @class SimplePropertyRef Property.h GaudiKernel/Property.h
 *
 *  SimplePropertyRef templated class
 *
 *  @author Paul Maley
 *  @author CTDay
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 */
// ============================================================================
template< class TYPE, class VERIFIER = NullVerifier<TYPE> >
class SimplePropertyRef :
  public PropertyWithVerifier<TYPE,VERIFIER>
{
public:
  using ValueType = typename PropertyWithVerifier<TYPE,VERIFIER>::ValueType;
  using Traits = typename PropertyWithVerifier<TYPE,VERIFIER>::Traits;
  using VerifierType = typename PropertyWithVerifier<TYPE,VERIFIER>::VerifierType;
public:
  /// Constructor from the name, the value and the verifier
  SimplePropertyRef
  ( std::string        name                  ,
    ValueType&         value                 ,  ///< NB! non-const reference
    VerifierType       verifier = VerifierType() )
    : PropertyWithVerifier<TYPE,VERIFIER> ( std::move(name) , &value , false , verifier )
  {}
  /// copy constructor (must be!)
  SimplePropertyRef ( const SimplePropertyRef& right )
    : PropertyWithVerifier<TYPE,VERIFIER>
      ( right.name() , right.i_get() , false , right.verifier() )
  {}
  /// virtual Destructor
  ~SimplePropertyRef() override = default;
  /// implementation of Property::clone
  SimplePropertyRef* clone() const override { return new SimplePropertyRef(*this) ; }
  /// assignment form the value
  SimplePropertyRef& operator=( const TYPE& value ) {
    PropertyWithVerifier<TYPE,VERIFIER>::operator=( value ) ;
    return *this ;
  }
  /// assignment form the other property type
  template <class OTHER>
  SimplePropertyRef& operator=( const PropertyWithValue<OTHER>& right ) {
    PropertyWithVerifier<TYPE,VERIFIER>::operator=( right );
    return *this ;
  }
private:
  // the default constructor is disabled
  SimplePropertyRef() = delete;
};
// ============================================================================




// Typedef Properties for built-in types
typedef SimpleProperty< bool >              BooleanProperty;
typedef SimpleProperty< char >              CharProperty;
typedef SimpleProperty< signed char >       SignedCharProperty;
typedef SimpleProperty< unsigned char >     UnsignedCharProperty;
typedef SimpleProperty< short >             ShortProperty;
typedef SimpleProperty< unsigned short >    UnsignedShortProperty;
typedef SimpleProperty< int >               IntegerProperty;
typedef SimpleProperty< unsigned int >      UnsignedIntegerProperty;
typedef SimpleProperty< long >              LongProperty;
typedef SimpleProperty< unsigned long >     UnsignedLongProperty;
typedef SimpleProperty< long long>          LongLongProperty;
typedef SimpleProperty< unsigned long long> UnsignedLongLongProperty;
typedef SimpleProperty< float >             FloatProperty;
typedef SimpleProperty< double >            DoubleProperty;
typedef SimpleProperty< long double >       LongDoubleProperty;

typedef SimpleProperty< std::string >       StringProperty;


// Typedef PropertyRefs for built-in types
typedef SimplePropertyRef< bool >                BooleanPropertyRef;
typedef SimplePropertyRef< char >                CharPropertyRef;
typedef SimplePropertyRef< signed char >         SignedCharPropertyRef;
typedef SimplePropertyRef< unsigned char >       UnsignedCharPropertyRef;
typedef SimplePropertyRef< short >               ShortPropertyRef;
typedef SimplePropertyRef< unsigned short >      UnsignedShortPropertyRef;
typedef SimplePropertyRef< int >                 IntegerPropertyRef;
typedef SimplePropertyRef< unsigned int >        UnsignedIntegerPropertyRef;
typedef SimplePropertyRef< long >                LongPropertyRef;
typedef SimplePropertyRef< unsigned long >       UnsignedLongPropertyRef;
typedef SimplePropertyRef< long long >           LongLongPropertyRef;
typedef SimplePropertyRef< unsigned long long >  UnsignedLongLongPropertyRef;
typedef SimplePropertyRef< float >               FloatPropertyRef;
typedef SimplePropertyRef< double >              DoublePropertyRef;
typedef SimplePropertyRef< long double >         LongDoublePropertyRef;

typedef SimplePropertyRef< std::string >         StringPropertyRef;


// Typedef "Arrays" of Properties for built-in types
typedef SimpleProperty< std::vector< bool > >                BooleanArrayProperty;
typedef SimpleProperty< std::vector< char > >                CharArrayProperty;
typedef SimpleProperty< std::vector< signed char > >         SignedCharArrayProperty;
typedef SimpleProperty< std::vector< unsigned char > >       UnsignedCharArrayProperty;
typedef SimpleProperty< std::vector< short > >               ShortArrayProperty;
typedef SimpleProperty< std::vector< unsigned short > >      UnsignedShortArrayProperty;
typedef SimpleProperty< std::vector< int > >                 IntegerArrayProperty;
typedef SimpleProperty< std::vector< unsigned int > >        UnsignedIntegerArrayProperty;
typedef SimpleProperty< std::vector< long > >                LongArrayProperty;
typedef SimpleProperty< std::vector< unsigned long > >       UnsignedLongArrayProperty;
typedef SimpleProperty< std::vector< long long > >           LongLongArrayProperty;
typedef SimpleProperty< std::vector< unsigned long long > >  UnsignedLongLongArrayProperty;
typedef SimpleProperty< std::vector< float > >               FloatArrayProperty;
typedef SimpleProperty< std::vector< double > >              DoubleArrayProperty;
typedef SimpleProperty< std::vector< long double > >         LongDoubleArrayProperty;

typedef SimpleProperty< std::vector< std::string > >         StringArrayProperty;


// Typedef "Arrays" of PropertyRefs for built-in types
typedef SimplePropertyRef< std::vector< bool > >                 BooleanArrayPropertyRef;
typedef SimplePropertyRef< std::vector< char > >                 CharArrayPropertyRef;
typedef SimplePropertyRef< std::vector< signed char > >          SignedCharArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned char > >        UnsignedCharArrayPropertyRef;
typedef SimplePropertyRef< std::vector< short > >                ShortArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned short > >       UnsignedShortArrayPropertyRef;
typedef SimplePropertyRef< std::vector< int > >                  IntegerArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned int > >         UnsignedIntegerArrayPropertyRef;
typedef SimplePropertyRef< std::vector< long > >                 LongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned long > >        UnsignedLongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< long long > >            LongLongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< unsigned long long > >   UnsignedLongLongArrayPropertyRef;
typedef SimplePropertyRef< std::vector< float > >                FloatArrayPropertyRef;
typedef SimplePropertyRef< std::vector< double > >               DoubleArrayPropertyRef;
typedef SimplePropertyRef< std::vector< long double > >          LongDoubleArrayPropertyRef;

typedef SimplePropertyRef< std::vector< std::string > >          StringArrayPropertyRef;

// forward-declaration is sufficient here
class GaudiHandleBase;

// implementation in header file only where the GaudiHandleBase class
// definition is not needed. The rest goes into the .cpp file.
// The goal is to decouple the header files, to avoid that the whole
// world depends on GaudiHandle.h
class GAUDI_API GaudiHandleProperty : public Property {
public:
  GaudiHandleProperty( std::string name, GaudiHandleBase& ref );

  GaudiHandleProperty& operator=( const GaudiHandleBase& value ) {
        setValue( value );
        return *this;
  }

  GaudiHandleProperty* clone() const override  {
    return new GaudiHandleProperty( *this );
  }

  bool load( Property& destination ) const override  {
    return destination.assign( *this );
  }

  bool assign( const Property& source ) override  {
    return fromString( source.toString() ).isSuccess();
  }

  std::string toString() const override;

  void toStream(std::ostream& out) const override;

  StatusCode fromString(const std::string& s) override;

  const GaudiHandleBase& value() const  {
    useReadHandler();
    return *m_pValue;
  }

  bool setValue( const GaudiHandleBase& value );

private:
  /** Pointer to the real property. Reference would be better, but ROOT does not
      support references yet */
   GaudiHandleBase* m_pValue;
};


// forward-declaration is sufficient here
class GaudiHandleArrayBase;

class GAUDI_API GaudiHandleArrayProperty : public Property {
public:

  GaudiHandleArrayProperty( std::string name, GaudiHandleArrayBase& ref );

  GaudiHandleArrayProperty& operator=( const GaudiHandleArrayBase& value ) {
    setValue( value );
    return *this;
  }

  GaudiHandleArrayProperty* clone() const override {
    return new GaudiHandleArrayProperty( *this );
  }

  bool load( Property& destination ) const override {
    return destination.assign( *this );
  }

  bool assign( const Property& source ) override {
    return fromString( source.toString() ) != 0;
  }

  std::string toString() const override;

  void toStream(std::ostream& out) const override;

  StatusCode fromString(const std::string& s) override;

  const GaudiHandleArrayBase& value() const {
    useReadHandler();
    return *m_pValue;
  }

  bool setValue( const GaudiHandleArrayBase& value );

private:
  /** Pointer to the real property. Reference would be better, but ROOT does not
      support references yet */
   GaudiHandleArrayBase* m_pValue;

};


namespace Gaudi
{
  namespace Utils
  {
    // ========================================================================
    /** simple function which check the existence of the property with
     *  the given name.
     *
     *  @code
     *
     *  const IProperty* p = ... ;
     *
     *  const bool = hasProperty( p , "Context" ) ;
     *
     *  @endcode
     *
     *  @param  p    pointer to IProperty object
     *  @param  name property name (case insensitive)
     *  @return true if "p" has a property with such name
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    GAUDI_API bool hasProperty ( const IProperty*   p , const std::string& name ) ;
    // ========================================================================
    /** simple function which check the existence of the property with
     *  the given name.
     *
     *  @code
     *
     *  IInterface* p = .
     *
     *  const bool = hasProperty( p , "Context" ) ;
     *
     *  @endcode
     *
     *  @param  p    pointer to IInterface   object (any component)
     *  @param  name property name (case insensitive)
     *  @return true if "p" has a property with such name
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    GAUDI_API bool hasProperty ( const IInterface*   p , const std::string& name ) ;
    // ========================================================================
    /** simple function which gets the property with given name
     *  from the component
     *
     *  @code
     *
     *  const IProperty* p = ... ;
     *
     *  const Property* pro = getProperty( p , "Context" ) ;
     *
     *  @endcode
     *
     *  @param  p    pointer to IProperty object
     *  @param  name property name (case insensitive)
     *  @return property with the given name (if exists), NULL otherwise
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    GAUDI_API Property* getProperty
    ( const IProperty*   p , const std::string& name ) ;
    // ========================================================================
    /** simple function which gets the property with given name
     *  from the component
     *
     *  @code
     *
     *  const IInterface* p = ... ;
     *
     *  const Property* pro = getProperty( p , "Context" ) ;
     *
     *  @endcode
     *
     *  @param  p    pointer to IInterface object
     *  @param  name property name (case insensitive)
     *  @return property with the given name (if exists), NULL otherwise
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    GAUDI_API Property* getProperty
    ( const IInterface*   p , const std::string& name ) ;
    // ========================================================================
    /** check  the property by name from  the list of the properties
     *
     *  @code
     *
     *   IJobOptionsSvc* svc = ... ;
     *
     *   const std::string client = ... ;
     *
     *  // get the property:
     *  bool context =
     *      hasProperty ( svc->getProperties( client ) , "Context" )
     *
     *  @endcode
     *
     *  @see IJobOptionsSvc
     *
     *  @param  p    list of properties
     *  @param  name property name (case insensitive)
     *  @return true if the property exists
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    GAUDI_API bool hasProperty
    ( const std::vector<const Property*>* p    ,
      const std::string&                  name ) ;
    // ========================================================================
    /** get the property by name from  the list of the properties
     *
     *  @code
     *
     *   IJobOptionsSvc* svc = ... ;
     *
     *   const std::string client = ... ;
     *
     *  // get the property:
     *  const Property* context =
     *      getProperty ( svc->getProperties( client ) , "Context" )
     *
     *  @endcode
     *
     *  @see IJobOptionsSvc
     *
     *  @param  p    list of properties
     *  @param  name property name (case insensitive)
     *  @return property with the given name (if exists), NULL otherwise
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date   2006-09-09
     */
    GAUDI_API const Property* getProperty
    ( const std::vector<const Property*>* p    ,
      const std::string&                  name ) ;
    // ========================================================================
    /** simple function to set the property of the given object from the value
     *
     *  @code
     *
     *  IProperty* component = ... ;
     *
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( componet , "Data" ,  data ) ;
     *
     *  @endcode
     *
     *  Note: the interface IProperty allows setting of the properties either
     *        directly from other properties or from strings only
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param value     value of the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    template <class TYPE>
    StatusCode setProperty
    ( IProperty*         component  ,
      const std::string& name       ,
      const TYPE&        value      ,
      const std::string& doc        ) ;
    // ========================================================================
    /** simple function to set the property of the given object from the value
     *
     *  @code
     *
     *  IProperty* component = ... ;
     *
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( componet , "Data" ,  data ) ;
     *
     *  @endcode
     *
     *  Note: the interface IProperty allows setting of the properties either
     *        directly from other properties or from strings only
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param value     value of the property
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    template <class TYPE>
    StatusCode setProperty
    ( IProperty*         component  ,
      const std::string& name       ,
      const TYPE&        value      )
    { return setProperty ( component , name , value , std::string() ) ; }
    // ========================================================================
    /** the full specialization of the
     *  previous method setProperty( IProperty, std::string, const TYPE&)
     *  for standard strings
     *
     *  @param component component which needs to be configured
     *  @param name      name of the property
     *  @param value     value of the property
     *  @param doc       the new documentation string
     *
     *  @see IProperty
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IProperty*         component  ,
      const std::string& name       ,
      const std::string& value      ,
      const std::string& doc   = "" ) ;
    // ========================================================================
    /** the full specialization of the
     *  method setProperty( IProperty, std::string, const TYPE&)
     *  for C-strings
     *
     *  @param component component which needs to be configured
     *  @param name      name of the property
     *  @param value     value of the property
     *  @param doc       the new documentation string
     *
     *  @see IProperty
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IProperty*         component  ,
      const std::string& name       ,
      const char*        value      ,
      const std::string& doc   = "" ) ;
    // ========================================================================
    /** the full specialization of the
     *  method setProperty( IProperty, std::string, const TYPE&)
     *  for C-arrays
     *
     *  @param component component which needs to be configured
     *  @param name      name of the property
     *  @param value     value of the property
     *  @param doc       the new documentation string
     *
     *  @see IProperty
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13
     */
    template <unsigned N>
    StatusCode setProperty
    ( IProperty*           component ,
      const std::string&   name      ,
      const char         (&value)[N] ,
      const std::string& doc   = ""  )
    {
      return component ? setProperty ( component , name ,
                                       std::string ( value , value + N ), doc )
                       : StatusCode::FAILURE;
    }
    // ========================================================================
    /** simple function to set the property of the given object from the value
     *
     *  @code
     *
     *  IProperty* component = ... ;
     *
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  data ) ;
     *
     *  std::map<std::string,double> cuts = ... ;
     *  sc = setProperty ( component , "Cuts" , cuts ) ;
     *
     *  std::map<std::string,std::string> dict = ... ;
     *  sc = setProperty ( component , "Dictionary" , dict ) ;
     *
     *  @endcode
     *
     *  Note: the native interface IProperty allows setting of the
     *        properties either directly from other properties or
     *        from strings only
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param value     value of the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    template <class TYPE>
    StatusCode setProperty
    ( IProperty*         component  ,
      const std::string& name       ,
      const TYPE&        value      ,
      const std::string& doc        )
    {
      return component && hasProperty(component,name) ?
                Gaudi::Utils::setProperty ( component , name ,
                                            Gaudi::Utils::toString ( value ) , doc ) :
                StatusCode::FAILURE;
    }
    // ========================================================================
    /** simple function to set the property of the given object from another
     *  property
     *
     *  @code
     *
     *  IProperty* component = ... ;
     *
     *  const Property* prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param property  the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IProperty*         component ,
      const std::string& name      ,
      const Property*    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another
     *  property
     *
     *  @code
     *
     *  IProperty* component = ... ;
     *
     *  const Property& prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param property  the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IProperty*         component ,
      const std::string& name      ,
      const Property&    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another
     *  property
     *
     *  @code
     *
     *  IProperty* component = ... ;
     *
     *  SimpleProperty<std::vector<int> > m_data = ... ;
     *
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param value     the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    template <class TYPE>
    StatusCode setProperty
    ( IProperty*                  component ,
      const std::string&          name      ,
      const SimpleProperty<TYPE>& value     ,
      const std::string&          doc = ""  )
    {
      return setProperty ( component , name , &value , doc ) ;
    }
    // ========================================================================
    /** simple function to set the property of the given object from the value
     *
     *  @code
     *
     *  IInterface* component = ... ;
     *
     *  std::vector<double> data = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  data ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param value     value of the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    template <class TYPE>
    StatusCode setProperty
    ( IInterface*        component ,
      const std::string& name      ,
      const TYPE&        value     ,
      const std::string& doc = ""  )
    {
      if ( !component ) { return StatusCode::FAILURE ; }
      auto property = SmartIF<IProperty>{  component } ;
      return property ? setProperty ( property , name , value , doc )
                      : StatusCode::FAILURE;
    }
    // ========================================================================
    /** the full specialization of the
     *  method setProperty( IInterface , std::string, const TYPE&)
     *  for standard strings
     *
     *  @param component component which needs to be configured
     *  @param name      name of the property
     *  @param value     value of the property
     *  @param doc       the new documentation string
     *
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IInterface*        component ,
      const std::string& name      ,
      const std::string& value     ,
      const std::string& doc  = "" ) ;
    // ========================================================================
    /** the full specialization of the
     *  method setProperty( IInterface , std::string, const TYPE&)
     *  for C-strings
     *
     *  @param component component which needs to be configured
     *  @param name      name of the property
     *  @param value     value of the property
     *  @param doc       the new documentation string
     *
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IInterface*        component ,
      const std::string& name      ,
      const char*        value     ,
      const std::string& doc  = "" ) ;
    // ========================================================================
    /** the full specialization of the
     *  method setProperty( IInterface, std::string, const TYPE&)
     *  for C-arrays
     *
     *  @param component component which needs to be configured
     *  @param name      name of the property
     *  @param value     value of the property
     *  @param doc       the new documentation string
     *
     *  @see IProperty
     *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
     *  @date 2007-05-13
     */
    template <unsigned N>
    StatusCode setProperty
    ( IInterface*          component ,
      const std::string&   name      ,
      const char         (&value)[N] ,
      const std::string& doc  = ""   )
    {
      if ( 0 == component ) { return StatusCode::FAILURE ; }
      return setProperty ( component , name ,
                           std::string{ value , value + N }, doc ) ;
    }
    // ========================================================================
    /** simple function to set the property of the given object from another
     *  property
     *
     *  @code
     *
     *  IInterface* component = ... ;
     *
     *  const Property* prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param property  the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IInterface*        component ,
      const std::string& name      ,
      const Property*    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another
     *  property
     *
     *  @code
     *
     *  IInterface* component = ... ;
     *
     *  const Property& prop = ... ;
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param property  the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    GAUDI_API StatusCode setProperty
    ( IInterface*        component ,
      const std::string& name      ,
      const Property&    property  ,
      const std::string& doc = ""  ) ;
    // ========================================================================
    /** simple function to set the property of the given object from another
     *  property
     *
     *  @code
     *
     *  IInterface* component = ... ;
     *
     *  SimpleProperty<std::vector<int> > m_data = ... ;
     *
     *  StatusCode sc = setProperty ( component , "Data" ,  prop  ) ;
     *
     *  @endcode
     *
     * @param component component which needs to be configured
     * @param name      name of the property
     * @param value     the property
     * @param doc       the new documentation string
     *
     * @see IProperty
     * @author Vanya BELYAEV ibelyaev@physics.syr.edu
     * @date 2007-05-13
     */
    template <class TYPE>
    StatusCode
    setProperty
    ( IInterface*                 component ,
      const std::string&          name      ,
      const SimpleProperty<TYPE>& value     ,
      const std::string&          doc = ""  )
    {
      return setProperty ( component , name , &value , doc ) ;
    }
    // ========================================================================
  } // end of namespace Gaudi::Utils
} // end of namespace Gaudi
// ============================================================================
// The END
// ============================================================================
#endif // GAUDIKERNEL_PROPERTY_H
// ============================================================================
