/****************************************************************************
 *
 *      lex.h: Generic lexical api
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
#ifndef __LEX_H
#define __LEX_H

#include<stdio.h>

class input_t
{
	public:
		virtual int getNextChar()=0;
		virtual ~input_t() {};
		virtual bool null()=0;
};

class inputFile_t : public input_t
{
	public:
		inputFile_t(FILE *f) {fin=f;};
		inputFile_t(const char *fname) {fin=fopen(fname,"rb");};
		virtual int getNextChar() { return fgetc(fin); };
		virtual ~inputFile_t() {if(!null()) fclose(fin);};
		virtual bool null() {return fin==NULL;};
	protected:
		FILE *fin;
};

class inputString_t : public input_t
{
	public:
		inputString_t(char *s) {str=(unsigned char *)s;i=0;};
		virtual int getNextChar() 
			{if(str[i]==0) return EOF;else return str[i++];};
		virtual ~inputString_t() {};
		virtual bool null() {return str==NULL;};
	protected:
		unsigned char *str;
		int i;
};

#define T_EOF 256

class lex_t
{
	public:
		virtual int nextToken()=0;
		virtual char *text()=0;

		virtual int line()=0;
		virtual int col()=0;
		virtual input_t * getInput()=0;
		virtual ~lex_t() {};
		
};

#endif
