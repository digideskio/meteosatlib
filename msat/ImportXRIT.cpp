//---------------------------------------------------------------------------
//
//  File        :   ImportXRIT.cpp
//  Description :   Import MSG HRIT format
//  Project     :   Lamma 2004
//  Author      :   Graziano Giuliani (Lamma Regione Toscana)
//                  modified by Enrico Zini (ARPA Emilia Romagna)
//  Source      :   n/a
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

#include "msat/ImportXRIT.h"
#include "msat/ImportUtils.h"
#include "facts.h"
#include "proj/Geos.h"
#include <hrit/MSG_HRIT.h>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <glob.h>

#include <msat/Image.tcc>
#include <msat/Progress.h>

namespace std {
ostream& operator<<(ostream& out, const msat::HRITImageData::AreaMap& a)
{
	return out << a.x << "," << a.y << " dim: " << a.width << "," << a.height << " start: " << a.startcolumn << ", " << a.startline;
}
}

using namespace std;

#define PATH_SEPARATOR "/"
// For windows use #define PATH_SEPARATOR "\"

// Pad the string 'base' with trailing underscores to ensure it's at least
// final_len characters long
static std::string underscoreit(const std::string& base, int final_len)
{
	string res = base;
	res.resize(final_len, '_');
	return res;
}

