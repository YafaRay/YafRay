#include "basicblocks.h"

using namespace std;

__BEGIN_YAFRAY

colorA_t floatToColor_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye,const scene_t *scene)const
{
	CFLOAT f=input->stdoutFloat(state,sp,eye,scene);
	return colorA_t(f,f,f);
}

shader_t * floatToColor_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _inputs;
	const string *inputs=&_inputs;
	shader_t *input=NULL;
	bparams.getParam("input",inputs);
	input=render.getShader(*inputs);
	if(input!=NULL)
		return new floatToColor_t(input);
	else return NULL;
}

shader_t * colorToFloat_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _inputs;
	const string *inputs=&_inputs;
	shader_t *input=NULL;
	bparams.getParam("input",inputs);
	input=render.getShader(*inputs);
	if(input!=NULL)
		return new colorToFloat_t(input);
	else return NULL;
}

shader_t * sinNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _inputs;
	const string *inputs=&_inputs;
	shader_t *input=NULL;
	bparams.getParam("input",inputs);
	input=render.getShader(*inputs);
	if(input!=NULL)
		return new sinNode_t(input);
	else return NULL;
}

shader_t * mulNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _in1,_in2;
	const string *in1=&_in1,*in2=&_in2;
	shader_t *input1=NULL,*input2=NULL;
	CFLOAT val=1.0;
	bparams.getParam("input1",in1);
	bparams.getParam("input2",in2);
	bparams.getParam("value",val);
	input1=render.getShader(*in1);
	input2=render.getShader(*in2);
	return new mulNode_t(input1,input2,val);
}

shader_t * coordsNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	int w=0;
	string _var;
	const string *var=&_var;
	bparams.getParam("coord",var);
	if(*var=="X") w=0;
	if(*var=="Y") w=1;
	if(*var=="Z") w=2;
	return new coordsNode_t(w);
}

//-------------------------------------------------------------------------
// CLOUDS

cloudsNode_t::cloudsNode_t(PFLOAT s, int dep, bool hd, int ct,
		const shader_t *in1, const shader_t *in2,
		const string &ntype, const string &btype)
		:tex(dep, s, hd, color_t(0.0), color_t(1.0), ntype, btype),
		ctype(ct), input1(in1), input2(in2)
{
}

CFLOAT cloudsNode_t::stdoutFloat(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye
				,const scene_t *scene)const
{
	return tex.getFloat(sp.P());
}

colorA_t cloudsNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene)const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	colorA_t rescol(intensidad);
	if (ctype==1) {
		point3d_t pt(sp.P());
		rescol.set(intensidad, tex.getFloat(point3d_t(pt.y, pt.x, pt.z)), tex.getFloat(point3d_t(pt.y, pt.z, pt.x)), 1.0);
	}
	if ((input1==NULL) || (input2==NULL)) return rescol;
	return input1->stdoutColor(state, sp, eye, scene)*intensidad
			 + input2->stdoutColor(state, sp, eye, scene)*(1.0-intensidad);
}

shader_t * cloudsNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
					renderEnvironment_t &render)
{
	string _in1, _in2, _ntype, _btype;
	const string *in1=&_in1, *in2=&_in2,
			*ntype=&_ntype, *btype=&_btype;
	CFLOAT size = 1.0;
	int dep=0, ctype=0;
	bool hard = false;
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	bparams.getParam("size", size);
	bparams.getParam("depth", dep);
	bparams.getParam("hard", hard);
	bparams.getParam("bias", btype);
	bparams.getParam("color_type", ctype);
	bparams.getParam("noise_type", ntype);
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	return new cloudsNode_t(size, dep, hard, ctype, input1, input2, *ntype, *btype);
}

//-------------------------------------------------------------------------
// MARBLE

marbleNode_t::marbleNode_t(PFLOAT sz, int dep, CFLOAT turb, CFLOAT shp, bool hrd,
		const shader_t *in1, const shader_t *in2, const string &ntype, const string &shape)
		:tex(dep, sz, color_t(0.0), color_t(1.0), turb, shp, hrd, ntype, shape),
		input1(in1), input2(in2)
{
}

CFLOAT marbleNode_t::stdoutFloat(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye
				,const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t marbleNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene) const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	if ((input1==NULL) || (input2==NULL))
		return colorA_t(intensidad);
	return (input1->stdoutColor(state,sp,eye,scene))*intensidad
		+ (input2->stdoutColor(state,sp,eye,scene))*(1.0-intensidad);
}

