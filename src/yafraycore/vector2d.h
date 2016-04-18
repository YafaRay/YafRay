/****************************************************************************
 *
 * 			vector2d.h: Vector 2d and point representation and manipulation api 
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
#ifndef __VECTOR2D_H
#define __VECTOR2D_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<cmath>
#include<iostream>

__BEGIN_YAFRAY

class vector2d_t
{
	public:
		vector2d_t() { x = y = 0; };
		vector2d_t(PFLOAT ix, PFLOAT iy) { x=ix;  y=iy; };
		vector2d_t(const vector2d_t &s) { x=s.x;  y=s.y; };
		void set(PFLOAT ix, PFLOAT iy) { x=ix;  y=iy; };
		void normalize();
		// normalizes and returns length
		PFLOAT normLen()
		{
			PFLOAT vl = x*x + y*y;
			if (vl!=0.0) {
				vl = sqrt(vl);
				PFLOAT d = 1.0/vl;
				x*=d; y*=d;
			}
			return vl;
		}
		// normalizes and returns length squared
		PFLOAT normLenSqr()
		{
			PFLOAT vl = x*x + y*y;
			if (vl!=0.0) {
				PFLOAT d = 1.0/sqrt(vl);
				x*=d; y*=d;
			}
			return vl;
		}
		PFLOAT length() const;
		bool null()const { return ((x==0) && (y==0)); }
		vector2d_t& operator = (const vector2d_t &s) { x=s.x;  y=s.y;  return *this;}
		vector2d_t& operator +=(const vector2d_t &s) { x+=s.x;  y+=s.y;return *this;}
		vector2d_t& operator -=(const vector2d_t &s) { x-=s.x;  y-=s.y;return *this;}
		vector2d_t& operator /=(PFLOAT s) { x/=s;  y/=s; return *this;}
		vector2d_t& operator *=(PFLOAT s) { x*=s;  y*=s; return *this;}
		void abs() { x=fabs(x);  y=fabs(y); }
		~vector2d_t() {};
		PFLOAT x,y;
};

class point2d_t
{
	public:
		point2d_t() { x = y = 0; }
		point2d_t(PFLOAT ix, PFLOAT iy ) { x=ix;  y=iy; }
		point2d_t(const point2d_t &s) { x=s.x;  y=s.y; }
		point2d_t(const vector2d_t &v) { x=v.x;  y=v.y; }
		void set(PFLOAT ix, PFLOAT iy) { x=ix;  y=iy; }
		PFLOAT length() const;
		point2d_t& operator= (const point2d_t &s) { x=s.x;  y=s.y; return *this; }
		point2d_t& operator *=(PFLOAT s) { x*=s;  y*=s;  return *this;}
		point2d_t& operator +=(PFLOAT s) { x+=s;  y+=s;  return *this;}
		point2d_t& operator +=(const point2d_t &s) { x+=s.x;  y+=s.y;  return *this;}
		point2d_t& operator -=(const point2d_t &s) { x-=s.x;  y-=s.y;  return *this;}
		~point2d_t() {};
		PFLOAT x,y;
};


inline std::ostream & operator << (std::ostream &out,const vector2d_t &v)
{
	out<<"("<<v.x<<","<<v.y<<")";
	return out;
}

inline std::ostream & operator << (std::ostream &out,const point2d_t &p)
{
	out<<"("<<p.x<<","<<p.y<<")";
	return out;
}

inline PFLOAT operator * ( const vector2d_t &a,const vector2d_t &b)
{
	return (a.x*b.x+a.y*b.y);
}

inline vector2d_t operator * ( PFLOAT f,const vector2d_t &b)
{
	return vector2d_t(f*b.x,f*b.y);
}

inline vector2d_t operator * (const vector2d_t &b,PFLOAT f)
{
	return vector2d_t(f*b.x,f*b.y);
}

inline point2d_t operator * (PFLOAT f,const point2d_t &b)
{
	return point2d_t(f*b.x,f*b.y);
}

inline vector2d_t operator / (const vector2d_t &b,PFLOAT f)
{
	return vector2d_t(b.x/f,b.y/f);
}

inline point2d_t operator / (const point2d_t &b,PFLOAT f)
{
	return point2d_t(b.x/f,b.y/f);
}

inline point2d_t operator * (const point2d_t &b,PFLOAT f)
{
	return point2d_t(b.x*f,b.y*f);
}

inline vector2d_t operator / (PFLOAT f,const vector2d_t &b)
{
	return vector2d_t(b.x/f,b.y/f);
}

inline vector2d_t  operator - ( const vector2d_t &a,const vector2d_t &b)
{
	return vector2d_t(a.x-b.x,a.y-b.y);
}

inline vector2d_t  operator - ( const point2d_t &a,const point2d_t &b)
{
	return vector2d_t(a.x-b.x,a.y-b.y);
}

inline point2d_t  operator - ( const point2d_t &a,const vector2d_t &b)
{
	return point2d_t(a.x-b.x,a.y-b.y);
}

inline vector2d_t  operator - ( const vector2d_t &b)
{
	return vector2d_t(-b.x,-b.y);
}

inline vector2d_t  operator + ( const vector2d_t &a,const vector2d_t &b)
{
	return vector2d_t(a.x+b.x,a.y+b.y);
}

inline point2d_t  operator + ( const point2d_t &a,const point2d_t &b)
{
	return point2d_t(a.x+b.x,a.y+b.y);
}

inline point2d_t  operator + ( const point2d_t &a,const vector2d_t &b)
{
	return point2d_t(a.x+b.x,a.y+b.y);
}

inline bool  operator == ( const point2d_t &a,const point2d_t &b)
{
	return ((a.x==b.x) && (a.y==b.y));
}

inline bool  operator == ( const vector2d_t &a,const vector2d_t &b)
{
	if(a.x!=b.x) return false;
	if(a.y!=b.y) return false;
	return true;
}

inline bool  operator != ( const vector2d_t &a,const vector2d_t &b)
{
	if(a.x!=b.x) return true;
	if(a.y!=b.y) return true;
	return false;
}

inline PFLOAT vector2d_t::length()const
{
	return sqrt(x*x+y*y);
}

inline PFLOAT point2d_t::length()const
{
	return sqrt(x*x+y*y);
}

inline void vector2d_t::normalize()
{
	PFLOAT len = x*x + y*y;
	if (len!=0)
	{
		len = 1.0/sqrt(len);
		x *= len;
		y *= len;
	}
}

inline vector2d_t toVector(const point2d_t &p)
{
	return vector2d_t(p.x,p.y);
}

__END_YAFRAY

#endif
