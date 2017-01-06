#ifdef __ICC
// disable icc remark #1572: floating-point equality and inequality comparisons are unreliable
//   The comparisons are meant
#pragma warning(disable:1572)
#endif
// ============================================================================
// Include files 
// ============================================================================
// STD & STL 
// ============================================================================
#include <cmath>
#include <limits>
#include <climits>
#include <iostream>
// ============================================================================
// AIDA 
// ============================================================================
#include "AIDA/IHistogram1D.h"
#include "AIDA/IAxis.h"
// ============================================================================
// GaudiUtils 
// ============================================================================
#include "GaudiUtils/HistoStats.h"
// ============================================================================
/** @file
 *  Implementation file for class Gaudi::Utils::HistoStats
 *  @author Vanya BELYAEV ibelyaev@physics.syr.edu
 *  @date 2007-08-06 
 */
// ============================================================================
namespace 
{
  // ==========================================================================
  /// define local "bad" value 
  const double s_bad      = std::numeric_limits<float>::lowest() ;
  /// define local "bad" value 
  const long   s_long_bad = std::numeric_limits<int>::min() ;
  // ==========================================================================
}
// ============================================================================
/*  get the moment of the certain around the specified  "value"
 *  @param histo histogram
 *  @param order the momentm order
 *  @param value central value 
 *  @return the evaluated moment
 */
// ============================================================================
double Gaudi::Utils::HistoStats::moment        
( const AIDA::IHistogram1D* histo , 
  const unsigned int        order ,  
  const double              value ) 
{
  if ( !histo     ) { return s_bad ; }                       // RETURN
  if ( 0 == order ) { return 1.0   ; }                       // RETURN
  if ( 1 == order ) { return mean( histo ) - value ; }       // RETURN
  if ( 2 == order ) 
  {
    const auto _r =         rms  ( histo ) ;
    const auto _d = value - mean ( histo ) ;
    return std::pow(_r,2) + std::pow(_d,2) ;                 // RETURN
  }
  const auto n = nEff ( histo )  ;
  if ( 0 >= n     ) { return 0.0   ; }                   // RETURN 
  
  // get the exis 
  const auto & axis = histo -> axis () ;
  // number of bins 
  const auto nBins = axis.bins() ;
  double result{0}, weight{0};
  // loop over all bins 
  for ( int i = 0 ; i < nBins ; ++i ) 
  {
    const auto lE   = axis . binLowerEdge ( i ) ;
    const auto uE   = axis . binUpperEdge ( i ) ;
    //
    const auto yBin = histo -> binHeight  ( i ) ;   // bin weight 
    const double xBin = 0.5 * ( lE + uE ) ;         // bin center 
    //
    weight += yBin ;
    result += yBin * std::pow ( xBin - value , order ) ;
  }    
  if ( 0 != weight ) { result /= weight ; }
  return result ;
}
// ============================================================================
/** evaluate the uncertanty for 'bin-by-bin'-moment
 *  @param histo histogram
 *  @param order the moment parameter 
 *  @param value central value 
 *  @return the evaluated uncertanty in the moment
 */
// ============================================================================
double Gaudi::Utils::HistoStats::momentErr
( const AIDA::IHistogram1D* histo , 
  const unsigned int        order ) 
{
  if ( !histo     ) { return s_bad ; }                   // RETURN 
  const auto n = nEff ( histo ) ;
  if ( 0 >= n     ) { return 0.0   ; }                   // RETURN
  const auto a2o = moment ( histo , 2 * order ) ;      // a(2o)
  const auto ao  = moment ( histo ,     order ) ;      // a(o) 
  const auto result = std::max ( 0.0 , ( a2o - std::pow(ao,2) ) / n ) ;
  return std::sqrt ( result ) ;                            // RETURN  
}
// ============================================================================
/*  evaluate the central momentum (around the mean value) 
 *  @param histo histogram
 *  @param order the momentm order
 *  @param value central value 
 *  @return the evaluated central moment
 */
