/****************************************************************************
 *
 * 			mesh.cc: Mesh object implementation 
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

extern int pcount;

#include "mesh.h"
using namespace std;
#include <iostream>
#include<set>
#include"geometree.h"
#include"triangletools.h"

#define BOX_MIN 0.00001
//#define BOX_MIN MIN_RAYDIST
//#include <stdio.h>
//
__BEGIN_YAFRAY

bound_t face_calc_bound(const vector<triangle_t *> &v)
{
	int size=v.size();
	if(size==0) return bound_t(point3d_t(),point3d_t());
	PFLOAT maxx,maxy,maxz,minx,miny,minz;
	maxx=minx=v[0]->a->x;
	maxy=miny=v[0]->a->y;
	maxz=minz=v[0]->a->z;
	for(int i=0;i<size;++i)
	{
		point3d_t p=*(v[i]->a);
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
		p=*(v[i]->b);
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
		p=*(v[i]->c);
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
	}
	minx-=BOX_MIN;
	miny-=BOX_MIN;
	minz-=BOX_MIN;
	maxx+=BOX_MIN;
	maxy+=BOX_MIN;
	maxz+=BOX_MIN;
	return bound_t(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
}

bool face_is_in_bound(triangle_t * const & t,bound_t &b)
{
	if(b.includes(*(t->a))) return true;
	if(b.includes(*(t->b))) return true;
	if(b.includes(*(t->c))) return true;
	return false;
}

point3d_t face_get_pos(triangle_t * const & t)
{
	point3d_t r=*(t->a);
	r=r+ *(t->b);
	r=r+ *(t->c);
	r=r/3;
	return r;
}

typedef pureBspTree_t<vector<triangle_t*> > tnode_t;
//typedef geomeTree_t<vector<triangle_t*> > tnode_t;
//typedef gBoundTreeNode_t<triangle_t *> tnode_t;

static PFLOAT foo1,foo2;

template<class T>
inline typename vector<T>::iterator convertIterator(vector<T> &d,const vector<T> &v,
		typename vector<T>::const_iterator i)
{
	if(i==v.end()) return d.end();
	return (i-v.begin())+d.begin();
}

template<class T>
inline T * convertPointer(vector<T> &d,const vector<T> &v,const T *p)
{
	return (T *)((p-&v[0])+&d[0]);
}

tnode_t * buildTriangleTree(std::vector<triangle_t*> *v, unsigned int maxdepth,
		const bound_t &bound,unsigned int dtol=1,unsigned int depth=1,unsigned int lostd=0,
		PFLOAT &avgdepth=foo1, PFLOAT &avgsize=foo2);

meshObject_t::meshObject_t(const vector<point3d_t> &ver, const vector<vector3d_t> &nor,
				const vector<triangle_t> &ts, const vector<GFLOAT> &fuv, const vector<CFLOAT> &fvcol)
{
	vertices=ver;
	normals=nor;
	triangles=ts;
	unt=true;
	hasorco=false;
	if ( (ver.empty()) || (ts.empty()))
		cout<<"Error null mesh\n";
	shader=NULL;
	if(ver.size()) recalcBound();
	facesuv = fuv;
	faces_vcol = fvcol;

	for(vector<triangle_t>::iterator i=triangles.begin();i!=triangles.end();++i)
	{
		i->a=convertPointer(vertices,ver,i->a);
		i->b=convertPointer(vertices,ver,i->b);
		i->c=convertPointer(vertices,ver,i->c);
		if(normals.size())
		{
			i->na=convertPointer(normals,nor,i->na);
			i->nb=convertPointer(normals,nor,i->nb);
			i->nc=convertPointer(normals,nor,i->nc);
		}
		i->uv=convertIterator(facesuv,fuv,i->uv);
		i->vcol=convertIterator(faces_vcol,fvcol,i->vcol);
	}
		
//	vector<triangle_t *> *ltri=new vector<triangle_t *>(ts.size());
//	for(vector<triangle_t>::iterator i=triangles.begin();i!=triangles.end();++i)
//		(*ltri)[i-triangles.begin()]=&(*i);

	//tree=buildGenericTree(ltri,face_calc_bound,face_is_in_bound,face_get_pos,10);
//	unsigned int maxdepth = (unsigned int)(8.0 + 1.8755035531556525*log((PFLOAT)triangles.size()));
//	tree=buildTriangleTree(ltri, maxdepth, face_calc_bound(*ltri),4);
	
	// Lynx ->
	const triangle_t **tris=new const triangle_t*[triangles.size()];
	for(unsigned int i=0;i<triangles.size();++i)
		tris[i] = &(triangles[i]);
	n_tree = new kdTree_t(tris, triangles.size(), -1, -1, 1.2, 0.40 );
	delete[] tris;
}

meshObject_t::meshObject_t(bool _hasorco, const matrix4x4_t &M, const vector<point3d_t> &ver, const vector<vector3d_t> &nor,
				const vector<triangle_t> &ts, const vector<GFLOAT> &fuv, const vector<CFLOAT> &fvcol)
{
	hasorco = _hasorco;
	vertices = ver;
	normals = nor;
	triangles = ts;
	unt = true;
	if ((ver.empty()) || (ts.empty()))
		cout << "Error null mesh\n";
	shader = NULL;
	//if(ver!=NULL) recalcBound();
	facesuv = fuv;
	faces_vcol = fvcol;

	for(vector<triangle_t>::iterator i=triangles.begin();i!=triangles.end();++i)
	{
		i->a=convertPointer(vertices,ver,i->a);
		i->b=convertPointer(vertices,ver,i->b);
		i->c=convertPointer(vertices,ver,i->c);
		if(normals.size())
		{
			i->na=convertPointer(normals,nor,i->na);
			i->nb=convertPointer(normals,nor,i->nb);
			i->nc=convertPointer(normals,nor,i->nc);
		}
		i->uv=convertIterator(facesuv,fuv,i->uv);
		i->vcol=convertIterator(faces_vcol,fvcol,i->vcol);
	}

	tree=NULL;
	n_tree=0;
	transform(M);
}

void meshObject_t::autoSmooth(PFLOAT angle)
{
	// if no smoothing needed, normal equal to geometric normal,
	// is returned automatically in surfacepoint, no calculation needed
	if (angle<1) return;
	unsigned int i1, i2, i3;
	vector<triangle_t>::iterator tri;
	// if everything is smoothed, only need as many normals as there are vertices
	if (angle>=180)
	{
		normals.resize(vertices.size());
		for (tri=triangles.begin();tri!=triangles.end();tri++)
		{
			i1 = tri->a - (&vertices[0]);
			i2 = tri->b - (&vertices[0]);
			i3 = tri->c - (&vertices[0]);
			normals[i1] += tri->N();
			normals[i2] += tri->N();
			normals[i3] += tri->N();
			tri->na = &normals[i1];
			tri->nb = &normals[i2];
			tri->nc = &normals[i3];
		}
		for (i1=0;i1<normals.size();i1++)
			normals[i1].normalize();
		return;
	}

	// angle dependant smoothing
	PFLOAT cosa = cos(angle*M_PI/180.0);
	vector<vector<triangle_t*> > cnx(vertices.size());
	for (tri=triangles.begin();tri!=triangles.end();tri++)
	{
		cnx[tri->a - (&vertices[0])].push_back(&(*tri));
		cnx[tri->b - (&vertices[0])].push_back(&(*tri));
		cnx[tri->c - (&vertices[0])].push_back(&(*tri));
	}
	vector<triangle_t*>::const_iterator vtri;

	// Concerning memory usage, since depending on smoothing angle, normals could be shared,
	// this could be further optimized to add only as many normals as needed.
	// Similarly, faces which are not smoothed at all don't need vtxnorms so could be skipped.
	// Array size of 3*number_of_triangles is really only the worst possible case.
	// However, for a minimum storage triangle class (index only),
	// for efficient access, it would probably be better to have a consistent layout.
	// Another optimization is to only store normals in 2D form for all normals
	// ie. only xy (or any other pair), z=sqrt(1-x^2-y^2) at runtime when needed, or in polar form (discard r),
	// though both could be more overhead in terms of extra decoding time needed, first one less so probably.
	normals.resize(3*triangles.size());
	unsigned int idx=0;
	for (tri=triangles.begin();tri!=triangles.end();tri++)
	{
		vector3d_t N = tri->N();
		i1 = tri->a - (&vertices[0]);
		i2 = tri->b - (&vertices[0]);
		i3 = tri->c - (&vertices[0]);

		vector3d_t sn(0, 0, 0);
		for (vtri=cnx[i1].begin();vtri!=cnx[i1].end();++vtri)
			if (((*vtri)->N()*N)>cosa) sn += (*vtri)->N();
		sn.normalize();
		normals[idx] = sn;
		tri->na = &normals[idx];

		sn.set(0, 0, 0);
		for (vtri=cnx[i2].begin();vtri!=cnx[i2].end();++vtri)
			if (((*vtri)->N()*N)>cosa) sn += (*vtri)->N();
		sn.normalize();
		normals[idx+1] = sn;
		tri->nb = &normals[idx+1];

		sn.set(0, 0, 0);
		for (vtri=cnx[i3].begin();vtri!=cnx[i3].end();++vtri)
			if (((*vtri)->N()*N)>cosa) sn += (*vtri)->N();
		sn.normalize();
		normals[idx+2] = sn;
		tri->nc = &normals[idx+2];
		idx += 3;
	}
}

// tangents, derived from uv, if no uv, orco coords instead
// call after autosmooth() and setting orco flag
void meshObject_t::tangentsFromUV()
{
	bool hasuv = (!facesuv.empty());
	if (!(hasuv || hasorco)) return;
	tangents.resize(vertices.size());
	vector3d_t sdir, tdir;
	vector<triangle_t>::iterator tri;
	for(tri=triangles.begin();tri!=triangles.end();tri++)
	{
		tri->ta = &tangents[tri->a - (&vertices[0])];
		tri->tb = &tangents[tri->b - (&vertices[0])];
		tri->tc = &tangents[tri->c - (&vertices[0])];
	}
	PFLOAT s1, s2, t1, t2;
	unsigned int i0, i1, i2;
	for(tri=triangles.begin();tri!=triangles.end();tri++)
	{
		if (hasuv) {
			// from uv
			if (tri->hasuv) {
				s1 = tri->uv[2] - tri->uv[0];
				s2 = tri->uv[4] - tri->uv[0];
				t1 = tri->uv[3] - tri->uv[1];
				t2 = tri->uv[5] - tri->uv[1];
			}
			else {
				// no uv coords assigned to this tri
				s1 = s2 = t1 = t2 = 0;
			}
		}
		else {
			// from orco
			i0 = (tri->a - (&vertices[0]))+1;
			i1 = (tri->b - (&vertices[0]))+1;
			i2 = (tri->c - (&vertices[0]))+1;
			s1 = 0.5*(vertices[i1].x - vertices[i0].x);
			s2 = 0.5*(vertices[i2].x - vertices[i0].x);
			t1 = 0.5*(vertices[i1].y - vertices[i0].y);
			t2 = 0.5*(vertices[i2].y - vertices[i0].y);
		}
		PFLOAT r = s1*t2 - s2*t1;
		if (r==0.0)
			createCS(tri->N(), sdir, tdir);
		else {
			vector3d_t e1 = *(tri->b) - *(tri->a);
			vector3d_t e2 = *(tri->c) - *(tri->a);
			sdir = (t2*e1 - t1*e2)/r;
			tdir = (s1*e2 - s2*e1)/r;
			if (((sdir^tdir)*tri->N())<0.0) sdir *= -1.0;
		}
		*(tri->ta) += sdir;
		*(tri->tb) += sdir;
		*(tri->tc) += sdir;
	}
	// could adjust to make orthogonal, but not really needed for shading
	for (unsigned int i=0;i<tangents.size();i++)
		tangents[i].normalize();	
}

void meshObject_t::recalcBound()
{
	PFLOAT maxx,maxy,maxz,minx,miny,minz;
	maxx=minx=triangles.front().a->x;
	maxy=miny=triangles.front().a->y;
	maxz=minz=triangles.front().a->z;
	for(vector<triangle_t>::iterator i=triangles.begin();i!=triangles.end();++i)
	{
		point3d_t p=*(i->a);
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
		p=*(i->b);
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
		p=*(i->c);
		if(p.x>maxx) maxx=p.x;
		if(p.y>maxy) maxy=p.y;
		if(p.z>maxz) maxz=p.z;
		if(p.x<minx) minx=p.x;
		if(p.y<miny) miny=p.y;
		if(p.z<minz) minz=p.z;
	}
	minx-=BOX_MIN;
	miny-=BOX_MIN;
	minz-=BOX_MIN;
	maxx+=BOX_MIN;
	maxy+=BOX_MIN;
	maxz+=BOX_MIN;
	bound.set(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
	/*
	PFLOAT minx,miny,minz;
	PFLOAT maxx,maxy,maxz;

	maxx=minx=(*vertices)[0].x;
	maxy=miny=(*vertices)[0].y;
	maxz=minz=(*vertices)[0].z;
	for(vector<point3d_t>::iterator ite=vertices->begin();ite!=vertices->end();
			ite++)
	{
		if( (*ite).x>maxx ) maxx=(*ite).x;
		if( (*ite).y>maxy ) maxy=(*ite).y;
		if( (*ite).z>maxz ) maxz=(*ite).z;

		if( (*ite).x<minx ) minx=(*ite).x;
		if( (*ite).y<miny ) miny=(*ite).y;
		if( (*ite).z<minz ) minz=(*ite).z;
	}

	maxx+=BOX_MIN;
	maxy+=BOX_MIN;
	maxz+=BOX_MIN;
	minx-=BOX_MIN;
	miny-=BOX_MIN;
	minz-=BOX_MIN;
	bound.set(point3d_t(minx,miny,minz),point3d_t(maxx,maxy,maxz));
	*/
}

