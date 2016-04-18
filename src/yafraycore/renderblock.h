#ifndef __RENDERBLOCK_H
#define __RENDERBLOCK_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include<vector>
#include <list>
#include"output.h"

__BEGIN_YAFRAY

struct renderArea_t
{
	renderArea_t(int x,int y,int w,int h):X(x),Y(y),W(w),H(h),
		realX(x),realY(y),realW(w),realH(h),image(w*h),depth(w*h),resample(w*h),fake(false)
	{};
	renderArea_t():fake(false) {};

	void set(int x,int y,int w,int h)
	{
		realX=X=x;
		realY=Y=y;
		realW=W=w;
		realH=H=h;
		image.resize(w*h);
		depth.resize(w*h);
		resample.resize(w*h);
	}
	void setReal(int x,int y,int w,int h)
	{
		realX=x;
		realY=y;
		realW=w;
		realH=h;
	}
	bool checkResample(CFLOAT threshold);
	bool out(colorOutput_t &o);

	colorA_t & imagePixel(int x,int y) {return image[(y-Y)*W+(x-X)];};
	PFLOAT & depthPixel(int x,int y)   {return depth[(y-Y)*W+(x-X)];};
	bool  resamplePixel(int x,int y)  {return resample[(y-Y)*W+(x-X)];};

	int X,Y,W,H,realX,realY,realW,realH;
	std::vector<colorA_t> image;
	std::vector<PFLOAT> depth;
	std::vector<bool> resample;
	bool fake;
};


class blockSpliter_t
{
	public:
		blockSpliter_t(int w,int h,int b);
		
		void getArea(renderArea_t &area);

		bool empty()const {return regions.empty();};
		int size()const {return regions.size();};

	protected:
		struct region_t
		{
			int x,y,w,h;
			int rx,ry,rw,rh;
		};
		int width,height,block;
		std::vector<region_t> regions;
};

__END_YAFRAY

#endif