// ============================================================================
double Gaudi::Utils::HistoStats::centralMoment 
( const AIDA::IHistogram1D* histo , 
  const unsigned int        order ) 
{
  if ( !histo     ) { return s_bad ; }                        // RETURN
  if ( 0 == order ) { return 1.0   ; }                        // RETURN
  if ( 1 == order ) { return 0.0   ; }                        // RETURN
  if ( 2 == order ) 
  {
    return std::pow( histo->rms(), 2 );                       // RETURN
  }
  // delegate the actual evaluation to another method:
  return moment ( histo , order , mean ( histo ) ) ;
}
// ============================================================================
/*  evaluate the uncertanty for 'bin-by-bin'-central momentum 
 *  (around the mean value)  
 *  ( the uncertanty is calculated with O(1/n2) precision)
 *  @param histo histogram
 *  @param order the moment parameter 
 *  @param value central value 
 *  @return the evaluated uncertanty in the central moment
 */
// ============================================================================
double Gaudi::Utils::HistoStats::centralMomentErr
( const AIDA::IHistogram1D* histo , 
  const unsigned int        order ) 
{
  if ( !histo     ) { return s_bad ; }                    // RETURN
  const auto n    = nEff ( histo ) ;
  if ( 0 >= n     ) { return 0.0   ; }                    // RETURN
  const auto mu2  = centralMoment ( histo , 2             ) ; // mu(2)
  const auto muo  = centralMoment ( histo ,     order     ) ; // mu(o)
  const auto mu2o = centralMoment ( histo , 2 * order     ) ; // mu(2o)
  const auto muom = centralMoment ( histo ,     order - 1 ) ; // mu(o-1)
  const auto muop = centralMoment ( histo ,     order + 1 ) ; // mu(o+1)
  const auto result = ( mu2o 
                        - ( 2.0   * order * muom * muop )
                        -  std::pow(muo,2)  
                        + ( order * order * mu2  * muom * muom ) ) / n ;
  return std::sqrt ( std::max ( 0.0 , result ) ) ;           // RETURN
}
// ============================================================================
// get the skewness for the histogram 
// ============================================================================
double Gaudi::Utils::HistoStats::skewness        
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo )      { return s_bad ; }                      // RETURN
  const auto mu3 = centralMoment ( histo , 3 ) ;
  const auto s3  = std::pow ( rms ( histo ) , 3 ) ;
  return ( std::fabs(s3)>0 ? mu3/s3 : 0.0 );
}
// ============================================================================
// get the error in skewness 
// ============================================================================
double Gaudi::Utils::HistoStats::skewnessErr 
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo     ) { return s_bad ; }                     // RETURN 
  const auto n = nEff ( histo ) ;
  if ( 2 > n      ) { return 0.0   ; }                     // RETURN
  const auto result = 6.0 * ( n - 2 ) / ( ( n + 1 ) * ( n + 3 ) );    
  return std::sqrt ( result ) ;  
}
// ============================================================================
// get the kurtosis for the histogram 
// ============================================================================
double Gaudi::Utils::HistoStats::kurtosis          
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; }                    // RETURN 
  const auto mu4 = centralMoment ( histo , 4 ) ;
  const auto s4  = std::pow ( rms ( histo ) , 4 ) ;
  return ( std::fabs(s4)>0 ? mu4/s4 - 3.0 : 0.0 );
}
// ============================================================================
// get the error in kurtosis
// ============================================================================
double Gaudi::Utils::HistoStats::kurtosisErr 
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; }                    // RETURN 
  const auto n = nEff ( histo ) ;
  if ( 3 > n  ) { return 0.0 ; }                      // RETURN   
  auto result = 24.0 * n ;
  result *= ( n - 2 ) * ( n - 3 ) ;
  result /= ( n + 1 ) * ( n + 1 ) ;
  result /= ( n + 3 ) * ( n + 5 ) ;
  return std::sqrt ( result ) ;  
}
// ============================================================================
// get the effective entries 
// ============================================================================
double Gaudi::Utils::HistoStats::nEff 
( const AIDA::IHistogram1D* histo ) 
{
  return ( histo ? histo->equivalentBinEntries() : s_bad );
}
// ============================================================================
// get the mean value for the histogram 
// ============================================================================
double Gaudi::Utils::HistoStats::mean              
( const AIDA::IHistogram1D* histo ) 
{
  return ( histo ? histo -> mean() : s_bad );
}
// ============================================================================
// get an error in the mean value 
// ============================================================================
double Gaudi::Utils::HistoStats::meanErr 
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; }
  const auto n = nEff ( histo ) ;
  return ( 0 >= n ? 0.0 : rms ( histo ) / std::sqrt ( n ) );
}
// ============================================================================
// get the rms value for the histogram 
// ============================================================================
double Gaudi::Utils::HistoStats::rms
( const AIDA::IHistogram1D* histo ) 
{
  return ( histo ? histo -> rms () : s_bad );
}
// ============================================================================
// get the error in rms 
// ============================================================================
double Gaudi::Utils::HistoStats::rmsErr  
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; }  
  const auto n = nEff ( histo ) ;    
  if ( 1 >=  n ) { return 0.0 ; }
  auto result = 2.0 + kurtosis ( histo ) ;
  result += 2.0 / ( n - 1 ) ;
  result /= 4.0 * n ;
  return histo -> rms() * std::sqrt ( std::max ( result , 0.0 ) ) ;
}
// ============================================================================
// get an error in the sum bin height ("in range integral")
// ============================================================================
double Gaudi::Utils::HistoStats::sumBinHeightErr
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; }  
  //
  double error2 = 0 ;
  // get the exis 
  const auto & axis = histo -> axis () ;
  // number of bins 
  const auto nBins = axis.bins() ;
  // loop over all bins 
  for ( int i = 0 ; i < nBins ; ++i ) 
  {
    // accumulate the errors in each bin
    error2 += std::pow( histo->binError(i), 2 ) ;    
  } 
  return std::sqrt ( error2 ) ;
}
// ============================================================================
// get an error in the sum all bin height ("integral")
// ============================================================================
double Gaudi::Utils::HistoStats::sumAllBinHeightErr
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; }  
  //
  const auto error = sumBinHeightErr( histo ) ;
  if ( 0 > error ) { return s_bad ; }
  ///
  const auto err1 = histo->binError ( AIDA::IAxis::UNDERFLOW_BIN ) ;
  const auto err2 = histo->binError ( AIDA::IAxis::OVERFLOW_BIN  ) ;
  //
  return std::sqrt ( std::pow(error,2) + std::pow(err1,2) + std::pow(err2,2) ) ;
}
// ============================================================================
// the fraction of overflow entries  (useful for shape comparison)
// ============================================================================
double Gaudi::Utils::HistoStats::overflowEntriesFrac
( const AIDA::IHistogram1D* histo     ) 
{
  if ( !histo ) { return s_bad ; }                                    // REUTRN  
  const auto overflow = histo -> binEntries ( AIDA::IAxis::OVERFLOW_BIN )  ;
  if ( 0 == overflow ) { return 0 ; }                                 // REUTRN 
  const auto all      = histo->allEntries() ;
  if ( 0 == all      ) { return 0 ; }                  // "CONVENTION?"  RETURN 
  if ( 0 >  all      ) { return s_bad ; }     // Lets be a bit paranoic, RETURN
  //
  return (double) overflow / (double) all;
}
// ============================================================================
// the fraction of underflow entries  (useful for shape comparison)
// ============================================================================
double Gaudi::Utils::HistoStats::underflowEntriesFrac
( const AIDA::IHistogram1D* histo     ) 
{
  if ( !histo ) { return s_bad ; }                                    // REUTRN  
  const auto underflow = histo -> binEntries ( AIDA::IAxis::UNDERFLOW_BIN )  ;
  if ( 0 == underflow ) { return 0 ; }                                // REUTRN 
  const auto all      = histo->allEntries() ;
  if ( 0 == all      ) { return 0 ; }                  // "CONVENTION?"  RETURN 
  if ( 0 >  all      ) { return s_bad ; }     // Lets be a bit paranoic, RETURN
  //
  return (double) underflow / (double) all;
}
// ============================================================================
// the fraction of overflow integral (useful for shape comparison)
// ============================================================================
double Gaudi::Utils::HistoStats::overflowIntegralFrac
( const AIDA::IHistogram1D* histo     ) 
{
  if ( !histo ) { return s_bad ; }                                    // REUTRN  
  const auto overflow = histo -> binHeight ( AIDA::IAxis::OVERFLOW_BIN )  ;
  if ( 0 == overflow ) { return 0 ; }                                 // REUTRN
  const auto all      = histo -> sumAllBinHeights() ;
  if ( 0 == all      ) { return 0 ; }                  // "CONVENTION?"  RETURN 
  //
  return (double)overflow / (double)all ;
}
// ============================================================================
// the fraction of underflow entries  (useful for shape comparison)
// ============================================================================
double Gaudi::Utils::HistoStats::underflowIntegralFrac
( const AIDA::IHistogram1D* histo     ) 
{
  if ( !histo ) { return s_bad ; }                                    // REUTRN  
  const auto underflow = histo -> binHeight ( AIDA::IAxis::UNDERFLOW_BIN )  ;
  if ( 0 == underflow ) { return 0 ; }                                // REUTRN 
  const auto all      = histo -> sumAllBinHeights() ;
  if ( 0 == all      ) { return 0 ; }                  // "CONVENTION?"  RETURN 
  //
  return (double)underflow / (double)all ; 
}
// ============================================================================
// error on fraction of overflow entries  (useful for shape comparison)
// ============================================================================
double Gaudi::Utils::HistoStats::overflowEntriesFracErr 
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; } // RETURN 
  // 
  const auto overflow = histo -> binEntries ( AIDA::IAxis::OVERFLOW_BIN );
  const auto all      = histo -> allEntries () ;
  //
  if ( 0 > overflow || 0 >= all || overflow > all ) { return s_bad ; }
  // 
  const double n  = std::max ( (double)overflow , 1.0 ) ;
  const double N  = all ;
  const double n1 = std::max ( N - n , 1.0 ) ;
  //
  return std::sqrt ( n * ( n1 / N ) ) / N ; // use the binomial formula 
}
// ============================================================================
// error on fraction of underflow entries  (useful for shape comparison)
// ============================================================================
double Gaudi::Utils::HistoStats::underflowEntriesFracErr 
( const AIDA::IHistogram1D* histo ) 
{
  if ( !histo ) { return s_bad ; } // RETURN 
  // 
  const auto underflow = histo -> binEntries ( AIDA::IAxis::UNDERFLOW_BIN );
  const auto all       = histo -> allEntries () ;
  //
  if ( 0 > underflow || 0 >= all || underflow > all ) { return s_bad ; }
  // 
  const double n  = std::max ( (double)underflow , 1.0 ) ;
  const double N  = all ;
  const double n1 = std::max ( N - n , 1.0 ) ;
  //
  return std::sqrt ( n * ( n1 / N ) ) / N ; // use the binomial formula 
}
// ============================================================================
// the error on fraction of overflow intergal 
// ============================================================================
double Gaudi::Utils::HistoStats::overflowIntegralFracErr 
( const AIDA::IHistogram1D* histo     ) 
{
  if ( !histo ) { return s_bad ; }                                    // RETURN
  //
  const auto all       = histo -> sumAllBinHeights () ;
  if ( 0 == all       ) { return s_bad ; }                            // RETURN 
  const auto overflow  = histo -> binHeight ( AIDA::IAxis::OVERFLOW_BIN )  ;
  const auto oErr      = histo -> binError  ( AIDA::IAxis::OVERFLOW_BIN )  ;
  if ( 0 > oErr       ) { return s_bad ; }                            // RETURN 
  const auto aErr      = sumAllBinHeightErr ( histo ) ;
  if ( 0 > aErr       ) { return s_bad ; }                            // RETURN 
  //
  double error2  = std::pow((double)oErr,2) ;
  error2        += ( std::pow((double)aErr,2) * 
                     std::pow((double)overflow,2) / std::pow((double)all,2) );
  error2        /= std::pow((double)all,2) ;
  //
  return std::sqrt ( error2 ) ;
}
// ============================================================================
// the error on fraction of overflow intergal 
// ============================================================================
double Gaudi::Utils::HistoStats::underflowIntegralFracErr 
( const AIDA::IHistogram1D* histo     ) 
{
  if ( !histo ) { return s_bad ; }                                    // RETURN
  //
  const auto all       = histo -> sumAllBinHeights () ;
  if ( 0 == all       ) { return s_bad ; }                            // RETURN 
  const auto underflow  = histo -> binHeight ( AIDA::IAxis::UNDERFLOW_BIN )  ;
  const auto oErr       = histo -> binError  ( AIDA::IAxis::UNDERFLOW_BIN )  ;
  if ( 0 > oErr       ) { return s_bad ; }                            // RETURN 
  const auto aErr      = sumAllBinHeightErr ( histo ) ;
  if ( 0 > aErr       ) { return s_bad ; }                            // RETURN 
  //
  double error2  = std::pow((double)oErr,2) ;
  error2        += ( std::pow((double)aErr,2) *
                     std::pow((double)underflow,2) / std::pow((double)all,2) );
  error2        /= std::pow((double)all,2) ;
  //
  return std::sqrt ( error2 ) ;
}
// ============================================================================
/*  get number of entries in histogram up to the certain 
 *  bin (not-included)
 *  @attention underflow bin is included! 
 *  @param histo the pointer to the histogram 
 *  @param imax  the bin number (not included) 
 *  @param number of entries 
 */