int rays=0;
int hitted=0;
int leafs=0;
int leafst=0;

meshObject_t::~meshObject_t()
{
	//cout<<"rays "<<rays<<" hitted "<<hitted<<endl;
	//cout<<"avgsize "<<((float)leafst/(float)leafs)<<endl;
	if (tree!=NULL) delete tree;
	if(n_tree) delete n_tree;
}

void meshObject_t::transform(const matrix4x4_t &m)
{
	matrix4x4_t mnotras=m;
	int step=(hasorco)? 2 :1;
	if(!unt)
	{
		for(vector<point3d_t>::iterator ite=vertices.begin();
				ite!=vertices.end();ite+=step)
			(*ite)=back*(*ite);
		for(vector<vector3d_t>::iterator ite=normals.begin();
				ite!=normals.end();ite++)
			(*ite)=back*(*ite);
	}

	back=m;
	back.inverse();
	// backRot is rotation only (only 3x3 used of 4x4),
	// so remove scaling by normalizing the matrix vectors.
	// and since inverse needed, use transpose of back directly.
	backRot.identity();
	vector3d_t mv(back[0][0], back[0][1], back[0][2]);
	mv.normalize();
	backRot.setRow(0, mv, 0);
	mv.set(back[1][0], back[1][1], back[1][2]);
	mv.normalize();
	backRot.setRow(1, mv, 0);
	mv.set(back[2][0], back[2][1], back[2][2]);
	mv.normalize();
	backRot.setRow(2, mv, 0);

	// orco matrix backOrco is m*scale derived from bound
	backOrco = m;
	recalcBound();	// need untransformed bound here
	point3d_t p1, p2;
	bound.get(p1, p2);
	mv = p2-p1;
	backOrco.scale(0.5*mv.x, 0.5*mv.y, 0.5*mv.z);

	for(vector<point3d_t>::iterator ite=vertices.begin();
			ite!=vertices.end();ite+=step)
		(*ite)=m*(*ite);
	for(vector<vector3d_t>::iterator ite=normals.begin();
			ite!=normals.end();ite++)
		(*ite)=m*(*ite);
	for(vector<triangle_t>::iterator ite=triangles.begin();
			ite!=triangles.end();ite++)
		(*ite).recNormal();
	unt=false;

//	vector<triangle_t *> *ltri=new vector<triangle_t *>(triangles.size());
//	for(vector<triangle_t>::iterator i=triangles.begin();
//			i!=triangles.end();++i)
//		(*ltri)[i-triangles.begin()]=&(*i);
//	if(tree!=NULL) delete tree;
	//tree=buildGenericTree(ltri,face_calc_bound,face_is_in_bound,face_get_pos,4);
//	unsigned int maxdepth = (unsigned int)(8.0 + 1.8755035531556525*log((PFLOAT)triangles.size()));
//	tree=buildTriangleTree(ltri, maxdepth, face_calc_bound(*ltri),4);
	recalcBound();
	
	
	// Lynx ->
	const triangle_t **tris=new const triangle_t*[triangles.size()];
	for(unsigned int i=0;i<triangles.size();++i)
		tris[i] = &(triangles[i]);
	if(n_tree != 0) delete n_tree;
	n_tree = new kdTree_t(tris, triangles.size(), -1, -1, 1.2, 0.40 );
	
	// backOrco, replace translation with (transformed!) bound center
	bound.get(p1, p2);
	mv = toVector(0.5*(p1+p2));
	backOrco.setColumn(3, mv, backOrco[3][3]);
	backOrco.inverse();

}


