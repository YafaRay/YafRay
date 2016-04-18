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

#include "texture.h"
#include "object3d.h"
#include <iostream>

using namespace std;

__BEGIN_YAFRAY

TEX_MODULATE string2texmode(const std::string &modename)
{
	// default "mix"
	if (modename=="add")
		return TMO_ADD;
	else if (modename=="sub")
		return TMO_SUB;
	else if (modename=="mul")
		return TMO_MUL;
	else if (modename=="screen")
		return TMO_SCREEN;
	else if (modename=="difference")
		return TMO_DIFF;
	else if (modename=="divide")
		return TMO_DIV;
	else if (modename=="darken")
		return TMO_DARK;
	else if (modename=="lighten")
		return TMO_LIGHT;
	return TMO_MIX;
}

//------------------------------------------------------------------------------------------
// texture mapping

// moved from hdri.cc
void angmap(const point3d_t &p, PFLOAT &u, PFLOAT &v)
{
	PFLOAT r = p.x*p.x + p.z*p.z;
	if (r!=0.f)  {
		r = 1.f/sqrt(r);
		if (p.y>1.f)
			r = 0;
		else if (p.y>=-1.f)
			r *= M_1_PI * acos(p.y);
	}
	if ((u = 0.5f - 0.5f*p.x*r)<0.f) u=0.f; else if (u>1.f) u=1.f;
	if ((v = 0.5f + 0.5f*p.z*r)<0.f) v=0.f; else if (v>1.f) v=1.f;
}

// slightly modified Blender's own functions,
// works better than previous functions which needed extra tweaks
void tubemap(const point3d_t &p, PFLOAT &u, PFLOAT &v)
{
	u = 0;
	v = 1 - (p.z + 1)*0.5;
	PFLOAT d = p.x*p.x + p.y*p.y;
	if (d>0) {
		d = 1/sqrt(d);
		u = 0.5*(1 - (atan2(p.x*d, p.y*d) *M_1_PI));
	}
}

void spheremap(const point3d_t &p, PFLOAT &u, PFLOAT &v)
{
	PFLOAT d = p.x*p.x + p.y*p.y + p.z*p.z;
	u = v= 0;
	if (d>0) {
		if ((p.x!=0) && (p.y!=0)) u = 0.5*(1 - atan2(p.x, p.y)*M_1_PI);
		v = acos(p.z/sqrt(d)) * M_1_PI;
	}
}

modulator_t::modulator_t(const texture_t *tex)
{
	_color = _specular = _hard = _transmision = _reflection = _displace = 0;
	_mode = TMO_MIX;
	_tex = (texture_t *)tex;
	_sizex = _sizey = _sizez = 1.0;
	tex_maptype = TXM_FLAT;
	tex_coords = TXC_ORCO;
	tex_Mtx.identity();
	has_transform = false;
	_ofsx = _ofsy = _ofsz = 0.0;
	_cropminx = _cropminy = 0.0;
	_cropmaxx = _cropmaxy = 1.0;
	_xrepeat = _yrepeat = 1;
	tex_clipmode = TCL_REPEAT;
	tex_projx = TEX_PROJX;
	tex_projy = TEX_PROJY;
	tex_projz = TEX_PROJZ;
	checker_odd = checker_even = false;
	checker_dist = 0;
}

modulator_t::~modulator_t() 
{
}

