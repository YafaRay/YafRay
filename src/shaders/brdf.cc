// mostly copy & paste of my own code, sampling removed (not for all models anyway), better save for redesign
// unoptimized, Kd/Ks/.. almost never used. Unnecessary PI div... microfacet not used...
// blah

#include "brdf.h"
// #include <math.h>

using namespace std;

__BEGIN_YAFRAY

CFLOAT ACOS(CFLOAT x)
{
	if (x<-1.f) return M_PI;
	if (x>1.f) return 0.f;
	return acos(x);
}

CFLOAT SQRT(CFLOAT x)
{
	if (x<=0.f) return 0.f;
	return sqrt(x);
}

CFLOAT SQR(CFLOAT x) { return x*x; }
CFLOAT CUBE(CFLOAT x) { return x*x*x; }

CFLOAT Lambert_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	return Kd*M_1_PI;
}

CFLOAT Phong_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	// HALFWAY|ORIGNAL aka Blinn-Phong, used in Blender as Phong
	CFLOAT cos_ti = N*L;
	if (cos_ti==0.f) return 0.f;
	CFLOAT i;
	switch (reflect_type) {
		case HALFWAY: {
			vector3d_t H(L+V);
			H.normalize();
			i = H*N;
			break;
		}
		default:
		case REFLECT:
			i = V*(N*2.f*cos_ti - L);
			break;
	}
	if (i<=0.f) return 0.f;
	// since exponent can be modulate, 'expo' var not used here, but 'modexp' instead
	switch (phong_type) {
		case PHYSICAL:
			return Ks * ((modexp+2.f)*M_1_PI*0.5f) * pow(i, modexp);
		default:
		case ORIGINAL:
			return Ks * M_1_PI * pow(i, modexp) / cos_ti;
	}
}

// ward elliptical (anisotropic)
CFLOAT Ward_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = L*N;
	if (cos_ti<=0.f) return 0.f;
	CFLOAT cos_to = N*V;
	if (cos_to<0.f) cos_to=0.f;
	vector3d_t H(L+V);
	H.normalize();
	CFLOAT t1 = cos_ti*cos_to;
	if (t1!=0.f) t1 = 1.f/sqrt(t1);
	CFLOAT hdx=(H*NU)*ui, hdy=(H*NV)*vi;
	CFLOAT t2 = 1.f + (H*N);
	if (t2!=0.f) t2 = exp(-2.f*(hdx*hdx + hdy*hdy)/t2);
	return Ks * t1 * t2 * uv4 * M_1_PI;
}

// Full Oren-Nayar model, looks a bit better than the simplified one used in Blender (no dark bands)
CFLOAT OrenNayar_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = N*L;
	if (cos_ti<=0.f) return 0.f;
	CFLOAT cos_to = N*V;
	if (cos_to<0.f) cos_to=0.f;
	CFLOAT alpha=ACOS(cos_ti), beta=ACOS(cos_to);
	if (alpha<beta) swap(alpha, beta);
	vector3d_t v1(L-cos_ti*N), v2(V-cos_to*N);
	v1.normalize();
	v2.normalize();
	CFLOAT c = v1*v2;
	CFLOAT C2 = (c>0.f) ? B*sin(alpha) : B*(sin(alpha) - CUBE(2.f*beta*M_1_PI));
	CFLOAT C3 = cf3*SQR(4.f*alpha*beta*M_1_PI*M_1_PI);
	CFLOAT L1 = A + c*C2*tan(beta) + (1.f-fabs(c))*C3*tan((alpha + beta)*0.5f);
	CFLOAT L2 = cf4*(1.f - c*SQR(2.f*beta*M_1_PI));
	return Kd * M_1_PI * (L1 + Kd*L2);
}

CFLOAT Minnaert_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = L*N;
	if (cos_ti<=0.f) return 0.f;
	CFLOAT cos_to = V*N;
	if (cos_to<=0.f) cos_to=0.f;

	// Do the same as Blender, K<=1 real, >1 blender/nvidia..
	// real model, including energy conserving factor,
	// so not entirely the same as Blender.
	if (K<=1.f) return Kd * (K+1.f)*0.5f*M_1_PI * pow(max(cos_ti*cos_to, 0.1f), K-1.f);

	// from 'surface reflection models.pdf' (from nvidia),
	// but looks different
	//return Kd * M_1_PI * pow(cos_ti, K) * pow(1.f-cos_to, 1.f-K);

	// blender adaptation, not nvidia either, more like Blender/Nvidia model...
	// appears to be designed to start from lambert, so where real Minnaert ends.
	return Kd * M_1_PI * pow(1.f-cos_to, K-1.f);
}

