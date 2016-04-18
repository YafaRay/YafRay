#include<limits>
#include "triangletools.h"
#include "vector2d.h"

using namespace std;

__BEGIN_YAFRAY

inline point2d_t to2d(const point3d_t &p)
{
	return point2d_t(p.x, p.y);
}

struct square_t
{
	bool isInside(const point2d_t &p)const
	{
		return
			p.x >= x1 && p.x <= x2 &&
			p.y >= y1 && p.y <= y2 ;
	};
	bool isInside(const point3d_t &p)const
	{
		return
			p.x >= x1 && p.x <= x2 &&
			p.y >= y1 && p.y <= y2 ;
	};

	PFLOAT x1,x2,y1,y2;
};

class planeEquation_t
{
	public:
		planeEquation_t(const point3d_t &a,const vector3d_t &N)
		{
			A=-N.x;
			B=-N.y;
			C=toVector(a)*N;
			PFLOAT temp=(N.z!=0.0) ? 1.0/N.z : 0.0;
			bad=(N.z!=0.0) ? false : true;
			A*=temp;
			B*=temp;
			C*=temp;
		};
		PFLOAT getZ(PFLOAT x,PFLOAT y)const { /*if(bad) cout<<"bad"<<endl;*/return A*x+B*y+C;};
		bool isPerp()const {return bad;};
	protected:
		PFLOAT A,B,C;
		bool bad;
};

static bool intersectY(const point3d_t &a,const point3d_t &b,PFLOAT y,PFLOAT x1,PFLOAT x2,point3d_t &res)
{
	res.y=y;
	PFLOAT diffX=b.x-a.x;
	PFLOAT diffY=b.y-a.y;
	PFLOAT diffZ=b.z-a.z;
	if(diffY==0.0) return false;
	PFLOAT lambda=(y-a.y)/diffY;
	if((lambda<0) || (lambda>1.0)) return false;
	res.x=lambda*diffX+a.x;
	res.z=lambda*diffZ+a.z;
	return ((res.x>=x1) && (res.x<=x2));
}

static bool intersectX(const point3d_t &a,const point3d_t &b,PFLOAT x,PFLOAT y1,PFLOAT y2,point3d_t &res)
{
	res.x=x;
	PFLOAT diffY=b.y-a.y;
	PFLOAT diffX=b.x-a.x;
	PFLOAT diffZ=b.z-a.z;
	if(diffX==0.0) return false;
	PFLOAT lambda=(x-a.x)/diffX;
	if((lambda<0) || (lambda>1.0)) return false;
	res.y=lambda*diffY+a.y;
	res.z=lambda*diffZ+a.z;
	return ((res.y>=y1) && (res.y<=y2));
}


template<class F>
bool applyVectorIntersect(const point3d_t &a,const point3d_t &b,
		const square_t &q,F &func)
{
	point3d_t pr;
	int c=0;
	if(intersectX(a, b, q.x1, q.y1, q.y2, pr)) {if(!func(pr)) return false;c++;}
	if(intersectX(a, b, q.x2, q.y1, q.y2, pr)) {if(!func(pr)) return false;c++;}
	if(c>1) return true;
	if(intersectY(a, b, q.y1, q.x1, q.x2, pr)) {if(!func(pr)) return false;c++;}
	if(c>1) return true;
	if(intersectY(a, b, q.y2, q.x1, q.x2, pr)) if(!func(pr)) return false;
	return true;
}

static bool isInside(const point2d_t &p,
		          const point2d_t &a,
		          const point2d_t &b,
		          const point2d_t &c)
{
		 vector2d_t w1(b.y-a.y,-(b.x-a.x));
		 vector2d_t pa=p-a;
		 vector2d_t ca=c-a;
		 if((w1*pa) * (w1*ca) < 0) return false;

		 vector2d_t w2(c.y-b.y,-(c.x-b.x));
		 vector2d_t pb=p-b;
		 vector2d_t ab=a-b;
		 if((w2*pb) * (w2*ab) < 0) return false;
		 
		 vector2d_t w3(a.y-c.y,-(a.x-c.x));
		 vector2d_t pc=p-c;
		 vector2d_t bc=b-c;
		 return ((w3*pc) * (w3*bc) >= 0); 
}


