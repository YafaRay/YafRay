/****************************************************************************
 *
 *      msin.h: Yafray fileformat parser api
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
#ifndef __MSIN_H
#define __MSIN_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "parser.h"
#include "mlex.h"
/*
#include "texture.h"
#include "shader.h"
*/
#include "mesh.h"
#include"params.h"

#include <string>
#include <map>
#include <stdlib.h>

__BEGIN_YAFRAY

#ifndef foreach
#define foreach(i,tipo,seq) \
	for(tipo::iterator i=(seq).begin();i!=(seq).end();++i)
#define cforeach(i,tipo,seq) \
	for(tipo::const_iterator i=(seq).begin();i!=(seq).end();++i)
#endif



#define V_JPEG				1
#define V_CLOUDS			2


#define V_MIX  1
#define V_ADD  2
#define V_MUL  3

#define V_GENERIC			1
#define V_CONSTANT		2

#define A_TYPE  1
#define A_NAME  2
#define A_SHADER  3
#define A_SHADOW  4
#define A_EMITR   5
#define A_RECVR   6
#define A_CIOR    7

#define A_POWER   8
#define A_BIAS    9
#define A_SAMPLES 10
#define A_PSAMPLES 11
#define A_PHOTONS  12
#define A_DEPTH  13
#define A_DISP   14


#define A_CAMERA  15
#define A_FROM    16
#define A_TO      17
#define A_UP      18
#define A_RESX    19
#define A_RESY    20
#define A_FOCAL   21
// dof params
#define A_APERTURE 53
#define A_USE_QMC  54
#define A_DOFDIST  64
// camtype
#define A_CAMTYPE  65
// bokeh
#define A_BOKEH		66
#define A_BKHBIAS	67
#define A_BKHROT	68
// aspect ratio
#define A_ASPECT	69

#define A_ATTR   22
#define A_FACTOR 23
#define A_RADIUS 24

#define A_TEXTURE 25
#define A_SIZE    26
#define A_SIZEX   27
#define A_SIZEY   28
#define A_SIZEZ   29
#define A_MODE    30

#define A_T_A   31
#define A_T_B   32
#define A_T_C   33
#define A_T_UA  34
#define A_T_UB  35
#define A_T_UC  36
#define A_T_VA  37
#define A_T_VB  38
#define A_T_VC  39
// vertex colors
#define A_T_VCOL_A_R	55
#define A_T_VCOL_A_G	56
#define A_T_VCOL_A_B	57
#define A_T_VCOL_B_R	58
#define A_T_VCOL_B_G	59
#define A_T_VCOL_B_B	60
#define A_T_VCOL_C_R	61
#define A_T_VCOL_C_G	62
#define A_T_VCOL_C_B	63

#define A_CAUS_RCOLOR 40
#define A_CAUS_TCOLOR 41
#define A_ORIGINAL    42

#define V_COLOR				1
#define V_SPECULAR		2
#define V_HARD				3
#define V_TRANSMISION 4
#define V_REFLECTION 	5
#define V_DISPLACE		6
#define V_DOP         7
#define V_ANOISE      8

#define A_COLOR				42
#define A_SPECULAR		43
#define A_HARD				44
#define A_TRANSMITED  45
#define A_REFLECTED 	46
#define A_IOR       	47
#define A_MINREFLE    48

#define A_AUTOSMOOTH 	49
#define A_RES         50
#define A_ANGLE       51
#define A_SEARCH      52
#define A_ORCO        53

class ast_t
{
	public :
		int id;
		virtual ~ast_t() {};
};

inline void check_ast(ast_t *ast,int tipo)
{
	if(ast==NULL)
	{
		cout<<"FATAL : sintree expected "<<tipo<<" found NULL\n";
		exit(1);
	}
	if(ast->id!=tipo)
	{
		cout<<"FATAL : sintree expected "<<tipo<<" found "<<ast->id<<"\n";
		exit(1);
	}
}