// ============================================================================
long Gaudi::Utils::HistoStats::nEntries 
( const AIDA::IHistogram1D* histo , 
  const int                 imax  ) 
{
  if ( !histo ) { return s_long_bad ; }                            // RETURN 
  if ( 0 > imax   ) { return 0          ; }                            // RETURN 
  long result = histo -> binEntries( AIDA::IAxis::UNDERFLOW_BIN )  ;
  
  // get the exis 
  const auto& axis = histo -> axis () ;
  // number of bins 
  const auto nBins = axis.bins() ;
  // loop over all bins 
  for ( int i = 0 ; i < nBins ; ++i ) 
  { if ( i < imax ) { result += histo->binEntries ( i ) ; } }
  // 
  if ( nBins  < imax )
  { result += histo -> binEntries( AIDA::IAxis::OVERFLOW_BIN ) ; }
  //
  return result ;
}
// ============================================================================
/*  get number of entries in histogram form the certain 
 *  minimal bin up to the certain maximal bin (not-included)
 *  @param histo the pointer to the histogram 
 *  @param imin  the minimal bin number (included) 
 *  @param imax  the maximal bin number (not included) 
 *  @param number of entries 
 */
// ============================================================================
long Gaudi::Utils::HistoStats::nEntries 
( const AIDA::IHistogram1D* histo , 
  const int                 imin  ,         //     minimal bin number (included) 
  const int                 imax  )         // maximal bin number (not included) 
{
  if ( !histo   )     { return s_long_bad ; }                         // RETURN 
  if ( imin == imax ) { return 0          ; }                         // RETURN 
  if ( imax < imin  ) { return nEntries  ( histo , imax ,imin ) ; }   // RETURN 
  //
  long result = 0 ;
  if ( 0 > imin  ) 
  { result += histo -> binEntries( AIDA::IAxis::UNDERFLOW_BIN )  ; }
  // get the exis 
  const auto& axis = histo -> axis () ;
  // number of bins 
  const auto nBins = axis.bins() ;
  if ( nBins  < imin ) { return 0 ; }                                 // RETURN 
  // loop over all bins 
  for ( int i = 0 ; i < nBins ; ++i ) 
  { if ( imin <= i && i < imax ) { result += histo->binEntries ( i ) ; } }
  //  
  if ( nBins < imax )
  { result += histo -> binEntries( AIDA::IAxis::OVERFLOW_BIN ) ; }
  //
  return result ;                                                      // RETURN 
}
// ============================================================================
/*  get the fraction of entries in histogram up to the certain 
 *  bin (not-included)
 *  @attention underflow bin is included! 
 *  @param histo the pointer to the histogram 
 *  @param imax  the bin number (not included) 
 *  @param fraction of entries 
 */
