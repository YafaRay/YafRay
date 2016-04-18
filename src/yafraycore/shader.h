/****************************************************************************
 *
 * 			shader.h: Shader general, genericshader, and constantshader api 
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
#ifndef __SHADER_H
#define __SHADER_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY
class shader_t;
__END_YAFRAY
#include "surface.h"
#include "vector3d.h"
#include "color.h"
#include "scene.h"
#include "texture.h"

#include <vector>

__BEGIN_YAFRAY
/// This class holds data of energy comming from a light.

class YAFRAYCORE_EXPORT energy_t
{
	public:
		/// Void constructor
		energy_t() {};

		/** The common constructor.
		 *
		 * @param _dir is a vector pointting from the point to the light.
		 * @param c is the light's color comming from that dir.
		 *
		 */
		energy_t(const vector3d_t &_dir,const color_t &c) {dir=_dir;color=c;};

		/// The incoming direction of the light
		vector3d_t dir;
		/// The color of the light
		color_t color;
};

/** This is the absrtact interface that is used for point shading.
 *
 * Each object has a shader assigned. It takes the incoming light energy and
 * returns a color. Its methods are invoked from light's illuminate method.
 * A shader Colud be viewed as a "material type" and a shader instance could
 * be viewed as a "material".
 * @see light_t
 *
 */
class YAFRAYCORE_EXPORT shader_t
{
	public:
		virtual ~shader_t() {};
		/// Light comming from diffuse reflection could be handled in a different way so
		//  I put this extra method.
		virtual color_t fromRadiosity(renderState_t &state,
				const surfacePoint_t &sp,const energy_t &ene,
				const vector3d_t &eye)const=0;

		/** Returns the reflected color for anergy comming from a light.
		 *
		 * This method could be called several times from all the lights in a scene
		 * for a given point. This is why is saparated from the "fromWorld" method.
		 *
		 * @param sp is the surface point being shaded.
		 * @param ene is the incoming energy.
		 * @param eye is a vector pointing to the viewer.
		 * @see surfacePoint_t
		 * @see energy_t
		 *
		 */
		virtual color_t fromLight(renderState_t &state,
				const surfacePoint_t &sp,const energy_t &ene,
				const vector3d_t &eye)const=0;

		/** Returns the color reflected from the environment.
		 *
		 * Usualy it's the reflected and transmited light comming from other
		 * objects. It takes the scene as an argument, so it can call "raytrace".
		 * This method is called only one time for each sampled point.
		 * @param sp is the surface point being shaded.
		 * @param scene is the scene being rendered.
		 * @param eye is a vector pointing to the viewer.
		 *
		 */
		virtual color_t fromWorld(renderState_t &state,
				const surfacePoint_t &sp,const scene_t &scene,
				const vector3d_t &eye)const=0;
		/** Returns the diffuse reflection component
		 *
		 * Used for radiosity calculation
		 *
		 */
		virtual const color_t getDiffuse(renderState_t &state,
				const surfacePoint_t &sp, const vector3d_t &eye) const=0;
		virtual void displace(renderState_t &state,surfacePoint_t &sp, 
				const vector3d_t &eye, PFLOAT res) const {}

		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const
		{
			return colorA_t(0.0);
		}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const
		{
			return 0.0;
		}
		virtual vector3d_t stdoutVector(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const
		{
			return vector3d_t(0,0,0);
		}
		// currently only used for the blendershader ramps using colorbands,
		// this allows a shader to be evaluated externally without having a shader input,
		// here for now only using a float as input,
		virtual colorA_t stdoutColor(CFLOAT x, renderState_t &state,
				const surfacePoint_t &sp, const vector3d_t &eye, const scene_t *scene=NULL) const
		{
			return colorA_t(0.0);
		}
		virtual bool getCaustics(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye,
				color_t &ref, color_t &trans, PFLOAT &ior) const
		{
			ref.black();
			trans.black();
			ior = 1.0;
			return false;
		}
		virtual bool discrete()const {return false;};
		virtual bool isRGB() const { return true; }
		virtual void getDispersion(PFLOAT &disp_pw, PFLOAT &A, PFLOAT &B, color_t &beer) const { disp_pw=A=B=0;  beer.black(); }
};

#define FACE_FORWARD(Ng,N,I) ((((Ng)*(I))<0) ? (-N) : (N))
#define IOR_FORWARD(Ng,IOR,I) ((((Ng)*(I))<0) ? (1.0/IOR) : (IOR))

__END_YAFRAY
#endif
