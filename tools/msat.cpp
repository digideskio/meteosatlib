//---------------------------------------------------------------------------
//
//  File        :   NetCDF2Grib.cpp
//  Description :   Convert NetCDF format in Grib format
//  Project     :   ?
//  Author      :   Enrico Zini (for ARPA SIM Emilia Romagna)
//  Source      :   derived from SAFH5CT2NetCDF.cpp by Le Duc, as modified by
//                  Francesca Di Giuseppe and from XRIT2Grib.cpp by Graziano
//                  Giuliani (Lamma Regione Toscana)
//  RCS ID      :   $Id$
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//  
//---------------------------------------------------------------------------

#include <config.h>

#include <msat/ImportGRIB.h>
#include <msat/ImportSAFH5.h>
#include <msat/ImportNetCDF.h>
#include <msat/ImportNetCDF24.h>
#include <msat/ImportXRIT.h>
#include <msat/ExportGRIB.h>
#include <msat/ExportNetCDF.h>
#include <msat/ExportNetCDF24.h>
#include <msat/Progress.h>

#ifdef HAVE_MAGICKPP
#include <msat/ExportImage.h>
#endif

#include <set>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

#include <getopt.h>

using namespace std;
using namespace msat;

void do_help(const char* argv0, ostream& out)
{
  out << "Usage: " << argv0 << " [options] file(s)..." << endl << endl
      << "Convert meteosat image files from and to various formats." << endl << endl
      << "Formats supported are:" << endl
#ifdef HAVE_NETCDF
      << "  NetCDF   Import/Export" << endl
      << "  NetCDF24 Import/Export" << endl
#endif
      << "  Grib     Import/Export" << endl
#ifdef HAVE_HDF5
      << "  SAFH5    Import only" << endl
#endif
#ifdef HAVE_HRIT
      << "  XRIT     Import only" << endl
#endif
      << "Options are:" << endl
      << "  --help           Print this help message" << endl
      << "  -q,--quiet       Work silently" << endl
      << "  --view           View the contents of a file" << endl
      << "  --dump           View the contents of a file, including the pixel data" << endl
      << "  --grib           Convert to GRIB" << endl
#ifdef HAVE_HDF5
      << "  --netcdf         Convert to NetCDF" << endl
      << "  --netcdf24       Convert to NetCDF" << endl
#endif
#ifdef HAVE_MAGICKPP
      << "  --jpg            Convert to JPEG" << endl
      << "  --display        Display the image on a X11 window" << endl
#endif
      << "  --area='x,dx,y,dy' Crop the source image(s) to the given area" << endl
      << "  --Area='latmin,latmax,lonmin,lonmax' Crop the source image(s) to the given coordinates" << endl
      << "  --around='lat,lon,lath,lonw' Create an image centered at the given location and with the given width and height" << endl;
}
void usage(char *pname)
{
  std::cout << pname << ": Convert meteosat image files from and to various formats."
	    << std::endl;
  std::cout << std::endl << "Usage:" << std::endl << "\t"
            << pname << "<output option> file(s)..."
            << std::endl << std::endl
            << "Examples: " << std::endl << "\t" << pname
            << "  " << pname << " --grib Data0.nc SAFNWC_MSG1_CT___04356_051_EUROPE______.h5 hritdir/H:MSG1:HRV:200605031200"
            << std::endl;
  return;
}

enum Action { VIEW, DUMP, GRIB
#ifdef HAVE_NETCDF
	, NETCDF, NETCDF24
#endif
#ifdef HAVE_MAGICKPP
	, JPG, DISPLAY
#endif
};

/*
 * Create an importer for the given file, auto-detecting the file type.
 * 
 * If no supported file type could be detected, returns an empty auto_ptr.
 */
std::auto_ptr<ImageImporter> getImporter(const std::string& filename)
{
	if (isGrib(filename))
		return createGribImporter(filename);
#ifdef HAVE_NETCDF
	if (isNetCDF(filename))
		return createNetCDFImporter(filename);
	if (isNetCDF24(filename))
		return createNetCDF24Importer(filename);
#endif
#ifdef HAVE_HDF5
	if (isSAFH5(filename))
		return createSAFH5Importer(filename);
#endif
#ifdef HAVE_HRIT
	if (isXRIT(filename))
		return createXRITImporter(filename);
#endif
	return std::auto_ptr<ImageImporter>();
}

/*
 * Created an exported based on the given action
 */
