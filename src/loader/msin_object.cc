/****************************************************************************
 *
 *      msin_object.cc: object 3D construction parsing implementation
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

#include "msin.h"

__BEGIN_YAFRAY
#define WARNING cerr<<"[Warning]: "

lval_t join_point(vector<lval_t>::iterator v)
{
	lval_t res;
	point_data_t *np=new point_data_t;
	np->label=v[1].text;
	for(int i=0;i<3;++i)
	{
		check_ast(v[i+2].ast,AST_ATTRDATA);
		attr_data_t *attr=(attr_data_t *)v[i+2].ast;
		if(!attr->f)
		{
			WARNING<<"Only float attribute allowed in point\n";
			delete attr;
			continue;
		}
		if(attr->I.size()!=1)
		{
			WARNING<<"Unknown point attribute > "<<attr->I<<"\n";
			delete attr;
			continue;
		}
		char c=attr->I[0];
		switch(c)
		{
			case 'x' : np->P.x=attr->F;break;
			case 'y' : np->P.y=attr->F;break;
			case 'z' : np->P.z=attr->F;break;
			default : WARNING<<"Unknown point attribute > "<<c<<"\n";
		}
		delete attr;
	}
	res.ast=np;
	return res;
}


lval_t join_lpoint(vector<lval_t>::iterator v)
{
	lval_t res;
	check_ast(v[0].ast,AST_LPOINT);
	check_ast(v[1].ast,AST_POINT);
	lpoint_data_t *lp=(lpoint_data_t *)v[0].ast;
	point_data_t *p=(point_data_t *)v[1].ast;

	lp->points.push_back(p->P);
	delete p;
	res.ast=lp;
	return res;
}

lval_t join_empty_lpoint(vector<lval_t>::iterator v)
{
	lval_t res;
	lpoint_data_t *lp=new lpoint_data_t;
	res.ast=lp;
	return res;
}

lval_t join_points(vector<lval_t>::iterator v)
{
	check_ast(v[1].ast,AST_LPOINT);
	return v[1];
}

string_value_t face_attrs[]=
{
	{ "a", A_T_A },
	{ "b", A_T_B },
	{ "c", A_T_C },
	{ "u_a", A_T_UA },
	{ "u_b", A_T_UB },
	{ "u_c", A_T_UC },
	{ "v_a", A_T_VA },
	{ "v_b", A_T_VB },
	{ "v_c", A_T_VC },
	{ "shader_name", A_SHADER},
	{ "vcol_a_r", A_T_VCOL_A_R },
	{ "vcol_a_g", A_T_VCOL_A_G },
	{ "vcol_a_b", A_T_VCOL_A_B },
	{ "vcol_b_r", A_T_VCOL_B_R },
	{ "vcol_b_g", A_T_VCOL_B_G },
	{ "vcol_b_b", A_T_VCOL_B_B },
	{ "vcol_c_r", A_T_VCOL_C_R },
	{ "vcol_c_g", A_T_VCOL_C_G },
	{ "vcol_c_b", A_T_VCOL_C_B },
	{ NULL , 0 }
};

lval_t join_face(vector<lval_t>::iterator v)
{
	lval_t res;
	check_ast(v[2].ast,AST_LATTRDATA);
	lattr_data_t *la=(lattr_data_t *)v[2].ast;
	face_data_t *face=new face_data_t;
	face->shader="";
	foreach(i,list<attr_data_t *>,la->l)
	{
		attr_data_t &attr=**i;
		if(!attr.f && (attr.I!="shader_name"))
		{
			WARNING<<"Attributes for face only can be numbers, bad value for "<<
				attr.I<<endl;
			continue;
		}
		int attr_id=string_value(face_attrs,attr.I.c_str());
		int n;
		switch(attr_id)
		{
			case A_T_A :
				n=(int)attr.F;
				attr.F-=(float)n;
				if(attr.F!=0.0) WARNING<<"Trunccating non integer value for "<<
															attr.I<<" attribute\n";
				face->T.a=(point3d_t *)n;
				break;
			case A_T_B :
				n=(int)attr.F;
				attr.F-=(float)n;
				if(attr.F!=0.0) WARNING<<"Trunccating non integer value for "<<
															attr.I<<" attribute\n";
				face->T.b=(point3d_t *)n;
				break;
			case A_T_C :
				n=(int)attr.F;
				attr.F-=(float)n;
				if(attr.F!=0.0) WARNING<<"Warning trunccating non integer value for "<<
															attr.I<<" attribute\n";
				face->T.c=(point3d_t *)n;
				break;
			case A_T_UA :
				face->uv[0]=attr.F;  face->T.hasuv=true;
				break;
			case A_T_UB :
				face->uv[2]=attr.F;  face->T.hasuv=true;
				break;
			case A_T_UC :
				face->uv[4]=attr.F;  face->T.hasuv=true;
				break;
			case A_T_VA :
				face->uv[1]=attr.F;  face->T.hasuv=true;
				break;
			case A_T_VB :
				face->uv[3]=attr.F;  face->T.hasuv=true;
				break;
			case A_T_VC :
				face->uv[5]=attr.F;  face->T.hasuv=true;
				break;
			case A_SHADER:
				if(attr.f) WARNING<<"Only an identifier can be the name of a shader\n";
				face->shader=attr.D;
				break;
			case A_T_VCOL_A_R:
				face->vcol[0]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_A_G:
				face->vcol[1]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_A_B:
				face->vcol[2]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_B_R:
				face->vcol[3]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_B_G:
				face->vcol[4]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_B_B:
				face->vcol[5]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_C_R:
				face->vcol[6]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_C_G:
				face->vcol[7]=attr.F;  face->T.has_vcol=true;
				break;
			case A_T_VCOL_C_B:
				face->vcol[8]=attr.F;  face->T.has_vcol=true;
				break;
			default:
				WARNING<<"Unknown attribute for face >"<<attr.I<<endl;
		}
	}
	delete la;
	res.ast=face;
	return res;
}

lval_t join_lface(vector<lval_t>::iterator v)
{
	lval_t res;
	check_ast(v[0].ast,AST_LFACE);
	check_ast(v[1].ast,AST_FACE);
	lface_data_t *lf=(lface_data_t *)v[0].ast;
	face_data_t *f=(face_data_t *)v[1].ast;
	
	if(f->T.hasuv)
		lf->facesuv.insert(lf->facesuv.end(), f->uv, f->uv+6);
	if(f->T.has_vcol)
		lf->faces_vcol.insert(lf->faces_vcol.end(), f->vcol, f->vcol+9);
	if(f->shader=="") f->T.setShader((shader_t *)-1);
	else
	{
		unsigned int i;
		for(i=0;i<lf->shaders.size();++i)
			if(lf->shaders[i]==f->shader) break;
		if(i==lf->shaders.size())
			lf->shaders.push_back(f->shader);
		f->T.setShader((shader_t *)i);
	}

	lf->faces.push_back(f->T);
	delete f;
	res.ast=lf;
	return res;
}

lval_t join_empty_lface(vector<lval_t>::iterator v)
{
	lval_t res;
	lface_data_t *lf=new lface_data_t;
	res.ast=lf;
	return res;
}

lval_t join_faces(vector<lval_t>::iterator v)
{
	check_ast(v[1].ast,AST_LFACE);
	return v[1];
}

string_value_t mesh_attrs[]=
{
	{ "autosmooth", A_AUTOSMOOTH },
	{ "has_orco", A_ORCO },
};

//$mesh   = $st_mesh  $lattr   '>' $points  $faces  $en_mesh  &join_mesh  ;

lval_t join_mesh(vector<lval_t>::iterator v)
{
	lval_t res;
	mesh_data_t *mesh=new mesh_data_t;
	mesh->autosmooth=false;
	mesh->orco=false;
	check_ast(v[1].ast,AST_LATTRDATA);
	check_ast(v[3].ast,AST_LPOINT);
	check_ast(v[4].ast,AST_LFACE);

	mesh->points=(lpoint_data_t *)v[3].ast;
	mesh->faces=(lface_data_t *)v[4].ast;
	
	lattr_data_t *la=(lattr_data_t *)v[1].ast;

	foreach(i,list<attr_data_t *>,la->l)
	{
		attr_data_t &attr=**i;
		int attr_id=string_value(mesh_attrs,attr.I.c_str());
		switch(attr_id)
		{
			case A_AUTOSMOOTH :
				if(attr.f)
				{
					mesh->autosmooth=true;
					mesh->angle=attr.F;
				}
				else WARNING<<"Only float value accepted for autosmooth\n";
				break;
			case A_ORCO:
				if(!attr.f && attr.D=="on")
					mesh->orco=true;
				break;
			default:
				WARNING<<"Unknown attribute > "<<attr.I<<" for mesh\n";
		}
	}
	delete la;
	res.ast=mesh;
	return res;
}

string_value_t sphere_attrs[]=
{
	{ "radius", A_RADIUS },
};

lval_t join_sphere(vector<lval_t>::iterator v)
{
	lval_t res;
	sphere_data_t *sphere=new sphere_data_t;
	sphere->center.x=0;
	sphere->center.y=0;
	sphere->center.z=0;
	sphere->radius=0;
	check_ast(v[1].ast,AST_LATTRDATA);
	check_ast(v[3].ast,AST_POINT);

	if(((point_data_t *)v[3].ast)->label!="center")
		WARNING<<"Only the center point should be given to sphere\n";
	else
		sphere->center=((point_data_t *)v[3].ast)->P;
	
	lattr_data_t *la=(lattr_data_t *)v[1].ast;

	foreach(i,list<attr_data_t *>,la->l)
	{
		attr_data_t &attr=**i;
		int attr_id=string_value(sphere_attrs,attr.I.c_str());
		switch(attr_id)
		{
			case A_RADIUS :
				if(attr.f)
					sphere->radius=attr.F;
				else WARNING<<"Only float value accepted for sphere radius\n";
				break;
			default:
				WARNING<<"Unknown attribute > "<<attr.I<<" for sphere\n";
		}
	}
	delete la;
	res.ast=sphere;
	return res;
}

string_value_t object_labels[]=
{
	{ "caus_rcolor", A_CAUS_RCOLOR },
	{ "caus_tcolor", A_CAUS_TCOLOR },
	{ NULL , 0 }
};

lval_t join_oattr( vector<lval_t>::iterator v )
{
	lval_t res;
	object_attr_t *oattr=new object_attr_t;
	check_ast(v[2].ast,AST_CDATA);
	oattr->color=(color_data_t *)v[2].ast;
	oattr->attr=string_value(object_labels,v[1].text.c_str());
	if(oattr->attr==0)
		WARNING<<"Unknown object label "<<v[1].text<<endl;
	res.ast=oattr;
	return res;
}

string_value_t object_attrs[]=
{
	{ "name", A_NAME },
	{ "original", A_ORIGINAL },
	{ "shader_name", A_SHADER },
	{ "shadow", A_SHADOW },
	{ "emit_rad", A_EMITR },
	{ "recv_rad", A_RECVR },
	{ "caus_IOR", A_CIOR },
	{ NULL , 0 }
};

lval_t join_object( vector<lval_t>::iterator v )
{
	lval_t res;
	check_ast(v[1].ast,AST_LATTRDATA);
	check_ast(v[3].ast,AST_LOATTR);
	//check_ast(v[4].ast,AST_MESH);
	object_data_t *obj=new object_data_t;
	obj->geometry=v[4].ast;
	obj->shadow=true;
	obj->emit_rad=true;
	obj->recv_rad=true;
	obj->caustics=false;
	obj->caus_IOR=1.0;
	obj->caus_rcolor.r=0.0;
	obj->caus_rcolor.g=0.0;
	obj->caus_rcolor.b=0.0;
	obj->caus_tcolor.r=0.0;
	obj->caus_tcolor.g=0.0;
	obj->caus_tcolor.b=0.0;

	lattr_data_t *la=(lattr_data_t *)v[1].ast;

	foreach(i,list<attr_data_t *>,la->l)
	{
		attr_data_t &attr=**i;
		int attr_id=string_value(object_attrs,attr.I.c_str());
		switch(attr_id)
		{
			case A_NAME : 
				if(attr.f) WARNING<<"Only an identifier can be the name of an object :"
					<<attr.F<<endl;
				else
					obj->name=attr.D;
				break;
			case A_ORIGINAL: 
				if(attr.f) WARNING<<"Only an identifier can be the name of an object :"
					<<attr.F<<endl;
				else
					obj->original=attr.D;
				break;
			case A_SHADER : 
				if(attr.f) WARNING<<"Only an identifier can be the name of a shader :"
					<<attr.F<<endl;
				else
					obj->shader=attr.D;
				break;
			case A_SHADOW :
				if(attr.D=="on") obj->shadow=true;
				else if(attr.D=="off") obj->shadow=false;
				else WARNING<<"Bad value for shadow attr in object: "<<attr.D
					<<endl;
				break;
			case A_EMITR :
				if(attr.D=="on") obj->emit_rad=true;
				else if(attr.D=="off") obj->emit_rad=false;
				else WARNING<<"Bad value for emit_rad attr in object: "<<attr.D
					<<endl;
				break;
			case A_RECVR :
				if(attr.D=="on") obj->recv_rad=true;
				else if(attr.D=="off") obj->recv_rad=false;
				else WARNING<<"Bad value for recv_rad attr in object: "<<attr.D
					<<endl;
				break;
			case A_CIOR :
				if(!attr.f) WARNING<<"Only float value accepted for caus_IOR: "<<attr.D
					<<endl;
				else
				{
					obj->caus_IOR=attr.F;
					obj->caustics=true;
				}
				break;
			default:
				WARNING<<"Unknown attribute > "<<attr.I<<" for object\n";
		}
	}
	delete la;
	if(obj->name=="") WARNING<<"Object with no name\n";
	if((obj->shader=="") && (obj->geometry!=NULL)) WARNING<<"Object with no shader\n";
	lobject_attr_t *loattr=(lobject_attr_t *)v[3].ast;
		
	foreach(i,list<object_attr_t *>,loattr->l)
	{
		object_attr_t &attr=**i;
		switch(attr.attr)
		{
			case A_CAUS_RCOLOR :
				obj->caustics=true;
				obj->caus_rcolor=*(attr.color);
				break;
			case A_CAUS_TCOLOR :
				obj->caustics=true;
				obj->caus_tcolor=*(attr.color);
				break;
		}
	}
	delete loattr;

	res.ast=obj;
	return res;
}
__END_YAFRAY
