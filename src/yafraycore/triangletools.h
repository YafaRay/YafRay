#ifndef __TRIANGLETOOLS_H
#define __TRIANGLETOOLS_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "vector3d.h"
#include "triangle.h"
#include "bound.h"

__BEGIN_YAFRAY

#define AXISX 0
#define AXISY 1
#define AXISZ 2



struct trianglePosition_t
{
	enum
	{
		NONE,
		LOWER,
		HIGHER,
		INTERSECT
	};
};

int expensivePosition(const triangle_t &tri,const bound_t &bound,PFLOAT Z,int axis);
int cheapPosition(const triangle_t &tri,const bound_t &bound,PFLOAT Z,int axis);

PFLOAT maximize(const std::vector<triangle_t *> faces,const bound_t &bound,int axis);
PFLOAT minimize(const std::vector<triangle_t *> faces,const bound_t &bound,int axis);

__END_YAFRAY

#endif
