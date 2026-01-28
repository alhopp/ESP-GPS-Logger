#pragma once

// -----------------------------------------------------------------------------
// Distance between two GPS points (local planar approximation)
//
// Computes the straight-line distance between two latitude/longitude points
// using a locally flat Earth approximation (sufficient for short distances).
//
// Parameters:
// - lambda1, phi1 : longitude / latitude of point 1
// - lambda2, phi2 : longitude / latitude of point 2
//
// Returns:
// - Distance in meters
// -----------------------------------------------------------------------------
double afstandPunten(double lambda1, double phi1,
                     double lambda2, double phi2);

                     
// -----------------------------------------------------------------------------
// Geometry helper: signed distance from a point to a line
//
// Calculates the perpendicular distance (in meters) from point P to the line AB.
// The sign of the returned value indicates which side of the line the point lies on.
//
// Parameters:
// - lambda0, phi0 : longitude / latitude of point P
// - lambda1, phi1 : longitude / latitude of line point A
// - lambda2, phi2 : longitude / latitude of line point B
//
// Returns:
// - Signed distance in meters
//   > 0  : point is on one side of the line
//   < 0  : point is on the opposite side
// -----------------------------------------------------------------------------
// float Dis_point_line(float long_act,float lat_act,float long_1,float lat_1,float long_2,float lat_2);
double Dis_point_line(double lambda0, double phi0,
                      double lambda1, double phi1,
                      double lambda2, double phi2);
