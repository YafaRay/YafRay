#include "blendershader.h"
#include "spectrum.h"

__BEGIN_YAFRAY

//------------------------------------------------------------------------------------------
// texture mapping

blenderMapperNode_t::blenderMapperNode_t(const shader_t *m):mapped(m)
{
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

// creates texture coord from surface point, returns true if texture clipped
bool blenderMapperNode_t::doMapping(const surfacePoint_t &sp, const vector3d_t &eye,
				point3d_t &texpt) const
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
	if (mapped->discrete()) {
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
				if (has_transform && (tex_coords==TXC_GLOB)) {
					// assume object/global mapping, cubemap_ob()
					aN = tex_Mtx * sp.Nd();
					aN.normalize();
				}
				else if ((tex_coords==TXC_GLOB) || (tex_coords==TXC_REFL))
					aN = sp.Nd();
				else
					aN = sp.getObject()->toObjectRot(sp.Nd());	// global mapping but not used for yafray global mapping(!), cubemap_glob()
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

		// repeat, only valid for REPEAT clipmode
		if (tex_clipmode==TCL_REPEAT) {
			if (_xrepeat>1) {
				texpt.x *= (PFLOAT)_xrepeat;
				if (texpt.x>1.0) texpt.x -= int(texpt.x); else if (texpt.x<0.0) texpt.x += 1-int(texpt.x);
			}
			if (_yrepeat>1) {
				texpt.y *= (PFLOAT)_yrepeat;
				if (texpt.y>1.0) texpt.y -= int(texpt.y); else if (texpt.y<0.0) texpt.y += 1-int(texpt.y);
			}
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
				if (texpt.x>0.99999f) texpt.x=0.99999f; else if (texpt.x<0) texpt.x=0;
				if (texpt.y>0.99999f) texpt.y=0.99999f; else if (texpt.y<0) texpt.y=0;
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

CFLOAT blenderMapperNode_t::stdoutFloat(renderState_t &state,
		const surfacePoint_t &sp, const vector3d_t &eye,
		const scene_t *scene) const
{
	point3d_t mpoint;
	if (doMapping(sp, eye, mpoint)) return 0.0;
	surfacePoint_t tempsp(sp);
	tempsp.P() = mpoint;
	return mapped->stdoutFloat(state, tempsp, eye, scene);
}

colorA_t blenderMapperNode_t::stdoutColor(renderState_t &state,
		const surfacePoint_t &sp, const vector3d_t &eye,
		const scene_t *scene) const
{
	point3d_t mpoint;
	if (doMapping(sp, eye, mpoint)) return color_t(0.0);
	surfacePoint_t tempsp(sp);
	tempsp.P() = mpoint;
	return mapped->stdoutColor(state, tempsp, eye, scene);
}

void blenderMapperNode_t::string2maptype(const std::string &mapname)
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

void blenderMapperNode_t::string2texcotype(const std::string &texconame)
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

void blenderMapperNode_t::string2cliptype(const std::string &clipname)
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

void blenderMapperNode_t::string2texprojection(const std::string &x_axis,
		const std::string &y_axis, const std::string &z_axis)
{
	// string find pos. corresponds to TEX_PROJ0/X/Y/Z, if not found, assume disabled (PROJ0)
	const std::string axes = "nxyz";
	if ((tex_projx = axes.find(x_axis))==-1) tex_projx = TEX_PROJ0;
	if ((tex_projy = axes.find(y_axis))==-1) tex_projy = TEX_PROJ0;
	if ((tex_projz = axes.find(z_axis))==-1) tex_projz = TEX_PROJ0;
}

using namespace std;

shader_t * blenderMapperNode_t::factory(paramMap_t &bparams,
		std::list<paramMap_t> &lparams,renderEnvironment_t &render)
{
	string _childname;
	const string *childname=&_childname;
	bparams.getParam("input",childname);
	shader_t *child=render.getShader(*childname);
	if(child==NULL)
	{
		cerr << "Null input in blender mapper\n";
		return NULL;
	}
	blenderMapperNode_t *mapper=new blenderMapperNode_t(child);

	GFLOAT size=1, sizex=1, sizey=1, sizez=1;
	
	string _mapping="flat", _texco="orco", _clipping="repeat";	// defaults
	string _projx="x", _projy="y", _projz="z";
	const string *mapping=&_mapping, *texco=&_texco, *clipping=&_clipping;	// defaults
	const string *projx=&_projx, *projy=&_projy, *projz=&_projz;

	int xrep=1, yrep=1;
	matrix4x4_t txm(1);
	GFLOAT ofsx=0, ofsy=0, ofsz=0;
	GFLOAT minx=0, miny=0, maxx=1, maxy=1;
	bool ROT90=false;

	bparams.getParam("size", size);
	bparams.getParam("sizex", sizex);
	bparams.getParam("sizey", sizey);
	bparams.getParam("sizez", sizez);
	bparams.getParam("mapping", mapping);
	bparams.getParam("texco", texco);
	bparams.getParam("clipping", clipping);
	bparams.getParam("xrepeat", xrep);
	bparams.getParam("yrepeat", yrep);
	bparams.getParam("ofsx", ofsx);
	bparams.getParam("ofsy", ofsy);
	bparams.getParam("ofsz", ofsz);
	bparams.getParam("cropmin_x", minx);
	bparams.getParam("cropmin_y", miny);
	bparams.getParam("cropmax_x", maxx);
	bparams.getParam("cropmax_y", maxy);
	bparams.getParam("proj_x", projx);
	bparams.getParam("proj_y", projy);
	bparams.getParam("proj_z", projz);
	bparams.getParam("rot90", ROT90);

	// extra checker clipping mode params
	string _chm = "";
	const string *chm=&_chm;
	GFLOAT chd = 0;
	bparams.getParam("checker_mode", chm);
	bparams.getParam("checker_dist", chd);

	// texture matrix
	bool hasmtx = bparams.getParam("m00", txm[0][0]);
	hasmtx |= bparams.getParam("m01", txm[0][1]);
	hasmtx |= bparams.getParam("m02", txm[0][2]);
	hasmtx |= bparams.getParam("m03", txm[0][3]);
	hasmtx |= bparams.getParam("m10", txm[1][0]);
	hasmtx |= bparams.getParam("m11", txm[1][1]);
	hasmtx |= bparams.getParam("m12", txm[1][2]);
	hasmtx |= bparams.getParam("m13", txm[1][3]);
	hasmtx |= bparams.getParam("m20", txm[2][0]);
	hasmtx |= bparams.getParam("m21", txm[2][1]);
	hasmtx |= bparams.getParam("m22", txm[2][2]);
	hasmtx |= bparams.getParam("m23", txm[2][3]);
	hasmtx |= bparams.getParam("m30", txm[3][0]);
	hasmtx |= bparams.getParam("m31", txm[3][1]);
	hasmtx |= bparams.getParam("m32", txm[3][2]);
	hasmtx |= bparams.getParam("m33", txm[3][3]);

	mapper->setChecker(*chm, chd);
	mapper->setRot90(ROT90);
	mapper->sizeX(sizex);
	mapper->sizeY(sizey);
	mapper->sizeZ(sizez);
	mapper->ofsX(ofsx);
	mapper->ofsY(ofsy);
	mapper->ofsZ(ofsz);
	if (size!=1.0) mapper->size(size);
	mapper->string2maptype(*mapping);
	mapper->string2texcotype(*texco);
	mapper->string2cliptype(*clipping);
	mapper->string2texprojection(*projx, *projy, *projz);
	mapper->setRepeat(xrep, yrep);
	mapper->setCrop(minx, miny, maxx, maxy);
	if (hasmtx) mapper->setTransform(txm);

	return mapper;
}


colorA_t texture_rgb_blend(const colorA_t &tex, const colorA_t &out, CFLOAT fact, CFLOAT facg, TEX_MODULATE blendtype)
{
	
	switch(blendtype) {
		case TMO_MUL:
			fact *= facg;
			return (colorA_t(1.f-facg) + fact*tex)*out;

		case TMO_SCREEN: {
			colorA_t white(1.0);
			fact *= facg;
			return white - (colorA_t(1.f-facg) + fact*(white-tex)) * (white-out);
		}

		case TMO_SUB:
			fact = -fact;
		case TMO_ADD:
			fact *= facg;
			return fact*tex + out;

		case TMO_DIV: {
			fact *= facg;
			colorA_t itex(tex);
			itex.invertRGB();
			return (1.f-fact)*out + fact*out*itex;
		}

		case TMO_DIFF: {
			fact *= facg;
			colorA_t tmo(tex-out);
			tmo.absRGB();
			return (1.f-fact)*out + fact*tmo;
		}

		case TMO_DARK: {
			fact *= facg;
			colorA_t col(fact*tex);
			col.darkenRGB(out);
			return col;
		}

		case TMO_LIGHT: {
			fact *= facg;
			colorA_t col(fact*tex);
			col.lightenRGB(out);
			return col;
		}

		default:
		case TMO_MIX:
			fact *= facg;
			return fact*tex + (1.f-fact)*out;
	}

}

CFLOAT texture_value_blend(CFLOAT tex, CFLOAT out, CFLOAT fact, CFLOAT facg, TEX_MODULATE blendtype, bool flip)
{
	fact *= facg;
	CFLOAT facm = 1.f-fact;
	if (flip) swap(fact, facm);

	switch(blendtype) {
		case TMO_MUL:
			facm = 1.f-facg;
			return (facm+fact*tex)*out;

		case TMO_SCREEN:
			facm = 1.f-facg;
			return 1.f-(facm+fact*(1.f-tex))*(1.f-out);

		case TMO_SUB:
			fact = -fact;
		case TMO_ADD:
			return fact*tex + out;

		case TMO_DIV:
			if (tex==0.f) return 0.f;
			return facm*out + fact*out/tex;

		case TMO_DIFF:
			return facm*out + fact*fabs(tex-out);

		case TMO_DARK: {
			CFLOAT col = fact*tex;
			if (col<out) return col;
			return out;
		}

		case TMO_LIGHT: {
			CFLOAT col = fact*tex;
			if (col>out) return col;
			return out;
		}

		default:
		case TMO_MIX:
			return fact*tex + facm*out;
	}
}

void blenderModulator_t::blenderModulate(colorA_t &col, colorA_t &colspec, colorA_t &colmir, CFLOAT &ref,
		CFLOAT &spec, CFLOAT &har, CFLOAT &emit, CFLOAT &alpha, CFLOAT &raymir,
		CFLOAT &stencilTin, renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye) const
{
	colorA_t texcolor = input->stdoutColor(state, sp, eye);
	CFLOAT Tin=texcolor.energy(), Ta=texcolor.getA();
	bool Talpha = true;

	// procedural textures are intensity only by default
	bool TEX_RGB = input->isRGB();
	if (TEX_RGB) {
		Talpha = ((alpha_flag & TXA_USEALPHA)!=0);
		if (Talpha) {
			if (alpha_flag & TXA_CALCALPHA) texcolor.setAlpha(max(texcolor.getR(), max(texcolor.getG(), texcolor.getB())));
			if (alpha_flag & TXA_NEGALPHA) Ta=1.f-texcolor.getA();
		}
		// contrast & brightness
		texcolor = _filtercolor*((texcolor-colorA_t(0.5))*_contrast + colorA_t(_brightness - 0.5));
		texcolor.clampRGB0();
	}
	else {
		Tin = (Tin-0.5)*_contrast + _brightness - 0.5;
		if (Tin<0) Tin=0; else if (Tin>1) Tin=1;
	}

	if (texflag & TXF_RGBTOINT) {
		Tin = 0.35f*texcolor.getR() + 0.45f*texcolor.getG() + 0.2f*texcolor.getB();
		TEX_RGB = false;
	}
	if (texflag & TXF_NEGATIVE) {
		if (TEX_RGB) texcolor = colorA_t(1.f)-texcolor;
		Tin = 1.f-Tin;
	}
	CFLOAT fact;
	if (texflag & TXF_STENCIL) {
		if (TEX_RGB) {
			fact = Ta;
			Ta *= stencilTin;
			stencilTin *= fact;
		}
		else {
			fact = Tin;
			Tin *= stencilTin;
			stencilTin *= fact;
		}
	}
	else {
		Ta *= stencilTin;
		Tin *= stencilTin;
	}

	// color type modulation
	// _color|_csp|_cmir all switches, positive only, value not used
	if ((_color!=0) || (_csp!=0) || (_cmir!=0)) {

		if (!TEX_RGB)
			texcolor = texture_col;
		else if (_alpha!=0)
			Tin = stencilTin;	// MTEX_ALPHAMIX seems to be unused (old Blender code?)
		else
			Tin = Ta;

		// diffuse color modulation
		if (_color!=0) {
			col = texture_rgb_blend(texcolor, col, Tin, colfac, _mode);
			col.clampRGB0();
		}

		// specular color modulation
		if (_csp!=0) {
			colspec = texture_rgb_blend(texcolor, colspec, Tin, colfac, _mode);
			colspec.clampRGB0();
		}

		// mirror color modulation (blender, not reflection)
		if (_cmir!=0) {
			colmir = texture_rgb_blend(texcolor, colmir, Tin, colfac, _mode);
			colmir.clampRGB0();
		}
	}

	// intensity type modulation
	if ((_ref!=0) || (_specular!=0) || (_hard!=0) || (_alpha!=0) || (_emit!=0) || (_raymir!=0)) {

		if (TEX_RGB) {
			if (Talpha)
				Tin = Ta;
			else
				Tin = 0.35f*texcolor.getR() + 0.45f*texcolor.getG() + 0.2f*texcolor.getB();
		}

		if (_ref!=0) {
			ref = texture_value_blend(def_var, ref, Tin, varfac, _mode, (_ref<0));
			if (ref<0.f) ref=0.f;
		}

		if (_specular!=0) {
			spec = texture_value_blend(def_var, spec, Tin, varfac, _mode, (_specular<0));
			if (spec<0.f) spec=0.f;
		}

		if (_emit!=0) {
			emit = texture_value_blend(def_var, emit, Tin, varfac, _mode, (_emit<0));
			if (emit<0.f) emit=0.f;
		}

		if (_alpha!=0) {
			alpha = texture_value_blend(def_var, alpha, Tin, varfac, _mode, (_alpha<0));
			if (alpha<0.f) alpha=0.f; else if (alpha>1.f) alpha=1.f;
		}

		if (_hard!=0) {
			har = 128.f*texture_value_blend(def_var, har/128.f, Tin, varfac, _mode, (_hard<0));
			if (har<0.f) har=0.f;
		}

		if (_raymir!=0) {
			raymir = texture_value_blend(def_var, raymir, Tin, varfac, _mode, (_raymir<0));
			if (raymir<0.f) raymir=0.f; else if (raymir>1.f) raymir=1.f;
		}
	}

}

void blenderModulator_t::blenderDisplace(renderState_t &state,surfacePoint_t &sp,
		const vector3d_t &eye, PFLOAT res) const
{
	if (_displace==0.0) return;

	PFLOAT nfac = _displace/res;
	if (rgbnormap) {
		color_t nc = input->stdoutColor(state, sp, eye, NULL);
		vector3d_t dno((nc.getR()-0.5)*2.0, (nc.getG()-0.5)*2.0, nc.getB());
		vector3d_t Ru=sp.NU()*nfac, Rv=sp.NV()*nfac, N=sp.N();
		dno.set(dno.x*Ru.x + dno.y*Rv.x + dno.z*N.x,
						dno.x*Ru.y + dno.y*Rv.y + dno.z*N.y,
						dno.x*Ru.z + dno.y*Rv.z + dno.z*N.z);
		PFLOAT nf = 1.0-fabs(nfac);
		if (nf<0.0) nf=0.0;
		sp.N() = nf*sp.N() + dno;
		sp.N().normalize();
		return;
	}

	point3d_t texpt = sp.P();
	point3d_t old = sp.P();
	bool orco = sp.hasOrco();

	sp.hasOrco(false);
	float ou=0, ov=0;
	if (sp.hasUV())
	{
		ou = sp.u();
		ov = sp.v();
	}
	vector3d_t NU=sp.NU()*res, NV=sp.NV()*res;
	PFLOAT diru=0, dirv=0;

	sp.P() = texpt-NU;
	if (sp.hasUV()) { sp.u()=ou-sp.dudNU()*res;  sp.v()=ov-sp.dvdNU()*res; }
	diru = input->stdoutFloat(state, sp, eye);
	sp.P() = texpt+NU;
	if (sp.hasUV()) { sp.u()=ou+sp.dudNU()*res;  sp.v()=ov+sp.dvdNU()*res; }
	diru -= input->stdoutFloat(state, sp, eye);
	diru *= nfac;

	sp.P() = texpt-NV;
	if (sp.hasUV()) { sp.u()=ou-sp.dudNV()*res;  sp.v()=ov-sp.dvdNV()*res; }
	dirv = input->stdoutFloat(state, sp, eye);
	sp.P() = texpt+NV;
	if (sp.hasUV()) { sp.u()=ou+sp.dudNV()*res;  sp.v()=ov+sp.dvdNV()*res; }
	dirv -= input->stdoutFloat(state, sp, eye);
	dirv *= nfac;

	PFLOAT nless = 1.0 - ((fabs(diru)>fabs(dirv))? fabs(diru) : fabs(dirv));
	if (nless<0.0) nless=0;
	sp.N() = sp.N()*nless + sp.NU()*diru + sp.NV()*dirv;
	sp.N().normalize();
	if (sp.hasUV())
	{
		sp.u() = ou;
		sp.v() = ov;
	}
	sp.P() = old;
	sp.hasOrco(orco);
}



void ramp_blend(TEX_MODULATE type, colorA_t &rgb, CFLOAT fac, const colorA_t &col)
{
	switch (type) {
		case TMO_ADD:
			rgb += fac*col;
			break;
		case TMO_MUL:
			rgb *= (colorA_t(1.f-fac) + fac*col);
			break;
		case TMO_SCREEN: {
			colorA_t white(1.f);
			rgb = white-(colorA_t(1.f-fac) + (white - col))*(white - rgb);
			break;
		}
		case TMO_SUB:
			rgb -= fac*col;
			break;
		case TMO_DIV: {
			colorA_t icol(col);
			icol.invertRGB();
			rgb = (1.f-fac)*rgb + fac*rgb*icol;
			break;
		}
		case TMO_DIFF: {
			colorA_t dcol(rgb-col);
			dcol.absRGB();
			rgb = (1.f-fac)*rgb + fac*dcol;
			break;
		}
		case TMO_DARK: {
			colorA_t tmp(fac*col);
			tmp.darkenRGB(rgb);
			rgb = tmp;
			break;
		}
		case TMO_LIGHT: {
			colorA_t tmp(fac*col);
			tmp.lightenRGB(rgb);
			rgb = tmp;
			break;
		}
		default:
		case TMO_MIX:
			rgb = (1.f-fac)*rgb + fac*col;
	}
}

// result mode is for all accumulated lighting, can't be done in shader,
// or rather could possibly be done, but would make things totally and needlessly way too complex.
// energy mode can sort of be emulated, though results will still not be the same,
// because of the lighting model differences of Blender & Yafray.
// And the really big problem is GI, because everything is so divided up in yafray,
// only 'normal' will work properly with GI, for others, total change of colors will happen, not good...
#define do_ramp(rcol, cur_col, cur_ramp, brdf_res, cur_ramp_mode, cur_ramp_fac, cur_ramp_blend)\
{\
	CFLOAT rfac;\
	switch (cur_ramp_mode) {\
		case RMP_ENERGY:\
			rfac = 0.3f*ecol.getR() + 0.58f*ecol.getG() + 0.12f*ecol.getB();\
			break;\
		case RMP_NORMAL:\
			rfac = edir*N;\
			break;\
		default:\
		case RMP_SHADER:\
			rfac = brdf_res;\
	}\
	colorA_t cb = cur_ramp->stdoutColor(rfac, state, sp, eye);\
	rfac = cb.getA()*cur_ramp_fac;\
	colorA_t colt(cur_col);\
	ramp_blend(cur_ramp_blend, colt, rfac, cb);\
	rcol *= colt;\
	rcol.clampRGB0();\
}

color_t blenderShader_t::fromRadiosity(renderState_t &state, const surfacePoint_t &sp,
																		const energy_t &ene, const vector3d_t &eye) const
{
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), edir);
	if ((N*ene.dir)<0) return color_t(0.0);

	colorA_t col(scolor), spcol(speccol), cm(mircol);
	if (sp.hasVertexCol()) {
		if (mat_mode & MAT_VCOL_PAINT) col = sp.vertex_col();
	}

	// needed for raymir modulation, have to calculate fresnel here too, since diffuse affected by it
	CFLOAT fKr, fKt;
	if (use_fastf)
		fast_fresnel(edir, N, fastf_IOR, fKr, fKt);
	else
		fresnel(edir, N, IOR, fKr, fKt);
	CFLOAT raym = fresnelOfs + fKr;
	if (raym<0) raym=0; else if (raym>1) raym=1;
	raym *= reflect_amt;

	CFLOAT ref=edif, em=emit;
	if (!mods.empty())
	{
		CFLOAT stt=1, spa=specam, h=hard, al=alpha;
		for(vector<blenderModulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).blenderModulate(col, spcol, cm, ref, spa, h, em, al, raym, stt, state, sp, eye);
		}
	}
	if (ene.color.null()) return em*col;

	// ene.dir only really valid when called from photonlight in diffuse mode,
	// so brdf can't be evaluated here either, will never really be valid,
	// and since photonlights in diffuse mode are probably not even used anymore,
	// might just as well only enable for RMP_NORMAL mode as in getDIffuse()...
	colorA_t diff(ref);
	if (diffRamp && (dramp_mode==RMP_NORMAL)) {
		color_t ecol(ene.color);
		do_ramp(diff, col, diffRamp, 0.f, dramp_mode, dramp_factor, dramp_blend);
	}
	else diff *= col;
	// diffuse also affected by raymir modulation
	diff *= (1.f-raym);

	return (colorA_t)ene.color*diff + em*col;
}