shader_t * marbleNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _in1, _in2, _ntype, _shape;
	const string *in1=&_in1,*in2=&_in2, *ntype=&_ntype, *shape=&_shape;
	CFLOAT size = 1;
	int dep = 2;
	CFLOAT turb=1, shp=1;
	bool hrd = false;
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	bparams.getParam("size", size);
	bparams.getParam("depth", dep);
	bparams.getParam("turbulence", turb);
	bparams.getParam("sharpness", shp);
	bparams.getParam("hard", hrd);
	bparams.getParam("noise_type", ntype);
	bparams.getParam("shape", shape);
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	return new marbleNode_t(size, dep, turb, shp, hrd, input1, input2, *ntype, *shape);
}

//-------------------------------------------------------------------------
// WOOD

woodNode_t::woodNode_t(PFLOAT sz, int dep, CFLOAT turb, bool hrd,
			const shader_t *in1, const shader_t *in2,
			const string &ntype, const string &wtype, const string &shape)
		:tex(dep, sz, color_t(0.0), color_t(1.0), turb, hrd, ntype, wtype, shape),
		input1(in1), input2(in2)
{
}

CFLOAT woodNode_t::stdoutFloat(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye
				,const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t woodNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene) const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	if ((input1==NULL) || (input2==NULL))
		return colorA_t(intensidad);
	return (input1->stdoutColor(state,sp,eye,scene))*intensidad
		+ (input2->stdoutColor(state,sp,eye,scene))*(1.0-intensidad);
}

shader_t * woodNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _in1, _in2, _ntype, _wtype, _shape;
	const string *in1=&_in1,*in2=&_in2, *ntype=&_ntype, *wtype=&_wtype, *shape=&_shape;
	PFLOAT size=1, turb=1, old_rxy;
	int dep = 2;
	bool hrd = false;
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	bparams.getParam("size", size);
	bparams.getParam("depth", dep);
	bparams.getParam("turbulence", turb);
	bparams.getParam("hard", hrd);
	bparams.getParam("wood_type", wtype);
	bparams.getParam("noise_type", ntype);
	bparams.getParam("shape", shape);
	if (bparams.getParam("ringscale_x", old_rxy) || bparams.getParam("ringscale_y", old_rxy))
		cerr << "[woodnode]: 'ringscale_x' and 'ringscale_y' are obsolete, use 'size' instead" << endl;
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	return new woodNode_t(size, dep, turb, hrd, input1, input2, *ntype, *wtype, *shape);
}

//-------------------------------------------------------------------------

CFLOAT colorBandNode_t::stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene) const
{
	return stdoutColor(state, sp, eye, scene).energy();
}

colorA_t colorBandNode_t::stdoutColor(renderState_t &state, const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene) const
{
	if (input==NULL) return colorA_t(0.0);
	CFLOAT f = input->stdoutFloat(state, sp, eye, scene);
	unsigned int i;
	for (i=0;i<band.size();++i) if (f<band[i].first) break;
	if (i==0) return band.front().second;
	if (i==band.size()) return band.back().second;
	CFLOAT range = band[i].first - band[i-1].first;
	if (range<=0.f) return band[i].second;
	CFLOAT mix = (f - band[i-1].first) / range;
	return band[i-1].second*(1.f-mix) + band[i].second*mix;
}

// currently only used with blendershader ramps, input not used
colorA_t colorBandNode_t::stdoutColor(CFLOAT x, renderState_t &state,
		const surfacePoint_t &sp, const vector3d_t &eye, const scene_t *scene) const
{
	unsigned int i;
	for (i=0;i<band.size();++i) if (x<band[i].first) break;
	if (i==0) return band.front().second;
	if (i==band.size()) return band.back().second;
	CFLOAT range = band[i].first - band[i-1].first;
	if (range<=0.f) return band[i].second;
	CFLOAT mix = (x - band[i-1].first) / range;
	return band[i-1].second*(1.f-mix) + band[i].second*mix;
}