// copy of blendermapper::doMapping
// creates texture coord from surface point, returns true if texture clipped
bool modulator_t::doMapping(const surfacePoint_t &sp, const vector3d_t &eye, point3d_t &texpt) const
{
	bool outside = false;	// clipflag

	switch (tex_coords) {
		case TXC_UV: {
			// this might seem weird, but since the texture coords are changed later on
			// have to adapt the uv-coords here as well...
			texpt.x = sp.u()*2.0 - 1.0;
			texpt.y = 1.0 - sp.v()*2.0;
			texpt.z = 0.0;
			break;
		}
		case TXC_GLOB: {
			// coords as is without transform to object
			texpt = sp.P();
			break;
		}
		case TXC_ORCO: {
			// scaled/rotated/translated to fit object size/rotation/position
			if (sp.hasOrco())
				texpt = sp.orco();
			else
				texpt = sp.getObject()->toObjectOrco(sp.P());
			break;
		}
		case TXC_WIN: {
			// Current (normalized) screen sampling coords.
			sp.getScreenPos(texpt.x, texpt.y);
			texpt.z = 0.0;
			break;
		}
		case TXC_NOR: {
			// current normal
			texpt = sp.N();
			break;
		}
		case TXC_REFL: {
			// reflection vector
			vector3d_t neye = eye;
			neye.normalize();
			vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), neye);
			texpt = point3d_t(reflect(N, neye));
		}
	}

	// For render from blender, 'object' texture mode is also set as global,
	// but a transform is used to rotate/scale/translate the texture accordingly.
	// This is also used with reflection mapping, translation ignored for reflect & normal mapping
	if (has_transform) {
		if ((tex_coords==TXC_REFL) || (tex_coords==TXC_NOR))
			texpt = point3d_t(tex_Mtx * toVector(texpt));
		else
			texpt = tex_Mtx * texpt;
	}

	// for projection axes swap
	float texT[3] = {texpt.x, texpt.y, texpt.z};

	// no more PROC_TEX/IMAGE_TEX flag, can determine the same from discrete()
	if (_tex->discrete()) {
		// parameters only relevant to image texture

		// projection first
		if (tex_projx) texpt.x=texT[tex_projx-1]; else texpt.x=0.0;
		if (tex_projy) texpt.y=texT[tex_projy-1]; else texpt.y=0.0;
		if (tex_projz) texpt.z=texT[tex_projz-1]; else texpt.z=0.0;

		// mapping next
		switch (tex_maptype) {
			case TXM_FLAT: {
				texpt.x = (texpt.x+1)*0.5;
				texpt.y = (1-texpt.y)*0.5;
				break;
			}
			case TXM_CUBE: {
				// orient normal, Blender's cubemap() not implemented yet,
				// needs face puno flag (needed at all? sofar everything seems to work without it)
				vector3d_t aN;
				// bug was here, cannot use the displaced normal for texture coords here
				if ((has_transform) && (tex_coords==TXC_GLOB)) {
					// assume object/global mapping, cubemap_ob()
					aN = tex_Mtx * sp.Nd();
					aN.normalize();
				}
				else aN = sp.getObject()->toObjectRot(sp.Nd());	// global mapping but not used for yafray global mapping(!), cubemap_glob()
				aN.abs();
				if ((aN.x>=aN.y) && (aN.x>=aN.z))
					texpt.set((texpt.y+1)*0.5, (1-texpt.z)*0.5, texpt.z);
				else if ((aN.y>=aN.x) && (aN.y>=aN.z))
					texpt.set((texpt.x+1)*0.5, (1-texpt.z)*0.5, texpt.z);
				else
					texpt.set((texpt.x+1)*0.5, (1-texpt.y)*0.5, texpt.z);
				break;
			}
			case TXM_TUBE: {
				PFLOAT u, v;
				tubemap(texpt, u, v);
				texpt.x = u;
				texpt.y = v;
				break;
			}
			case TXM_SPHERE: {
				PFLOAT u, v;
				spheremap(texpt, u, v);
				texpt.x = u;
				texpt.y = v;
				break;
			}
		}

		// repeat
		if (_xrepeat>1) {
			texpt.x *= (PFLOAT)_xrepeat;
			if (texpt.x>1.0) texpt.x -= int(texpt.x); else if (texpt.x<0.0) texpt.x += 1-int(texpt.x);
		}
		if (_yrepeat>1) {
			texpt.y *= (PFLOAT)_yrepeat;
			if (texpt.y>1.0) texpt.y -= int(texpt.y); else if (texpt.y<0.0) texpt.y += 1-int(texpt.y);
		}

		// crop
		if ((_cropminx!=0.0) || (_cropmaxx!=1.0)) texpt.x = _cropminx + texpt.x*(_cropmaxx - _cropminx);
		if ((_cropminy!=0.0) || (_cropmaxy!=1.0)) texpt.y = _cropminy + texpt.y*(_cropmaxy - _cropminy);

		// size & offset last, ofsy is negated
		// if rot90 enabled, swap u & v, and negate both size & ofs
		if (_rot90) {
			PFLOAT tx = texpt.x;
			texpt.x = -_sizey*(texpt.y-0.5) + _ofsy + 0.5;
			texpt.y = -_sizex*(tx-0.5) - _ofsx + 0.5;
		}
		else {
			texpt.x = _sizex*(texpt.x-0.5) + _ofsx + 0.5;
			texpt.y = _sizey*(texpt.y-0.5) - _ofsy + 0.5;
		}

		// clipping
		switch (tex_clipmode) {
			case TCL_CLIPCUBE: {
				if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1) || (texpt.z<-1) || (texpt.z>1))
					outside = true;
				else
					outside = false;
				break;
			}
			case TCL_CHECKER: {
				int xs=(int)floor(texpt.x), ys=(int)floor(texpt.y);
				texpt.x -= xs;
				texpt.y -= ys;
				outside = false;
				if (!checker_odd) {
					if (((xs+ys) & 1)==0) {
						outside = true;
						break;
					}
				}
				if (!checker_even) {
					if ((xs+ys) & 1) {
						outside = true;
						break;
					}
				}
				// scale around center, (0.5, 0.5)
				if (checker_dist<1.0) {
					texpt.x = (texpt.x-0.5) / (1.0-checker_dist) + 0.5;
					texpt.y = (texpt.y-0.5) / (1.0-checker_dist) + 0.5;
				}
				// continue to TCL_CLIP
			}
			case TCL_CLIP: {
				if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1))
					outside = true;
				else
					outside = false;
				break;
			}
			case TCL_EXTEND: {
				if (texpt.x>1) texpt.x=1; else if (texpt.x<0) texpt.x=0;
				if (texpt.y>1) texpt.y=1; else if (texpt.y<0) texpt.y=0;
				// no break, fall thru to TEX_REPEAT
			}
			default:
			case TCL_REPEAT: {
				outside = false;
			}
		}

	}
	else {
		// procedural texture
		if (tex_projx) texpt.x=_sizex*(texT[tex_projx-1]+_ofsx); else texpt.x=_sizex*_ofsx;
		if (tex_projy) texpt.y=_sizey*(texT[tex_projy-1]+_ofsy); else texpt.y=_sizey*_ofsy;
		if (tex_projz) texpt.z=_sizez*(texT[tex_projz-1]+_ofsz); else texpt.z=_sizez*_ofsz;
		outside = false;	// no clipping for this type
	}

	return outside;
}

