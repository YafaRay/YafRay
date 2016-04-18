#ifndef __MIXBLOCKS_H
#define __MIXBLOCKS_H

#include "metashader.h"
#include "params.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY

class mixModeNode_t : public shaderNode_t
{
	public:
		
		enum mixModes 
		{
			ADDITIVE,
			SUBTRACTIVE,
			MMULTIPLY,
			AVERAGE,
			SCREEN,
			EXCLUSION,
			SOFTLIGHT,
			DIFFERENCE,
			NEGATION,
			STAMP,
			COLORDODGE,
			COLORBURN,
			REFLECT,
			FREEZE,
			LIGHTEN,
			DARKEN,
			OVERLAY,
			HARDLIGHT
		};
		
		mixModeNode_t(const shader_t *r,const shader_t *t,mixModes ty):input1(r),
			input2(t),type(ty) 	{};

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene)const;

		static void fillModes();
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				renderEnvironment_t &);
	protected:
			const shader_t *input1,*input2;
			mixModes type;
};

__END_YAFRAY
#endif
