/****************************************************************************
 *
 * 			triangle.h: Face representation and manipulation api 
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
#ifndef __TRIANGLE_H
#define __TRIANGLE_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "surface.h"
#include "vector3d.h"
#include "matrix4.h"

#include <vector>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT triangle_t
{
	public:
		triangle_t(point3d_t *va,point3d_t *vb,point3d_t *vc);
		triangle_t();
		~triangle_t() {};
		void setVertices(point3d_t *va,point3d_t *vb,point3d_t *vc);
		bool itsZP() {return (normal.z==0);};
		bool itsYP() {return (normal.y==0);};
		bool itsXP() {return (normal.x==0);};
		bool Z_hit();
		bool hit(const point3d_t &from,const vector3d_t &ray)
		{
			const vector3d_t va=(*a)-from,vb=(*b)-from,vc=(*c)-from;
			vector3d_t r;
			if((ray*normal)<0) r=-ray;
			else r=ray;
			if( ((va^vb)*r)<0 ) return false;
			if( ((vb^vc)*r)<0 ) return false;
			if( ((vc^va)*r)<0 ) return false;
			return true;
		}
		/*
		//Tomas Moller and Ben Trumbore ray intersection scheme (simplified)
		bool hit(const point3d_t &from, const vector3d_t &ray)
		{
			 vector3d_t edge1, tvec, pvec;
			 PFLOAT det,inv_det,u,v;
			 edge1=(*b)-(*a);
			 pvec= ray^((*c)-(*a));
			 det = edge1*pvec;
			 if ((det>-MIN_RAYDIST) && (det<MIN_RAYDIST)) return false;
			 inv_det = 1.0 / det;
			 tvec=from-(*a);
			 u = (tvec*pvec) * inv_det;
			 if (u < 0.0 || u > 1.0) return false;
			 v = (ray*(tvec^edge1)) * inv_det;
			 if ((v<0.0) || ((u+v)>1.0) ) return false;
			 return true;
		}
		*/
		
		PFLOAT Z_intersect()
		{
			return (normal*(toVector(*a)))/normal.z;
		};
		PFLOAT intersect(const point3d_t &from,const vector3d_t &ray)
		{
			return (normal*(*a-from))/(normal*ray);
		}
		void recNormal();
		const vector3d_t & N() const {return normal;};

		surfacePoint_t  getSurface(point3d_t &h,PFLOAT d,bool orco=false)const;
		void setNormals(vector3d_t *_na,vector3d_t *_nb,vector3d_t *_nc)
			{na=_na;nb=_nb;nc=_nc;};
		void setUV(std::vector<GFLOAT>::iterator _uv) { uv=_uv;  hasuv=true; }
		void setVCOL(std::vector<CFLOAT>::iterator _vcol) { vcol=_vcol;  has_vcol=true; }
		void setShader(const shader_t *sha) {shader=sha;};
		const shader_t * getShader()const {return shader;};

		point3d_t *a,*b,*c;
		vector3d_t *na,*nb,*nc;
		vector3d_t *ta, *tb, *tc;
		std::vector<GFLOAT>::iterator uv;
		std::vector<CFLOAT>::iterator vcol;
		bool hasuv, has_vcol;
	protected:
		const shader_t *shader;
		vector3d_t normal;
};

__END_YAFRAY
#endif
