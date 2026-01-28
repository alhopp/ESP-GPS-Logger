#pragma once

// -----------------------------------------------------------------------------
// afstandPunten
//
// Straight-line distance between two GPS points (meters),
// using a local planar approximation (sufficient for short ranges).
//
// Inputs:
//   lambdaX = longitude (degrees)
//   phiX    = latitude  (degrees)
//
// Returns:
//   Distance in meters
// -----------------------------------------------------------------------------
double afstandPunten(double lambda1,double phi1,
                     double lambda2,double phi2);

// -----------------------------------------------------------------------------
// Dis_point_line
//
// Signed perpendicular distance (meters) from point P to line AB.
// Sign indicates which side of the line P lies on.
//
// Inputs:
//   (lambda0,phi0) = point P
//   (lambda1,phi1) = line point A
//   (lambda2,phi2) = line point B
//
// Returns:
//   Signed distance in meters
// -----------------------------------------------------------------------------
double Dis_point_line(double lambda0,double phi0,
                      double lambda1,double phi1,
                      double lambda2,double phi2);
