/****************************************************************************
 *
 *      Lexical api for the gram loader
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
#ifndef __GRAMLEX_H
#define __GRAMLEX_H

#include<stdio.h>
#include"lex.h"


#define T_VAR 259
#define T_TOK 260
#define T_ALL 261
#define T_REDUCE 262
#define T_SHIFT 263
#define T_FUNC 264


#define MAX_CHARS 30

class gram_lex_t : public lex_t
{
	public:
		gram_lex_t(input_t *i) {in=i;getNextChar();l=c=1;};
		gram_lex_t(const gram_lex_t &s) {in=s.in;cchar=s.cchar;l=s.l;c=s.c;};
		virtual int nextToken();
		virtual char *text() {return yytext;};

		virtual int line() {return l;};
		virtual int col() {return c;};
		virtual input_t *getInput() {return in;};
		virtual ~gram_lex_t() {};

	protected:
		bool match_blank(int c);
		bool match_sop(int c);
		bool match_int(int c);
		bool match_id_first(int c);
		bool match_id_rest(int c);
		bool match_comment(int c);
		bool match_end_comment(int c);
		void getNextChar() 
				{ if(cchar=='\n') {c=1;l++;} else c++;cchar=in->getNextChar();};
		int l,c;
		input_t *in;
		int cchar;
		char yytext[MAX_CHARS+1];
};

#endif
