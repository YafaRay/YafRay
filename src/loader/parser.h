/****************************************************************************
 *
 *      parser.h: Generic parser api
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
#ifndef __PARSER_H
#define __PARSER_H

#include<vector>
#include<list>
#include<set>
#include<string>
#include<map>

#include "lex.h"
#include<iostream>

#define MAX_PROD 10


#ifndef foreach
#define foreach(i,tipo,seq) \
	for(tipo::iterator i=(seq).begin();i!=(seq).end();++i)
#define cforeach(i,tipo,seq) \
	for(tipo::const_iterator i=(seq).begin();i!=(seq).end();++i)
#endif

/*
struct shash
{
	  size_t operator () (const string &s) const
	  {
	    return el_hash(s.c_str());
	  }
	  hash<const char *> el_hash;
};
*/

template<class V>
struct production_t
{
	int I;
	int D[MAX_PROD];
	int len;
	V  (*join)(typename std::vector<V>::iterator);
};

struct gset_t
{
	int id;
	int I;
	int *D;
	int len;
	int dot;
	std::set<int> follow;
};

inline std::ostream & operator << (std::ostream &out,const gset_t &g)
{
	out<<g.I<<" ->";
	for(int i=0;i<g.len;++i)
	{
		if(i==g.dot) out<<" .";
		if(g.D[i]<256) out<<" "<<(char)g.D[i];
		else out<<" "<<g.D[i];
	}
	out<<";";
	return out;
}

inline std::ostream & operator << (std::ostream &out,const std::list<gset_t> &l)
{
	cforeach(i,std::list<gset_t>,l)
		out<<*i<<"\n";
	return out;
}

class node_t;

struct arc_t
{
	int s;
	node_t *node;
};

class node_t
{
	public:
		node_t(std::list<gset_t> *_l) {l=_l;};
		~node_t() { if(l!=NULL) delete l;};
		int id;
		std::list<gset_t> *l;
		std::list<arc_t> next;
};
#define ISVAR(c) (c>=5000)
#define ISTERM(c) ((c<5000) && (c>0))
#define START 5000
#define LAMBDA -1

#ifdef MSVC
#undef ERROR
#endif

#define REDUCE 0
#define SHIFT  1
#define ERROR  2
#define ACCEPT  3

#define BUILD_ACTION(accion,estado) ( (accion) | ((estado)<<2))
#define ACTION(ac) (ac & 3)
#define STATE(ac) (ac >> 2)
#define PRODUCTION(ac) (ac >> 2)

struct _rule_t
{
	int token;
	int action;
};

typedef _rule_t rule_t;

template<class V>
class parser_t
{
	public:

		//typedef hash_map<string,int,shash> mapVars_t;
		//typedef hash_map<string,int,shash> mapToks_t;
		//typedef hash_map<string,V (*)(vector<V>::iterator),shash> mapJoins_t;
		typedef std::map<std::string,int> mapVars_t;
		typedef std::map<std::string,int> mapToks_t;
		typedef std::map<std::string,V (*)(typename std::vector<V>::iterator)> mapJoins_t;


		template<class C,class RC>
		parser_t(C &p,RC &r,lex_t &l):
		//parser_t(production_t<V> *p,rule_t **r,lex_t &l):
			lex(l),stack(10),val_stack(10),ssize(10)
			{sp=0;buildTables(p,r);err_fun=NULL;};
		parser_t(FILE *in,mapToks_t &tokens,mapJoins_t &joins,lex_t &l);
		bool parse();
		V & result() { return val_stack[1];};
		int line() { return lex.line();};
		int col() { return lex.col();};
		char * text() { return lex.text();};
		void setErrorHandler(bool (*ef)(typename std::vector<V>::iterator,
				typename std::vector<V>::iterator,
				typename std::list<int> &,int &,lex_t &)) {err_fun=ef;};

		bool error;
		
