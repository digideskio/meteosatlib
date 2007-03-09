//-----------------------------------------------------------------------------
//
//  File        : Latlon.h
//  Description : Mapping Algorithms interface - Latitude-Longitude grid
//  Author      : Enrico Zini (ARPA SIM Emilia Romagna)
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

#include <msat/proj/Projection.h>

#ifndef METEOSATLIB_PROJ_LATLON_H
#define METEOSATLIB_PROJ_LATLON_H

namespace msat {
namespace proj {

class Latlon : public Projection
{
	double latmin, lonmin;
public:
	Latlon(double latmin, double lonmin) : latmin(latmin), lonmin(lonmin) {}
	virtual void mapToProjected(const MapPoint& m, ProjectedPoint& p) const;
	virtual void projectedToMap(const ProjectedPoint& p, MapPoint& m) const;
	virtual std::string format() const { return "Latlon"; }
	Projection* clone() const { return new Latlon(latmin, lonmin); }
};

}
}

// vim:set ts=2 sw=2:
#endif