#ifndef __METASHADER_H
#define __METASHADER_H

#include "shader.h"
#include "texture.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT shaderNode_t : public shader_t
{
	public:
		virtual color_t fromRadiosity(renderState_t &state, const surfacePoint_t &sp,
					const energy_t &ene, const vector3d_t &eye) const;
		virtual color_t fromLight(renderState_t &state, const surfacePoint_t &sp,
					const energy_t &ene, const vector3d_t &eye) const;
		virtual color_t fromWorld(renderState_t &state, const surfacePoint_t &sp,
					const scene_t &scene, const vector3d_t &eye) const;
		virtual const color_t getDiffuse(renderState_t &state,
					const surfacePoint_t &sp, const vector3d_t &eye) const;
		virtual void displace(renderState_t &state, surfacePoint_t &sp,
					const vector3d_t &eye, PFLOAT res) const;
		virtual ~shaderNode_t();
};

__END_YAFRAY
#endif
