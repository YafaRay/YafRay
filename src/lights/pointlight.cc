/****************************************************************************
 *
 * 			pointlight.cc: Point light implementation 
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

#include "pointlight.h"

__BEGIN_YAFRAY

color_t pointLight_t::illuminate(renderState_t &state, const scene_t &s,
				const surfacePoint_t sp, const vector3d_t &eye) const
{
	vector3d_t L = from-sp.P();
	vector3d_t dir = L;
	dir.normalize();
	const shader_t *sha = sp.getShader();

	CFLOAT id2 = L*L;
	if (id2!=0.f) id2=1.f/id2;
	color_t col(0.f);
	const void *oldorigin = state.skipelement;
	state.skipelement = sp.getOrigin();
	if (cast_shadows ? !s.isShadowed(state, sp, from) : true)
		col = sha->fromLight(state, sp, energy_t(dir, color*id2), eye);
	state.skipelement = oldorigin;

	if (glow_int>0) col += glow_int * color * getGlow(from, sp, eye, glow_ofs, glow_type);
	return col;
}

pointEmitter_t::pointEmitter_t(const point3d_t &f, const color_t &c): from(f), color(c)
{
}

pointEmitter_t::~pointEmitter_t()
{
}

void pointEmitter_t::numSamples(int n)
{
	lcol = color/((CFLOAT)n);
}

vector3d_t dummy(0,0,1);

void pointEmitter_t::getDirection(int num, point3d_t &p, vector3d_t &dir, color_t &c) const
{
	dir = RandomSpherical();
	p = from;
	c = lcol;
}

light_t *pointLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t from;
	color_t color(1.0);
	CFLOAT power = 1.0;
	bool shadow = true;

	params.getParam("from",from);
	params.getParam("color",color);
	params.getParam("power",power);
	params.getParam("cast_shadows",shadow);

	// glow params
	CFLOAT gli=0, glo=0;
	int glt=0;
	params.getParam("glow_intensity", gli);
	params.getParam("glow_type", glt);
	params.getParam("glow_offset", glo);

	return new pointLight_t(from, color, power, shadow, gli, glo, glt);
}

pluginInfo_t pointLight_t::info()
{
	pluginInfo_t info;

	info.name="pointlight";
	info.description="Simple point direct light";

	info.params.push_back(buildInfo<POINT>("from","Light position"));
	info.params.push_back(buildInfo<COLOR>("color","Light color"));
	info.params.push_back(buildInfo<FLOAT>("power",0,10000,1,"Light power"));
	info.params.push_back(buildInfo<BOOL>("cast_shadows","Whenever to cast shadows"));

	return info;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("pointlight",pointLight_t::factory);
	std::cout<<"Registered pointlight\n";
}

}
__END_YAFRAY
