#ifndef __HASH3D_H
#define __HASH3D_H

#include<vector>
#include<list>
#include"vector3d.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<map>

__BEGIN_YAFRAY

template<class T>
class hash3d_iterator;
template<class T>
class hash3d_const_iterator;

template<class T>
class hash3d_t 
{
		friend class hash3d_iterator<T>;
		friend class hash3d_const_iterator<T>;
	public:

		typedef class hash3d_iterator<T> iterator;
		typedef class hash3d_const_iterator<T> const_iterator;
		
		hash3d_t(PFLOAT cell,unsigned int size=512);
		~hash3d_t();
		void insert(const T & );
		void clear();

		const T *findExistingBox(const point3d_t &)const;
		const T *findExistingBox(int x,int y,int z)const;
		T *findExistingBox(const point3d_t &);
		T *findExistingBox(int x,int y,int z);
		
		T & findBox(const point3d_t &p) 
		{point3d_t box=getBox(p);return findCreateBox(box);};

		void getBox(const point3d_t &p,int &nx,int &ny,int &nz)const;
		point3d_t getBox(int nx,int ny,int nz)const;

		unsigned int numBoxes()const {return numBox;};
		iterator begin();
		iterator end();
		const_iterator begin()const;
		const_iterator end()const;

	protected:

		typedef std::map<int,std::map<int,std::map<int,T> > > hashContainer_t;
		typedef std::map<int,std::map<int,std::map<int,T> > > hashXcont_t;
		typedef std::map<int,std::map<int,T> >  hashYcont_t;
		typedef std::map<int,T> hashZcont_t;


		point3d_t getBox(const point3d_t &)const;
		T & findCreateBox(const point3d_t &);

		PFLOAT cellsize;
		unsigned int numElements;
		unsigned int numBox;
		hashContainer_t container;
};


template<class T>
class hash3d_iterator
{
	public:
		void operator ++();
		void operator ++(int) {return operator ++();};
		T & operator *() {return z->second;};

		typename hash3d_t<T>::hashXcont_t::iterator x,xend;
		typename hash3d_t<T>::hashYcont_t::iterator y,yend;
		typename hash3d_t<T>::hashZcont_t::iterator z,zend;
};

template<class T>
class hash3d_const_iterator
{
	public:
		void operator ++();
		void operator ++(int) {return operator ++();};
		const T & operator *() {return z->second;};

		typename hash3d_t<T>::hashXcont_t::const_iterator x,xend;
		typename hash3d_t<T>::hashYcont_t::const_iterator y,yend;
		typename hash3d_t<T>::hashZcont_t::const_iterator z,zend;
};

template<class T>
void hash3d_iterator<T>::operator ++()
{
	z++;
	if(z==zend)
	{
		y++;
		if(y==yend)
		{
			x++;
			if(x==xend) return;
			y=x->second.begin();
			yend=x->second.end();
		}
		z=y->second.begin();
		zend=y->second.end();
	}
}

template<class T>
void hash3d_const_iterator<T>::operator ++()
{
	z++;
	if(z==zend)
	{
		y++;
		if(y==yend)
		{
			x++;
			if(x==xend) return;
			y=x->second.begin();
			yend=x->second.end();
		}
		z=y->second.begin();
		zend=y->second.end();
	}
}
/*
template<class T>
inline bool operator == (const hash3d_iterator<T> &a,const hash3d_iterator<T> &b)
{
	if(a.x!=b.x) return false;
	if(a.y!=b.y) return false;
	if(a.z!=b.z) return false;
	return true;
}
*/
template<class T>
inline bool operator != (const hash3d_iterator<T> &a,const hash3d_iterator<T> &b)
{
	// SPEED HACK (also necessary since we don't have the exact last iterator in constant time
	if(b.x==b.xend) return (a.x!=b.x);
		
	if(a.x!=b.x) return true;
	if(a.y!=b.y) return true;
	if(a.z!=b.z) return true;
	return false;
}
/*
template<class T>
inline bool operator == (const hash3d_const_iterator<T> &a,const hash3d_const_iterator<T> &b)
{
	if(a.x!=b.x) return false;
	if(a.y!=b.y) return false;
	if(a.z!=b.z) return false;
	return true;
}
*/
template<class T>
inline bool operator != (const hash3d_const_iterator<T> &a,const hash3d_const_iterator<T> &b)
{
	// SPEED HACK (also necessary since we don't have the exact last iterator in constant time
	if(b.x==b.xend) return (a.x!=b.x);
		
	if(a.x!=b.x) return true;
	if(a.y!=b.y) return true;
	if(a.z!=b.z) return true;
	return false;
}

template<class T>
hash3d_t<T>::hash3d_t(PFLOAT cell,unsigned int size)
{
	cellsize=cell;
	numBox=0;
}

template<class T>
hash3d_t<T>::~hash3d_t()
{
}

template<class T>
void hash3d_t<T>::clear()
{
	container.clear();
	numBox=0;
}

template<class T>
point3d_t hash3d_t<T>::getBox(const point3d_t &p)const
{
	int nx,ny,nz;
	getBox(p,nx,ny,nz);
	point3d_t box(cellsize*(PFLOAT)nx,cellsize*(PFLOAT)ny,cellsize*(PFLOAT)nz);
	box.x+=0.5*cellsize;
	box.y+=0.5*cellsize;
	box.z+=0.5*cellsize;
	return box;
}

