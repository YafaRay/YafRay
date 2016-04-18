/****************************************************************************
 *
 *      gram_loader.h: Gram loader api for the gram loader
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
#ifndef __GRAML_H
#define __GRAML_H

#include "gram_lex.h"

struct sym_t
{
	std::string name;
	bool var;
};

#define ALLREDUCE 1
#define ALLSHIFT  2
#define LSHIFT    3

struct rul_t
{
	std::set<std::string> *tokens;
	int mode;
	~rul_t() {if(tokens!=NULL) delete tokens;};
};

struct prod_t
{
	std::string I;
	std::list<sym_t *> *D;
	std::string func;
	rul_t *rule;

	~prod_t()
	{
		if(rule!=NULL) delete rule;
		foreach(i,std::list<sym_t *>,*D) delete *i;
		delete D;
	}
};

struct gllval_t
{
	std::string text;
	union
	{
		std::list<sym_t *> *slist_data;
		sym_t *sym_data;
		rul_t *rul_data;
		std::set<std::string> *tlist_data;
		prod_t *prod_data;
		std::list<prod_t *> *plist_data;
	}data;
};

extern production_t<gllval_t> *gl_productions;
extern rule_t **gl_rules;


template<class V>
parser_t<V>::parser_t(FILE *in,mapToks_t &tokens,mapJoins_t &joins,lex_t &l):
	lex(l),stack(10),val_stack(10),ssize(10)
{
	
	error=false;
	inputFile_t input(in);
	gram_lex_t glex(&input);
	parser_t<gllval_t> parser(gl_productions,gl_rules,glex);

	if(!parser.parse())
	{
		std::cout<<"Error en linea "<<parser.line()<<" columna "<<parser.col()
						<<" token "<<parser.text()<<"\n";
		error=true;
		return;
	}

	std::list<prod_t *> *plist=parser.result().data.plist_data;

	std::vector<production_t<V> > gprods;
	std::vector<rule_t *> grules;

	mapVars_t var;
	
	int vcount=5001;
	
	foreach(p,std::list<prod_t *>,*plist)
	{
		production_t<V> np;
		if((*p)->I=="start")
			np.I=START;
		else
		{
			if(var.find((*p)->I)==var.end()) 
				var[(*p)->I]=vcount++;
			np.I=var[(*p)->I];
		}
		int ic=0;
		foreach(s,std::list<sym_t *>,*((*p)->D))
		{
			if(ic>=MAX_PROD)
			{
				std::cout<<"Warning: reached maximal production lenght\n";
				error=true;
				break;
			}
			if((*s)->var)
			{
				if(var.find((*s)->name)==var.end()) 
					var[(*s)->name]=vcount++;
				np.D[ic]=var[(*s)->name];
			}
			else
			{
				if(tokens.find((*s)->name)==tokens.end())
				{
					std::cout<<"Error, undefined token "<<(*s)->name<<"\n";
					error=true;
					continue;
				}
				else
					np.D[ic]=tokens[(*s)->name];
			}
			ic++;
		}
		np.len=ic;
		if(joins.find((*p)->func)!=joins.end())
			np.join=joins[(*p)->func];
		else
		{
			std::cout<<"Warning: undefined join function "<<(*p)->func<<"\n";
			error=true;
			np.join=NULL;
		}
		if((*p)->rule!=NULL)
		{
			int nrules=tokens.size();
			rule_t *rule=new rule_t[nrules];
			int rc=0;
			switch((*p)->rule->mode)
			{
				case ALLSHIFT :
					foreach(token,mapVars_t,tokens)
					{
						rule[rc].token=(*token).second;
						rule[rc].action=SHIFT;
						rc++;
					}
					break;
				case ALLREDUCE :
					foreach(token,mapVars_t,tokens)
					{
						rule[rc].token=(*token).second;
						rule[rc].action=REDUCE;
						rc++;
					}
					break;
				case LSHIFT :
					foreach(token,mapVars_t,tokens)
					{
						rule[rc].token=(*token).second;
						if((*p)->rule->tokens->find((*token).first)==
								(*p)->rule->tokens->end())
							rule[rc].action=REDUCE;
						else
							rule[rc].action=SHIFT;
						rc++;
					}
					break;
			}
			grules.push_back(rule);
		}
		else grules.push_back(NULL);
		gprods.push_back(np);
		delete *p;
	}
	production_t<V> last;
	last.I=0;
	gprods.push_back(last);

	delete plist;
	sp=0;
	buildTables(gprods,grules);
	err_fun=NULL;
	foreach(r,std::vector<rule_t *>,grules)
		delete [] *r;
}

#endif
