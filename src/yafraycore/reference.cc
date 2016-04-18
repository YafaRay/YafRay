
#include "reference.h"

#include<vector>

using namespace std;

__BEGIN_YAFRAY

referenceObject_t::referenceObject_t(const matrix4x4_t &m,object3d_t *org)
{
	original = org;
	M = m;
	back = M;
	back.inverse();

	// rotation only matrix for vectors,
	// was missing, caused shading errors (norvec scaling)
	// from M
	MRot.identity();
	vector3d_t mv(M[0][0], M[0][1], M[0][2]);
	mv.normalize();
	MRot.setRow(0, mv, 0);
	mv.set(M[1][0], M[1][1], M[1][2]);
	mv.normalize();
	MRot.setRow(1, mv, 0);
	mv.set(M[2][0], M[2][1], M[2][2]);
	mv.normalize();
	MRot.setRow(2, mv, 0);

	// from back (sofar worked fine without, but just in case)
	backRot.identity();
	mv.set(back[0][0], back[0][1], back[0][2]);
	mv.normalize();
	backRot.setRow(0, mv, 0);
	mv.set(back[1][0], back[1][1], back[1][2]);
	mv.normalize();
	backRot.setRow(1, mv, 0);
	mv.set(back[2][0], back[2][1], back[2][2]);
	mv.normalize();
	backRot.setRow(2, mv, 0);

	shader = original->getShader();
	radiosity = original->useForRadiosity();
	rad_pasive = original->reciveRadiosity();
	shadow = original->castShadows();
	caus = original->caustics();
	original->getCaustic(caus_rcolor,caus_tcolor,caus_IOR);
}

referenceObject_t::~referenceObject_t() 
{
}

void referenceObject_t::transform(const matrix4x4_t &m)
{
	M = m;
	back = m;
	back.inverse();

	// rotation only matrix for vectors, was missing, caused shading errors
	// from M
	MRot.identity();
	vector3d_t mv(M[0][0], M[0][1], M[0][2]);
	mv.normalize();
	MRot.setRow(0, mv, 0);
	mv.set(M[1][0], M[1][1], M[1][2]);
	mv.normalize();
	MRot.setRow(1, mv, 0);
	mv.set(M[2][0], M[2][1], M[2][2]);
	mv.normalize();
	MRot.setRow(2, mv, 0);

	// from back (though was not really needed, but just in case)
	backRot.identity();
	mv.set(back[0][0], back[0][1], back[0][2]);
	mv.normalize();
	backRot.setRow(0, mv, 0);
	mv.set(back[1][0], back[1][1], back[1][2]);
	mv.normalize();
	backRot.setRow(1, mv, 0);
	mv.set(back[2][0], back[2][1], back[2][2]);
	mv.normalize();
	backRot.setRow(2, mv, 0);

}


point3d_t referenceObject_t::toObject(const point3d_t &p)const
{
	point3d_t res = back*p;
	return original->toObject(res);
}

vector3d_t referenceObject_t::toObjectRot(const vector3d_t &v) const
{
	vector3d_t res = backRot*v;
	return original->toObjectRot(res);
}


point3d_t referenceObject_t::toObjectOrco(const point3d_t &p) const
{
	point3d_t tp = back*p;
	return original->toObjectOrco(tp);
}


bool referenceObject_t::shoot(renderState_t &state,
		surfacePoint_t &where, const point3d_t &from,
		const vector3d_t &ray,bool shadow,PFLOAT dis)const
{
	// vectors xform rot. only mtx (see above)! (except ray)
	point3d_t myfrom = back*from;
	vector3d_t myray = back*ray;
	if(original->shoot(state,where,myfrom,myray,shadow,dis))
	{
		where.N() = MRot*where.N();
		where.Nd() = MRot*where.Nd();
		where.Ng() = MRot*where.Ng();
		where.P() = M*where.P();
		where.NU() = MRot*where.NU();
		where.NV() = MRot*where.NV();
		where.TU() = MRot*where.TU();
		where.TV() = MRot*where.TV();
		where.setObject((object3d_t*)this);
		return true;
	}
	else return false;
}

bound_t referenceObject_t::getBound() const
{
	point3d_t a, g;
	original->getBound().get(a, g);
	vector<point3d_t> cube(8);
	PFLOAT tox=g.x-a.x, toy=g.y-a.y, toz=g.z-a.z;
	cube[0] = a;
	cube[1] = a;  cube[1].x += tox;
	cube[2] = a;  cube[2].y += toy;
	cube[3] = a;  cube[3].y += toy;  cube[3].x += tox;
	for(int i=0;i<4;++i)
	{
		cube[4+i] = cube[i];
		cube[4+i].z += toz;
	}
	PFLOAT minx, miny, minz, maxx, maxy, maxz;
	cube[0] = M*cube[0];
	minx = maxx = cube[0].x;
	miny = maxy = cube[0].y;
	minz = maxz = cube[0].z;
	for(int i=1;i<8;++i)
	{
		cube[i] = M*cube[i];
		if (cube[i].x<minx) minx = cube[i].x;
		if (cube[i].y<miny) miny = cube[i].y;
		if (cube[i].z<minz) minz = cube[i].z;
		if (cube[i].x>maxx) maxx = cube[i].x;
		if (cube[i].y>maxy) maxy = cube[i].y;
		if (cube[i].z>maxz) maxz = cube[i].z;
	}
	bound_t bound(point3d_t(minx, miny, minz), point3d_t(maxx, maxy, maxz));
	return bound;
}

referenceObject_t * referenceObject_t::factory(const matrix4x4_t &M,object3d_t *org)
{
	return new referenceObject_t(M, org);
}

__END_YAFRAY
