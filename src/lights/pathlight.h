/****************************************************************************
 *
 *		  pathlight.h simplified pathtracing interface
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

#ifndef __PATHLIGHT_H
#define __PATHLIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "light.h"
#include "mcqmc.h"
#include "hash3d.h"
#include "ccthreads.h"
#include "params.h"
#include "photon.h"
#include <vector>
#include <list>
#include <utility>
#include <map>
#include "pathtools.h"
#include "lightcache.h"
#include "cacheproxy.h"
#include "globalphotonlight.h"


__BEGIN_YAFRAY

class photonData_t : public context_t::destructible
{
	public:
		photonData_t(PFLOAT r,std::vector<foundPhoton_t> *f) {radius=r;found=f;};
		virtual ~photonData_t() {delete found;};
		bool valid()const {return found!=NULL;};

		PFLOAT radius;
		std::vector<foundPhoton_t> *found;
};


class pathLight_t : public light_t
{
	friend struct photonData_t;
	public:
		pathLight_t(int nsam, CFLOAT pwr, int depth, int cdepth,bool uQ,
				bool ca=false,PFLOAT casiz=1.0,CFLOAT thr=0.1,bool recal=true,
				bool di=false, bool shows=false,int grid=36,int ref=2,
				bool _occmode=false, PFLOAT occdist=-1, bool _ignorms=false);

		void setCacheThreshold(PFLOAT s,int se) 
		{
			shadow_threshold=s;
			search=se;
			desiredWeight=1.0/shadow_threshold;
			weightLimit=0.8*desiredWeight;
		};
		virtual color_t illuminate(renderState_t &state,const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye) const;
		color_t normalSample(renderState_t &state,const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye) const;
		color_t interpolate(renderState_t &state,const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye) const;
		color_t cached(renderState_t &state,const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye) const;
		// has no position, return origin
		virtual point3d_t position() const { return point3d_t(0, 0, 0); };
		virtual void init(scene_t &scene);
		virtual void postInit(scene_t &scene);
		virtual ~pathLight_t();

		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);
		static pluginInfo_t info();
		
	protected:
		
		int samples, grid;	// number of samples, sqrt of samples
		PFLOAT gridiv, sampdiv;	// reciprocal of gridside & samples
		CFLOAT power;
		int maxdepth;
		int maxcausdepth;
		bool use_QMC;
		Halton* HSEQ;
		color_t takeSample(renderState_t &state,const vector3d_t &N,const surfacePoint_t &sp,
											const scene_t &sc,PFLOAT &avgD,PFLOAT &minD,bool caching=false)const;

		static CFLOAT weight(const lightSample_t &sample,const point3d_t &P,
				const vector3d_t &N,CFLOAT maxweight);
		static CFLOAT weightNoPrec(const lightSample_t &sample,const point3d_t &P,
				const vector3d_t &N,CFLOAT maxweight);
		static CFLOAT weightNoDist(const lightSample_t &sample,const point3d_t &P,
				const vector3d_t &N,CFLOAT maxweight);
		static CFLOAT weightNoDev(const lightSample_t &sample,const point3d_t &P,
				const vector3d_t &N,CFLOAT maxweight);

		void setIrradiance(lightSample_t &sample,PFLOAT &radius);

		color_t getLight(renderState_t &state,const surfacePoint_t &sp,
				const scene_t &sc,const vector3d_t &eye,photonData_t *data)const;
		bool testRefinement(const scene_t &sc);

		hemiSampler_t *getSampler(renderState_t &state,const scene_t &sc)const;
		photonData_t *getPhotonData(renderState_t &state)const;
		cacheProxy_t *getProxy(renderState_t &state,const scene_t &sc)const;

		bool cache;
		PFLOAT dist_to_sample;

		PFLOAT shadow_threshold;
		int maxrefinement,refined;
		bool recalculate,direct,show_samples;
		int search,gridsize;
		PFLOAT lastRadius,searchRadius;
		hemiSampler_t *_sampler;
		const globalPhotonMap_t *pmap;
		const globalPhotonMap_t *imap;
		const globalPhotonLight_t::irHash_t *irhash;
		photonData_t *photonData;
		CFLOAT threshold,devaluated,desiredWeight,weightLimit;
		bool occmode;
		PFLOAT occ_maxdistance;
		bool ignorms;

		std::vector<foundSample_t> stsamples;
		cacheProxy_t *_proxy;
};

__END_YAFRAY

#endif