// ============================================================================
double Gaudi::Utils::HistoStats::nEntriesFrac 
( const AIDA::IHistogram1D* histo , 
  const int                 imax  ) 
{
  if ( !histo       ) { return s_bad ; }                              // RETURN
  //
  const auto N = histo->allEntries () ;
  if ( 0 >= N       ) { return s_bad ; }                              // RETURN 
  const auto n = nEntries ( histo , imax ) ;
  if ( 0 >  n       ) { return s_bad ; }                              // RETURN 
  if ( n >  N       ) { return s_bad ; }                              // RETURN 
  //
  return (double)n / (double)N ;                                      // REUTRN 
}
// ============================================================================
/*  get fraction of entries in histogram form the certain 
 *  minimal bin up to the certain maximal bin (not-included)
 *  @param histo the pointer to the histogram 
 *  @param imin  the minimal bin number (included) 
 *  @param imax  the maximal bin number (not included) 
 *  @param fraction of entries 
 */
// ============================================================================
double Gaudi::Utils::HistoStats::nEntriesFrac 
( const AIDA::IHistogram1D* histo , 
  const int                 imin  ,        //     minimal bin number (included) 
  const int                 imax  )        // maximal bin number (not included) 
{
  if ( !histo       ) { return s_bad ; }                              // RETURN
  const auto N = histo->allEntries () ;
  if ( 0 >= N       ) { return s_bad ; }                              // RETURN 
  const auto n = nEntries ( histo , imin , imax ) ;
  if ( 0 >  n       ) { return s_bad ; }                              // RETURN 
  if ( n >  N       ) { return s_bad ; }                              // RETURN 
  //
  return (double)n / (double)N ;                                      // REUTRN 
}
// ============================================================================
/*  get the (binominal) error for the fraction of entries 
 *  in histogram up to the certain bin (not-included)
 *  @attention underflow bin is included! 
 *  @param histo the pointer to the histogram 
 *  @param imax  the bin number (not included) 
 *  @param error for the fraction of entries 
 */
