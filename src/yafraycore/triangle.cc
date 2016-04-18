/****************************************************************************
 *
 * 			triangle.cc: Face representation and manipulation implementation
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

#include "triangle.h"
using namespace std;
#include <cstdio>
#include <cstdlib>

__BEGIN_YAFRAY

triangle_t::triangle_t(point3d_t *va,point3d_t *vb,point3d_t *vc)
{
	a=va;
	b=vb;
	c=vc;
	normal=((*b)-(*a))^((*c)-(*a));
	normal.normalize();
	//uv=NULL;
	hasuv = false;
	has_vcol = false;
	na=nb=nc=NULL;
	ta = tb = tc = NULL;
	shader=NULL;
}

void triangle_t::recNormal()
{
	normal=((*b)-(*a))^((*c)-(*a));
	normal.normalize();
}

triangle_t::triangle_t():normal(0,0,0)
{
	a=NULL;
	b=NULL;
	c=NULL;
	hasuv = false;
	has_vcol = false;
	na=nb=nc=NULL;
	ta = tb = tc = NULL;
	shader=NULL;
}

void triangle_t::setVertices(point3d_t *va,point3d_t *vb,point3d_t *vc)
{
	a=va;
	b=vb;
	c=vc;
	normal=((*b)-(*a))^((*c)-(*a));
	normal.normalize();
	na=nb=nc=NULL;
	ta = tb = tc = NULL;
}

static bool getInterpolation(const vector3d_t &N, const point3d_t &a,const point3d_t &b,const point3d_t &c
		,const point3d_t &h,GFLOAT &fa,GFLOAT &fb,GFLOAT &fc)
//static bool getInterpolation(const point3d_t &a,const point3d_t &b,const point3d_t &c
//		,const point3d_t &h,GFLOAT &fa,GFLOAT &fb,GFLOAT &fc)
{
/*
	vector3d_t va=a-h;
	vector3d_t vb=b-h;
	vector3d_t vc=c-h;
	vector3d_t v1(va.x,vb.x,vc.x);
	vector3d_t v2(va.y,vb.y,vc.y);
	vector3d_t v3(va.z,vb.z,vc.z);
	va=v1^v2;  va.abs();
	vb=v2^v3;  vb.abs();
	vc=v1^v3;  vc.abs();
	PFLOAT smod1=va.x+va.y+va.z;
	PFLOAT smod2=vb.x+vb.y+vb.z;
	PFLOAT smod3=vc.x+vc.y+vc.z;
	if( (smod1>smod2) && (smod1>smod3)) { v1=va; }
	else if( (smod2>smod1) && (smod2>smod3)) { smod1=smod2;  v1=vb; }
	else { smod1=smod3;  v1=vc; }
	if(smod1==0.0)
		return false;
	v1/=smod1;
	fa=v1.x;  fb=v1.y;  fc=v1.z;
	return true;
*/
	PFLOAT uu0, vv0, uu1, vv1, uu2, vv2;
	PFLOAT nx=fabs(N.x), ny=fabs(N.y), nz=fabs(N.z);
	if (nx>=ny && nx>=nz) {
		uu0 = h.y - a.y;
		vv0 = h.z - a.z;
		uu1 = b.y - a.y;
		vv1 = b.z - a.z;
		uu2 = c.y - a.y;
		vv2 = c.z - a.z;
	}
	else if (ny>=nx && ny>=nz) {
		uu0 = h.x - a.x;
		vv0 = h.z - a.z;
		uu1 = b.x - a.x;
		vv1 = b.z - a.z;
		uu2 = c.x - a.x;
		vv2 = c.z - a.z;
	}
	else {
		uu0 = h.x - a.x;
		vv0 = h.y - a.y;
		uu1 = b.x - a.x;
		vv1 = b.y - a.y;
		uu2 = c.x - a.x;
		vv2 = c.y - a.y;
	}

	PFLOAT alpha_numer = uu0*vv2 - vv0*uu2;
	PFLOAT beta_numer = uu1*vv0-vv1*uu0;
	PFLOAT denum =  uu1*vv2-vv1*uu2;

	fb = alpha_numer / denum;
	fc = beta_numer / denum;
	fa = 1.f - fb - fc;
	return true;
}

