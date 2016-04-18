#ifndef __BRDF_H
#define __BRDF_H

#include "vector3d.h"
#include "color.h"
#include <string>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

__BEGIN_YAFRAY

// Base
// for blender 'modexp' added, modulated exponent
// only used for Phong/BlenderCookTorr/BlenderBlinn speculars
class brdf_t
{
public:
	brdf_t() {}
	virtual ~brdf_t() {}
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const=0;
};

// Lambert diffuse
class Lambert_t : public brdf_t
{
public:
	Lambert_t():Kd(0.8) {}
	Lambert_t(CFLOAT kd): Kd(kd) {}
	virtual ~Lambert_t() {}
	void set(CFLOAT kd) { Kd=kd; }
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Kd;
};

// Phong specular, several types
class Phong_t : public brdf_t
{
public:
	enum PTYPE {ORIGINAL, PHYSICAL} phong_type;
	enum RTYPE {REFLECT, HALFWAY} reflect_type;
	Phong_t():phong_type(ORIGINAL), reflect_type(REFLECT), Ks(0.2), expo(40)  {}
	Phong_t(CFLOAT ks, CFLOAT e, PTYPE pt=ORIGINAL, RTYPE rt=REFLECT) { set(ks, e, pt, rt); }
	virtual ~Phong_t() {}
	void set(CFLOAT ks, CFLOAT e, PTYPE pt=ORIGINAL, RTYPE rt=REFLECT)
	{
		Ks = ks;
		expo = e;
		phong_type = pt;
		reflect_type = rt;
	}
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Ks, expo;
};

// ward anisotropic specular
class Ward_t : public brdf_t
{
public:
	Ward_t() { set(0.2, 0.35, 0.2); }
	Ward_t(CFLOAT ks, CFLOAT u, CFLOAT v) { set(ks, u, v); }
	virtual ~Ward_t() {}
	void set(CFLOAT ks, CFLOAT u, CFLOAT v)
	{
		Ks = ks;
		// u & v roughness (& recip.)
		ui = (u!=0)?(1.f/u):0;
		vi = (v!=0)?(1.f/v):0;
		uv4 = 4*u*v;
		if (uv4!=0) uv4 = 1.f/uv4;
	}
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Ks;
	CFLOAT uv4, ui, vi;
};

// Oren-Nayar diffuse
class OrenNayar_t : public brdf_t
{
public:
	OrenNayar_t() { set(1, 0); }
	OrenNayar_t(CFLOAT kd, CFLOAT sigma) { set(kd, sigma); }
	virtual ~OrenNayar_t() {}
	void set(CFLOAT kd, CFLOAT sigma)
	{
		Kd = kd;
		sigma2 = sigma*sigma;
		A = 1 - 0.5 * sigma2 / (sigma2 + 0.33);
		B = 0.45 * sigma2 / (sigma2 + 0.09);
		cf3 = 0.125 * sigma2 / (sigma2 + 0.09);
		cf4 = 0.17 * sigma2 / (sigma2 + 0.13);
	}
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Kd, sigma2;
	CFLOAT A, B, cf3, cf4;
};

// minnaert diffuse
class Minnaert_t : public brdf_t
{
public:
	Minnaert_t():Kd(1), K(0.5) {}
	Minnaert_t(CFLOAT kd, CFLOAT k): Kd(kd), K(k) {}
	virtual ~Minnaert_t() {}
	void set(CFLOAT kd, CFLOAT k) { Kd=kd;  K=k; }
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Kd, K;
};

// Ashikhmin diffuse
class AshikhminDiffuse_t : public brdf_t
{
public:
	AshikhminDiffuse_t():Kd(0.8), Ks(0.2) {}
	AshikhminDiffuse_t(float kd, float ks) { set(kd, ks); }
	virtual ~AshikhminDiffuse_t() {}
	void set(float kd, float ks)
	{
		Kd = kd;
		Ks = (ks>1)?1:ks;
	}
	virtual float evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	float Kd, Ks;
};

// Ashikhmin (anisotropic) specular
class AshikhminSpecular_t : public brdf_t
{
public:
	AshikhminSpecular_t():Ks(0.2), Nu(0.01), Nv(0.01) {}
	AshikhminSpecular_t(float ks, float _nu, float _nv) { set(ks, _nu, _nv); }
	virtual ~AshikhminSpecular_t() {}
	void set(float ks, float _nu, float _nv)
	{
		Ks = (ks>1)?1:ks;
		Nu = (_nu==0.f)?0.f:(1.f/_nu);
		Nv = (_nv==0.f)?0.f:(1.f/_nv);
	}
	virtual float evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	float Ks, Nu, Nv;
};

class BlenderBlinn_t : public brdf_t
{
public:
	BlenderBlinn_t():Ks(0.2), expo(20), eta(4) {}
	BlenderBlinn_t(CFLOAT ks, CFLOAT e, CFLOAT refr_idx) { set(ks, e, refr_idx); }
	virtual ~BlenderBlinn_t() {}
	void set(CFLOAT ks, CFLOAT e, CFLOAT refr_idx) { Ks=ks;  expo=e;  eta=refr_idx; }
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Ks, expo, eta;
};

class BlenderCookTorr_t : public brdf_t
{
public:
	BlenderCookTorr_t():Ks(0.2), expo(20) {}
	BlenderCookTorr_t(CFLOAT ks, CFLOAT e) { set(ks, e); }
	virtual ~BlenderCookTorr_t() {}
	void set(CFLOAT ks, CFLOAT e) { Ks=ks;  expo=e; }
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Ks, expo;
};


// blender simple toon, diffuse
class simpleToonDiffuse_t : public brdf_t
{
public:
	simpleToonDiffuse_t():Kd(0.8), dsize(1.5), dsmooth(1.5), edge(0.3) {}
	simpleToonDiffuse_t(CFLOAT kd, CFLOAT dsz, CFLOAT dsm, CFLOAT edg) { set(kd, dsz, dsm, edg); }
	virtual ~simpleToonDiffuse_t() {}
	void set(CFLOAT kd, CFLOAT dsz, CFLOAT dsm, CFLOAT edg)
	{
		Kd = kd;
		dsize = dsz;
		dsmooth = dsm;
		edge = edg;
	}
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Kd, dsize, dsmooth, edge;
};

// blender simple toon, specular
class simpleToonSpecular_t : public brdf_t
{
public:
	simpleToonSpecular_t():Ks(0.8), dsize(1.5), dsmooth(1.5) {}
	simpleToonSpecular_t(CFLOAT ks, CFLOAT dsz, CFLOAT dsm) { set(ks, dsz, dsm); }
	virtual ~simpleToonSpecular_t() {}
	void set(CFLOAT ks, CFLOAT dsz, CFLOAT dsm)
	{
		Ks = ks;
		dsize = dsz;
		dsmooth = dsm;
	}
	virtual CFLOAT evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp=1) const;
protected:
	CFLOAT Ks, dsize, dsmooth;
};

__END_YAFRAY

#endif //__BRDF_H
