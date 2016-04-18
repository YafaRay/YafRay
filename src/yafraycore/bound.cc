/****************************************************************************
 *
 * 			bound.cc: Bound and tree implementation for raytracing acceleration
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

#include <iostream>
#include <set>
using namespace std;
#include "bound.h"
#include "object3d.h"
#include "yafutils.h"

__BEGIN_YAFRAY

#define MIN(a,b) ( ((a)<(b)) ? (a) : (b))
#define MAX(a,b) ( ((a)>(b)) ? (a) : (b))

bound_t::bound_t(const bound_t &s)
{
	a=s.a;
	g=s.g;
	null=s.null;
}

bound_t::bound_t(const bound_t &r,const bound_t &l)
{
	PFLOAT minx=MIN(r.a.x,l.a.x);
	PFLOAT miny=MIN(r.a.y,l.a.y);
	PFLOAT minz=MIN(r.a.z,l.a.z);
	PFLOAT maxx=MAX(r.g.x,l.g.x);
	PFLOAT maxy=MAX(r.g.y,l.g.y);
	PFLOAT maxz=MAX(r.g.z,l.g.z);
	a.set(minx,miny,minz);
	g.set(maxx,maxy,maxz);
}

GFLOAT bound_t::vol() const
{
	GFLOAT ret=(g.y-a.y)*(g.x-a.x)*(g.z-a.z);
	if( ret<0 ) cout<<"warning usorted bounding points\n";
	return ret;
}

GFLOAT b_intersect(const bound_t &r,const bound_t &l)
{
	PFLOAT minx=MAX(r.a.x,l.a.x);
	PFLOAT miny=MAX(r.a.y,l.a.y);
	PFLOAT minz=MAX(r.a.z,l.a.z);
	PFLOAT maxx=MIN(r.g.x,l.g.x);
	PFLOAT maxy=MIN(r.g.y,l.g.y);
	PFLOAT maxz=MIN(r.g.z,l.g.z);
	if(maxx<=minx) return 0;
	if(maxy<=miny) return 0;
	if(maxz<=minz) return 0;
	return (GFLOAT)((maxx-minx)*(maxy-miny)*(maxz-minz));
}

GFLOAT bound_distance(const bound_t &l,const bound_t &r)
{
	bound_t nb(l,r);
	GFLOAT voids=nb.vol()-l.vol()-r.vol()+b_intersect(l,r);
	if(voids<0) voids=0;
	return voids +nb.vol()+fabs(l.vol()-r.vol());
}


boundTreeNode_t::boundTreeNode_t(boundTreeNode_t *l,boundTreeNode_t *r):
bound(l->bound,r->bound)
{
	_object=NULL;
	_left=l;
	_right=r;
	count=l->count+r->count;
	l->_parent=this;
	r->_parent=this;
	_parent=NULL;
}

boundTreeNode_t::boundTreeNode_t(object3d_t *obj):bound(obj->getBound())
{
	_object=obj;
	count=1;
	_parent=NULL;
	_left=NULL;
	_right=NULL;
}
boundTreeNode_t::~boundTreeNode_t()
{
	if(_object==NULL)
	{
		if(_left==NULL) cout<<"Corrupted boundtree\n";
		else delete _left;
		if(_right==NULL) cout<<"Corrupted boundtree\n";
		else delete _right;
	}
}

struct nodeTreeDist_f
{
	PFLOAT operator () (const boundTreeNode_t *a,const boundTreeNode_t *b)const
	{
		return bound_distance(a->getBound(),b->getBound()) /* *
			(fabs((GFLOAT)( a->getCount() - b->getCount() ) ))*/;
	}
};


struct nodeTreeJoin_f
{
	boundTreeNode_t * operator () (boundTreeNode_t *a,boundTreeNode_t *b)const
	{
		return new boundTreeNode_t(a,b);
	}
};

