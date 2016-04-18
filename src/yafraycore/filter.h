/****************************************************************************
 *
 * 			filter.h: Post rendering filter api 
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
#ifndef __FILTER_H
#define __FILTER_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "buffer.h"

__BEGIN_YAFRAY

class YAFRAYCORE_EXPORT filter_t
{
	public :
		virtual ~filter_t() {};
		virtual void apply(cBuffer_t &colorBuffer,fBuffer_t &ZBuffer,
											fBuffer_t &ABuffer)const=0;
};

class YAFRAYCORE_EXPORT filterDOF_t : public filter_t
{
	public :
		filterDOF_t ( GFLOAT ffocus,GFLOAT nrad,GFLOAT frad,GFLOAT scale=1.0 ) 
		{focus=ffocus;near_radius=nrad;far_radius=frad;exponent=scale;};
		virtual ~filterDOF_t () {};
		virtual void apply(cBuffer_t &colorBuffer,fBuffer_t &ZBuffer,
											fBuffer_t &ABuffer)const;
	protected :
		GFLOAT near_radius,far_radius,focus,exponent;
};

class YAFRAYCORE_EXPORT filterAntiNoise_t : public filter_t
{
	public :
		filterAntiNoise_t ( GFLOAT rad, GFLOAT tol ) {radius=rad;
																		delta=tol;};
		virtual ~filterAntiNoise_t () {};
		virtual void apply(cBuffer_t &colorBuffer,fBuffer_t &ZBuffer,
											fBuffer_t &ABuffer)const;
	protected :
		GFLOAT radius, delta;
};

__END_YAFRAY

#endif
