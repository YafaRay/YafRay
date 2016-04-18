#ifndef __IMAGEBACKG_H
#define __IMAGEBACKG_H

#include "basictex.h"
#include "background.h"
#include "params.h"
#include "matrix4.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY
// image
class imageBackground_t: public background_t
{
	public:
		enum mappingType {IBG_SPHERE, IBG_ANGULAR, IBG_TUBE};
		imageBackground_t(const char* fname, const std::string &intp, CFLOAT bri_adj, const matrix4x4_t &m,
					mappingType mt=IBG_SPHERE, bool prefilt=false);
		virtual ~imageBackground_t();
		virtual color_t operator() (const vector3d_t &dir, renderState_t &state, bool filtered=false) const;

		static background_t *factory(paramMap_t &,renderEnvironment_t &);
	protected:
		mappingType mType;
		texture_t* img;
		CFLOAT brightness_scale;
		matrix4x4_t mtx;
};

__END_YAFRAY

#endif