color_t blenderShader_t::fromLight(renderState_t &state, const surfacePoint_t &sp,
																	const energy_t &energy, const vector3d_t &eye) const
{
	if (mat_mode & MAT_SHADELESS) return color_t(0.0);
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), edir);
	const CFLOAT inte = M_PI*(N*energy.dir);
	if (inte<=0) return color_t(0.0);

	colorA_t col(scolor), spcol(speccol), cm(mircol);
	if (sp.hasVertexCol()) {
		if (mat_mode & MAT_VCOL_PAINT) col = sp.vertex_col();
	}

	// needed for raymir modulation, have to calculate fresnel here too, since diffuse affected by it
	CFLOAT fKr, fKt;
	if (use_fastf)
		fast_fresnel(edir, N, fastf_IOR, fKr, fKt);
	else
		fresnel(edir, N, IOR, fKr, fKt);
	CFLOAT raym = fresnelOfs + fKr;
	if (raym<0) raym=0; else if (raym>1) raym=1;
	raym *= reflect_amt;

	CFLOAT ref=edif, spa=specam, h=hard, al=alpha, em=emit;
	if (!mods.empty())
	{
		CFLOAT stt=1;
		for(vector<blenderModulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).blenderModulate(col, spcol, cm, ref, spa, h, em, al, raym, stt, state, sp, eye);
		}
	}

	CFLOAT diffuse, specular;
	// brdf uses tangents when needed
	vector3d_t tu=sp.TU(), tv=sp.TV();
	if (aniso_angle!=0.f) {
		CFLOAT angle = aniso_angle*(CFLOAT)M_PI/180.f;
		tu = cos(angle)*tu + sin(angle)*tv;
		tv = tu VCROSS N;
	}
	
	diffuse = inte * diffBRDF->evaluate(edir, energy.dir, N, tu, tv);
	// 'hardness' modulation only used by Phong/BlenderCookTorr/BlenderBlinnn
	specular = inte * specBRDF->evaluate(edir, energy.dir, N, tu, tv, h);
	if (specular<0.f) specular=0.f;

	colorA_t diff(ref*diffuse);
	if (diffRamp && (dramp_mode!=RMP_RESULT)) {
		color_t ecol(energy.color * diffuse);
		do_ramp(diff, col, diffRamp, diffuse, dramp_mode, dramp_factor, dramp_blend);
	}
	else diff *= col;
	// diffuse also affected by raymir modulation
	diff *= (1.f-raym);

	colorA_t spec(spa*specular);
	if (specRamp && (sramp_mode!=RMP_RESULT)) {
		color_t ecol(energy.color * specular);
		do_ramp(spec, spcol, specRamp, specular, sramp_mode, sramp_factor, sramp_blend);
	}
	else spec *= spcol;

	// alpha modulation is separated in a fromLight() and fromWorld() part
	// positive alpha part here
	return (colorA_t)energy.color*(al*diff + spec);
}


