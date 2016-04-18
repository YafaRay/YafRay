/****************************************************************************
 *
 *      render.cc: Yafray fileformat semantic proccesor implementation
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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
#include "interface_impl.h"

#include "sphere.h"

#if HAVE_PTHREAD
#include "threadedscene.h"
#endif

#include "mesh.h"
#include "reference.h"

#include "targaIO.h"
#include "HDR_io.h"

#if HAVE_EXR
#include "EXR_io.h"
#endif

#include "yafsystem.h"

using namespace std;

#define WARNING cerr<<"[Warning]: "
#define ERRORMSG cerr<<"[Error]: "
#define INFO cerr<<"[Loader]: "

__BEGIN_YAFRAY


interfaceImpl_t::interfaceImpl_t(int ncpus,const string &pluginpath):M(1.0)
{
	cpus=ncpus;
	cachedPathLight=false;
	loadPlugins(pluginpath);
}

interfaceImpl_t::~interfaceImpl_t()
{
#define MAPOF(type) map<string,type *>
#define FREEMAP(type,name)\
	for(MAPOF(type)::iterator i=name.begin();i!=name.end();++i) delete i->second;name.clear()

	FREEMAP(texture_t,texture_table);
	FREEMAP(shader_t,shader_table);
	FREEMAP(object3d_t,object_table);
	FREEMAP(camera_t,camera_table);
	FREEMAP(light_t,light_table);
	FREEMAP(filter_t,filter_table);
	FREEMAP(background_t,background_table);
#undef FREEMAP
#undef MAPOF
}

void interfaceImpl_t::clear()
{
#define MAPOF(type) map<string,type *>
#define FREEMAP(type,name)\
	for(MAPOF(type)::iterator i=name.begin();i!=name.end();++i) delete i->second;name.clear()
	FREEMAP(texture_t,texture_table);
	FREEMAP(shader_t,shader_table);
	FREEMAP(object3d_t,object_table);
	FREEMAP(camera_t,camera_table);
	FREEMAP(light_t,light_table);
	FREEMAP(filter_t,filter_table);
	FREEMAP(background_t,background_table);
	cachedPathLight=false;
	tstack.clear();
#undef FREEMAP
#undef MAPOF
}

void interfaceImpl_t::loadPlugins(const string &path)
{
	typedef void (reg_t)(renderEnvironment_t &);
	cout<<"Loading plugins from '"<<path<<"'..."<<endl;
	list<string> plugins=listDir(path);
	for(list<string>::iterator i=plugins.begin();i!=plugins.end();++i)
	{
		sharedlibrary_t plug(i->c_str());
		if(!plug.isOpen()) continue;
		reg_t *registerPlugin;
		registerPlugin=(reg_t *)plug.getSymbol("registerPlugin");
		if(registerPlugin==NULL) continue;
		registerPlugin(*this);
		pluginHandlers.push_back(plug);
	}
	if(pluginHandlers.size() == 0) cout<<"[WARNING]: ";
	cout<<"found "<<pluginHandlers.size()<<" plugins!\n";
}

void  interfaceImpl_t::transformPush(float *m)
{
	matrix4x4_t L;
	for(int i=0;i<4;++i)
		for(int j=0;j<4;++j)
			L[i][j]=m[i*4+j];
	tstack.push_back(M);
	M=M*L;
}

void  interfaceImpl_t::transformPop()
{
	if(tstack.size()>0)
	{
		M=tstack.back();
		tstack.pop_back();
	}
}

void  interfaceImpl_t::addTexture(paramMap_t &params)
{
	texture_t *ntex=NULL;;
	string _name,_type;
	const string *name=&_name,*type=&_type;

	params.getParam("name",name);
	params.getParam("type",type);
	if(*name=="") return;

	map<string,texture_factory_t *>::iterator i=texture_factory.find(*type);
	if(i!=texture_factory.end()) 
		ntex=i->second(params,*this);
	else WARNING<<"Texture "<<*type<<" not found"<<endl;

	params.checkUnused("texture");

	if (ntex==NULL) return;
	if(texture_table.find(*name)!=texture_table.end())
	{
		WARNING<<"Texture "<<name<<" redefined\n";
		delete texture_table[*name];
	}
	texture_table[*name]=ntex;

}



void interfaceImpl_t::addShader(paramMap_t &params,list<paramMap_t> &lparams)
{
	string _name,_type;
	const string *name=&_name,*type=&_type;
	shader_t *ns=NULL;

	params.getParam("name",name);
	params.getParam("type",type);
	if(*name=="") return;

	map<string,shader_factory_t *>::iterator i=shader_factory.find(*type);
	if(i!=shader_factory.end()) 
		ns=i->second(params,lparams,*this);
	else
	{
		WARNING<<"Unknown shader type "<<*type<<endl;
		return;
	}
	
	if(ns==NULL) WARNING<<"Wrong shader definition for "<<*name<<endl;
	if(ns==NULL) return;
	params.checkUnused("shader");

	if(shader_table.find(*name)!=shader_table.end())
	{
		WARNING<<"Shader "<<*name<<" redefined\n";
		delete shader_table[*name];
	}
	shader_table[*name]=ns;
	INFO<<"Added shader "<<*name<<endl;
}


void interfaceImpl_t::addObject_trimesh(const std::string &name,
				std::vector<point3d_t> &verts, const std::vector<int> &faces,
				std::vector<GFLOAT> &facesuv, std::vector<CFLOAT> &vcol,
				const std::vector<std::string> &shaders, const std::vector<int> &faceshader,
				float sm_angle, bool castShadows, bool useR, bool receiveR, bool caus, bool has_orco,
				const color_t &caus_rcolor, const color_t &caus_tcolor, float caus_IOR)
{
	string shader;
	if(shaders.size()>0) shader=shaders[0];
	if( (name=="") || (shader=="")) return;
	if(shader_table.find(shader)==shader_table.end())
	{
		ERRORMSG<<"undefined shader "<<shader<<" object ignored"<<endl;
		return;
	}
	vector<shader_t *> shader_pointer;
	for(vector<string>::const_iterator i=shaders.begin();i!=shaders.end();++i)
	{
		map<string,shader_t *>::iterator f=shader_table.find(*i);
		if(f==shader_table.end())
			shader_pointer.push_back(NULL);
		else
			shader_pointer.push_back(f->second);
	}

	meshObject_t *obj;
	vector<triangle_t> rfaces;

	for(vector<int>::const_iterator i=faces.begin();i<faces.end();i+=3)
	{
		if((i[0]>(int)verts.size()) || (i[1]>(int)verts.size()) || 
				(i[2]>(int)verts.size()))
			WARNING<<"Skiping face with verts out of bounds\n";
		rfaces.push_back(triangle_t( &( verts[i[0]] ),&( verts[i[1]] ),
					&( verts[i[2]] ) ) );
	}
	if(facesuv.size() >= (rfaces.size()*6) )
	{
		for(unsigned int i=0;i<rfaces.size();++i)
			rfaces[i].setUV(facesuv.begin()+6*i);
	}
	if(vcol.size() >= (rfaces.size()*9) )
	{
		for(unsigned int i=0;i<rfaces.size();++i)
			rfaces[i].setVCOL(vcol.begin()+9*i);
	}

	if(faceshader.size()==rfaces.size())
		for(unsigned int i=0;i<faceshader.size();++i)
			rfaces[i].setShader(shader_pointer[faceshader[i]]);

	vector<vector3d_t> normals;
	obj=meshObject_t::factory(has_orco, M, verts,normals,rfaces,facesuv,vcol);
	//obj->hasOrco(has_orco);
	//obj->transform(M);
	if(sm_angle>0.0)
		obj->autoSmooth(sm_angle);
	obj->tangentsFromUV();

	if(object_table.find(name)!=object_table.end())
	{
		WARNING<<"Object "<<name<<" redefined\n";
		delete object_table[name];
	}
	obj->castShadows(castShadows);
	obj->useForRadiosity(useR);
	obj->reciveRadiosity(receiveR);
	obj->caustics(caus);
	obj->setCaustic(caus_rcolor, caus_tcolor, caus_IOR);
	obj->setShader(shader_table[shader]);
	object_table[name]=obj;
	INFO<<"Added object "<<name<<endl;
}

void interfaceImpl_t::addObject_reference(const string &name,const string &original)
{
	object3d_t *obj=NULL;
	if((object_table.find(original)==object_table.end())
			|| (original==name))
		WARNING<<"Object "<<name<<" undefined\n";
	else
		obj=referenceObject_t::factory(M,object_table[original]);
	
	if(obj==NULL) return;
	if(object_table.find(name)!=object_table.end())
	{
		WARNING<<"Object "<<name<<" redefined\n";
		delete object_table[name];
	}
	object_table[name]=obj;
}

void interfaceImpl_t::addLight(paramMap_t &params)
{
	string _name,_type;
	const string *name=&_name,*type=&_type;
	bool render=true,indirect=true;
	light_t *l=NULL;

	params.getParam("name",name);
	params.getParam("type",type);
	params.getParam("use_in_render",render);
	params.getParam("use_in_indirect",indirect);

	if(*name=="") return;

	map<string,light_factory_t *>::iterator i=light_factory.find(*type);
	if(i!=light_factory.end()) 
		l=i->second(params,*this);

	params.checkUnused("light");

	if(l==NULL) return;

	l->useInRender(render);
	l->useInIndirect(indirect);

	if(light_table.find(*name)!=light_table.end())
	{
		WARNING<<"Light "<<*name<<" redefined\n";
		delete light_table[*name];
	}
	light_table[*name]=l;
	INFO<<"Added "<<*type<<" light "<<*name<<endl;

}

void interfaceImpl_t::addCamera(paramMap_t &params)
{
	string _name, _type="perspective",
			_bkhtype="disk1", _bkhbias="uniform";
	const string *name=&_name, *type=&_type,
			*bkhtype=&_bkhtype, *bkhbias=&_bkhbias;
	point3d_t from(0,1,0), to(0,0,0), up(0,1,1);
	int resx=320, resy=200;
	float aspect=1, dfocal=1, apt=0, dofd=0, bkhrot=0;
	bool useq = false;
	params.getParam("name", name);
	if (*name=="") return;
	params.getParam("from", from);
	params.getParam("to", to);
	params.getParam("up", up);
	params.getParam("resx", resx);
	params.getParam("resy", resy);
	params.getParam("focal", dfocal);
	params.getParam("aperture", apt);
	params.getParam("dof_distance", dofd);
	params.getParam("use_qmc", useq);
	params.getParam("type", type);
	params.getParam("bokeh_type", bkhtype);
	params.getParam("bokeh_bias", bkhbias);
	params.getParam("bokeh_rotation", bkhrot);
	params.getParam("aspect_ratio", aspect);
	// camera type
	camera_t::cameraType ct = camera_t::CM_PERSPECTIVE;
	if (*type=="ortho")
		ct = camera_t::CM_ORTHO;
	else if (*type=="spherical")
		ct = camera_t::CM_SPHERICAL;
	else if (*type=="lightprobe")
		ct = camera_t::CM_LIGHTPROBE;
	// bokeh type
	camera_t::bokehType bt = camera_t::BK_DISK1;
	if (*bkhtype=="disk2")
		bt = camera_t::BK_DISK2;
	else if (*bkhtype=="triangle")
		bt = camera_t::BK_TRI;
	else if (*bkhtype=="square")
		bt = camera_t::BK_SQR;
	else if (*bkhtype=="pentagon")
		bt = camera_t::BK_PENTA;
	else if (*bkhtype=="hexagon")
		bt = camera_t::BK_HEXA;
	else if (*bkhtype=="ring")
		bt = camera_t::BK_RING;
	// bokeh bias
	camera_t::bkhBiasType bbt = camera_t::BB_NONE;
	if (*bkhbias=="center")
		bbt = camera_t::BB_CENTER;
	else if (*bkhbias=="edge")
		bbt = camera_t::BB_EDGE;
	camera_t *cam=new camera_t(from, to, up,
														resx, resy, aspect, dfocal,
														apt, dofd, useq,
														ct, bt, bbt, bkhrot);
	if (camera_table.find(*name)!=camera_table.end())
	{
		WARNING << "Camera " << name << " redefined\n";
		delete camera_table[*name];
	}
	camera_table[*name] = cam;
	INFO << "Added camera " << *name << endl;

}


void interfaceImpl_t::addFilter(paramMap_t &params)
{
	string _name,_type;
	const string *name=&_name,*type=&_type;
	filter_t *f=NULL;

	params.getParam("name",name);
	params.getParam("type",type);

	if(*name=="") return;

	if(*type=="dof") 	f=filter_dof(params);
	if(*type=="antinoise") f=filter_antinoise(params);

	params.checkUnused("filter");

	if(f==NULL) return;

	if(filter_table.find(*name)!=filter_table.end()) 
	{
		WARNING<<"Filter "<<*name<<" redefined\n";
		delete filter_table[*name];
	}
	filter_table[*name]=f;

	INFO<<"Added "<<*type<<" filter "<<*name<<endl;
}

filter_t * interfaceImpl_t::filter_dof(paramMap_t &params)
{
	/*
	GFLOAT focus=1.0;
	GFLOAT near_radius=1.0;
	GFLOAT far_radius=1.0,scale=1.0;
	params.getParam("focus",focus);
	params.getParam("near_blur",near_radius);
	params.getParam("far_blur",far_radius);
	params.getParam("scale",scale);
	return new filterDOF_t(focus,near_radius,far_radius,scale);
	*/
	return NULL;
}

