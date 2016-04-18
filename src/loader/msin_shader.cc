/****************************************************************************
 *
 *      msin_shader.cc: shader construction parsing implementation
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

__BEGIN_YAFRAY
#define WARNING cerr<<"[Warning]: "

lval_t join_shader(vector<lval_t>::iterator v)
{
	lval_t res;
	shader_data_t *ns=new shader_data_t;
	res.ast=ns;
	check_ast(v[1].ast,AST_LATTRDATA);
	lattr_data_t *lgattr=(lattr_data_t *)v[1].ast;
	check_ast(v[3].ast,AST_LLABEL);
	llabel_data_t *ll=(llabel_data_t *)v[3].ast;
	check_ast(v[4].ast,AST_LMODDATA);
	ns->lmod=(lmod_data_t *)v[4].ast;
	
	foreach(i,list<attr_data_t *>,lgattr->l)
	{
		attr_data_t &attr=**i;
		if(attr.f)
			ns->params[attr.I]=parameter_t(attr.F);
		else
			ns->params[attr.I]=parameter_t(attr.D);
	}
	delete lgattr;
	
	foreach(i,list<label_data_t *>,ll->l)
	{
		if(label_is_value(*i))
		{
			attr_data_t &attr=**((*i)->lattr->l.begin());
			if(attr.f)
				ns->params[(*i)->label_name]=parameter_t(attr.F);
			else
				ns->params[(*i)->label_name]=parameter_t(attr.D);
		}
		else if(label_is_color(*i))
		{
			color_data_t c=get_color((*i)->lattr->l);
			ns->params[(*i)->label_name]=parameter_t(c.r,c.g,c.b);
		}
		else if(label_is_point(*i))
			ns->params[(*i)->label_name]=parameter_t(get_point((*i)->lattr->l));
		else
			WARNING<<"Unknown label structure "<<(*i)->label_name<<" in xml\n";
	}
	delete ll;
	return res;
}

__END_YAFRAY
