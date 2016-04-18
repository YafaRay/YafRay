#ifndef __BLENDERSHADER_H
#define __BLENDERSHADER_H

#include "metashader.h"
#include "params.h"
#include "brdf.h"
#include "spectrum.h"

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

__BEGIN_YAFRAY

class blenderMapperNode_t : public shaderNode_t
{
	public:
		blenderMapperNode_t(const shader_t *m);
		virtual ~blenderMapperNode_t() {};

		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual bool discrete()const {return mapped->discrete();};
		virtual bool isRGB() const { return mapped->isRGB(); }
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);

	protected:

		bool doMapping(const surfacePoint_t &sp, const vector3d_t &eye,point3d_t &texpt) const;
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
		void setCrop(GFLOAT minx, GFLOAT miny, GFLOAT maxx, GFLOAT maxy)
		{ _cropminx=minx;  _cropminy=miny;  _cropmaxx=maxx;  _cropmaxy=maxy; }

		// repeat
		void setRepeat(int x, int y) { _xrepeat=x;  _yrepeat=y; }

		void string2maptype(const std::string &mapname);

		void string2texcotype(const std::string &texconame);

		void string2cliptype(const std::string &clipname);
		void string2texprojection(const std::string &x_axis, const std::string &y_axis, const std::string &z_axis);

		void setRot90(bool r90) { _rot90=r90; }

		void setChecker(const std::string &chm, GFLOAT chd)
		{
			if (int(chm.find("odd"))!=-1) checker_odd = true;
			if (int(chm.find("even"))!=-1) checker_even = true;
			checker_dist = chd;
		}

		const shader_t *mapped;
		GFLOAT _sizex, _sizey, _sizez;	// texture scale factors
		int _mode;
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
		bool _rot90, checker_odd, checker_even;
		GFLOAT checker_dist;
};

// alpha flag
#define TXA_CALCALPHA	1
#define TXA_USEALPHA	2
#define TXA_NEGALPHA	4

class blenderModulator_t : public modulator_t
{
	public :
		blenderModulator_t(const shader_t *sh):modulator_t(NULL)
		{
			input = sh;
			texflag = 0;
			texture_col = color_t(0.0);
			_csp = _cmir = _ref = _alpha = _emit = _raymir = 0;
			_contrast = _brightness = 1.0;
			alpha_flag = 0;
			rgbnormap = false;
		};
		~blenderModulator_t() {};

		void colspec(int cs) { _csp=cs; }
		void cmir(int cm) { _cmir=cm; }
		void difref(int dr) { _ref=dr; }
		void alpha(int al) { _alpha=al; }
		void emit(int em) { _emit=em; }
		void raymir(int rm) { _raymir=rm; }

		void blenderDisplace(renderState_t &state, surfacePoint_t &sp,
				const vector3d_t &eye, PFLOAT res) const;
		void blenderModulate(colorA_t &col, colorA_t &colspec, colorA_t &colmir, CFLOAT &ref,
				CFLOAT &spec, CFLOAT &har, CFLOAT &emit, CFLOAT &alpha, CFLOAT &raymir,
				CFLOAT &stencilTin, renderState_t &state,const surfacePoint_t &sp,
				const vector3d_t &eye) const;

		void setColFac(CFLOAT cf) { colfac = cf; }
		void setDVar(CFLOAT dv) { def_var = dv; }
		void setVarFac(CFLOAT vf) { varfac = vf; }
		void setTexFlag(const std::string &st)
		{
			// string can contain multiple 'flags'
			if (int(st.find("stencil"))!=-1) texflag |= TXF_STENCIL;
			if (int(st.find("negative"))!=-1) texflag |= TXF_NEGATIVE;
			if (int(st.find("no_rgb"))!=-1) texflag |= TXF_RGBTOINT;
		}
		void setTexCol(const color_t &tc) { texture_col = tc; }

		void setFilterCol(const color_t &fc) { _filtercolor = fc; }
		void setContrast(CFLOAT c) { _contrast = c; }
		void setBrightness(CFLOAT b) { _brightness = b; }
		void setAlphaFlag(const std::string &st)
		{
			if (int(st.find("calc_alpha"))!=-1) alpha_flag |= TXA_CALCALPHA;
			if (int(st.find("use_alpha"))!=-1) alpha_flag |= TXA_USEALPHA;
			if (int(st.find("neg_alpha"))!=-1) alpha_flag |= TXA_NEGALPHA;
		}
		// use texture as normalmap
		void setNormap(bool nmap) { rgbnormap = nmap; }

