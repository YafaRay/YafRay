/****************************************************************************
 *
 * 			light.h: Generic light api 
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
#ifndef __LIGHT_H
#define __LIGHT_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif


__BEGIN_YAFRAY
class light_t;
__END_YAFRAY
#include"vector3d.h"
#include"scene.h"
#include"color.h"


__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT emitter_t
{
	public:
		virtual ~emitter_t() {};
		virtual void numSamples(int n) {};
		virtual void getDirection(int num,point3d_t &p,vector3d_t &dir,color_t &c)const=0;
		virtual bool storeDirect()const {return false;};
};

/** Abstract interface for light rendering.
 * 
 * This is the interface the render will use to handle lights.
 * If you implement a light, you have to inherit from this
 * and cover the virtual methods
 *
 */

class YAFRAYCORE_EXPORT light_t
{
	public:
		/// Constructor common for all lights
		light_t() {use_in_render=true;use_in_indirect=true;};
		virtual ~light_t() {};
		/** Returns the color for a given point.
		 *
		 * It takes a surface point, and taking its assigned shader must
		 * compute the reflected color
		 * @param s is the scene being rendered.
		 * @param sp is the surface point being shaded.
		 * @param eye is a vector pointing to the viewer.
		 * @see surfacePoint_t
		 *
		 */
		virtual color_t illuminate(renderState_t &state,const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye)const=0;
		/// Returns the position if it's possible.
		virtual point3d_t position() const=0;
		virtual emitter_t * getEmitter(int maxsamples)const {return NULL;};

		/** Light initialization.
		 * 
		 * Sometimes a light has to precompute data from the scene before
		 * the render starts. It happens with shadow map lights or photon
		 * lights. This method is called before the render.
		 * 
		 * @param scene is the scene that will be rendered.
		 *
		 */
		virtual void init(scene_t &scene)=0;
		virtual void postInit(scene_t &scene) {};

		bool useInRender()const {return use_in_render;};
		void useInRender(bool u) {use_in_render=u;};
		bool useInIndirect()const {return use_in_indirect;};
		void useInIndirect(bool u) {use_in_indirect=u;};

	protected:
		bool use_in_render;
		bool use_in_indirect;
};


// simple fake glow/halo, more or less foglights
// type 0, own hack.
// type 1, based on idea by Han-Wen Nienhuys described in RTNews7v3, also used in megapov
// recent paper by Ramamoorthi, analytic single scattering... might be interesting,
// though looks much more expensive then these simpler hacks
inline CFLOAT getGlow(const point3d_t &pos, const surfacePoint_t &sp,
					const vector3d_t &eye, PFLOAT glow_ofs, int glow_type)
{
	const point3d_t ray_o = sp.P()+eye;
	vector3d_t ray_d = -eye;
	ray_d.normalize();
	if (glow_type==0) {
		vector3d_t toL(pos - ray_o);
		const CFLOAT d0 = glow_ofs + (pos - (ray_o + (ray_d VDOT toL)*ray_d)).length();
		if (d0<=0.f) return 0.f;
		toL.normalize();
		vector3d_t toP(sp.P() - pos);
		toP.normalize();
		const CFLOAT da = toP VDOT toL;
		if (da<=0.f) return 0.f;
		return da/d0;
	}
	const CFLOAT t0 =  ray_d VDOT (ray_o - pos);
	CFLOAT d0 = glow_ofs + ((ray_o - t0*ray_d) - pos).length();
	if (d0<=0.f) return 0.f;
	d0 = 1.f/d0;
	return (atan((sp.Z() + t0)*d0) - atan(t0*d0))*d0;
}

__END_YAFRAY

#endif
