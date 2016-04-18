/****************************************************************************
 *
 * 			bound.h: Bound and tree api for general raytracing acceleration
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

#ifndef __BOUND_H
#define __BOUND_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "vector3d.h"
#include "matrix4.h"
#include <vector>
#include <list>

__BEGIN_YAFRAY

//#include <set>

/** Bounding box
 *
 * The bounding box class. A box aligned with the axis used to skip
 * object, photons, and faces intersection when possible.
 *
 */

class bound_t;
YAFRAYCORE_EXPORT GFLOAT bound_distance(const bound_t &l,const bound_t &r);
GFLOAT b_intersect(const bound_t &l,const bound_t &r);

class YAFRAYCORE_EXPORT bound_t 
{
	/// A friend
	friend YAFRAYCORE_EXPORT GFLOAT bound_distance(const bound_t &l,const bound_t &r);
	/// A friend
	friend GFLOAT b_intersect(const bound_t &l,const bound_t &r);
	public:

		/** Main constructor
		 *
		 * The box is defined by two points, this constructor just takes them.
		 *
		 * @param _a is the low corner (minx,miny,minz)
		 * @param _g is the up corner (maxx,maxy,maxz)
		 *
		 */
		bound_t(const point3d_t & _a,const point3d_t & _g) 
		{a=_a;g=_g;null=false;};
		/// Empty constructor
		bound_t() {null=true;};
		/// Copy constructor
		bound_t(const bound_t &s);

		/** Two child constructor
		 *
		 * This creates a bound that includes the two given bounds. It's used when
		 * building a bounding tree
		 * 
		 * @param a is one child bound
		 * @param b is another child bound
		 *
		 */
		bound_t(const bound_t &a,const bound_t &b);
		/// Sets the bound like the constructor
		void set(const point3d_t &_a,const point3d_t &_g) 
		{a=_a;g=_g;null=false;};
		void get(point3d_t &_a,point3d_t &_g)const {_a=a;_g=g;};

		/// Returns true if the given ray crosses teh bound
		bool cross(const point3d_t &from,const vector3d_t &ray)const;
		/// Returns true if the given ray crosses teh bound closer than dist
		bool cross(const point3d_t &from,const vector3d_t &ray,PFLOAT dist)const;
		bool cross(const point3d_t &from,const vector3d_t &ray,
				PFLOAT &where,PFLOAT dist)const;
		bool cross(const point3d_t &from,const vector3d_t &ray,
				PFLOAT &enter,PFLOAT &leave,PFLOAT dist)const;

		/// Returns the volume of the bound
		GFLOAT vol() const;
		/// Returns the lenght along X axis
		PFLOAT longX()const {return g.x-a.x;};
		/// Returns the lenght along Y axis
		PFLOAT longY()const {return g.y-a.y;};
		/// Returns the lenght along Y axis
		PFLOAT longZ()const {return g.z-a.z;};
		/// Cuts the bound to have the given max X
		void setMaxX(PFLOAT X) {g.x=X;};
		/// Cuts the bound to have the given min X
		void setMinX(PFLOAT X) {a.x=X;};
		
		/// Cuts the bound to have the given max Y
		void setMaxY(PFLOAT Y) {g.y=Y;};
		/// Cuts the bound to have the given min Y
		void setMinY(PFLOAT Y) {a.y=Y;};

		/// Cuts the bound to have the given max Z
		void setMaxZ(PFLOAT Z) {g.z=Z;};
		/// Cuts the bound to have the given min Z
		void setMinZ(PFLOAT Z) {a.z=Z;};
		/// Returns true if the point is inside the bound
		bool includes(const point3d_t &pn)const
		{
			return  (( pn.x >= a.x ) && ( pn.x <= g.x) &&
							(  pn.y >= a.y ) && ( pn.y <= g.y) &&
							(  pn.z >= a.z ) && ( pn.z <= g.z) );
		};
		PFLOAT centerX()const {return (g.x+a.x)*0.5;};
		PFLOAT centerY()const {return (g.y+a.y)*0.5;};
		PFLOAT centerZ()const {return (g.z+a.z)*0.5;};
		void grow(PFLOAT d)
		{
			a.x-=d;
			a.y-=d;
			a.z-=d;
			g.x+=d;
			g.y+=d;
			g.z+=d;
		};
		
//	protected: // Lynx
		/// Flag telling if the bound is null
		bool null;
		/// Two points define the box
		point3d_t a,g;
};

/** Distance betwen two bounds
 *
 * The distance is defined by the volume that is void in the
 * bound that includes the two given bounds.
 *
 */
YAFRAYCORE_EXPORT GFLOAT bound_distance(const bound_t &l,const bound_t &r);

class object3d_t;