namespace msat {

HRITImageData::~HRITImageData()
{
        if (calibration) delete[] calibration;
}

// #define tmprintf(...) fprintf(stderr, __VA_ARGS__)
#define tmprintf(...) do {} while(0)

MSG_SAMPLE HRITImageData::sample(size_t x, size_t y) const
{
	// Shift and cut the area according to cropping
	tmprintf("%zd,%zd -> ", x, y);

	size_t linestart = da.line_start(y);
	if (x < linestart) return 0;
	if (x + linestart >= da.columns) return 0;

	MSG_SAMPLE buf[da.columns];
	da.line_read(y, buf);
	return buf[x - linestart];
}

float HRITImageData::scaled(int column, int line) const
{
	// Get the wanted sample
	MSG_SAMPLE s = sample(column, line);

	// Perform calibration

	// To avoid spurious data, negative values after calibration are
	// converted to missing values
	if (s > 0 && calibration[s] >= 0)
		return calibration[s];
	else
		return missingValue;
}

int HRITImageData::scaledToInt(int column, int line) const
{
	if (!this->scalesToInt)
		throw std::runtime_error("Image samples cannot be scaled to int");
	return sample(column, line);
}

int HRITImageData::unscaledMissingValue() const
{
	if (!this->scalesToInt)
		throw std::runtime_error("Image samples cannot be scaled to int");
	// HRIT samples have 0 as missing value
	return 0;
}

std::auto_ptr<Image> importXRIT(const XRITImportOptions& opts)
{
	opts.ensureComplete();

	ProgressTask p("Reading HRIT from " + opts.toString());

  std::auto_ptr<Image> img(new Image);

	img->quality = opts.resolution[0];

	auto_ptr<HRITImageData> data(new HRITImageData);

	// Scan segment headers
	MSG_data PRO_data;
	MSG_data EPI_data;
	MSG_header header;
	data->da.scan(opts, PRO_data, EPI_data, header);

	// Image time
  struct tm *tmtime = PRO_data.prologue->image_acquisition.PlannedAquisitionTime.TrueRepeatCycleStart.get_timestruct( );
  img->year = tmtime->tm_year+1900;
	img->month = tmtime->tm_mon+1;
	img->day = tmtime->tm_mday;
	img->hour = tmtime->tm_hour;
	img->minute = tmtime->tm_min;

  // FIXME: and this? pds.sh = header[0].image_navigation->satellite_h;



#if 0
	data->LowerEastColumnActual = 1;
	data->LowerNorthLineActual = 8064;
	data->LowerWestColumnActual = 5568;
	//" LSLA: " << EPI_data.epilogue->product_stats.ActualL15CoverageHRV.LowerSouthLineActual
	data->UpperEastColumnActual = 2064;
	data->UpperSouthLineActual = 8065;
	data->UpperWestColumnActual = 7631;
#endif

#if 0
		cerr << " LSLA: " << EPI_data.epilogue->product_stats.ActualL15CoverageHRV.LowerSouthLineActual - 1
			   << " LNLA: " << data->LowerNorthLineActual
			   << " LECA: " << data->LowerEastColumnActual
			   << " LWCA: " << data->LowerWestColumnActual
			   << " USLA: " << data->UpperSouthLineActual
			   << " UNLA: " << EPI_data.epilogue->product_stats.ActualL15CoverageHRV.UpperNorthLineActual - 1
			   << " UECA: " << data->UpperEastColumnActual
			   << " UWCA: " << data->UpperWestColumnActual
			   << endl;
#endif

	// Fill in image information
        img->proj.reset(new proj::Geos(header.image_navigation->subsatellite_longitude, ORBIT_RADIUS));
        img->projWKT = facts::spaceviewWKT(header.image_navigation->subsatellite_longitude);
        img->spacecraft_id = facts::spacecraftIDFromHRIT(header.segment_id->spacecraft_id);
        img->channel_id = header.segment_id->spectral_channel_id;
        data->bpp = header.image_structure->number_of_bits_per_pixel;
	img->unit = facts::channelUnit(img->spacecraft_id, img->channel_id);

        if (data->da.hrv)
        {
                data->hrvNorth.x = 11136 - data->da.UpperWestColumnActual;
                data->hrvNorth.y = 11136 - data->da.UpperNorthLineActual;
                data->hrvNorth.width = data->da.UpperWestColumnActual - data->da.UpperEastColumnActual;
                data->hrvNorth.height = data->da.UpperNorthLineActual - data->da.UpperSouthLineActual;
                data->hrvNorth.startcolumn = 0;
                data->hrvNorth.startline = 0;

                data->hrvSouth.x = 11136 - data->da.LowerWestColumnActual;
                data->hrvSouth.y = 11136 - data->da.LowerNorthLineActual;
                data->hrvSouth.width = data->da.LowerWestColumnActual - data->da.LowerEastColumnActual;
                data->hrvSouth.height = data->da.LowerNorthLineActual - data->da.LowerSouthLineActual;
                data->hrvSouth.startcolumn = 0;
                data->hrvSouth.startline = data->hrvNorth.height - 1;

                // Since we are omitting the first (11136-UpperWestColumnActual) of the
                // rotated image, we need to shift the column offset accordingly
                // FIXME: don't we have a way to compute this from the HRIT data?
                //img->column_offset = 5568 - (11136 - data->UpperWestColumnActual - 1);
                img->column_offset = 5566;
                img->line_offset = 5566;
#if 0
                cerr << "COFF " << header.image_navigation->column_offset << endl;
                cerr << "LOFF " << header.image_navigation->line_offset << endl;
                cerr << "COFF " << header.image_navigation->COFF << endl;
                cerr << "LOFF " << header.image_navigation->LOFF << endl;
                cerr << "cCOFF " << img->column_offset << endl;
                cerr << "cLOFF " << img->line_offset << endl;
#endif
                data->columns = 11136;
                data->lines = 11136;
        } else {
                data->hrvNorth.x = 1856 - header.image_navigation->column_offset;
                data->hrvNorth.y = 1856 - header.image_navigation->line_offset;
                data->hrvNorth.width = data->da.UpperWestColumnActual - data->da.UpperEastColumnActual;
                data->hrvNorth.height = data->da.lines;
                data->hrvNorth.startcolumn = 0;
                data->hrvNorth.startline = 0;

                img->column_offset = 1856;
                img->line_offset = 1856;

                // img->column_offset = header.image_navigation->column_offset;
                // img->line_offset = header.image_navigation->line_offset;
                data->columns = 3712;
                data->lines = 3712;
        }

        // Horizontal scaling coefficient computed as (2**16)/delta, where delta is
        // size in micro Radians of one pixel
        img->column_res = abs(header.image_navigation->column_scaling_factor) * exp2(-16);
        // Vertical scaling coefficient computed as (2**16)/delta, where delta is the
        // size in micro Radians of one pixel
        img->line_res = abs(header.image_navigation->line_scaling_factor) * exp2(-16);

	double pixelSizeX, pixelSizeY;
	int column_offset, line_offset, x0 = 0, y0 = 0;
	if (data->da.hrv)
	{
		pixelSizeX = 1000 * PRO_data.prologue->image_description.ReferenceGridHRV.ColumnDirGridStep;
		pixelSizeY = 1000 * PRO_data.prologue->image_description.ReferenceGridHRV.LineDirGridStep;

		// Since we are omitting the first (11136-UpperWestColumnActual) of the
		// rotated image, we need to shift the column offset accordingly
		// FIXME: don't we have a way to compute this from the HRIT data?
		//img->column_offset = 5568 - (11136 - data->UpperWestColumnActual - 1);
		column_offset = 5568;
		line_offset = 5568;
	} else {
		pixelSizeX = 1000 * PRO_data.prologue->image_description.ReferenceGridVIS_IR.ColumnDirGridStep;
		pixelSizeY = 1000 * PRO_data.prologue->image_description.ReferenceGridVIS_IR.LineDirGridStep;

		column_offset = 1856;
		line_offset = 1856;
	}
	//img->geotransform[0] = -(band->column_offset - band->x0) * band->column_res;
	//img->geotransform[3] = -(band->line_offset   - band->y0) * band->line_res;
	img->geotransform[0] = -(column_offset - x0) * fabs(pixelSizeX);
	img->geotransform[3] = (line_offset   - y0) * fabs(pixelSizeY);
	//img->geotransform[1] = band->column_res;
	//img->geotransform[5] = band->line_res;
	img->geotransform[1] = fabs(pixelSizeX);
	img->geotransform[5] = -fabs(pixelSizeY);
	img->geotransform[2] = 0.0;
	img->geotransform[4] = 0.0;

	img->x0 = 0;
	img->y0 = 0;




	//cerr << "HRVNORTH " << data->hrvNorth << endl;
	//cerr << "HRVSOUTH " << data->hrvSouth << endl;

	/*
	// Special handling for HRV images
	if (data->hrv)
	{
		// Widen the image to include both image parts, placed in their right
		// position
		data->origColumns += data->UpperEastColumnActual + 1;
	}
	*/

	//data->columns = data->origColumns;
	//data->lines = data->origLines;

  // Get calibration values
  data->calibration = PRO_data.prologue->radiometric_proc.get_calibration(img->channel_id, data->bpp);

	// Get offset and slope
	double slope;
	double offset;
  PRO_data.prologue->radiometric_proc.get_slope_offset(img->channel_id, slope, offset, data->scalesToInt);
  data->slope = slope;
  data->offset = offset;

	img->setData(data.release());

  return img;
}

class XRITImageImporter : public ImageImporter
{
	XRITImportOptions opts;

public:
	XRITImageImporter(const std::string& filename) : opts(filename) {}

	virtual void read(ImageConsumer& output)
	{
		std::auto_ptr<Image> img = importXRIT(opts);
		img->defaultFilename = util::satelliteSingleImageFilename(*img);
		img->shortName = util::satelliteSingleImageShortName(*img);
		img->addToHistory("Imported from HRIT " + opts.resolution + ' ' + opts.productid1 + ' ' + opts.productid2 + ' ' + opts.timing);
		output.processImage(img);
	}
};

std::auto_ptr<ImageImporter> createXRITImporter(const std::string& filename)
{
	return std::auto_ptr<ImageImporter>(new XRITImageImporter(filename));
}

}

// vim:set ts=2 sw=2:
