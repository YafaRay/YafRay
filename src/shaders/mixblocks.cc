
#include "mixblocks.h"

__BEGIN_YAFRAY

colorA_t mixModeNode_t::stdoutColor(renderState_t &state,
		const surfacePoint_t &sp,
		const vector3d_t &eye,const scene_t *scene)const
{
	colorA_t c1,c2,out;
	c1 = input1->stdoutColor(state,sp,eye,scene);
	c2 = input2->stdoutColor(state,sp,eye,scene);

	switch(type)
	{
		case ADDITIVE: return c1+c2;
		case SUBTRACTIVE: return c1+c2-1;
		case MMULTIPLY: return c1*c2;
		case AVERAGE: return (c1+c2)/2;
		case SCREEN: return 1 - (1-c1) * (1-c2);
		case EXCLUSION: return c1+c2 - 2*c1*c2;
		case SOFTLIGHT: return (1-c1) * (c1*c2) + ( 1 - (1-c1) * (1-c2) );
		case STAMP: return c1+2.0*c2-colorA_t(1.0);
		default: break;
	}

	CFLOAT r1,g1,b1,r2,g2,b2;
	r1 = c1.getR();
	g1 = c1.getG();
	b1 = c1.getB();
	r2 = c2.getR();
	g2 = c2.getG();
	b2 = c2.getB();

	switch(type)
	{
		case DIFFERENCE: return colorA_t(fabs(r1-r2),fabs(g1-g2),fabs(b1-b2));
		case NEGATION: return colorA_t(1-fabs(1-r1-r2),1-fabs(1-g1-g2),1-fabs(1-b1-b2));
		case COLORDODGE: return colorA_t(r1/(1-r2),g1/(1-g2),b1/(1-b2));
		case COLORBURN: return colorA_t(1-(1-r1)/r2,1-(1-g1)/g2,1-(1-b1)/b2);
		case REFLECT: return colorA_t((r1*r1)/(1-r2),(g1*g1)/(1-g2),(b1*b1)/(1-b2));
		case FREEZE: return colorA_t(1 - ((1-r1)*(1-r1))/r2,
																1 - ((1-g1)*(1-g1))/g2,
																1 - ((1-b1)*(1-b1))/b2);
		case DARKEN: return colorA_t((r1<r2) ? r1 : r2,
										 						(g1<g2) ? g1 : g2,
																(b1<b2) ? b1 : b2);
		case LIGHTEN: return colorA_t((r1>r2) ? r1 : r2,
                 		 						(g1>g2) ? g1 : g2,
                 								(b1>b2) ? b1 : b2);
		case OVERLAY: return colorA_t(
			(r1 < 0.5) ? 2*r1*r2 : 1 - 2*(1-r1)*(1-r2),
			(g1 < 0.5) ? 2*g1*g2 : 1 - 2*(1-g1)*(1-g2),
			(b1 < 0.5) ? 2*b1*b2 : 1 - 2*(1-b1)*(1-b2) );
		case HARDLIGHT: return colorA_t(
			(r2 < 0.5) ? 2*r1*r2 : 1 - 2*(1-r1)*(1-r2),
			(g2 < 0.5) ? 2*g1*g2 : 1 - 2*(1-g1)*(1-g2),
			(b2 < 0.5) ? 2*b1*b2 : 1 - 2*(1-b1)*(1-b2) );
		default: break;
	}
	return colorA_t();
}

using namespace std;
static map<string,mixModeNode_t::mixModes> modes;

void mixModeNode_t::fillModes()
{
	modes["add"]= ADDITIVE;
	modes["subtract"]= SUBTRACTIVE;
	modes["multiply"]= MMULTIPLY;
	modes["average"]= AVERAGE;
	modes["screen"]= SCREEN;
	modes["exclusion"]= EXCLUSION;
	modes["softlight"]= SOFTLIGHT;
	modes["difference"]= DIFFERENCE;
	modes["negation"]= NEGATION;
	modes["stamp"]= STAMP;
	modes["colordodge"]= COLORDODGE;
	modes["colorburn"]= COLORBURN;
	modes["reflect"]= REFLECT;
	modes["freeze"]= FREEZE;
	modes["lighten"]= LIGHTEN;
	modes["darken"]= DARKEN;
	modes["overlay"]= OVERLAY;
	modes["hardlight"]= HARDLIGHT;
}

shader_t * mixModeNode_t::factory(paramMap_t &bparams,
		list<paramMap_t> &lmod,renderEnvironment_t &render)
{
	string _in1,_in2,_type;
	const string *in1=&_in1,*in2=&_in2,*type=&_type;
	mixModeNode_t::mixModes tin;
	shader_t *input1=NULL,*input2=NULL;

	bparams.getParam("input1",in1);
	bparams.getParam("input2",in2);
	input1=render.getShader(*in1);
	input2=render.getShader(*in2);

	if((input1==NULL) || (input2==NULL)) return NULL;

	bparams.getParam("mode",type);

	if(modes.empty()) fillModes();

	map<string,mixModeNode_t::mixModes>::const_iterator i=modes.find(*type);
	if(i!=modes.end()) tin=i->second;
	else
	{
		cerr<<"Unknown mix mode "<<*type<<" for mix block\n";
		return NULL;
	}

	return new mixModeNode_t(input1,input2,tin);
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("mix",mixModeNode_t::factory);
	std::cout<<"Registered mix block\n";
}

}
__END_YAFRAY
