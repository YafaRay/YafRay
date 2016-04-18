/****************************************************************************
 *
 * 		  hemilight.cc: Hemi (Sky/Environment) light implementation
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

#include "hemilight.h"
using namespace std;

__BEGIN_YAFRAY

#define WARNING cerr<<"[hemilight]: "
#define INFO cerr<<"[hemilight]: "

hemiLight_t::hemiLight_t(int nsam, const color_t &c, CFLOAT pwr, PFLOAT mdist, bool usebg, bool useqmc)
	: samples(nsam), color(c), power(pwr), maxdistance(mdist), use_background(usebg), use_QMC(useqmc)
{
	if (use_QMC) {
		// two dim only
		HSEQ = new Halton[2];
		HSEQ[0].setBase(2);
		HSEQ[1].setBase(3);
	}
	else {
		// samples must be integer squared value for jittered sampling
		int g = int(sqrt((float)samples));
		g *= g;
		if (g!=samples) {
			cout << "Samples value changed from " << samples << " to " << g << endl;
			samples = g;
		}
		grid = int(sqrt((float)samples));
		gridiv = 1.0/PFLOAT(grid);
		gridiv2pi = gridiv * 2.0 * M_PI;
		HSEQ = NULL;
	}
	//sampdiv = 2.0*power/(PFLOAT)samples;	// unif.hemi pdf=2
	sampdiv = power/(PFLOAT)samples;
}


color_t hemiLight_t::illuminate(renderState_t &state,const scene_t &sc, const surfacePoint_t sp,
				const vector3d_t &eye) const
{
	vector3d_t dir;
	const shader_t *sha = sp.getShader();
	color_t totalcolor(0.0);
	vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), eye);

	energy_t fake(N, color_t(1.0));
	if (maxAbsDiff(sha->fromLight(state, sp, fake, eye), color_t(0.0))<0.05f) return totalcolor;

	const void *oldorigin=state.skipelement;
	state.skipelement=sp.getOrigin();

	//CFLOAT totalocc = 0;
	//vector3d_t avgdir(0, 0, 0);
	for (int sm=0;sm<samples;sm++)
	{
		dir = getNext(N, sm, sp.NU(), sp.NV());
		CFLOAT occ = dir*N;
		if ((occ>0) && (!((maxdistance>0) ?
					sc.isShadowed(state, sp, sp.P()+maxdistance*dir) :
					sc.isShadowed(state, sp, dir))))
		{
			
			if (use_background)
				totalcolor += sc.getBackground(dir, state, true) * occ;
			else
				totalcolor += color * occ;
			
			//totalocc += occ;
			//avgdir += dir;
		}
	}
	state.skipelement = oldorigin;
	return sampdiv * totalcolor * sha->fromLight(state, sp, fake, eye);
	/*
	avgdir.normalize();
	fake.dir = avgdir;
	if (use_background) return sampdiv * totalocc * sha->fromLight(state, sp, fake, eye) * sc.getBackground(avgdir, state, true);
	return sampdiv * totalocc * sha->fromLight(state, sp, fake, eye) * color;
	*/
}

// returns new hemi vector with uniform distribution
vector3d_t hemiLight_t::getNext(const vector3d_t &normal, int cursample, const vector3d_t &Ru, const vector3d_t &Rv) const
{
	PFLOAT z1, z2;
	if (use_QMC) {
		z1=HSEQ[0].getNext();  z2=HSEQ[1].getNext()*2.0*M_PI;
	}
	else {
		z1 = (PFLOAT(cursample / grid) + ourRandom()) * gridiv;
		z2 = (PFLOAT(cursample % grid) + ourRandom()) * gridiv2pi;
	}
	return (Ru*cos(z2) + Rv*sin(z2))*sqrt(1.0-z1*z1) + normal*z1;
}

light_t *hemiLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	color_t color;
	CFLOAT power = 1.0;
	int samples = 16;
	PFLOAT mdist = -1;	// infinite default
	bool use_background = false;
	bool useqmc = false;

	if (!params.getParam("color", color)) {
		INFO << "No color set for hemilight, using scene background color instead.\n";
		use_background = true;
	}

	params.getParam("power", power);
	params.getParam("samples", samples);
	if (samples<1) {
		WARNING << "Samples value too low, minimum is one\n";
		samples = 1;
	}
	params.getParam("use_QMC", useqmc);
	
	params.getParam("maxdistance", mdist);
	return new hemiLight_t(samples, color, power, mdist, use_background, useqmc);
}

pluginInfo_t hemiLight_t::info()
{
	pluginInfo_t info;

	info.name="hemilight";
	info.description="Soft sky (background) illumination";

	info.params.push_back(buildInfo<COLOR>("color","If given this is used to \
				color the light, otherwise, backgorund color is used"));
	info.params.push_back(buildInfo<FLOAT>("power",0,10000,1,"Power of the light"));
	info.params.push_back(buildInfo<INT>("samples",1,5000,16,"Shadow samples, \
				the higher the slower and the better"));
	info.params.push_back(buildInfo<BOOL>("use_QMC","Whenever to use quasi montecarlo"));
	return info;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("hemilight",hemiLight_t::factory);
	std::cout<<"Registered hemilight\n";
}

}
__END_YAFRAY

