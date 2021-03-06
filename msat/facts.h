#ifndef MSAT_FACTS_H
#define MSAT_FACTS_H

#include <string>
#include <math.h>

namespace msat {
namespace facts {

// Equatorial raius of the earth (Km.)
#define EARTH_RADIUS 6378.1370	// R (from http://home.online.no/~sigurdhu/WGS84_Eng.html)
//#define EARTH_RADIUS 6378.1350	// R (from http://en.wikipedia.org/wiki/Earth_radius)
// More at http://en.wikipedia.org/wiki/Figure_of_the_Earth

// Orbit_Radius (Km.)
#define ORBIT_RADIUS 42164.0	// orbit
//#define ORBIT_RADIUS 42155.0	// orbit (from http://www.eumetsat.int/Home/Main/What_We_Do/Satellites/Orbits/SP_1119354856486?l=en)

// see the HRIT driver for gdal for how this is computed
#define ORBIT_RADIUS_FOR_GDAL 35785831

/// Camera height in units of earth radius
#define SEVIRI_CAMERA_H    (ORBIT_RADIUS / EARTH_RADIUS)

#define SEVIRI_ORIENTATION 180.0

#define METEOSAT_IMAGE_NCOLUMNS 3712
#define METEOSAT_IMAGE_NLINES 3712

#define METEOSAT_IMAGE_NCOLUMNS_HRV 11136
#define METEOSAT_IMAGE_NLINES_HRV 11136

#define METEOSAT_PIXELSIZE_X 3000.40307617188
#define METEOSAT_PIXELSIZE_Y 3000.40307617188
#define METEOSAT_PIXELSIZE_X_HRV 1000.13433837891
#define METEOSAT_PIXELSIZE_Y_HRV 1000.13433837891

/// Pixel size in "metres" from column resolution in microradians
double pixelHSizeFromCFAC(double cfac);

/// Pixel size in "metres" from line resolution in microradians
double pixelVSizeFromLFAC(double lfac);

/// Inverse of pixelHSizeFromCFAC
double CFACFromPixelHSize(double psx);

/// Inverse of pixelVSizeFromLFAC
double LFACFromPixelVSize(double psx);

/// Earth dimension scanned by Seviri in the X direction
int seviriDXFromColumnRes(double column_res);

/// Earth dimension scanned by Seviri in the Y direction
int seviriDYFromLineRes(double line_res);

/// Set the column factor from a seviri DX value
double columnResFromSeviriDX(int seviriDX);

/// Set the column factor from a seviri DY value
double lineResFromSeviriDY(int seviriDY);

/// Earth dimension scanned by Seviri in the X direction
int seviriDXFromCFAC(double column_res);

/// Earth dimension scanned by Seviri in the Y direction
int seviriDYFromLFAC(double line_res);

/// Set the column factor from a seviri DX value
double CFACFromSeviriDX(int seviriDX);

/// Set the column factor from a seviri DY value
double LFACFromSeviriDY(int seviriDY);

int seviriDXFromPixelHSize(double psx);
int seviriDYFromPixelVSize(double psy);
double pixelHSizeFromSeviriDX(int dx);
double pixelVSizeFromSeviriDY(int dy);

/// Convert the HRIT spacecraft ID to the ID as in WMO Common code table C-5
int spacecraftIDFromHRIT(int id);

/// Convert the spacecraft ID as in WMO Common code table C-5 to the value
/// used by HRIT
int spacecraftIDToHRIT(int id);

/// Get the spacecraft name from the WMO Common code table C-5 satellite ID
const char* spacecraftName(int hritID);

/// Get the spacecraft id by name from the WMO Common code table C-5 satellite ID
int spacecraftID(const std::string& name);

/// Get the sensor name from the given WMO Common code table C-5 satellite ID
const char* sensorName(int spacecraftID);

/// Get the channel name from the given WMO Common code table C-5 satellite ID and channel ID
const char* channelName(int spacecraftID, int channelID);

/// Get the measure unit name from the given WMO Common code table C-5 satellite ID and channel ID
const char* channelUnit(int spacecraftID, int channelID);

/// Get the data level string "1", "1.5", "2", "3"... for the given channel
const char* channelLevel(int spacecraftID, int channelID);

/// Get the central wavelength (in µm) for the given channel
double channel_central_wavelength(int spacecraftID, int channelID);

/// Get the central wave number (m**-1) for the given channel
double channel_central_wave_number(int spacecraftID, int channelID);

/// Get the channel number given the central wavelength in µm
int channel_from_central_wavelength(int spacecraftID, double wavelength);

/// Get the channel number given the central wave number (m**-1)
int channel_from_central_wave_number(int spacecraftID, double wave_number);

/// Get the default missing value that can be used for the given channel
double defaultPackedMissing(int channel);

/// Get the default missing value that can be used for the given channel
double defaultScaledMissing(int channel);

/// Number of significant digits for scaled values in the given channel
int significantDigitsForChannel(int channel);

/// Compute the satellite zenith angle at a given location
double sat_za(double lat, double lon);

/// Julian day from a gregorian calendar date
int jday(int yr, int month, int day);

/**
 * Compute the cosine of the solar zenith angle at a given time and location
 *
 * Time is given using a normal gregorian calendar date in UTC
 */
double cos_sol_za(int yr, int month, int day, int hour, int minute,
           double lat, double lon);

/**
 * Compute the cosine of the solar zenith angle at a given time and location
 *
 * Time is given using UTC time and julian day
 *
 * See http://en.wikipedia.org/wiki/Solar_zenith_angle
 */
double cos_sol_za(int jday, int hour, int minute, double dlat, double dlon);

/**
 * Compute the cosine of the solar zenith angle at a given time and location
 *
 * Time is given using UTC fractional hour and julian day
 *
 * See http://en.wikipedia.org/wiki/Solar_zenith_angle
 */
double cos_sol_za(int jday, double hour, double dlat, double dlon);

}
}

#endif
