/****************************************************************************
 *
 *      mlex.cc: Yafray fileformat lexical implementation
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

#include<stdio.h>
#include<iostream>
#include "mlex.h"
using namespace std;
#include<string>
#define ISLET(c) ((((c)>='a') && ((c)<='z')) || (((c)>='A') && ((c)<='Z'))) 
#define ISDIG(c) (((c)>='0') && ((c)<='9'))

#define WARNING cerr<<"[Warning]: "
#define ERROR cerr<<"[Error]: "

char sops[]={
				'<',
				'>',
				'=',
				'/',
				0 };

struct reserved_t
{
	char *word;
	int token;
};

reserved_t reserved[]=
{
	{ "scene", T_SCENE },
	{ "modulate", T_MODULATE },
	{ "modulator", T_MODULATOR },
	{ "shader", T_SHADER },
	{ "attributes", T_ATTR },
	{ "texture", T_TEXTURE },
	{ "points", T_POINTS },
	{ "faces", T_FACES },
	{ "mesh", T_MESH },
	{ "object", T_OBJECT },
	{ "camera", T_CAMERA },
	{ "render", T_RENDER },
	{ "sphere", T_SPHERE },
	{ "softlight", T_SLIGHT },
	{ "filter", T_FILTER },
	{ "light", T_LIGHT },
	{ "transform", T_TRANS },
	{ "background", T_BACKG },
	{ "null", T_REDEFINE },
	{ NULL , 0 }
};

bool mlex_t::match_blank(int c)
{
	if(c==' ') return true;
	if(c=='\t') return true;
	if(c=='\n') return true;
	if(c=='\r') return true;
	return false;
}

bool mlex_t::match_sop(int c)
{
	for(int i=0;sops[i]!=0;++i)
		if(sops[i]==c) return true;
	return false;
}

bool mlex_t::match_id_first(int c)
{
	if(ISLET(c)) return true;
  return false;
}

bool mlex_t::match_id_rest(int c)
{
	if(ISLET(c)) return true;
	if(ISDIG(c)) return true;
	if(c=='_') return true;
	return false;
}

bool mlex_t::match_comment(int c)
{
	return (c=='#');
}

bool mlex_t::match_end_comment(int c)
{
	return (c=='\n');
}

void mlex_t::eatXmlComment()
{
	getNextChar();
	if(cchar!='-') 
	{
		cchar='!';
		return;
	}
	getNextChar();
	if(cchar!='-')
	{
		cchar='-';
		return;
	}
	getNextChar();

	while(1)
	{
		if(cchar==EOF)
		{
			ERROR<<"Unfinished comment at end of file\n";
			return;
		}
		if(cchar=='-')
		{
			getNextChar();
			if(cchar=='-')
			{
				getNextChar();
				if(cchar=='>')
				{
					getNextChar();
					return;
				}
			}
		}
		else getNextChar();
	}
}

bool mlex_t::match_float(char *s)
{
	int dots=0;
	int es=0;
	int i=0;
	if(s[0]=='-')
		i++;
	for(;s[i]!=0;++i)
	{
		if(s[i]=='.')
			dots++;
		else if(s[i]=='e')
		{
			es++;
			if(s[i+1]=='-') i++;
		}
		else if(!ISDIG(s[i])) return false;
	}
	return (dots<2);
}


int mlex_t::nextToken()
{
	bool repeat=true;
	while(repeat)
	{
		repeat=false;
	while(match_blank(cchar))
			getNextChar();
	
	if(match_sop(cchar))
	{
		yytext[0]=cchar;
		yytext[1]=0;
		getNextChar();
		if( (yytext[0]=='<') && (cchar=='!') )
		{
			eatXmlComment();
			repeat=true;
		}
		else return yytext[0];
	}
	if(match_id_first(cchar))
	{
		yytext[0]=cchar;
		getNextChar();
		int i;
		for(i=1;match_id_rest(cchar) && (i<MMAX_CHARS);++i,getNextChar())
			yytext[i]=cchar;
		yytext[i]=0;

		for(i=0;reserved[i].word!=NULL;++i)
			if(!strcmp(reserved[i].word,yytext)) return reserved[i].token;
		return T_IDE;
	}
	if(cchar=='"')
	{
		int i;
		getNextChar();
		for(i=0;(cchar!='"') && (i<MMAX_CHARS);++i,getNextChar())
			yytext[i]=cchar;
		yytext[i]=0;
		getNextChar();
		if(match_float(yytext)) return T_FLOAT;
		else return T_LITE;
	}
	if(cchar==EOF)
	{
		yytext[0]=0;
		return T_EOF;
	}
	}
	yytext[0]=cchar;
	yytext[1]=0;
	return 0;
}



void preprocessor_t::pushState()
{
	/*
	if(lexs.empty()) return;
	lex_t *lex=lexs.front();
	lexState_t state;
	state.token=lex->nextToken();
	while((state.token==T_EOF) && (lexs.size()>1))
	{
		popFile();
		lex=lexs.front();
		state.token=lex->nextToken();
	}
	state.l=lex->line();
	state.c=lex->col();
	state.text=new char[MMAX_CHARS+1];
	strcpy(state.text,lex->text());
	states.push_back(state);
	if(state.token==T_EOF) popFile();
	*/
	if(lexs.empty()) return;
	if(su>=MAX_LOOK_AHEAD)
	{
		ERROR<<"preprocesor stack overflow\n";
		return;
	}
	lex_t *lex=lexs.front();
	states[su].token=lex->nextToken();
	while((states[su].token==T_EOF) && (lexs.size()>1))
	{
		popFile();
		lex=lexs.front();
		states[su].token=lex->nextToken();
	}
	states[su].l=lex->line();
	states[su].c=lex->col();
	strcpy(states[su].text,lex->text());
	if(states[su].token==T_EOF) popFile();
	su++;
}

