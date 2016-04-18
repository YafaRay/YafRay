#ifndef __SUNSKY_H
#define __SUNSKY_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif
#include "background.h"
#include "params.h"

__BEGIN_YAFRAY
// constant
class constBackground_t: public background_t
{
	public:
		constBackground_t(const color_t &c) {color=c;};
		virtual color_t operator() (const vector3d_t &dir,renderState_t &state, bool filtered=false) const { return color; }
		virtual ~constBackground_t() {};
		static background_t *factory(paramMap_t &,renderEnvironment_t &);
	protected:
		color_t color;
};

// sunsky
class sunskyBackground_t: public background_t
{
	public:
		sunskyBackground_t(const point3d_t dir, PFLOAT turb,
			PFLOAT a_var, PFLOAT b_var, PFLOAT c_var, PFLOAT d_var, PFLOAT e_var);
		virtual color_t operator() (const vector3d_t &dir, renderState_t &state, bool filtered=false) const;
		virtual ~sunskyBackground_t() {};
		static background_t *factory(paramMap_t &,renderEnvironment_t &);
	protected:
		vector3d_t sunDir;
		PFLOAT turbidity;
		double thetaS, phiS;	// sun coords
		double theta2, theta3, T, T2;
		double zenith_Y, zenith_x, zenith_y;
		double perez_Y[5], perez_x[5], perez_y[5];
		double AngleBetween(double thetav, double phiv) const;
		double PerezFunction(const double *lam, double theta, double gamma, double lvz) const;
};

__END_YAFRAY
#endif
