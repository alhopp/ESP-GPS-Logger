

#include "GPS/gps_geometry.h"
#include <math.h>



double Dis_point_line(double lambda0, double phi0,
                      double lambda1, double phi1,
                      double lambda2, double phi2
                      ) {
                        // Schaalfactoren
    #define DEGREE_TO_METER_LONG 111320.0
    #define DEGREE_TO_METER_LAT 110540.0
    //#define DEGREE_TO_METER_LAT 111132.92
   // #define DEGREE_TO_METER_LONG_AT_EQUATOR 111319.49
    // Gemiddelde breedtegraad voor cosinuscorrectie
    double phi_m = (phi1 + phi2 + phi0) / 3.0;
    double phi_m_rad = phi_m * M_PI / 180.0;
    // Referentiepunt (bijvoorbeeld punt A)
    double lambda_ref = lambda1;
    double phi_ref = phi1;
    // Omrekenen naar lokale vlakke coördinaten (in meters)
    double x1 = (lambda1 - lambda_ref) * cos(phi_m_rad) * DEGREE_TO_METER_LONG;
    double y1 = (phi1 - phi_ref) * DEGREE_TO_METER_LAT;
    double x2 = (lambda2 - lambda_ref) * cos(phi_m_rad) * DEGREE_TO_METER_LONG;
    double y2 = (phi2 - phi_ref) * DEGREE_TO_METER_LAT;
    double x0 = (lambda0 - lambda_ref) * cos(phi_m_rad) * DEGREE_TO_METER_LONG;
    double y0 = (phi0 - phi_ref) * DEGREE_TO_METER_LAT;
    // Coëfficiënten van de lijn
    double A = y1 - y2;
    double B = x2 - x1;
    double C = x1 * y2 - x2 * y1;
    // Zijdedetectie
    double waarde = A * x0 + B * y0 + C;
    // Afstand in meters
    double afstand = fabs(waarde) / sqrt(A * A + B * B);
    if(waarde<0)afstand = -afstand;
    return afstand;
}



/**
 * Bereken de afstand tussen twee GPS-punten met een lokale vlakke benadering.
 *
 * @param lambda1: lengtegraad punt 1 (in graden)
 * @param phi1: breedtegraad punt 1 (in graden)
 * @param lambda2: lengtegraad punt 2 (in graden)
 * @param phi2: breedtegraad punt 2 (in graden)
 * @return afstand in meters
 */
double afstandPunten(double lambda1, double phi1, double lambda2, double phi2) {
    // Converteer breedtegraad naar radialen voor de correctie van de lengtegraad
    double phi_rad = (phi1 + phi2) / 2.0 * M_PI / 180.0;
    // Correctiefactor voor lengtegraad (cosinus van gemiddelde breedtegraad)
    double meter_per_degree_long = DEGREE_TO_METER_LONG * cos(phi_rad);
    // Verschillen in graden
    double d_lambda = lambda2 - lambda1;
    double d_phi = phi2 - phi1;
    // Omrekenen naar meters
    double dx = d_lambda * meter_per_degree_long;
    double dy = d_phi * DEGREE_TO_METER_LAT;
    // Pythagoras
    double afstand = sqrt(dx * dx + dy * dy);
    return afstand;
}