boundTree_t::boundTree_t(list<object3d_t *> &obj_list)
{
	treeBuilder_t<boundTreeNode_t*,PFLOAT,nodeTreeDist_f,nodeTreeJoin_f> builder;
	//set<boundTreeNode_t *> nodes;
	boundTreeNode_t *nuevo;

	for(list<object3d_t *>::const_iterator ite=obj_list.begin();
			ite!=obj_list.end();++ite)
	{
		nuevo=new boundTreeNode_t(*ite);
		if (nuevo==NULL)
		{
			cout<<"Error allocating memory in bound tree\n";
			exit(1);
		}
		builder.push(nuevo);
		//nodes.insert(nuevo);
	}
/*
	while(nodes.size()>1)
	{
		GFLOAT mindis=-1;
		boundTreeNode_t *l,*r;
		for(set<boundTreeNode_t *>::iterator ite1=nodes.begin();
				ite1!=nodes.end();++ite1)
		{
			set<boundTreeNode_t *>::iterator ite2=ite1;
			ite2++;
			for(;ite2!=nodes.end();ite2++)
			{
				GFLOAT cdis=bound_distance((*ite1)->bound,(*ite2)->bound);
				cdis*=fabs((GFLOAT)( (*ite1)->getCount() - (*ite2)->getCount() ) );
				if((mindis<0) || (cdis<mindis))
				{
					mindis=cdis;
					l=*ite1;
					r=*ite2;
				}
			}
		}
		
		nuevo=new boundTreeNode_t(l,r);
		nodes.erase(l);
		nodes.erase(r);
		nodes.insert(nuevo);
	}
	*/
	_root=NULL;
	if(obj_list.size())
	{
		builder.build();
		_root=builder.solution();
	}
	//if(nodes.size())
	//	_root=*(nodes.begin());
}

ostream & operator << (ostream &out,boundTreeNode_t &n)
{
	if(n.isLeaf())
	{
		out<<&n;
		if(n.parent()!=NULL)
			out<<"(L "<<n.parent()<<" "<<n.object()<<")";
		else
			out<<"(L "<<n.object()<<")";
	}
	else
	{
		out<<(*n.left())<<"\n";
		out<<(*n.right())<<"\n";
		
		out<<&n;
		if(n.parent()!=NULL)
			out<<"("<<n.left()<<" "<<n.parent()<<" "<<n.right()<<")";
		else
			out<<"("<<n.left()<<" NULL "<<n.right()<<")";
	}
	return out;
}


objectIterator_t::objectIterator_t(boundTree_t &_btree, const point3d_t &f,
		const vector3d_t &r,PFLOAT _dist):
	btree(_btree),from(f),ray(r)
{
	current=btree.root();
	if(current==NULL)
	{
		end=true;
		return;
	}
	dist=_dist;
	if(dist>0)
	{
		if(!current->cross(from,ray,dist))
		{
			end=true;
			return;
		}
	}
	else
	{
		if(!current->cross(from,ray))
		{
			end=true;
			return;
		}
	}
	end=false;
	downLeft();
	if(!(current->isLeaf()))
		++(*this);
}


void objectIterator_t::upFirstRight()
{
	boundTreeNode_t *old=current;
	if(TOP(current)) 
	{
		current=NULL;
		return;
	}
	old=current;
	UP(current);
	if(dist>0)
	{
		while(WAS_RIGHT(old,current) || !(current->right()->cross(from,ray,dist)))
		{
			if(TOP(current))
			{
				current=NULL;
				return;
			}
			old=current;
			UP(current);
		}
	}
	else
	{
		while(WAS_RIGHT(old,current) || !(current->right()->cross(from,ray)))
		{
			if(TOP(current))
			{
				current=NULL;
				return;
			}
			old=current;
			UP(current);
		}
	}
}

void objectIterator_t::downLeft()
{
	if(dist>0)
	{
		while(!(current->isLeaf()))
		{
			while( !(current->isLeaf()) && (current->left()->cross(from,ray,dist)) )
			{
				DOWN_LEFT(current);
			}
			if(!(current->isLeaf())) 
			{
				if((current->right()->cross(from,ray,dist)))
					DOWN_RIGHT(current);
				else return;
			}
		}
	}
	else
	{
		while(!(current->isLeaf()))
		{
			while( !(current->isLeaf()) && (current->left()->cross(from,ray)) )
			{
				DOWN_LEFT(current);
			}
			if(!(current->isLeaf())) 
			{
				if((current->right()->cross(from,ray)))
					DOWN_RIGHT(current);
				else return;
			}
		}
	}
}

void objectIterator_t::operator ++ ()
{
	bool first=true;
	while(first || !(current->isLeaf()))
	{
		first=false;
		upFirstRight();
		if(current==NULL)
		{
			end=true;
			return;
		}
		DOWN_RIGHT(current);
		downLeft();
	}
}

bool bound_t::cross(const point3d_t &from,const vector3d_t &ray,PFLOAT &where,PFLOAT dist)const
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
		if((lmin <= lmax) && (lmax >= 0) && (lmin<=dist))
		{
			where=(lmin>0) ? lmin : 0;
			return true;
		}
		else return false;
}

bool bound_t::cross(const point3d_t &from,const vector3d_t &ray,PFLOAT &enter,PFLOAT &leave,PFLOAT dist)const
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
		if((lmin <= lmax) && (lmax >= 0) && (lmin<=dist))
		{
			enter=/*(lmin>0) ? */lmin/* : 0*/;
			leave=lmax;
			return true;
		}
		else return false;
}

__END_YAFRAY