shader_t * colorBandNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _inputs;
	const string *inputs=&_inputs;
	shader_t *input=NULL;
	bparams.getParam("input", inputs);
	input = render.getShader(*inputs);
	
	// for the blendershader ramps, colorband can be used without input
	// so, if no input, only show as warning.
	// On the other hand, to prevent confusion about errors, just accept silently...
	/*
	if (input==NULL)
	{
		cerr << "[colorbandnode]: Input shader not found\n";
		return NULL;
	}
	*/
	
	vector<pair<CFLOAT,colorA_t> > band;
	for(list<paramMap_t>::iterator i=lparams.begin();i!=lparams.end();++i)
	{
		pair<CFLOAT, colorA_t> par;
		paramMap_t &params=*i;
		params.getParam("value", par.first);
		params.getParam("color", par.second);
		band.push_back(par);
	}
	shader_t *sha = new colorBandNode_t(band, input);
	return sha;
}

//-------------------------------------------------------------------------

color_t phongNode_t::fromRadiosity(renderState_t &state,const surfacePoint_t &sp,
		const energy_t &ene,const vector3d_t &eye)const
{
	if( ((FACE_FORWARD(sp.Ng(),sp.N(),eye) * ene.dir)<0) || (color==NULL)) return color_t(0.0);
	return (color_t)color->stdoutColor(state, sp, eye)*ene.color;
}

color_t phongNode_t::fromLight(renderState_t &state,const surfacePoint_t &sp,
		const energy_t &energy,const vector3d_t &eye)const
{
	vector3d_t edir = eye;
	edir.normalize();
	color_t C(0.0);
	vector3d_t N = FACE_FORWARD(sp.Ng(), sp.N(), edir);
	CFLOAT inte = N*energy.dir;
	if (inte<=0.f) return color_t(0.0);
	if (color!=NULL) C = inte * color->stdoutColor(state, sp, eye);
	if (specular!=NULL) {
		CFLOAT refle = reflect(N, edir) VDOT energy.dir;
		if (refle>0) {
			refle = std::pow((CFLOAT)refle, (CFLOAT)hard);
			C += refle * specular->stdoutColor(state, sp, eye);
		}
	}
	return C * energy.color;
}

color_t phongNode_t::fromWorld(renderState_t &state,const surfacePoint_t &sp,const scene_t &scene,
		const vector3d_t &eye)const
{
	if (env!=NULL) return env->stdoutColor(state,sp,eye,&scene);
	return color_t(0.0);
}

const color_t phongNode_t::getDiffuse(renderState_t &state,const surfacePoint_t &sp, const vector3d_t &eye)const
{
	vector3d_t eye2 = sp.N();
	if(color!=NULL) return color->stdoutColor(state, sp, eye2);
	return color_t(0.0);
}

bool phongNode_t::getCaustics(renderState_t &state, const surfacePoint_t &sp, const vector3d_t &eye,
														color_t &ref, color_t &trans, PFLOAT &ior) const
{
	if (caus_rcolor!=NULL) ref = caus_rcolor->stdoutColor(state,sp,eye);
	if (caus_tcolor!=NULL) trans = caus_tcolor->stdoutColor(state,sp,eye);
	ior = IOR;
	return (!(ref.null() && trans.null()));
}

