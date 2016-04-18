/****************************************************************************
 *
 *      spherelight.h : Spherelight header, this is the api for the spherelight
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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

#ifndef __AREALIGHT_H
#define __AREALIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "light.h"
#include "params.h"
#include "mcqmc.h"

__BEGIN_YAFRAY

class sphereEmitter_t : public emitter_t
{
	public:
		sphereEmitter_t(const color_t &c, const point3d_t &p, PFLOAT rad);
		virtual ~sphereEmitter_t();
		virtual void numSamples(int n);
		virtual void getDirection(int num, point3d_t &p, vector3d_t &dir, color_t &c) const;
		virtual bool storeDirect() const { return true; }
	protected:
		color_t color, lcol;
		point3d_t pos;
		PFLOAT radius;
};


class sphereLight_t : public light_t
{
	public:
		sphereLight_t(const point3d_t &p, PFLOAT r, int nsam, int psam,
				const color_t &c, CFLOAT pw, int qmcm=0, bool dm=false, CFLOAT gli=0, CFLOAT glo=0, int glt=0);
		virtual color_t illuminate(renderState_t &state, const scene_t &s, const surfacePoint_t sp, const vector3d_t &eye) const;
		virtual point3d_t position() const { return pos; }
		virtual void init(scene_t &scene) {}
		virtual ~sphereLight_t() { delete[] HSEQ;  HSEQ=NULL; }

		virtual emitter_t * getEmitter(int maxsamples) const { return new sphereEmitter_t(color, pos, rad); }

		static light_t *factory(paramMap_t &params, renderEnvironment_t &render);
		static pluginInfo_t info();
	protected:
		point3d_t pos;
		PFLOAT rad;
		color_t color;
		int samples, pred_samples;
		int qmc_method;
		CFLOAT samdiv;
		bool dummy;
		Halton* HSEQ;
		CFLOAT glow_int, glow_ofs;
		int glow_type;
};

__END_YAFRAY

#endif