template<class T>
point3d_t hash3d_t<T>::getBox(int nx,int ny,int nz)const
{
	point3d_t box(cellsize*(PFLOAT)nx,cellsize*(PFLOAT)ny,cellsize*(PFLOAT)nz);
	box.x+=0.5*cellsize;
	box.y+=0.5*cellsize;
	box.z+=0.5*cellsize;
	return box;
}

template<class T>
void hash3d_t<T>::getBox(const point3d_t &p,int &nx,int &ny,int &nz)const
{
	nx=(int)(p.x/cellsize);
	ny=(int)(p.y/cellsize);
	nz=(int)(p.z/cellsize);

	if(p.x<0.0) nx--;
	if(p.y<0.0) ny--;
	if(p.z<0.0) nz--;
}
	
template<class T>
T & hash3d_t<T>::findCreateBox(const point3d_t &p)
{
	int bx,by,bz;
	getBox(p,bx,by,bz);
	typename hashXcont_t::iterator x=container.find(bx);
	if(x==container.end()) 
	{
		numBox++;
		return container[bx][by][bz];
	}
	typename hashYcont_t::iterator y=x->second.find(by);
	if(y==x->second.end()) 
	{
		numBox++;
		return x->second[by][bz];
	}
	typename hashZcont_t::iterator z=y->second.find(bz);
	if(z==y->second.end()) 
	{
		numBox++;
		return y->second[bz];
	}
	return z->second;
}

template<class T>
const T * hash3d_t<T>::findExistingBox(const point3d_t &p)const
{
	int bx,by,bz;
	getBox(p,bx,by,bz);
	typename hashXcont_t::const_iterator x=container.find(bx);
	if(x==container.end()) return NULL;
	typename hashYcont_t::const_iterator y=x->second.find(by);
	if(y==x->second.end()) return NULL;
	typename hashZcont_t::const_iterator z=y->second.find(bz);
	if(z==y->second.end()) return NULL;
	return &(z->second);
}

template<class T>
const T * hash3d_t<T>::findExistingBox(int bx,int by,int bz)const
{
	typename hashXcont_t::const_iterator x=container.find(bx);
	if(x==container.end()) return NULL;
	typename hashYcont_t::const_iterator y=x->second.find(by);
	if(y==x->second.end()) return NULL;
	typename hashZcont_t::const_iterator z=y->second.find(bz);
	if(z==y->second.end()) return NULL;
	return &(z->second);
}

template<class T>
T * hash3d_t<T>::findExistingBox(const point3d_t &p)
{
	int bx,by,bz;
	getBox(p,bx,by,bz);
	typename hashXcont_t::iterator x=container.find(bx);
	if(x==container.end()) return NULL;
	typename hashYcont_t::iterator y=x->second.find(by);
	if(y==x->second.end()) return NULL;
	typename hashZcont_t::iterator z=y->second.find(bz);
	if(z==y->second.end()) return NULL;
	return &(z->second);
}

template<class T>
T * hash3d_t<T>::findExistingBox(int bx,int by,int bz)
{
	typename hashXcont_t::iterator x=container.find(bx);
	if(x==container.end()) return NULL;
	typename hashYcont_t::iterator y=x->second.find(by);
	if(y==x->second.end()) return NULL;
	typename hashZcont_t::iterator z=y->second.find(bz);
	if(z==y->second.end()) return NULL;
	return &(z->second);
}

template<class T>
void hash3d_t<T>::insert(const T &e)
{
	int bx,by,bz;
	getBox(e.position(),bx,by,bz);
	typename hashXcont_t::iterator x=container.find(bx);
	if(x==container.end()) 
	{
		numBox++;
		container[bx][by][bz]=e;
		return;
	}
	typename hashYcont_t::iterator y=x->second.find(by);
	if(y==x->second.end()) 
	{
		numBox++;
		x->second[by][bz]=e;
		return;
	}
	typename hashZcont_t::iterator z=y->second.find(bz);
	if(z==y->second.end()) 
	{
		numBox++;
		y->second[bz]=e;
		return;
	}
	z->second=e;
}

template<class T>
hash3d_iterator<T> hash3d_t<T>::begin()
{
	iterator i;
	i.x=container.begin();
	i.xend=container.end();
	if(i.x!=i.xend)
	{
		i.y=i.x->second.begin();
		i.yend=i.x->second.end();
		if(i.y!=i.yend)
		{
			i.z=i.y->second.begin();
			i.zend=i.y->second.end();
		}
	}
	return i;
}

template<class T>
hash3d_iterator<T> hash3d_t<T>::end()
{
	iterator i;
	i.x=container.end();
	i.xend=container.end();
	return i;
}

template<class T>
hash3d_const_iterator<T> hash3d_t<T>::begin()const
{
	const_iterator i;
	i.x=container.begin();
	i.xend=container.end();
	if(i.x!=i.xend)
	{
		i.y=i.x->second.begin();
		i.yend=i.x->second.end();
		if(i.y!=i.yend)
		{
			i.z=i.y->second.begin();
			i.zend=i.y->second.end();
		}
	}
	return i;
}

template<class T>
hash3d_const_iterator<T> hash3d_t<T>::end()const
{
	const_iterator i;
	i.x=container.end();
	i.xend=container.end();
	return i;
}

__END_YAFRAY

#endif