void phongNode_t::displace(renderState_t &state, surfacePoint_t &sp, const vector3d_t &eye, PFLOAT res) const
{
	if ((bump==NULL) || (_displace==0.f)) return;

	PFLOAT nfac = _displace/res;
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
	diru = bump->stdoutFloat(state, sp, eye);
	sp.P() = texpt+NU;
	if (sp.hasUV()) { sp.u()=ou+sp.dudNU()*res;  sp.v()=ov+sp.dvdNU()*res; }
	diru -= bump->stdoutFloat(state, sp, eye);
	diru *= nfac;

	sp.P() = texpt-NV;
	if (sp.hasUV()) { sp.u()=ou-sp.dudNV()*res;  sp.v()=ov-sp.dvdNV()*res; }
	dirv = bump->stdoutFloat(state, sp, eye);
	sp.P() = texpt+NV;
	if (sp.hasUV()) { sp.u()=ou+sp.dudNV()*res;  sp.v()=ov+sp.dvdNV()*res; }
	dirv -= bump->stdoutFloat(state, sp, eye);
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


shader_t * phongNode_t::factory(paramMap_t &bparams, std::list<paramMap_t> &lparams,
			renderEnvironment_t &render)
{
	string _c, _s, _e, _cr, _ct, _bp;
	const string *c=&_c, *s=&_s, *e=&_e, *cr=&_cr, *ct=&_ct, *bp=&_bp;
	CFLOAT hard=1.0, ior=1.0;
	shader_t *color=NULL, *specular=NULL, *env=NULL,
		*causr=NULL, *caust=NULL, *bump=NULL;
	bparams.getParam("color", c);
	bparams.getParam("specular", s);
	bparams.getParam("environment", e);
	bparams.getParam("caus_rcolor", cr);
	bparams.getParam("caus_tcolor", ct);
	bparams.getParam("hard", hard);
	bparams.getParam("IOR", ior);
	bparams.getParam("bump", bp);
	CFLOAT dp = 0;
	bparams.getParam("normal", dp);
	color = render.getShader(*c);
	specular = render.getShader(*s);
	env = render.getShader(*e);
	causr = render.getShader(*cr);
	caust = render.getShader(*ct);
	bump = render.getShader(*bp);

	if((*c!="") && (color==NULL)) cerr<<"Input shader "<<*c<<" not found\n";
	if((*s!="") && (specular==NULL)) cerr<<"Input shader "<<*s<<" not found\n";
	if((*e!="") && (env==NULL)) cerr<<"Input shader "<<*e<<" not found\n";
	if((*cr!="") && (causr==NULL)) cerr<<"Input shader "<<*cr<<" not found\n";
	if((*ct!="") && (caust==NULL)) cerr<<"Input shader "<<*ct<<" not found\n";

	return new phongNode_t(color, specular, env, causr, caust, bump, hard, ior, dp);
}

rgbNode_t::rgbNode_t(const shader_t *in1, const shader_t *in2, const shader_t *in3,
				const color_t &c)
{
	inputred = in1;
	inputgreen = in2;
	inputblue = in3;

	color=c;
}

colorA_t rgbNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,const scene_t *scene)const
{
	return colorA_t((inputred == NULL) ? color.getR() : inputred->stdoutFloat(state, sp, eye, scene),
					(inputgreen == NULL) ? color.getG() : inputgreen->stdoutFloat(state, sp, eye, scene),
					(inputblue == NULL) ? color.getB() : inputblue->stdoutFloat(state, sp, eye, scene));
}

shader_t * rgbNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _in1,_in2,_in3;
	const string *in1=&_in1,*in2=&_in2,*in3=&_in3;
	shader_t *inputred=NULL,*inputgreen=NULL,*inputblue=NULL;

	bparams.getParam("inputred",in1);
	bparams.getParam("inputgreen",in2);
	bparams.getParam("inputblue",in3);
	inputred=render.getShader(*in1);
	inputgreen=render.getShader(*in2);
	inputblue=render.getShader(*in3);

	color_t color(0.0);
	bparams.getParam("color",color);

	return new rgbNode_t(inputred,inputgreen,inputblue,color);
}

hsvNode_t::hsvNode_t(const shader_t *in1, const shader_t *in2,
		const shader_t *in3, CFLOAT h, CFLOAT s, CFLOAT v)
{
	inputhue = in1;
	inputsaturation = in2;
	inputvalue = in3;

	hue = h;
	saturation = s;
	value = v;
}

colorA_t hsvNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye,const scene_t *scene)const
{
	CFLOAT h,s,v;
	CFLOAT red,green,blue;

	if(inputhue!=NULL)			h = inputhue->stdoutFloat(state,sp,eye,scene);		else h = hue;
	if(inputsaturation!=NULL)	s = inputsaturation->stdoutFloat(state,sp,eye,scene);	else s = saturation;
	if(inputvalue!=NULL)		v = inputvalue->stdoutFloat(state,sp,eye,scene);		else v = value;

	int i;
	CFLOAT f, p, q, t;

	if( s == 0 ) {
		// achromatic (grey)
		red = green = blue = v;
		return colorA_t(red,green,blue);
	}

	if (h == 1.0f) h = 0.0f;
	h *= 6;
	i = (int)floor( h );
	f = h - i;			// factorial part of h
	p = v * ( 1 - s );
	q = v * ( 1 - s * f );
	t = v * ( 1 - s * ( 1 - f ) );
	switch( i ) {
		case 0:
			red = v;
			green = t;
			blue = p;
			break;
		case 1:
			red = q;
			green = v;
			blue = p;
			break;
		case 2:
			red = p;
			green = v;
			blue = t;
			break;
		case 3:
			red = p;
			green = q;
			blue = v;
			break;
		case 4:
			red = t;
			green = p;
			blue = v;
			break;
		default:		// case 5:
			red = v;
			green = p;
			blue = q;
			break;
	}

	return colorA_t(red,green,blue);
}

