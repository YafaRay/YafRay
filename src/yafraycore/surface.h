/****************************************************************************
 *
 * 			surface.h: Surface sampling representation and api
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
#ifndef __SURFACE_H
#define __SURFACE_H
#include "vector3d.h"
#include "color.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif
__BEGIN_YAFRAY
//class surfacePoint_t;
class object3d_t;
class shader_t;

/** This holds a sampled point's data
 *
 * When a ray intersects an object, a surfacePoint_t is computed.
 * It contains data about normal, position, assigned shader and other
 * things.
 *
 */

class YAFRAYCORE_EXPORT surfacePoint_t
{
	/// A friend
	//friend class object3d_t;
	/// A friend
	//friend class meshObject_t;
	/// A friend
	//friend class scene_t;
	public:

	/** A simple constructor taken all the info
	 *
	 * @param o is the object wich this point comes from.
	 * @param p is a 3d point in the world representing the position.
	 * @param n is the surface normal in that point.
	 * @param g is the geometric (not smoothed) normal in that point.
	 * @param u is the u texture coordinate in that point.
	 * @param v is the v texture coordinate in that point.
	 * @param d is the depth of the point from the viewer (i.e the distance)
	 *
	 */
		surfacePoint_t(object3d_t *o, const point3d_t &p, const point3d_t &orc,
				const vector3d_t &n,const vector3d_t &g,
				GFLOAT u, GFLOAT v, color_t vcol,
				PFLOAT d,const shader_t *sha=NULL,
				bool uv=false, bool hvcol=false,bool horco=false)
		{
			suP=p;  suN=n;  suNg=g;
			suU=u;  suV=v;  vtxcol=vcol;
			suZ=d;  obj=o; shader=sha;  hasuv=uv;  has_vcol=hvcol;
			hasorco = horco;
			orcoP = orc;
			createCS(suN, suNU, suNV);
			// tangents
			suTU = suNU;
			suTV = suNV;
			suNd = n;	// unmodified normal (not displaced)
			dudu = dudv = dvdu = dvdv = 0;
			originelement=NULL;
		}

		///An empty constructor
		surfacePoint_t() { hasorco=false;hasuv=false;  has_vcol=false;  shader=NULL; originelement=NULL; }
		/// Destructor
		~surfacePoint_t() {}

		/// Returns the normal.
		const vector3d_t &N() const { return suN; }
		vector3d_t &N() { return suN; }
		/// Returns the unmodified normal (copy of N before displacement)
		const vector3d_t &Nd() const { return suNd; }
		vector3d_t &Nd() { return suNd; }
		/// Returns the geometric normal.
		const vector3d_t &Ng() const { return suNg; }
		vector3d_t &Ng() { return suNg; }
		/// Returns the position.
		const point3d_t &P() const { return suP; }
		/// Returns the position.
		point3d_t &P() { return suP; }
		point3d_t &orco() { return orcoP; }
		const point3d_t &orco()const { return orcoP; }
		/// Returns the u texture coord.
		GFLOAT u() const { return suU; }
		/// Returns the v texture coord.
		GFLOAT v() const { return suV; }
		/// Returns whatever it has valid UV or not
		bool hasUV() const { return hasuv; }
		bool hasOrco() const { return hasorco; }
		void hasOrco(bool b) { hasorco=b; }
		/// Returns a reference to the u texture coord
		GFLOAT & u() { return suU; }
		/// Returns a reference to the v texture coord
		GFLOAT & v() { return suV; }
		/// Returns the vertex color.
		color_t vertex_col() const { return vtxcol; }
		/// Returns if the point has a valid vertex color
		bool hasVertexCol() const { return has_vcol; }
		/// Returns reference to the vertex color.
		color_t & vertex_col() { return vtxcol; }
		/// Returns the depth of the point.
		PFLOAT Z() const { return suZ; }
		/// Returns the object owner of the point.
		const object3d_t *getObject() const { return obj; }
		/// Returns the object owner of the point.
		object3d_t *getObject() { return obj; }
		/// Sets the object owner for a point
		void setObject(object3d_t *o) { obj=o; }
		/// Returns the surface shader
		const shader_t *getShader() const { return shader; }
		/// Sets the surface shader
		void setShader(const shader_t *sha) { shader=sha; }
		const vector3d_t & NU() const { return suNU; }
		const vector3d_t & NV() const { return suNV; }
		vector3d_t & NU() { return suNU; }
		vector3d_t & NV() { return suNV; }
		GFLOAT dudNU() const { return dudu; }
		GFLOAT dudNV() const { return dudv; }
		GFLOAT dvdNU() const { return dvdu; }
		GFLOAT dvdNV() const { return dvdv; }
		void setGradient(GFLOAT uu, GFLOAT uv, GFLOAT vu, GFLOAT vv)
		{ dudu=uu;  dudv=uv;  dvdu=vu;  dvdv=vv; }

		// tangent vectors, from uv, TV crossp of N and tu
		void setTangent(const vector3d_t &tu) { suTU=tu;  suTV=suN^tu; }
		const vector3d_t & TU() const { return suTU; }
		vector3d_t & TU() { return suTU; }
		const vector3d_t & TV() const { return suTV; }
		vector3d_t & TV() { return suTV; }

		// only used with 'win' texture coord. mode
		void setScreenPos(const point3d_t &p) { screenpos=p; }
		void getScreenPos(PFLOAT &sx, PFLOAT &sy) const { sx=screenpos.x;  sy=screenpos.y; }
	
		void setOrigin(const void *it) {originelement=it;};
		const void *getOrigin()const {return originelement;};
	protected:
		/// The surface normal
		vector3d_t suN, suNU, suNV, suTU, suTV, suNd;
		/// The geometric normal (not smoothed)
		vector3d_t suNg;
		/// surface texture coords
		GFLOAT suU,suV;
		/// The point itself
		point3d_t suP,orcoP;
		/// Depth from the viewer
		PFLOAT suZ;
		/// The object owner of the point
		object3d_t *obj;
		/// Surface shader
		const shader_t *shader;
		bool hasuv, has_vcol,hasorco;
		GFLOAT dudu,dudv,dvdu,dvdv;
		// only used with 'win' texture coord. mode
		point3d_t screenpos;
		// vertex color
		color_t vtxcol;
		const void *originelement;
};

__END_YAFRAY

#endif
