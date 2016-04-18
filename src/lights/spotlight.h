/****************************************************************************
 *
 * 			spotlight.h: Spotlight api
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alfredo de Greef
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
#ifndef __SPOTLIGHT_H
#define __SPOTLIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "light.h"
#include "params.h"

__BEGIN_YAFRAY

class spotEmitter_t : public emitter_t
{
	public:
		spotEmitter_t(const point3d_t &f,const vector3d_t &dir,PFLOAT ca,
				const color_t &c);
		virtual ~spotEmitter_t();
		virtual void numSamples(int n);
		virtual void getDirection(int num, point3d_t &p, vector3d_t &dir, color_t &c)const;
	protected:
		point3d_t from;
		vector3d_t direction, diru, dirv;
		PFLOAT cosa;
		color_t color,scolor;
};

class spotLight_t : public light_t
{
	public:
		spotLight_t(	const point3d_t &fm, const point3d_t &to,
				const color_t &cl, CFLOAT pw,
				PFLOAT ca, PFLOAT cda, PFLOAT bd,
				bool cs=false)
		{
			from=fm;  dir=fm-to;  dir.normalize();
			ndir = -dir;
			color=cl;  power=pw;
			cda = (ca*cda)*M_PI/180.0;
			ca *= M_PI/180.0;
			cosin = cos(ca-cda);  cosout=cos(ca);  beamDist=bd;
			angle = ca;
			cosa = cos(ca);
			tana = tan(ca);
			sina = isina = sin(ca);
			if (isina!=0.0) isina = 1.0/isina;
			cast_shadows = cs;
			use_map = halo = false;
			createCS(ndir, vx, vy);
		};

		void setMap(int res, int ss, PFLOAT b);
		void setHalo(const color_t &f, CFLOAT d, PFLOAT b=0, PFLOAT s=0.1);

		virtual color_t illuminate(renderState_t &state,const scene_t &s, 
				const surfacePoint_t sp, const vector3d_t &eye) const;
		virtual point3d_t position() const { return from; };
		virtual emitter_t * getEmitter(int maxsamples)const 
		{return new spotEmitter_t(from,-dir,cosa,color*power*(angle/M_PI));};
		virtual void init(scene_t &scene) {if(halo) buildShadowMap(scene);};
		virtual ~spotLight_t() {};

		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);
		static pluginInfo_t info();
	protected:
		
		point3d_t from;
		vector3d_t dir, ndir;
		color_t color;
		bool cast_shadows;
		CFLOAT power;
		PFLOAT beamDist;
		PFLOAT cosin, cosout, angle;
		bool halo, use_map;
		
		// Volumetric needed data
		
		color_t getVolume(const scene_t &s, const surfacePoint_t sp, 
				const vector3d_t &eye) const;
		PFLOAT & shadow(int x,int y) {return shadow_map[y*resolution+x];};
		const PFLOAT & shadow(int x,int y)const 
		{
			if((x>=resolution) || (y>=resolution) || (x<0) || (y<0)) 
				return noshadow;
			return shadow_map[y*resolution+x];
		};
		color_t getMappedLight(const surfacePoint_t &sp)const;
		color_t sumLine(const point3d_t &s,const point3d_t &e)const;
		color_t getFog(PFLOAT d)const;
		void buildShadowMap(scene_t &scene);

		vector3d_t vx,vy;
		PFLOAT cosa, tana, sina, isina;
		std::vector<PFLOAT> shadow_map;
		int resolution;
		PFLOAT halfres;
		PFLOAT noshadow;
		PFLOAT sblur, hblur;
		int shadow_samples;
		PFLOAT stepsize;
		color_t fog;
		CFLOAT fden;
};

__END_YAFRAY
#endif