	protected:

		const shader_t *input;
		// blender modulation
		unsigned char texflag;	//bitmask
		CFLOAT colfac, def_var, varfac;
		colorA_t texture_col;
		char _csp, _cmir, _ref, _alpha, _emit, _raymir;
		// texture brightness, contrast & color adjustment
		colorA_t _filtercolor;
		CFLOAT _contrast, _brightness;
		char alpha_flag;
		bool rgbnormap;
};

// material modes
#define MAT_TRACEABLE		1
#define MAT_SHADOW			2
#define MAT_SHADELESS		4
#define MAT_VCOL_LIGHT	8
#define MAT_VCOL_PAINT	16
#define MAT_ZTRANSP			32
#define MAT_ONLYSHADOW	64

// the blendershader
class blenderShader_t : public shader_t
{
	public:
		enum RMP_MODE {RMP_SHADER, RMP_ENERGY, RMP_NORMAL, RMP_RESULT};
		blenderShader_t(const shader_t* ramp1, const shader_t* ramp2, const shader_t* env,
										TEX_MODULATE tm_r1, TEX_MODULATE tm_r2,
										const std::string &md_r1, const std::string &md_r2,
										CFLOAT r1f, CFLOAT r2f,
										const color_t &color, const color_t &spc, const color_t &mirc,
										CFLOAT dr, CFLOAT spam, CFLOAT h, CFLOAT al, CFLOAT em,
										CFLOAT refl_am, brdf_t* dbrdf, brdf_t* sbrdf,
										CFLOAT aniso_ang,
										CFLOAT frsofs=0, PFLOAT _IOR=1.5, CFLOAT filt=0,
										bool fastf=false, bool _tir=false, bool rfL=false, bool rfR=false,
										CFLOAT disp_pw=0, int disp_sam=0, bool disp_jit=false, color_t beer=color_t(0.0))
		{
			// ramps
			diffRamp = ramp1;
			specRamp = ramp2;
			dramp_blend = tm_r1;
			sramp_blend = tm_r2;
			dramp_mode = RMP_SHADER;
			if (md_r1=="energy")
				dramp_mode = RMP_ENERGY;
			else if (md_r1=="normal")
				dramp_mode = RMP_NORMAL;
			else if (md_r1=="result")
				dramp_mode = RMP_RESULT;
			sramp_mode = RMP_SHADER;
			if (md_r2=="energy")
				sramp_mode = RMP_ENERGY;
			else if (md_r2=="normal")
				sramp_mode = RMP_NORMAL;
			else if (md_r2=="result")
				sramp_mode = RMP_RESULT;
			dramp_factor = r1f;
			sramp_factor = r2f;
			// anisotropic direction rotation angle
			aniso_angle = aniso_ang;
			//----------------
			// environment shader
			environment = env;
			scolor = color;
			speccol = spc;
			mircol = mirc;
			edif = dr;
			specam = spam;
			hard = h;
			alpha = al;
			emit = em;
			reflect_amt = refl_am;
			fresnelOfs = frsofs;
			IOR = _IOR;
			filter = filt;
			do_reflect = rfL;
			do_refract = rfR;
			// for fast_fresnel()
			fastf_IOR = (IOR - 1.0) / (IOR + 1.0);
			fastf_IOR *= fastf_IOR;
			mat_mode=0;
			use_fastf = fastf;
			tir = _tir;
			// brdf's
			diffBRDF = dbrdf;
			specBRDF = sbrdf;
			// dispersion
			dispersion_power = dispersion_samples = 0;
			if ((disp_pw>0) && (disp_sam>0)) {
				dispersion_power = disp_pw;
				dispersion_samples = disp_sam;
				dispersion_jitter = disp_jit;
			}
			beer_sigma_a = beer;
			CauchyCoefficients(IOR, disp_pw, CauchyA, CauchyB);
		};
		virtual color_t fromRadiosity(renderState_t &state, const surfacePoint_t &sp, const energy_t &ene,
															const vector3d_t &eye) const;
		virtual color_t fromLight(renderState_t &state, const surfacePoint_t &sp, const energy_t &energy,
															const vector3d_t &eye) const;
		virtual color_t fromWorld(renderState_t &state, const surfacePoint_t &sp, const scene_t &scene,
															const vector3d_t &eye) const;
		virtual const color_t getDiffuse(renderState_t &state,
									const surfacePoint_t &sp, const vector3d_t &eye) const;
		virtual ~blenderShader_t()
		{
			if (diffBRDF) {
				delete diffBRDF;
				diffBRDF = NULL;
			}
			if (specBRDF) {
				delete specBRDF;
				specBRDF = NULL;
			}
		}

