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

#include "render.h"

#include "sphere.h"
#include "reference.h"
#include "threadedscene.h"
#include "forkedscene.h"

#include "targaIO.h"
#include "HDR_io.h"
#include "EXR_io.h"

#define WARNING cerr<<"[Warning]: "
#define ERRORMSG cerr<<"[Error]: "
#define INFO cerr<<"[Loader]: "

#include "yafsystem.h"

__BEGIN_YAFRAY

render_t::render_t(int ncpus, strategy_t strat, const string &pluginpath):M(1.0)
{
	for(int i=0;i<MAX_AST;++i)
		handler[i]=NULL;
	cpus=ncpus;
	strategy = strat;
	scymin=scxmin=-2;
	scymax=scxmax=2;
	handler[AST_LITEM]=&render_t::itemList;
	handler[AST_TDATA]=&render_t::texture;
	handler[AST_SDATA]=&render_t::shader;
	handler[AST_OBJECT]=&render_t::object;
	handler[AST_MESH]=&render_t::mesh;
	handler[AST_SPHERE]=&render_t::sphere;
	handler[AST_CAMERA]=&render_t::camera;
	handler[AST_RENDER]=&render_t::render;
	handler[AST_LIGHT]=&render_t::light;
	handler[AST_FILTER]=&render_t::filter;
	handler[AST_TRANS]=&render_t::transform;
	handler[AST_BACKG]=&render_t::background;

	loadPlugins(pluginpath);
	cachedPathLight=false;
}

