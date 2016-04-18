/****************************************************************************
 *
 *      gram_loader.cc: Gram loader implementation for the gram loader
 *      This is part of the ADOP package integrated in yafray
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
#include "gram_lex.h"
#include "gram_loader.h"



using namespace std;

gllval_t  join_copy(vector<gllval_t>::iterator v)
{
	return v[0];
}
gllval_t join_var(vector<gllval_t>::iterator v)
{
	gllval_t res;
	sym_t *ns=new sym_t;
	ns->name=v[0].text;
	ns->var=true;
	res.data.sym_data=ns;
	return res;
}

gllval_t join_tok(vector<gllval_t>::iterator v)
{
	gllval_t res;
	sym_t *ns=new sym_t;
	ns->name=v[0].text;
	ns->var=false;
	res.data.sym_data=ns;
	return res;
}


gllval_t join_rulAR(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.rul_data=new rul_t;
	res.data.rul_data->mode=ALLREDUCE;
	res.data.rul_data->tokens=NULL;
	return res;
}

gllval_t join_rulAS(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.rul_data=new rul_t;
	res.data.rul_data->mode=ALLSHIFT;
	res.data.rul_data->tokens=NULL;
	return res;
}

gllval_t join_rulLS(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.rul_data=new rul_t;
	res.data.rul_data->mode=LSHIFT;
	res.data.rul_data->tokens=v[0].data.tlist_data;
	return res;
}

gllval_t join_lsym(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.slist_data=v[0].data.slist_data;
	res.data.slist_data->push_back(v[1].data.sym_data);
	return res;
}

gllval_t join_empty_lsym(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.slist_data=new list<sym_t *>;
	return res;
}

gllval_t join_tlist(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.tlist_data=v[0].data.tlist_data;
	res.data.tlist_data->insert(v[1].text);
	return res;
}

gllval_t join_empty_tlist(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.tlist_data=new set<string>;
	return res;
}

gllval_t join_prod(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.prod_data=new prod_t;
	res.data.prod_data->I=v[0].text;
	res.data.prod_data->D=v[2].data.slist_data;
	res.data.prod_data->func=v[3].text;
	res.data.prod_data->rule=NULL;
	return res;
}

gllval_t join_prod_rul(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.prod_data=new prod_t;
	res.data.prod_data->I=v[0].text;
	res.data.prod_data->D=v[2].data.slist_data;
	res.data.prod_data->func=v[3].text;
	res.data.prod_data->rule=v[5].data.rul_data;
	return res;
}

gllval_t join_plist(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.plist_data=v[0].data.plist_data;
	res.data.plist_data->push_back(v[1].data.prod_data);
	return res;
}

gllval_t join_empty_plist(vector<gllval_t>::iterator v)
{
	gllval_t res;
	res.data.plist_data=new list<prod_t *>;
	return res;
}

#define GRAM   5001
#define PLIST  5002
#define PROD   5003
#define SYM    5004
#define RLIST  5005
#define TLIST  5006
#define SLIST  5007
#define RUL    5008


static production_t<gllval_t> productions[]=
{
	{ START , { GRAM } , 1 , join_copy },

	{ GRAM , { PLIST }, 1 , join_copy },
	{ PLIST, { PLIST, PROD }, 2 , join_plist },
	{ PLIST, {}, 0, join_empty_plist },
	{ PROD,  { T_VAR, '=' , SLIST, T_FUNC, ';', RUL },6, join_prod_rul },
	{ PROD,  { T_VAR, '=' , SLIST, T_FUNC, ';' },5, join_prod },
	{ SLIST, { SLIST, SYM },2,join_lsym },
	{ SLIST, {},0,join_empty_lsym },
	{ SYM,   { T_VAR }, 1, join_var },
	{ SYM,   { T_TOK }, 1, join_tok },
	{ RUL,   { T_ALL, T_REDUCE }, 2, join_rulAR },
	{ RUL,   { T_ALL, T_SHIFT }, 2,  join_rulAS },
	{ RUL,   { TLIST, T_SHIFT }, 2,  join_rulLS },
	{ TLIST, { TLIST, T_TOK },2, join_tlist },
	{ TLIST, {},0, join_empty_tlist},
	
	
	{ 0, {} }
};

production_t<gllval_t> *gl_productions=productions;

static rule_t *rules[]=
{
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

rule_t **gl_rules=rules;