#define AST_SDATA      1
#define AST_ATTRDATA   2
#define AST_LATTRDATA  3
#define AST_CDATA      4
#define AST_MODDATA    9
#define AST_LMODDATA  10
#define AST_LITEM     11
#define AST_LABEL     12
#define AST_LLABEL    13
#define AST_TDATA     14
#define AST_POINT     15
#define AST_LPOINT    16
#define AST_FACE      17
#define AST_LFACE     18
#define AST_MESH      19
#define AST_OATTR     20
#define AST_LOATTR    21
#define AST_OBJECT    22
#define AST_CAMERA    23
#define AST_RENDER    24
#define AST_PLIGHT    25
#define AST_SPHERE    26
#define AST_CLABEL    27
#define AST_FILTER    31
#define AST_TRANS     32
#define AST_LIGHT    33
#define AST_BACKG    34


#define MAX_AST 40


struct lval_t
{
	lval_t() {ast=NULL;};
	std::string text;
	ast_t *ast;
};


//GENERAL SINTAX
//

typedef struct
{
	char *str;
	int value;
}string_value_t;

template<class T,int LID>
class list_data_t : public ast_t
{
	public:
		list_data_t() {id=LID;};
		std::list<T *> l;
		virtual ~list_data_t()
		{
			foreach(i,typename std::list<T *>,l)
				delete *i;
		};
};

template<class T,int LID>
lval_t join_list(std::vector<lval_t>::iterator v)
{
	lval_t res;
	check_ast(v[0].ast,LID);
	check_ast(v[1].ast,LID-1);
	
	list_data_t<T,LID> *l=(list_data_t<T,LID> *)v[0].ast;
	T *a=(T *)v[1].ast;
	l->l.push_back(a);
	res.ast=l;
	return res;
}

template<class T,int LID>
lval_t join_empty_list(std::vector<lval_t>::iterator v)
{
	lval_t res;
	res.ast=new list_data_t<T,LID>;
	return res;
}


class attr_data_t : public ast_t
{
	public :
		attr_data_t() {id=AST_ATTRDATA;};
		~attr_data_t() {};
		std::string I;
		std::string D;
		float F;
		bool f;
};


typedef list_data_t<attr_data_t,AST_LATTRDATA> lattr_data_t;

class color_data_t : public ast_t
{
	public :
		color_data_t() {id=AST_CDATA;};
		~color_data_t() {};
		CFLOAT r,g,b,a;
};


class label_data_t : public ast_t
{
	public:
		label_data_t() {id=AST_LABEL;};
		std::string label_name;
		lattr_data_t *lattr;
		virtual ~label_data_t()
		{
			delete lattr;
		};
};

typedef list_data_t<label_data_t,AST_LLABEL> llabel_data_t;

int string_value(const string_value_t *values,const char *ello);
lval_t join_copy(std::vector<lval_t>::iterator);
lval_t join_copy1(std::vector<lval_t>::iterator);
lval_t join_null(std::vector<lval_t>::iterator);
#define join_lattr join_list<lattr_data_t,AST_LATTRDATA>
#define join_empty_lattr join_empty_list<lattr_data_t,AST_LATTRDATA>
//lval_t join_lattr(vector<lval_t>::iterator);
//lval_t join_empty_lattr(vector<lval_t>::iterator);
lval_t join_attr(std::vector<lval_t>::iterator);
lval_t join_attrf(std::vector<lval_t>::iterator);
lval_t join_color(std::vector<lval_t>::iterator);
lval_t join_colorA(std::vector<lval_t>::iterator);
lval_t join_label( std::vector<lval_t>::iterator v );
#define join_llabel join_list<label_data_t,AST_LLABEL>
#define join_empty_llabel join_empty_list<label_data_t,AST_LLABEL>
//lval_t join_llabel( vector<lval_t>::iterator v );
//lval_t join_empty_llabel( vector<lval_t>::iterator v );

color_data_t get_color(const std::list<attr_data_t *> &l);
point3d_t get_point(const std::list<attr_data_t *> &l);
bool label_is_color(label_data_t *label);
bool label_is_point(label_data_t *label);
bool label_is_value(label_data_t *label);

