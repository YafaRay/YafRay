#ifndef __GEOMETREE_H
#define __GEOMETREE_H

#include"bound.h"

__BEGIN_YAFRAY

template<class T>
class geomeTree_t
{
	public:
		geomeTree_t(T *element,const bound_t &b,bool fd=false):
			bound(b),leaf(element),count(1),freedata(fd) {};
		geomeTree_t(geomeTree_t<T> *l,geomeTree_t<T> *r,bool fd=false):
			bound(l->getBound(),r->getBound()),left(l),right(r),leaf(NULL),
			count(l->getCount()+r->getCount()),freedata(fd) {};

		~geomeTree_t() {if(!isLeaf()) {delete left;delete right;} if(freedata) delete leaf;};

		bool isLeaf()const {return leaf!=NULL;};
		const bound_t &getBound()const {return bound;};
		const geomeTree_t<T> *goLeft()const {return left;};
		const geomeTree_t<T> *goRight()const {return right;};
		const T *getElement()const {return leaf;};
		int getCount()const {return count;};
	protected:
		bound_t bound;
		geomeTree_t<T> *left,*right;
		T *leaf;
		int count;
		bool freedata;
};

#include<vector>

template<class T>
class geomeIterator_t
{
	public:
		geomeIterator_t(const geomeTree_t<T> *root,PFLOAT m,
				const point3d_t &f,const vector3d_t &r,bool sh=false);
		
		void operator ++ ();
		void operator ++ (int) {++(*this);};
		bool operator ! () {return current!=NULL;};
		const T * operator * () {return current;};
		void limit(PFLOAT Z) {if(Z<maximun) maximun=Z;};
	protected:
		void down(const geomeTree_t<T> *node);
		
		struct state_t
		{
			state_t(const geomeTree_t<T> *n,PFLOAT e):node(n),enter(e) {};
			const geomeTree_t<T> *node; // branch left to go through
			PFLOAT enter;
		};
		std::vector<state_t> stack;
		const T *current;
		PFLOAT maximun;
		const point3d_t &from;
		const vector3d_t &ray;
		bool shadow;
};


template<class T>
geomeIterator_t<T>::geomeIterator_t(const geomeTree_t<T> *root,PFLOAT m,
																		const point3d_t &f,const vector3d_t &r,bool sh):
maximun(m),from(f),ray(r),shadow(sh)
{
	if(root==NULL)
		current=NULL;
	else
	{
		PFLOAT where=0;
		if(root->getBound().cross(from,ray,where,maximun))
		{
			stack.reserve(16);
			down(root);
		}
		else
			current=NULL;
	}
}

template<class T>
void geomeIterator_t<T>::down(const geomeTree_t<T> *node)
{
	while(!node->isLeaf())
	{
		PFLOAT hleft=std::numeric_limits<PFLOAT>::infinity(),hright=std::numeric_limits<PFLOAT>::infinity();
		bool hitleft=node->goLeft()->getBound().cross(from,ray,hleft,maximun);
		bool hitright=true;
		if(!shadow || !hitleft)
			hitright=node->goRight()->getBound().cross(from,ray,hright,maximun);
		if(!hitleft && !hitright)
		{
			if(stack.empty())
			{
				current=NULL;
				return;
			}
			node=stack.back().node;
			stack.pop_back();
			continue;
		}
		if(shadow)
		{
			if(hitleft)
			{
				if(hitright)
					stack.push_back(state_t(node->goRight(),0));
				node=node->goLeft();
			}
			else
				node=node->goRight();
		}
		else
		{
			if(hleft<hright)
			{
				if(hitright)
					stack.push_back(state_t(node->goRight(),hright));
				node=node->goLeft();
			}
			else
			{
				if(hitleft)
					stack.push_back(state_t(node->goLeft(),hleft));
				node=node->goRight();
			}
		}
	}
	current=node->getElement();
}

template<class T>
inline void geomeIterator_t<T>::operator ++ ()
{
	current=NULL;
	PFLOAT foo;
	if(shadow)
		while(stack.size())
		{
			if(stack.back().node->getBound().cross(from,ray,foo,maximun)) break;
			stack.pop_back();
		}
	else
		while(stack.size() && (stack.back().enter>maximun))
			stack.pop_back();
	if(stack.size())
	{
		const geomeTree_t<T> *node=stack.back().node;
		stack.pop_back();
		down(node);
	}
}

// PURE BSP
//

class YAFRAYCORE_EXPORT plane_t
{
	public:
		static const int XAXIS=0;
		static const int YAXIS=1;
		static const int ZAXIS=2;
		plane_t() {};
		plane_t(const bound_t &b,PFLOAT cut,int ax):zcut(cut),axis(ax) {};
		void set(const bound_t &b,PFLOAT cut,int ax)
		{
			axis=ax;
			zcut=cut;
		};
		bool cross(point3d_t from,vector3d_t ray,PFLOAT minD,PFLOAT maxD,PFLOAT &where,int &side)const
		{
			PFLOAT fz,rz;
			switch(axis)
			{
				case XAXIS:fz=from.x;rz=ray.x;break;
				case YAXIS:fz=from.y;rz=ray.y;break;
				default:fz=from.z;rz=ray.z;break;
			}
			side=(fz<zcut) ? 0 : 1;
			if(rz==0) return false;
			PFLOAT D=(zcut-fz)/rz;
			if(D<0) return false;
			if(D<minD) {side=(~side)&1;return false;}
			if(D>maxD) return false;
			where=D;
			return true;
		};
		PFLOAT zcut;
		int axis;
};