point3d_t meshObject_t::toObject(const point3d_t &p)const
{
	if(unt) return p;
	return (back*p);
}

vector3d_t meshObject_t::toObjectRot(const vector3d_t &v) const
{
	if (unt) return v;
	return (backRot*v);
}

point3d_t meshObject_t::toObjectOrco(const point3d_t &p) const
{
	if (unt) return p;
	return (backOrco*p);
}


bool meshObject_t::shoot(renderState_t &state,surfacePoint_t &where,
		const point3d_t &from, const vector3d_t &ray,bool shadow,PFLOAT dis) const
{
	rays++;
	triangle_t *hitt=NULL;
	//const object3d_t * lasto=state.lastobject;
	//igBoundTreeNode_t<triangle_t*> * lastb=
	//	(gBoundTreeNode_t<triangle_t*> *)state.lastobjectelement;
	//mray_t mray;
	//mray.from=from;
	//mray.ray=ray;
	PFLOAT minZ=-1;
	if(dis<0) dis=numeric_limits<PFLOAT>::infinity();

	//Lynx
	bool isec;
	PFLOAT Z=dis;
	if(shadow) return n_tree->IntersectS(from, ray, dis, &hitt);
	else isec = n_tree->Intersect(from, ray, dis, &hitt, Z);

	if(!isec) return false;
	/*
	pureBspIterator_t<vector<triangle_t *> > ite(tree,dis,bound,from,ray,shadow);
	for(;!ite;++ite)
	{
		for(vector<triangle_t *>::const_iterator i=(*ite)->begin();i!=(*ite)->end();++i)
		{
			hitted++;
			if((triangle_t *)state.skipelement==*i) continue;
			if(hitt==*i) continue;
			if( (*i)->hit(from,ray))
			{
				PFLOAT Z=(*i)->intersect(from,ray);
				if(shadow)
				{
					hitt=(*i);
					if((Z>0) && ((Z<dis) || (dis<0)))
						return true;
					//{state.lastobjectelement=(void *)ite.currentNode();return true;}
				}
				else
				if( ((minZ<0) && (Z>0)) || ( (Z>0) && (Z<minZ) ))
				{
					ite.limit(Z);
					minZ=Z;
					hitt=(*i);
				}
			}
		}
	}
	if(shadow) return false;
	*/
	if(hitt==NULL){cout << "d'oh!"; return false; }
	point3d_t h=from+/*min*/Z*ray;
	surfacePoint_t temp=hitt->getSurface(h,/*min*/Z,hasorco);
	temp.setObject((object3d_t *)this);
	temp.setOrigin(hitt);
	if(temp.getShader()==NULL) temp.setShader(shader);
	where=temp;
	return true;
}
/*
static bool crossLineZ(const point3d_t &a,const point3d_t &b,const point3d_t &c,
		PFLOAT cut,point3d_t &ra,point3d_t &rb)
{
	vector3d_t vba=b-a,vca=c-a,vbc=b-c;
	PFLOAT limBA=vba.normLen();
	PFLOAT limCA=vca.normLen();
	PFLOAT limBC=vbc.normLen();
	PFLOAT distBA=-1,distCA=-1,distBC=-1;
	if(vba.z!=0)
		distBA=(cut-a.z)/vba.z;
	if(vca.z!=0)
		distCA=(cut-a.z)/vca.z;
	if(vbc.z!=0)
		distBC=(cut-c.z)/vbc.z;
	int hit=0;
	if((distBA>=0) && (distBA<=limBA))
	{
		ra=vba*distBA+a;
		hit++;
	}
	if((distCA>=0) && (distCA<=limCA))
	{
		if(hit>0)
			rb=vca*distCA+a;
		else
			ra=vca*distCA+a;
		hit++;
	}
	if((distBC>=0) && (distBC<=limBC) && (hit<2))
	{
		if(hit>0)
			rb=vbc*distBC+c;
		else
			ra=vbc*distBC+c;
		hit++;
	}
	return (hit>0);
}

static bool crossQuadXY(const point3d_t &a, const point3d_t &b,
												const point3d_t &min,const point3d_t &max)
{
	vector3d_t line=b-a;
	PFLOAT length=line.normLen();
	if(length<MIN_RAYDIST)
		return (((b.x>=min.x) && (b.x<=max.x) && (b.y>=min.y) && (b.y<=max.y)) ||
				    ((a.x>=min.x) && (a.x<=max.x) && (a.y>=min.y) && (a.y<=max.y)));
	PFLOAT cmin=-1,cmax=-1;
	if(line.x!=0)
	{
		cmin=(min.x-a.x)/line.x;
		cmax=(max.x-a.x)/line.x;
		if(cmin>cmax) swap(cmin,cmax);
		if((cmax<0) || (cmin>length)) return false;
	}
	if(line.y!=0)
	{
		PFLOAT tmp1,tmp2;
		tmp1=(min.y-a.y)/line.y;
		tmp2=(max.y-a.y)/line.y;
		if(tmp1>tmp2) swap(tmp1,tmp2);
		if(tmp1>cmin) cmin=tmp1;
		if((tmp2<cmax) || (cmax<0)) cmax=tmp2;
	}
	return ((cmax>=0) && (cmin<=cmax) && (cmin<=length));
}

static bool checkTriangleInPlane(triangle_t *t,point3d_t min,point3d_t max,int axis,bool verb=false)
{
	point3d_t linea,lineb,a=*(t->a),b=*(t->b),c=*(t->c);
	switch(axis)
	{
		case XAXIS:
			swap(a.x,a.z);
			swap(b.x,b.z);
			swap(c.x,c.z);
			swap(min.x,min.z);
			swap(max.x,max.z);
			break;
		case YAXIS:
			swap(a.y,a.z);
			swap(b.y,b.z);
			swap(c.y,c.z);
			swap(min.y,min.z);
			swap(max.y,max.z);
			break;
	};
			
	PFLOAT cut=min.z;
	if(!crossLineZ(a,b,c,cut,linea,lineb)) return false;
	linea.z=0;
	lineb.z=0;
	if(verb) cout<<"pasa de crossline cut "<<linea<<" "<<lineb<<endl;
	return crossQuadXY(linea,lineb,min,max);
}
*/
//#define XAXIS 0
//#define YAXIS 1
//#define ZAXIS 2

