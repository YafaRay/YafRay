/****************************************************************************
 *
 * 			sunlight.h
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *      
 */
#ifndef __SUNLIGHT_H
#define __SUNLIGHT_H

#include "light.h"
#include "params.h"

__BEGIN_YAFRAY

class sunLight_t : public light_t
{
	public:
		sunLight_t(const point3d_t &dir, const color_t &c, CFLOAT pwr, bool shd);
		virtual color_t illuminate(renderState_t &state,const scene_t &s, const surfacePoint_t sp,
					const vector3d_t &eye) const;
		// has no position
		virtual point3d_t position() const { return point3d_t(0, 0, 0); };
		virtual void init(scene_t &scene) {};
		virtual ~sunLight_t() {};
		
		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);

	protected:
		vector3d_t direction;
		color_t color;
		bool cast_shadows;
};

__END_YAFRAY
#endif	// __SUNLIGHT_H
