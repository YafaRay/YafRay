/****************************************************************************
 *
 *      spherelight.cc : this is the implementation for spherelight
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

#include "spherelight.h"

__BEGIN_YAFRAY

sphereLight_t::sphereLight_t(const point3d_t &p, PFLOAT r, int nsam, int psam,
			const color_t &c, CFLOAT pw, int qmcm, bool dm, CFLOAT gli, CFLOAT glo, int glt)
{
	pos = p;
	rad = r;
	if (psam<0) psam=0;
	pred_samples = psam;
	samples = nsam+psam;	// samples is total number of samples of which pred_samples used to estimate shadowing
	// samples must be at least 1
	if (samples<1) {
		samples = 1;
		std::cerr << "[spherelight]: number of samples must be at least 1\n";
	}
	// if radius <= 0.01, assume light is pointlight, only one sample needed
	if (rad<=0.01) {
		rad = 0.0;
		std::cerr << "[spherelight]: radius of light very small, assuming pointlight\n";
		samples = 1;
	}
	samdiv = 1.0/(CFLOAT)samples;
	color = c*pw;
	qmc_method = qmcm;
	HSEQ = new Halton[2];
	HSEQ[0].setBase(2);
	HSEQ[1].setBase(3);
	dummy = dm;
	glow_int = gli;
	glow_ofs = glo;
	glow_type = glt;
}


color_t sphereLight_t::illuminate(renderState_t &state, const scene_t &s, const surfacePoint_t sp, const vector3d_t &eye) const
{
	if (dummy) return color_t(0.0);
	
	const shader_t *sha= sp.getShader();
	vector3d_t dir = pos-sp.P();
	CFLOAT Ld  = dir*dir;
	if (Ld!=0.0) Ld=1.0/Ld;
	dir.normalize();

	if (rad==0.0) {
		// pointlight
		const void *oldorigin = state.skipelement;
		state.skipelement = sp.getOrigin();
		color_t col(0.0);
		if (!s.isShadowed(state, sp, pos)) {
			col = sha->fromLight(state, sp, energy_t(dir, color*Ld), eye);
			if (glow_int>0) col += glow_int * color * getGlow(pos, sp, eye, glow_ofs, glow_type);
		}
		state.skipelement = oldorigin;
		return col;
	}

	PFLOAT du, dv;
	vector3d_t u, v;
	color_t totalcolor(0.0);

	createCS(dir, u, v);

	if (qmc_method) {
		HSEQ[0].setStart(ourRandomI());
		HSEQ[1].setStart(ourRandomI());
	}

	int sm, Ltot=0;
	point3d_t dp = pos;
	for (sm=0;sm<samples;sm++)
	{
		if ((pred_samples) && (sm==pred_samples)) {
			// if points totally lit or totally shadowed, stop now
			if (Ltot==sm) {
				totalcolor *= (1.f/(CFLOAT)sm);
				if (glow_int>0) totalcolor += glow_int * color * getGlow(pos, sp, eye, glow_ofs, glow_type);
				return totalcolor;
			}
			else if (Ltot==0) return color_t(0.0);	// noglo
		}
		ShirleyDisk(HSEQ[0].getNext(), HSEQ[1].getNext(), du, dv);
		dp = pos + rad*(du*u + dv*v);
		dir = dp - sp.P();
		Ld = dir*dir;
		if (Ld!=0.0) Ld=1.0/Ld;
		dir.normalize();
		const void *oldorigin=state.skipelement;
		state.skipelement=sp.getOrigin();
		if (!s.isShadowed(state, sp, dp)) {
			Ltot++;
			totalcolor += sha->fromLight(state, sp, energy_t(dir, color*Ld), eye);
		}
		state.skipelement=oldorigin;
	}
	totalcolor *= samdiv;
	if (glow_int>0) totalcolor += ((CFLOAT)Ltot*samdiv) * glow_int * color * getGlow(pos, sp, eye, glow_ofs, glow_type);
	return totalcolor;
}


sphereEmitter_t::sphereEmitter_t(const color_t &c, const point3d_t &p, PFLOAT rad)
{
	color = c;
	pos = p;
	radius = rad;
}

sphereEmitter_t::~sphereEmitter_t()
{
}

void sphereEmitter_t::numSamples(int n)
{
	lcol = color/((CFLOAT)n);
}

void sphereEmitter_t::getDirection(int num, point3d_t &p, vector3d_t &dir, color_t &c) const
{
	dir = RandomSpherical();
	p = pos + radius*dir;
	c = lcol;
}


light_t *sphereLight_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	color_t col(1.0);
	PFLOAT r = 1;
	point3d_t p;
	CFLOAT pw = 1;
	int nsam=16, psam=0;
	int qmcm = 0;
	bool dm = false;

	params.getParam("from", p);
	params.getParam("radius", r);
	params.getParam("color", col);
	params.getParam("power",  pw);
	params.getParam("samples", nsam);
	params.getParam("psamples", psam);
	params.getParam("qmc_method", qmcm);
	params.getParam("dummy", dm);

	// glow params
	CFLOAT gli=0, glo=0;
	int glt=0;
	params.getParam("glow_intensity", gli);
	params.getParam("glow_type", glt);
	params.getParam("glow_offset", glo);

	return new sphereLight_t(p, r, nsam, psam, col, pw, qmcm, dm, gli, glo, glt);
}

pluginInfo_t sphereLight_t::info()
{
	pluginInfo_t info;

	info.name = "spherelight";
	info.description="spherical arealight";

	info.params.push_back(buildInfo<POINT>("from", "Position of the spherelight"));
	info.params.push_back(buildInfo<FLOAT>("radius", 0.0f, 100000.0f, 1.0f, "Radius of the spherelight"));
	info.params.push_back(buildInfo<COLOR>("color", "Light color"));
	info.params.push_back(buildInfo<FLOAT>("power", 0.0f, 100000.0f, 1.0f, "Light intensity"));
	info.params.push_back(buildInfo<INT>("samples",1,5000,50, "Number of shadow samples"));
	info.params.push_back(buildInfo<INT>("psamples",0,1000,0, "Minimum of samples to estimate shadowing"));
	info.params.push_back(buildInfo<INT>("qmc_method",0, 1, 0, "The sampling method"));
	info.params.push_back(buildInfo<BOOL>("dummy", "Use only to shoot photons, no direct lighting"));

	return info;

}

extern "C"
{

YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("spherelight", sphereLight_t::factory);
	std::cout<<"Registered spherelight\n";
}

}
__END_YAFRAY
