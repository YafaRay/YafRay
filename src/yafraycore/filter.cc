/****************************************************************************
 *
 * 			filter.cc: Post rendering filter implementation 
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
#include "filter.h"
#include "color.h"
using namespace std;
#include <iostream>
#include <cstdio>
//#include <cmath>

__BEGIN_YAFRAY

color_t mix_circle(cBuffer_t &colorBuffer,fBuffer_t &ZBuffer,GFLOAT minz,
													int x,int y,GFLOAT rad,GFLOAT tol)
{

	int minx=x-(int)rad;
	if(minx<0) minx=0;
	int maxx=x+(int)rad;
	if(maxx>=colorBuffer.resx()) maxx=colorBuffer.resx()-1;
	
	int miny=y-(int)rad;
	if(miny<0) miny=0;
	int maxy=y+(int)rad;
	if(maxy>=colorBuffer.resy()) maxy=colorBuffer.resy()-1;

	GFLOAT div=0;
	color_t color(0.0,0.0,0.0);
	color_t pixel;
	for(int i=miny;i<=maxy;++i)
		for(int j=minx;j<=maxx;++j)
		{
			if( (ZBuffer(j,i)>=(minz-tol)) )
			{
				colorBuffer(j,i)>>pixel;
				color=color+pixel;
				div+=1.0;
			}
		}
	if(div>1.0)
		color=color/((CFLOAT)div);
	return color;
}

void filterDOF_t::apply(cBuffer_t &colorBuffer,fBuffer_t &ZBuffer,
											fBuffer_t &ABuffer)const
{
	cBuffer_t temp(ZBuffer.resx(),ZBuffer.resy());
	GFLOAT maxradius,radius;

	maxradius=(near_radius>far_radius) ? near_radius : far_radius;

	printf("Applying DOF filter ... ");
	fflush(stdout);
	for(int t=0;t<(int)maxradius;++t)
	{
		printf("\rApplying DOF filter [ %d / %d ] ...   ",t,(int)maxradius);
		fflush(stdout);
		for(int i=0;i<ZBuffer.resy();++i)
			for(int j=0;j<ZBuffer.resx();++j)
			{
				GFLOAT dis=ZBuffer(j,i)-focus;
				if(dis<0) radius=near_radius;
				else radius=far_radius;
				dis=(fabs(dis)-0.1*focus*exponent)/focus;
				GFLOAT rad=dis*radius;
				color_t color;
				if(rad>=(GFLOAT)t)
					color=mix_circle(colorBuffer,ZBuffer,ZBuffer(j,i),j,i,1,focus*0.1);
				else
					colorBuffer(j,i)>>color;
				temp(j,i)<<color;
			}
		colorBuffer=temp;
	}
	printf("\rApplying DOF filter [ %d / %d ] ...   ",(int)maxradius,
					(int)maxradius);
	fflush(stdout);
	cout<<"OK\n";
}

void filterAntiNoise_t::apply(cBuffer_t &colorBuffer,fBuffer_t &ZBuffer,
												fBuffer_t &ABuffer)const
{
	int i;
	cBuffer_t temp(colorBuffer.resx(),colorBuffer.resy());

	printf("Applying AntiNoise filter ... ");
	fflush(stdout);
	for(i=0;i<colorBuffer.resy();++i)
	{
		printf("\rApplying AntiNoise filter [ %d / %d ] ...   ",i,colorBuffer.resy());
		fflush(stdout);
		for(int j=0;j<colorBuffer.resx();++j)
		{
			color_t color(0.0,0.0,0.0);
			color_t pixel;
			int ncolor=0;
			GFLOAT auxradius=0;
			color_t actual;
			colorBuffer(j,i)>>actual;

			for(int auxi=(i-(int)fabs(radius));auxi<=(i+(int)fabs(radius));auxi++)
			{
				for(int auxj=(j-(int)auxradius);auxj<=(j+(int)auxradius);auxj++)
				{
					if(auxi>=0 && auxj>=0 && auxi<colorBuffer.resy() && auxj<colorBuffer.resx())
					{
						colorBuffer(auxj,auxi)>>pixel;
						GFLOAT max=maxAbsDiff(pixel,actual);
						if(max < delta)
						{
							color=color+pixel;
							ncolor++;
						}
					}
				}	
				
				if (auxi < i)
						auxradius++;
				else
						auxradius--;
			}
			color=color/(CFLOAT)ncolor;
			temp(j,i)<<color;
		}
	}
	colorBuffer=temp;
	printf("\rApplying AntiNoise filter [ %d / %d ] ...   ",i,colorBuffer.resy());
	fflush(stdout);
	cout<<"OK\n";
}
__END_YAFRAY