template<class F>
bool intersectApply(const point3d_t &a,const point3d_t &b,
										const point3d_t &c,const square_t &q,planeEquation_t &plane,F &func)
{
	if(!applyVectorIntersect(a, b, q, func)) return false;
	if(!applyVectorIntersect(b, c, q, func)) return false;
	if(!applyVectorIntersect(c, a, q, func)) return false;
	if (q.isInside(a)) if(!func(a)) return false;
	if (q.isInside(b)) if(!func(b)) return false;
	if (q.isInside(c)) if(!func(c)) return false;

	if(!plane.isPerp())
	{
		point2d_t qp ( q.x1, q.y1 );
		point2d_t a2=to2d(a),b2=to2d(b),c2=to2d(c);
		if (isInside(qp, a2,b2,c2)) if(!func(point3d_t(qp.x, qp.y, plane.getZ(qp.x, qp.y)))) return false;
		qp.set( q.x2, q.y1 );
		if (isInside(qp, a2,b2,c2)) if(!func(point3d_t(qp.x, qp.y, plane.getZ(qp.x, qp.y)))) return false;
		qp.set( q.x2, q.y2 );
		if (isInside(qp, a2,b2,c2)) if(!func(point3d_t(qp.x, qp.y, plane.getZ(qp.x, qp.y)))) return false;
		qp.set( q.x1, q.y2 );
		if (isInside(qp, a2,b2,c2)) if(!func(point3d_t(qp.x, qp.y, plane.getZ(qp.x, qp.y)))) return false;
	}
	return true;
}

struct checkPosition_f
{
	checkPosition_f(PFLOAT z):
		Z(z),decision(trianglePosition_t::NONE) {};

	bool operator () (const point3d_t & p)
	{
		const PFLOAT &z=p.z;
		if(z==Z) decision=trianglePosition_t::INTERSECT;
		else if(decision==trianglePosition_t::NONE)
		{
			if(z<Z) decision=trianglePosition_t::LOWER;
			else decision=trianglePosition_t::HIGHER;
		}
		else
			if( ((z<Z) && (decision==trianglePosition_t::HIGHER)) || 
					((z>Z) && (decision==trianglePosition_t::LOWER)) ) 
				decision=trianglePosition_t::INTERSECT;
		
		if(decision==trianglePosition_t::INTERSECT) return false;
		else return true;
	};

	PFLOAT Z;
	int decision;
};

struct maximize_f
{
	maximize_f(): Z(-numeric_limits<PFLOAT>::infinity()) {};

	bool operator () (const point3d_t & p)
	{
		if(p.z>Z) Z=p.z;
		else return true;
	};

	PFLOAT Z;
};

struct minimize_f
{
	minimize_f(): Z(numeric_limits<PFLOAT>::infinity()) {};

	bool operator () (const point3d_t & p)
	{
		if(p.z<Z) Z=p.z;
		else return true;
	};

	PFLOAT Z;
};

int cheapPosition(const triangle_t &tri,const bound_t &bound,PFLOAT Z,int axis)
{
	PFLOAT az=0,bz=0,cz=0;
	int decision=trianglePosition_t::NONE;
	point3d_t min,max;
	bound.get(min,max);
	bool inside=false;
	
	switch(axis)
	{
		case AXISX:
			az=tri.a->x;bz=tri.b->x;cz=tri.c->x;
			inside  =((tri.a->y>=min.y) && (tri.a->y<=max.y) && (tri.a->z>=min.z) && (tri.a->z<=max.z));
			inside&=((tri.b->y>=min.y) && (tri.b->y<=max.y) && (tri.b->z>=min.z) && (tri.b->z<=max.z));
			inside&=((tri.c->y>=min.y) && (tri.c->y<=max.y) && (tri.c->z>=min.z) && (tri.c->z<=max.z));
			break;
		case AXISY:
			az=tri.a->y;bz=tri.b->y;cz=tri.c->y;
			inside  =((tri.a->x>=min.x) && (tri.a->x<=max.x) && (tri.a->z>=min.z) && (tri.a->z<=max.z));
			inside&=((tri.b->x>=min.x) && (tri.b->x<=max.x) && (tri.b->z>=min.z) && (tri.b->z<=max.z));
			inside&=((tri.c->x>=min.x) && (tri.c->x<=max.x) && (tri.c->z>=min.z) && (tri.c->z<=max.z));
			break;
		case AXISZ:
			az=tri.a->z;bz=tri.b->z;cz=tri.c->z;
			inside  =((tri.a->x>=min.x) && (tri.a->x<=max.x) && (tri.a->y>=min.y) && (tri.a->y<=max.y));
			inside&=((tri.b->x>=min.x) && (tri.b->x<=max.x) && (tri.b->y>=min.y) && (tri.b->y<=max.y));
			inside&=((tri.c->x>=min.x) && (tri.c->x<=max.x) && (tri.c->y>=min.y) && (tri.c->y<=max.y));
			break;
	}
	
	int intersection;
	if(inside)
		intersection=trianglePosition_t::INTERSECT;
	else
		intersection=trianglePosition_t::NONE; // possible intersection, we don't know

	if(az==Z) return intersection;
	if(az<Z) decision=trianglePosition_t::LOWER; else decision=trianglePosition_t::HIGHER;
	if(bz==Z) return intersection;
	if((bz>Z) && (decision==trianglePosition_t::LOWER)) return intersection;
	if((bz<Z) && (decision==trianglePosition_t::HIGHER)) return intersection;
	if(cz==Z) return intersection;
	if((cz>Z) && (decision==trianglePosition_t::LOWER)) return intersection;
	if((cz<Z) && (decision==trianglePosition_t::HIGHER)) return intersection;
	
	return decision;
}

