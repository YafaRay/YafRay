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
#ifndef __BASICSHADER_H
#define __BASICSHADER_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "shader.h"
#include "params.h"
#include "spectrum.h"

#include <vector>

__BEGIN_YAFRAY

class genericShader_t : public shader_t
{
	public: 

		/** The constructor.
		 *
		 * @param color is the diffuse reflected color
		 * @param h is the "hardness" of the specular highlights.
		 * @param ed is the amount of diffuse light reflected.
		 * @param er is the reflected color (black equals to no reflection).
		 * @param erfr is the refracted (transmited) color.
		 * @param Ks is the specular higlight color (phong shading).
		 * @param minr is the minimum reflected amount of light for fresnel formula.
		 * @param maxt is the maximun transmited amount of light (unnecesary).
		 * @param _IOR is the index of refraction for the material (relative).
		 *
		 */
		genericShader_t(const color_t &color, CFLOAT h,CFLOAT ed, const color_t &er,
				const color_t &erfr, const color_t &Ks,
				const color_t &er2, const color_t &erfr2,
				const shader_t* env,
				CFLOAT minr=0, CFLOAT maxt=1, PFLOAT _IOR=1.5,
				bool fastf=false,bool _tir=false,
				CFLOAT disp_pw=0, int disp_sam=0, bool disp_jit=false, color_t beer=color_t(0.0))
		{
			scolor=color;  hard=h;  edif=ed;  eref=er;  erefr=erfr;
			ks=Ks;  minR=minr;  maxT=maxt;
			IOR=_IOR;
			// for fast_fresnel()
			fastf_IOR = (IOR - 1.0) / (IOR + 1.0);
			fastf_IOR *= fastf_IOR;
			use_fastf = fastf;
			tir=_tir;
			// for use with fresnel color modulation
			eref2=er2;  erefr2=erfr2;
			// dispersion
			dispersion_power = dispersion_samples = 0;
			if ((disp_pw>0) && (disp_sam>0)) {
				dispersion_power = disp_pw;
				dispersion_samples = disp_sam;
				dispersion_jitter = disp_jit;
			}
			beer_sigma_a = beer;
			CauchyCoefficients(IOR, disp_pw, CauchyA, CauchyB);
			// environment shader
			environment = env;
		};
		/// @see shader_t
		virtual color_t fromRadiosity(renderState_t &state,const surfacePoint_t &sp,const energy_t &ene,
															const vector3d_t &eye)const;
		/// @see shader_t
		virtual color_t fromLight(renderState_t &state,const surfacePoint_t &sp,const energy_t &energy,
															const vector3d_t &eye)const;
		/// @see shader_t
		virtual color_t fromWorld(renderState_t &state,const surfacePoint_t &sp,const scene_t &scene,
															const vector3d_t &eye)const;
		/// @see shader_t
		virtual const color_t getDiffuse(renderState_t &state,
									const surfacePoint_t &sp, const vector3d_t &eye)const;
		/// Destructor
		virtual ~genericShader_t() {};

		virtual void displace(renderState_t &state,surfacePoint_t &sp, 
				const vector3d_t &eye, PFLOAT res) const;

		virtual void getDispersion(PFLOAT &disp_pw, PFLOAT &A, PFLOAT &B, color_t &beer) const
		{
			disp_pw = dispersion_power;
			beer = beer_sigma_a;
			A = CauchyA;
			B = CauchyB;
		}
		virtual bool getCaustics(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye,
														color_t &ref, color_t &trans, PFLOAT &ior) const
		{
			ref = eref;
			trans = erefr;
			ior = IOR;
			return ((!ref.null()) | (!trans.null()));
		}

		/** Adds a modulator.
		 *
		 * Pushes a new modulator in the vector so it will affect the shading.
		 * @see modulator_t
		 *
		 */
		void addModulator(const modulator_t &mod) {mods.push_back(mod);};

		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);

	protected:
		/// surface diffuse and specular color
		color_t scolor,ks;
		/// reflected and transmitted colors
		color_t eref,erefr,edif;
		// for use with fresnel factor
		color_t eref2, erefr2;
		/// hardness, minimum reflection and maximum transmision.
		CFLOAT hard,minR,maxT;
		/// amount of diffuse light and index of refraction.
		PFLOAT IOR;
		/// IOR precalculated for fast fresnel function
		PFLOAT fastf_IOR;
		bool use_fastf,tir;
		/// The sequence of modulators
		std::vector<modulator_t> mods;
		// for dispersion
		PFLOAT dispersion_power, CauchyA, CauchyB;
		int dispersion_samples;
		bool dispersion_jitter;
		color_t beer_sigma_a;
		// environment shader, for use with other shaders to affect reflect/refract
		const shader_t *environment;
};

/** The constant shader implementation
 *
 * It shades a surface with a constant color
 */
class constantShader_t : public shader_t
{
	public:
		/// The constructor
		constantShader_t(const color_t &color)
				{scolor=color;};
		/// @see shader_t
		virtual color_t fromRadiosity(renderState_t &state,const surfacePoint_t &sp,const energy_t &ene,
															const vector3d_t &eye)const {return scolor;};
		/// @see shader_t
		virtual color_t fromLight(renderState_t &state,const surfacePoint_t &sp,const energy_t &energy,
															const vector3d_t &eye)const {return color_t(0.0);};
		/// @see shader_t
		virtual color_t fromWorld(renderState_t &state,const surfacePoint_t &sp,const scene_t &scene,
															const vector3d_t &eye)const {return scolor;};
		/// @see shader_t
		virtual const color_t getDiffuse(renderState_t &state,
							const surfacePoint_t &sp, const vector3d_t &eye)const
			{return scolor;};
		/// Destructor
		virtual ~constantShader_t() {};

		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		/// Surface color
		color_t scolor;
};

__END_YAFRAY
#endif