// ============================================================================
double Gaudi::Utils::HistoStats::nEntriesFracErr 
( const AIDA::IHistogram1D* histo , 
  const int                 imax  ) 
{
  if ( !histo       ) { return s_bad ; }                              // RETURN
  //
  const auto N = histo->allEntries () ;
  if ( 0 >= N       ) { return s_bad ; }                              // RETURN 
  const auto n = nEntries ( histo , imax ) ;
  if ( 0 >  n       ) { return s_bad ; }                              // RETURN 
  if ( n >  N       ) { return s_bad ; }                              // RETURN 
  //
  const double _n1 = std::max ( (double)       n   , 1.0 ) ;
  const double _n2 = std::max ( (double) ( N - n ) , 1.0 ) ;
  //
  return std::sqrt ( _n1 * ( _n2 / N ) ) / N ;                             // RETURN 
}
// ============================================================================
/*  get the (binomial) error for the fraction of entries in histogram 
 *  from the certain minimal bin up to the certain maximal bin (not-included)
 *  @param histo the pointer to the histogram 
 *  @param imin  the minimal bin number (included) 
 *  @param imax  the maximal bin number (not included) 
 *  @param error for the fraction of entries 
 */
 // ============================================================================
double Gaudi::Utils::HistoStats::nEntriesFracErr 
( const AIDA::IHistogram1D* histo , 
  const int                 imin  ,        //     minimal bin number (included) 
  const int                 imax  )        // maximal bin number (not included) 
{
  if ( !histo       ) { return s_bad ; }                              // RETURN
  //
  const auto N = histo->allEntries () ;
  if ( 0 >= N       ) { return s_bad ; }                              // RETURN 
  const auto n = nEntries ( histo , imin , imax ) ;
  if ( 0 >  n       ) { return s_bad ; }                              // RETURN 
  if ( n >  N       ) { return s_bad ; }                              // RETURN 
  //
  const double _n1 = std::max ( (double)       n   , 1.0 ) ;
  const double _n2 = std::max ( (double) ( N - n ) , 1.0 ) ;
  //
  return std::sqrt ( _n1 * ( _n2 / N ) ) / N ;                             // RETURN 
}
// ============================================================================
// The END 
// ============================================================================