surfacePoint_t triangle_t::getSurface(point3d_t &h,PFLOAT d,bool orco)const
{
	if( (!hasuv) && (!has_vcol) && (na==NULL) && !orco)
		return surfacePoint_t(NULL, h,h, normal, normal, -1, -1, color_t(0.0), d, shader);
	vector3d_t nn=normal;
	GFLOAT fa,fb,fc;
	//if (!getInterpolation(*a, *b, *c, h, fa, fb, fc))
	//	return surfacePoint_t(NULL, h, h, normal, normal, -1, -1, color_t(0.0), d, shader);
	if (!getInterpolation(normal, *a, *b, *c, h, fa, fb, fc))
		return surfacePoint_t(NULL, h, h, normal, normal, -1, -1, color_t(0.0), d, shader);
	point3d_t orcoP=h;
	if(orco) orcoP=*(a+1)*fa+*(b+1)*fb+*(c+1)*fc;
	if(na!=NULL)
	{
		nn = (*na)*fa+(*nb)*fb+(*nc)*fc;
		nn.normalize();
	}
	GFLOAT u=0, v=0;
	if (hasuv)
	{
		u=uv[0]*fa+uv[2]*fb+uv[4]*fc;
		v=uv[1]*fa+uv[3]*fb+uv[5]*fc;
	}
	color_t vcolor(0.0);
	if (has_vcol) {
		vcolor.set(vcol[0]*fa + vcol[3]*fb + vcol[6]*fc,
							 vcol[1]*fa + vcol[4]*fb + vcol[7]*fc,
							 vcol[2]*fa + vcol[5]*fb + vcol[8]*fc);
	}

	surfacePoint_t temp(NULL, h,orcoP,nn, normal, u, v, vcolor, d, shader, hasuv, has_vcol,orco);
	if (hasuv)
	{
		vector3d_t vb=(*b)-(*a);
		vector3d_t vc=(*c)-(*a);
		GFLOAT lenb=vb.length();

		GFLOAT dub=(uv[2]-uv[0])/lenb;
		GFLOAT dvb=(uv[3]-uv[1])/lenb;
		vb/=lenb;
		GFLOAT nuc=uv[4],nvc=uv[5],proj=vc*vb;

		vc=vc-vb*proj;
		nuc-=proj*dub;
		nvc-=proj*dvb;

		GFLOAT lenc=vc.length();
		GFLOAT duc=(nuc-uv[0])/lenc;
		GFLOAT dvc=(nvc-uv[1])/lenc;
		vc/=lenc;

		GFLOAT projC=temp.NU()*vc,projB=temp.NU()*vb;
		GFLOAT dudu=projC*duc+projB*dub;
		GFLOAT dvdu=projC*dvc+projB*dvb;

		projC=temp.NV()*vc,projB=temp.NV()*vb;
		GFLOAT dudv=projC*duc+projB*dub;
		GFLOAT dvdv=projC*dvc+projB*dvb;

		temp.setGradient(dudu,dudv,dvdu,dvdv);
	}
	// tangents
	if (orco | hasuv)
	{
		vector3d_t tn((*ta)*fa + (*tb)*fb + (*tc)*fc);
		tn.normalize();
		temp.setTangent(tn);
	}
	return temp;
}

