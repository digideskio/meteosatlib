#ifndef CONV_NETCDF_UTILS_H
#define CONV_NETCDF_UTILS_H

//---------------------------------------------------------------------------
//
//  File        :   NetCDFUtils.h
//  Description :   NetCDF utility functions
//  Project     :   ?
//  Author      :   Enrico Zini (for ARPA SIM Emilia Romagna)
//  RCS ID      :   $Id: /local/meteosatlib/conv/ExportNetCDF.cpp 1778 2006-09-20T16:26:07.745577Z enrico  $
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

//#include "../config.h"

#include <netcdfcpp.h>

#include <sstream>
#include <stdexcept>
#include <memory>

#include <math.h>

namespace msat {

template<typename NCObject, typename T>
static void ncfAddAttr(NCObject& ncf, const char* name, const T& val)
{
  if (!ncf.add_att(name, val))
  {
		std::stringstream msg;
    msg << "Adding '" << name << "' attribute " << val;
    throw std::runtime_error(msg.str());
  }
}

template<typename Sample>
static inline NcType getNcType() { throw std::runtime_error("requested NcType for unknown C++ type"); }
template<> static inline NcType getNcType<ncbyte>() { return ncByte; }
template<> static inline NcType getNcType<char>() { return ncChar; }
template<> static inline NcType getNcType<short>() { return ncShort; }
template<> static inline NcType getNcType<int>() { return ncInt; }
template<> static inline NcType getNcType<float>() { return ncFloat; }
template<> static inline NcType getNcType<double>() { return ncDouble; }

struct NcEncoder
{
	virtual NcType getType() = 0;
	virtual void setData(NcVar& var, const Image& img) = 0;
};

template<typename Sample>
struct NcEncoderImpl : public NcEncoder
{
	virtual NcType getType() { return getNcType<Sample>(); }
	virtual void setData(NcVar& var, const Image& img)
	{
		Sample* pixels = new Sample[img.data->columns * img.data->lines];
		for (size_t y = 0; y < img.data->lines; ++y)
			for (size_t x = 0; x < img.data->columns; ++x)
				pixels[y * img.data->columns + x] = img.data->scaledToInt(x, y);

		if (!var.put(pixels, 1, img.data->lines, img.data->columns))
			throw std::runtime_error("writing image values failed");

		delete[] pixels;
	}
};

static std::auto_ptr<NcEncoder> createEncoder(size_t bpp)
{
	if (bpp > 15)
		return std::auto_ptr<NcEncoder>(new NcEncoderImpl<int>);
	else if (bpp > 7)
		return std::auto_ptr<NcEncoder>(new NcEncoderImpl<short>);
	else
		return std::auto_ptr<NcEncoder>(new NcEncoderImpl<ncbyte>);
}

// Recompute the BPP for an image made of integer values, by looking at what is
// their maximum value.  Assume unsigned integers.
static void computeBPP(ImageData& img)
{
	if (img.scalesToInt)
	{
		int max = img.scaledToInt(0, 0);
		for (size_t y = 0; y < img.lines; ++y)
			for (size_t x = 0; x < img.columns; ++x)
			{
				int sample = img.scaledToInt(x, y);
				if (sample > max)
					max = sample;
			}
		img.bpp = (int)ceil(log2(max + 1));
	}
	// Else use the original BPPs
}

}

// vim:set ts=2 sw=2:
#endif