	protected:
		void shift(int state);
		void reduce(int p);
		template<class RC>
		int getAction(int p,int in,RC &rules);
		void compNulls();
		std::set<int> * first(int X);
		std::set<int> * follow(int X);
		void gsetFollow(gset_t &next,const gset_t &prev);
		std::list<gset_t> * closure(const std::list<gset_t> &I);
		std::list<gset_t> * goutu(const std::list<gset_t> &I,int X);
		node_t * find(const std::list<node_t *> &l,const std::list<gset_t> &e);
		std::list<node_t *> * buildAutomaton();
		void printTables();
		template<class C,class RC>
		void buildTables(C &gram,RC &rules);
		bool procError();

		lex_t &lex;
		std::vector<V  (*)(typename std::vector<V>::iterator)> join;
		bool (*err_fun)(typename std::vector<V>::iterator,
			typename std::vector<V>::iterator,
			typename std::list<int> &,int & ,lex_t &);

		std::vector<gset_t> gramar;
		std::vector<int> stack;
		std::vector<V> val_stack;
		//hash_map<int,vector<int> > action;
		//hash_map<int,vector<int> > go_to;
		std::map<int,std::vector<int> > action;
		std::map<int,std::vector<int> > go_to;
		std::set<int> vars;
		std::set<int> terms;
		std::set<int> nulls;
		int sp;
		int token;
		int ssize;
};

template<class V>
void parser_t<V>::shift(int state)
{
	if(sp==ssize)
	{
		ssize=2*ssize;
		stack.resize(ssize);
		val_stack.resize(ssize);
	}
	stack[sp]=state;
	val_stack[sp].text=lex.text();
	token=lex.nextToken();
	sp++;
}

template<class V>
void parser_t<V>::reduce(int p)
{
	if((gramar[p].len==0) && (sp==ssize))
	{
		ssize=2*ssize;
		stack.resize(ssize);
		val_stack.resize(ssize);
	}

	sp-=gramar[p].len;
	stack[sp]=STATE(go_to[gramar[p].I][stack[sp-1]]);
	val_stack[sp]=join[p](val_stack.begin()+sp);
	sp++;
}

template<class V>
template<class RC>
int parser_t<V>::getAction(int p,int in,RC &rules)
{
	if(rules[p]==NULL)
	{
		std::cout<<"Unresolved shift/reduce conflict. Defaults to reduce in "<<p<<"\n";
		error=true;
		return REDUCE;
	}
	rule_t *r=rules[p];
	for(int i=0;r[i].token!=0;++i)
		if(r[i].token==in) return r[i].action;
	std::cout<<"Unresolved shift/reduce conflict. Defaults to reduce in "<<p<<"\n";
	return REDUCE;
}


template<class V>
void parser_t<V>::compNulls()
{
	bool changed=true;
	while(changed)
	{
		changed=false;
		for(unsigned int i=0;i<gramar.size();++i)
		{
			if(nulls.find(gramar[i].I)!=nulls.end()) continue;
			bool vacio=true;
			for(int j=0;j<gramar[i].len;++j)
				if(nulls.find(gramar[i].D[j])==nulls.end())
				{
					vacio=false;
					break;
				}
			if(vacio)
			{
				nulls.insert(gramar[i].I);
				changed=true;
			}
		}
	}
}


template<class V>
std::set<int> * parser_t<V>::first(int X)
{
	std::set<int> *f=new std::set<int>;
	if(ISTERM(X))
	{
		f->insert(X);
		return f;
	}
	else
	{
		for(unsigned int i=0;i<gramar.size();++i)
		{
			if(gramar[i].I==X)
			{
				if(gramar[i].len==0)
					f->insert(LAMBDA);
				else if(ISTERM(gramar[i].D[0]))
					f->insert(gramar[i].D[0]);
				else
				{
					bool goon=true;int j=0;
					while(goon && (j<gramar[i].len))
					{
						if(ISTERM(gramar[i].D[j]))
						{
							f->insert(gramar[i].D[j]);
							goon=false;
						}
						else
						{
							bool null=(nulls.find(gramar[i].D[j])!=nulls.end());
							if(gramar[i].D[j]!=X)
							{
								std::set<int> *r=first(gramar[i].D[j]);
								if(null) r->erase(LAMBDA);
								f->insert(r->begin(),r->end());
								delete r;
							}
							if(!null) goon=false;
						}
						++j;
					}
					if(goon) f->insert(LAMBDA);
				}
			}
		}
	}
	return f;
}

