/****************************************************************************
 *
 * 			texture.cc: Texture and modulation implementation
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

#include "basictex.h"
#include "object3d.h"
#include <iostream>

#include "targaIO.h"
#include "HDR_io.h"

#if HAVE_EXR
#include "EXR_io.h"
#endif

using namespace std;

__BEGIN_YAFRAY

noiseGenerator_t* newNoise(const string &ntype)
{
	if (ntype=="blender")
		return new blenderNoise_t();
	else if (ntype=="stdperlin")
		return new stdPerlin_t();
	else if (int(ntype.find("voronoi"))!=-1) {
		voronoi_t::voronoiType vt = voronoi_t::V_F1;	// default
		if (ntype=="voronoi_f1")
			vt = voronoi_t::V_F1;
		else if (ntype=="voronoi_f2")
			vt = voronoi_t::V_F2;
		else if (ntype=="voronoi_f3")
			vt = voronoi_t::V_F3;
		else if (ntype=="voronoi_f4")
			vt = voronoi_t::V_F4;
		else if (ntype=="voronoi_f2f1")
			vt = voronoi_t::V_F2F1;
		else if (ntype=="voronoi_crackle")
			vt = voronoi_t::V_CRACKLE;
		return new voronoi_t(vt);
	}
	else if (ntype=="cellnoise")
		return new cellNoise_t();
	// default
	return new newPerlin_t();
}

//-----------------------------------------------------------------------------------------
// Clouds Texture
//-----------------------------------------------------------------------------------------

textureClouds_t::textureClouds_t(int dep, PFLOAT sz, bool hd,
		const color_t &c1, const color_t &c2,
		const string &ntype, const string &btype)
		:depth(dep), size(sz), hard(hd), color1(c1), color2(c2)
{
	bias = 0;	// default, no bias
	if (btype=="positive") bias=1;
	else if (btype=="negative") bias=2;
	nGen = newNoise(ntype);
}


CFLOAT textureClouds_t::getFloat(const point3d_t &p) const
{
	CFLOAT v = turbulence(nGen, p, depth, size, hard);
	if (bias) {
		v *= v;
		if (bias==1) return -v;	// !!!
	}
	return v;
}

colorA_t textureClouds_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}


texture_t *textureClouds_t::factory(paramMap_t &params,
		renderEnvironment_t &render)
{
	color_t color1(0.0), color2(1.0);
	int depth = 2;
	string _ntype, _btype;
	const string *ntype = &_ntype, *btype=&_btype;
	PFLOAT size = 1;
	bool hard = false;
	params.getParam("noise_type", ntype);
	params.getParam("color1", color1);
	params.getParam("color2", color2);
	params.getParam("depth", depth);
	params.getParam("size", size);
	params.getParam("hard", hard);
	params.getParam("bias", btype);
	return new textureClouds_t(depth, size, hard, color1, color2, *ntype, *btype);
}

//-----------------------------------------------------------------------------------------
// Simple Marble Texture
//-----------------------------------------------------------------------------------------

textureMarble_t::textureMarble_t(int oct, PFLOAT sz, const color_t &c1, const color_t &c2,
			PFLOAT _turb, PFLOAT shp, bool hrd, const string &ntype, const string &shape)
	:octaves(oct), color1(c1), color2(c2), turb(_turb), size(sz), hard(hrd)
{
	sharpness = 1.0;
	if (shp>1) sharpness = 1.0/shp;
	nGen = newNoise(ntype);
	wshape = SIN;
	if (shape=="saw") wshape = SAW;
	else if (shape=="tri") wshape = TRI;
}

CFLOAT textureMarble_t::getFloat(const point3d_t &p) const
{
	PFLOAT w = (p.x + p.y + p.z)*5.0
					+ ((turb==0.0) ? 0.0 : turb*turbulence(nGen, p, octaves, size, hard));
	switch (wshape) {
		case SAW:
			w *= (PFLOAT)(0.5*M_1_PI);
			w -= floor(w);
			break;
		case TRI:
			w *= (PFLOAT)(0.5*M_1_PI);
			w = fabs((PFLOAT)2.0*(w-floor(w))-(PFLOAT)1.0);
			break;
		default:
		case SIN:
			w = (PFLOAT)0.5 + (PFLOAT)0.5*sin(w);
	}
	return pow(w, sharpness);
}

colorA_t textureMarble_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}

texture_t *textureMarble_t::factory(paramMap_t &params,
		renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	int oct = 2;
	PFLOAT turb=1.0, shp=1.0, sz=1.0;
	bool hrd = false;
	string _ntype, _shape;
	const string *ntype=&_ntype, *shape=&_shape;
	params.getParam("noise_type", ntype);
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	params.getParam("depth", oct);
	params.getParam("turbulence", turb);
	params.getParam("sharpness", shp);
	params.getParam("size", sz);
	params.getParam("hard", hrd);
	params.getParam("shape", shape);
	return new textureMarble_t(oct, sz, col1, col2, turb, shp, hrd, *ntype, *shape);
}

//-----------------------------------------------------------------------------------------
// Simple Wood Texture
//-----------------------------------------------------------------------------------------

textureWood_t::textureWood_t(int oct, PFLOAT sz, const color_t &c1, const color_t &c2, PFLOAT _turb,
		bool hrd, const string &ntype, const string &wtype, const string &shape)
	:octaves(oct), color1(c1), color2(c2), turb(_turb), size(sz), hard(hrd)
{
	rings = (wtype=="rings");
	nGen = newNoise(ntype);
	wshape = SIN;
	if (shape=="saw") wshape = SAW;
	else if (shape=="tri") wshape = TRI;
}

CFLOAT textureWood_t::getFloat(const point3d_t &p) const
{
	PFLOAT w;
	if (rings)
		w = sqrt(p.x*p.x + p.y*p.y + p.z*p.z)*20.0;
	else
		w = (p.x + p.y + p.z)*10.0;
	w += (turb==0.0) ? 0.0 : turb*turbulence(nGen, p, octaves, size, hard);
	switch (wshape) {
		case SAW:
			w *= (PFLOAT)(0.5*M_1_PI);
			w -= floor(w);
			break;
		case TRI:
			w *= (PFLOAT)(0.5*M_1_PI);
			w = fabs((PFLOAT)2.0*(w-floor(w))-(PFLOAT)1.0);
			break;
		default:
		case SIN:
			w = (PFLOAT)0.5 + (PFLOAT)0.5*sin(w);
	}
	return w;
}

colorA_t textureWood_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}

texture_t *textureWood_t::factory(paramMap_t &params,
		renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	int oct = 2;
	PFLOAT turb=1.0, sz=1.0, old_rxy;
	bool hrd = false;
	string _ntype, _wtype, _shape;
	const string *ntype=&_ntype, *wtype=&_wtype, *shape=&_shape;
	params.getParam("noise_type", ntype);
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	params.getParam("depth", oct);
	params.getParam("turbulence", turb);
	params.getParam("size", sz);
	params.getParam("hard", hrd);
	params.getParam("wood_type", wtype);
	params.getParam("shape", shape);
	if (params.getParam("ringscale_x", old_rxy) || params.getParam("ringscale_y", old_rxy))
		cerr << "[texturewood]: 'ringscale_x' and 'ringscale_y' are obsolete, use 'size' instead" << endl;
	return new textureWood_t(oct, sz, col1, col2, turb, hrd, *ntype, *wtype, *shape);
}

//-----------------------------------------------------------------------------------------
// Image Texture
//-----------------------------------------------------------------------------------------

extern cBuffer_t* load_jpeg(const char *name);

textureImage_t::textureImage_t(const char *filename, const string &intp)
{
	// interpolation type, bilinear default
	intp_type = BILINEAR;
	if (intp=="none")
		intp_type = NONE;
	else if (intp=="bicubic")
		intp_type = BICUBIC;
	
	// Load image, try to determine from extensions first
	char *ext = strrchr(filename, '.');
	bool jpg_tried = false;
	bool tga_tried = false;
	bool hdr_tried = false;
#if HAVE_EXR
	bool exr_tried = false;
#endif

	image = NULL;
	float_image = NULL;

	cout << "Loading image file " << filename << endl;

	// try loading image using extension as indication of imagetype
	if (ext) {
		// hdr
		if ((!strcmp(ext, ".hdr")) || (!strcmp(ext, ".HDR")) ||
			(!strcmp(ext, ".pic")) || (!strcmp(ext, ".PIC")) ) // the "official" extension for radiance image files
		{
			float_image = loadHDR(filename);
			hdr_tried = true;
		}
		// jpeg
		if ( (!strcmp(ext, ".jpg")) || (!strcmp(ext, ".jpeg")) ||
			 (!strcmp(ext, ".JPG")) || (!strcmp(ext, ".JPEG")) )
#ifdef HAVE_JPEG
		{
			image = load_jpeg(filename);
			jpg_tried = true;
		}
#else
		cout << "Warning, yafray was compiled without jpeg support, cannot load image.\n";
#endif

		// OpenEXR
		if ((!strcmp(ext, ".exr")) || (!strcmp(ext, ".EXR")))
#if HAVE_EXR
		{
			float_image = loadEXR(filename);
			exr_tried = true;
		}
#else
		cout << "Warning, yafray was compiled without OpenEXR support, cannot load image.\n";
#endif

			// targa, apparently, according to ps description, on mac tga extension can be .tpic
		if (((!strcmp(ext, ".tga")) || (!strcmp(ext, ".tpic"))) ||
				((!strcmp(ext, ".TGA")) || (!strcmp(ext, ".TPIC")))) 
		{
			image = loadTGA(filename, false);
			tga_tried = true;
		}

	}
	// if none was able to load (or no extension), try every type until one or none succeeds
	// targa last (targa has no ID)
	if ((float_image==NULL) && (image==NULL)) {
		std::cout << "unknown file extension, testing format...";
		for(;;) {

			if (!hdr_tried) {
				float_image = loadHDR(filename);
				if (float_image)
				{
					std::cout << "identified as Radiance format!\n";
					break;
				}
			}

#ifdef HAVE_JPEG
			if (!jpg_tried) {
				image = load_jpeg(filename);
				if (image)
				{
					std::cout << "identified as Jpeg format!\n";
					break;
				}
			}
#endif

#if HAVE_EXR
			if (!exr_tried) {
				float_image = loadEXR(filename);
				if (float_image)
				{
					std::cout << "identified as OpenEXR format!\n";
					break;
				}
			}
#endif

			if (!tga_tried) {
				image = loadTGA(filename, true);
				if (image)
				{
					std::cout << "identified as Targa format!\n";
					break;
				}
			}

			// nothing worked, give up
			std::cout << "\nunknown format!\n";
			break;

		}

	}

	if (image || float_image) {
		cout << "OK\n";
		failed = false;
	}
	else {
		cout << "Could not load image\n";
		failed = true;
	}

	prefilt = false;
}

textureImage_t::~textureImage_t()
{
	if (image) {
		delete image;
		image = NULL;
	}
	if (float_image) {
		delete float_image;
		float_image = NULL;
	}
}

// for use as background, pre-integrate image, currently assumes angular map
// based on "An Efficient Representation for Irradiance Environment Maps" by Ramamoorthi/Hanrahan.
void textureImage_t::preFilter(bool spheremap)
{
	if ((!image) && (!float_image)) return;
	cout << "Pre-filtering...";
	int width, height;
	if (image) {
		width = image->resx();
		height = image->resy();
	}
	else {
		width = float_image->resx();
		height = float_image->resy();
	}

	float sa = 4.f*M_PI*M_PI/(width*height);
	if (spheremap) sa *= 0.5f;

	GFLOAT r = 1; //always 1 for spheremap
	GFLOAT theta, phi, sinphi, domega, x, y, z;
	color_t col;

	for (int j=0;j<height;j++) {
		GFLOAT v = 1.f-2.f*(j/(GFLOAT)height);
		for (int i=0;i<width;i++) {
			GFLOAT u = 2.f*(i/(GFLOAT)width)-1.f;
			if (!spheremap) r = u*u + v*v;
			if (r<=1.f) {
				if (spheremap) {
					phi = (v*0.5f+0.5f)*M_PI;
					theta = -u*M_PI;
					sinphi = sin(phi);
					domega = sa * sinphi; // latlong
					x=sinphi*sin(theta);  y=sinphi*cos(theta);  z=cos(phi);
				}
				else {
					phi = M_PI*sqrt(r);
					if ((u==0.f) && (v==0.f)) theta=0.5f*M_PI; else theta=atan2(-v, -u);
					sinphi = sin(phi);
					domega = sa * ((phi==0.f)?1.f:(sinphi/phi)); // sinc(phi) -> probe
					x=sinphi*cos(theta);  y=cos(phi);  z=sinphi*sin(theta);
				}
				if (image)
					(*image)(i, (height-1)-j) >> col;
				else
					(*float_image)(i, (height-1)-j) >> col;
				GFLOAT dc2=0.488603f*domega, dc3=1.092548f*domega;
				SH_coeffs[0] += col * 0.282095f*domega;
				SH_coeffs[1] += col * dc2 * y;
				SH_coeffs[2] += col * dc2 * z;
				SH_coeffs[3] += col * dc2 * x;
				SH_coeffs[4] += col * dc3 * x*y;
				SH_coeffs[5] += col * dc3 * y*z;
				SH_coeffs[7] += col * dc3 * x*z;
				SH_coeffs[6] += col * 0.315392f*domega * (3.f*z*z - 1.f);
				SH_coeffs[8] += col * 0.546274f*domega * (x*x - y*y);
			}
		}
	}
	cout << " Done" << endl;
	prefilt = true;
}

colorA_t textureImage_t::getColorSH(const vector3d_t &n) const
{
	if ((!image) && (!float_image)) return colorA_t(0.0);
	const float c1=0.429043f, c2=0.511664f, c3=0.743125f, c4=0.886227f, c5=0.247708f;
	return M_1_PI * (c1*SH_coeffs[8]*(n.x*n.x - n.y*n.y) + c3*SH_coeffs[6]*n.z*n.z + c4*SH_coeffs[0] - c5*SH_coeffs[6]
						+ 2.f*c1*(SH_coeffs[4]*n.x*n.y + SH_coeffs[7]*n.x*n.z + SH_coeffs[5]*n.y*n.z)
						+ 2.f*c2*(SH_coeffs[3]*n.x + SH_coeffs[1]*n.y + SH_coeffs[2]*n.z));
}

colorA_t cubicInterpolate(const colorA_t &c1, const colorA_t &c2,
													const colorA_t &c3, const colorA_t &c4, CFLOAT x)
{
	colorA_t t2(c3-c2);
	colorA_t t1(t2 - (c2-c1));
	t2 = (c4-c3) - t2;
	CFLOAT ix = 1.f-x;
	return x*c3 + ix*c2 + ((4.f*t2 - t1)*(x*x*x-x) + (4.f*t1 - t2)*(ix*ix*ix-ix))*0.06666667f;
}

template<typename T>
colorA_t interpolateImage(T image, textureImage_t::INTERPOLATE_TYPE intp, const point3d_t &p)
{
	int x, y, x2, y2;
	int resx=image->resx(), resy=image->resy();
	CFLOAT xf = ((CFLOAT)resx * (p.x - floor(p.x)));
	CFLOAT yf = ((CFLOAT)resy * (p.y - floor(p.y)));
	if (intp!=textureImage_t::NONE) { xf -= 0.5f;  yf -= 0.5f; }
	if ((x=(int)xf)<0) x = 0;
	if ((y=(int)yf)<0) y = 0;
	if (x>=resx) x = resx-1;
	if (y>=resy) y = resy-1;
	colorA_t c1;
	(*image)(x, y) >> c1;
	if (intp==textureImage_t::NONE) return c1;
	colorA_t c2, c3, c4;
	if ((x2=x+1)>=resx) x2 = resx-1;
	if ((y2=y+1)>=resy) y2 = resy-1;
	(*image)(x2, y)  >> c2;
	(*image)(x,  y2) >> c3;
	(*image)(x2, y2) >> c4;
	CFLOAT dx=xf-floor(xf), dy=yf-floor(yf);
	if (intp==textureImage_t::BILINEAR) {
		CFLOAT w0=(1-dx)*(1-dy), w1=(1-dx)*dy, w2=dx*(1-dy), w3=dx*dy;
		return colorA_t(w0*c1.getR() + w1*c3.getR() + w2*c2.getR() + w3*c4.getR(),
										w0*c1.getG() + w1*c3.getG() + w2*c2.getG() + w3*c4.getG(),
										w0*c1.getB() + w1*c3.getB() + w2*c2.getB() + w3*c4.getB(),
										w0*c1.getA() + w1*c3.getA() + w2*c2.getA() + w3*c4.getA());
	}
	int x0=x-1, x3=x2+1, y0=y-1, y3=y2+1;
	if (x0<0) x0 = 0;
	if (y0<0) y0 = 0;
	if (x3>=resx) x3 = resx-1;
	if (y3>=resy) y3 = resy-1;
	colorA_t c0, c5, c6, c7, c8, c9, cA, cB, cC, cD, cE, cF;
	(*image)(x0, y0) >> c0;
	(*image)(x,  y0) >> c5;
	(*image)(x2, y0) >> c6;
	(*image)(x3, y0) >> c7;
	(*image)(x0, y)  >> c8;
	(*image)(x3, y)  >> c9;
	(*image)(x0, y2) >> cA;
	(*image)(x3, y2) >> cB;
	(*image)(x0, y3) >> cC;
	(*image)(x,  y3) >> cD;
	(*image)(x2, y3) >> cE;
	(*image)(x3, y3) >> cF;
	c0 = cubicInterpolate(c0, c5, c6, c7, dx);
	c8 = cubicInterpolate(c8, c1, c2, c9, dx);
	cA = cubicInterpolate(cA, c3, c4, cB, dx);
	cC = cubicInterpolate(cC, cD, cE, cF, dx);
	return cubicInterpolate(c0, c8, cA, cC, dy);
}

colorA_t textureImage_t::getColor(const point3d_t &p) const
{
	// p->x/y == u, v
	if (image)
		return interpolateImage(image, intp_type, p);
	else if (float_image)
		return interpolateImage(float_image, intp_type, p);
	return color_t(0.0);
}

CFLOAT textureImage_t::getFloat(const point3d_t &p) const
{
	return getColor(p).energy();
}

texture_t *textureImage_t::factory(paramMap_t &params,
		renderEnvironment_t &render)
{
	string _name, _intp="bilinear";	// default bilinear interpolation
	const string *name=&_name, *intp=&_intp;
	params.getParam("interpolate", intp);
	params.getParam("filename", name);	
	if (*name=="")
		cerr << "Required argument filename not found for image texture\n";
	else
		return new textureImage_t(name->c_str(), *intp);
	return NULL;
}

//-----------------------------------------------------------------------------------------
// voronoi block

textureVoronoi_t::textureVoronoi_t(const color_t &c1, const color_t &c2,
		int ct,
		CFLOAT _w1, CFLOAT _w2, CFLOAT _w3, CFLOAT _w4,
		PFLOAT mex, PFLOAT sz,
		CFLOAT isc, const string &dname)
		:w1(_w1), w2(_w2), w3(_w3), w4(_w4), size(sz), coltype(ct)
{
	voronoi_t::dMetricType dm = voronoi_t::DIST_REAL;
	if (dname=="squared")
		dm = voronoi_t::DIST_SQUARED;
	else if (dname=="manhattan")
		dm = voronoi_t::DIST_MANHATTAN;
	else if (dname=="chebychev")
		dm = voronoi_t::DIST_CHEBYCHEV;
	else if (dname=="minkovsky_half")
		dm = voronoi_t::DIST_MINKOVSKY_HALF;
	else if (dname=="minkovsky_four")
		dm = voronoi_t::DIST_MINKOVSKY_FOUR;
	else if (dname=="minkovsky")
		dm = voronoi_t::DIST_MINKOVSKY;
	vGen.setDistM(dm);
	vGen.setMinkovskyExponent(mex);
	aw1 = fabs(_w1);
	aw2 = fabs(_w2);
	aw3 = fabs(_w3);
	aw4 = fabs(_w4);
	iscale = aw1 + aw2 + aw3 + aw4;
	if (iscale!=0) iscale = isc/iscale;
}

CFLOAT textureVoronoi_t::getFloat(const point3d_t &p) const
{
	vGen.getFeatures(p*size);
	return iscale * fabs(w1*vGen.getDistance(0) + w2*vGen.getDistance(1)
			+ w3*vGen.getDistance(2) + w4*vGen.getDistance(3));
}

colorA_t textureVoronoi_t::getColor(const point3d_t &p) const
{
	CFLOAT inte = getFloat(p);
	colorA_t col(0.0);
	if (coltype) {
		col += aw1 * cellNoiseColor(vGen.getPoint(0));
		col += aw2 * cellNoiseColor(vGen.getPoint(1));
		col += aw3 * cellNoiseColor(vGen.getPoint(2));
		col += aw4 * cellNoiseColor(vGen.getPoint(3));
		if (coltype>=2) {
			CFLOAT t1 = (vGen.getDistance(1) - vGen.getDistance(0))*10.0;
			if (t1>1) t1=1;
			if (coltype==3) t1*=inte; else t1*=iscale;
			col *= t1;
		}
		else col *= iscale;
	}
	else col.set(inte, inte, inte, inte);
	return col;
}

texture_t *textureVoronoi_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	string _cltype, _dname;
	const string *cltype=&_cltype, *dname=&_dname;
	CFLOAT fw1=1, fw2=0, fw3=0, fw4=0;
	PFLOAT mex=2.5;	// minkovsky exponent
	CFLOAT isc=1;	// intensity scale
	PFLOAT sz=1;	// size
	int ct=0;	// default "int" color type (intensity)
	
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	
	params.getParam("color_type", cltype);
	if (*cltype=="col1") ct=1;
	else if (*cltype=="col2") ct=2;
	else if (*cltype=="col3") ct=3;
	
	params.getParam("weight1", fw1);
	params.getParam("weight2", fw2);
	params.getParam("weight3", fw3);
	params.getParam("weight4", fw4);
	params.getParam("mk_exponent", mex);
	
	params.getParam("intensity", isc);
	params.getParam("size", sz);
	
	params.getParam("distance_metric", dname);
	
	return new textureVoronoi_t(col1, col2, ct, fw1, fw2, fw3, fw4, mex, sz, isc, *dname);
}

//-----------------------------------------------------------------------------------------
// Musgrave block

textureMusgrave_t::textureMusgrave_t(const color_t &c1, const color_t &c2,
				PFLOAT H, PFLOAT lacu, PFLOAT octs, PFLOAT offs, PFLOAT gain,
				PFLOAT _size, CFLOAT _iscale,
				const string &ntype, const string &mtype)
				:color1(c1), color2(c2), size(_size), iscale(_iscale)
{
	nGen = newNoise(ntype);
	if (mtype=="multifractal")
		mGen = new mFractal_t(H, lacu, octs, nGen);
	else if (mtype=="heteroterrain")
		mGen = new heteroTerrain_t(H, lacu, octs, offs, nGen);
	else if (mtype=="hybridmf")
		mGen = new hybridMFractal_t(H, lacu, octs, offs, gain, nGen);
	else if (mtype=="ridgedmf")
		mGen = new ridgedMFractal_t(H, lacu, octs, offs, gain, nGen);
	else	// 'fBm' default
		mGen = new fBm_t(H, lacu, octs, nGen);
}


CFLOAT textureMusgrave_t::getFloat(const point3d_t &p) const
{
	return iscale * (*mGen)(p*size);
}

colorA_t textureMusgrave_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}

texture_t *textureMusgrave_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	string _ntype, _mtype;
	const string *ntype=&_ntype, *mtype=&_mtype;
	PFLOAT H=1, lacu=2, octs=2, offs=1, gain=1, size=1, iscale=1;
	
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	
	params.getParam("type", mtype);
	params.getParam("noise_type", ntype);
	
	params.getParam("H", H);
	params.getParam("lacunarity", lacu);
	params.getParam("octaves", octs);
	params.getParam("offset", offs);
	params.getParam("gain", gain);
	params.getParam("size", size);
	params.getParam("intensity", iscale);

	return new textureMusgrave_t(col1, col2, H, lacu, octs, offs, gain, size, iscale, *ntype, *mtype);
}

//-----------------------------------------------------------------------------------------
// Distored Noise block

textureDistortedNoise_t::textureDistortedNoise_t(const color_t &c1, const color_t &c2,
			PFLOAT _distort, PFLOAT _size,
			const string &noiseb1, const string noiseb2)
			:color1(c1), color2(c2), distort(_distort), size(_size)
{
	nGen1 = newNoise(noiseb1);
	nGen2 = newNoise(noiseb2);
}

CFLOAT textureDistortedNoise_t::getFloat(const point3d_t &p) const
{
	// get a random vector and scale the randomization
	const point3d_t ofs(13.5, 13.5, 13.5);
	point3d_t tp(p*size);
	point3d_t rv(getSignedNoise(nGen1, tp+ofs), getSignedNoise(nGen1, tp), getSignedNoise(nGen1, tp-ofs));
	return getSignedNoise(nGen2, tp+rv*distort);	// distorted-domain noise
}

colorA_t textureDistortedNoise_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}

texture_t *textureDistortedNoise_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	string _ntype1, _ntype2;
	const string *ntype1=&_ntype1, *ntype2=&_ntype2;
	PFLOAT dist=1, size=1;
	
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	
	params.getParam("noise_type1", ntype1);
	params.getParam("noise_type2", ntype2);
	
	params.getParam("distort", dist);
	params.getParam("size", size);
	
	return new textureDistortedNoise_t(col1, col2, dist, size, *ntype1, *ntype2);
}

//-----------------------------------------------------------------------------------------
// Gradient block

textureGradient_t::textureGradient_t(const color_t &c1, const color_t &c2,
			const string &_gtype, bool fxy)
			:color1(c1), color2(c2), flip_xy(fxy)
{
	if (_gtype=="quadratic")
		gtype = 1;
	else if (_gtype=="cubic")
		gtype = 2;
	else if (_gtype=="diagonal")
		gtype = 3;
	else if (_gtype=="sphere")
		gtype = 4;
	else if (_gtype=="halo")
		gtype = 5;
	else gtype = 0;	// default, linear
}

CFLOAT textureGradient_t::getFloat(const point3d_t &p) const
{
	CFLOAT x, y, gr;
	if (flip_xy) { x=p.y;  y=p.x; }
	else { x=p.x;  y=p.y; }
	switch (gtype) {
		case 1:  // quadratic
			gr = (1.0+x)*0.5;
			if (gr<0) return 0;
			return gr*gr;
		case 2:  // cubic
			gr = (1.0+x)*0.5;
			if (gr<0) return 0;
			if (gr>1) return 1;
			return gr*gr*(3.0-2.0*gr);
		case 3:  // diagonal
			return (2.0+x+y)*0.25;
		case 4:  // sphere
			gr = 1.0 - sqrt(x*x+ y*y + p.z*p.z);
			if (gr<0) return 0;
			return gr;
		case 5:  // halo
			gr = 1.0 - sqrt(x*x+ y*y + p.z*p.z);
			if (gr<0) return 0;
			return gr*gr;
		default:
		case 0:  // linear
			return (1.0+x)*0.5;
	}
}

colorA_t textureGradient_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}

texture_t *textureGradient_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	string _gtp;
	const string *gtp=&_gtp;
	bool flip = false;
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	params.getParam("gradient_type", gtp);
	params.getParam("flip_xy", flip);
	return new textureGradient_t(col1, col2, *gtp, flip);
}

//-----------------------------------------------------------------------------------------
// Random Noise block

CFLOAT textureRandomNoise_t::getFloat(const point3d_t &p) const
{
	CFLOAT div=3;
	int ran = ourRandomI() & 0x7fffffff;
	int val = (ran & 3);
	int loop = depth;
	while (loop--) {
		ran >>= 2;
		val *= (ran & 3);
		div *= 3.0;
	}
	return (CFLOAT)val/div;
}

colorA_t textureRandomNoise_t::getColor(const point3d_t &p) const
{
	return color1 + getFloat(p)*(color2 - color1);
}

texture_t *textureRandomNoise_t::factory(paramMap_t &params, renderEnvironment_t &render)
{
	color_t col1(0.0), col2(1.0);
	int depth = 0;
	params.getParam("depth", depth);
	return new textureRandomNoise_t(col1, col2, depth);
}

//-----------------------------------------------------------------------------------------

__END_YAFRAY