/*
#define TYPE_FLOAT  0
#define TYPE_STRING 1
#define TYPE_POINT  2
#define TYPE_COLOR  3
#define TYPE_NONE   -1

class parameter_t
{
	public:
		parameter_t(const std::string &s):type(TYPE_STRING),used(false),str(s) {};
		parameter_t(float f):type(TYPE_FLOAT),used(false),fnum(f) {};
		parameter_t(const color_data_t &c):type(TYPE_COLOR),used(false)
			,C(c.r,c.g,c.b) {};
		parameter_t(const point3d_t &p):type(TYPE_POINT),used(false),P(p) {};
		parameter_t():type(TYPE_NONE),used(false) {};
		const std::string 		&getStr() {used=true;return str;};
		float 					&getFnum() {used=true;return fnum;};
		const point3d_t &getP() {used=true;return P;};
		const color_t 	&getC() {used=true;return C;};
		int type;
		bool used;
	protected:
		std::string str;
		float fnum;
		point3d_t P;
		color_t C;
};

class paramMap_t : public map<std::string,parameter_t>
{
	public:
		bool getParam(const std::string &name,std::string &s)
		{if(includes(name,TYPE_STRING)) s=(*this)[name].getStr();
			else return false;return true;}
			
		bool getParam(const std::string &name,bool &b)
		{
			std::string str;
			if(includes(name,TYPE_STRING)) 
			{
				str=(*this)[name].getStr();
				if(str=="on") b=true;
				else if(str=="off") b=false;
				else return false;
			}
			else return false;
			return true;
		}

		bool getParam(const std::string &name,float &f)
		{if(includes(name,TYPE_FLOAT)) f=(*this)[name].getFnum();
			else return false;return true;}
			
		bool getParam(const std::string &name,double &f)
		{if(includes(name,TYPE_FLOAT)) f=(*this)[name].getFnum();
			else return false;return true;}
			
		bool getParam(const std::string &name,int &i)
		{if(includes(name,TYPE_FLOAT)) i=(int)(*this)[name].getFnum();
			else return false;return true;}

		bool getParam(const std::string &name,point3d_t &p)
		{if(includes(name,TYPE_POINT)) p=(*this)[name].getP();
			else return false;return true;}
			
		bool getParam(const std::string &name,color_t &c)
		{if(includes(name,TYPE_COLOR)) c=(*this)[name].getC();
			else return false;return true;}

		bool includes(const std::string &label,int type)
		{
			const_iterator i=find(label);
			if(i==end()) return false;
			if((*i).second.type!=type) return false;
			return true;
		}

		void checkUnused(const std::string &env)
		{
			for(const_iterator i=begin();i!=end();++i)
				if(!( (*i).second.used ))
					cout<<"[WARNING]:Unused param "<<(*i).first<<" in "<<env<<"\n";
		};

};
*/

class generic_data_t : public ast_t
{
	public:
		generic_data_t(int ide) {id=ide;};
		paramMap_t params;
		virtual ~generic_data_t() {};
};

lval_t join_generic( std::vector<lval_t>::iterator v );

// SHADER SINTAX
//

typedef list_data_t<generic_data_t,AST_LMODDATA> lmod_data_t;

class shader_data_t : public ast_t
{
	public:
		shader_data_t() {id=AST_SDATA;};
		paramMap_t params;
		lmod_data_t *lmod;
		virtual ~shader_data_t()
		{
			delete lmod;
		};
};

class litem_data_t : public ast_t
{
	public:
		litem_data_t() {id=AST_LITEM;};
		std::list<ast_t *> l;
		virtual ~litem_data_t()
		{
			foreach(i,std::list<ast_t *>,l)
				delete *i;
		};
};

class transform_data_t : public ast_t
{
	public:
		transform_data_t() {id=AST_TRANS;};
		paramMap_t params;
		litem_data_t *litem;
		virtual ~transform_data_t() 
		{
			delete litem;
		};
};


#define join_lmod join_list<generic_data_t,AST_LMODDATA>
#define join_empty_lmod join_empty_list<generic_data_t,AST_LMODDATA>
lval_t join_shader( std::vector<lval_t>::iterator v );
lval_t join_litem( std::vector<lval_t>::iterator v );
lval_t join_empty_litem( std::vector<lval_t>::iterator v );
lval_t join_transform( std::vector<lval_t>::iterator v );

//TEXTURE

/*
class tex_data_t : public ast_t
{
	public:
		tex_data_t() {id=AST_TDATA;};
		int type;
		string name;
		llabel_data_t *ltattr;
		virtual ~tex_data_t() { delete ltattr;};
};


lval_t join_tdata( vector<lval_t>::iterator v );

*/
// MESH

class point_data_t : public ast_t
{
	public:
		point_data_t() {id=AST_POINT;};
		std::string label;
		point3d_t P;
		virtual ~point_data_t() {};
};

class lpoint_data_t : public ast_t
{
	public:
		lpoint_data_t() {id=AST_LPOINT;};
		std::vector<point3d_t> points;
		virtual ~lpoint_data_t() {};
};


class face_data_t : public ast_t
{
	public:
		face_data_t() {id=AST_FACE;};
		triangle_t T;
		GFLOAT uv[6];
		CFLOAT vcol[9];
		string shader;
		virtual ~face_data_t() {};
};

class lface_data_t : public ast_t
{
	public:
		lface_data_t() {id=AST_LFACE;};
		std::vector<triangle_t> faces;
		std::vector<GFLOAT> facesuv;
		std::vector<CFLOAT> faces_vcol;
		std::vector<string> shaders;
		virtual ~lface_data_t() {};
};

class mesh_data_t : public ast_t
{
	public:
		mesh_data_t() {id=AST_MESH;};
		bool autosmooth;
		bool orco;
		PFLOAT angle;
		lpoint_data_t *points;
		lface_data_t *faces;
		virtual ~mesh_data_t()
		{
			delete points;
			delete faces;
		};
};

class sphere_data_t : public ast_t
{
	public:
		sphere_data_t() {id=AST_SPHERE;};
		point3d_t center;
		PFLOAT radius;
		virtual ~sphere_data_t() {};
};


class object_attr_t : public ast_t
{
	public:
		object_attr_t() {id=AST_OATTR;};
		int attr;
		color_data_t *color;
		virtual ~object_attr_t() {delete color;};
};

typedef list_data_t<object_attr_t,AST_LOATTR> lobject_attr_t;



class object_data_t : public ast_t
{
	public:
		object_data_t() {id=AST_OBJECT;};
		std::string name;
		std::string original;
		std::string shader;
		bool shadow;
		bool emit_rad;
		bool recv_rad;
		bool caustics;
		PFLOAT caus_IOR;
		color_data_t caus_rcolor;
		color_data_t caus_tcolor;
		ast_t *geometry;
		virtual ~object_data_t()
		{
			if(geometry!=NULL) delete geometry;
		};
};


lval_t join_point( std::vector<lval_t>::iterator v );
lval_t join_lpoint( std::vector<lval_t>::iterator v );
lval_t join_empty_lpoint( std::vector<lval_t>::iterator v );
lval_t join_points( std::vector<lval_t>::iterator v );

lval_t join_face( std::vector<lval_t>::iterator v );
lval_t join_lface( std::vector<lval_t>::iterator v );
lval_t join_empty_lface( std::vector<lval_t>::iterator v );
lval_t join_faces( std::vector<lval_t>::iterator v );

lval_t join_mesh( std::vector<lval_t>::iterator v );
lval_t join_sphere( std::vector<lval_t>::iterator v );
lval_t join_oattr( std::vector<lval_t>::iterator v );
#define join_loattr join_list<object_attr_t,AST_LOATTR>
#define join_empty_loattr join_empty_list<object_attr_t,AST_LOATTR>
lval_t join_object( std::vector<lval_t>::iterator v );

// CAMERA

class camera_data_t : public ast_t
{
	public:
		camera_data_t() { id=AST_CAMERA; }
		std::string name, camtype, bkhtype, bkhbias;
		point3d_t from, to, up;
		int resx, resy;
		GFLOAT dfocal, laperture, dofdist, bkh_ro, aspect;
		bool dof_use_qmc;
		virtual ~camera_data_t() {};
};
/*
class render_data_t : public ast_t
{
	public:
		render_data_t() {id=AST_RENDER;};
		string camera;
		int samples;
		int raydepth;
};
*/
/*

class filter_data_t : public ast_t
{
	public:
		filter_data_t() {id=AST_FILTER;};
		int type;
		string name;
		llabel_data_t *ltattr;
		virtual ~filter_data_t() { delete ltattr;};
};
*/

lval_t join_camera( std::vector<lval_t>::iterator v );
//lval_t join_render( vector<lval_t>::iterator v );
//lval_t join_filter( vector<lval_t>::iterator v );

//TESTING
//

__END_YAFRAY
#endif
