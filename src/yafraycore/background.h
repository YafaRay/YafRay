#ifndef __BACKGROUND_H
#define __BACKGROUND_H

#include "color.h"
#include "vector3d.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY

struct renderState_t;

class YAFRAYCORE_EXPORT background_t
{
	public:
		virtual color_t operator() (const vector3d_t &dir, renderState_t &state, bool filtered=false) const=0;
		virtual ~background_t() {};
};

__END_YAFRAY

#endif
