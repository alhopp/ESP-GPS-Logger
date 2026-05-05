#include "GPS/Geometry/gps_geometry.h"
#include <math.h>

// -----------------------------------------------------------------------------
// Local geometry constants (meters per degree)
// -----------------------------------------------------------------------------
#define DEGREE_TO_METER_LONG 111320.0
#define DEGREE_TO_METER_LAT  110540.0

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// -----------------------------------------------------------------------------
// Dis_point_line
//
// Signed perpendicular distance (meters) from point P to line AB.
// Sign indicates which side of the line P lies on.
//
// Uses a local planar approximation with latitude-based correction.
// -----------------------------------------------------------------------------
double Dis_point_line(double lambda0,double phi0,
                      double lambda1,double phi1,
                      double lambda2,double phi2)
{
  // Mean latitude for longitude scaling
  const double phi_m_rad=((phi1+phi2+phi0)/3.0)*M_PI/180.0;

  // Use A as local reference to improve numeric stability
  const double x1=0.0;
  const double y1=0.0;
  const double x2=(lambda2-lambda1)*cos(phi_m_rad)*DEGREE_TO_METER_LONG;
  const double y2=(phi2-phi1)*DEGREE_TO_METER_LAT;
  const double x0=(lambda0-lambda1)*cos(phi_m_rad)*DEGREE_TO_METER_LONG;
  const double y0=(phi0-phi1)*DEGREE_TO_METER_LAT;

  // Line coefficients: Ax + By + C = 0
  const double A=y1-y2;
  const double B=x2-x1;
  const double C=x1*y2-x2*y1;

  const double v=A*x0+B*y0+C;
  double d=fabs(v)/sqrt(A*A+B*B);
  if(v<0) d=-d;   // preserve side information

  return d;
}

// -----------------------------------------------------------------------------
// afstandPunten
//
// Distance (meters) between two GPS points using local planar approximation.
// -----------------------------------------------------------------------------
double afstandPunten(double lambda1,double phi1,
                     double lambda2,double phi2)
{
  const double phi_rad=((phi1+phi2)/2.0)*M_PI/180.0;
  const double m_per_deg_lon=DEGREE_TO_METER_LONG*cos(phi_rad);

  const double dx=(lambda2-lambda1)*m_per_deg_lon;
  const double dy=(phi2-phi1)*DEGREE_TO_METER_LAT;

  return sqrt(dx*dx+dy*dy);
}