/*
// creates texture coord from surface point, returns true if texture clipped
bool modulator_t::doMapping(const surfacePoint_t &sp, const vector3d_t &eye,
				point3d_t &texpt) const
{
	bool outside = false;	// clipflag

	switch (tex_coords) {
		case TXC_UV: {
			// this might seem weird, but since the texture coords are changed later on
			// have to adapt the uv-coords here as well...
			texpt.x = sp.u()*2.0 - 1;
			texpt.y = sp.v()*-2.0 + 1;
			texpt.z = -0.98;	// some arbitrary(?) value, (R.lo[2] in Blender, orco?), never seems to change
			break;
		}
		case TXC_GLOB: {
			// coords as is without transform to object
			texpt = sp.P();
			break;
		}
		case TXC_ORCO: {
			// scaled/rotated/translated to fit object size/rotation/position
			texpt = sp.getObject()->toObjectOrco(sp.P());
			break;
		}
		case TXC_WIN: {
			// Current (normalized) screen sampling coords.
			sp.getScreenPos(texpt.x, texpt.y);
			texpt.z = -0.98;	// some arbitrary(?) value, (R.lo[2] in Blender, orco?), never seems to change
			break;
		}
		case TXC_NOR: {
			// current normal
			vector3d_t N = sp.N();
			texpt = point3d_t(-N.x, N.z, N.y);
			break;
		}
		case TXC_REFL: {
			// reflection vector
			vector3d_t neye = eye;
			neye.normalize();
			vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), neye);
			texpt = point3d_t(reflect(N, neye));
		}
	}

	// For render from blender, 'object' texture mode is also set as global,
	// but a transform is used to rotate/scale/translate the texture accordingly.
	// This is also used with reflection mapping, BUT full mtx only used with flat, otherwise translation ignored
	if (has_transform) {
		if (tex_coords==TXC_REFL) {
			if (tex_maptype==TXM_FLAT)
				texpt = tex_Mtx * texpt;
			else
				texpt = point3d_t(tex_Mtx * toVector(texpt));
		}
		else texpt = tex_Mtx * texpt;
	}

	// for projection axes swap
	float texT[3] = {texpt.x, texpt.y, texpt.z};

	// no more PROC_TEX/IMAGE_TEX flag, can determine the same from discrete()
	if (_tex->discrete()) {
		// parameters only relevant to image texture

		// projection first
		if (tex_projx) texpt.x=texT[tex_projx-1]; else texpt.x=0.0;
		if (tex_projy) texpt.y=texT[tex_projy-1]; else texpt.y=0.0;
		if (tex_projz) texpt.z=texT[tex_projz-1]; else texpt.z=0.0;

		// mapping next
		switch (tex_maptype) {
			case TXM_FLAT: {
				texpt.x = (texpt.x+1)*0.5;
				texpt.y = (1-texpt.y)*0.5;
				break;
			}
			case TXM_CUBE: {
				// orient normal, Blender's cubemap() not implemented yet,
				// needs face puno flag (needed at all? sofar everything seems to work without it)
				vector3d_t aN;
				if ((has_transform) || (tex_coords==TXC_GLOB)) {
					// assume object/global mapping, cubemap_ob()
					aN = tex_Mtx * sp.N();
					aN.normalize();
				}
				else aN = sp.getObject()->toObjectRot(sp.N());	// global mapping but not used for yafray global mapping(!), cubemap_glob()
				aN.abs();
				if ((aN.x>=aN.y) && (aN.x>=aN.z))
					texpt.set((texpt.y+1)*0.5, (1-texpt.z)*0.5, texpt.z);
				else if ((aN.y>=aN.x) && (aN.y>=aN.z))
					texpt.set((texpt.x+1)*0.5, (1-texpt.z)*0.5, texpt.z);
				else
					texpt.set((texpt.x+1)*0.5, (1-texpt.y)*0.5, texpt.z);
				break;
			}
			case TXM_TUBE: {
				PFLOAT u, v;
				tubemap(texpt, u, v);
				texpt.x = u;
				texpt.y = v;
				break;
			}
			case TXM_SPHERE: {
				PFLOAT u, v;
				spheremap(texpt, u, v);
				texpt.x = u;
				texpt.y = v;
				break;
			}
		}

		// repeat
		if (_xrepeat>1) {
			texpt.x *= (PFLOAT)_xrepeat;
			if (texpt.x>1.0) texpt.x -= int(texpt.x); else if (texpt.x<0.0) texpt.x += 1-int(texpt.x);
		}
		if (_yrepeat>1) {
			texpt.y *= (PFLOAT)_yrepeat;
			if (texpt.y>1.0) texpt.y -= int(texpt.y); else if (texpt.y<0.0) texpt.y += 1-int(texpt.y);
		}

		// crop
		if ((_cropminx!=0.0) || (_cropmaxx!=1.0)) texpt.x = _cropminx + texpt.x*(_cropmaxx - _cropminx);
		// cropminy/maxy swapped
		if ((_cropminy!=1.0) || (_cropmaxy!=0.0)) texpt.y = _cropminy + texpt.y*(_cropmaxy - _cropminy);

		// size & offset last
		texpt.x = _sizex * (texpt.x-0.5) + _ofsx + 0.5;
		texpt.y = _sizey * (texpt.y-0.5) + _ofsy + 0.5;

		// clipping

		switch (tex_clipmode) {
			case TCL_CLIPCUBE: {
				if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1) || (texpt.z<-1) || (texpt.z>1))
					outside = true;
				else
					outside = false;
				break;
			}
			case TCL_CLIP: {
				if ((texpt.x<0) || (texpt.x>1) || (texpt.y<0) || (texpt.y>1))
					outside = true;
				else
					outside = false;
				break;
			}
			case TCL_EXTEND: {
				if (texpt.x>1) texpt.x=1; else if (texpt.x<0) texpt.x=0;
				if (texpt.y>1) texpt.y=1; else if (texpt.y<0) texpt.y=0;
				// no break, fall thru to TEX_REPEAT
			}
			case TCL_REPEAT: {
				outside = false;
				break;
			}
		}

	}
	else {
		// procedural texture
		if (tex_projx) texpt.x=_sizex*(texT[tex_projx-1]+_ofsx); else texpt.x=_sizex*_ofsx;
		if (tex_projy) texpt.y=_sizey*(texT[tex_projy-1]+_ofsy); else texpt.y=_sizey*_ofsy;
		if (tex_projz) texpt.z=_sizez*(texT[tex_projz-1]+_ofsz); else texpt.z=_sizez*_ofsz;
		outside = false;	// no clipping for this type
	}

	return outside;
}
*/

