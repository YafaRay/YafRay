/****************************************************************************
 *
 *      mlex.h: Yafray fileformat lexical api
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
#ifndef __MLEX_H
#define __MLEX_H

#include<stdio.h>
#include"lex.h"


#define T_IDE 257
#define T_LITE 258
#define T_MODULATE 259
#define T_MODULATOR 260
#define T_FLOAT 261
#define T_SHADER 262
#define T_ATTR 263
#define T_TEXTURE 264
#define T_POINTS 265
#define T_FACES 266
#define T_MESH  267
#define T_OBJECT  268
#define T_CAMERA  269
#define T_RENDER  270
#define T_SPHERE  272
#define T_SCENE  274
#define T_SLIGHT  276
#define T_FILTER  277
#define T_LIGHT  279
#define T_TRANS  280
#define T_BACKG  281
#define T_REDEFINE  282


#define MMAX_CHARS 1024

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#ifdef HAVE_ZLIB

#include<zlib.h>

class inputGzip_t : public input_t
{
	public:
		inputGzip_t(gzFile f) {fin=f;};
		inputGzip_t(const char *fname) {fin=gzopen(fname,"rb");};
		virtual int getNextChar() { return gzgetc(fin); };
		virtual ~inputGzip_t() { if(!null()) gzclose(fin);};
		virtual bool null() {return fin==NULL;};
		gzFile getFile() {return fin;};
	protected:
		gzFile fin;
};

#endif

class mlex_t : public lex_t
{
	public:
		mlex_t(input_t *i) {in=i;getNextChar();l=c=1;};
		mlex_t(const mlex_t &s) {in=s.in;cchar=s.cchar;l=s.l;c=s.c;};
		mlex_t() {};

		void setInput(input_t *i) {in=i;getNextChar();l=c=1;};
		virtual int nextToken();
		virtual char *text() {return yytext;};

		virtual int line() {return l;};
		virtual int col() {return c;};
		virtual input_t *getInput() {return in;};
		virtual ~mlex_t() {};
	protected:
		bool match_blank(int c);
		bool match_sop(int c);
		bool match_id_first(int c);
		bool match_id_rest(int c);
		bool match_comment(int c);
		bool match_end_comment(int c);
		void eatXmlComment();
		bool match_float(char *s);
		void getNextChar() 
				{ if(cchar=='\n') {c=1;l++;} else c++;cchar=in->getNextChar();};
		int l,c;
		input_t *in;
		int cchar;
		char yytext[MMAX_CHARS+1];
};

using namespace std;
#include<list>
#define MAX_LOOK_AHEAD 10
#define MAX_FILE_INCLUDE 5

class preprocessor_t : public lex_t
{
	public:
		preprocessor_t() {sl=su=0;};
		virtual ~preprocessor_t() {};
		void setInput(lex_t *l) {lexs.push_back(l);};

		virtual int nextToken();
		virtual char *text() {return states[sl].text;};
		virtual int line() {return states[sl].l;};
		virtual int col() {return states[sl].c;};
		virtual input_t *getInput() {return NULL;};
	protected:
		void pushState();
		void popState();
		void popFile();
		void pushFile(const char *name);
		struct lexState_t
		{
			int token,l,c;
			//char *text;
			char text[MMAX_CHARS+1];
		};
		//list<lexState_t> states;
		lexState_t states[MAX_LOOK_AHEAD];
		int sl,su;
		list<lex_t *> lexs;
};

#endif