filter_t *interfaceImpl_t::filter_antinoise(paramMap_t &params)
{
	/*
	GFLOAT radius=1.0;
	GFLOAT delta=1.0;
	params.getParam("radius",radius);
	params.getParam("max_delta",delta);
	return new filterAntiNoise_t(radius,delta);
	*/
	return NULL;
}


void interfaceImpl_t::addBackground(paramMap_t &params)
{
	string _name,_type;
	const string *name=&_name,*type=&_type;
	background_t *b=NULL;

	params.getParam("name",name);
	params.getParam("type",type);

	if (*name=="") return;

	map<string,background_factory_t *>::iterator i=background_factory.find(*type);
	if(i!=background_factory.end()) 
		b=i->second(params,*this);
	else
	{
		WARNING << "Wrong type for background " << *type << endl;
		return;
	}

	params.checkUnused("background");

	if(b==NULL) return;

	if(background_table.find(*name)!=background_table.end())
	{
		WARNING << "background " << *name << " redefined\n";
		delete background_table[*name];
	}
	background_table[*name]=b;

	INFO << "Added " << *type << " background " << *name << endl;
}


void interfaceImpl_t::render(paramMap_t &params)
{
	string _camera, _outfile="salida.tga",_background;
	const string *camera=&_camera, *outfile=&_outfile,*background=&_background;
	int raydepth = 3;
	PFLOAT bias = 0.1;
	GFLOAT xmin=-1.5,xmax=1.5,ymin=-1.5,ymax=1.5;
	bool save_alpha = false;

	params.getParam("camera_name", camera);
	params.getParam("outfile", outfile);
	params.getParam("save_alpha", save_alpha);
	params.getParam("raydepth", raydepth);
	params.getParam("bias", bias);
	params.getParam("background_name", background);
	params.getParam("border_xmin",xmin);
	params.getParam("border_xmax",xmax);
	params.getParam("border_ymin",ymin);
	params.getParam("border_ymax",ymax);

	// get new AA parameters
	int AA_passes=0, AA_minsamples=1;
	//check if old names used, if so, warn, and convert to new
	if (params.getParam("samples", AA_passes)) {
		WARNING << "'samples' parameter obsolete, use 'AA_passes' instead\n";
		AA_passes -= 1; // old samples value includes first pass
	}
	else params.getParam("AA_passes", AA_passes);
	if (AA_passes<0) {
		INFO << "AA_passes cannot be less than 0\n";
		AA_passes = 1;
	}
	params.getParam("AA_minsamples", AA_minsamples);
	if ((AA_passes) && (AA_minsamples<1)) {
		INFO << "AA_minsamples must be at least 1\n";
		AA_minsamples = 1;
	}
	PFLOAT AA_pixelwidth=1.5, AA_threshold=0.05;
	params.getParam("AA_pixelwidth", AA_pixelwidth);
	if (AA_pixelwidth<1.0) {
	  INFO << "AA_pixelwidth minimum is 1.0\n";
	  AA_pixelwidth = 1.0;
	}
	else if (AA_pixelwidth>2.0) {
	  INFO << "AA_pixelwidth maximum is 2.0\n";
	  AA_pixelwidth = 2.0;
	}
	if (params.getParam("tolerance", AA_threshold))
		WARNING << "'tolerance' parameter obsolete, use 'AA_threshold' instead\n";
	else
		params.getParam("AA_threshold", AA_threshold);
	bool AA_jitterfirst = false;
	params.getParam("AA_jitterfirst", AA_jitterfirst);
	bool clamp_rgb = false;
	params.getParam("clamp_rgb", clamp_rgb);

	if(*camera=="")
	{
		ERRORMSG<<"No camera to render\n";
		return;
	}
	if(camera_table.find(*camera)==camera_table.end())
	{
		ERRORMSG<<"Camera "<<*camera<<" does not exist\n";
		return;
	}

	// exposure and gamma adjustment controls
	// when exposure set to 0, it is disabled, gamma=1 is no change
	CFLOAT exposure=0, gamma=1;
	params.getParam("exposure", exposure);
	params.getParam("gamma", gamma);

	// simple fog parameters, default is no fog
	CFLOAT fgdens=0;
	params.getParam("fog_density", fgdens);
	// default fog color is white
	color_t fgcol(1.0);
	params.getParam("fog_color", fgcol);


#if HAVE_PTHREAD
	scene_t *pscene=threadedscene_t::factory();
#else
	scene_t *pscene=scene_t::factory();
#endif
	scene_t &scene=*pscene;

	camera_t *cam=camera_table[*camera];

	scene.setCamera(cam);
	for(map<string,object3d_t *>::iterator i=object_table.begin();
			i!=object_table.end();++i)
		scene.addObject((*i).second);
	for(map<string,light_t *>::iterator i=light_table.begin();
			i!=light_table.end();++i)
		scene.addLight((*i).second);
	for(map<string,filter_t *>::iterator i=filter_table.begin();
			i!=filter_table.end();++i)
		scene.addFilter((*i).second);

	// output filetype, types=tga/hdr/exr, default = tga
	string _output_type="tga";
	const string *output_type=&_output_type;
	params.getParam("output_type", output_type);
	// for exr output, extra flags to set save (half|float) and compression type
	string _exr_flags="";	// defaults half, zip, no zbuf
	const string *exr_flags=&_exr_flags;
	params.getParam("exr_flags", exr_flags);

	// alpha premultiply, off by default
	string _alpha_premul="off";
	const string *alpha_premul=&_alpha_premul;
	params.getParam("alpha_premultiply", alpha_premul);
	// alpha mask background, off by default
	string _alpha_mbg="off";
	const string *alpha_mbg=&_alpha_mbg;
	params.getParam("alpha_maskbackground", alpha_mbg);

	cout << "Rendering with " << raydepth << " raydepth\n";
	if (AA_passes)
		cout << AA_passes << " anti-alias passes and "
			<< AA_minsamples << " minimum samples per pass, "
			<< AA_passes*AA_minsamples << " samples total.\n";
	else cout << "No anti-aliasing.\n";

	scene.setMaxRayDepth(raydepth);

	if(*background!="")
	{
		if(background_table.find(*background)==background_table.end())
			WARNING<<"Background "<<*background<<" does not exist\n";
		else
			scene.setBackground(background_table[*background]);
	}

	// alpha premultiply flag
	if (*alpha_premul=="on")
		scene.alphaPremultiply(true);
	else
		scene.alphaPremultiply(false);
	// alpha background mask
	if (*alpha_mbg=="on")
		scene.alphaMaskBackground(true);
	else
		scene.alphaMaskBackground(false);

	// gamma & exposure
	scene.setExposure(exposure);
	scene.setGamma(gamma);
	// fog
	scene.setFog(fgdens, fgcol);

	// set the AA params
	scene.setAASamples(AA_passes, AA_minsamples, AA_pixelwidth, AA_threshold, AA_jitterfirst);
	scene.clampRGB(clamp_rgb);
	scene.setRegion(xmin,xmax,ymin,ymax);
	scene.setBias(bias);
	if(cachedPathLight) scene.setRepeatFirst();
	
	int nthreads=1;
	if(params.getParam("threads", nthreads))
		scene.setCPUs(nthreads);
	else
		scene.setCPUs(cpus);

	// tone mapping bypassed when hdr/exr output is requested
	if (*output_type=="hdr") {
		outHDR_t hdrout(cam->resX(), cam->resY(), outfile->c_str());
		scene.tonemap(false);
		scene.render(hdrout);
		hdrout.flush();
	}
	else if (int((*output_type).find("exr"))!=-1) {
#if HAVE_EXR
		outEXR_t exrout(cam->resX(), cam->resY(), outfile->c_str(), *exr_flags);
		scene.tonemap(false);
		scene.render(exrout);
		exrout.flush();
#else
		cout << "Yafray was compiled without OpenEXR support.\nImage saved in hdr format instead." << endl;
		outHDR_t hdrout(cam->resX(), cam->resY(), outfile->c_str());
		scene.tonemap(false);
		scene.render(hdrout);
		hdrout.flush();
#endif
	}
	else {
		outTga_t tgaout(cam->resX(), cam->resY(), outfile->c_str(), save_alpha);
		scene.tonemap(true);
		scene.render(tgaout);
		tgaout.flush();
	}

	delete pscene;
}

