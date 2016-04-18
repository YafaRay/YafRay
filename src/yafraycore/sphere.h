/****************************************************************************
 *
 * 			sphere.h: Sphere object api 
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
#ifndef __SPHERE_H
#define __SPHERE_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "object3d.h"
#include "vector3d.h"

__BEGIN_YAFRAY
class YAFRAYCORE_EXPORT sphere_t : public object3d_t
{
	public:
		sphere_t(const point3d_t &center,PFLOAT _radius);
		virtual ~sphere_t() {};
		virtual int type() const { return SPHERE;};
		virtual void transform(const matrix4x4_t &m) {};
		virtual point3d_t toObject(const point3d_t &p) const { return p; }
		virtual vector3d_t toObjectRot(const vector3d_t &v) const { return v; }
		virtual point3d_t toObjectOrco(const point3d_t &p) const { return p; }
		virtual bool shoot(renderState_t &state,surfacePoint_t &where, const point3d_t &from,
				const vector3d_t &ray,bool shadow=false,PFLOAT dis=-1) const;
		void recalcBound();
		virtual bound_t getBound() const {return bound;};
	protected:
		sphere_t(const sphere_t &s) {}; //forbiden
		point3d_t c;
		point3d_t north;
		PFLOAT radius;
		PFLOAT R2;
		bound_t bound;
};

__END_YAFRAY
#endif
