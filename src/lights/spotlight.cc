/****************************************************************************
 *
 * 			spotlight.cc: spot light implementation
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

#include "spotlight.h"

__BEGIN_YAFRAY

// renderman smoothstep function, might be useful elsewhere
inline PFLOAT smoothstep(PFLOAT min, PFLOAT max, PFLOAT value)
{
  if (value<=min) return 0;
  if (value>=max) return 1;
  PFLOAT v = (value-min) / (max-min);
  return v*v*(3.0-2.0*v);
}

color_t spotLight_t::illuminate(renderState_t &state,const scene_t &s, 
		const surfacePoint_t sp, const vector3d_t &eye) const
{
	vector3d_t L = from-sp.P();
	CFLOAT dist_atten = L*L;
	if (dist_atten!=0) dist_atten = 1.0/dist_atten;
	L.normalize();
	const shader_t *sha= sp.getShader();
	CFLOAT atten, ca = L * dir;
	bool skipHalo=state.rayDivision>1;
	// only calculate when within light cone
	if (ca>=cosout) {
		if(use_map)
		{
			atten = pow(ca, beamDist) * dist_atten * smoothstep(cosout, cosin, ca) * power;
			energy_t ene(L, atten*getMappedLight(sp));
			if (halo && !skipHalo)
				return sha->fromLight(state,sp, ene, eye) + getVolume(s,sp,eye);
			else return sha->fromLight(state,sp, ene, eye);
		}
		else
		{
			const void *oldorigin=state.skipelement;
			state.skipelement=sp.getOrigin();
			bool islit = (cast_shadows)?(!s.isShadowed(state,sp, from)):true;
			state.skipelement=oldorigin;
			if (islit)
			{
				atten = pow(ca, beamDist) * dist_atten * smoothstep(cosout, cosin, ca) * power;
				energy_t ene(L, atten*color);
				if(halo) return sha->fromLight(state,sp, ene, eye) + getVolume(s,sp,eye);
				else return sha->fromLight(state,sp, ene, eye);
			}
		}
	}
	energy_t ene(dir, color_t(0.0));
	if (halo && !skipHalo)
		return sha->fromLight(state,sp, ene, eye) + getVolume(s,sp,eye);
	return sha->fromLight(state,sp, ene, eye);
}

using namespace std;

inline color_t spotLight_t::getFog(PFLOAT d)const
{
	CFLOAT fgi = exp(-d*fden);
	return (1.0-fgi)*fog;
}

color_t spotLight_t::getVolume(const scene_t &s, const surfacePoint_t sp, const vector3d_t &eye) const
{
	if (!use_map) return color_t(0.0);
	point3d_t rstart = sp.P()+eye;
	point3d_t rstop = sp.P();

	vector3d_t vi = rstart-from;
	vector3d_t vf = rstop-from;
	rstart.set(vi*vx, vi*vy, vi*ndir);
	rstop.set(vf*vx, vf*vy, vf*ndir);

	vector3d_t ray = rstop-rstart;
	PFLOAT dist = ray.normLen();
	PFLOAT T2 = tana*tana;
	PFLOAT A = ray.z*ray.z*T2-ray.x*ray.x-ray.y*ray.y;
	PFLOAT B = 2*rstart.z*T2*ray.z-2*rstart.x*ray.x-2*rstart.y*ray.y;
	PFLOAT C = rstart.z*rstart.z*T2-rstart.x*rstart.x-rstart.y*rstart.y;

	PFLOAT sq = B*B-4*A*C;

	bool iin=false, fin=false;
	vi.normalize();
	if (vi*ndir>cosa) iin=true;
	vf.normalize();
	if (vf*ndir>cosa) fin=true;

	color_t res(0.0);
	if(sq<0) return color_t(0.0);
	PFLOAT D, D2;
	if (A!=0.0)
	{
		D = (-B-sqrt(sq))/(2*A);
		D2 = (sqrt(sq)-B)/(2*A);
		if (D>D2) swap(D,D2);
	}
		
	if (iin && fin) return getFog(dist)*sumLine(rstart, rstop);
	if (iin)
	{
		if (A==0.0) return getFog(dist)*color*power;
		if (D<0) D=D2;
		return getFog(D)*sumLine(rstart,rstart+D*ray);
	}
	if (fin)
	{
		if(A==0.0) return getFog(dist)*color*power;
		if (D<0) D=D2;
		return getFog(dist-D)*sumLine(rstart+D*ray, rstop);
	}
		
	if (A==0.0) return res;

	if (D<0) return color_t(0.0);
	if (D>dist) return color_t(0.0);
	if (D2>dist) D2=dist;
	
	rstart = rstart+ray*D;
	if(rstart.z<0) return color_t(0.0);
	
	return getFog(D2-D)*sumLine(rstart, rstart+(D2-D)*ray);
}

color_t spotLight_t::sumLine(const point3d_t &s,const point3d_t &e)const
{
	vector3d_t start=toVector(s), end=toVector(e);
	vector3d_t initpos=start, ldir=end-start;

	start.normalize();
	end.normalize();
	PFLOAT dist = ldir.normLen();
	PFLOAT light=0;
	PFLOAT sx=halfres + halfres*start.x*isina;
	PFLOAT sy=halfres + halfres*start.y*isina;
	PFLOAT ex=halfres + halfres*end.x*isina;
	PFLOAT ey=halfres + halfres*end.y*isina;
	PFLOAT bix=ey-sy, biy=sx-ex, L=sqrt(bix*bix+biy*biy);
	if (L!=0.0) L=1.0/L;
	bix *= L;
	biy *= L;

	PFLOAT curdist = ourRandom()*stepsize;
	int totsam = 0;
	while (curdist<dist)
	{
		vector3d_t pos = initpos + ldir*curdist;
		curdist += stepsize;
		PFLOAT d2 = pos.normLenSqr();
		PFLOAT d = sqrt(d2);
		if (d2!=0.0) d2=1.0/d2;
		PFLOAT x = halfres + halfres*pos.x*isina, y = halfres + halfres*pos.y*isina;
		if (hblur!=0.0)
		{
			PFLOAT r2 = ourRandom();
			PFLOAT dis = halfres*hblur*r2;
			x += bix*dis;
			y += biy*dis;
		}
		PFLOAT ca=pos.z;
		if((shadow((int)x,(int)y)>d) || (shadow((int)x,(int)y)<0)) {
			light += pow(ca, beamDist)*smoothstep(cosout, cosin, ca)*d2;
			totsam++;
		}
	}
	if (totsam) light /= (CFLOAT)totsam;
	return color*power*light;
}

color_t spotLight_t::getMappedLight(const surfacePoint_t &sp)const
{
	if(!use_map) return color_t(0.0);

	vector3d_t tvP=sp.P()-from;
	vector3d_t vP(tvP*vx, tvP*vy, tvP*ndir);
	vector3d_t vu(sp.NU()*vx, sp.NU()*vy, sp.NU()*ndir);
	vector3d_t vv(sp.NV()*vx, sp.NV()*vy, sp.NV()*ndir);
	PFLOAT D = vP.z*tana*sblur;
	
	color_t light(0.0);

	int sqs = int(sqrt((PFLOAT)shadow_samples));
	if (sqs<1) sqs=1;
	PFLOAT dv = sqs;
	if (dv!=0) dv = 1.0/sqs;
	for(int x=0;x<sqs;++x) {
		for (int y=0;y<sqs;++y) {
			PFLOAT r1=(x+ourRandom())*dv-0.5, r2=(y+ourRandom())*dv-0.5;
			vector3d_t pos = vP + D*(vu*r1 + vv*r2);
			PFLOAT d = pos.normLen();
			PFLOAT _x=halfres+halfres*pos.x*isina, _y=halfres+halfres*pos.y*isina;
			if((shadow((int)_x,(int)_y)>(d-0.3)) || (shadow((int)_x,(int)_y)<0)) light += color;
		}
	}
	return light/((CFLOAT)(sqs*sqs));
}

void spotLight_t::buildShadowMap(scene_t &scene)
{
	cerr << "Building volumetric shadow map... ";
	cerr.flush();
	surfacePoint_t sp;
	renderState_t state;
	for(int y=0;y<resolution;++y)
	{
		PFLOAT leny = 2*sina*((PFLOAT)y-halfres)/(PFLOAT)resolution;
		for(int x=0;x<resolution;++x)
		{
			PFLOAT lenx = 2*sina*((PFLOAT)x - halfres)/(PFLOAT)resolution;
			PFLOAT lenz = sqrt(1.0 - lenx*lenx - leny*leny);
			vector3d_t ray = ndir*lenz + vx*lenx + vy*leny;
			if (!scene.firstHit(state, sp, from, ray, true))
				shadow(x,y) = -1;
			else
				shadow(x,y) = sp.Z()+scene.selfBias();
		}
	}
	cerr << "OK\n";
}


void spotLight_t::setMap(int res, int ss, PFLOAT b)
{
	use_map = true;
	shadow_map.resize(res*res);
	resolution = res;
	halfres = resolution*0.5;
	noshadow = 0;
	sblur = b;
	shadow_samples = ss;
}

void spotLight_t::setHalo(const color_t &f, CFLOAT d, PFLOAT b, PFLOAT s)
{
	halo = true;
	hblur = b;
	stepsize = s;
	fog = f;
	fden = d;
}

spotEmitter_t::spotEmitter_t(const point3d_t &f, const vector3d_t &dir,
		PFLOAT ca, const color_t &c): from(f), direction(dir), cosa(ca), color(c), scolor(c)
{
	createCS(dir, diru, dirv);
}

spotEmitter_t::~spotEmitter_t()
{
}

void spotEmitter_t::numSamples(int n) 
{
	scolor = color/(CFLOAT)n;
}

void spotEmitter_t::getDirection(int num,point3d_t &p, vector3d_t &dir, color_t &c) const
{
	dir = randomVectorCone(direction, diru, dirv, cosa, ourRandom(), ourRandom());
	p = from;
	c = scolor;
}

light_t *spotLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t from, to;
	color_t color(1.0);
	CFLOAT power = 1.0;
	bool shadows = true;
	PFLOAT size = 45.0;
	PFLOAT blend = 0.15;
	PFLOAT falloff = 2.0;
	bool halo = false;
	int res = 512;
	int shadow_samples;
	PFLOAT stepsize = 1;	//replacement for 'samples'
	PFLOAT hblur=0, sblur=0;

	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("size", size);
	params.getParam("blend", blend);
	params.getParam("beam_falloff", falloff);
	params.getParam("cast_shadows", shadows);
	params.getParam("halo", halo);
	params.getParam("res", res);
	shadow_samples = res;
	
	if (params.getParam("samples", stepsize)) {
		cout << "[spotlight]: 'samples' deprecated, use 'stepsize' instead" << endl;
		// convert from old integer samples value to some reasonable stepsize
		if (stepsize<1) stepsize=1;
		stepsize = 1.0/sqrt(stepsize);
	}
	params.getParam("stepsize", stepsize);
	if (stepsize<=9.765625e-4) stepsize=9.765625e-4;	// 1/1024, probably small enough

	params.getParam("shadow_samples", shadow_samples);
	if (shadow_samples<1) shadow_samples=1;
	params.getParam("halo_blur", hblur);
	params.getParam("shadow_blur", sblur);

	spotLight_t *spot=new spotLight_t(from, to, color, power, size, blend, falloff, shadows);
	if(halo)
	{
		color_t fog(0.0);
		CFLOAT fden = 0.0;
		params.getParam("fog", fog);
		params.getParam("fog_density", fden);
		spot->setMap(res, shadow_samples, sblur);
		spot->setHalo(fog, fden, hblur, stepsize);
	}
	return spot;
}

pluginInfo_t spotLight_t::info()
{
	pluginInfo_t info;

	info.name = "spotlight";
	info.description = "Directional spot light";

	info.params.push_back(buildInfo<POINT>("from", "Light position"));
	info.params.push_back(buildInfo<POINT>("to", "Light target"));
	info.params.push_back(buildInfo<COLOR>("color", "Light color"));
	info.params.push_back(buildInfo<FLOAT>("power", 0, 10000, 1, "Light power"));
	info.params.push_back(buildInfo<FLOAT>("size", 0, 180, 45, "Aperture of the cone"));
	
	info.params.push_back(buildInfo<BOOL>("cast_shadows", "Whenever to cast shadows"));

	return info;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("spotlight", spotLight_t::factory);
	std::cout << "Registered spotlight\n";
}

}
__END_YAFRAY
