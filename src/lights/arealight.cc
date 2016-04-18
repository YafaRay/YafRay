/****************************************************************************
 *
 *      arealight.cc : this is the implementation for arealight
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

#include "arealight.h"
//#include <cmath>
#include <cstdlib>

using namespace std;

__BEGIN_YAFRAY

int areaLight_t::fillQuad(const point3d_t &a,const point3d_t &b,
											const point3d_t &c,const point3d_t &d,
											vector<point3d_t> &points_vector,
											vector<pair<vector3d_t,vector3d_t> > &jit_vector,int samp)
{
	PFLOAT max_v,max_h;
	PFLOAT ab_len=(b-a).length();
	PFLOAT bc_len=(c-b).length();
	PFLOAT cd_len=(d-c).length();
	PFLOAT da_len=(a-d).length();

	max_v= (bc_len>da_len) ? bc_len : da_len;
	max_h= (ab_len>cd_len) ? ab_len : cd_len;

	int v_samp=(int)(( max_v / (max_v+max_h) )*2.0*std::sqrt(static_cast<PFLOAT>(samp)));
	int h_samp=(int)(( max_h / (max_v+max_h) )*2.0*std::sqrt(static_cast<PFLOAT>(samp)));
	
	if (points_vector.size()==1) return 1;

	vector3d_t inc_v_a= (d-a)/v_samp;
	vector3d_t inc_v_b= (c-b)/v_samp;
	point3d_t line_a=a+0.5*inc_v_a;
	point3d_t line_b=b+0.5*inc_v_b;
	int index=0;
	for(int i=0;i<v_samp;++i)
	{
		vector3d_t inc_h=(line_b-line_a)/h_samp;
		point3d_t point=line_a+0.5*inc_h;
		for(int j=0;j<h_samp;++j)
		{
			PFLOAT vf=((PFLOAT)j)/((PFLOAT)h_samp);
			vector3d_t inc_v=inc_v_a*(1.0-vf)+inc_v_b*vf;
			points_vector[index]=point;
			jit[index].first=inc_h;
			jit[index].second=inc_v;
			index++;
			point=point+inc_h;
		}
		line_a=line_a+inc_v_a;
		line_b=line_b+inc_v_b;
	}
	return index;
}

areaLight_t::areaLight_t(const point3d_t &a,const point3d_t &b,
												const point3d_t &c,const point3d_t &d,
												int nsam, const color_t &col, 
												CFLOAT inte,int fsam,bool dum):
												points(nsam),jit(nsam),dummy(dum)
{
	samples = fillQuad(a, b, c, d, points, jit, nsam);
	direction = (b-a)^(d-a);
	direction.normalize();
	color = col;
	pow = inte;
	from = (a+b+c+d)*0.25;
	fsamples = fsam;
	corner = a;
	toX = b-a;
	toY = d-a;
}

color_t areaLight_t::illuminate(renderState_t &state,const scene_t &s,const surfacePoint_t sp,
															const vector3d_t &eye)const
{
	vector3d_t cdir=from-sp.P();
	color_t resul(0,0,0);
	if(dummy) return resul;
	vector3d_t dir,L;
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N=FACE_FORWARD(sp.Ng(),sp.N(),edir);
	const shader_t *sha= sp.getShader();
	
	dir=sp.P()-from;
	dir.normalize();
	PFLOAT plane_at=(sp.P()-from)*direction;

	if(plane_at<=0)
	{
		energy_t ene(direction, 0*color);
		return sha->fromLight(state,sp,ene,eye);
	}
	
	const void *oldorigin=state.skipelement;
	state.skipelement=sp.getOrigin();
	
	if (samples==1) {
		point3d_t sampleP = corner + toX*ourRandom() + toY*ourRandom();
		L = sampleP-sp.P();
		if ((L*N)<0) { state.skipelement=oldorigin; return color_t(0.0); }
		if (!s.isShadowed(state, sp, sampleP)) {
			dir = L;
			CFLOAT LD2 = dir.normLenSqr();
			LD2 = (LD2!=0) ? (1.0/LD2) : 1.0;
			energy_t ene(dir, (pow*color)*LD2);
			resul += sha->fromLight(state, sp, ene, eye);
		}
		state.skipelement = oldorigin;
		return (plane_at*resul);
	}
	
	int status=guessLight(state,s,sp,N);
	switch(status)
	{
		case PENUMBRA:
			for(int i=0;i<samples;++i)
			{
				PFLOAT dish=ourRandom()-0.5, disv=ourRandom()-0.5;
				point3d_t sampleP = points[i] + jit[i].first*dish + jit[i].second*disv;
				L = sampleP-sp.P();
				if ((L*N)<0) continue;
				if (!s.isShadowed(state, sp, sampleP))
				{
					dir = L;
					CFLOAT LD2 = dir.normLenSqr();
					LD2 = (LD2!=0) ? (1.0/LD2) : 1.0;
					energy_t ene(dir, (pow*color)*LD2);
					resul += sha->fromLight(state, sp, ene, eye);
				}
			}
			state.skipelement=oldorigin;
			return (plane_at*resul/ ((CFLOAT)samples));
		case LIGHT:
			for(int i=0;i<samples;++i)
			{
				L = points[i]-sp.P();
				dir = L;
				CFLOAT LD2 = dir.normLenSqr();
				LD2 = (LD2!=0) ? (1.0/LD2) : 1.0;
				energy_t ene(dir, (pow*color)*LD2);
				resul += sha->fromLight(state, sp, ene, eye);
			}
			state.skipelement=oldorigin;
			return (plane_at*resul/ ((CFLOAT)samples));
		case SHADOW:
			energy_t ene(direction, 0*color);
			state.skipelement=oldorigin;
			return sha->fromLight(state,sp,ene,eye);
	}
	state.skipelement=oldorigin;
	energy_t ene(direction, 0*color);
	return sha->fromLight(state,sp,ene,eye);
}

int areaLight_t::guessLight(renderState_t &state,const scene_t &s,const surfacePoint_t &sp,const vector3d_t &N)const
{
	if(fsamples==0) return PENUMBRA;
	bool luz=false,sombra=false;
	vector3d_t L;
	int randsamp;
	
	for(int i=0;i<fsamples;++i)
	{
		if(luz && sombra) return PENUMBRA;
		randsamp=rand()%samples;
		L=points[randsamp]-sp.P();
		if((L*N)<0)
			sombra=true;
		else if(s.isShadowed(state,sp,points[randsamp]))
			sombra=true;
		else
			luz=true;
	}
	if(luz && sombra) return PENUMBRA;
	if(luz) return LIGHT;
	return SHADOW;
}

quadEmitter_t::quadEmitter_t(const point3d_t &corn,const vector3d_t &tox,
		const vector3d_t &toy,const vector3d_t &dir,const color_t &c):
	corner(corn),toX(tox),toY(toy),direction(dir),color(c),scolor(c)
{
	NU=toX;
	NV=toY;
	NU.normalize();
	NV.normalize();
}

quadEmitter_t::~quadEmitter_t()
{
}

void quadEmitter_t::numSamples(int n) 
{
	scolor=color/(CFLOAT)n;
}

inline vector3d_t HemiVec_CONE(const vector3d_t &nrm,
				const vector3d_t &Ru, const vector3d_t &Rv,
				PFLOAT cosang, PFLOAT z1, PFLOAT z2)
{
  PFLOAT t1=2.0*M_PI*z1, t2=1.0-(1.0-cosang)*z2;
  return (Ru*cos(t1) + Rv*sin(t1))*sqrt(1.0-t2*t2) + nrm*t2;
}

void quadEmitter_t::getDirection(int num,point3d_t &p,vector3d_t &dir,color_t &c)const
{
	//dir=randomVectorCone(direction,0.01,ourRandom(),ourRandom());
	dir=HemiVec_CONE(direction,NU,NV,0.0001,ourRandom(),ourRandom());
	p=corner+toX*ourRandom()+toY*ourRandom();
	c=scolor*(direction*dir);
}

light_t *areaLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t a,b,c,d;
	color_t color;
	CFLOAT power=1.0;
	int samples=50,psamples=0;
	bool dummy=false;

	params.getParam("a",a);
	params.getParam("b",b);
	params.getParam("c",c);
	params.getParam("d",d);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("samples",samples);
	params.getParam("psamples",psamples);
	params.getParam("dummy",dummy);

	return new areaLight_t(a,b,c,d,samples,color,power,psamples,dummy);
}

pluginInfo_t areaLight_t::info()
{
	pluginInfo_t info;

	info.name="arealight";
	info.description="random sampled quad area light";

	info.params.push_back(buildInfo<POINT>("a","Corner of the quad"));
	info.params.push_back(buildInfo<POINT>("b","Corner of the quad"));
	info.params.push_back(buildInfo<POINT>("c","Corner of the quad"));
	info.params.push_back(buildInfo<POINT>("d","Corner of the quad"));
	info.params.push_back(buildInfo<COLOR>("color","Light color"));
	info.params.push_back(buildInfo<FLOAT>("power",0.0f,100000.0f,1.0f,
				"Light color"));
	info.params.push_back(buildInfo<INT>("samples",1,5000,50,
				"Number of samples for shadowing"));
	
	info.params.push_back(buildInfo<INT>("psamples",0,1000,0,
				"Number of samples to guess penumbra"));
	info.params.push_back(buildInfo<BOOL>("dummy",
				"Use only to shoot photons, no direct lighting"));

	return info;
		
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("arealight",areaLight_t::factory);
	std::cout<<"Registered arealight\n";
}

}
__END_YAFRAY