color_t blenderShader_t::fromWorld(renderState_t &state, const surfacePoint_t &sp,
																const scene_t &s, const vector3d_t &eye) const
{
	if (environment) return environment->stdoutColor(state, sp, eye, &s);

	const void *oldorigin=state.skipelement;
	state.skipelement=sp.getOrigin();
	
	colorA_t Rresul(0.0), Tresul(0.0);
	vector3d_t edir = eye;
	edir.normalize();

	colorA_t col(scolor), cm(mircol);
	if (sp.hasVertexCol()) {
		if (mat_mode & MAT_VCOL_PAINT) col = sp.vertex_col();
	}

	CFLOAT al=alpha, em=emit, raym=reflect_amt;
	if (!mods.empty())
	{
		colorA_t spcol(speccol);
		CFLOAT spa=specam, h=hard, stt=1, ref=edif;
		for(vector<blenderModulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).blenderModulate(col, spcol, cm, ref, spa, h, em, al, raym, stt, state, sp, eye);
		}
	}

	// result color
	colorA_t rc(col);
	// emit, and alpha modulation here
	if ((mat_mode & MAT_SHADELESS)==0) rc *= em;
	// for alpha modulation, the color visible through this object
	if (mat_mode & MAT_ZTRANSP) rc = rc*al + (colorA_t)((1.f-al)*s.raytrace(state, sp.P(), -edir));

	if (sp.hasVertexCol()) {
		// vcol_paint has priority over vcol_light
		if ((mat_mode & (MAT_VCOL_LIGHT | MAT_VCOL_PAINT))==MAT_VCOL_LIGHT) rc += col*(colorA_t)sp.vertex_col();
	}

	if (do_refract || do_reflect)
	{
		CFLOAT &cont = state.contribution;
		bool &chroma = state.chromatic;
		PFLOAT &cur_ior = state.cur_ior;
		CFLOAT oldcont = cont;

		CFLOAT fKr, fKt;
		vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), edir);
		vector3d_t Ng = FACE_FORWARD(sp.Ng(), sp.Ng(), edir);
		if ((N*eye)<0) N=Ng;
		if (use_fastf)
			fast_fresnel(edir, N, fastf_IOR, fKr, fKt);
		else
			fresnel(edir, N, IOR, fKr, fKt);

		if (do_reflect) cm *= raym;	// if reflect, reflection color affected by raymir modulation (and alpha if no refraction)
		if (!do_refract) cm *= al;

		colorA_t cur_rfLcol(cm);
		if (do_reflect && !cur_rfLcol.null())
		{
			if(((sp.Ng()*eye)>0) || (state.raylevel<1))
			{
				vector3d_t ref = reflect(N, edir);

				PFLOAT offset = ref*Ng;
				if (offset<=0.05)
				{
					ref += Ng*(0.05-offset);
					ref.normalize();
				}

				CFLOAT nr = fresnelOfs+fKr;
				if (nr>1.0) nr=1.0;
				if ((nr*cont)>0.01)
				{
					cont *= nr;
					colorA_t nref = cur_rfLcol * nr;
					Rresul = nref*(colorA_t)s.raytrace(state, sp.P(), ref);
					cont = oldcont;
				}
			}
		}

		// in blender transmit color only affected by diffuse color depending on filter setting,
		// if filter is 0, color is always white, also affected by both alpha and raymirror
		col = ((1.f-filter)*colorA_t(1.0) + filter*col)*(1.f-al);
		colorA_t cur_rfRcol(col);
		// In Blender, refraction color is attenuated by the amount of reflection (transmitcolor*(1-raymirror)),
		// however, we still want to have transparency when fresnel fully controls reflection (raymir=1, frsofs=0)
		// so attenuate raymir itself as well with the fresnel offset factor
		col *= (1.f-(raym*fresnelOfs));
		if (do_refract && !cur_rfRcol.null())
		{
			vector3d_t ref;
			if (chroma && (dispersion_power>0.0)) { // && ((sp.N()*edir)<0)) {
				Tresul.black();
				colorA_t dispcol(1.0);
				CFLOAT ds_scale=1.f/(PFLOAT)dispersion_samples;
				for (int ds=0;ds<dispersion_samples;ds++) {
					PFLOAT djt = dispersion_jitter ? ourRandom() : 0.5;
					PFLOAT nior = getIORcolor((ds+djt)*ds_scale, CauchyA, CauchyB, dispcol);
					ref = refract(sp.N(), edir, nior);
					if (ref.null() && tir) ref = reflect(N, edir);
					if (!ref.null())
					{
						CFLOAT nt = (1.0<fKt) ? 1.0 : fKt;
						if ((nt*cont)>0.01)
						{
							cont *= nt;
							chroma = false;
							cur_ior = nior;
							Tresul += dispcol * nt * (colorA_t)s.raytrace(state, sp.P(), ref);
							cont = oldcont;
						}
					}
				}
				Tresul *= ds_scale*cur_rfRcol;
			}
			else {
				if (!chroma)
					ref = refract(sp.N(), edir, cur_ior);
				else
					ref = refract(sp.N(), edir, IOR);
				if (ref.null() && tir) ref = reflect(N, edir);
				if (!ref.null())
				{
					CFLOAT nt = (1.0<fKt) ? 1.0 : fKt;
					if ((nt*cont)>0.01)
					{
						cont *= nt;
						Tresul = cur_rfRcol * nt * (colorA_t)s.raytrace(state, sp.P(), ref);
						// absorption
						if ((!beer_sigma_a.null()) && (state.raylevel>0)) {
							color_t be(-sp.Z()*beer_sigma_a);
							be.set(exp(be.getR()), exp(be.getG()), exp(be.getB()));
							Tresul *= be;
						}
						cont = oldcont;
					}
				}
			}
		}
	}

	state.skipelement = oldorigin;
	return rc + (Tresul + Rresul);
}


