#ifndef __BASICBLOCKS_H
#define __BASICBLOCKS_H

#include "metashader.h"
#include "basictex.h"
#include "params.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

__BEGIN_YAFRAY

class floatToColor_t : public shaderNode_t
{
	public:
		floatToColor_t(const shader_t *in):input(in) {};
		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene=NULL)const;
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		const shader_t *input;
};

class colorToFloat_t : public shaderNode_t
{
	public:
		colorToFloat_t(const shader_t *in):input(in) {};
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene=NULL)const
		{return input->stdoutColor(state,sp,eye,scene).energy();};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		const shader_t *input;
};


class cloudsNode_t : public shaderNode_t
{
	public:
		cloudsNode_t(PFLOAT s, int dep, bool hd, int ct,
			const shader_t *in1, const shader_t *in2,
			const std::string &ntype, const std::string &btype);
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (((input1!=NULL) && (input2!=NULL)) || (ctype==1)); }
		virtual ~cloudsNode_t() {};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		textureClouds_t tex;
		int ctype;
		const shader_t *input1, *input2;
};


class marbleNode_t : public shaderNode_t
{
	public:
		marbleNode_t(PFLOAT sz, int dep, CFLOAT turb, CFLOAT shp, bool hrd,
				const shader_t *in1, const shader_t *in2,
				const std::string &ntype, const std::string &shape);
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (input1!=NULL) && (input2!=NULL); }
		virtual ~marbleNode_t() {};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		textureMarble_t tex;
		const shader_t *input1, *input2;
};


class woodNode_t : public shaderNode_t
{
	public:
		woodNode_t(PFLOAT sz, int dep, CFLOAT turb, bool hrd,
				const shader_t *in1, const shader_t *in2,
				const std::string &ntype, const std::string &wtype, const std::string &shape);
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye,
				const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (input1!=NULL) && (input2!=NULL); }
		virtual ~woodNode_t() {};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		textureWood_t tex;
		const shader_t *input1, *input2;
};


class colorBandNode_t : public shaderNode_t
{
	public:
		colorBandNode_t(std::vector<std::pair<CFLOAT,colorA_t> > &b, const shader_t *in):
			band(b), input(in) {}
		virtual ~colorBandNode_t() {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(CFLOAT x, renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;

		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		std::vector<std::pair<CFLOAT,colorA_t> > band;
		const shader_t *input;
};


class coordsNode_t : public shaderNode_t
{
	public:
		coordsNode_t(int c):coord(c) {};
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene=NULL)const
		{
			switch(coord)
			{
				case 0: return sp.P().x;
				case 1: return sp.P().y;
				default: return sp.P().z;
			}
		};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);

	protected:
		int coord;
};

class mulNode_t : public shaderNode_t
{
	public:
		mulNode_t(const shader_t *in1,const shader_t *in2,CFLOAT v):
			input1(in1),input2(in2),value(v) {};
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene=NULL)const
		{
			CFLOAT res=value;
			if(input1!=NULL) res*=input1->stdoutFloat(state,sp,eye,scene);
			if(input2!=NULL) res*=input2->stdoutFloat(state,sp,eye,scene);
			return res;
		};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		const shader_t *input1,*input2;
		CFLOAT value;
};

class sinNode_t : public shaderNode_t
{
	public:
		sinNode_t(const shader_t *in):
			input(in) {};
		virtual CFLOAT stdoutFloat(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene=NULL)const
		{
			return 0.5*sin(input->stdoutFloat(state,sp,eye,scene))+0.5;
		};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		const shader_t *input;
};

class phongNode_t : public shader_t
{
	public:
		phongNode_t(const shader_t *c, const shader_t *s, const shader_t *e,
								const shader_t *cr, const shader_t *ct, const shader_t *bp,
								CFLOAT h, PFLOAT ior, CFLOAT d)
						:color(c), specular(s), env(e), caus_rcolor(cr), caus_tcolor(ct), bump(bp),
						hard(h), IOR(ior), _displace(d) {}
		/// @see shader_t
		virtual color_t fromRadiosity(renderState_t &state,const surfacePoint_t &sp,const energy_t &ene,
															const vector3d_t &eye)const;
		/// @see shader_t
		virtual color_t fromLight(renderState_t &state,const surfacePoint_t &sp,const energy_t &energy,
															const vector3d_t &eye)const;
		/// @see shader_t
		virtual color_t fromWorld(renderState_t &state,const surfacePoint_t &sp,const scene_t &scene,
															const vector3d_t &eye)const;
		/// @see shader_t
		virtual const color_t getDiffuse(renderState_t &state,
							const surfacePoint_t &sp, const vector3d_t &eye)const;

		virtual bool getCaustics(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye,
														color_t &ref, color_t &trans, PFLOAT &ior) const;

		virtual void displace(renderState_t &state, surfacePoint_t &sp, const vector3d_t &eye, PFLOAT res) const;

		/// Destructor
		virtual ~phongNode_t() {};

		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		const shader_t *color, *specular, *env, *caus_rcolor, *caus_tcolor, *bump;
		CFLOAT hard;
		PFLOAT IOR;
		CFLOAT _displace;
};