class boundTreeNode_t
{
	friend class boundTree_t;
	public:
		boundTreeNode_t(boundTreeNode_t *l,boundTreeNode_t *r);
		boundTreeNode_t(object3d_t *obj);
		~boundTreeNode_t();
		bool cross(const point3d_t &from,const vector3d_t &ray)const
					{return bound.cross(from,ray);};
		bool cross(const point3d_t &from,const vector3d_t &ray,PFLOAT dist)const
					{return bound.cross(from,ray,dist);};
		bool isLeaf() {return (_object!=NULL);};
		boundTreeNode_t *right() {return _right;};
		boundTreeNode_t *left() {return _left;};
		boundTreeNode_t *parent() {return _parent;};
		object3d_t *object() {return _object;};
		int getCount()const {return count;};
		const bound_t & getBound()const {return bound;};
	protected:
		boundTreeNode_t *_left;
		boundTreeNode_t *_right;
		boundTreeNode_t *_parent;
		bound_t bound;
		int count;
		object3d_t *_object;
};

class boundTree_t
{
	public:
		boundTree_t(std::list<object3d_t *> &obj_list);
		~boundTree_t() {if(_root!=NULL) delete _root;};
		boundTreeNode_t *root() {return _root;};
	protected:
		boundTreeNode_t *_root;
};

std::ostream & operator << (std::ostream &out,boundTreeNode_t &n);

class objectIterator_t 
{
	public:
		objectIterator_t(boundTree_t &_btree,const point3d_t &f,
				const vector3d_t &r,PFLOAT _d=-1);
		void upFirstRight();
		void downLeft();
		void operator ++ ();
		void operator ++ (int) {++(*this);};
		bool operator ! () {return !end;};
		object3d_t * operator * () {return current->object();};
	protected:
		boundTreeNode_t *current;
		boundTree_t &btree;
		PFLOAT dist;
		bool end;
		const point3d_t &from;
		const vector3d_t &ray;
};

// GENERIC BOUND

template<class T>
class gBoundTreeNode_t
{
	//friend class gBoundTree_t;
	public:
		gBoundTreeNode_t(gBoundTreeNode_t<T> *l,gBoundTreeNode_t<T> *r,bound_t &b) 
			{_left=l;_right=r;bound=b;r->_parent=l->_parent=this;_parent=NULL;};
		
		gBoundTreeNode_t(const std::vector<T> & v,const bound_t b):_child(v) 
		{_left=_right=NULL;_parent=NULL;bound=b;};
		
		~gBoundTreeNode_t() {if(_left!=NULL) {delete _left;delete _right;} };
		
		bool isLeaf()const  {return (_left==NULL);};
		gBoundTreeNode_t<T> *right() {return _right;};
		const gBoundTreeNode_t<T> *right()const {return _right;};
		gBoundTreeNode_t<T> *left() {return _left;};
		const gBoundTreeNode_t<T> *left()const {return _left;};
		gBoundTreeNode_t<T> *parent() {return _parent;};
		const gBoundTreeNode_t<T> *parent()const {return _parent;};
		std::vector<T> & child() {return _child;};
		const std::vector<T> & child()const {return _child;};
		bound_t &getBound() {return bound;};
		const bound_t &getBound()const {return bound;};

		typename std::vector<T>::const_iterator begin()const {return _child.begin();};
		typename std::vector<T>::const_iterator end()const {return _child.end();};
	protected:
		gBoundTreeNode_t<T> *_left;
		gBoundTreeNode_t<T> *_right;
		gBoundTreeNode_t<T> *_parent;
		bound_t bound;
		std::vector<T> _child;
};

