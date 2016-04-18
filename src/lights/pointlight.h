/****************************************************************************
 *
 * 			pointlight.h: Point light api 
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
#ifndef __POINTLIGHT_H
#define __POINTLIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "light.h"
#include "params.h"

__BEGIN_YAFRAY

class pointEmitter_t : public emitter_t
{
	public:
		pointEmitter_t(const point3d_t &f, const color_t &c);
		virtual ~pointEmitter_t();
		virtual void getDirection(int num, point3d_t &p, vector3d_t &dir, color_t &c) const;
		virtual void numSamples(int n);
	protected:
		point3d_t from;
		color_t color, lcol;
};

class pointLight_t : public light_t
{
	public:
		pointLight_t(const point3d_t &f, const color_t &c, CFLOAT inte,
			bool sh=false, CFLOAT gli=0, CFLOAT glo=0, int glt=0)
		{
			from = f;
			color = c*inte;
			cast_shadows = sh;
			glow_int = gli;
			glow_ofs = glo;
			glow_type = glt;
		}
		virtual color_t illuminate(renderState_t &state, const scene_t &s,
					const surfacePoint_t sp, const vector3d_t &eye) const;
		virtual point3d_t position() const { return from; }
		virtual emitter_t * getEmitter(int maxsamples) const { return new pointEmitter_t(from, color); }
		virtual void init(scene_t &scene) {}
		virtual ~pointLight_t() {}
		
		static light_t *factory(paramMap_t &params, renderEnvironment_t &render);
		static pluginInfo_t info();

	protected:
		point3d_t from;
		color_t color;
		bool cast_shadows;
		// glow
		CFLOAT glow_int, glow_ofs;
		int glow_type;
};

__END_YAFRAY
#endif
