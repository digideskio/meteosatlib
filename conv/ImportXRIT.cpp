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

#include <conv/ImportXRIT.h>
#include <proj/const.h>
#include <proj/Geos.h>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <hrit/MSG_HRIT.h>
#include <glob.h>

#include <conv/Image.tcc>
#include <conv/Progress.h>

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

// dir/res:prodid1:prodid2:time
bool isXRIT(const std::string& filename)
{
	// check that it contains at least 3 ':' signs
	size_t pos = 0;
	for (int i = 0; i < 3; ++i, ++pos)
		if ((pos = filename.find(':', pos)) == string::npos)
			return false;
	return true;
}

// dir/res:prodid1:prodid2:time
XRITImportOptions::XRITImportOptions(const std::string& filename)
{
	size_t beg;
	size_t end = filename.rfind('/');
	if (end == string::npos)
	{
		directory = ".";
		beg = 0;
	}
	else
	{
		directory = filename.substr(0, end);
		if (directory.size() == 0) directory = "/";
		beg = end + 1;
	}

	if ((end = filename.find(':', beg)) == string::npos)
		throw std::runtime_error("XRIT name " + filename + " is not in the form [directory/]resolution:productid1:productid2:datetime");
	resolution = filename.substr(beg, end-beg);

	beg = end + 1;
	if ((end = filename.find(':', beg)) == string::npos)
		throw std::runtime_error("XRIT name " + filename + " is not in the form [directory/]resolution:productid1:productid2:datetime");
	productid1 = filename.substr(beg, end-beg);

	beg = end + 1;
	if ((end = filename.find(':', beg)) == string::npos)
		throw std::runtime_error("XRIT name " + filename + " is not in the form [directory/]resolution:productid1:productid2:datetime");
	productid2 = filename.substr(beg, end-beg);

	beg = end + 1;
	timing = filename.substr(beg);
}

void XRITImportOptions::ensureComplete() const
{
	if (directory.empty())
		throw std::runtime_error("source directory is missing");
	if (resolution.empty())
		throw std::runtime_error("resolution is missing");
	if (productid1.empty())
		throw std::runtime_error("first product ID is missing");
	if (productid2.empty())
		throw std::runtime_error("second product ID is missing");
	if (timing.empty())
		throw std::runtime_error("timing is missing");
}

std::string XRITImportOptions::prologueFile() const
{
  std::string filename = directory
		       + PATH_SEPARATOR
					 + resolution
		       + "-???-??????-"
					 + underscoreit(productid1, 12) + "-"
					 + underscoreit("_", 9) + "-"
					 + "PRO______-"
					 + timing
					 + "-__";

  glob_t globbuf;
  globbuf.gl_offs = 1;

  if ((glob(filename.c_str(), GLOB_DOOFFS, NULL, &globbuf)) != 0)
    throw std::runtime_error("No such file(s)");

  if (globbuf.gl_pathc > 1)
    throw std::runtime_error("Non univoque prologue file.... Do not trust calibration.");

	string res(globbuf.gl_pathv[1]);
  globfree(&globbuf);
  return res;
}

std::string XRITImportOptions::epilogueFile() const
{
  std::string filename = directory
		       + PATH_SEPARATOR
					 + resolution
		       + "-???-??????-"
					 + underscoreit(productid1, 12) + "-"
					 + underscoreit("_", 9) + "-"
					 + "EPI______-"
					 + timing
					 + "-__";

  glob_t globbuf;
  globbuf.gl_offs = 1;

  if ((glob(filename.c_str(), GLOB_DOOFFS, NULL, &globbuf)) != 0)
    throw std::runtime_error("No such file(s)");

  if (globbuf.gl_pathc > 1)
    throw std::runtime_error("Non univoque prologue file.... Do not trust calibration.");

	string res(globbuf.gl_pathv[1]);
  globfree(&globbuf);
  return res;
}

