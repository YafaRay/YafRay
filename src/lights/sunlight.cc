/****************************************************************************
 *
 * 			sunlight.cc: sun light implementation, directional light
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

#include "sunlight.h"

__BEGIN_YAFRAY

sunLight_t::sunLight_t(const point3d_t &dir, const color_t &c, CFLOAT pwr, bool shd):cast_shadows(shd)
{
	direction.set(dir.x, dir.y, dir.z);
	direction.normalize();
	color = c * pwr;
}


color_t sunLight_t::illuminate(renderState_t &state,const scene_t &s, 
		const surfacePoint_t sp, const vector3d_t &eye) const
{
	const shader_t *sha = sp.getShader();
	if (cast_shadows) 
	{
		const void *oldorigin=state.skipelement;
		state.skipelement=sp.getOrigin();
		if (!s.isShadowed(state,sp, direction)) 
		{
			state.skipelement=oldorigin;
			energy_t ene(direction, color);
			return sha->fromLight(state,sp, ene, eye);
		}
		state.skipelement=oldorigin;
		return color_t(0, 0, 0);
	}
	else {
		energy_t ene(direction, color);
		return sha->fromLight(state,sp, ene, eye);
	}

}

light_t *sunLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t from;
	color_t color(1.0);
	CFLOAT power = 1.0;
	bool shadows = true;

	// sunlight has no position, so from is interpreted as a direction.
	params.getParam("from", from);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("cast_shadows", shadows);

	return new sunLight_t(from, color, power, shadows);
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("sunlight",sunLight_t::factory);
	std::cout<<"Registered sunlight\n";
}

}
__END_YAFRAY