class rgbNode_t : public shaderNode_t
{
	public:
		rgbNode_t(const shader_t *in1, const shader_t *in2, const shader_t *in3, 
				const color_t &c);

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const;
		virtual ~rgbNode_t() {};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);

	protected:
		const shader_t *inputred;
		const shader_t *inputgreen;
		const shader_t *inputblue;
		color_t color;
};

class hsvNode_t : public shaderNode_t
{
	public:
		hsvNode_t(const shader_t *in1, const shader_t *in2, const shader_t *in3, 
				CFLOAT h, CFLOAT s, CFLOAT v);

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const;
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);

	protected:
		const shader_t *inputhue;
		const shader_t *inputsaturation;
		const shader_t *inputvalue;

		CFLOAT hue, saturation, value;
};

class coneTraceNode_t : public shaderNode_t
{
	public:
		coneTraceNode_t(const color_t &c,PFLOAT angle,int s,PFLOAT ior,bool r);

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const;

		virtual ~coneTraceNode_t() {};
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);
	protected:
		bool ref;
		color_t color;
		PFLOAT cosa, IOR, sqrdiv, exponent;
		CFLOAT div;
		int samples,sqr;
};

class fresnelNode_t : public shaderNode_t
{
	public:
		fresnelNode_t(const shader_t *r,const shader_t *t,PFLOAT ior,CFLOAT minr):trans(t),
			ref(r),minref(minr)
			{
				IOR = (ior - 1.0) / (ior + 1.0);
				IOR *= IOR;
			};

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const;
		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);

	protected:
			const shader_t *trans,*ref;
			PFLOAT IOR;
			CFLOAT minref;
};

class imageNode_t : public shaderNode_t
{
	public:
		imageNode_t(const char *filename, const std::string &intp):tex(filename, intp) {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
			const vector3d_t &eye, const scene_t *scene=NULL) const
		{
			return tex.getFloat(sp.P());
		}
		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,
			const vector3d_t &eye, const scene_t *scene=NULL) const
		{
			return tex.getColor(sp.P());
		}
		virtual bool discrete() const { return true; }
		virtual ~imageNode_t() {}
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		textureImage_t tex;
};

class goboNode_t : public shaderNode_t
{
	public:
		goboNode_t(const shader_t *in1, const shader_t *in2, const shader_t *inGoboColor, 
			const shader_t *inGoboFloat,const bool inHardEdge,const CFLOAT inEdgeVal ):
			input1(in1),input2(in2),goboColor(inGoboColor),goboFloat(inGoboFloat),
			hardEdge(inHardEdge),edgeVal(inEdgeVal){};
		virtual ~goboNode_t() {};

		virtual colorA_t stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const;

		static shader_t * factory(paramMap_t &,std::list<paramMap_t> &,
				        renderEnvironment_t &);

	protected:
		const shader_t *input1;
		const shader_t *input2;
		const shader_t *goboColor;
		const shader_t *goboFloat;
		bool hardEdge;
		CFLOAT edgeVal;
};

class voronoiNode_t : public shaderNode_t
{
	public:
		voronoiNode_t(const shader_t *in1, const shader_t *in2,
				int ct,
				CFLOAT w1, CFLOAT w2, CFLOAT w3, CFLOAT w4,
				PFLOAT mex, PFLOAT size,
				CFLOAT isc, const std::string &dname);
		virtual ~voronoiNode_t() {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (((input1!=NULL) && (input2!=NULL)) || iscolor); }
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		textureVoronoi_t tex;
		bool iscolor;
		const shader_t *input1, *input2;
};

class musgraveNode_t : public shaderNode_t
{
	public:
		musgraveNode_t(const shader_t *in1, const shader_t *in2,
				PFLOAT H, PFLOAT octs, PFLOAT lacu, PFLOAT offs, PFLOAT gain,
				PFLOAT size, CFLOAT iscale, const std::string &ntype, const std::string &mtype);
		virtual ~musgraveNode_t() {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (input1!=NULL) && (input2!=NULL); }
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		textureMusgrave_t tex;
		const shader_t *input1, *input2;
};

class distortedNoiseNode_t : public shaderNode_t
{
	public:
		distortedNoiseNode_t(const shader_t *in1, const shader_t *in2,
				PFLOAT distort, PFLOAT size,
				const std::string &ntype1, const std::string &ntype2);
		virtual ~distortedNoiseNode_t() {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (input1!=NULL) && (input2!=NULL); }
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		textureDistortedNoise_t tex;
		const shader_t *input1, *input2;
};

class gradientNode_t : public shaderNode_t
{
	public:
		gradientNode_t(const shader_t *in1, const shader_t *in2,
				const std::string &gtype, bool fxy);
		virtual ~gradientNode_t() {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (input1!=NULL) && (input2!=NULL); }
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		textureGradient_t tex;
		const shader_t *input1, *input2;
};

class randomNoiseNode_t : public shaderNode_t
{
	public:
		randomNoiseNode_t(const shader_t *in1, const shader_t *in2, int depth);
		virtual ~randomNoiseNode_t() {}
		virtual CFLOAT stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual colorA_t stdoutColor(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene=NULL) const;
		virtual bool isRGB() const { return (input1!=NULL) && (input2!=NULL); }
		static shader_t * factory(paramMap_t &, std::list<paramMap_t> &, renderEnvironment_t &);
	protected:
		textureRandomNoise_t tex;
		const shader_t *input1, *input2;
};

__END_YAFRAY
#endif