/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/
//#include <math.h>
#include <stdio.h>


#define FINDMINMAX(x0,x1,x2,min,max) \
  min = max = x0;   \
  if(x1<min) min=x1;\
  if(x1>max) max=x1;\
  if(x2<min) min=x2;\
  if(x2>max) max=x2;

bool planeBoxOverlap(const vector3d_t &normal,PFLOAT d, const point3d_t &maxbox)
{
  vector3d_t vmin,vmax;
  if(normal.x>0.0) { vmin.x=-maxbox.x; vmax.x=maxbox.x; }
  else { vmin.x=maxbox.x; vmax.x=-maxbox.x; }
  if(normal.y>0.0) { vmin.y=-maxbox.y; vmax.y=maxbox.y; }
  else { vmin.y=maxbox.y; vmax.y=-maxbox.y; }
  if(normal.z>0.0) { vmin.z=-maxbox.z; vmax.z=maxbox.z; }
  else { vmin.z=maxbox.z; vmax.z=-maxbox.z; }

  if((normal*vmin+d)>0.0) return false;
  if((normal*vmax+d)>=0.0) return true;

  return false;
}


/*======================== X-tests ========================*/
#define AXISTEST_X01(a, b, fa, fb)         \
  p0 = a*v0.y - b*v0.z;                \
  p2 = a*v2.y - b*v2.z;                \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
  rad = fa * boxhalfsize.y + fb * boxhalfsize.z;   \
  if(min>rad || max<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)        \
  p0 = a*v0.y - b*v0.z;                \
  p1 = a*v1.y - b*v1.z;                \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
  rad = fa * boxhalfsize.y + fb * boxhalfsize.z;   \
  if(min>rad || max<-rad) return 0;