std::auto_ptr<ImageConsumer> getExporter(Action action)
{
	switch (action)
	{
		case VIEW: return createImageDumper(false);
		case DUMP: return createImageDumper(true);
		case GRIB: return createGribExporter();
#ifdef HAVE_NETCDF
		case NETCDF: return createNetCDFExporter();
		case NETCDF24: return createNetCDF24Exporter();
#endif
#ifdef HAVE_MAGICKPP
		case JPG: return createImageExporter("jpg");
		case DISPLAY: return createImageDisplayer();
#endif
	}
	return auto_ptr<ImageConsumer>(0);
}

/* ************************************************************************* */
/* Reads Data Images, performs calibration and store output in NetCDF format */
/* ************************************************************************* */

int main( int argc, char* argv[] )
{
	// Defaults to view
  Action action = VIEW;
	int ax = 0, ay = 0, aw = 0, ah = 0;
	double latmin = 1000, latmax = 1000, lonmin = 1000, lonmax = 1000;
	bool quiet = false;

  static struct option longopts[] = {
    { "help",	0, NULL, 'H' },
    { "quiet", 0, NULL, 'q' },
		{ "view",	0, NULL, 'V' },
		{ "dump",	0, NULL, 'D' },
		{ "grib",	0, NULL, 'G' },
#ifdef HAVE_NETCDF
		{ "netcdf",	0, NULL, 'N' },
		{ "netcdf24",	0, NULL, '2' },
#endif
#ifdef HAVE_MAGICKPP
		{ "jpg",	0, NULL, 'j' },
		{ "display",	0, NULL, 'd' },
#endif
		{ "area", 1, 0, 'a' },
		{ "Area", 1, 0, 'A' },
		{ "around", 1, 0, 'C' },
		{ 0, 0, 0, 0 },
  };

  bool done = false;
  while (!done) {
    int c = getopt_long(argc, argv, "q", longopts, (int*)0);
    switch (c) {
      case 'H':	// --help
				do_help(argv[0], cout);
				return 0;
			case 'q': // -q,--quiet
				quiet = true;
				break;
      case 'V': // --view
				action = VIEW;
				break;
      case 'D': // --dump
				action = DUMP;
				break;
      case 'G': // --grib
				action = GRIB;
				break;
#ifdef HAVE_NETCDF
      case 'N': // --netcdf
				action = NETCDF;
				break;
      case '2': // --netcdf24
				action = NETCDF24;
				break;
#endif
#ifdef HAVE_MAGICKPP
			case 'j': // --jpg
				action = JPG;
				break;
			case 'd': // --display
				action = DISPLAY;
				break;
#endif
			case 'a':
				if (sscanf(optarg, "%d,%d,%d,%d", &ax,&aw,&ay,&ah) != 4)
				{
					cerr << "Area value should be in the format x,dx,y,dy" << endl;
					do_help(argv[0], cerr);
					return 1;
				}
				break;
			case 'A':
				if (sscanf(optarg, "%lf,%lf,%lf,%lf", &latmin,&latmax,&lonmin,&lonmax) != 4)
				{
					cerr << "Area value should be in the format latmin,latmax,lonmin,lonmax" << endl;
					do_help(argv[0], cerr);
					return 1;
				}
				break;
			case 'C': {
				double lat, lon, h, w;
				if (sscanf(optarg, "%lf,%lf,%lf,%lf", &lat,&lon,&h,&w) != 4)
				{
					cerr << "around value should be in the format lat,lon,lath,lonw" << endl;
					do_help(argv[0], cerr);
					return 1;
				}
				latmin = lat - h/2;
				lonmin = lon - w/2;
				latmax = lat + h/2;
				lonmax = lon + w/2;
				break;
			}
      case -1:
				done = true;
				break;
      default:
				cerr << "Error parsing commandline." << endl;
				do_help(argv[0], cerr);
				return 1;
    }
  }

  if (optind == argc)
  {
    do_help(argv[0], cerr);
    return 1;
  }

	if (!quiet)
		Progress::get().setHandler(new StreamProgressHandler(cerr));

  try
  {
		for (int i = optind; i < argc; ++i)
		{
			std::auto_ptr<ImageImporter> importer = getImporter(argv[i]);
			if (!importer.get())
			{
				cerr << "No importer found for " << argv[i] << ": ignoring." << endl;
				continue;
			}
			importer->cropX = ax;
			importer->cropY = ay;
			importer->cropWidth = aw;
			importer->cropHeight = ah;
			importer->cropLatMin = latmin;
			importer->cropLatMax = latmax;
			importer->cropLonMin = lonmin;
			importer->cropLonMax = lonmax;
			std::auto_ptr<ImageConsumer> consumer = getExporter(action);
			importer->read(*consumer);
		}
  }
  catch (std::exception& e)
  {
    cerr << e.what() << endl;
    return 1;
  }
  
  return(0);
}

// vim:set ts=2 sw=2: