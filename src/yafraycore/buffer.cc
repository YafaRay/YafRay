/****************************************************************************
 *
 * 			buffer.cc: Buffers (color and float) implementation 
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
#include "buffer.h"
using namespace std;
#include <iostream>
#include <cstdlib>

__BEGIN_YAFRAY
/*
cBuffer_t::cBuffer_t(int x, int y)
{
	data=new unsigned char [x*y*3];
	if(data==NULL)
	{
		cout<<"Error allocating memory in cBuffer\n";
		exit(1);
	}
	mx=x;
	my=y;
}

cBuffer_t::~cBuffer_t()
{
	if(data!=NULL)
		delete [] data;
}

void cBuffer_t::set(int x, int y)
{
	if(data!=NULL)
		delete [] data;
	data=new unsigned char [x*y*3];
	if(data==NULL)
	{
		cout<<"Error allocating memory in cBuffer\n";
		exit(1);
	}
	mx=x;
	my=y;
}

cBuffer_t & cBuffer_t::operator = (const cBuffer_t &source)
{
	if( (mx!=source.mx) || (my!=source.my) )
	{
		cout<<"Error, trying to assign  buffers of a diferent size\n";
	}
	if( (data == NULL) || (source.data == NULL) )
	{
		cout<<"Assigning unallocated buffers\n";
	}
	int total=mx*my*3;
	for(int i=0;i<total;++i) data[i]=source.data[i];
	
	return *this;
	
}
*/


fBuffer_t::fBuffer_t(int x, int y)
{
	data=new GFLOAT [x*y];
	if(data==NULL)
	{
		cout<<"Error allocating memory in cBuffer\n";
		exit(1);
	}
	mx=x;
	my=y;
}

fBuffer_t::~fBuffer_t()
{
	if(data!=NULL)
		delete [] data;
}

void fBuffer_t::set(int x, int y)
{
	if(data!=NULL)
		delete [] data;
	data=new GFLOAT [x*y];
	if(data==NULL)
	{
		cout<<"Error allocating memory in cBuffer\n";
		exit(1);
	}
	mx=x;
	my=y;
}

fBuffer_t & fBuffer_t::operator = (const fBuffer_t &source)
{
	if( (mx!=source.mx) || (my!=source.my) )
	{
		cout<<"Error, trying to assign  buffers of a diferent size\n";
	}
	if( (data == NULL) || (source.data == NULL) )
	{
		cout<<"Assigning unallocated buffers\n";
	}
	int total=mx*my;
	for(int i=0;i<total;++i) data[i]=source.data[i];
	
	return *this;
	
}
__END_YAFRAY
