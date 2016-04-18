
#include "sss.h"

using namespace std;

__BEGIN_YAFRAY

sssNode_t::sssNode_t(const color_t &c,PFLOAT r,int s):color(c),radius(r)
{
	sqrtsamples=(int)sqrt((PFLOAT)s);
	samples=sqrtsamples*sqrtsamples;
	exponent=log(0.1)/radius; // radius is the distance at which prob falls below 0.1
	expinv=-1.0/exponent;
	halfradius=radius*0.2; 
	farradius=radius*1.5; 
}

color_t sssNode_t::sampleObject(renderState_t &state,const object3d_t *obj,
		                            const point3d_t &from,const vector3d_t &ray,
																const point3d_t &outpoint,
																CFLOAT &W,const scene_t *scene)const
{
	try
	{
		surfacePoint_t sp;
		if(!obj->shoot(state,sp,from,ray,false,farradius+halfradius)) throw true;
		PFLOAT distance=(sp.P()-outpoint).length();
		if(distance>farradius) throw true;
		W=exp(exponent*distance);
		if(W<0.01) throw true;
		return scene->light(state,sp,sp.P()+sp.Ng()); // This includes specular highlights, shouldn't
	}
	catch (bool useless)
	{
		W=0;
		return color_t(0,0,0);
	}
}

point3d_t sssNode_t::getSamplingPoint(renderState_t &state,const surfacePoint_t &sp,const vector3d_t &eye)const
{
	vector3d_t Ng=FACE_FORWARD(sp.Ng(),sp.N(),eye);
	const object3d_t *obj=sp.getObject();
	const void *oldo=state.skipelement;
	state.skipelement=sp.getOrigin();
	surfacePoint_t tempsp;
	PFLOAT dist;

	if(!obj->shoot(state,tempsp,sp.P(),-Ng,false,2.1*halfradius)) dist=halfradius;
	else
	{
		PFLOAT safedis=tempsp.Z()*0.5;
		if(safedis<halfradius) dist=safedis;
		else dist=halfradius;
	}
	state.skipelement=oldo;
	
	return sp.P()-Ng*dist;
}

colorA_t sssNode_t::stdoutColor(renderState_t &state,const surfacePoint_t &sp,
		const vector3d_t &eye,const scene_t *scene)const
{
	if(scene==NULL) return colorA_t(0,0,0);
	if(state.rayDivision>1) return colorA_t(0,0,0); // avoid indirect recursion
	state.rayDivision+=samples;
	//point3d_t above=sp.P()+Ng*halfradius;
	point3d_t below=getSamplingPoint(state,sp,eye);
	CFLOAT Wtotal=(CFLOAT)samples;
	color_t total(0,0,0);

	PFLOAT anglestep=2.0*M_PI/sqrtsamples;
	PFLOAT diststep=1.0/sqrtsamples;
	PFLOAT jitta=ourRandom()*anglestep;
	PFLOAT jittd=ourRandom()*diststep;
	PFLOAT angle=jitta;
	for(int i=0;i<sqrtsamples;++i)
	{
		PFLOAT dist=jittd;
		for(int j=0;j<sqrtsamples;++j)
		{
			CFLOAT W=0;
			vector3d_t off=sp.NU()*cos(angle)+sp.NV()*sin(angle);
			off.normalize();
			off*=log(dist)/exponent;
			vector3d_t ray=(sp.P()+off)-below;
			ray.normalize();
			color_t sample=sampleObject(state,sp.getObject(),below,ray,sp.P(),W,scene);
			total+=W*sample;
			sample=sampleObject(state,sp.getObject(),below,-ray,sp.P(),W,scene);
			total+=W*sample;

			dist+=diststep;
		}
		angle+=anglestep;
	}
	state.rayDivision-=samples;
	if(Wtotal>0.0) Wtotal=1.0/Wtotal;
	return (total*color)*Wtotal*expinv;
	/*
	for(int i=0;i<(samples>>1);++i)
	{
		// from avobe
		PFLOAT angle=ourRandom()*2.0*M_PI;
		vector3d_t off=sp.NU()*cos(angle)+sp.NV()*sin(angle);
		off.normalize();
		CFLOAT W=ourRandom();
		off*=log(W)/exponent;
		vector3d_t ray=(sp.P()+off)-above;
		ray.normalize();
		color_t sample=sampleObject(state,sp.getObject(),above,ray,sp.P(),W,scene);
		total+=W*sample;
		Wtotal+=W;
		// from below
		angle=ourRandom()*2.0*M_PI;
		off=sp.NU()*cos(angle)+sp.NV()*sin(angle);
		off.normalize();
		W=ourRandom();
		off*=log(W)/exponent;
		ray=(sp.P()+off)-below;
		ray.normalize();
		sample=sampleObject(state,sp.getObject(),below,ray,sp.P(),W,scene);
		total+=W*sample;
		Wtotal+=W;
	}
	*/
}

shader_t * sssNode_t::factory(paramMap_t &bparams,std::list<paramMap_t> &lparams,
				        renderEnvironment_t &render)
{
	color_t color(0.0);
	float radius=0.1;
	int samples=32;

	bparams.getParam("color",color);
	bparams.getParam("radius",radius);
	bparams.getParam("samples",samples);

	return new sssNode_t(color,radius,samples);
}
extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("sss", sssNode_t::factory);
	std::cout << "Registered sss\n";
}

}

__END_YAFRAY
