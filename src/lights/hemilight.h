/****************************************************************************
 *
 *		  hemilight.h Hemi (Sky/Environment) light interface
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alfredo de Greef
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

#ifndef __HEMILIGHT_H
#define __HEMILIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "light.h"
#include "params.h"
#include "mcqmc.h"
#include <vector>

__BEGIN_YAFRAY

class hemiLight_t : public light_t
{
	public:
		hemiLight_t(int nsam, const color_t &c, CFLOAT pwr, PFLOAT mdist, bool usebg, bool useqmc=false);
		virtual color_t illuminate(renderState_t &state,const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye) const;
		// has no position, return origin
		virtual point3d_t position() const { return point3d_t(0, 0, 0); };
		virtual void init(scene_t &scene) {};
		virtual ~hemiLight_t() { if (HSEQ) delete[] HSEQ;  HSEQ=NULL; };

		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);
		static pluginInfo_t info();
	protected:
		int samples;	// number of samples
		PFLOAT sampdiv;	// reciprocal of samples
		color_t color;
		CFLOAT power;
		PFLOAT maxdistance;	// maximum occlusion distance
		bool use_background;
		int grid;
		PFLOAT gridiv, gridiv2pi;
		vector3d_t getNext(const vector3d_t &nrm, int cursam,
					const vector3d_t &ru, const vector3d_t &Rv) const;
		// QMC sampling
		bool use_QMC;
		Halton* HSEQ;
};

__END_YAFRAY
#endif