std::vector<std::string> XRITImportOptions::segmentFiles() const
{
  string filename = directory
					 + PATH_SEPARATOR
					 + resolution
           + "-???-??????-"
           + underscoreit(productid1, 12) + "-"
           + underscoreit(productid2, 9) + "-"
           + "0?????___" + "-"
           + timing + "-" + "?_";

  glob_t globbuf;
  globbuf.gl_offs = 1;

  if ((glob(filename.c_str( ), GLOB_DOOFFS, NULL, &globbuf)) != 0)
    throw std::runtime_error("No such file(s)");

	std::vector<std::string> res;
	for (size_t i = 0; i < globbuf.gl_pathc; ++i)
		res.push_back(globbuf.gl_pathv[i+1]);
  globfree(&globbuf);
	return res;
}

std::string XRITImportOptions::toString() const
{
	std::stringstream str;
	str <<        "dir: " << directory
		  <<       " res: " << resolution
			<<     " prod1: " << productid1
			<<     " prod2: " << productid2
			<<      " time: " << timing;
	return str.str();
}

/**
 * ImageData implementation that reads data directly from the HRIT file on
 * disk, keeping in memory not more than one segment at a time
 */
struct HRITImageData : public ImageData
{
	/// HRV parameters used to locate the two image parts
	int LowerEastColumnActual;
	int LowerNorthLineActual;
	int LowerWestColumnActual;
	int UpperEastColumnActual;
	int UpperSouthLineActual;
	int UpperWestColumnActual;

	/// Number of pixels in every segment
	size_t npixperseg;

	/// True if the image needs to be swapped horizontally
	bool swapX;

	/// True if the image needs to be swapped vertically
	bool swapY;

	/// True if the image is an HRV image divided in two parts
	bool hrv;

	/// Pathnames of the segment files, indexed with their index
	std::vector<string> segnames;

	/// Cached segment
	mutable MSG_data* m_segment;

	/// Index of the currently cached segment
	mutable int m_segment_idx;

	/// Calibration vector
	float* calibration;

	/// Number of columns in the uncropped image
	size_t origColumns;

	/// Number of lines in the uncropped image
	size_t origLines;

	/// Cropping edges
	int cropX, cropY;

	HRITImageData() : npixperseg(0), m_segment(0), m_segment_idx(-1), calibration(0), cropX(0), cropY(0) {}
  virtual ~HRITImageData()
	{
		if (m_segment) delete m_segment;
		if (calibration) delete[] calibration;
	}

	/**
	 * Return the MSG_data corresponding to the segment with the given index.
	 *
	 * The pointer could be invalidated by another call to segment()
	 */
	MSG_data* segment(size_t idx) const
	{
		if ((int)idx != m_segment_idx)
		{
			// Delete old segment if any
			if (m_segment) delete m_segment;
			m_segment = 0;
			m_segment_idx = 0;
			if (idx >= segnames.size()) return 0;
			if (segnames[idx].empty()) return m_segment;

			ProgressTask p("Reading segment " + segnames[idx]);
			std::ifstream hrit(segnames[idx].c_str(), (std::ios::binary | std::ios::in));
			if (hrit.fail())
				throw std::runtime_error("Cannot open input hrit segment " + segnames[idx]);
			MSG_header header;
			header.read_from(hrit);
			if (header.segment_id->data_field_format == MSG_NO_FORMAT)
				throw std::runtime_error("Product dumped in binary format.");
			m_segment = new MSG_data;
			m_segment->read_from(hrit, header);
			hrit.close( );

			m_segment_idx = idx;
		}
		return m_segment;
	}