/*======================== Y-tests ========================*/
#define AXISTEST_Y02(a, b, fa, fb)         \
  p0 = -a*v0.x + b*v0.z;               \
  p2 = -a*v2.x + b*v2.z;                     \
        if(p0<p2) {min=p0; max=p2;} else {min=p2; max=p0;} \
  rad = fa * boxhalfsize.x + fb * boxhalfsize.z;   \
  if(min>rad || max<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)        \
  p0 = -a*v0.x + b*v0.z;               \
  p1 = -a*v1.x + b*v1.z;                   \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
  rad = fa * boxhalfsize.x + fb * boxhalfsize.z;   \
  if(min>rad || max<-rad) return 0;

/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)         \
  p1 = a*v1.x - b*v1.y;                \
  p2 = a*v2.x - b*v2.y;                \
        if(p2<p1) {min=p2; max=p1;} else {min=p1; max=p2;} \
  rad = fa * boxhalfsize.x + fb * boxhalfsize.y;   \
  if(min>rad || max<-rad) return 0;

#define AXISTEST_Z0(a, b, fa, fb)        \
  p0 = a*v0.x - b*v0.y;          \
  p1 = a*v1.x - b*v1.y;                \
        if(p0<p1) {min=p0; max=p1;} else {min=p1; max=p0;} \
  rad = fa * boxhalfsize.x + fb * boxhalfsize.y;   \
  if(min>rad || max<-rad) return 0;