int expensivePosition(const triangle_t &tri,const bound_t &bound,PFLOAT Z,int axis)
{
	const point3d_t &a3=*(tri.a),&b3=*(tri.b),&c3=*(tri.c);
	point3d_t bmin,bmax;
	const vector3d_t &n=tri.N();
	bound.get(bmin,bmax);

	point3d_t a=a3,b=b3,c=c3;
	vector3d_t N;
	point3d_t planepoint;
	square_t q;
	
	switch(axis)
	{
		case AXISX:
			q.x1=bmin.z;
			q.x2=bmax.z;
			q.y1=bmin.y;
			q.y2=bmax.y;
			N.x=n.z;N.y=n.y;N.z=n.x;
			swap(a.x, a.z);
			swap(b.x, b.z);
			swap(c.x, c.z);
			planepoint.x=a3.z;planepoint.y=a3.y;planepoint.z=a3.x;
			break;
		case AXISY:
			q.x1=bmin.x;
			q.x2=bmax.x;
			q.y1=bmin.z;
			q.y2=bmax.z;
			N.x=n.x;N.y=n.z;N.z=n.y;
			swap(a.y, a.z);
			swap(b.y, b.z);
			swap(c.y, c.z);
			planepoint.x=a3.x;planepoint.y=a3.z;planepoint.z=a3.y;
			break;
		case AXISZ:
			q.x1=bmin.x;
			q.x2=bmax.x;
			q.y1=bmin.y;
			q.y2=bmax.y;
			N.x=n.x;N.y=n.y;N.z=n.z;
			planepoint.x=a3.x;planepoint.y=a3.y;planepoint.z=a3.z;
			break;
	}
	checkPosition_f func(Z);
	planeEquation_t plane(planepoint,N);
	intersectApply(a,b,c,q,plane,func);
	return func.decision;
}

inline PFLOAT cheapMaximize(const triangle_t &tri,int axis)
{
	const point3d_t &a=*(tri.a),&b=*(tri.b),&c=*(tri.c);
	PFLOAT Z=0;
	switch(axis)
	{
		case AXISX:
			Z=a.x;
			if(b.x>Z) Z=b.x;
			if(c.x>Z) Z=c.x;
			break;
		case AXISY:
			Z=a.y;
			if(b.y>Z) Z=b.y;
			if(c.y>Z) Z=c.y;
			break;
		case AXISZ: 
			Z=a.z;
			if(b.z>Z) Z=b.z;
			if(c.z>Z) Z=c.z;
			break;
	}
	return Z;
}

inline PFLOAT cheapMinimize(const triangle_t &tri,int axis)
{
	const point3d_t &a=*(tri.a),&b=*(tri.b),&c=*(tri.c);
	PFLOAT Z=0;
	switch(axis)
	{
		case AXISX:
			Z=a.x;
			if(b.x<Z) Z=b.x;
			if(c.x<Z) Z=c.x;
			break;
		case AXISY:
			Z=a.y;
			if(b.y<Z) Z=b.y;
			if(c.y<Z) Z=c.y;
			break;
		case AXISZ: 
			Z=a.z;
			if(b.z<Z) Z=b.z;
			if(c.z<Z) Z=c.z;
			break;
	}
	return Z;
}