shader_t * hsvNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _in1,_in2,_in3;
	const string *in1=&_in1,*in2=&_in2,*in3=&_in3;
	shader_t *inputhue=NULL,*inputsaturation=NULL,*inputvalue=NULL;

	bparams.getParam("inputhue",in1);
	bparams.getParam("inputsaturation",in2);
	bparams.getParam("inputvalue",in3);
	inputhue=render.getShader(*in1);
	inputsaturation=render.getShader(*in2);
	inputvalue=render.getShader(*in3);

	CFLOAT hue=1.0,saturation=1.0,value=1.0;
	bparams.getParam("hue",hue);
	bparams.getParam("saturation",saturation);
	bparams.getParam("value",value);

	return new hsvNode_t(inputhue,inputsaturation,inputvalue,hue,saturation,value);
}

coneTraceNode_t::coneTraceNode_t(const color_t &c, PFLOAT angle, int s, PFLOAT ior, bool r)
{
	samples = s;
	IOR = ior;
	color = c;
	ref = r;
	if ((samples<2) || (angle<=0))
	{
		samples = 1;
		cosa = 1.0;
	}
	else {
		// cosa not really used anymore
		cosa = cos(angle*M_PI/180.0);
		exponent = 1.f-cosa;
		if (exponent<1e-4) exponent=20000; else exponent=2.f/exponent;
	}
	sqr = (int)sqrt((PFLOAT)samples);
	if ((sqr*sqr)!=samples)
		cerr << "Using " << (sqr*sqr) << " samples in conetrace instead of " << samples << endl;

	div = 1.0/(CFLOAT)samples;
	sqrdiv = 1.0/(PFLOAT)sqr;
}

colorA_t coneTraceNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye,const scene_t *scene)const
{
	if(scene==NULL) return colorA_t(0.0);
	if (ref && ((sp.Ng()*eye)<=0) && (state.raylevel>0)) return colorA_t(0.0);
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N = FACE_FORWARD(sp.Ng(),sp.N(),edir);
	vector3d_t Ng = FACE_FORWARD(sp.Ng(),sp.Ng(),edir);
	point3d_t P = sp.P();
	if ((N*eye)<0) N = Ng;
	vector3d_t basedir;
	PFLOAT offset;
	if (ref) basedir = reflect(N,edir);
	else basedir = refract(sp.N(),edir,IOR);
	if (!ref) Ng = -Ng;
	offset = basedir*Ng;
	if (offset<=0.05)
	{
		basedir += Ng*(0.05-offset);
		basedir.normalize();
	}
	int oldlevel = state.rayDivision;

	const void *oldorigin = state.skipelement;
	state.skipelement = sp.getOrigin();
	if ((cosa==1.0) || (oldlevel>1)) 
	{
		color_t res = scene->raytrace(state,P, basedir)*color;
		state.skipelement = oldorigin;
		return res;
	}

	vector3d_t Ru, Rv;
	createCS(basedir, Ru, Rv);

	state.rayDivision = samples;
	color_t res(0.0);
	for(int i=0;i<sqr;++i)
		for(int j=0;j<sqr;++j)
		{
			PFLOAT phi = sqrdiv*(j + ourRandom()) * M_PI * 2.f;
			PFLOAT ct = pow(sqrdiv*(i+ourRandom()), 1.f/(exponent+1.f));
			vector3d_t ray = basedir*ct + sqrt(fabs(1.f-ct*ct))*(sin(phi)*Rv + cos(phi)*Ru);
			offset = ray*Ng;
			if (offset<=0.05)
			{
				ray += Ng*(0.05-offset);
				ray.normalize();
			}
			res += scene->raytrace(state,P, ray);
		}
	res *= div;
	state.rayDivision = oldlevel;
	state.skipelement = oldorigin;
	return res*color;
}

shader_t * coneTraceNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	color_t color(0.0);
	float angle=0.0,IOR=1.5;
	int samples=1;
	bool ref;

	bparams.getParam("color",color);
	bparams.getParam("angle",angle);
	bparams.getParam("IOR",IOR);
	bparams.getParam("samples",samples);
	bparams.getParam("reflect",ref);

	return new coneTraceNode_t(color, angle, samples, IOR, ref);
}