bool triBoxOverlap(const bound_t &bound,const point3d_t &tria,
    const point3d_t &trib,const point3d_t &tric)
{

  point3d_t boxcenter(bound.centerX(),bound.centerY(),bound.centerZ());
  point3d_t tmp1,tmp2;
  bound.get(tmp1,tmp2);
  point3d_t boxhalfsize(0.51*(tmp2.x-tmp1.x)+0.00001,0.51*(tmp2.y-tmp1.y)+0.00001,0.51*(tmp2.z-tmp1.z)+0.00001);
  /*    use separating axis theorem to test overlap between triangle and box */
  /*    need to test for overlap in these directions: */
  /*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
  /*       we do not even need to test these) */
  /*    2) normal of the triangle */
  /*    3) crossproduct(edge from tri, {x,y,z}-directin) */
  /*       this gives 3x3=9 more tests */
   vector3d_t v0,v1,v2;
   vector3d_t axis;
   PFLOAT min,max,d,p0,p1,p2,rad,fex,fey,fez;
   vector3d_t normal,e0,e1,e2;

   /* This is the fastest branch on Sun */
   /* move everything so that the boxcenter is in (0,0,0) */
   v0=tria-boxcenter;
   v1=trib-boxcenter;
   v2=tric-boxcenter;

   /* compute triangle edges */
   e0=v1-v0;      /* tri edge 0 */
   e1=v2-v1;      /* tri edge 1 */
   e2=v0-v2;      /* tri edge 2 */

   /* Bullet 3:  */
   /*  test the 9 tests first (this was faster) */
   fex = fabs(e0.x);
   fey = fabs(e0.y);
   fez = fabs(e0.z);
   AXISTEST_X01(e0.z, e0.y, fez, fey);
   AXISTEST_Y02(e0.z, e0.x, fez, fex);
   AXISTEST_Z12(e0.y, e0.x, fey, fex);

   fex = fabs(e1.x);
   fey = fabs(e1.y);
   fez = fabs(e1.z);
   AXISTEST_X01(e1.z, e1.y, fez, fey);
   AXISTEST_Y02(e1.z, e1.x, fez, fex);
   AXISTEST_Z0(e1.y, e1.x, fey, fex);

   fex = fabs(e2.x);
   fey = fabs(e2.y);
   fez = fabs(e2.z);
   AXISTEST_X2(e2.z, e2.y, fez, fey);
   AXISTEST_Y1(e2.z, e2.x, fez, fex);
   AXISTEST_Z12(e2.y, e2.x, fey, fex);

   /* Bullet 1: */
   /*  first test overlap in the {x,y,z}-directions */
   /*  find min, max of the triangle each direction, and test for overlap in */
   /*  that direction -- this is equivalent to testing a minimal AABB around */
   /*  the triangle against the AABB */

   /* test in X-direction */
   FINDMINMAX(v0.x,v1.x,v2.x,min,max);
   if(min>boxhalfsize.x || max<-boxhalfsize.x) return false;

   /* test in Y-direction */
   FINDMINMAX(v0.y,v1.y,v2.y,min,max);
   if(min>boxhalfsize.y || max<-boxhalfsize.y) return false;

   /* test in Z-direction */
   FINDMINMAX(v0.z,v1.z,v2.z,min,max);
   if(min>boxhalfsize.z || max<-boxhalfsize.z) return false;

   /* Bullet 2: */
   /*  test if the box intersects the plane of the triangle */
   /*  compute plane equation of triangle: normal*x+d=0 */
   normal=e0^e1;
   d=-(normal*v0);  /* plane eq: normal.x+d=0 */
   if(!planeBoxOverlap(normal,d,boxhalfsize)) return false;

   return true;   /* box and triangle overlaps */
}