inline bool planeCrossInv(PFLOAT zcut,int axis,const point3d_t &from,const vector3d_t &invray,
		PFLOAT minD,PFLOAT maxD,PFLOAT &where,int &side)
{
	PFLOAT fz,irz;
	switch(axis)
	{
		case plane_t::XAXIS:fz=from.x;irz=invray.x;break;
		case plane_t::YAXIS:fz=from.y;irz=invray.y;break;
		default:fz=from.z;irz=invray.z;break;
	}
	side=(fz<zcut) ? 0 : 1;
	if(irz==0) return false;
	PFLOAT D=(zcut-fz)*irz;
	if(D<0) return false;
	if(D<minD) {side=(~side)&1;return false;}
	if(D>maxD) return false;
	where=D;
	return true;
}

template<class T>
class pureBspTree_t
{
	public:
		pureBspTree_t(T *element):
			left(NULL),right(NULL),leaf(element) {};
		
		pureBspTree_t(pureBspTree_t<T> *l,pureBspTree_t<T> *r,const plane_t &p):
			plane(p),left(l),right(r),leaf(NULL) {};

		~pureBspTree_t() 
		{
			if(left!=NULL) delete left;
			if(right!=NULL) delete right; 
			if(isLeaf()) delete leaf;
		};

		bool isLeaf()const {return leaf!=NULL;};
		PFLOAT getPlane()const {return plane.zcut;};
		int getAxis()const {return plane.axis;};
		const pureBspTree_t<T> *goLeft()const {return left;};
		const pureBspTree_t<T> *goRight()const {return right;};
		const T *getElement()const {return leaf;};
	protected:
		plane_t plane;
		pureBspTree_t<T> *left,*right;
		T *leaf;
};

template<class T>
class pureBspIterator_t
{
	public:
		pureBspIterator_t(const pureBspTree_t<T> *root,PFLOAT m,const bound_t &bound,
				const point3d_t &f,const vector3d_t &r,bool sh=false);
		
		void operator ++ ();
		void operator ++ (int) {++(*this);};
		bool operator ! () {return current!=NULL;};
		const T * operator * () {return current;};
		void limit(PFLOAT Z) {if(Z<farest) farest=Z;};
	protected:
		void down(const pureBspTree_t<T> *node,PFLOAT minimun,PFLOAT maximun);
		
		struct state_t
		{
			state_t(const pureBspTree_t<T> *n,PFLOAT e,PFLOAT l):node(n),enter(e),leave(l) {};
			const pureBspTree_t<T> *node; // branch left to go through
			PFLOAT enter;
			PFLOAT leave;
		};
		std::vector<state_t> stack;
		const T *current;
		const point3d_t &from;
		const vector3d_t &ray;
		vector3d_t invray;
		PFLOAT farest;
		bool shadow;
};


template<class T>
pureBspIterator_t<T>::pureBspIterator_t(const pureBspTree_t<T> *root,PFLOAT m,const bound_t &bound,
																		const point3d_t &f,const vector3d_t &r,bool sh):
from(f),ray(r),shadow(sh)
{
	if(root==NULL)
		current=NULL;
	else
	{
		PFLOAT min,max;
		if(!bound.cross(from,ray,min,max,m))
			current=NULL;
		else
		{
			if(ray.x!=0) invray.x=1.0/ray.x; else invray.x=0;
			if(ray.y!=0) invray.y=1.0/ray.y; else invray.y=0;
			if(ray.z!=0) invray.z=1.0/ray.z; else invray.z=0;
			farest=(m<max) ? m : max;
			stack.reserve(16);
			down(root,min,farest);
		}
	}
}

template<class T>
inline void pureBspIterator_t<T>::down(const pureBspTree_t<T> *node,PFLOAT minimun,PFLOAT maximun)
{
	PFLOAT limit=0;
	int side;
	bool cross;
	while(!node->isLeaf())
	{
		cross=planeCrossInv(node->getPlane(),node->getAxis(),
												from,invray,minimun,maximun,limit,side);
		if(side==0) //left
		{
			if(cross)
				stack.push_back(state_t(node->goRight(),limit,maximun));
			node=node->goLeft();
		}
		else
		{
			if(cross)
				stack.push_back(state_t(node->goLeft(),limit,maximun));
			node=node->goRight();
		}
		if(cross) maximun=limit;
	}
	current=node->getElement();
}

template<class T>
void pureBspIterator_t<T>::operator ++ ()
{
	current=NULL;
	if(stack.size() && (stack.back().enter<farest))
	{
		const pureBspTree_t<T> *node=stack.back().node;
		PFLOAT enter=stack.back().enter;
		PFLOAT leave=stack.back().leave;
		if(farest<leave) leave=farest;
		stack.pop_back();
		down(node,enter,leave);
	}
}
__END_YAFRAY
#endif