template<class V>
std::set<int> * parser_t<V>::follow(int X)
{
	std::set<int> *f=new std::set<int>;
	if(X==START)
	{
		f->insert(T_EOF);
		return f;
	}
	for(unsigned int i=0;i<gramar.size();++i)
	{
		bool has_f=false;
		for(int j=0;j<gramar[i].len;++j)
		{
			if(gramar[i].D[j]==X) 
			{
				if (j!=(gramar[i].len-1))
				{
					std::set<int> *r=first(gramar[i].D[j+1]);
					f->insert(r->begin(),r->end());
					delete r;
				}
				if(!has_f)
				{
					int k;
					for(k=j+1;k<gramar[i].len;++k)
					{
						std::set<int> *r=first(gramar[i].D[k]);
						if(r->find(LAMBDA)==r->end()) break;
						delete r;
					}
					if(k<gramar[i].len) has_f=false; else has_f=true;
				}
			}
		}
		if((gramar[i].I!=X) && has_f)
		{
			std::set<int> *r=follow(gramar[i].I);
			f->insert(r->begin(),r->end());
			delete r;
		}
	}
	f->erase(LAMBDA);
	return f;
}

template<class V>
void parser_t<V>::gsetFollow(gset_t &next,const gset_t &prev)
{
	int dot=prev.dot+1;
	
	while(dot<prev.len)
	{
		std::set<int> *r=first(prev.D[dot]);
		next.follow.insert(r->begin(),r->end());
		delete r;
		if(ISVAR(prev.D[dot]) && (nulls.find(prev.D[dot])!=nulls.end()))
			dot++;
		else
			break;
	}
	next.follow.erase(LAMBDA);
	if(dot==prev.len)
		next.follow.insert(prev.follow.begin(),prev.follow.end());
}
					
template<class V>
std::list<gset_t> * parser_t<V>::closure(const std::list<gset_t> &I)
{
	std::set<int> added;
	std::list<gset_t> *cl=new std::list<gset_t>(I);
	bool modified=true;
	while(modified)
	{
		modified=false;
		foreach(i,std::list<gset_t>,*cl)
		{
			if((*i).dot>=(*i).len) continue;
			int sim=(*i).D[(*i).dot];
			if(added.find(sim)!=added.end()) 
			{
				foreach(j,std::list<gset_t>,*cl)
					if((*j).I==sim)
						gsetFollow(*j,*i);
				continue;
			}
			added.insert(sim);
			for(unsigned int j=0;j<gramar.size();++j)
			{
				if(gramar[j].I==sim)
				{
					cl->push_back(gramar[j]);
					gsetFollow(cl->back(),*i);
					modified=true;
				}
			}
		}
	}
	return cl;
}

template<class V>
std::list<gset_t> * parser_t<V>::goutu(const std::list<gset_t> &I,int X)
{
	std::list<gset_t> *go=new std::list<gset_t>;
	cforeach(i,std::list<gset_t>,I)
	{
		if( ((*i).dot<(*i).len) && ((*i).D[(*i).dot] == X) )
		{
			go->push_back(*i);
			go->back().dot++;
		}
	}
	std::list<gset_t> *res=closure(*go);
	delete go;
	return res;
}

inline bool operator == (const gset_t &a,const gset_t &b)
{
	if(a.D!=b.D) return false;
	if(a.dot!=b.dot) return false;
	return (a.follow==b.follow);
}