// Blinn as implemented in Blender
// fresnel has missing factor 1/2, also slightly different from microfacet implementation below
CFLOAT BlenderBlinn_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = L*N;
	if (cos_ti<=0.f) return 0.f;
	vector3d_t H(L+V);
	H.normalize();
	CFLOAT nh = N*H;
	if (nh<=0.f) return 0.f;
	CFLOAT cos_to = V*N;
	CFLOAT vh = V*H;
	CFLOAT ivh = (vh!=0.f)?(1.f/vh):0.0;
	CFLOAT G = min(1.f, min(2.f*nh*cos_to*ivh, 2.f*nh*cos_ti*ivh));
	CFLOAT g = SQRT(eta*eta + vh*vh - 1.f);
	CFLOAT F = (SQR(g-vh)/SQR(g+vh)) * (1.f+SQR(vh*(g+vh)-1.f)/SQR(vh*(g-vh)+1.f));
	// since exponent can be modulate, 'expo' var not used here, but 'modexp' instead
	CFLOAT sp = (modexp<100.f)?(sqrt(1.f/modexp)):(10.f/modexp);
	return Ks * M_1_PI * F * G * exp(-SQR(ACOS(nh)) / (2.f*sp*sp)) / cos_ti;
}

// Ashikhmin diffuse
CFLOAT AshikhminDiffuse_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = N*L;
	if (cos_ti<=0.f) return 0.f;
	CFLOAT cos_to = V*N;
	if (cos_to<=0.f) return 0.f;
	CFLOAT ti=1.f-cos_ti*0.5f, to=1.f-cos_to*0.5f;
	CFLOAT ti2=ti*ti, to2=to*to;
	return (28.f/(23.f*M_PI)) * (1.f-Ks) * (1.f-ti*ti2*ti2) * (1.f-to*to2*to2);
}

// Ashikhmin specular
CFLOAT AshikhminSpecular_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	vector3d_t H(L+V);
	H.normalize();
	CFLOAT hn = N*H;
	if (hn<=0.f) return 0.f;
	CFLOAT sqd = sqrt((Nu+1.f)*(Nv+1.f))*0.125f*M_1_PI;
	CFLOAT hu=H*NU, hv=H*NV, hk=H*L;
	CFLOAT an=1.f, ihn=1.f-hn*hn;
	if (ihn>0.f) an = pow(hn, (Nu*hu*hu + Nv*hv*hv)/ihn);
	CFLOAT t = hk * max((N*L), (N*V));
	if (t!=0.f) an /= t;
	t = 1.f-hk;
	CFLOAT t2 = t*t;
	return (Ks + (1.f-Ks)*t2*t2*t) * sqd * an;
}

// blender original specular, isn't really anything like real Cook-Torrance
CFLOAT BlenderCookTorr_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = L*N;
	if (cos_ti==0.f) cos_ti=0.f;
	vector3d_t H(L+V);
	H.normalize();
	CFLOAT nh = N*H;
	if (nh<=0.f) return 0.f;
	CFLOAT nv = N*V;
	if (nv<0.f) nv=0.f;
	// since exponent can be modulate, 'expo' var not used here, but 'modexp' instead
	return Ks * M_1_PI * (pow(nh, modexp) / (0.1f+nv)) / cos_ti;  // blender spec() is int.pow
}

// Blender Toon, with extra param for simple edge
CFLOAT simpleToonDiffuse_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = L*N;
	if (cos_ti<=0.f) return 0.f;
	CFLOAT diff, ang = ACOS(cos_ti);
	if (ang<dsize) diff=1.0;
	else if ((ang>=(dsize + dsmooth)) || (dsmooth==0.f)) diff=0.f;
	else diff = 1.f - ((ang-dsize)/dsmooth);
	// extra edge factor
	CFLOAT e = V*N;
	if (e<edge) e=0.f; else e=1.f;
	return Kd * M_1_PI * (diff*e)/cos_ti;
}

CFLOAT simpleToonSpecular_t::evaluate(const vector3d_t &V, const vector3d_t &L, const vector3d_t &N,
				const vector3d_t &NU, const vector3d_t &NV, CFLOAT modexp) const
{
	CFLOAT cos_ti = L*N;
	if (cos_ti==0.f) return 0.f;
	vector3d_t H(L+V);
	H.normalize();
	CFLOAT hn = N*H;
	if (hn<=0.f) return 0.f;
	CFLOAT ang = ACOS(hn);
	if (ang<dsize) hn=1.0;
	else if ((ang>=(dsize+dsmooth)) || (dsmooth==0.f)) hn=0.f;
	else hn = 1.f - ((ang-dsize)/dsmooth);
	return Ks * M_1_PI * hn / cos_ti;
}

__END_YAFRAY
