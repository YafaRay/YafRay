/****************************************************************************
 *
 *      msin_general.cc: general construction parsing implementation
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

#include "parser.h"
#include "msin.h"
#include<string.h>

__BEGIN_YAFRAY
#define WARNING cerr<<"[Warning]: "
#define FATAL cerr<<"[Fatal (bug)]: "

int string_value(const string_value_t *values,const char *ello)
{
	int i=0;
	while(values[i].str!=NULL)
	{
		if(!strcmp(values[i].str,ello))
			return values[i].value;
		++i;
	}
	return 0;
}

lval_t join_copy(vector<lval_t>::iterator v)
{
	return v[0];
}

lval_t join_copy1(vector<lval_t>::iterator v)
{
	return v[1];
}

lval_t join_null(vector<lval_t>::iterator v)
{
	lval_t res;
	res.ast=NULL;
	return res;
	//return v[0];
}


lval_t join_attr(vector<lval_t>::iterator v)
{
	lval_t res;
	attr_data_t *ad=new attr_data_t;
	ad->I=v[0].text;
	ad->D=v[2].text;
	ad->f=false;
	res.ast=ad;
	return res;
}

lval_t join_attrf(vector<lval_t>::iterator v)
{
	lval_t res;
	attr_data_t *ad=new attr_data_t;
	ad->I=v[0].text;
	ad->F=atof(v[2].text.c_str());
	ad->f=true;
	res.ast=ad;
	return res;
}

lval_t join_color(vector<lval_t>::iterator v)
{
	lval_t res;
	color_data_t *nc=new color_data_t;
	res.ast=nc;
	nc->r=nc->g=nc->b=0.0;
	nc->a=1.0;
	for(int i=0;i<3;++i)
	{
		check_ast(v[i].ast,AST_ATTRDATA);
		attr_data_t *attr=(attr_data_t *)v[i].ast;
		if(attr->I.size()!=1)
		{
			WARNING<<"Unknown color attribute > "<<attr->I<<"\n";
			delete attr;
			continue;
		}
		char c=attr->I[0];
		switch(c)
		{
			case 'r' : nc->r=attr->F;break;
			case 'g' : nc->g=attr->F;break;
			case 'b' : nc->b=attr->F;break;
			default : WARNING<<"Unknown color attribute > "<<c<<"\n";
		}
		delete attr;
	}
	return res;
}

lval_t join_colorA(vector<lval_t>::iterator v)
{
	lval_t res;
	color_data_t *nc=new color_data_t;
	res.ast=nc;
	nc->r=nc->g=nc->b=0.0;
	nc->a=1.0;
	for(int i=0;i<4;++i)
	{
		check_ast(v[i].ast,AST_ATTRDATA);
		attr_data_t *attr=(attr_data_t *)v[i].ast;
		if(attr->I.size()!=1)
		{
			WARNING<<"Unknown color attribute > "<<attr->I<<"\n";
			delete attr;
			continue;
		}
		char c=attr->I[0];
		switch(c)
		{
			case 'r' : nc->r=attr->F;break;
			case 'g' : nc->g=attr->F;break;
			case 'b' : nc->b=attr->F;break;
			case 'a' : nc->a=attr->F;break;
			default : WARNING<<"Unknown color attribute > "<<c<<"\n";
		}
		delete attr;
	}
	return res;
}


lval_t join_label( vector<lval_t>::iterator v )
{
	lval_t res;
	check_ast(v[2].ast,AST_LATTRDATA);
	label_data_t *label=new label_data_t;
	label->label_name=v[1].text;
	label->lattr=(lattr_data_t *)v[2].ast;
	res.ast=label;
	return res;
}

lval_t join_litem( vector<lval_t>::iterator v )
{
	lval_t res;
	check_ast(v[0].ast,AST_LITEM);
	litem_data_t *la=(litem_data_t *)v[0].ast;
	ast_t *a=(ast_t *)v[1].ast;
	
	la->l.push_back(a);
	res.ast=la;
	return res;
}

lval_t join_empty_litem( vector<lval_t>::iterator v )
{
	lval_t res;
	res.ast=new litem_data_t;
	return res;
}

color_data_t get_color(const list<attr_data_t *> &l)
{
	color_data_t color;
	color.r=color.g=color.b=0.0;
	color.a=1.0;
	cforeach(j,list<attr_data_t *>,l)
	{
		attr_data_t *attr=*j;
		if(!attr->f)
			continue;
		if(attr->I.size()!=1)
			continue;
		char c=attr->I[0];
		switch(c)
		{
			case 'r' : color.r=attr->F;break;
			case 'g' : color.g=attr->F;break;
			case 'b' : color.b=attr->F;break;
			case 'a' : color.a=attr->F;break;
		}
	}
	return color;
}

bool label_is_color(label_data_t *label)
{
	list<attr_data_t *> &la=label->lattr->l;
	if((la.size()!=3) && (la.size()!=4)) return false;
	cforeach(j,list<attr_data_t *>,la)
	{
		attr_data_t *attr=*j;
		if(!attr->f)
			return false;
		if(attr->I.size()!=1)
			return false;;
		char c=attr->I[0];
		switch(c)
		{
			case 'r' : break;
			case 'g' : break;
			case 'b' : break;
			case 'a' : break;
			default: return false;
		}
	}
	return true;
}

point3d_t get_point(const list<attr_data_t *> &l)
{
	point3d_t p;
	cforeach(j,list<attr_data_t *>,l)
	{
		attr_data_t *attr=*j;
		if(!attr->f)
			continue;
		if(attr->I.size()!=1)
			continue;
		char c=attr->I[0];
		switch(c)
		{
			case 'x' : p.x=attr->F;break;
			case 'y' : p.y=attr->F;break;
			case 'z' : p.z=attr->F;break;
		}
	}
	return p;
}

bool label_is_point(label_data_t *label)
{
	list<attr_data_t *> &la=label->lattr->l;
	if(la.size()!=3) return false;
	cforeach(j,list<attr_data_t *>,la)
	{
		attr_data_t *attr=*j;
		if(!attr->f)
			return false;
		if(attr->I.size()!=1)
			return false;;
		char c=attr->I[0];
		switch(c)
		{
			case 'x' : break;
			case 'y' : break;
			case 'z' : break;
			default: return false;
		}
	}
	return true;
}

bool label_is_value(label_data_t *label)
{
	list<attr_data_t *> &la=label->lattr->l;
	if(la.size()!=1) return false;
	cforeach(j,list<attr_data_t *>,la)
	{
		attr_data_t *attr=*j;
		if(attr->I!="value")
			return false;;
	}
	return true;
}

string_value_t generic_structs[]=
{
	{ "light", AST_LIGHT },
	{ "modulator", AST_MODDATA },
	{ "filter", AST_FILTER },
	{ "render", AST_RENDER },
	{ "texture", AST_TDATA },
	{ "background", AST_BACKG },
	{ NULL , 0 }
};

lval_t join_generic( vector<lval_t>::iterator v )
{
	lval_t res;
	int type=string_value(generic_structs,v[1].text.c_str());
	if(type==0) FATAL<<"Type mistake in generic parser\n";

	check_ast(v[2].ast,AST_LATTRDATA);
	check_ast(v[4].ast,AST_LLABEL);
	lattr_data_t *la=(lattr_data_t *)v[2].ast;
	llabel_data_t *ll=(llabel_data_t *)v[4].ast;
	generic_data_t *obj=new generic_data_t(type);

	foreach(i,list<attr_data_t *>,la->l)
	{
		attr_data_t &attr=**i;
		if(attr.f)
			obj->params[attr.I]=parameter_t(attr.F);
		else
			obj->params[attr.I]=parameter_t(attr.D);
	}
	delete la;
	foreach(i,list<label_data_t *>,ll->l)
	{
		if(label_is_value(*i))
		{
			attr_data_t &attr=**((*i)->lattr->l.begin());
			if(attr.f)
				obj->params[(*i)->label_name]=parameter_t(attr.F);
			else
				obj->params[(*i)->label_name]=parameter_t(attr.D);
		}
		else if(label_is_color(*i))
		{
			color_data_t c=get_color((*i)->lattr->l);
			obj->params[(*i)->label_name]=parameter_t(c.r,c.g,c.b,c.a);
		}
		else if(label_is_point(*i))
			obj->params[(*i)->label_name]=parameter_t(get_point((*i)->lattr->l));
		else
			WARNING<<"Unknown label structure "<<(*i)->label_name<<" in xml\n";
	}
	delete ll;
	res.ast=obj;
	return res;
}

lval_t join_transform( vector<lval_t>::iterator v )
{
	lval_t res;
	check_ast(v[2].ast,AST_LATTRDATA);
	check_ast(v[4].ast,AST_LITEM);
	lattr_data_t *la=(lattr_data_t *)v[2].ast;
	litem_data_t *li=(litem_data_t *)v[4].ast;
	transform_data_t *obj=new transform_data_t;

	foreach(i,list<attr_data_t *>,la->l)
	{
		attr_data_t &attr=**i;
		if(attr.f)
			obj->params[attr.I]=parameter_t(attr.F);
		else
			obj->params[attr.I]=parameter_t(attr.D);
	}
	delete la;
	obj->litem=li;
	res.ast=obj;
	return res;
}

__END_YAFRAY
