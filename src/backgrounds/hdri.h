#ifndef __HDRI_H
#define __HDRI_H

#include "color.h"
#include "vector3d.h"
#include "HDR_io.h"
#include "params.h"
#include "background.h"

__BEGIN_YAFRAY

// HDR image
class HDRI_Background_t: public background_t
{
	public:
		HDRI_Background_t(const char* fname, GFLOAT expadj, bool mp);
		virtual ~HDRI_Background_t();
		virtual color_t operator() (const vector3d_t &dir, renderState_t &state, bool filtered=false) const;
		static background_t *factory(paramMap_t &,renderEnvironment_t &);
	protected:
		HDRimage_t* img;
		bool mapProbe;
};

__END_YAFRAY

#endif
