#ifndef __SSS_H
#define __SSS_H

#include "metashader.h"
#include "basictex.h"
#include "params.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY


class sssNode_t : public shaderNode_t
{
	public:
		sssNode_t(const color_t &c,PFLOAT r,int s);

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const;

		virtual ~sssNode_t() {};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		color_t sampleObject(renderState_t &state,const object3d_t *obj,
												 const point3d_t &from,const vector3d_t &ray,
												 const point3d_t &outpoint,
												 CFLOAT &W,const scene_t *scene)const;
		point3d_t getSamplingPoint(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye)const;

		color_t color;
		PFLOAT radius,halfradius,farradius,exponent,expinv;
		int samples,sqrtsamples;
};


__END_YAFRAY
#endif
