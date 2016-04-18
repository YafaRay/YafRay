
#include "metashader.h"

__BEGIN_YAFRAY

color_t shaderNode_t::fromRadiosity(renderState_t &state,const surfacePoint_t &sp,const energy_t &ene,
	                              const vector3d_t &eye)const 
{
	return color_t(0.0);
}

color_t shaderNode_t::fromLight(renderState_t &state,const surfacePoint_t &sp,const energy_t &ene,
																const vector3d_t &eye)const 
{
	return color_t(0.0);
}

color_t shaderNode_t::fromWorld(renderState_t &state,const surfacePoint_t &sp,const scene_t &scene,
															const vector3d_t &eye)const 
{
	return color_t(0.0);
}

const color_t shaderNode_t::getDiffuse(renderState_t &state,
					const surfacePoint_t &sp,const vector3d_t &eye) const 
{
	return color_t(0.0);
}

void shaderNode_t::displace(renderState_t &state,surfacePoint_t &sp, 
				const vector3d_t &eye, PFLOAT res) const 
{
}

shaderNode_t::~shaderNode_t() 
{
}


__END_YAFRAY