void preprocessor_t::popFile()
{
	if(lexs.empty()) return;
	if(lexs.size()>1)
	{
		delete lexs.front()->getInput();
		delete lexs.front();
	}
	lexs.pop_front();
}

void preprocessor_t::pushFile(const char *name)
{
	if(lexs.size()>=MAX_FILE_INCLUDE)
	{
		cerr<<"Maximal include depth reached, ignoring include\n";
		return;
	}
#ifdef HAVE_ZLIB
	inputGzip_t *input=new inputGzip_t(name);
#else
	inputFile_t *input=new inputFile_t(name);
#endif
	if(input->null())
	{
		cerr<<"Could not open included file : "<<name<<endl;
		cerr<<"To avoid unexpected results we stop here\n";
		cerr<<"Press enter to continue or CTRL-C to exit\n";
		getchar();
		delete input;
		return;
	}
	mlex_t *lex=new mlex_t;
	lex->setInput(input);
	lexs.push_front(lex);
}

#define EMPTYSTATES() (sl>=su)

void preprocessor_t::popState()
{
	/*
	if(states.empty()) return;
	delete [] states.front().text;
	states.pop_front();
	*/
	if(EMPTYSTATES()) return;
	sl++;
	if(sl==su) sl=su=0;
}

int preprocessor_t::nextToken()
{
	if( !EMPTYSTATES() && (states[sl].token==T_EOF) )
		return T_EOF;
	popState();
	while(1)
	{
		if(!EMPTYSTATES()) return states[sl].token;
		pushState();
		if(states[su-1].token!='<') 
			return states[sl].token;
		pushState();
		if( (states[su-1].token!=T_IDE) || strcmp(states[su-1].text,"include") )
			return states[sl].token;
		pushState();
		if( (states[su-1].token!=T_IDE) || strcmp(states[su-1].text,"file") )
			return states[sl].token;
		pushState();
		if(states[su-1].token!='=') 
			return states[sl].token;
		pushState();
		if(states[su-1].token!=T_LITE)
			return states[sl].token;
		string fname=states[su-1].text;
		pushState();
		if(states[su-1].token!='/')
			return states[sl].token;
		pushState();
		if(states[su-1].token!='>')
			return states[sl].token;
		cout<<"Loading file "<<fname<<endl;
		pushFile(fname.c_str());
		sl=su=0;
	}
	return T_EOF;
}

