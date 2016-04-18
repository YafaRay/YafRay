/****************************************************************************
 *
 * 			mesh.h: Mesh object api 
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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
#ifndef __MESH_H
#define __MESH_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "object3d.h"
#include "vector3d.h"
#include "triangle.h"
#include "kdtree.h" //Lynx
#include <vector>


__BEGIN_YAFRAY

struct mray_t
{
	point3d_t from;
	vector3d_t ray;
};


struct rayCross_f
{
	bool operator() (const mray_t &m,const bound_t &b) {return b.cross(m.from,m.ray);};
};

template<class T> class pureBspTree_t;

class YAFRAYCORE_EXPORT meshObject_t : public object3d_t
{
	public:
		void hasOrco(bool b) { hasorco=b; }
		void autoSmooth(PFLOAT angle);
		void tangentsFromUV();
		virtual ~meshObject_t();
		virtual int type() const {return MESH;};
		virtual void transform(const matrix4x4_t &m);
		virtual point3d_t toObject(const point3d_t &p)const;
		virtual vector3d_t toObjectRot(const vector3d_t &v) const;
		virtual point3d_t toObjectOrco(const point3d_t &p) const;
		virtual bool shoot(renderState_t &state,surfacePoint_t &where,const point3d_t &from,
				const vector3d_t &ray,bool shadow=false,PFLOAT dis=-1) const;
		virtual bound_t getBound() const {return bound;};

		static meshObject_t *factory(const std::vector<point3d_t> &ver, const std::vector<vector3d_t> &nor,
				        const std::vector<triangle_t> &ts, const std::vector<GFLOAT> &fuv, const std::vector<CFLOAT> &fvcol);
		static meshObject_t *factory(bool _hasorco, const matrix4x4_t &M, const std::vector<point3d_t> &ver,
				const std::vector<vector3d_t> &nor, const std::vector<triangle_t> &ts,
				const std::vector<GFLOAT> &fuv, const std::vector<CFLOAT> &fvcol);

	protected:
		meshObject_t(const std::vector<point3d_t> &ver, const std::vector<vector3d_t> &nor,
				const std::vector<triangle_t> &ts, const std::vector<GFLOAT> &fuv, const std::vector<CFLOAT> &fvcol);
		meshObject_t(bool _hasorco, const matrix4x4_t &M, const std::vector<point3d_t> &ver,
				const std::vector<vector3d_t> &nor, const std::vector<triangle_t> &ts,
				const std::vector<GFLOAT> &fuv, const std::vector<CFLOAT> &fvcol);
		meshObject_t()
		{
			unt=true;
			shader=NULL;
			tree=NULL;
			n_tree=0;
			hasorco=false;
		};
		meshObject_t(const meshObject_t &m) {}; //forbiden
		
		void recalcBound();
		std::vector<point3d_t> vertices;
		std::vector<vector3d_t> normals, tangents;
		std::vector<triangle_t> triangles;
		std::vector<GFLOAT> facesuv;
		std::vector<CFLOAT> faces_vcol;
		bound_t bound;
		bool unt,hasorco;
		// backRot -> rotation only matrix
		// backOrco -> orco matrix for texture mapping
		matrix4x4_t back, backRot, backOrco;
		//gBoundTreeNode_t<triangle_t *> *tree;
		//geomeTree_t<std::vector<triangle_t*> > *tree;
		pureBspTree_t<std::vector<triangle_t*> > *tree;
		kdTree_t *n_tree; //Lynx
};

__END_YAFRAY
#endif