	/// Get an unscaled sample from the given coordinates in the normalised image
	MSG_SAMPLE sample(size_t x, size_t y) const
	{
		// Shift and cut the area according to cropping
		if (x >= columns) return 0;
		if (y >= lines) return 0;
		x += cropX;
		y += cropY;

		// Rotate if needed
		if (swapX) x = origColumns - x - 1;
		if (swapY) y = origLines - y - 1;

		// Absolute position in image data
		size_t pos;
		if (hrv)
		{
			// Check if we are in the shifted HRV upper area
			// FIXME: the '-1' should not be there, but if I take it out I see one
			//        line badly offset
			if (y >= UpperSouthLineActual - 1)
			{
				if (x < UpperEastColumnActual)
					return 0;
				if (x > UpperWestColumnActual)
					return 0;
				x -= UpperEastColumnActual;
			} else {
				if (x < LowerEastColumnActual)
					return 0;
				if (x > LowerWestColumnActual)
					return 0;
				x -= LowerEastColumnActual;
			}
			pos = y * (origColumns - UpperEastColumnActual - 1) + x;
		} else
			pos = y * origColumns + x;

		// Segment number where is the pixel
		size_t segno = pos / npixperseg;
		MSG_data* d = segment(segno);
		if (d == 0) return 0;

		// Offset of the pixel in the segment
		size_t segoff = pos - (segno * npixperseg);
		return d->image->data[segoff];
	}

  /// Image sample as physical value (already scaled with slope and offset)
  float scaled(int column, int line) const
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

	/// Image sample scaled to int using slope and offset.
	/// The function throws if scalesToInt is false.
	virtual int scaledToInt(int column, int line) const
	{
		if (!this->scalesToInt)
			throw std::runtime_error("Image samples cannot be scaled to int");
		return sample(column, line);
	}

	/// Value used to represent a missing value in the unscaled int
	/// data, if available
	virtual int unscaledMissingValue() const
	{
		if (!this->scalesToInt)
			throw std::runtime_error("Image samples cannot be scaled to int");
		// HRIT samples have 0 as missing value
		return 0;
	}

	/// Crop the image to the given rectangle
	virtual void crop(int x, int y, int width, int height)
	{
		// Virtual cropping: we just limit the area of the image we read
		cropX += x;
		cropY += y;
		columns = width;
		lines = height;
	}
};