colorA_t fresnelNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye,const scene_t *scene)const
{
	vector3d_t edir=eye;
	edir.normalize();
	vector3d_t N=FACE_FORWARD(sp.Ng(),sp.N(),edir);
	vector3d_t Ng=FACE_FORWARD(sp.Ng(),sp.Ng(),edir);
	if ((N*eye)<0) N=Ng;
	CFLOAT fKr,fKt;
	fast_fresnel(edir, N, IOR, fKr, fKt);
	fKr+=minref;
	if(fKr>1.0) fKr=1.0;
	colorA_t R=(ref!=NULL) ? ref->stdoutColor(state,sp,eye,scene) : colorA_t(0.0);
	colorA_t T=(trans!=NULL) ? trans->stdoutColor(state,sp,eye,scene) : colorA_t(0.0);
	return R*fKr+T*fKt;
}

shader_t * fresnelNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				renderEnvironment_t &render)
{
	string _inR,_inT;
	const string *inR=&_inR,*inT=&_inT;
	shader_t *inputR=NULL,*inputT=NULL;
	PFLOAT ior=1.0;
	CFLOAT minr=0.0;
	bparams.getParam("reflected",inR);
	bparams.getParam("transmitted",inT);
	bparams.getParam("IOR",ior);
	bparams.getParam("min_refle",minr);
	inputR=render.getShader(*inR);
	inputT=render.getShader(*inT);
	return new fresnelNode_t(inputR,inputT,ior,minr);
}

shader_t * imageNode_t::factory(paramMap_t &bparams, std::list<paramMap_t> &lparams,
				renderEnvironment_t &render)
{
	string _name, _intp="bilinear";	// default bilinear interpolation
	const string *name=&_name, *intp=&_intp;
	bparams.getParam("interpolate", intp);
	bparams.getParam("filename", name);	
	if (*name=="")
		cerr << "Required argument filename not found for image block\n";
	else
		return new imageNode_t(name->c_str(), *intp);
	return NULL;
}

colorA_t goboNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye,
				const scene_t *scene)const
{
	if(input1 == NULL || input2 == NULL) return colorA_t(0,0,0);
	if(goboColor == NULL && goboFloat == NULL) return colorA_t(0,0,0);

	colorA_t out,gobo,in1,in2;
	in1 = input1->stdoutColor(state,sp,eye,scene);
	in2 = input2->stdoutColor(state,sp,eye,scene);

	if(goboColor != NULL)
		gobo = goboColor->stdoutColor(state,sp,eye,scene);
	else
	{
		CFLOAT i = goboFloat->stdoutFloat(state,sp,eye,scene);
		gobo.set(i,i,i);
	}

	if(hardEdge == true)
	{
		CFLOAT r,g,b;
		if (gobo.getR() >= edgeVal)
			r = in1.getR();
		else
			r = in2.getR();

		if (gobo.getG() >= edgeVal)
			g = in1.getG();
		else
			g = in2.getG();

		if (gobo.getB() >= edgeVal)
			b = in1.getB();
		else
			b = in2.getB();
		return colorA_t(r, g, b);
	}
	else
	{
		CFLOAT r,g,b;
		r = in1.getR()*gobo.getR()+ in2.getR()*(1-gobo.getR());
		g = in1.getG()*gobo.getG()+ in2.getG()*(1-gobo.getG());
		b = in1.getB()*gobo.getB()+ in2.getB()*(1-gobo.getB());
		return colorA_t(r,g,b);			
	}

}

shader_t * goboNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	string _si1,_si2,_sgC,_sgV;
	const string *si1=&_si1,*si2=&_si2,*sgC=&_sgC,*sgV=&_sgV;
	shader_t *i1=NULL,*i2=NULL,*gC=NULL,*gV=NULL;
	bool hard = true;
	CFLOAT ev = 0.5;

	bparams.getParam("input1",si1);
	bparams.getParam("input2",si2);
	bparams.getParam("goboColor",sgC);
	bparams.getParam("goboFloat",sgV);
	bparams.getParam("hardedge",hard);
	bparams.getParam("edgeval",ev);

	i1=render.getShader(*si1);
	i2=render.getShader(*si2);
	gV=render.getShader(*sgC);
	gC=render.getShader(*sgV);

	if(i1 == NULL) cerr << "Input Shader 1 -" << si1 << "- not found\n";
	if(i2 == NULL) cerr << "Input Shader 2 -" << si2 << "- not found\n";
	if(gC == NULL && gV == NULL) cerr << "No Gobo Specified\n";
	if(gC != NULL && gV != NULL)
	{
		cerr << "2 Gobo's Specified - Using Color Gobo\n";
		gV = NULL;
	}
	return new goboNode_t(i1,i2,gC,gV,hard,ev);
}

