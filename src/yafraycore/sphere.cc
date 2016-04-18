/****************************************************************************
 *
 * 			sphere.cc: Sphere object implementation 
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
#include "sphere.h"

__BEGIN_YAFRAY

sphere_t::sphere_t(const point3d_t &center,PFLOAT _radius)
{
	radius=_radius;
	R2=radius*radius;
	c=center;
	north=c;
	north.z+=radius;
	shader=NULL;
	recalcBound();
}


bool sphere_t::shoot(renderState_t &state,surfacePoint_t &where,
		const point3d_t &from,
		const vector3d_t &ray,bool shadow,PFLOAT dis) const
{
	vector3d_t vf=from-c;
	PFLOAT ea=ray*ray;
	PFLOAT eb=2.0*vf*ray;
	PFLOAT ec=vf*vf-R2;
	PFLOAT osc=eb*eb-4.0*ea*ec;
	if(osc<0) return false;
	osc=sqrt(osc);
	PFLOAT sol1=(-eb-osc)/(2.0*ea);
	PFLOAT sol2=(-eb+osc)/(2.0*ea);
	PFLOAT sol=sol1;
	if(sol<=0.0) sol=sol2;
	if(sol<=0.0) return false;
	if(shadow && ((sol<dis) || (dis<0))) return true;
	point3d_t hit=from+sol*ray;
	vector3d_t normal=hit-c;
	normal.normalize();
	surfacePoint_t temp((object3d_t *)this, hit,hit, normal,normal, -1, -1, color_t(0.0), sol, shader);
	temp.setOrigin(this);
	where=temp;
	return true;
}

void sphere_t::recalcBound()
{
	PFLOAT minx=c.x-radius-0.000001;
	PFLOAT miny=c.y-radius-0.000001;
	PFLOAT minz=c.z-radius-0.000001;
	PFLOAT maxx=c.x+radius+0.000001;
	PFLOAT maxy=c.y+radius+0.000001;
	PFLOAT maxz=c.z+radius+0.000001;
	bound.set(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}


__END_YAFRAY