const color_t blenderShader_t::getDiffuse(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye) const
{
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), edir);

	colorA_t col(scolor), spcol(speccol), cm(mircol);
	if (sp.hasVertexCol()) {
		if (mat_mode & MAT_VCOL_PAINT) col = sp.vertex_col();
	}

	// needed for raymir modulation, have to calculate fresnel here too, since diffuse affected by it
	CFLOAT fKr, fKt;
	if (use_fastf)
		fast_fresnel(edir, N, fastf_IOR, fKr, fKt);
	else
		fresnel(edir, N, IOR, fKr, fKt);
	CFLOAT raym = fresnelOfs + fKr;
	if (raym<0) raym=0; else if (raym>1) raym=1;
	raym *= reflect_amt;

	CFLOAT ref=edif, al=alpha;
	if (!mods.empty())
	{
		CFLOAT stt=1, spa=specam, h=hard, em=emit;
		for(vector<blenderModulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).blenderModulate(col, spcol, cm, ref, spa, h, em, al, raym, stt, state, sp, eye);
		}
	}

	// no lighting here, no brdf eval, only usable mode is RMP_NORMAL
	colorA_t diff(ref);
	if (diffRamp && (dramp_mode==RMP_NORMAL)) {
		color_t ecol(0.f);
		do_ramp(diff, col, diffRamp, 0.f, dramp_mode, dramp_factor, dramp_blend);
	}
	else diff *= col;

	// diffuse also affected by raymir modulation and alpha
	return al*diff*(1.f-raym);
}