template<class F>
PFLOAT expensiveMaxMin(const triangle_t &tri,const square_t &q,int axis,F &func)
{
	const point3d_t &a3=*(tri.a),&b3=*(tri.b),&c3=*(tri.c);
	const vector3d_t &n=tri.N();

	point3d_t a=a3,b=b3,c=c3;
	vector3d_t N;
	point3d_t planepoint;
	
	switch(axis)
	{
		case AXISX:
			N.x=n.z;N.y=n.y;N.z=n.x;
			swap(a.x, a.z);
			swap(b.x, b.z);
			swap(c.x, c.z);
			planepoint.x=a3.z;planepoint.y=a3.y;planepoint.z=a3.x;
			break;
		case AXISY:
			N.x=n.x;N.y=n.z;N.z=n.y;
			swap(a.y, a.z);
			swap(b.y, b.z);
			swap(c.y, c.z);
			planepoint.x=a3.x;planepoint.y=a3.z;planepoint.z=a3.y;
			break;
		case AXISZ:
			N.x=n.x;N.y=n.y;N.z=n.z;
			planepoint.x=a3.x;planepoint.y=a3.y;planepoint.z=a3.z;
			break;
	}
	planeEquation_t plane(planepoint,N);
	intersectApply(a,b,c,q,plane,func);
	return func.Z;
}

PFLOAT minimize(const vector<triangle_t *> faces,const bound_t &bound,int axis)
{
	point3d_t bmin,bmax;
	bound.get(bmin,bmax);

	square_t q;
	PFLOAT sequrity=MIN_RAYDIST;
	switch(axis)
	{
		case AXISX:
			q.x1=bmin.z;
			q.x2=bmax.z;
			q.y1=bmin.y;
			q.y2=bmax.y;
			sequrity*=bound.longX();
			break;
		case AXISY:
			q.x1=bmin.x;
			q.x2=bmax.x;
			q.y1=bmin.z;
			q.y2=bmax.z;
			sequrity*=bound.longY();
			break;
		case AXISZ:
			q.x1=bmin.x;
			q.x2=bmax.x;
			q.y1=bmin.y;
			q.y2=bmax.y;
			sequrity*=bound.longZ();
			break;
	}
	PFLOAT Z=numeric_limits<PFLOAT>::infinity();
	for(vector<triangle_t *>::const_iterator i=faces.begin();i!=faces.end();++i)
	{
		const point3d_t &a=*((*i)->a),&b=*((*i)->b),&c=*((*i)->c);
		PFLOAT z;
		minimize_f func;
		if(q.isInside(a) && q.isInside(b) && q.isInside(c)) z=cheapMinimize(**i,axis);
		else z=expensiveMaxMin(**i,q,axis,func);
		if(z<Z) Z=z;
	}
	return Z-sequrity;
}

PFLOAT maximize(const vector<triangle_t *> faces,const bound_t &bound,int axis)
{
	point3d_t bmin,bmax;
	bound.get(bmin,bmax);

	square_t q;
	PFLOAT sequrity=MIN_RAYDIST;
	switch(axis)
	{
		case AXISX:
			q.x1=bmin.z;
			q.x2=bmax.z;
			q.y1=bmin.y;
			q.y2=bmax.y;
			sequrity*=bound.longX();
			break;
		case AXISY:
			q.x1=bmin.x;
			q.x2=bmax.x;
			q.y1=bmin.z;
			q.y2=bmax.z;
			sequrity*=bound.longY();
			break;
		case AXISZ:
			q.x1=bmin.x;
			q.x2=bmax.x;
			q.y1=bmin.y;
			q.y2=bmax.y;
			sequrity*=bound.longZ();
			break;
	}
	PFLOAT Z=-numeric_limits<PFLOAT>::infinity();
	for(vector<triangle_t *>::const_iterator i=faces.begin();i!=faces.end();++i)
	{
		const point3d_t &a=*((*i)->a),&b=*((*i)->b),&c=*((*i)->c);
		PFLOAT z;
		maximize_f func;
		if(q.isInside(a) && q.isInside(b) && q.isInside(c)) z=cheapMaximize(**i,axis);
		else z=expensiveMaxMin(**i,q,axis,func);
		if(z>Z) Z=z;
	}
	return Z+sequrity;
}

__END_YAFRAY