template<class T>
gBoundTreeNode_t<T> * buildGenericTree(const std::vector<T> &v,
		bound_t (*calc_bound)(const std::vector<T> &v),
		bool (*is_in_bound)(const T &t,bound_t &b),
		point3d_t (*get_pos)(const T &t),
		unsigned int dratio=1,unsigned int depth=1,
		bool skipX=false,bool skipY=false,bool skipZ=false)
{
	typedef typename std::vector<T>::const_iterator vector_const_iterator;
	if((v.size()<=dratio) || (skipX && skipY && skipZ))
		return new gBoundTreeNode_t<T>(v,calc_bound(v));
	PFLOAT lx,ly,lz;
	bool usedX=false,usedY=false,usedZ=false;
	bound_t bound=calc_bound(v);
	lx=bound.longX();
	ly=bound.longY();
	lz=bound.longZ();

	bound_t bl,br;
	if(((lx>=ly) || skipY) && ((lx>=lz) || skipZ) && !skipX)
	{
		PFLOAT media=0;
		for(vector_const_iterator i=v.begin();i!=v.end();++i)
			media+=get_pos(*i).x;
		media/=(PFLOAT)v.size();
		bl=bound;bl.setMaxX(media);
		br=bound;br.setMinX(media);
		usedX=true;
	}
	else if(((ly>=lx) || skipX) && ((ly>=lz) || skipZ) && !skipY)
	{
		PFLOAT media=0;
		for(vector_const_iterator i=v.begin();i!=v.end();++i)
			media+=get_pos(*i).y;
		media/=(PFLOAT)v.size();
		bl=bound;bl.setMaxY(media);
		br=bound;br.setMinY(media);
		usedY=true;
	}
	else
	{
		PFLOAT media=0;
		for(vector_const_iterator i=v.begin();i!=v.end();++i)
			media+=get_pos(*i).z;
		media/=(PFLOAT)v.size();
		bl=bound;bl.setMaxZ(media);
		br=bound;br.setMinZ(media);
		usedZ=true;
	}
	std::vector<T> vl,vr,vm;
	for(vector_const_iterator i=v.begin();i!=v.end();++i)
	{
		if(is_in_bound(*i,bl))
		{
			if(is_in_bound(*i,br)) 
				vm.push_back(*i);
			else
				vl.push_back(*i);
		}
		else
			vr.push_back(*i);
	}

	if( (vl.size()==v.size()) || (vr.size()==v.size()) || (vm.size()==v.size()))
		return buildGenericTree(v,calc_bound,is_in_bound,get_pos,dratio,depth,
						skipX || usedX,skipY || usedY,skipZ || usedZ);
	if(vl.size()==0) return new gBoundTreeNode_t<T>(buildGenericTree(vr,calc_bound,is_in_bound,
				get_pos,dratio,depth+1,skipX,skipY,skipZ),buildGenericTree(vm,calc_bound,
					is_in_bound,get_pos,dratio,depth+1, skipX,skipY,skipZ),bound);
	if(vr.size()==0) return new gBoundTreeNode_t<T>(buildGenericTree(vl,calc_bound,is_in_bound,
				get_pos,dratio,depth+1,skipX,skipY,skipZ),buildGenericTree(vm,calc_bound,
					is_in_bound,get_pos,dratio,depth+1, skipX,skipY,skipZ),bound);
	if(vm.size()==0) return new gBoundTreeNode_t<T>(buildGenericTree(vl,calc_bound,is_in_bound,
				get_pos,dratio,depth+1,skipX,skipY,skipZ),buildGenericTree(vr,calc_bound,
					is_in_bound,get_pos,dratio,depth+1, skipX,skipY,skipZ),bound);
	//else 
	//{
		gBoundTreeNode_t<T> *balanced=new gBoundTreeNode_t<T>( 
			buildGenericTree(vl,calc_bound,is_in_bound,get_pos,dratio,depth+1,
				skipX,skipY,skipZ),
			buildGenericTree(vr,calc_bound,is_in_bound,get_pos,dratio,depth+1,
				skipX,skipY,skipZ),bound);
		if(vm.size()==0) return balanced;
		return new gBoundTreeNode_t<T>(
			balanced,
			buildGenericTree(vm,calc_bound,is_in_bound,get_pos,dratio,depth+1,
				skipX,skipY,skipZ),bound);
	//}
}

template<class T,class D,class CROSS>
class gObjectIterator_t 
{
	public:
		gObjectIterator_t(const gBoundTreeNode_t<T> *r,const D &d);
		void upFirstRight();
		void downLeft();
		void operator ++ ();
		void operator ++ (int) {++(*this);};
		bool operator ! () {return !end;};
		//T & operator * () {return *currT;};
		const T & operator * () {return *currT;};
		const gBoundTreeNode_t<T> *currentNode() {return currN;};
	protected:
		const gBoundTreeNode_t<T> *currN;
		const gBoundTreeNode_t<T> *root;
		const D &dir;
		PFLOAT dist;
		bool end;
		CROSS cross;
		typename std::vector<T>::const_iterator currT;
		typename std::vector<T>::const_iterator currTend;
};

#define DOWN_LEFT(c) c=c->left()
#define DOWN_RIGHT(c) c=c->right()
#define UP(c) c=c->parent()
#define WAS_LEFT(o,c) (c->left()==o)
#define WAS_RIGHT(o,c) (c->right()==o)
#define TOP(c) (c->parent()==NULL)

