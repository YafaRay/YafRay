/****************************************************************************
 *
 * 			texture.h: Texture and modulation api
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
#ifndef __TEXTURE_H
#define __TEXTURE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "surface.h"
#include "vector3d.h"
#include "color.h"
#include "buffer.h"
//#include "noise.h"
#include "matrix4.h"
#include <string>

__BEGIN_YAFRAY

// instead of TEX_TYPE flag, now uses tex->discrete() to test for image textures, no double code
class YAFRAYCORE_EXPORT texture_t
{
	public :
		texture_t() {};
		virtual ~texture_t() {};

		// loadFailed() only used for image
		virtual bool loadFailed() const { return false; }

		virtual colorA_t getColor(const point3d_t &p) const { return color_t(0.0); }
		virtual CFLOAT getFloat(const point3d_t &p) const { return 0; }

		// only used with image backgrounds for SH lighting
		virtual void preFilter(bool spheremap) {}
		virtual colorA_t getColorSH(const vector3d_t &n) const { return colorA_t(0.0); }
		virtual bool has_SH() const { return false; }

		virtual bool discrete() { return false; }
		virtual GFLOAT toPixelU(GFLOAT u) { return u; }
		virtual GFLOAT toPixelV(GFLOAT v) { return v; }
};


enum TEX_MODULATE {TMO_MIX, TMO_ADD, TMO_SUB, TMO_MUL, TMO_SCREEN, TMO_DIFF, TMO_DIV, TMO_DARK, TMO_LIGHT};
enum TEX_MAPTYPE {TXM_FLAT, TXM_CUBE, TXM_TUBE, TXM_SPHERE};
enum TEX_COORDS {TXC_UV, TXC_GLOB, TXC_ORCO, TXC_WIN, TXC_NOR, TXC_REFL};
enum TEX_CLIPMODE {TCL_EXTEND, TCL_CLIP, TCL_CLIPCUBE, TCL_REPEAT, TCL_CHECKER};

// projection
#define TEX_PROJ0 0
#define TEX_PROJX 1
#define TEX_PROJY 2
#define TEX_PROJZ 3

// texture flag
#define TXF_RGBTOINT	1
#define TXF_STENCIL		2
#define TXF_NEGATIVE	4
#define TXF_ALPHAMIX	8

YAFRAYCORE_EXPORT TEX_MODULATE string2texmode(const std::string &modename);

class YAFRAYCORE_EXPORT modulator_t
{
	public :
		modulator_t(const texture_t *tex);
		~modulator_t();

		TEX_MODULATE mode() const { return _mode; }
		void mode(TEX_MODULATE m) { _mode=m; }
		CFLOAT color() const { return _color; }
		void color(CFLOAT c) { _color=c; }
		CFLOAT specular() const { return _specular; }
		void specular(CFLOAT c) { _specular=c; }
		CFLOAT hard() const { return _hard; }
		void hard(CFLOAT c) { _hard=c; }
		CFLOAT transmision() const { return _transmision; }
		void transmision(CFLOAT c) { _transmision=c; }
		CFLOAT reflection() const { return _reflection; }
		void reflection(CFLOAT c) { _reflection=c; }
		CFLOAT displace() const { return _displace; }
		void displace(CFLOAT c) { _displace=c; }

		// size factors
		void sizeX(GFLOAT c) { _sizex=c; }
		void sizeY(GFLOAT c) { _sizey=c; }
		void sizeZ(GFLOAT c) { _sizez=c; }
		void size(GFLOAT c) { _sizex=_sizey=_sizez=c; }

		// offset
		void ofsX(GFLOAT c) { _ofsx=c; }
		void ofsY(GFLOAT c) { _ofsy=c; }
		void ofsZ(GFLOAT c) { _ofsz=c; }

		// matrix
		void setTransform(const matrix4x4_t &M) { tex_Mtx=M;  has_transform=true; }

		// crop
		void setCrop(GFLOAT minx, GFLOAT miny, GFLOAT maxx, GFLOAT maxy) { _cropminx=minx;  _cropminy=miny;  _cropmaxx=maxx;  _cropmaxy=maxy; }

		// repeat
		void setRepeat(int x, int y) { _xrepeat=x;  _yrepeat=y; }

		void string2maptype(const std::string &mapname)
		{
			// default "flat"
			tex_maptype = TXM_FLAT;
			if (mapname=="cube")
				tex_maptype = TXM_CUBE;
			else if (mapname=="tube")
				tex_maptype = TXM_TUBE;
			else if (mapname=="sphere")
				tex_maptype = TXM_SPHERE;
		}

		void string2texcotype(const std::string &texconame)
		{
			// default "uv"
			tex_coords = TXC_UV;
			if (texconame=="global")
				tex_coords = TXC_GLOB;
			else if (texconame=="orco")
				tex_coords = TXC_ORCO;
			else if (texconame=="window")
				tex_coords = TXC_WIN;
			else if (texconame=="normal")
				tex_coords = TXC_NOR;
			else if (texconame=="reflect")
				tex_coords = TXC_REFL;
		}

		void string2cliptype(const std::string &clipname)
		{
			// default "repeat"
			tex_clipmode = TCL_REPEAT;
			if (clipname=="extend")
				tex_clipmode = TCL_EXTEND;
			else if (clipname=="clip")
				tex_clipmode = TCL_CLIP;
			else if (clipname=="clipcube")
				tex_clipmode = TCL_CLIPCUBE;
			else if (clipname=="checker")
				tex_clipmode = TCL_CHECKER;
		}

		void string2texprojection(const std::string &x_axis, const std::string &y_axis, const std::string &z_axis)
		{
			// string find pos. corresponds to TEX_PROJ0/X/Y/Z, if not found, assume disabled (PROJ0)
			const std::string axes = "nxyz";
			if ((tex_projx = axes.find(x_axis))==-1) tex_projx = TEX_PROJ0;
			if ((tex_projy = axes.find(y_axis))==-1) tex_projy = TEX_PROJ0;
			if ((tex_projz = axes.find(z_axis))==-1) tex_projz = TEX_PROJ0;
		}

		void string2modetype(const std::string &modename)
		{
			_mode = string2texmode(modename);
		}

		void setRot90(bool r90) { _rot90=r90; }

		void setChecker(const std::string &chm, GFLOAT chd)
		{
			if (int(chm.find("odd"))!=-1) checker_odd = true;
			if (int(chm.find("even"))!=-1) checker_even = true;
			checker_dist = chd;
		}

		void modulate(color_t &C, color_t &S,CFLOAT &H, const surfacePoint_t &sp, const vector3d_t &eye) const;
		void modulate(color_t &T, color_t &R, const surfacePoint_t &sp, const vector3d_t &eye) const;
		void displace(surfacePoint_t &sp, const vector3d_t &eye, PFLOAT res) const;

		const texture_t & texture() const { return *_tex; };

		bool doMapping(const surfacePoint_t &sp, const vector3d_t &eye,
					point3d_t &texpt) const;

	protected:
		CFLOAT _color, _specular, _hard, _transmision, _reflection, _displace;
		GFLOAT _sizex, _sizey, _sizez;	// texture scale factors
		TEX_MODULATE _mode;
		texture_t *_tex;
		TEX_MAPTYPE tex_maptype;								// image texture mapping mode
		TEX_COORDS tex_coords;									// type of texture coords used for the mapping
		bool has_transform;
		matrix4x4_t tex_Mtx;										// texture transform matrix
		GFLOAT _ofsx, _ofsy, _ofsz;							// texture offset factors
		GFLOAT _cropminx, _cropminy,
					 _cropmaxx, _cropmaxy;						// image texture crop factors, range [0, 1]
		int _xrepeat, _yrepeat;									// image texture repeat factors
		TEX_CLIPMODE	tex_clipmode;							// image texture clipping mode
		char tex_projx, tex_projy, tex_projz;		// texture projection axes
		bool _rot90, checker_odd, checker_even;	// 90 degree rotation of texture, checker clip mode flags
		GFLOAT checker_dist;										// checker clip mode distance between checker texture blocks ( 'mortar')
};

// texture mapping
YAFRAYCORE_EXPORT void angmap(const point3d_t &p, PFLOAT &u, PFLOAT &v);
YAFRAYCORE_EXPORT void tubemap(const point3d_t &p, PFLOAT &u, PFLOAT &v);
YAFRAYCORE_EXPORT void spheremap(const point3d_t &p, PFLOAT &u, PFLOAT &v);

__END_YAFRAY

#endif