inline bool operator == (const std::list<gset_t> &a,const std::list<gset_t> &b)
{
	if(a.size()!=b.size()) return false;
	for(std::list<gset_t>::const_iterator i=a.begin(),j=b.begin();i!=a.end();++i,++j)
		if(!(*j==*i)) return false;
	/*
	cforeach(i,list<gset_t>,a)
	{
		list<gset_t>::const_iterator j;
		for(j=b.begin();j!=b.end();++j)
			if(*j==*i) break;
		if(j==b.end()) return false;
	}	
	cforeach(i,list<gset_t>,b)
	{
		list<gset_t>::const_iterator j;
		for(j=a.begin();j!=a.end();++j)
			if(*j==*i) break;
		if(j==a.end()) return false;
	}
	*/
	return true;	
}

template<class V>
node_t * parser_t<V>::find(const std::list<node_t *> &l,const std::list<gset_t> &e)
{
	cforeach(i,std::list<node_t *>,l)
		if(*((*i)->l)==e) return *i;
	return NULL;
}

template<class V>
std::list<node_t *> * parser_t<V>::buildAutomaton()
{
	int c=0;
	std::list<gset_t> init;
	init.push_back(gramar[0]);
	init.back().follow.insert(T_EOF);
	node_t *ini=new node_t(closure(init));
	ini->id=c;
	c++;
	std::list<node_t *> *res=new std::list<node_t *>;
	std::list<node_t *> nuevos;
	nuevos.push_back(ini);
	while(!nuevos.empty())
	{
		std::list<node_t *> calc;
		res->insert(res->end(),nuevos.begin(),nuevos.end());
		foreach(i,std::list<node_t *>,nuevos)
		{
			std::set<int> dir;
			foreach(j,std::list<gset_t>,*((*i)->l))
			{
				if((*j).dot<(*j).len)
					dir.insert((*j).D[(*j).dot]);
			}
			foreach(j,std::set<int>,dir)
			{
				arc_t arc;
				arc.s=*j;
				std::list<gset_t> *go=goutu(*((*i)->l),*j);
				node_t *found=find(*res,*go);
				if(found==NULL)
				{
					node_t *nuevo=new node_t(go);
					nuevo->id=c;c++;
					calc.push_back(nuevo);
					arc.node=calc.back();
				}
				else
				{
					delete go;
					arc.node=found;
				}
				(*i)->next.push_back(arc);
			}
		}
		nuevos=calc;
	}
	return res;	
}

template<class V>
void parser_t<V>::printTables()
{
	foreach(i,std::set<int>,terms)
	{
		if(*i<256) std::cout<<(char)*i<<"   > ";
		else std::cout<<*i<<" > ";
		for(unsigned int j=0;j<action[T_EOF].size();++j)
		{
			std::cout<<"| ";
			if(ACTION(action[*i][j])==SHIFT) std::cout<<"s"<<STATE(action[*i][j]);
			if(ACTION(action[*i][j])==REDUCE) std::cout<<"r"<<STATE(action[*i][j]);
			if(ACTION(action[*i][j])==ACCEPT) std::cout<<"ac";
			if(ACTION(action[*i][j])==ERROR) std::cout<<"  ";
		}
		std::cout<<"\n";
	}
	foreach(i,std::set<int>,vars)
	{
		std::cout<<*i<<">";
		for(unsigned int j=0;j<action[T_EOF].size();++j)
		{
			std::cout<<"| ";
			if(ACTION(go_to[*i][j])!=ERROR) std::cout<<" "<<STATE(go_to[*i][j]);
			if(ACTION(go_to[*i][j])==ERROR) std::cout<<"  ";
		}
		std::cout<<"\n";
	}
}