		virtual void displace(renderState_t &state,surfacePoint_t &sp,
				const vector3d_t &eye, PFLOAT res) const;

		virtual void getDispersion(PFLOAT &disp_pw, PFLOAT &A, PFLOAT &B, color_t &beer) const
		{
			disp_pw = dispersion_power;
			beer = beer_sigma_a;
			A = CauchyA;
			B = CauchyB;
		}
		virtual bool getCaustics(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye,
														color_t &ref, color_t &trans, PFLOAT &ior) const
		{
			if (do_reflect) ref=mircol; else ref.set(0,0,0);
			if (do_refract) trans=scolor; else trans.set(0,0,0);
			ior = IOR;
			return ((do_reflect && (!trans.null())) | (do_refract && (!ref.null())));
		}

		void setDiffuseBRDF(brdf_t* db);
		void setSpecularBRDF(brdf_t* sb);
		
		void addModulator(const blenderModulator_t &mod) { mods.push_back(mod); }

		void setMode(const std::string &mst)
		{
			if (int(mst.find("traceable"))!=-1) mat_mode |= MAT_TRACEABLE;
			if (int(mst.find("shadow"))!=-1) mat_mode |= MAT_SHADOW;
			if (int(mst.find("shadeless"))!=-1) mat_mode |= MAT_SHADELESS;
			if (int(mst.find("vcol_light"))!=-1) mat_mode |= MAT_VCOL_LIGHT;
			if (int(mst.find("vcol_paint"))!=-1) mat_mode |= MAT_VCOL_PAINT;
			if (int(mst.find("ztransp"))!=-1) mat_mode |= MAT_ZTRANSP;
			if (int(mst.find("onlyshadow"))!=-1) mat_mode |= MAT_ONLYSHADOW;
		}

		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);

	protected:
		// ramp shaders
		const shader_t *diffRamp, *specRamp;
		/// surface diffuse/specular/mirror color
		/// mirror_color replaces the old separate reflect.col.
		/// and scolor also now used for transmit color
		colorA_t scolor, speccol, mircol;
		/// diffuse/specular reflection amount
		CFLOAT edif, specam;
		/// alpha and emit amount
		CFLOAT alpha, emit;
		/// controls reflection amount
		CFLOAT reflect_amt;
		/// hardness, fresnel offset, transmit filter
		CFLOAT hard, fresnelOfs, filter;
		/// index of refraction.
		PFLOAT IOR;
		/// IOR precalculated for fast fresnel function
		PFLOAT fastf_IOR;
		bool use_fastf, tir, do_reflect, do_refract;
		/// The sequence of modulators
		std::vector<blenderModulator_t> mods;
		// Blender material modes
		unsigned short mat_mode;
		// diffuse & specular brdf's
		brdf_t* diffBRDF;
		brdf_t* specBRDF;
		// anisotropic direction rotation angle
		CFLOAT aniso_angle;
		// for dispersion
		PFLOAT dispersion_power, CauchyA, CauchyB;
		int dispersion_samples;
		bool dispersion_jitter;
		color_t beer_sigma_a;
		// for ramps
		CFLOAT dramp_factor, sramp_factor;
		TEX_MODULATE dramp_blend, sramp_blend;
		RMP_MODE dramp_mode, sramp_mode;
		// environment shader, for use with other shaders to affect reflect/refract
		const shader_t *environment;
};

__END_YAFRAY

#endif