template<class T,class D,class CROSS>
gObjectIterator_t<T,D,CROSS>::gObjectIterator_t(const gBoundTreeNode_t<T> *r,const D &d):
	dir(d)
{
	root=currN=r;
	if(!cross(dir,currN->getBound()))
	{
		end=true;
		return;
	}
	else end=false;
	downLeft();
	if(!(currN->isLeaf()))
	{
		currT=currTend=currN->end();
		while(currT==currTend)
		{
			bool first=true;
			while(first || !(currN->isLeaf()))
			{
				first=false;
				upFirstRight();
				if(currN==NULL)
				{
					end=true;
					return;
				}
				DOWN_RIGHT(currN);
				downLeft();
			}
			currT=currN->begin();
			currTend=currN->end();
		}
	}
	else
	{
		currT=currN->begin();
		currTend=currN->end();
		//if(currT==currTend) ++(*this);
		while(currT==currTend)
		{
			bool first=true;
			while(first || !(currN->isLeaf()))
			{
				first=false;
				upFirstRight();
				if(currN==NULL)
				{
					end=true;
					return;
				}
				DOWN_RIGHT(currN);
				downLeft();
			}
			currT=currN->begin();
			currTend=currN->end();
		}
	}
}

template<class T,class D,class CROSS>
void gObjectIterator_t<T,D,CROSS>::upFirstRight()
{
	const gBoundTreeNode_t<T> *old=currN;
	if(TOP(currN)) 
	{
		currN=NULL;
		return;
	}
	old=currN;
	UP(currN);
	while(WAS_RIGHT(old,currN) || !cross(dir,currN->right()->getBound()))
	{
		if(TOP(currN))
		{
			currN=NULL;
			return;
		}
		old=currN;
		UP(currN);
	}
}

template<class T,class D,class CROSS>
void gObjectIterator_t<T,D,CROSS>::downLeft()
{
	while(!(currN->isLeaf()))
	{
		while( !(currN->isLeaf()) && cross(dir,currN->left()->getBound()) )
			DOWN_LEFT(currN);
		if(!(currN->isLeaf())) 
		{
			if(cross(dir,currN->right()->getBound()))
				DOWN_RIGHT(currN);
			else return;
		}
	}
}


template<class T,class D,class CROSS>
inline void gObjectIterator_t<T,D,CROSS>::operator ++ ()
{
	currT++;
	while(currT==currTend)
	{
		bool first=true;
		while(first || !(currN->isLeaf()))
		{
			first=false;
			upFirstRight();
			if(currN==NULL)
			{
				end=true;
				return;
			}
			DOWN_RIGHT(currN);
			downLeft();
		}
		currT=currN->begin();
		currTend=currN->end();
	}
	
}
extern int bcount;
//inlined boun funcions
//
inline bool bound_t::cross(const point3d_t &from,const vector3d_t &ray)const
{
		const point3d_t &a0=a,&a1=g;
		vector3d_t p;
		p=from-a0;
		bcount++;
    PFLOAT lmin=-1, lmax=-1,tmp1,tmp2;
		if(ray.x!=0.0)
		{
	    tmp1 =               -p.x/ray.x;
	    tmp2 = ((a1.x-a0.x)-p.x)/ray.x;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    lmin = tmp1;
	    lmax = tmp2;	// si  < 0, podemos devolver false
			if(lmax<0) return false;
		}
		if(ray.y!=0.0)
		{
	    tmp1 = -p.y/ray.y;
	    tmp2 = ((a1.y-a0.y)-p.y)/ray.y;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	// si  < 0, podemos devolver false
			if(lmax<0) return false;
		}
		if(ray.z!=0.0)
		{
	    tmp1 = -p.z/ray.z;
	    tmp2 = ((a1.z-a0.z)-p.z)/ray.z;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	
		} 
    return (lmin <= lmax) && (lmax >= 0);
}

inline bool bound_t::cross(const point3d_t &from,const vector3d_t &ray,PFLOAT dist)const
{
		const point3d_t &a0=a,&a1=g;
		vector3d_t p;
		p=from-a0;

    PFLOAT lmin=-1, lmax=-1,tmp1,tmp2;
		if(ray.x!=0.0)
		{
	    tmp1 =               -p.x/ray.x;
	    tmp2 = ((a1.x-a0.x)-p.x)/ray.x;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    lmin = tmp1;
	    lmax = tmp2;	// si  < 0, podemos devolver false
			if((lmax<0) || (lmin>dist)) return false;
		}
		if(ray.y!=0.0)
		{
	    tmp1 = -p.y/ray.y;
	    tmp2 = ((a1.y-a0.y)-p.y)/ray.y;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    //if (tmp2 < lmax)
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	// si  < 0, podemos devolver false
			if((lmax<0) || (lmin>dist)) return false;
		}
		if(ray.z!=0.0)
		{
	    tmp1 = -p.z/ray.z;
	    tmp2 = ((a1.z-a0.z)-p.z)/ray.z;
	    if (tmp1 > tmp2)
	       std::swap(tmp1, tmp2);
	    if (tmp1 > lmin)
	        lmin = tmp1;
	    if ((tmp2 < lmax) || (lmax<0))
	        lmax = tmp2;	
		} 
    return (lmin <= lmax) && (lmax >= 0) && (lmin<=dist);
}


__END_YAFRAY

#endif