std::auto_ptr<Image> importXRIT(const XRITImportOptions& opts)
{
	opts.ensureComplete();

	ProgressTask p("Reading HRIT from " + opts.toString());

  std::auto_ptr<Image> img(new Image);

	img->quality = opts.resolution[0];

	auto_ptr<HRITImageData> data(new HRITImageData);

	// Read prologue
  MSG_header PRO_head;
  MSG_data PRO_data;
	p.activity("Reading prologue " + opts.prologueFile());
	std::ifstream hrit(opts.prologueFile().c_str(), (std::ios::binary | std::ios::in));
	if (hrit.fail())
		throw std::runtime_error("Cannot open input hrit file " + opts.prologueFile());
	PRO_head.read_from(hrit);
	PRO_data.read_from(hrit, PRO_head);
	hrit.close();

	// Image time
  struct tm *tmtime = PRO_data.prologue->image_acquisition.PlannedAquisitionTime.TrueRepeatCycleStart.get_timestruct( );
  img->year = tmtime->tm_year+1900;
	img->month = tmtime->tm_mon+1;
	img->day = tmtime->tm_mday;
	img->hour = tmtime->tm_hour;
	img->minute = tmtime->tm_min;

  // FIXME: and this? pds.sh = header[0].image_navigation->satellite_h;


	// Read epilogue
	MSG_header EPI_head;
	MSG_data EPI_data;
	p.activity("Reading epilogue " + opts.epilogueFile());
	hrit.open(opts.epilogueFile().c_str(), (std::ios::binary | std::ios::in));
	if (hrit.fail())
		throw std::runtime_error("Cannot open input hrit file " + opts.prologueFile());
	EPI_head.read_from(hrit);
	EPI_data.read_from(hrit, EPI_head);
	hrit.close();

	// Subtracting one because they start from 1 instead of 0
	data->LowerEastColumnActual = EPI_data.epilogue->product_stats.ActualL15CoverageHRV.LowerEastColumnActual - 1;
	data->LowerNorthLineActual = EPI_data.epilogue->product_stats.ActualL15CoverageHRV.LowerNorthLineActual - 1;
	data->LowerWestColumnActual = EPI_data.epilogue->product_stats.ActualL15CoverageHRV.LowerWestColumnActual - 1;
	//" LSLA: " << EPI_data.epilogue->product_stats.ActualL15CoverageHRV.LowerSouthLineActual
	data->UpperEastColumnActual = EPI_data.epilogue->product_stats.ActualL15CoverageHRV.UpperEastColumnActual - 1;
	data->UpperSouthLineActual = EPI_data.epilogue->product_stats.ActualL15CoverageHRV.UpperSouthLineActual - 1;
	data->UpperWestColumnActual = EPI_data.epilogue->product_stats.ActualL15CoverageHRV.UpperWestColumnActual - 1;
	//" UNLA: " << EPI_data.epilogue->product_stats.ActualL15CoverageHRV.UpperNorthLineActual

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

	// Sort the segment names by their index
	vector<string> segfiles = opts.segmentFiles();
	for (vector<string>::const_iterator i = segfiles.begin();
				i != segfiles.end(); ++i)
	{
		p.activity("Scanning segment " + *i);
		std::ifstream hrit(i->c_str(), (std::ios::binary | std::ios::in));
		if (hrit.fail())
			throw std::runtime_error("Cannot open input hrit segment " + *i);
		MSG_header header;
		header.read_from(hrit);
		hrit.close( );

		if (header.segment_id->data_field_format == MSG_NO_FORMAT)
			throw std::runtime_error("Product dumped in binary format.");

		// Read common info just once from a random segment
		if (data->npixperseg == 0)
		{
			// Decoding information
			int totalsegs = header.segment_id->planned_end_segment_sequence_number;
			int seglines = header.image_structure->number_of_lines;
			data->origColumns = header.image_structure->number_of_columns;
			data->origLines = seglines * totalsegs;
			data->npixperseg = data->origColumns * seglines;

			// Image metadata
			img->proj.reset(new proj::Geos(header.image_navigation->subsatellite_longitude, ORBIT_RADIUS));
			img->channel_id = header.segment_id->spectral_channel_id;
			data->hrv = img->channel_id == MSG_SEVIRI_1_5_HRV;
			img->spacecraft_id = Image::spacecraftIDFromHRIT(header.segment_id->spacecraft_id);
			// See if the image needs to be rotated
			data->swapX = header.image_navigation->column_scaling_factor < 0;
			data->swapY = header.image_navigation->line_scaling_factor < 0;
			img->column_factor = abs(header.image_navigation->column_scaling_factor);
			img->line_factor = abs(header.image_navigation->line_scaling_factor);
			if (data->hrv)
			{
				// Since we are omitting the first (11136-7631) of the rotated image,
				// we need to shift the column offset accordingly
				img->column_offset = 5566 - (11136 - 7630);
				img->line_offset = 5566;
			} else {
				img->column_offset = header.image_navigation->column_offset;
				img->line_offset = header.image_navigation->line_offset;
			}
			img->x0 = 1;
			img->y0 = 1;
			data->bpp = header.image_structure->number_of_bits_per_pixel;
		}

		int idx = header.segment_id->sequence_number-1;
		if (idx < 0) continue;
		if ((size_t)idx >= data->segnames.size())
			data->segnames.resize(idx + 1);
		data->segnames[idx] = *i;
	}

	// Special handling for HRV images
	if (data->hrv)
	{
		// Widen the image to include both image parts, placed in their right
		// position
		data->origColumns += data->UpperEastColumnActual + 1;
	}

	data->columns = data->origColumns;
	data->lines = data->origLines;

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
		cropIfNeeded(*img);
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