__END_YAFRAY
/*
surfacePoint_t  triangle_t::getSurface(point3d_t &h,PFLOAT d)
{
	point3d_t pa,pb,pc;

	if( (uv==NULL) && (na==NULL))
		return surfacePoint_t(NULL,h,normal,normal,-1,-1,d);
	
	PFLOAT mnx=fabs(normal.x);
	PFLOAT mny=fabs(normal.y);
	PFLOAT mnz=fabs(normal.z);
	
	if( (mnz>=mnx) && (mnz>=mny) )
	{
		pa.x=a->x-h.x;
		pa.y=a->y-h.y;
		pa.z=0;
		pb.x=b->x-h.x;
		pb.y=b->y-h.y;
		pb.z=0;
		pc.x=c->x-h.x;
		pc.y=c->y-h.y;
		pc.z=0;
	}
	else if( (mny>=mnx) && (mny>=mnz) )
	{
		pa.x=a->x-h.x;
		pa.y=a->z-h.z;
		pa.z=0;
		pb.x=b->x-h.x;
		pb.y=b->z-h.z;
		pb.z=0;
		pc.x=c->x-h.x;
		pc.y=c->z-h.z;
		pc.z=0;
	}
	else if( (mnx>=mnz) && (mnx>=mny) )
	{
		pa.x=a->y-h.y;
		pa.y=a->z-h.z;
		pa.z=0;
		pb.x=b->y-h.y;
		pb.y=b->z-h.z;
		pb.z=0;
		pc.x=c->y-h.y;
		pc.y=c->z-h.z;
		pc.z=0;
	}
	else 
	{
		cout<<"Error grave proyectando triangulo\n";
		exit(1);
	}
	
	vector3d_t *__na,*__nb,*__nc;
	point3d_t tt;
	GFLOAT *__uva,*__uvb,*__uvc;
	if( (pb.y*pc.y)>=0 )
	{
		__na=na;
		__nb=nb;
		__nc=nc;
		__uva=uv;
		__uvb=uv+2;
		__uvc=uv+4;
	}
	else if( (pa.y*pc.y)>=0 )
	{
		__na=nb;
		__nb=na;
		__nc=nc;
		__uva=uv+2;
		__uvb=uv;
		__uvc=uv+4;
		tt=pa;
		pa=pb;
		pb=tt;
	}
	else if( (pa.y*pb.y)>=0 )
	{
		__na=nc;
		__nb=nb;
		__nc=na;
		__uva=uv+4;
		__uvb=uv+2;
		__uvc=uv;
		tt=pa;
		pa=pc;
		pc=tt;
	}
	else
	{
		cout<<"Grave error en getsurface triangle\n";
		__na=NULL;
		__nb=NULL;
		__nc=NULL;
		__uva=NULL;
		__uvb=NULL;
		__uvc=NULL;
		exit(1);
	}

		

	PFLOAT fab;
	fab=pa.y/(pa.y-pb.y);
	PFLOAT fac;
	fac=pa.y/(pa.y-pc.y);
	PFLOAT ibx;
	ibx=pa.x+(pb.x-pa.x)*fab;
	PFLOAT icx;
	icx=pa.x+(pc.x-pa.x)*fac;
	PFLOAT fbc;
	fbc=ibx/(ibx-icx);

	vector3d_t nn;
	if(na!=NULL)
	{
		 vector3d_t nnb=(*__na)+fab*((*__nb)-(*__na));
		 vector3d_t nnc=(*__na)+fac*((*__nc)-(*__na));
		 nn=nnb+fbc*(nnc-nnb);
		 nn.normalize();
	}
	else nn=normal;
	GFLOAT u,v;
	if(uv!=NULL)
	{
		GFLOAT ub=	__uva[0] + fab*(__uvb[0]-__uva[0]);
		GFLOAT uc=	__uva[0] + fac*(__uvc[0]-__uva[0]);
		u=ub+fbc*(uc-ub);
		GFLOAT vb=	__uva[1] + fab*(__uvb[1]-__uva[1]);
		GFLOAT vc=	__uva[1] + fac*(__uvc[1]-__uva[1]);
		v=vb+fbc*(vc-vb);
	}
	else u=v=-1;
	return surfacePoint_t(NULL,h,nn,normal,u,v,d);
}

bool triangle_t::Z_hit()
{
	PFLOAT a_ab,a_bc,a_ca;
	//if(itsZP()) return false;
	a_ab=FAST_ANGLE(*(a),*(b));
	a_bc=FAST_ANGLE(*(b),*(c));
	a_ca=FAST_ANGLE(*(c),*(a));

	//if(a_ab==0) return false;
	//if(a_bc==0) return false;
	//if(a_ca==0) return false;

	if((a_ab*a_bc)<0) return false;
	if((a_bc*a_ca)<0) return false;
	if((a_ab*a_ca)<0) return false;
	return true;
}


#define RESE(a,b,m)\
do{\
	(a)[0]-=(b)[0]*m;\
	(a)[1]-=(b)[1]*m;\
	(a)[2]-=(b)[2]*m;\
	(a)[3]-=(b)[3]*m;\
}while(0)

void res_eq(PFLOAT (*matrix)[4],PFLOAT *sol)
{
	int cj=0,i,j;
	PFLOAT max;
	int used[3]={0,0,0};

	for(i=0;i<3;++i)
	{
		max=-1;
		for(j=0;j<3;++j)
		{
			PFLOAT lmax=0;
			for(int li=0;li<4;++li)
				if(fabs(matrix[j][li]-matrix[j][i])>lmax) 
					lmax=fabs(matrix[j][li]-matrix[j][i]);
							
			if( !(used[j]) && (max<0))
			{
				max=fabs(matrix[j][i]-lmax);
				cj=j;
			}
			else if( !(used[j]) && (fabs(matrix[j][i]-lmax)<max))
			{
				max=fabs(matrix[j][i]-lmax);
				cj=j;
			}
		}
		if(max<0)
		{
			fprintf(stderr,"Error grave en ecuacion\n");
			cout<<"paso "<<i<<"\n";
			cout<<matrix[0][0]<<" "<<matrix[0][1]<<" "<<matrix[0][2]<<"\n";
			cout<<matrix[1][0]<<" "<<matrix[1][1]<<" "<<matrix[1][2]<<"\n";
			cout<<matrix[2][0]<<" "<<matrix[2][1]<<" "<<matrix[2][2]<<"\n";
			exit(1);
		}
		max=matrix[cj][i];
		matrix[cj][0]/=max;
		matrix[cj][1]/=max;
		matrix[cj][2]/=max;
		matrix[cj][3]/=max;

		used[cj]=i+1;
		PFLOAT mul;
		for(j=0;j<3;++j)
		{
			if(j!=cj)
			{
				mul=matrix[j][i];
				RESE(matrix[j],matrix[cj],mul);
			}
		}
	}
	sol[used[0]-1]=matrix[0][3];
	sol[used[1]-1]=matrix[1][3];
	sol[used[2]-1]=matrix[2][3];
}			

PFLOAT guessZ(point3d_t pa,point3d_t pb,point3d_t pc)
{
	point3d_t tt;
	if( (pb.y*pc.y)>=0 )
	{
	}
	else if( (pa.y*pc.y)>=0 )
	{
		tt=pa;
		pa=pb;
		pb=tt;
	}
	else if( (pa.y*pb.y)>=0 )
	{
		tt=pa;
		pa=pc;
		pc=tt;
	}
	else
	{
		cout<<"Grave error en guessZ triangle\n";
		exit(1);
	}

		

	PFLOAT fab;
	fab=pa.y/(pa.y-pb.y);
	PFLOAT fac;
	fac=pa.y/(pa.y-pc.y);
	PFLOAT ibx;
	ibx=pa.x+(pb.x-pa.x)*fab;
	PFLOAT icx;
	icx=pa.x+(pc.x-pa.x)*fac;
	PFLOAT fbc;
	fbc=ibx/(ibx-icx);

	PFLOAT zb=(pa.z)+fab*(pb.z-pa.z);
	PFLOAT zc=(pa.z)+fac*(pc.z-pa.z);
	PFLOAT Z=zb+fbc*(zc-zb);
	return Z;
}

double triangle_t::Z_intersect()
{
	//return guessZ(*a,*b,*c);
	vector3d_t pn=(*b-*a)^(*c-*a);
	return (pn*(*a-point3d_t(0,0,0)))/pn.z;
}
*/
