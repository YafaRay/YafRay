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
#ifndef __BASICTEX_H
#define __BASICTEX_H

#include "texture.h"
#include "params.h"
#include "noise.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif


__BEGIN_YAFRAY

class textureClouds_t : public texture_t
{
	public:
		textureClouds_t(int dep, PFLOAT sz, bool hd,
				const color_t &c1, const color_t &c2,
				const std::string &ntype, const std::string &btype);
		virtual ~textureClouds_t()
		{
			if (nGen) {
				delete nGen;
				nGen = NULL;
			}
		}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params,renderEnvironment_t &render);
	protected:
		int depth, bias;
		PFLOAT size;
		bool hard;
		color_t color1, color2;
		noiseGenerator_t* nGen;
};


class textureMarble_t : public texture_t
{
	public:
		textureMarble_t(int oct, PFLOAT sz, const color_t &c1, const color_t &c2,
				PFLOAT _turb, PFLOAT shp, bool hrd, const std::string &ntype, const std::string &shape);
		virtual ~textureMarble_t()
		{
			if (nGen) {
				delete nGen;
				nGen = NULL;
			}
		}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params,renderEnvironment_t &render);
	protected:
		int octaves;
		color_t color1, color2;
		PFLOAT turb, sharpness, size;
		bool hard;
		noiseGenerator_t* nGen;
		enum {SIN, SAW, TRI} wshape;
};


class textureWood_t : public texture_t
{
	public:
		textureWood_t(int oct, PFLOAT sz, const color_t &c1, const color_t &c2, PFLOAT _turb,
				bool hrd, const std::string &ntype, const std::string &wtype, const std::string &shape);
		virtual ~textureWood_t()
		{
			if (nGen) {
				delete nGen;
				nGen = NULL;
			}
		}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params,renderEnvironment_t &render);
	protected:
		int octaves;
		color_t color1, color2;
		PFLOAT turb, size;
		bool hard, rings;
		noiseGenerator_t* nGen;
		enum {SIN, SAW, TRI} wshape;
};

class textureVoronoi_t : public texture_t
{
	public:
		textureVoronoi_t(const color_t &c1, const color_t &c2,
				int ct,
				CFLOAT _w1, CFLOAT _w2, CFLOAT _w3, CFLOAT _w4,
				PFLOAT mex, PFLOAT sz,
				CFLOAT isc, const std::string &dname);
		virtual ~textureVoronoi_t() {}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params, renderEnvironment_t &render);
	protected:
		color_t color1, color2;
		CFLOAT w1, w2, w3, w4;	// feature weights
		CFLOAT aw1, aw2, aw3, aw4;	// absolute value of above
		PFLOAT size;
		int coltype;	// color return type
		CFLOAT iscale;	// intensity scale
		voronoi_t vGen;
};

class textureMusgrave_t : public texture_t
{
	public:
		textureMusgrave_t(const color_t &c1, const color_t &c2,
				PFLOAT H, PFLOAT lacu, PFLOAT octs, PFLOAT offs, PFLOAT gain,
				PFLOAT _size, CFLOAT _iscale,
				const std::string &ntype, const std::string &mtype);
		virtual ~textureMusgrave_t()
		{
			if (nGen) {
				delete nGen;
				nGen = NULL;
			}
			if (mGen) {
				delete mGen;
				mGen = NULL;
			}
		}

		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		PFLOAT size, iscale;
		noiseGenerator_t* nGen;
		musgrave_t* mGen;
};

class textureDistortedNoise_t : public texture_t
{
	public:
		textureDistortedNoise_t(const color_t &c1, const color_t &c2,
					PFLOAT _distort, PFLOAT _size,
					const std::string &noiseb1, const std::string noiseb2);
		virtual ~textureDistortedNoise_t()
		{
			if (nGen1) {
				delete nGen1;
				nGen1 = NULL;
			}
			if (nGen2) {
				delete nGen2;
				nGen2 = NULL;
			}
		}
		
		virtual colorA_t getColor(const point3d_t &p) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		PFLOAT distort, size;
		noiseGenerator_t *nGen1, *nGen2;
};

class textureGradient_t : public texture_t
{
	public:
		textureGradient_t(const color_t &c1, const color_t &c2, const std::string &_gtype, bool fxy);
		virtual ~textureGradient_t() {}

		virtual colorA_t getColor(const point3d_t &sp) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		int gtype;
		bool flip_xy;
};

class textureRandomNoise_t : public texture_t
{
	public:
		textureRandomNoise_t(const color_t &c1, const color_t &c2, int d)
				:color1(c1), color2(c2), depth(d) {}
		virtual ~textureRandomNoise_t() {}

		virtual colorA_t getColor(const point3d_t &sp) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		static texture_t *factory(paramMap_t &params, renderEnvironment_t &render);

	protected:
		color_t color1, color2;
		int depth;
};

class textureImage_t : public texture_t
{
	public:
		enum INTERPOLATE_TYPE {NONE, BILINEAR, BICUBIC};
		textureImage_t(const char *filename, const std::string &intp);
		virtual ~textureImage_t();

		virtual colorA_t getColor(const point3d_t &sp) const;
		virtual CFLOAT getFloat(const point3d_t &p) const;

		// for Spherical harmonic coefficients
		virtual void preFilter(bool spheremap);
		virtual colorA_t getColorSH(const vector3d_t &n) const;
		virtual bool has_SH() const { return ((!failed) && (prefilt)); }

		virtual bool loadFailed() const { return failed; }
		virtual bool discrete() { return true; }
		virtual GFLOAT toPixelU(GFLOAT u)
		{
			if (failed) return 0.0;
			return u*(GFLOAT)image->resx();
		}
		virtual GFLOAT toPixelV(GFLOAT v)
		{
			if (failed) return 0.0;
			return v*(GFLOAT)image->resy();
		}
		static texture_t *factory(paramMap_t &params,renderEnvironment_t &render);
	protected:
		cBuffer_t *image;
		fcBuffer_t *float_image;
		bool failed, prefilt;
		INTERPOLATE_TYPE intp_type;
		color_t SH_coeffs[9];
};


__END_YAFRAY
#endif
