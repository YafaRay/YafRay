/****************************************************************************
 *
 *      arealight.h : Arealight header, this is the api for the arealight
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

__BEGIN_YAFRAY

class quadEmitter_t : public emitter_t
{
	public:
		quadEmitter_t(const point3d_t &corn,const vector3d_t &tox,
				const vector3d_t &toy,const vector3d_t &dir,const color_t &c);
		virtual ~quadEmitter_t();
		virtual void numSamples(int n);
		virtual void getDirection(int num,point3d_t &p,vector3d_t &dir,color_t &c)const;
		virtual bool storeDirect()const {return true;};
	protected:
		point3d_t corner;
		vector3d_t toX,toY,direction,NU,NV;
		color_t color,scolor;
};

/** Implementation of a quad Arealight
 *
 * Creates a quad filled with samples to illuminate objects. It
 * has penumbra prediction with a configurable numer of samples.
 * So if you create it with 60 samples and 20 for penumbra prediction,
 * it will shoot first 20 random rays trying to skip 60 if possible.
 *
 * @see light_t
 */

class areaLight_t : public light_t
{
	public:
		/** Main constructor
		 *
		 * @param a is a corner of the quad
		 * @param b is another corner of the quad
		 * @param c is another corner of the quad
		 * @param d is another corner of the quad
		 * @param nsam is the number of samples
		 * @param c is the color of the light
		 * @param inte is the intensity of the light
		 * @param fsam is the number of samples for penumbra prediction,if it's 0
		 * then penumbra prediction is not used
		 * 
		 */
		areaLight_t(const point3d_t &a,const point3d_t &b,
												const point3d_t &,const point3d_t &d,
												int nsam, const color_t &c,CFLOAT inte,
												int fsam=0,bool dum=false);
		///@see light_t
		virtual color_t illuminate(renderState_t &state,const scene_t &s,const surfacePoint_t sp,
															const vector3d_t &eye)const;
		///@see light_t
		virtual point3d_t position()const {return from;};
		///@see light_t
		virtual void init(scene_t &scene) {};
		/// Destructor
		virtual ~areaLight_t() {};

		virtual emitter_t * getEmitter(int maxsamples)const 
		{
			if(dummy) return new quadEmitter_t(corner,toX,toY,
													direction,color*pow*(toX^toY).length()*0.5); 
			else return NULL;
		};

		void setDummy() {dummy=true;};

		static light_t *factory(paramMap_t &params,renderEnvironment_t &render);
		static pluginInfo_t info();
	protected:
		/** Penumbra predictor
		 *
		 * It tries to guess if the current shading point is in total shadow,
		 * total light or penumbra.
		 *
		 * @param s is the scene being rendered
		 * @param P is the surface point being shaded
		 * @param N is the normal of the surface correctly fliped
		 *
		 */
		int guessLight(renderState_t &state,const scene_t &s,
				const surfacePoint_t &P,const vector3d_t &N)const;
		/** Generates the samples in the quad
		 *
		 * Fills the given vectors with samples inside the quad, and also
		 * calculates the random vectors to be added at shading time
		 *
		 * @param a is a corner of the quad
		 * @param b is another corner of the quad
		 * @param c is another corner of the quad
		 * @param d is another corner of the quad
		 * @param points_vector is where the sample points are going to be stored
		 * @param jit_vector is there the random jitter vectors are going to be 
		 * stored
		 * @param samp is the number of samples
		 * 
		 */
		int fillQuad(const point3d_t &a,const point3d_t &b,
									const point3d_t &c,const point3d_t &d,
									std::vector<point3d_t> &points_vector,
									std::vector<std::pair<vector3d_t,vector3d_t> > &jit_vector,int samp);
		/// The sample points
		std::vector<point3d_t> points;
		/// The jitter vectors, they are randomly added to the samples while shading
		std::vector<std::pair<vector3d_t,vector3d_t> > jit;
		/// An average point to return as position
		point3d_t from;
		/// The normal of the quad
		vector3d_t direction;
		/// The color of the light
		color_t color;
		/// The intensity of the light
		CFLOAT pow;
		/// Numeber of samples
		int samples;
		/// Number of penumbra prediction samples
		int fsamples;
		bool dummy;
		point3d_t corner;
		vector3d_t toX,toY;
};

#define SHADOW 0
#define PENUMBRA 1
#define LIGHT 2

__END_YAFRAY

#endif
