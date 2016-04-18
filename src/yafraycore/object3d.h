/****************************************************************************
 *
 * 			object3d.h: Generic 3D object api 
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
#ifndef __OBJECT3D_H
#define __OBJECT3D_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "matrix4.h"
__BEGIN_YAFRAY
class object3d_t;
__END_YAFRAY
#include "surface.h"
#include "shader.h"
#include "bound.h"
//#include "spectrum.h"

__BEGIN_YAFRAY
#define MESH 0
#define SPHERE 1
#define REFERENCE 2

class YAFRAYCORE_EXPORT object3d_t
{
	friend class photonLight_t;
	public:
		object3d_t()
		{
			radiosity = true;
			rad_pasive = true;
			shadow = true;
			caus = false;
			tag = NULL;
		}
		virtual ~object3d_t() {};
		virtual int type() const =0;
		virtual void transform(const matrix4x4_t &m) =0;
		virtual point3d_t toObject(const point3d_t &p)const=0;
		// transforms vector using the rotation matrix only, needed for cubemapping
		virtual vector3d_t toObjectRot(const vector3d_t &v) const=0;
		// transforms point taking the objects bounds into account (for 'orco' texturemapping)
		virtual point3d_t toObjectOrco(const point3d_t &p) const=0;
		virtual bool shoot(renderState_t &state,surfacePoint_t &where, const point3d_t &from,
				const vector3d_t &ray,bool shadow=false,PFLOAT dis=-1)const=0;
		virtual bound_t getBound() const =0;
		void setShader(shader_t *shad) {shader=shad;};
		shader_t *getShader() const {return shader;};
		bool useForRadiosity() const  {return radiosity;};
		void useForRadiosity(bool r) {radiosity=r;};
		bool reciveRadiosity() const  {return rad_pasive;};
		void reciveRadiosity(bool r) {rad_pasive=r;};
		bool castShadows() const  {return shadow;};
		void castShadows(bool r) {shadow=r;};
		bool caustics() const  { return caus; }
		void caustics(bool r) { caus=r; }
		void setCaustic(const color_t &r, const color_t &t, PFLOAT ior)
		{
			caus_rcolor = r;
			caus_tcolor = t;
			caus_IOR = ior;
		}
		void getCaustic(color_t &r, color_t &t, PFLOAT &ior)
		{
			r = caus_rcolor;
			t = caus_tcolor;
			ior = caus_IOR;
		}
	protected:
		shader_t *shader;
		bool radiosity;
		bool rad_pasive;
		bool shadow;
		bool caus;
		void *tag;
		color_t caus_rcolor;
		color_t caus_tcolor;
		PFLOAT caus_IOR;
};

template<class T> class geomeTree_t;

geomeTree_t<object3d_t> * buildObjectTree(std::list<object3d_t *> &obj_list);

__END_YAFRAY
#endif
