/****************************************************************************
 *
 * 			shader.cc: Shader general,generic,and constant implementation 
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

#include "basicshaders.h"
#include "texture.h"
using namespace std;
//#include <cmath>

__BEGIN_YAFRAY

color_t genericShader_t::fromRadiosity(renderState_t &state,const surfacePoint_t &sp,
																		const energy_t &ene,const vector3d_t &eye)const
{
	if( (FACE_FORWARD(sp.Ng(),sp.N(),eye) * ene.dir)<0) return color_t(0.0);
	color_t lkc=scolor, lks=ks;
	CFLOAT lkh=hard;
	if (!mods.empty())
	{
		for(vector<modulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).modulate(lkc,lks,lkh,sp, eye);
		}
	}
	return edif*ene.color*lkc;
}

color_t genericShader_t::fromLight(renderState_t &state,const surfacePoint_t &sp,
		const energy_t &energy,const vector3d_t &eye)const
{
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N=FACE_FORWARD(sp.Ng(),sp.N(),edir);
	CFLOAT inte=N*energy.dir;
	if (inte<0) return color_t(0.0);

	color_t lkc=scolor,lks=ks;
	CFLOAT lkh=hard;

	if (!mods.empty())
	{
		for(vector<modulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).modulate(lkc,lks,lkh,sp, eye);
		}
	}
	edir = reflect(N,edir);
	CFLOAT refle=edir*energy.dir;
	if (refle<0) refle=0; else refle=std::pow((CFLOAT)refle,(CFLOAT)lkh);

	return (edif*inte*energy.color*lkc) + lks*refle*energy.color;
}

color_t genericShader_t::fromWorld(renderState_t &state,const surfacePoint_t &sp,const scene_t &s,
															const vector3d_t &eye)const
{
	if (environment) return environment->stdoutColor(state, sp, eye, &s);

	color_t Rresul(0.0), Tresul(0.0);

	CFLOAT fKr,fKt;
	if (eref.null() && erefr.null() ) return color_t(0.0);
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N=FACE_FORWARD(sp.Ng(),sp.N(),edir);
	vector3d_t Ng=FACE_FORWARD(sp.Ng(),sp.Ng(),edir);
	CFLOAT &cont=state.contribution;
	bool &chroma = state.chromatic;
	PFLOAT &cur_ior = state.cur_ior;
	CFLOAT oldcont=cont;

	if ((N*eye)<0) N=Ng;

	if (use_fastf)
		fast_fresnel(edir, N, fastf_IOR, fKr, fKt);
	else
		fresnel(edir, N, IOR, fKr, fKt);

	// fresnel color interpolation
	color_t cur_rfLcol = eref + fKr*(eref2 - eref);
	color_t cur_rfRcol = erefr2 + fKt*(erefr - erefr2);

	const void *oldorigin=state.skipelement;
	state.skipelement=sp.getOrigin();

	if (!cur_rfLcol.null())
	{
		if(((sp.Ng()*eye)>0) || (state.raylevel<1))
		{
			vector3d_t ref=reflect(N,edir);

			PFLOAT offset=ref*Ng;
			if(offset<=0.05)
			{
				ref=ref+Ng*(0.05-offset);
				ref.normalize();
			}

			CFLOAT nr= minR+fKr;
			if (nr>1.0) nr=1.0;
			if ((nr*cont)>0.01)
			{
				cont *= nr;
				color_t nref= cur_rfLcol * nr;
				Rresul = nref*s.raytrace(state,sp.P(), ref);
				cont = oldcont;
			}
		}
	}
	if (!cur_rfRcol.null())
	{
		vector3d_t ref;
		if (chroma && (dispersion_power>0.0)) {
			Tresul.black();
			color_t dispcol(1.0);
			CFLOAT ds_scale=1.f/(PFLOAT)dispersion_samples;
			for (int ds=0;ds<dispersion_samples;ds++) {
				PFLOAT djt = dispersion_jitter ? ourRandom() : 0.5;
				PFLOAT nior = getIORcolor((ds+djt)*ds_scale, CauchyA, CauchyB, dispcol);
				ref = refract(sp.N(), edir, nior);
				if (ref.null() && tir) ref = reflect(N, edir);
				if (!ref.null())
				{
					CFLOAT nt = (1.0<fKt) ? 1.0 : fKt;
					if ((nt*cont)>0.01)
					{
						cont *= nt;
						chroma = false;
						cur_ior = nior;
						Tresul += dispcol * nt * s.raytrace(state, sp.P(), ref);
						cont = oldcont;
					}
				}
			}
			Tresul *= ds_scale*cur_rfRcol;
		}
		else {
			if (!chroma)
				ref = refract(sp.N(), edir, cur_ior);
			else
				ref = refract(sp.N(), edir, IOR);
			if (ref.null() && tir) ref = reflect(N, edir);
			if (!ref.null())
			{
				CFLOAT nt = (1.0<fKt) ? 1.0 : fKt;
				if ((nt*cont)>0.01)
				{
					cont *= nt;
					Tresul = cur_rfRcol * nt * s.raytrace(state, sp.P(), ref);
					// absorption
					if ((!beer_sigma_a.null()) && (state.raylevel>0)) {
						color_t be(-sp.Z()*beer_sigma_a);
						be.set(exp(be.getR()), exp(be.getG()), exp(be.getB()));
						Tresul *= be;
					}
					cont = oldcont;
				}
			}
		}
	}

	if (!mods.empty())
	{
		for(vector<modulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).modulate(Tresul,Rresul,sp, eye);
		}
	}
	state.skipelement=oldorigin;
	return (Tresul+Rresul);
}

const color_t genericShader_t::getDiffuse(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye) const
{
	color_t lkc=scolor;
	if( ! mods.empty() )
	{
		color_t lks=ks;
		CFLOAT lkh=hard;
		for(vector<modulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).modulate(lkc,lks,lkh,sp, eye);
		}
	}
	return lkc;
}

void genericShader_t::displace(renderState_t &state, surfacePoint_t &sp, const vector3d_t &eye, PFLOAT res)const
{
	if( ! mods.empty() )
	{
		for(vector<modulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).displace(sp, eye, res*sp.Z());
		}
	}
}

shader_t * genericShader_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lmod,
				        renderEnvironment_t &render)
{
	color_t color(0.0),specular(0.0),reflected(0.0),transmitted(0.0);
	CFLOAT hard=1.0,ior=1.0,minref=0.0;
	bool fast_fresnel=false,tir=false;
	// new fresnel angle dependent colors
	color_t reflected2(0.0), transmitted2(0.0);

	// dispersion parameters
	CFLOAT disp_pw = 0;
	int disp_sam = 0;
	bool disp_jit = false;
	color_t beer(0.0);
	bparams.getParam("dispersion_power", disp_pw);
	bparams.getParam("dispersion_samples", disp_sam);
	bparams.getParam("dispersion_jitter", disp_jit);

	// transmit absorption coefficient
	bparams.getParam("absorption", beer);

	bparams.getParam("color", color);
	bparams.getParam("specular", specular);
	bparams.getParam("reflected", reflected);
	if (bparams.getParam("transmited", transmitted))
		cerr << "Use transmitted instead of transmited\n";
	else bparams.getParam("transmitted", transmitted);
	bparams.getParam("hard", hard);
	bparams.getParam("IOR", ior);
	bparams.getParam("min_refle", minref);
	bparams.getParam("fast_fresnel", fast_fresnel);
	bparams.getParam("tir", tir);

	// if not specified, make second fresnel color = first fresnel color
	if (!bparams.getParam("reflected2", reflected2)) reflected2 = reflected;
	if (!bparams.getParam("transmitted2", transmitted2)) transmitted2 = transmitted;

	// environment shader, for use with other blocks, used in fromWorld()
	string _env_name="";
	const string *env_name=&_env_name;
	bparams.getParam("environment", env_name);
	shader_t* env = render.getShader(*env_name);

	genericShader_t *ns=new genericShader_t(color,hard, (CFLOAT)1.0, reflected, transmitted,
						specular, reflected2, transmitted2, env,
						minref, (CFLOAT)1.0, ior, fast_fresnel, tir, disp_pw, disp_sam, disp_jit, beer);
	for(list<paramMap_t>::iterator i=lmod.begin();i!=lmod.end();++i)
	{
		string _texname;
		const string *texname=&_texname;
		GFLOAT size=1, sizex=1, sizey=1, sizez=1;
		CFLOAT color=0, specular=0, hard=0, trans=0, refle=0, displace=0;
		
		string _mode="mix", _mapping="flat", _texco="orco", _clipping="extend";	// defaults
		string _projx="x", _projy="y", _projz="z";
		const string *mode=&_mode, *mapping=&_mapping, *texco=&_texco, *clipping=&_clipping;	// defaults
		const string *projx=&_projx, *projy=&_projy, *projz=&_projz;

		int xrep=1, yrep=1;
		matrix4x4_t txm;
		txm.identity();
		GFLOAT ofsx=0, ofsy=0, ofsz=0;
		GFLOAT minx=0, miny=0, maxx=1, maxy=1;

		paramMap_t &params=*i;

		params.getParam("texname",texname);
		texture_t *texture=render.getTexture(*texname);
		if(texture==NULL)
		{
			cerr<<"Undefined texture : "<<texname<<endl;
			continue;
		}
		params.getParam("size", size);
		params.getParam("sizex", sizex);
		params.getParam("sizey", sizey);
		params.getParam("sizez", sizez);
		params.getParam("color", color);
		params.getParam("specular", specular);
		params.getParam("hard", hard);
		params.getParam("transmission", trans);
		params.getParam("reflection", refle);
		params.getParam("normal", displace);
		params.getParam("mode", mode);
		params.getParam("mapping", mapping);
		params.getParam("texco", texco);
		params.getParam("clipping", clipping);
		params.getParam("xrepeat", xrep);
		params.getParam("yrepeat", yrep);
		params.getParam("ofsx", ofsx);
		params.getParam("ofsy", ofsy);
		params.getParam("ofsz", ofsz);
		params.getParam("cropmin_x", minx);
		params.getParam("cropmin_y", miny);
		params.getParam("cropmax_x", maxx);
		params.getParam("cropmax_y", maxy);
		params.getParam("proj_x", projx);
		params.getParam("proj_y", projy);
		params.getParam("proj_z", projz);

		// texture matrix directly in modulator
		params.getParam("m00", txm[0][0]);
		params.getParam("m01", txm[0][1]);
		params.getParam("m02", txm[0][2]);
		params.getParam("m03", txm[0][3]);
		params.getParam("m10", txm[1][0]);
		params.getParam("m11", txm[1][1]);
		params.getParam("m12", txm[1][2]);
		params.getParam("m13", txm[1][3]);
		params.getParam("m20", txm[2][0]);
		params.getParam("m21", txm[2][1]);
		params.getParam("m22", txm[2][2]);
		params.getParam("m23", txm[2][3]);
		params.getParam("m30", txm[3][0]);
		params.getParam("m31", txm[3][1]);
		params.getParam("m32", txm[3][2]);
		params.getParam("m33", txm[3][3]);

		modulator_t modu(texture);
		modu.sizeX(sizex);
		modu.sizeY(sizey);
		modu.sizeZ(sizez);
		modu.ofsX(ofsx);
		modu.ofsY(ofsy);
		modu.ofsZ(ofsz);
		if (size!=1.0) modu.size(size);
		/*
		if (*mode=="mix") modu.mode(MIX);
		if (*mode=="mul") modu.mode(MUL);
		if (*mode=="add") modu.mode(ADD);
		if (*mode=="sub") modu.mode(SUB);
		*/
		modu.string2modetype(*mode);
		modu.color(color);
		modu.specular(specular);
		modu.hard(hard);
		modu.transmision(trans);
		modu.reflection(refle);
		modu.displace(displace);
		modu.string2maptype(*mapping);
		modu.string2texcotype(*texco);
		modu.string2cliptype(*clipping);
		modu.string2texprojection(*projx, *projy, *projz);
		modu.setRepeat(xrep, yrep);
		modu.setCrop(minx, miny, maxx, maxy);
		modu.setTransform(txm);
		ns->addModulator(modu);
		params.checkUnused("modulator");
	}
	return ns;
}

shader_t * constantShader_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lmod,
				        renderEnvironment_t &render)
{
	color_t color(0.0);
	bparams.getParam("color",color);
	constantShader_t *ns=new constantShader_t(color);
	return ns;
}

extern "C"
{

YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("generic",genericShader_t::factory);
	render.registerFactory("constant",constantShader_t::factory);
	std::cout<<"Registered basicshaders\n";
}

}
__END_YAFRAY