//------------------------------------------------------------------------------------------
// modulator

void modulator_t::modulate(color_t &C, color_t &S, CFLOAT &H, const surfacePoint_t &sp, const vector3d_t &eye) const
{
	point3d_t texpt;
	if (doMapping(sp, eye, texpt)) return;	// doMapping returns true if texture clipped
	color_t texcolor = _tex->getColor(texpt);
	CFLOAT texfloat = _tex->getFloat(texpt);

	if (_mode==TMO_MIX)
	{
		if (_color>0)				C = mix(texcolor, C, _color);
		if (_specular>0)	S = mix(texcolor, S, _specular);
		if (_hard>0)				H = texfloat*_hard + H*(1-_hard);
		return;
	}
	if (_mode==TMO_MUL)
	{
		if (_color>0)				C = mix(texcolor, color_t(1.0), _color) * C;
		if (_specular>0)	S = mix(texcolor, color_t(1.0), _specular) * S;
		if (_hard>0)				H = (texfloat*_hard + (1-_hard)) * H;
		return;
	}
	if (_mode==TMO_ADD)
	{
		if (_color>0)				C = (texcolor*_color) + C;
		if (_specular>0)	S = (texcolor*_specular) + S;
		if (_hard>0)				H = texfloat*_hard + H;
		return;
	}
	if (_mode==TMO_SUB)
	{
		if (_color>0)				C = (texcolor * -_color) + C;
		if (_specular>0)	S = (texcolor * -_specular) + S;
		if (_hard>0)				H = (texfloat * -_hard) + H;
		return;
	}
}