void interfaceImpl_t::render(paramMap_t &params,colorOutput_t &output)
{
	string _camera,_background;
	const string *camera=&_camera,*background=&_background;
	int raydepth = 3;
	GFLOAT xmin=-1.5,xmax=1.5,ymin=-1.5,ymax=1.5;
	PFLOAT bias = 0.1;

	params.getParam("camera_name", camera);
	params.getParam("raydepth", raydepth);
	params.getParam("bias", bias);
	params.getParam("background_name", background);
	params.getParam("border_xmin",xmin);
	params.getParam("border_xmax",xmax);
	params.getParam("border_ymin",ymin);
	params.getParam("border_ymax",ymax);

	// get new AA parameters
	int AA_passes=0, AA_minsamples=1;
	//check if old names used, if so, warn, and convert to new
	if (params.getParam("samples", AA_passes)) {
		WARNING << "'samples' parameter obsolete, use 'AA_passes' instead\n";
		AA_passes -= 1; // old samples value includes first pass
	}
	else params.getParam("AA_passes", AA_passes);
	if (AA_passes<0) {
		INFO << "AA_passes cannot be less than 0\n";
		AA_passes = 1;
	}
	params.getParam("AA_minsamples", AA_minsamples);
	if ((AA_passes) && (AA_minsamples<1)) {
		INFO << "AA_minsamples must be at least 1\n";
		AA_minsamples = 1;
	}
	PFLOAT AA_pixelwidth=1.5, AA_threshold=0.05;
	params.getParam("AA_pixelwidth", AA_pixelwidth);
	if (AA_pixelwidth<1.0) {
	  INFO << "AA_pixelwidth minimum is 1.0\n";
	  AA_pixelwidth = 1.0;
	}
	else if (AA_pixelwidth>2.0) {
	  INFO << "AA_pixelwidth maximum is 2.0\n";
	  AA_pixelwidth = 2.0;
	}
	if (params.getParam("tolerance", AA_threshold))
		WARNING << "'tolerance' parameter obsolete, use 'AA_threshold' instead\n";
	else
		params.getParam("AA_threshold", AA_threshold);
	bool AA_jitterfirst = false;
	params.getParam("AA_jitterfirst", AA_jitterfirst);
	bool clamp_rgb = false;
	params.getParam("clamp_rgb", clamp_rgb);

	if(*camera=="")
	{
		ERRORMSG<<"No camera to render\n";
		return;
	}
	if(camera_table.find(*camera)==camera_table.end())
	{
		ERRORMSG<<"Camera "<<*camera<<" does not exist\n";
		return;
	}

	// exposure and gamma adjustment controls
	// when exposure set to 0, it is disabled, gamma=1 is no change
	CFLOAT exposure=0, gamma=1;
	params.getParam("exposure", exposure);
	params.getParam("gamma", gamma);

	// simple fog parameters, default is no fog
	CFLOAT fgdens=0;
	params.getParam("fog_density", fgdens);
	// default fog color is white
	color_t fgcol(1.0);
	params.getParam("fog_color", fgcol);

#if HAVE_PTHREAD
	scene_t *pscene=threadedscene_t::factory();
#else
	scene_t *pscene=scene_t::factory();
#endif
	scene_t &scene=*pscene;
	camera_t *cam=camera_table[*camera];

	scene.setCamera(cam);
	for(map<string,object3d_t *>::iterator i=object_table.begin();
			i!=object_table.end();++i)
		scene.addObject((*i).second);
	for(map<string,light_t *>::iterator i=light_table.begin();
			i!=light_table.end();++i)
		scene.addLight((*i).second);
	for(map<string,filter_t *>::iterator i=filter_table.begin();
			i!=filter_table.end();++i)
		scene.addFilter((*i).second);

	// alpha premultiply, off by default
	string _alpha_premul="off";
	const string *alpha_premul=&_alpha_premul;
	params.getParam("alpha_premultiply", alpha_premul);
	// alpha mask background, off by default
	string _alpha_mbg="off";
	const string *alpha_mbg=&_alpha_mbg;
	params.getParam("alpha_maskbackground", alpha_mbg);

	cout << "Rendering with " << raydepth << " raydepth\n";
	if (AA_passes)
		cout << AA_passes << " anti-alias passes and "
			<< AA_minsamples << " minimum samples per pass, "
			<< AA_passes*AA_minsamples << " samples total.\n";
	else cout << "No anti-aliasing.\n";

	scene.setMaxRayDepth(raydepth);

	if(*background!="")
	{
		if(background_table.find(*background)==background_table.end())
			WARNING<<"Background "<<*background<<" does not exist\n";
		else
			scene.setBackground(background_table[*background]);
	}

	// alpha premultiply flag
	if (*alpha_premul=="on")
		scene.alphaPremultiply(true);
	else
		scene.alphaPremultiply(false);
	// alpha background mask
	if (*alpha_mbg=="on")
		scene.alphaMaskBackground(true);
	else
		scene.alphaMaskBackground(false);

	// gamma & exposure
	scene.setExposure(exposure);
	scene.setGamma(gamma);
	// forgot this part, for external render, always set tonemap to true,
	// user can still bypass it by setting gamma=1, exposure=0
	scene.tonemap(true);
	
	// fog
	scene.setFog(fgdens, fgcol);

	// set the AA params
	scene.setAASamples(AA_passes, AA_minsamples, AA_pixelwidth, AA_threshold, AA_jitterfirst);
	scene.clampRGB(clamp_rgb);
	scene.setRegion(xmin,xmax,ymin,ymax);
	scene.setBias(bias);
	if(cachedPathLight) scene.setRepeatFirst();
	
	int nthreads=1;
	if(params.getParam("threads", nthreads))
		scene.setCPUs(nthreads);
	else
		scene.setCPUs(cpus);
	scene.render(output);

	output.flush();
}