void blenderShader_t::displace(renderState_t &state,surfacePoint_t &sp, const vector3d_t &eye, PFLOAT res) const
{
	if (!mods.empty())
	{
		for(vector<blenderModulator_t>::const_iterator ite=mods.begin();ite!=mods.end();++ite)
		{
			(*ite).blenderDisplace(state, sp, eye, res*state.traveled);
		}
	}
}

shader_t * blenderShader_t::factory(paramMap_t &bparams, std::list<paramMap_t> &lmod,
				renderEnvironment_t &render)
{
	color_t color(0.0), specular(0.0), mirror(0.0);
	CFLOAT difref=1, specam=0, hard=1, alpha=1, emit=0, ior=1, frsofs=0, filter=0;
	bool fast_fresnel=false,tir=false;
	string _matmodes;
	const string *matmodes = &_matmodes;

	// dispersion parameters
	CFLOAT disp_pw = 0;
	int disp_sam = 0;
	bool disp_jit = false;
	color_t beer(0.0);
	bparams.getParam("dispersion_power", disp_pw);
	bparams.getParam("dispersion_samples", disp_sam);
	bparams.getParam("dispersion_jitter", disp_jit);

	// transmit absorption coefficient
	bparams.getParam("absorption", beer);

	// to make blendershader as compatible with blendershader as much as possible,
	// it is now not an extension of generic anymore, so reflected/transmitted removed.
	// 'reflected' now mirror_color, and 'transmitted' is the same as diffuse 'color'
	// 'min_refle' now is fresnel_offset.
	// Update: since this version wasn't released yet and Blender was already updated to use the new syntax,
	// this caused confusion and concern that yafray didn't work anymore since it was used with the old yafray.
	// For backward compatibility both 'reflected' & 'transmitted' are exported by Blender, so to avoid more confusion
	// when the new version is released, silently accept the parameters without warnings, they are not used though.
	color_t nada1;
	bparams.getParam("reflected", nada1);
	bparams.getParam("transmitted", nada1);
	CFLOAT nada2;
	bparams.getParam("min_refle", nada2);

	// reflect/refract switches
	bool rfL=false, rfR=false;
	bparams.getParam("reflect", rfL);
	bparams.getParam("refract", rfR);

	bparams.getParam("color", color);
	bparams.getParam("specular_color", specular);
	bparams.getParam("mirror_color", mirror);
	bparams.getParam("diffuse_reflect", difref);
	bparams.getParam("specular_amount", specam);
	bparams.getParam("hard", hard);
	bparams.getParam("alpha", alpha);
	bparams.getParam("emit", emit);

	bparams.getParam("IOR", ior);
	// new transmit filter param.
	bparams.getParam("transmit_filter", filter);
	bparams.getParam("fresnel_offset", frsofs);
	bparams.getParam("fast_fresnel", fast_fresnel);
	bparams.getParam("tir", tir);
	bparams.getParam("matmodes", matmodes);

	// removed reflected2/transmitted2, never has been used, and not controllable from Blender anyway..
	color_t refl2(0.0), trns2(0.0);
	if (bparams.getParam("reflected2", refl2) || bparams.getParam("transmitted2", trns2))
	{
		cerr << "What? Someone actually uses reflected2/transmitted2 in the Blendershader?\n"
				<< "Well, too bad, they're gone... Try the generic shader instead, sorry..." << endl;
	}

	// new reflect_amount parameter, easier to control reflection modulation like in Blender that way
	CFLOAT refl_am = 0;
	bparams.getParam("reflect_amount", refl_am);

	// diffuse & specular brdf, all just quick hacks for now,
	// only parameters for the ones supported in Blender (plus basic aniso for the xml hackers)
	string _diffname, _specname;
	const string *diffname=&_diffname, *specname=&_specname;
	bparams.getParam("diffuse_brdf", diffname);
	bparams.getParam("specular_brdf", specname);
	// common parameters
	// specular exponent (phong, blinn, blendercooktorr) from hard param
	CFLOAT sigma=0;	// OrenNayar 'roughness'
	CFLOAT bl_eta=4;	// blenderblinn refraction index, of course should really be material ior itself...
	CFLOAT td_size=1.5, td_smooth=1.5, td_edge=0.0;	// toon diffuse
	CFLOAT ts_size=1.5, ts_smooth=1.5;	// toon specular
	CFLOAT urough=0.1, vrough=0.1;	//aniso (iso def)
	// Minnaert darkening limb, also affects model choice
	// when >=1, nvidia model is used
	CFLOAT dark=1.0;
	bparams.getParam("roughness", sigma);
	bparams.getParam("blinn_ior", bl_eta);
	bparams.getParam("toondiffuse_size", td_size);
	bparams.getParam("toondiffuse_smooth", td_smooth);
	bparams.getParam("toondiffuse_edge", td_edge);
	bparams.getParam("toonspecular_size", ts_size);
	bparams.getParam("toonspecular_smooth", ts_smooth);
	bparams.getParam("u_roughness", urough);
	bparams.getParam("v_roughness", vrough);
	bparams.getParam("darkening", dark);
	brdf_t *dbrdf=NULL, *sbrdf=NULL;
	// diffuse, lambert default
	if (*diffname=="oren_nayar")
		dbrdf = new OrenNayar_t(1.0, sigma);
	else if (*diffname=="toon")
		dbrdf = new simpleToonDiffuse_t(1.0, td_size, td_smooth, td_edge);
	else if (*diffname=="ashikhmin")
		dbrdf = new AshikhminDiffuse_t(1.0, specam);	// depends on specular amount
	else if (*diffname=="minnaert")
		dbrdf = new Minnaert_t(1.0, dark);
	else
		dbrdf = new Lambert_t(1.0);
	// specular, phong(blinn) default
	if (*specname=="blinn")
		sbrdf = new BlenderBlinn_t(1.0, hard, bl_eta);
	else if (*specname=="toon")
		sbrdf = new simpleToonSpecular_t(1.0, ts_size, ts_smooth);
	else if (*specname=="blender_cooktorr")
		sbrdf = new BlenderCookTorr_t(1.0, hard);
	else if (*specname=="ward")
		sbrdf = new Ward_t(1.0, urough, vrough);
	else if (*specname=="ashikhmin")
		sbrdf = new AshikhminSpecular_t(1.0, urough, vrough);
	else
		sbrdf = new Phong_t(1.0, hard, Phong_t::ORIGINAL, Phong_t::HALFWAY);

	CFLOAT aniso_ang = 0;
	bparams.getParam("anisotropic_angle", aniso_ang);
	
	// ramps
	string _ramp1_name="", _ramp2_name="";
	const string *ramp1_name=&_ramp1_name, *ramp2_name=&_ramp2_name;
	bparams.getParam("diffuse_ramp", ramp1_name);
	shader_t* ramp1 = render.getShader(*ramp1_name);
	bparams.getParam("specular_ramp", ramp2_name);
	shader_t* ramp2 = render.getShader(*ramp2_name);
	// ramp blend mode
	string _ramp1_blend, _ramp2_blend;
	const string *ramp1_blend=&_ramp1_blend, *ramp2_blend=&_ramp2_blend;
	bparams.getParam("diffuse_ramp_blend", ramp1_blend);
	bparams.getParam("specular_ramp_blend", ramp2_blend);
	TEX_MODULATE r1blend = string2texmode(*ramp1_blend);
	TEX_MODULATE r2blend = string2texmode(*ramp2_blend);
	// ramp input modes
	string _ramp1_mode="shader", _ramp2_mode="shader";
	const string *ramp1_mode=&_ramp1_mode, *ramp2_mode=&_ramp2_mode;
	bparams.getParam("diffuse_ramp_mode", ramp1_mode);
	bparams.getParam("specular_ramp_mode", ramp2_mode);
	// mix factors
	CFLOAT ramp1F=1, ramp2F=1;
	bparams.getParam("diffuse_ramp_factor", ramp1F);
	bparams.getParam("specular_ramp_factor", ramp2F);

	// environment shader, for use with other blocks, used in fromWorld()
	string _env_name="";
	const string *env_name=&_env_name;
	bparams.getParam("environment", env_name);
	shader_t* env = render.getShader(*env_name);

	blenderShader_t *ns = new blenderShader_t(ramp1, ramp2, env,
	                                          r1blend, r2blend, *ramp1_mode, *ramp2_mode, ramp1F, ramp2F,
	                                          color, specular, mirror, difref, specam, hard, alpha, emit,
	                                          refl_am, dbrdf, sbrdf, aniso_ang, frsofs, ior, filter,
	                                          fast_fresnel, tir, rfL, rfR, disp_pw, disp_sam, disp_jit, beer);
	ns->setMode(*matmodes);

	for(list<paramMap_t>::iterator i=lmod.begin();i!=lmod.end();++i)
	{
		CFLOAT	displace=0, trans=0, refle=0;
		int color=0,  csp=0, cmir=0, ref=0,
				specular=0, hard=0, alpha=0, emit=0, raymir=0;
		CFLOAT colfac=1, varfac=1, def_var=1;
		color_t texcol(1.0, 0.0, 1.0);	//blender default purple
		color_t filtercol(1.0, 1.0, 1.0);	// texture filter color
		CFLOAT bri=1, con=1;	// texture brightness & contrast

		string _mode="mix";
		string _aflag="";	// alpha flag, can contain calc_alpha, use_alpha and/or neg_alpha
		string _texflag = "";
		string _shname;
		const string *mode=&_mode;
		const string *aflag=&_aflag;	// alpha flag, can contain calc_alpha, use_alpha and/or neg_alpha
		const string *texflag = &_texflag;
		const string *shname= &_shname;
		bool nmap = false;	// rgb as normal
		
		paramMap_t &params=*i;

		params.getParam("input", shname);
		shader_t *input = render.getShader(*shname);
		if (input==NULL)
		{
			cerr << "Undefined block : " << *shname << endl;
			continue;
		}

		params.getParam("color", color);
		params.getParam("normal", displace);
		params.getParam("colspec", csp);
		params.getParam("colmir", cmir);
		params.getParam("difref", ref);
		params.getParam("specular", specular);
		params.getParam("hard", hard);
		params.getParam("alpha", alpha);
		params.getParam("emit", emit);
		params.getParam("raymir", raymir);

		params.getParam("transmission", trans);
		params.getParam("reflection", refle);

		params.getParam("mode", mode);

		// string with possible multiple keywords: stencil/negative/no_rgb
		params.getParam("texflag", texflag);

		// the variation parameters
		params.getParam("colfac", colfac);
		params.getParam("def_var", def_var);
		params.getParam("varfac", varfac);

		// blender texture color
		params.getParam("texcol", texcol);

		params.getParam("filtercolor", filtercol);
		params.getParam("contrast", con);
		params.getParam("brightness", bri);

		params.getParam("alpha_flag", aflag);

		params.getParam("normalmap", nmap);

		blenderModulator_t modu(input);
		modu.string2modetype(*mode);

		modu.color(color);
		modu.displace(displace);
		modu.colspec(csp);
		modu.cmir(cmir);
		modu.difref(ref);
		modu.specular(specular);
		modu.hard(hard);
		modu.alpha(alpha);
		modu.emit(emit);
		modu.raymir(raymir);

		modu.transmision(trans);
		modu.reflection(refle);

		modu.setColFac(colfac);
		modu.setDVar(def_var);
		modu.setVarFac(varfac);
		modu.setTexFlag(*texflag);
		modu.setTexCol(texcol);

		modu.setFilterCol(filtercol);
		modu.setContrast(con);
		modu.setBrightness(bri);

		modu.setAlphaFlag(*aflag);

		modu.setNormap(nmap);

		ns->addModulator(modu);

		params.checkUnused("modulator");
	}
	return ns;
}


extern "C"
{

YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("blendermapper", blenderMapperNode_t::factory);
	render.registerFactory("blendershader", blenderShader_t::factory);
	std::cout << "Registered blendershaders\n";
}

}
__END_YAFRAY