void modulator_t::modulate(color_t &T,color_t &R, const surfacePoint_t &sp, const vector3d_t &eye) const
{
	point3d_t texpt;
	if (doMapping(sp, eye, texpt)) return;		//returns true if texture clipped
	color_t texcolor = _tex->getColor(texpt);

	if(_mode==TMO_MIX)
	{
		if (_transmision>0)	T = mix(texcolor, T, _transmision);
		if (_reflection>0)		R = mix(texcolor, R, _reflection);
		return;
	}
	if(_mode==TMO_MUL)
	{
		if (_transmision>0)	T *= mix(texcolor, color_t(1.0), _transmision);
		if (_reflection>0)		R *= mix(texcolor, color_t(1.0), _reflection);
		return;
	}
	if(_mode==TMO_ADD)
	{
		if (_transmision>0)	T += (texcolor * _transmision);
		if (_reflection>0)		R += (texcolor * _reflection);
		return;
	}
	if(_mode==TMO_SUB)
	{
		if (_transmision>0)	T += (texcolor * -_transmision);
		if (_reflection>0)		R += (texcolor * -_reflection);
		return;
	}
}


void modulator_t::displace(surfacePoint_t &sp, const vector3d_t &eye, PFLOAT res) const
{
	if(_displace==0.0) return;

	point3d_t texpt, ntexpt;
	if (doMapping(sp, eye, texpt)) return;		//returns true if texture clipped

	GFLOAT u=texpt.x, v=texpt.y;

	vector3d_t NU=sp.NU()*res, NV=sp.NV()*res;
	PFLOAT diru=0, dirv=0;

	// this should be changed so other texmappings work too,
	// it now depends on UV coords for images
	if ((tex_coords==TXC_UV) && (_tex->discrete() && sp.hasUV()))
	{
		GFLOAT du = _tex->toPixelU(sp.dudNU());
		GFLOAT dv = _tex->toPixelV(sp.dvdNU());
		GFLOAT urel = sqrt(du*du+dv*dv);
		if (urel==0.0) urel=1.0;
		du = _tex->toPixelU(sp.dudNV());
		dv = _tex->toPixelV(sp.dvdNV());
		GFLOAT vrel = sqrt(du*du+dv*dv);
		if (vrel==0.0) vrel=1.0;
		res = 1.0;
		GFLOAT dudNU = sp.dudNU()/urel;
		GFLOAT dvdNU = sp.dvdNU()/urel;
		GFLOAT dudNV = sp.dudNV()/vrel;
		GFLOAT dvdNV = sp.dvdNV()/vrel;

		ntexpt.set(u-dudNU, v-dvdNU, 0);
		diru = _tex->getFloat(ntexpt);
		ntexpt.set(u+dudNU, v+dvdNU, 0);
		diru -= _tex->getFloat(ntexpt);
		diru *= _displace;

		ntexpt.set(u-dudNV, v-dvdNV);
		dirv = _tex->getFloat(ntexpt);
		ntexpt.set(u+dudNV, v+dvdNV);
		dirv -= _tex->getFloat(ntexpt);

	}
	else
	{
		ntexpt = texpt-NU;
		diru = _tex->getFloat(ntexpt);
		ntexpt = texpt+NU;
		diru -= _tex->getFloat(ntexpt);
		diru *= _displace/res;

		ntexpt = texpt-NV;
		dirv = _tex->getFloat(ntexpt);
		ntexpt = texpt+NV;
		dirv -= _tex->getFloat(ntexpt);
		dirv *= _displace/res;
	}
	PFLOAT nless=1.0;
	nless -= ((fabs(diru)>fabs(dirv))? fabs(diru) : fabs(dirv));
	if (nless<0.0) nless=0;
	sp.N() = sp.N()*nless + sp.NU()*diru + sp.NV()*dirv;
	sp.N().normalize();
}

__END_YAFRAY

//-----------------------------------------------------------------------------------------