//-------------------------------------------------------------------------
// Voronoi node
//-------------------------------------------------------------------------

voronoiNode_t::voronoiNode_t(const shader_t *in1, const shader_t *in2,
		int ct,
		CFLOAT w1, CFLOAT w2, CFLOAT w3, CFLOAT w4,
		PFLOAT mex, PFLOAT size,
		CFLOAT isc, const string &dname)
		:tex(color_t(0.0), color_t(1.0),
				ct, w1, w2, w3, w4,
				mex, size, isc, dname),
		input1(in1), input2(in2)
{
	iscolor = (ct>0);
}

CFLOAT voronoiNode_t::stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t voronoiNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene)const
{
	colorA_t rescol = tex.getColor(sp.P());
	if ((input1==NULL) || (input2==NULL)) return rescol;
	colorA_t irescol(1.0-rescol.getR(), 1.0-rescol.getG(), 1.0-rescol.getB(), rescol.getA());
	return (input1->stdoutColor(state, sp, eye, scene))*rescol
			 + (input2->stdoutColor(state, sp, eye, scene))*irescol;
}

shader_t * voronoiNode_t::factory(paramMap_t &bparams,
				std::list<paramMap_t> &lparams, renderEnvironment_t &render)
{
	string _in1, _in2;
	const string *in1=&_in1, *in2=&_in2;
	string _cltype, _dname;
	const string *cltype=&_cltype, *dname=&_dname;
	CFLOAT fw1=1, fw2=0, fw3=0, fw4=0;
	PFLOAT mex=2.5;	// minkovsky exponent
	CFLOAT isc=1;	// intensity scale
	PFLOAT sz=1;	// size
	int ct=0;	// default "int" color type (intensity)
	
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);

	bparams.getParam("color_type", cltype);
	if (*cltype=="col1") ct=1;
	else if (*cltype=="col2") ct=2;
	else if (*cltype=="col3") ct=3;
	
	bparams.getParam("weight1", fw1);
	bparams.getParam("weight2", fw2);
	bparams.getParam("weight3", fw3);
	bparams.getParam("weight4", fw4);
	bparams.getParam("mk_exponent", mex);
	
	bparams.getParam("intensity", isc);
	bparams.getParam("size", sz);
	
	bparams.getParam("distance_metric", dname);
	
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	return new voronoiNode_t(input1, input2, ct, fw1, fw2, fw3, fw4, mex, sz, isc, *dname);
}

//-------------------------------------------------------------------------
// Musgrave node
//-------------------------------------------------------------------------

musgraveNode_t::musgraveNode_t(const shader_t *in1, const shader_t *in2,
				PFLOAT H, PFLOAT lacu, PFLOAT octs, PFLOAT offs, PFLOAT gain,
				PFLOAT size, CFLOAT iscale,
				const string &ntype, const string &mtype)
				:tex(color_t(0.0), color_t(1.0),
					H, lacu, octs, offs, gain, size, iscale,
					ntype, mtype),
				input1(in1), input2(in2)
{
}

CFLOAT musgraveNode_t::stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t musgraveNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene)const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	if ((input1==NULL) || (input2==NULL)) return colorA_t(intensidad);
	return (input1->stdoutColor(state, sp, eye, scene))*intensidad
			 + (input2->stdoutColor(state, sp, eye, scene))*(1.0-intensidad);
}

shader_t * musgraveNode_t::factory(paramMap_t &bparams,
				std::list<paramMap_t> &lparams, renderEnvironment_t &render)
{
	string _in1, _in2;
	const string *in1=&_in1, *in2=&_in2;
	string _mtype, _ntype;
	const string *mtype=&_mtype, *ntype=&_ntype;
	PFLOAT H=1, lacu=2, octs=2, offs=1, gain=1, size=1, iscale=1;
	
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	
	bparams.getParam("musgrave_type", mtype);
	bparams.getParam("noise_type", ntype);
	
	bparams.getParam("H", H);
	bparams.getParam("lacunarity", lacu);
	bparams.getParam("octaves", octs);
	bparams.getParam("offset", offs);
	bparams.getParam("gain", gain);
	bparams.getParam("size", size);
	bparams.getParam("intensity", iscale);
	
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	
	return new musgraveNode_t(input1, input2, H, lacu, octs, offs, gain, size, iscale, *ntype, *mtype);
}

