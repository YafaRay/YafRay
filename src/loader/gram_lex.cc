/****************************************************************************
 *
 *      gram_lex.cc: Lexical implementation for the gram loader
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

#include<stdio.h>
#include<stdlib.h>
#include<iostream>
#include<string.h>
#include "gram_lex.h"
#define ISLET(c) ((((c)>='a') && ((c)<='z')) || (((c)>='A') && ((c)<='Z'))) 
#define ISDIG(c) (((c)>='0') && ((c)<='9'))

static char sops[]={
				'=',
				';',
				0 };

struct reserved_t
{
	char *word;
	int token;
};

static reserved_t reserved[]=
{
	{ "ALL" , T_ALL },
	{ "REDUCE" , T_REDUCE },
	{ "SHIFT" , T_SHIFT },
	{ NULL , 0 }
};

bool gram_lex_t::match_blank(int c)
{
	if(c==' ') return true;
	if(c=='\t') return true;
	if(c=='\n') return true;
	if(c=='\r') return true;
	return false;
}

bool gram_lex_t::match_sop(int c)
{
	for(int i=0;sops[i]!=0;++i)
		if(sops[i]==c) return true;
	return false;
}

bool gram_lex_t::match_id_first(int c)
{
	if(ISLET(c)) return true;
	if(c=='$') return true;
	if(c=='&') return true;

  return false;
}

bool gram_lex_t::match_id_rest(int c)
{
	if(ISLET(c)) return true;
	if(ISDIG(c)) return true;
	if(c=='_') return true;
	return false;
}

bool gram_lex_t::match_comment(int c)
{
	return (c=='#');
}

bool gram_lex_t::match_end_comment(int c)
{
	return (c=='\n');
}


int gram_lex_t::nextToken()
{
	while(match_comment(cchar) || match_blank(cchar))
	{
		while(match_blank(cchar))
			getNextChar();
		if(match_comment(cchar))
		{
			getNextChar();
			while(!match_end_comment(cchar))
				getNextChar();
			getNextChar();
		}
	}
	
	if(match_sop(cchar))
	{
		yytext[0]=cchar;
		yytext[1]=0;
		getNextChar();
		return yytext[0];
	}
	if(match_id_first(cchar))
	{
		int ret=T_TOK;
		int i=0;

		switch(cchar)
		{
			case '$': ret=T_VAR;break;
			case '&': ret=T_FUNC;break;
			default : yytext[i++]=cchar;break;
		}
		getNextChar();

		for(;match_id_rest(cchar) && (i<MAX_CHARS);++i,getNextChar())
			yytext[i]=cchar;
		yytext[i]=0;
		if(yytext[0]==0) return 0;
		for(i=0;reserved[i].word!=NULL;++i)
			if(!strcmp(reserved[i].word,yytext)) return reserved[i].token;
		return ret;
	}
	if(cchar=='\'')
	{
		getNextChar();
		yytext[0]=cchar;
		yytext[1]=0;
		getNextChar();
		if(cchar!='\'') return 0;
		getNextChar();
		return T_TOK;
	}
	if(cchar==EOF)
	{
		yytext[0]=0;
		return T_EOF;
	}
	yytext[0]=cchar;
	yytext[1]=0;
	return 0;
}