shader_t *interfaceImpl_t::getShader(const std::string name)const
{
	map<string,shader_t *>::const_iterator i=shader_table.find(name);
	if(i!=shader_table.end()) return i->second;
	else return NULL;
}

texture_t *interfaceImpl_t::getTexture(const std::string name)const
{
	map<string,texture_t *>::const_iterator i=texture_table.find(name);
	if(i!=texture_table.end()) return i->second;
	else return NULL;
}

void interfaceImpl_t::repeatFirstPass()
{
	cachedPathLight=true;
}

void interfaceImpl_t::registerFactory(const std::string &name,light_factory_t *f)
{
	light_factory[name]=f;
}

void interfaceImpl_t::registerFactory(const std::string &name,shader_factory_t *f)
{
	shader_factory[name]=f;
}

void interfaceImpl_t::registerFactory(const std::string &name,texture_factory_t *f)
{
	texture_factory[name]=f;
}

void interfaceImpl_t::registerFactory(const std::string &name,filter_factory_t *f)
{
	filter_factory[name]=f;
}

void interfaceImpl_t::registerFactory(const std::string &name,background_factory_t *f)
{
	background_factory[name]=f;
}

extern "C"
{

YAFRAYPLUGIN_EXPORT yafrayInterface_t *getYafray(int cpus,const string &ppath)
{
	return new interfaceImpl_t(cpus,ppath);
}

}

__END_YAFRAY