#undef FINDMINMAX
#undef AXISTEST_X01
#undef AXISTEST_X2
#undef AXISTEST_Y02
#undef AXISTEST_Y1
#undef AXISTEST_Z12
#undef AXISTEST_Z0

tnode_t * buildTriangleTree(std::vector<triangle_t*> *v, unsigned int maxdepth,
		const bound_t &bound,unsigned int dtol,unsigned int depth,unsigned int lostd,
		PFLOAT &avgdepth,PFLOAT &avgsize)
{
	typedef std::vector<triangle_t*>::const_iterator vector_const_iterator;
	
	if((v->size()<=dtol) || (lostd>3) || (depth>=maxdepth))
	{
		if(v->size())
		{
			leafs++;
			leafst+=v->size();
		}
		avgdepth=(PFLOAT)depth;
		avgsize=(PFLOAT)v->size();
		//cout<<"created "<<v->size()<<" at "<<depth<<endl;
		return new tnode_t(v);
	}
	bool usedX=false,usedY=false,usedZ=false;
	int axis;
	PFLOAT media;
	PFLOAT lx,ly,lz;
	lx=bound.longX();
	ly=bound.longY();
	lz=bound.longZ();

	bound_t bl,br;
	plane_t plane;
	if((lx>=ly) && (lx>=lz))
	{
		media=bound.centerX();
		bl=bound;bl.setMaxX(media);
		br=bound;br.setMinX(media);
		plane.set(bound,media,plane_t::XAXIS);
		axis=AXISX;
		usedX=true;
	}
	else if( (ly>=lz) )
	{
		media=bound.centerY();
		bl=bound;bl.setMaxY(media);
		br=bound;br.setMinY(media);
		plane.set(bound,media,plane_t::YAXIS);
		axis=AXISY;
		usedY=true;
	}
	else
	{
		media=bound.centerZ();
		bl=bound;bl.setMaxZ(media);
		br=bound;br.setMinZ(media);
		plane.set(bound,media,plane_t::ZAXIS);
		axis=AXISZ;
		usedZ=true;
	}
	std::vector<triangle_t *> *vl=new vector<triangle_t *>(),*vr=new vector<triangle_t *>();
	vl->reserve(v->size()/2);
	vr->reserve(v->size()/2);
	for(vector_const_iterator i=v->begin();i!=v->end();++i)
	{
		int pos=cheapPosition(**i,bound,media,axis);
		if(pos==trianglePosition_t::NONE) pos=expensivePosition(**i,bound,media,axis);
		/*
		if(pos==trianglePosition_t::NONE)
		{
			if(triBoxOverlap(bl,*((*i)->a),*((*i)->b),*((*i)->c))) pos=trianglePosition_t::LOWER;
			if(triBoxOverlap(br,*((*i)->a),*((*i)->b),*((*i)->c)))
			{
				if(pos!=trianglePosition_t::NONE)	pos=trianglePosition_t::INTERSECT;
				else pos=trianglePosition_t::HIGHER;
			}
		}
		*/
		if(pos==trianglePosition_t::LOWER) vl->push_back(*i);
		else if(pos==trianglePosition_t::HIGHER) vr->push_back(*i);
		else //INTERSECT
		{
			vl->push_back(*i);
			vr->push_back(*i);
		}
	}

	if(vl->empty() || vr->empty())
	{
		if(vl->empty())	media=minimize(*vr,bound,axis);
		else media=maximize(*vl,bound,axis);
		switch(axis)
		{
			case AXISX:
				bl.setMaxX(media);
				br.setMinX(media);
				plane.set(bound,media,plane_t::XAXIS);
				break;
			case AXISY:
				bl.setMaxY(media);
				br.setMinY(media);
				plane.set(bound,media,plane_t::YAXIS);
				break;
			case AXISZ:
				bl.setMaxZ(media);
				br.setMinZ(media);
				plane.set(bound,media,plane_t::ZAXIS);
				break;
		}
	}

	int lostl=((vl->size()==v->size())) ? (lostd+1) : (lostd-1);
	int lostr=((vr->size()==v->size())) ? (lostd+1) : (lostd-1);
	if(lostl<0) lostl=0;
	if(lostr<0) lostr=0;
	delete v;
	PFLOAT avgd,avgs;
	tnode_t *left=buildTriangleTree(vl, maxdepth, bl,dtol,depth+1,lostl,avgd,avgs);
	avgdepth=avgd;
	avgsize=avgs;
	tnode_t *right=buildTriangleTree(vr, maxdepth, br,dtol,depth+1,lostr,avgd,avgs);
	avgdepth+=avgd;
	avgsize+=avgs;
	avgdepth*=0.5;
	avgsize*=0.5;

	//if( (((PFLOAT)v.size()-avgsize)*4.0) > (avgdepth-(PFLOAT)depth))
		return new tnode_t(left,right,plane);
		/*
	else
	{
		// doesn't worth the effort
		delete left;
		delete right;
		avgdepth=(PFLOAT)depth;
		avgsize=(PFLOAT)v.size();
		//cout<<"pruned "<<v.size()<<endl;
		return new tnode_t(new vector<triangle_t*>(v));
	}*/
}
/*
tnode_t * buildTriangleTree(const std::vector<triangle_t*> &v,
		const bound_t &bound,unsigned int dtol,unsigned int depth,unsigned int lostdepth,
		PFLOAT &avgdepth,PFLOAT &avgsize,bool skipX,bool skipY,bool skipZ)
{
	typedef std::vector<triangle_t*>::const_iterator vector_const_iterator;
	
	if((v.size()<=dtol) || (depth>20) || (lostdepth>2))
	{
		avgdepth=(PFLOAT)depth;
		avgsize=(PFLOAT)v.size();
		return new tnode_t(new vector<triangle_t*>(v),bound,true);
	}
	bool usedX=false,usedY=false,usedZ=false;
	int axis;
	PFLOAT lx,ly,lz;
	lx=bound.longX();
	ly=bound.longY();
	lz=bound.longZ();

	bound_t bl,br;
	if(((lx>=ly) || skipY) && ((lx>=lz) || skipZ) && !skipX)
	{
		PFLOAT media=bound.centerX();
		bl=bound;bl.setMaxX(media);
		br=bound;br.setMinX(media);
		usedX=true;
		axis=XAXIS;
	}
	else if( ((ly>=lz) || skipZ) && !skipY)
	{
		PFLOAT media=bound.centerY();
		bl=bound;bl.setMaxY(media);
		br=bound;br.setMinY(media);
		usedY=true;
		axis=YAXIS;
	}
	else
	{
		PFLOAT media=bound.centerZ();
		bl=bound;bl.setMaxZ(media);
		br=bound;br.setMinZ(media);
		usedZ=true;
		axis=ZAXIS;
	}
	std::vector<triangle_t *> vl,vr;
	bound_t gbr=br,gbl=bl;
	gbl.grow(MIN_RAYDIST);
	gbr.grow(MIN_RAYDIST);
	for(vector_const_iterator i=v.begin();i!=v.end();++i)
	{
		if(triBoxOverlap(gbl,*((*i)->a),*((*i)->b),*((*i)->c)))
			vl.push_back(*i);
		if(triBoxOverlap(gbr,*((*i)->a),*((*i)->b),*((*i)->c)))
			vr.push_back(*i);
	}

	if(vl.empty())
		return buildTriangleTree(vr,br,dtol,depth,lostdepth,avgdepth,avgsize,skipX,skipY,skipZ);
	if(vr.empty())
		return buildTriangleTree(vl,bl,dtol,depth,lostdepth,avgdepth,avgsize,skipX,skipY,skipZ);

	bool alltried=(skipX||usedX) && (skipY||usedY) && (skipZ||usedZ);
	if((std::abs((int)(vl.size()-v.size()))<dtol) && 
		 (std::abs((int)(vl.size()-v.size()))<dtol) && !alltried)
		return buildTriangleTree(v,bound,dtol,depth,lostdepth,avgdepth,avgsize,skipX||usedX,skipY||usedY,skipZ||usedZ);
	else skipX=skipY=skipZ=false;
	
	int lostl,lostr;
	if(std::abs((int)(vl.size()-v.size()))<dtol) lostl=lostdepth+1;
	else lostl=0;
	if(std::abs((int)(vl.size()-v.size()))<dtol) lostr=lostdepth+1;
	else lostr=0;
	PFLOAT avgd,avgs;
	tnode_t *left=buildTriangleTree(vl,bl,dtol,depth+1,lostl,avgd,avgs,skipX,skipY,skipZ);
	avgdepth=avgd;
	avgsize=avgs;
	tnode_t *right=buildTriangleTree(vr,br,dtol,depth+1,lostr,avgd,avgs,skipX,skipY,skipZ);
	avgdepth+=avgd;
	avgsize+=avgs;
	avgdepth*=0.5;
	avgsize*=0.5;

	if( ((PFLOAT)v.size()-avgsize)< ((avgdepth-(PFLOAT)depth)*12.0))
	{
		// doesn't worth the effort
		delete left;
		delete right;
		avgdepth=(PFLOAT)depth;
		avgsize=(PFLOAT)v.size();
		return new tnode_t(new vector<triangle_t*>(v),bound,true);
	}
	else return new tnode_t(left,right,true);
}
*/

meshObject_t *meshObject_t::factory(const std::vector<point3d_t> &ver, const std::vector<vector3d_t> &nor,
		const std::vector<triangle_t> &ts, const std::vector<GFLOAT> &fuv, const std::vector<CFLOAT> &fvcol)
{
	return new meshObject_t(ver,nor,ts,fuv,fvcol);
}

		
meshObject_t *meshObject_t::factory(bool _hasorco, const matrix4x4_t &M, const std::vector<point3d_t> &ver,
		const std::vector<vector3d_t> &nor, const std::vector<triangle_t> &ts,
		const std::vector<GFLOAT> &fuv, const std::vector<CFLOAT> &fvcol)
{
	return new meshObject_t(_hasorco, M, ver,nor,ts,fuv,fvcol);
}

__END_YAFRAY
