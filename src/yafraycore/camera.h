/****************************************************************************
 *
 * 			camera.h: Camera implementation api
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

#ifndef __CAMERA_H
#define __CAMERA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "vector3d.h"
#include "matrix4.h"
#include "mcqmc.h"
#include <vector>

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT camera_t
{
	public:
		enum cameraType {CM_PERSPECTIVE, CM_ORTHO, CM_SPHERICAL, CM_LIGHTPROBE};
		enum bokehType {BK_DISK1, BK_DISK2, BK_TRI=3, BK_SQR, BK_PENTA, BK_HEXA, BK_RING};
		enum bkhBiasType {BB_NONE, BB_CENTER, BB_EDGE};
		camera_t(const point3d_t &pos, const point3d_t &look, const point3d_t &up,
			int _resx, int _resy, PFLOAT aspect=1,
			PFLOAT df=1, PFLOAT ap=0, PFLOAT dofd=0, bool useq=false,
			cameraType ct=CM_PERSPECTIVE, bokehType bt=BK_DISK1, bkhBiasType bbt=BB_NONE,
			PFLOAT bro=0);
		~camera_t();
		int resX() const { return resx; }
		int resY() const { return resy; }
		const point3d_t & position() const { return _position; }
		vector3d_t shootRay(PFLOAT px, PFLOAT py, PFLOAT &wt);
		PFLOAT getFocal() const { return focal_distance; }
	protected:
		void biasDist(PFLOAT &r) const;
		void sampleTSD(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const;
		void getLensUV(PFLOAT r1, PFLOAT r2, PFLOAT &u, PFLOAT &v) const;
		point3d_t _eye, _position, eye_O;
		PFLOAT focal_distance, dof_distance;
		vector3d_t vto, vup, vright, dof_up, dof_rt;
		vector3d_t vright_O, vup_O, dir_O;
		vector3d_t camu, camv, camw;
		int resx, resy;
		PFLOAT fdist, aperture;
		bool use_qmc;
		Halton HSEQ1, HSEQ2;
		cameraType camtype;
		bokehType bkhtype;
		bkhBiasType bkhbias;
		std::vector<PFLOAT> LS;
};

__END_YAFRAY

#endif