//-------------------------------------------------------------------------
// Distorted Noise node (Variable Lacunarity Noise)
//-------------------------------------------------------------------------

distortedNoiseNode_t::distortedNoiseNode_t(const shader_t *in1, const shader_t *in2,
		PFLOAT distort, PFLOAT size,
		const string &ntype1, const string &ntype2)
		:tex(color_t(0.0), color_t(1.0), distort, size, ntype1, ntype2),
		input1(in1), input2(in2)
{
}

CFLOAT distortedNoiseNode_t::stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t distortedNoiseNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene)const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	if ((input1==NULL) || (input2==NULL)) return colorA_t(intensidad);
	return (input1->stdoutColor(state, sp, eye, scene))*intensidad
			 + (input2->stdoutColor(state, sp, eye, scene))*(1.0-intensidad);
}

shader_t * distortedNoiseNode_t::factory(paramMap_t &bparams,
				std::list<paramMap_t> &lparams, renderEnvironment_t &render)
{
	string _in1, _in2;
	const string *in1=&_in1, *in2=&_in2;
	string _ntype1, _ntype2;
	const string *ntype1=&_ntype1, *ntype2=&_ntype2;
	PFLOAT dist=1, size=1;
	
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	
	bparams.getParam("noise_type1", ntype1);
	bparams.getParam("noise_type2", ntype2);
	
	bparams.getParam("distort", dist);
	bparams.getParam("size", size);
	
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	
	return new distortedNoiseNode_t(input1, input2, dist, size, *ntype1, *ntype2);
}

//-------------------------------------------------------------------------
// Gradient node
//-------------------------------------------------------------------------

gradientNode_t::gradientNode_t(const shader_t *in1, const shader_t *in2,
		const string &gtype, bool fxy)
		:tex(color_t(0.0), color_t(1.0), gtype, fxy),
		input1(in1), input2(in2)
{
}

CFLOAT gradientNode_t::stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t gradientNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene)const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	if ((input1==NULL) || (input2==NULL)) return colorA_t(intensidad);
	return (input1->stdoutColor(state, sp, eye, scene))*intensidad
			 + (input2->stdoutColor(state, sp, eye, scene))*(1.0-intensidad);
}

shader_t * gradientNode_t::factory(paramMap_t &bparams,
				std::list<paramMap_t> &lparams, renderEnvironment_t &render)
{
	string _in1, _in2;
	const string *in1=&_in1, *in2=&_in2;
	string _gtype;
	const string *gtype=&_gtype;
	bool flip_xy = false;
	
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	
	bparams.getParam("gradient_type", gtype);
	bparams.getParam("flip_xy", flip_xy);
	
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	
	return new gradientNode_t(input1, input2, *gtype, flip_xy);
}

//-------------------------------------------------------------------------
// Random Noise node
//-------------------------------------------------------------------------

randomNoiseNode_t::randomNoiseNode_t(const shader_t *in1, const shader_t *in2, int depth)
		:tex(color_t(0.0), color_t(1.0), depth),
		input1(in1), input2(in2)
{
}

CFLOAT randomNoiseNode_t::stdoutFloat(renderState_t &state, const surfacePoint_t &sp,
				const vector3d_t &eye, const scene_t *scene) const
{
	return tex.getFloat(sp.P());
}

colorA_t randomNoiseNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye, const scene_t *scene)const
{
	CFLOAT intensidad = tex.getFloat(sp.P());
	if ((input1==NULL) || (input2==NULL)) return colorA_t(intensidad);
	return (input1->stdoutColor(state, sp, eye, scene))*intensidad
			 + (input2->stdoutColor(state, sp, eye, scene))*(1.0-intensidad);
}

shader_t * randomNoiseNode_t::factory(paramMap_t &bparams,
				std::list<paramMap_t> &lparams, renderEnvironment_t &render)
{
	string _in1, _in2;
	const string *in1=&_in1, *in2=&_in2;
	int depth = 0;
	
	shader_t *input1=NULL, *input2=NULL;
	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	
	bparams.getParam("depth", depth);
	
	input1 = render.getShader(*in1);
	input2 = render.getShader(*in2);
	
	return new randomNoiseNode_t(input1, input2, depth);
}

//-------------------------------------------------------------------------

__END_YAFRAY