render_t::~render_t()
{
#define MAPOF(type) map<string,type *>
#define FREEMAP(type,name)\
	for(MAPOF(type)::iterator i=name.begin();i!=name.end();++i) delete i->second

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

void render_t::loadPlugins(const string &path)
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

void * render_t::itemList(ast_t *ast)
{
	litem_data_t *litem=(litem_data_t *)ast;
	list<ast_t *> &li=litem->l;
	for(list<ast_t *>::iterator i=li.begin();i!=li.end();++i)
		call(*i);
	return NULL;
}

void * render_t::transform(ast_t *ast)
{
	transform_data_t *t=(transform_data_t *)ast;
	matrix4x4_t old=M;
	M.identity();

	t->params.getParam("m00",M[0][0]);
	t->params.getParam("m01",M[0][1]);
	t->params.getParam("m02",M[0][2]);
	t->params.getParam("m03",M[0][3]);
	t->params.getParam("m10",M[1][0]);
	t->params.getParam("m11",M[1][1]);
	t->params.getParam("m12",M[1][2]);
	t->params.getParam("m13",M[1][3]);
	t->params.getParam("m20",M[2][0]);
	t->params.getParam("m21",M[2][1]);
	t->params.getParam("m22",M[2][2]);
	t->params.getParam("m23",M[2][3]);
	t->params.getParam("m30",M[3][0]);
	t->params.getParam("m31",M[3][1]);
	t->params.getParam("m32",M[3][2]);
	t->params.getParam("m33",M[3][3]);
	M=old*M;
	itemList(t->litem);
	M=old;
	return NULL;
}

void * render_t::texture(ast_t *ast)
{
	generic_data_t *tex=(generic_data_t *)ast;
	texture_t *ntex=NULL;;
	string _name,_type;
	const string *name=&_name,*type=&_type;
	paramMap_t &params=tex->params;

	params.getParam("name",name);
	params.getParam("type",type);
	if(*name=="") return NULL;

	map<string,texture_factory_t *>::iterator i=texture_factory.find(*type);
	if(i!=texture_factory.end()) 
		ntex=i->second(params,*this);
	else WARNING<<"Texture "<<*type<<" not found"<<endl;

	params.checkUnused("texture");

	if (ntex==NULL) return NULL;
	if(texture_table.find(*name)!=texture_table.end())
	{
		WARNING<<"Texture "<<*name<<" redefined\n";
		delete texture_table[*name];
	}
	texture_table[*name]=ntex;

	INFO<<"Added texture "<<*name<<endl;
	return ntex;
}

/*
texture_t * render_t::texture_image(paramMap_t &params)
{
	string name;
	params.getParam("filename", name);
	if (name=="")
		WARNING << "Required argument filename not found for image texture\n";
	else
		return new textureImage_t(name.c_str());
	return NULL;
}


texture_t * render_t::texture_clouds(paramMap_t &params)
{
	color_t color1(0.0),color2(1.0);
	int depth = 2;
	params.getParam("color1", color1);
	params.getParam("color2", color2);
	params.getParam("depth", depth);
	return new textureClouds_t(depth, color1, color2);
}


texture_t * render_t::texture_marble(paramMap_t &params)
{
	color_t col1(0.0), col2(1.0);
	int oct = 2;
	CFLOAT turb=1.0, shp=1.0;
	bool hrd = false;
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	params.getParam("depth", oct);
	params.getParam("turbulence", turb);
	params.getParam("sharpness", shp);
	params.getParam("hard", hrd);
	return new textureMarble_t(oct, col1, col2, turb, shp, hrd);
}


texture_t * render_t::texture_wood(paramMap_t &params)
{
	color_t col1(0.0), col2(1.0);
	int oct = 2;
	CFLOAT turb = 1.0;
	PFLOAT rx=1, ry=1;
	bool hrd = false;
	params.getParam("color1", col1);
	params.getParam("color2", col2);
	params.getParam("depth", oct);
	params.getParam("turbulence", turb);
	params.getParam("ringscale_x", rx);
	params.getParam("ringscale_y", ry);
	params.getParam("hard", hrd);
	return new textureWood_t(oct, col1, col2, turb, rx, ry, hrd);
}
*/

void * render_t::shader(ast_t *ast)
{

	shader_data_t *shader=(shader_data_t *)ast;
	paramMap_t &params=shader->params;

	string _name,_type;
	const string *name=&_name,*type=&_type;
	shader_t *ns=NULL;

	params.getParam("name",name);
	params.getParam("type",type);
	if(*name=="") return NULL;

	list<paramMap_t> lparams;
	for(list<generic_data_t *>::iterator i=shader->lmod->l.begin();
			i!=shader->lmod->l.end();++i)
		lparams.push_back((*i)->params);

	map<string,shader_factory_t *>::iterator oi=shader_factory.find(*type);
	if(oi!=shader_factory.end()) 
		ns=oi->second(params,lparams,*this);
	else
	{
		WARNING<<"Unknown shader type "<<*type<<endl;
		return NULL;
	}
	
	if(ns==NULL) WARNING<<"Wrong shader definition for "<<*name<<endl;
	if(ns==NULL) return NULL;
	params.checkUnused("shader");
	if(shader_table.find(*name)!=shader_table.end())
	{
		WARNING<<"Shader "<<*name<<" redefined\n";
		delete shader_table[*name];
	}
	shader_table[*name]=ns;
	INFO<<"Added shader "<<*name<<endl;
	return ns;

}

/*
shader_t * render_t::shader_generic(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	color_t color(0.0),specular(0.0),reflected(0.0),transmitted(0.0);
	CFLOAT hard=1.0,ior=1.0,minref=0.0;
	bool fast_fresnel=false;
	// new fresnel angle dependent colors
	color_t reflected2(0.0), transmitted2(0.0);

	bparams.getParam("color", color);
	bparams.getParam("specular", specular);
	bparams.getParam("reflected", reflected);
	if (bparams.getParam("transmited", transmitted))
		WARNING << "Use transmitted instead of transmited\n";
	else bparams.getParam("transmitted", transmitted);
	bparams.getParam("hard", hard);
	bparams.getParam("IOR", ior);
	bparams.getParam("min_refle", minref);
	bparams.getParam("fast_fresnel", fast_fresnel);

	// if not specified, make second fresnel color = first fresnel color
	if (!bparams.getParam("reflected2", reflected2)) reflected2 = reflected;
	if (!bparams.getParam("transmitted2", transmitted2)) transmitted2 = transmitted;

	genericShader_t *ns=new genericShader_t(color,hard, 1.0, reflected, transmitted,
						specular, reflected2, transmitted2,
						minref, 1.0, ior, fast_fresnel);
	for(list<generic_data_t *>::iterator i=lmod.begin();i!=lmod.end();++i)
	{
		string texname;
		GFLOAT size=1,sizex=1,sizey=1,sizez=1;
		CFLOAT color=0,specular=0,hard=0,trans=0,refle=0,displace=0;
		string mode="mix", mapping="none";

		paramMap_t &params=(*i)->params;

		params.getParam("texname",texname);
		if(texname=="") continue;
		if(texture_table.find(texname)==texture_table.end())
		{
			INFO<<"Undefined texture : "<<texname<<endl;
			continue;
		}
		params.getParam("size", size);
		params.getParam("sizex", sizex);
		params.getParam("sizey", sizey);
		params.getParam("sizez", sizez);
		params.getParam("color", color);
		params.getParam("specular", specular);
		params.getParam("hard", hard);
		params.getParam("transmission", trans);
		params.getParam("reflection", refle);
		params.getParam("normal", displace);
		params.getParam("mode", mode);
		params.getParam("mapping", mapping);

		modulator_t modu(texture_table[texname]);
		modu.sizeX(sizex);
		modu.sizeY(sizey);
		modu.sizeZ(sizez);
		if(size!=1.0) modu.size(size);
		if(mode=="mix") modu.mode(MIX);
		if(mode=="mul") modu.mode(MUL);
		if(mode=="add") modu.mode(ADD);
		modu.color(color);
		modu.specular(specular);
		modu.hard(hard);
		modu.transmision(trans);
		modu.reflection(refle);
		modu.displace(displace);
		modu.string2maptype(mapping);
		ns->addModulator(modu);
		params.checkUnused("modulator");
	}
	return ns;
}

shader_t * render_t::shader_constant(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	color_t color(0.0);
	bparams.getParam("color",color);
	constantShader_t *ns=new constantShader_t(color);
	return ns;
}

// makehuman
shader_t * render_t::shader_skin(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	color_t ka(0.0);//put here default value
	color_t sheencolor(0.0);//default
	float eta=1.0;//default
	float thickness=0.0;//default
	
	bparams.getParam("ka",ka);//if not given, var is untouched
	bparams.getParam("sheencolor",sheencolor);
	bparams.getParam("eta",eta);
	bparams.getParam("thickness",thickness);
	
	
	shader_t *ns=NULL;
	return ns;
}

shader_t * render_t::shader_f2c(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string inputs;
	shader_t *input=NULL;
	bparams.getParam("input",inputs);
	if(shader_table.find(inputs)!=shader_table.end()) 
		input=shader_table[inputs];
	if(input!=NULL)
		return new floatToColor_t(input);
	else return NULL;
}

shader_t * render_t::shader_c2f(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string inputs;
	shader_t *input=NULL;
	bparams.getParam("input",inputs);
	if(shader_table.find(inputs)!=shader_table.end()) 
		input=shader_table[inputs];
	if(input!=NULL)
		return new colorToFloat_t(input);
	else return NULL;
}

shader_t * render_t::shader_cband(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string inputs;
	shader_t *input=NULL;
	bparams.getParam("input",inputs);
	if(shader_table.find(inputs)!=shader_table.end()) 
		input=shader_table[inputs];
	if(input==NULL) 
	{
		WARNING<<"Input shader for colorband not found\n";
		return NULL;
	}
	
	vector<pair<CFLOAT,color_t> > band;
	for(list<generic_data_t *>::iterator i=lmod.begin();i!=lmod.end();++i)
	{
		pair<CFLOAT,color_t> par;
		paramMap_t &params=(*i)->params;
		params.getParam("value",par.first);
		params.getParam("color",par.second);
		band.push_back(par);
	}
	shader_t *sha= new colorBandNode_t(band,input);
	return sha;
}

shader_t * render_t::shader_coords(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	int w=0;
	string var;
	bparams.getParam("coord",var);
	if(var=="X") w=0;
	if(var=="Y") w=1;
	if(var=="Z") w=2;
	return new coordsNode_t(w);
}

shader_t * render_t::shader_mul(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in1,in2;
	shader_t *input1=NULL,*input2=NULL;
	CFLOAT val=1.0;
	bparams.getParam("input1",in1);
	if(shader_table.find(in1)!=shader_table.end()) 
		input1=shader_table[in1];
	bparams.getParam("input2",in2);
	if(shader_table.find(in2)!=shader_table.end()) 
		input2=shader_table[in2];
	bparams.getParam("value",val);
	return new mulNode_t(input1,input2,val);
}

shader_t * render_t::shader_sin(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in;
	shader_t *input=NULL;
	bparams.getParam("input",in);
	if(shader_table.find(in)!=shader_table.end()) 
	{
		input=shader_table[in];
		return new sinNode_t(input);
	}
	else return NULL;
}

shader_t * render_t::shader_phong(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string c,s,e,cr,ct;
	CFLOAT hard=1.0,ior=1.0;
	shader_t *color=NULL,*specular=NULL,*env=NULL,*causr=NULL,*caust=NULL;
	bparams.getParam("color",c);
	bparams.getParam("specular",s);
	bparams.getParam("environment",e);
	bparams.getParam("caus_rcolor",cr);
	bparams.getParam("caus_tcolor",ct);
	bparams.getParam("hard",hard);
	bparams.getParam("IOR",hard);
	if(shader_table.find(c)!=shader_table.end()) color=shader_table[c];
	if(shader_table.find(s)!=shader_table.end()) specular=shader_table[s];
	if(shader_table.find(e)!=shader_table.end()) env=shader_table[e];
	if(shader_table.find(cr)!=shader_table.end()) causr=shader_table[cr];
	if(shader_table.find(ct)!=shader_table.end()) caust=shader_table[ct];
	
	if((c!="") && (color==NULL)) WARNING<<"Input shader "<<c<<" not found\n";
	if((s!="") && (specular==NULL)) WARNING<<"Input shader "<<s<<" not found\n";
	if((e!="") && (env==NULL)) WARNING<<"Input shader "<<e<<" not found\n";
	if((cr!="") && (causr==NULL)) WARNING<<"Input shader "<<cr<<" not found\n";
	if((ct!="") && (caust==NULL)) WARNING<<"Input shader "<<ct<<" not found\n";

	return new phongNode_t(color,specular,env,causr,caust,hard,ior);
}

shader_t * render_t::shader_clouds(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in1,in2;
	CFLOAT size;
	int dep;
	shader_t *input1=NULL,*input2=NULL;
	bparams.getParam("input1",in1);
	bparams.getParam("input2",in2);
	bparams.getParam("size",size);
	bparams.getParam("depth",dep);
	if(shader_table.find(in1)!=shader_table.end()) input1=shader_table[in1];
	if(shader_table.find(in2)!=shader_table.end()) input2=shader_table[in2];

	return new cloudsNode_t(size,dep,input1,input2);
}


shader_t * render_t::shader_marble(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in1, in2;
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

	if(shader_table.find(in1)!=shader_table.end()) input1=shader_table[in1];
	if(shader_table.find(in2)!=shader_table.end()) input2=shader_table[in2];

	return new marbleNode_t(size, dep, turb, shp, hrd, input1, input2);
}


shader_t * render_t::shader_wood(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in1, in2;
	CFLOAT size = 1;
	int dep = 2;
	CFLOAT turb = 1;
	PFLOAT rx=1, ry=1;
	bool hrd = false;
	shader_t *input1=NULL, *input2=NULL;

	bparams.getParam("input1", in1);
	bparams.getParam("input2", in2);
	bparams.getParam("size", size);
	bparams.getParam("depth", dep);
	bparams.getParam("turbulence", turb);
	bparams.getParam("ringscale_x", rx);
	bparams.getParam("ringscale_y", ry);
	bparams.getParam("hard", hrd);

	if(shader_table.find(in1)!=shader_table.end()) input1=shader_table[in1];
	if(shader_table.find(in2)!=shader_table.end()) input2=shader_table[in2];

	return new woodNode_t(size, dep, turb, rx, ry, hrd, input1, input2);
}

shader_t * render_t::shader_RGB(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in1,in2,in3;
	shader_t *inputred=NULL,*inputgreen=NULL,*inputblue=NULL;
	
	bparams.getParam("inputred",in1);
	if(shader_table.find(in1)!=shader_table.end()) 
		inputred=shader_table[in1];
	
	bparams.getParam("inputgreen",in2);
	if(shader_table.find(in2)!=shader_table.end()) 
		inputgreen=shader_table[in2];
	
	bparams.getParam("inputblue",in3);
	if(shader_table.find(in3)!=shader_table.end()) 
		inputblue=shader_table[in3];
	
	color_t color(0.0);
	bparams.getParam("color",color);
	
	return new rgbNode_t(inputred,inputgreen,inputblue,color);
}

shader_t * render_t::shader_HSV(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string in1,in2,in3;
	shader_t *inputhue=NULL,*inputsaturation=NULL,*inputvalue=NULL;
	
	bparams.getParam("inputhue",in1);
	if(shader_table.find(in1)!=shader_table.end()) 
		inputhue=shader_table[in1];
	
	bparams.getParam("inputsaturation",in2);
	if(shader_table.find(in2)!=shader_table.end()) 
		inputsaturation=shader_table[in2];
	
	bparams.getParam("inputvalue",in3);
	if(shader_table.find(in3)!=shader_table.end()) 
		inputvalue=shader_table[in3];
	
	CFLOAT hue=1.0;
	bparams.getParam("hue",hue);
	
	CFLOAT saturation=1.0;
	bparams.getParam("saturation",saturation);
	
	CFLOAT value=1.0;
	bparams.getParam("value",value);
	
	return new hsvNode_t(inputhue,inputsaturation,inputvalue,hue,saturation,value);
}

shader_t * render_t::shader_conetrace(paramMap_t &bparams,list<generic_data_t *> &lmod)
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

	int s=(int)sqrt((float)samples*samples);
	if(s!=samples)
		WARNING<<"Using "<<s<<" samples in conetrace instead of "<<samples<<endl;
	
	return new coneTraceNode_t(color,angle,s,IOR,ref);
}

shader_t * render_t::shader_fresnel(paramMap_t &bparams,list<generic_data_t *> &lmod)
{
	string inR,inT;
	shader_t *inputR=NULL,*inputT=NULL;
	PFLOAT ior=1.0;
	CFLOAT minr=0.0;
	bparams.getParam("reflected",inR);
	if(shader_table.find(inR)!=shader_table.end()) 
		inputR=shader_table[inR];
	bparams.getParam("transmitted",inT);
	if(shader_table.find(inT)!=shader_table.end()) 
		inputT=shader_table[inT];
	bparams.getParam("IOR",ior);
	bparams.getParam("min_refle",minr);
	return new fresnelNode_t(inputR,inputT,ior,minr);
}
*/
void * render_t::object(ast_t *ast)
{
	object_data_t *object=(object_data_t *)ast;
	bool isref=object->original!="";

	if(object->name=="") return NULL;
	if( !isref && (object->geometry==NULL))
	{
		INFO<<"Changing transformation matrix of "<<object->name<<endl;
		if(object_table.find(object->name)!=object_table.end())
			object_table[object->name]->transform(M);
		else WARNING<<"Object "<<object->name<<" undefined\n";
		return NULL;
	}

	if(!isref)
	{
		if(object->shader=="") return NULL;
		if(shader_table.find(object->shader)==shader_table.end())
		{
			ERRORMSG<<"undefined shader "<<object->shader<<" object ignored"<<endl;
			return NULL;
		}
	}
	
	object3d_t *obj=NULL;
	if(isref) 
	{
		if((object_table.find(object->original)==object_table.end())
				|| (object->original==object->name))
			WARNING<<"Object "<<object->name<<" undefined\n";
		else
			obj=referenceObject_t::factory(M,object_table[object->original]);
	}
	else
		obj=(object3d_t *)call(object->geometry);
	if(obj==NULL) return NULL;
	if(object_table.find(object->name)!=object_table.end())
	{
		WARNING<<"Object "<<object->name<<" redefined\n";
		delete object_table[object->name];
	}
	// error was here, don't reset vars when object is reference, already set from original object
	if (!isref) {
		obj->castShadows(object->shadow);
		obj->useForRadiosity(object->emit_rad);
		obj->reciveRadiosity(object->recv_rad);
		obj->caustics(object->caustics);
		color_t rcolor(object->caus_rcolor.r,
				object->caus_rcolor.g,
				object->caus_rcolor.b);
		color_t tcolor(object->caus_tcolor.r,
				object->caus_tcolor.g,
				object->caus_tcolor.b);
		obj->setCaustic(rcolor, tcolor, object->caus_IOR);
		obj->setShader(shader_table[object->shader]);
	}
	object_table[object->name]=obj;
	INFO<<"Added object "<<object->name<<endl;
	return obj;
}

void * render_t::mesh(ast_t *ast)
{
	mesh_data_t *mesh=(mesh_data_t *)ast;
	vector<triangle_t> &faces=mesh->faces->faces;
	vector<shader_t *> shaders;
	for(vector<string>::iterator i=mesh->faces->shaders.begin();
			i!=mesh->faces->shaders.end();++i)
	{
		if(shader_table.find(*i)==shader_table.end())
		{
			ERRORMSG<<"Undefined shader "<<*i<<endl;
			shaders.push_back(NULL);
		}
		else shaders.push_back(shader_table[*i]);
	}
	for(vector<triangle_t>::iterator i=faces.begin();i!=faces.end();++i)
	{
		long int n=(long int)((*i).a);
		if((n>=(long int)mesh->points->points.size()) || (n<0))
			{ WARNING<<"Point "<<n<<" out of bounds in object\n"; n=0; }
		(*i).a=&(mesh->points->points)[n];
		n=(long int)((*i).b);
		if((n>=(long int)mesh->points->points.size()) || (n<0))
			{ WARNING<<"Point "<<n<<" out of bounds in object\n"; n=0; }
		(*i).b=&(mesh->points->points)[n];
		n=(long int)((*i).c);
		if((n>=(long int)mesh->points->points.size()) || (n<0))
			{ WARNING<<"Point "<<n<<" out of bounds in object\n"; n=0; }
		(*i).c=&(mesh->points->points)[n];
		
		(*i).recNormal();
		long int ishader=(long int)(*i).getShader();
		if(ishader<0) 
			(*i).setShader(NULL);
		else
			(*i).setShader(shaders[ishader]);
	}
	int uv_offset=0, vcol_offset=0;
	for(vector<triangle_t>::iterator i=mesh->faces->faces.begin();
				i!=mesh->faces->faces.end();++i) {
		if(i->hasuv)
		{
			i->uv = mesh->faces->facesuv.begin() + uv_offset;
			uv_offset += 6;
		}
		if(i->has_vcol)
		{
			i->vcol = mesh->faces->faces_vcol.begin() + vcol_offset;
			vcol_offset += 9;
		}
	}
	vector<vector3d_t> normals;
	meshObject_t *obj = meshObject_t::factory(mesh->orco, M, mesh->points->points, normals,
						mesh->faces->faces, mesh->faces->facesuv, mesh->faces->faces_vcol);

	if(mesh->autosmooth) obj->autoSmooth(mesh->angle);
	obj->tangentsFromUV();
	
	return obj;
}

void * render_t::sphere(ast_t *ast)
{
	/*
	sphere_data_t *sphere=(sphere_data_t *)ast;
	sphere_t *sp=new sphere_t(M*(sphere->center),sphere->radius);
	return sp;
	*/
	return NULL;
}


void * render_t::light(ast_t *ast)
{
	generic_data_t *light=(generic_data_t *)ast;
	string _name,_type;
	const string *name=&_name,*type=&_type;
	bool render=true,indirect=true;
	light_t *l=NULL;

	light->params.getParam("name",name);
	light->params.getParam("type",type);
	light->params.getParam("use_in_render",render);
	light->params.getParam("use_in_indirect",indirect);

	if(*name=="") return NULL;

	map<string,light_factory_t *>::iterator i=light_factory.find(*type);
	if(i!=light_factory.end()) 
		l=i->second(light->params,*this);
	
	light->params.checkUnused("light");

	if(l==NULL) return NULL;

	l->useInRender(render);
	l->useInIndirect(indirect);

	if(light_table.find(*name)!=light_table.end())
	{
		WARNING<<"Light "<<*name<<" redefined\n";
		delete light_table[*name];
	}
	light_table[*name]=l;
	INFO<<"Added "<<*type<<" light "<<*name<<endl;

	return NULL;
}

void * render_t::camera(ast_t *ast)
{
	camera_data_t *camera=(camera_data_t *)ast;
	if (camera->name=="") return NULL;
	// camera type
	camera_t::cameraType ct = camera_t::CM_PERSPECTIVE;
	if (camera->camtype=="ortho")
		ct = camera_t::CM_ORTHO;
	else if (camera->camtype=="spherical")
		ct = camera_t::CM_SPHERICAL;
	else if (camera->camtype=="lightprobe")
		ct = camera_t::CM_LIGHTPROBE;
	// bokeh type
	camera_t::bokehType bt = camera_t::BK_DISK1;
	if (camera->bkhtype=="disk2")
		bt = camera_t::BK_DISK2;
	else if (camera->bkhtype=="triangle")
		bt = camera_t::BK_TRI;
	else if (camera->bkhtype=="square")
		bt = camera_t::BK_SQR;
	else if (camera->bkhtype=="pentagon")
		bt = camera_t::BK_PENTA;
	else if (camera->bkhtype=="hexagon")
		bt = camera_t::BK_HEXA;
	else if (camera->bkhtype=="ring")
		bt = camera_t::BK_RING;
	// bokeh bias
	camera_t::bkhBiasType bbt = camera_t::BB_NONE;
	if (camera->bkhbias=="center")
		bbt = camera_t::BB_CENTER;
	else if (camera->bkhbias=="edge")
		bbt = camera_t::BB_EDGE;
	camera_t *cam=new camera_t(camera->from, camera->to, camera->up,
														camera->resx, camera->resy, camera->aspect, camera->dfocal,
														camera->laperture, camera->dofdist, camera->dof_use_qmc,
														ct, bt, bbt, camera->bkh_ro);
	if (camera_table.find(camera->name)!=camera_table.end())
	{
		WARNING << "Camera " << camera->name << " redefined\n";
		delete camera_table[camera->name];
	}
	camera_table[camera->name]=cam;
	INFO << "Added camera " << camera->name << endl;
	return cam;

}


void * render_t::filter(ast_t *ast)
{
	generic_data_t *filter=(generic_data_t *)ast;
	string _name,_type;
	const string *name=&_name,*type=&_type;
	filter_t *f=NULL;

	filter->params.getParam("name",name);
	filter->params.getParam("type",type);

	if(*name=="") return NULL;

	if(*type=="dof") 	f=filter_dof(filter->params);
	if(*type=="antinoise") f=filter_antinoise(filter->params);

	filter->params.checkUnused("filter");

	if(f==NULL) return NULL;

	if(filter_table.find(*name)!=filter_table.end()) 
	{
		WARNING<<"Filter "<<*name<<" redefined\n";
		delete filter_table[*name];
	}
	filter_table[*name]=f;

	INFO<<"Added "<<*type<<" filter "<<*name<<endl;
	return f;
}

filter_t * render_t::filter_dof(paramMap_t &params)
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

filter_t *render_t::filter_antinoise(paramMap_t &params)
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


void * render_t::background(ast_t *ast)
{
	generic_data_t *background=(generic_data_t *)ast;
	string _name,_type;
	const string *name=&_name,*type=&_type;
	background_t *b=NULL;

	background->params.getParam("name",name);
	background->params.getParam("type",type);

	if (*name=="") return NULL;

	map<string,background_factory_t *>::iterator i=background_factory.find(*type);
	if(i!=background_factory.end()) 
		b=i->second(background->params,*this);
	else
	{
	//if (type=="constant")
	//	b = background_const(background->params);
	//else if (type=="sunsky")
	//	b = background_sunsky(background->params);
	//else if (type=="HDRI")
	//	b = background_HDRI(background->params);
	//else if (type=="image")
	//	b = background_image(background->params);
		WARNING << "Wrong type for background " << *type << endl;
		return NULL;
	}
	background->params.checkUnused("background");

	if(b==NULL) return NULL;

	if(background_table.find(*name)!=background_table.end())
	{
		WARNING << "background " << *name << " redefined\n";
		delete background_table[*name];
	}
	background_table[*name]=b;

	INFO << "Added " << *type << " background " << *name << endl;
	return b;
}

/*
// constant color background
background_t * render_t::background_const(paramMap_t &params)
{
	color_t color(1.0);
	params.getParam("color", color);
	return new constBackground_t(color);
}


// sunsky background
background_t * render_t::background_sunsky(paramMap_t &params)
{
	point3d_t dir(1,1,1);	// same as sunlight, position interpreted as direction
	CFLOAT turb = 4.0;	// turbidity of atmosphere
	bool add_sun = false;	// automatically add real sunlight
	PFLOAT pw = 1.0;	// sunlight power
	PFLOAT av, bv, cv, dv, ev;
	av = bv = cv = dv = ev = 1.0;	// color variation parameters, default is normal

	params.getParam("from", dir);
	params.getParam("turbidity", turb);

	// new color variation parameters
	params.getParam("a_var", av);
	params.getParam("b_var", bv);
	params.getParam("c_var", cv);
	params.getParam("d_var", dv);
	params.getParam("e_var", ev);

	// create sunlight with correct color and position?
	params.getParam("add_sun", add_sun);
	params.getParam("sun_power", pw);

	background_t * new_sunsky = new sunskyBackground_t(dir, turb, av, bv, cv, dv, ev);

	if (add_sun) {
		color_t suncol = (*new_sunsky)(vector3d_t(dir.x, dir.y, dir.z));
		light_t * sunsky_SUN = new sunLight_t(dir, suncol, pw, true);

		string name = "sunsky_SUN";
		// if light already in table with same name, append 'X' until unique
		while (light_table.find(name)!=light_table.end())
			name += "X";

		light_table[name] = sunsky_SUN;
		INFO << "(background_sunsky) Added sunlight " << name << endl;
	}

	return new_sunsky;
}


background_t * render_t::background_HDRI(paramMap_t &params)
{
  string filename;
  int expadj = 0;	// changes actual pixel exponent, can be negative
  string mapping = "probe";
  params.getParam("exposure_adjust", expadj);
  params.getParam("filename", filename);
  params.getParam("mapping", mapping);
  // two mapping types only at the moment, probe or sphere
  bool mapprobe = (mapping=="probe");
  if (filename=="") {
    INFO << "(background_HDRI) Error,  No filename given\n";
    return NULL;
  }
  return new HDRI_Background_t(filename.c_str(), expadj, mapprobe);
}


background_t * render_t::background_image(paramMap_t &params)
{
  string filename;
  CFLOAT pwr = 1.0;	// here just a brightness multiplier, same as for lights
  params.getParam("power", pwr);
  params.getParam("filename", filename);
  if (filename=="") {
    INFO << "(background_image) Error,  No filename given\n";
    return NULL;
  }
  return new imageBackground_t(filename.c_str(), pwr);
}
*/

void * render_t::render(ast_t *ast)
{
	//render_data_t *rend=(render_data_t *)ast;
	generic_data_t *rend=(generic_data_t *)ast;
	paramMap_t &params=rend->params;

	string _camera, _outfile="salida.tga",_background;
	const string *camera=&_camera, *outfile=&_outfile, *background=&_background;
	int raydepth = 3;
	PFLOAT bias = 0.1;
	bool save_alpha = false;

	params.getParam("camera_name", camera);
	params.getParam("outfile", outfile);
	params.getParam("save_alpha", save_alpha);
	params.getParam("raydepth", raydepth);
	params.getParam("bias", bias);
	params.getParam("background_name", background);

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
		return NULL;
	}
	if(camera_table.find(*camera)==camera_table.end())
	{
		ERRORMSG<<"Camera "<<*camera<<" does not exist\n";
		return NULL;
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

	scene_t *scene;
	switch (strategy) {
		case MONO:
			scene = scene_t::factory();
			break;
#if HAVE_PTHREAD
		case THREAD:
#if HAVE_PTHREAD
			scene = threadedscene_t::factory();
#else
			scene = scene_t::factory();
#endif
			//scene = new threadedscene_t;
			break;
#endif
		case FORK:
			scene = scene_t::factory();
			//scene = new forkedscene_t;
			break;
		default:
			scene = scene_t::factory();
			break;
	}

	camera_t *cam=camera_table[*camera];
	scene->setCamera(cam);

	for(map<string,object3d_t *>::iterator i=object_table.begin();
			i!=object_table.end();++i)
		scene->addObject((*i).second);

	for(map<string,light_t *>::iterator i=light_table.begin();
			i!=light_table.end();++i)
		scene->addLight((*i).second);

	for(map<string,filter_t *>::iterator i=filter_table.begin();
			i!=filter_table.end();++i)
		scene->addFilter((*i).second);

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

	scene->setMaxRayDepth(raydepth);

	if(*background!="")
	{
		if(background_table.find(*background)==background_table.end())
			WARNING<<"Background "<<*background<<" does not exist\n";
		else
			scene->setBackground(background_table[*background]);
	}

	// alpha premultiply
	if (*alpha_premul=="on")
		scene->alphaPremultiply(true);
	else
		scene->alphaPremultiply(false);
	// alpha background
	if (*alpha_mbg=="on")
		scene->alphaMaskBackground(true);
	else
		scene->alphaMaskBackground(false);

	// gamma & exposure
	scene->setExposure(exposure);
	scene->setGamma(gamma);
	// fog
	scene->setFog(fgdens, fgcol);

	// set the AA params
	scene->setAASamples(AA_passes, AA_minsamples, AA_pixelwidth, AA_threshold, AA_jitterfirst);
	scene->clampRGB(clamp_rgb);

	scene->setBias(bias);
	if(cachedPathLight) scene->setRepeatFirst();

	scene->setRegion(scxmin,scxmax,scymin,scymax);
	scene->setCPUs(cpus);

	// tone mapping bypassed when hdr/exr output is requested
	if (*output_type=="hdr") {
		outHDR_t hdrout(cam->resX(), cam->resY(), outfile->c_str());
		scene->tonemap(false);
		scene->render(hdrout);
		hdrout.flush();
	}
	else if (int((*output_type).find("exr"))!=-1) {
#if HAVE_EXR
		outEXR_t exrout(cam->resX(), cam->resY(), outfile->c_str(), *exr_flags);
		scene->tonemap(false);
		scene->render(exrout);
		exrout.flush();
#else
		cout << "Yafray was compiled without OpenEXR support.\nImage saved in hdr format instead." << endl;
		outHDR_t hdrout(cam->resX(), cam->resY(), outfile->c_str());
		scene->tonemap(false);
		scene->render(hdrout);
		hdrout.flush();
#endif
	}
	else {
		outTga_t tgaout(cam->resX(), cam->resY(), outfile->c_str(), save_alpha);
		scene->tonemap(true);
		scene->render(tgaout);
		tgaout.flush();
	}

	delete scene;

	return NULL;
}

shader_t *render_t::getShader(const std::string name)const
{
	map<string,shader_t *>::const_iterator i=shader_table.find(name);
	if(i!=shader_table.end()) return i->second;
	else return NULL;
}

texture_t *render_t::getTexture(const std::string name)const
{
	map<string,texture_t *>::const_iterator i=texture_table.find(name);
	if(i!=texture_table.end()) return i->second;
	else return NULL;
}

void render_t::repeatFirstPass()
{
	cachedPathLight=true;
}

void render_t::registerFactory(const std::string &name,light_factory_t *f)
{
	light_factory[name]=f;
}

void render_t::registerFactory(const std::string &name,shader_factory_t *f)
{
	shader_factory[name]=f;
}

void render_t::registerFactory(const std::string &name,texture_factory_t *f)
{
	texture_factory[name]=f;
}

void render_t::registerFactory(const std::string &name,filter_factory_t *f)
{
	filter_factory[name]=f;
}

void render_t::registerFactory(const std::string &name,background_factory_t *f)
{
	background_factory[name]=f;
}

__END_YAFRAY