template<class V>
template<class C,class RC>
void parser_t<V>::buildTables(C &gram,RC &rules)
{
	//convert the input gramar to our format
	int len;
	for(len=0;gram[len].I!=0;++len);
	gramar.resize(len);
	join.resize(len);
	for(int i=0;i<len;++i)
	{
		gramar[i].id=i;
		gramar[i].I=gram[i].I;
		gramar[i].D=gram[i].D;
		gramar[i].len=gram[i].len;
		join[i]=gram[i].join;
		gramar[i].dot=0;
	}
	compNulls();
	// Build the automaton
	std::list<node_t *> *aut=buildAutomaton();
	
	//extract vars and terminals from the gramar
	foreach(i,std::vector<gset_t>,gramar)
	{
		vars.insert((*i).I);
		for(int j=0;j<(*i).len;++j)
		{
			if(ISTERM((*i).D[j]))
				terms.insert((*i).D[j]);
		}
	}
	terms.insert(T_EOF);
	terms.insert(0);

	foreach(i,std::set<int>,vars)
	{
		go_to[*i].resize(aut->size());
		for(unsigned int j=0;j<aut->size();++j)
			go_to[*i][j]=BUILD_ACTION(ERROR,0);
	}
	
	// set default action
	foreach(i,std::set<int>,terms)
	{
		action[*i].resize(aut->size());
		for(unsigned int j=0;j<aut->size();++j)
			action[*i][j]=BUILD_ACTION(ERROR,0);
	}

	// build the table
	foreach(i,std::list<node_t *>,*aut)
	{
		int red_prod=0;
		//compute posible reduction
		foreach(j,std::list<gset_t>,*((*i)->l))
			if((*j).dot==(*j).len)
			{
				red_prod=(*j).id;
				foreach(k,std::set<int>,(*j).follow)
				{
					if(ACTION(action[*k][(*i)->id])!=ERROR)
					{
						error=true;
						std::cout<<"Warning: reduce conflict in prod "<<red_prod
										<<" token ";
						if(*k<256) std::cout<<(char)*k<<"\n";
						else std::cout<<*k<<"\n";
					}
					if((*j).I==START) 
						action[*k][(*i)->id]=BUILD_ACTION(ACCEPT,0);
					else
						action[*k][(*i)->id]=BUILD_ACTION(REDUCE,(*j).id);
				}
			}
		//compute possible shifts
		foreach(j,std::list<arc_t>,(*i)->next)
		{
			if(ISVAR((*j).s))
			{
				if(ACTION(go_to[(*j).s][(*i)->id])!=ERROR) 
				{
					std::cout<<"Warning: shift conflict\n";
					error=true;
				}
				go_to[(*j).s][(*i)->id]=BUILD_ACTION(0,(*j).node->id);
			}
			else
			{
				if(ACTION(action[(*j).s][(*i)->id])!=ERROR)
				{
					int resolv=getAction(red_prod,(*j).s,rules);
					if(resolv==SHIFT)
						action[(*j).s][(*i)->id]=BUILD_ACTION(SHIFT,(*j).node->id);
				}
				else
					action[(*j).s][(*i)->id]=BUILD_ACTION(SHIFT,(*j).node->id);
			}
		}
	}

#ifdef __DEBUG__
	printTables();
#endif
	
	//free memory
	foreach(i,std::list<node_t *>,*aut)
		delete *i;
	delete aut;
}

template<class V>
bool parser_t<V>::procError()
{
	if(err_fun==NULL) return true;
	std::list<int> expected;
	for(std::map<int,std::vector<int> >::iterator i=action.begin();
									i!=action.end();++i)
		if((*i).second[sp-1]!=ERROR) expected.push_back((*i).first);
	typename std::vector<V>::iterator ival=val_stack.begin();
	return err_fun(ival+1,ival+sp,expected,token,lex);
}

template<class V>
bool parser_t<V>::parse()
{
	sp=1;
	stack[0]=0;
	token=lex.nextToken();
	while((2+2)==4)
	{
		int ac;
		if(action[token].size()==0)
			ac=ERROR;
		else 
			ac=action[token][stack[sp-1]];
		switch(ACTION(ac))
		{
			case SHIFT :  shift(STATE(ac));break;
			case REDUCE : reduce(PRODUCTION(ac));break;
			case ERROR :  if(procError()) return false;break;
			case ACCEPT :  return (sp==2);
			default : std::cout<<"Error grave\n";break;
		}
	}
	return false;// que 2+2 != 4 ???!!!!?!?!?!?
}

#include "gram_loader.h"

#endif
