/****************************************************************************
 *
 * 			camera.cc: Camera implementation
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

#include "camera.h"

__BEGIN_YAFRAY

camera_t::camera_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
		int _resx, int _resy, PFLOAT aspect,
		PFLOAT df, PFLOAT ap, PFLOAT dofd, bool useq,
		cameraType ct, bokehType bt, bkhBiasType bbt, PFLOAT bro)
		:camtype(ct), bkhtype(bt), bkhbias(bbt)
{
	_eye = _position = pos;
	aperture = ap;
	dof_distance = dofd;
	resx = _resx;
	resy = _resy;
	vup = up - pos;
	vto = look - pos;
	vright = vup ^ vto;
	vup = vright ^ vto;
	vup.normalize();
	vright.normalize();
	
	// matrix vectors to orient cam for sphere/probe
	camu = vright;
	camv = vup;
	camw = vto;
	camw.normalize();
	
	vright *= -1.0; // vright is negative here
	fdist = vto.normLen();

	dof_rt = vright * aperture; // for dof, premul with aperture
	dof_up = vup * aperture;

	vup *= aspect * (PFLOAT)resy / (PFLOAT)resx;

	// for orthocam
	dir_O = vto;
	PFLOAT idf = fdist / df;
	eye_O = _eye - 0.5 * idf * (vup + vright);

	vto = (vto * df) - 0.5 * (vup + vright);
	vup /= (PFLOAT)resy;
	vright /= (PFLOAT)resx;	

	vright_O = vright * idf;
	vup_O = vup * idf;

	focal_distance = df;
	HSEQ1.setBase(2);
	HSEQ2.setBase(3);
	use_qmc = useq;
	
	int ns = (int)bkhtype;
	if ((ns>=3) && (ns<=6)) {
		PFLOAT w=bro*M_PI/180.0, wi=(2.0*M_PI)/(PFLOAT)ns;
		ns = (ns+2)*2;
		LS.resize(ns);
		for (int i=0;i<ns;i+=2) {
			LS[i] = cos(w);
			LS[i+1] = sin(w);
			w += wi;
		}
	}
}

camera_t::~camera_t() 
{
}


void camera_t::biasDist(PFLOAT &r) const
{
	switch (bkhbias) {
		case BB_CENTER:
			r = sqrt(sqrt(r)*r);
			break;
		case BB_EDGE:
			r = sqrt((PFLOAT)1.0-r*r);
			break;
		default:
		case BB_NONE:
			r = sqrt(r);
	}
}

void camera_t::sampleTSD(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const
{
	PFLOAT fn = (PFLOAT)bkhtype;
	int idx = int(r1*fn);
	r1 = (r1-((PFLOAT)idx)/fn)*fn;
	biasDist(r1);
	PFLOAT b1 = r1 * r2;
	PFLOAT b0 = r1 - b1;
	idx <<= 1;
	u = LS[idx]*b0 + LS[idx+2]*b1;
	v = LS[idx+1]*b0 + LS[idx+3]*b1;
}

void camera_t::getLensUV(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const
{
	switch (bkhtype) {
		case BK_TRI:
		case BK_SQR:
		case BK_PENTA:
		case BK_HEXA:
			sampleTSD(r1, r2, u, v);
			break;
		case BK_DISK2:
		case BK_RING: {
			PFLOAT w = (PFLOAT)2.0*M_PI*r2;
			if (bkhtype==BK_RING) r1 = sqrt((PFLOAT)0.707106781 + (PFLOAT)0.292893218);
			else biasDist(r1);
			u = r1*cos(w);
			v = r1*sin(w);
			break;
		}
		default:
		case BK_DISK1:
			ShirleyDisk(r1, r2, u, v);
	}
}

vector3d_t camera_t::shootRay(PFLOAT px, PFLOAT py, PFLOAT &wt)
{
	vector3d_t ray;
	wt = 1;	// for now always 1, except 0 for probe when outside sphere
	switch (camtype) {
		case CM_ORTHO: {
			_position = vright_O*px + vup_O*py + eye_O;
			ray = dir_O;
			break;
		}
		case CM_SPHERICAL: {
			_position = _eye;
			PFLOAT theta = M_PI_2 - M_PI * (1.0 - 2.0 * (px/(PFLOAT)(resx-1)));
			PFLOAT phi = M_PI - M_PI * (py/(PFLOAT)(resy-1));
			PFLOAT sp = sin(phi);
			ray.set(sp*cos(theta), cos(phi), sp*sin(theta));
			ray.set(ray.x*camu.x + ray.y*camv.x + ray.z*camw.x,
							ray.x*camu.y + ray.y*camv.y + ray.z*camw.y,
							ray.x*camu.z + ray.y*camv.z + ray.z*camw.z);
			break;
		}
		case CM_LIGHTPROBE: {
			_position = _eye;
			PFLOAT u = 1.0 - 2.0 * (px/(PFLOAT)(resx-1));
			PFLOAT v = 2.0 * (py/(PFLOAT)(resy-1)) - 1.0;
			PFLOAT insphere = sqrt(u*u + v*v);
			if (insphere>1) { wt=0; return ray; }
			PFLOAT theta=0;
			if (!((u==0) && (v==0))) theta = atan2(v,u);
			PFLOAT phi = insphere * M_PI;
			PFLOAT sp = sin(phi);
			ray.set(sp*cos(theta), sp*sin(theta), cos(phi));
			ray.set(ray.x*camu.x + ray.y*camv.x + ray.z*camw.x,
							ray.x*camu.y + ray.y*camv.y + ray.z*camw.y,
							ray.x*camu.z + ray.y*camv.z + ray.z*camw.z);
			break;
		}
		default:
		case CM_PERSPECTIVE: {
			_position = _eye;
			ray = vright*px + vup*py + vto;
			ray.normalize();
			break;
		}
	}

	if (aperture!=0) {
		PFLOAT r1, r2, u, v;
		if (use_qmc) {
			r1 = HSEQ1.getNext();
			r2 = HSEQ2.getNext();
		}
		else {
			r1 = ourRandom();
			r2 = ourRandom();
		}
		getLensUV(r1, r2, u, v);
		vector3d_t LI = dof_rt * u + dof_up * v;
		_position += point3d_t(LI);
		ray = (ray * dof_distance) - LI;
		ray.normalize();
	}

	return ray;
}
__END_YAFRAY
