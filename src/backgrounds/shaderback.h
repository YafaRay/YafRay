#ifndef __SHADERBACK_H
#define __SHADERBACK_H

#include "color.h"
#include "vector3d.h"
#include "params.h"
#include "background.h"
#include "metashader.h"

//#include "basictex.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif


__BEGIN_YAFRAY

// Pulls color from shader
class shader_Background_t: public background_t
{
	public:
		shader_Background_t(shader_t* in);
		//virtual ~shader_Background_t();
		virtual color_t operator () (const vector3d_t &dir, renderState_t &state, bool filtered) const;
		static background_t *factory(paramMap_t &,renderEnvironment_t &);
	protected:
		shader_t *input;
};

__END_YAFRAY

#endif
