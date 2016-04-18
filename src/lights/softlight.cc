
#include "softlight.h"
using namespace std;

__BEGIN_YAFRAY

softLight_t::softLight_t(const point3d_t &f, const color_t &c, CFLOAT p,
			int resol, int radius, GFLOAT biass, CFLOAT gli, CFLOAT glo, int glt)
{
	glow_int = gli;
	glow_ofs = glo;
	glow_type = glt;

	cube[0].set(0.0,-1.0,0.0);
	cube[1].set(1.0,0.0,0.0);
	cube[2].set(0.0,1.0,0.0);
	cube[3].set(-1.0,0.0,0.0);
	cube[4].set(0.0,0.0,1.0);
	cube[5].set(0.0,0.0,-1.0);

	from=f;
	color=c;
	res=resol;
	bias=biass;
	for(int i=0;i<6;++i)
	{
		buffer[i].set(res,res);
		//obj[i].set(res,res);
		for(int y=0;y<res;++y)
			for(int x=0;x<res;++x)
				buffer[i](x,y)=i*10000+y*100+x;
	}
	R=radius;
	R2=R*R;
	slim=sin(M_PI/4);
	pow=p;
	//FIRST FACE
	ad[0][SIDE_UP]=4;
	side[0][SIDE_UP]=SIDE_DOWN;
	flip[0][SIDE_UP]=false;

	ad[0][SIDE_RIGHT]=1;
	side[0][SIDE_RIGHT]=SIDE_LEFT;
	flip[0][SIDE_RIGHT]=false;
	
	ad[0][SIDE_DOWN]=5;
	side[0][SIDE_DOWN]=SIDE_DOWN;
	flip[0][SIDE_DOWN]=false;

	ad[0][SIDE_LEFT]=3;
	side[0][SIDE_LEFT]=SIDE_RIGHT;
	flip[0][SIDE_LEFT]=false;

	//SECOND FACE
	ad[1][SIDE_UP]=4;
	side[1][SIDE_UP]=SIDE_RIGHT;
	flip[1][SIDE_UP]=true;

	ad[1][SIDE_RIGHT]=2;
	side[1][SIDE_RIGHT]=SIDE_LEFT;
	flip[1][SIDE_RIGHT]=false;
	
	ad[1][SIDE_DOWN]=5;
	side[1][SIDE_DOWN]=SIDE_RIGHT;
	flip[1][SIDE_DOWN]=true;

	ad[1][SIDE_LEFT]=0;
	side[1][SIDE_LEFT]=SIDE_RIGHT;
	flip[1][SIDE_LEFT]=false;

	//THIRD FACE
	ad[2][SIDE_UP]=4;
	side[2][SIDE_UP]=SIDE_UP;
	flip[2][SIDE_UP]=true;

	ad[2][SIDE_RIGHT]=3;
	side[2][SIDE_RIGHT]=SIDE_LEFT;
	flip[2][SIDE_RIGHT]=false;
	
	ad[2][SIDE_DOWN]=5;
	side[2][SIDE_DOWN]=SIDE_UP;
	flip[2][SIDE_DOWN]=true;

	ad[2][SIDE_LEFT]=1;
	side[2][SIDE_LEFT]=SIDE_RIGHT;
	flip[2][SIDE_LEFT]=false;

	//FORTH FACE
	ad[3][SIDE_UP]=4;
	side[3][SIDE_UP]=SIDE_LEFT;
	flip[3][SIDE_UP]=false;

	ad[3][SIDE_RIGHT]=0;
	side[3][SIDE_RIGHT]=SIDE_LEFT;
	flip[3][SIDE_RIGHT]=false;
	
	ad[3][SIDE_DOWN]=5;
	side[3][SIDE_DOWN]=SIDE_LEFT;
	flip[3][SIDE_DOWN]=false;

	ad[3][SIDE_LEFT]=2;
	side[3][SIDE_LEFT]=SIDE_RIGHT;
	flip[3][SIDE_LEFT]=false;
	
	//FIFTH FACE
	ad[4][SIDE_UP]=2;
	side[4][SIDE_UP]=SIDE_UP;
	flip[4][SIDE_UP]=true;

	ad[4][SIDE_RIGHT]=1;
	side[4][SIDE_RIGHT]=SIDE_UP;
	flip[4][SIDE_RIGHT]=true;
	
	ad[4][SIDE_DOWN]=0;
	side[4][SIDE_DOWN]=SIDE_UP;
	flip[4][SIDE_DOWN]=false;

	ad[4][SIDE_LEFT]=3;
	side[4][SIDE_LEFT]=SIDE_UP;
	flip[4][SIDE_LEFT]=false;

	//SIXTH FACE
	ad[5][SIDE_UP]=2;
	side[5][SIDE_UP]=SIDE_DOWN;
	flip[5][SIDE_UP]=true;

	ad[5][SIDE_RIGHT]=1;
	side[5][SIDE_RIGHT]=SIDE_DOWN;
	flip[5][SIDE_RIGHT]=true;
	
	ad[5][SIDE_DOWN]=0;
	side[5][SIDE_DOWN]=SIDE_DOWN;
	flip[5][SIDE_DOWN]=false;

	ad[5][SIDE_LEFT]=3;
	side[5][SIDE_LEFT]=SIDE_DOWN;
	flip[5][SIDE_LEFT]=false;

	for(int i=0;i<6;++i)
		for(int j=0;j<4;++j)
		{
			int a=ad[i][j];
			int s=side[i][j];
			if(ad[a][s]!=i)
				cout<<"error ad cara "<<i<<" lado "<<j<<endl;
			if(side[a][s]!=j)
				cout<<"error side cara "<<i<<" lado "<<j<<endl;
			if(flip[i][j]!=flip[a][s])
				cout<<"error flip cara "<<i<<" lado "<<j<<endl;
		}
}

void softLight_t::init(scene_t &scene)
{
	cout<<"Building shadow maps ... ";
	cout.flush();
	fillCube(scene);
	cout<<"OK\n";
}

color_t softLight_t::illuminate(renderState_t &state,const scene_t &s,const surfacePoint_t sp,
																const vector3d_t &eye)const
{
	vector3d_t L = from-sp.P();
	vector3d_t dir = L;
	dir.normalize();
	vector3d_t shadowdir = sp.P()-from;
	shadowdir.normalize();
	const shader_t *sha= sp.getShader();
	GFLOAT x, y;
	int si = guessSide(shadowdir,x,y);
	int ix=(int)x, iy=(int)y;
	CFLOAT shadow = mixShadow(si, ix-R, iy-R, ix+R, iy+R, x, y, L.length());
	const color_t lcol(shadow*pow*color);
	energy_t ene(dir, lcol/(L*L));
	color_t col = sha->fromLight(state, sp, ene, eye);
	// not shadowed
	if (glow_int>0) col += glow_int * (pow*color) * getGlow(from, sp, eye, glow_ofs, glow_type);
	return col;
}

void softLight_t::fillSide(int s,const vector3d_t &corner,
		const vector3d_t &cx,const vector3d_t &cy,scene_t &scene)
{
	vector3d_t incx=(cx-corner)/res;
	vector3d_t incy=(cy-corner)/res;
	vector3d_t diry=corner+incx/2+incy/2;
	surfacePoint_t sp;
	renderState_t state;
	for(int y=0;y<res;++y)
	{
		vector3d_t dir=diry;
		for(int x=0;x<res;++x)
		{
			vector3d_t ray=dir;
			ray.normalize();
			if(!scene.firstHit(state,sp,from,ray,true))
			{
				buffer[s](x,y)=-1;
				//obj[s](x,y)=(object3d_t *)NULL;
			}
			else
			{
				buffer[s](x,y)=sp.Z();
				//obj[s](x,y)=sp.getObject();
			}
			dir=dir+incx;
		}
		diry=diry+incy;
	}
		
}

void softLight_t::fillCube(scene_t &scene)
{
	fillSide(0,vector3d_t(-1.0,-1.0 ,1.0),
						vector3d_t ( 1.0,-1.0 ,1.0),
						vector3d_t (-1.0,-1.0,-1.0),scene);
	fillSide(1,vector3d_t( 1.0,-1.0 ,1.0),
						vector3d_t ( 1.0 ,1.0 ,1.0),
						vector3d_t ( 1.0,-1.0,-1.0),scene);
	fillSide(2,vector3d_t( 1.0 ,1.0 ,1.0),
						vector3d_t (-1.0 ,1.0 ,1.0),
						vector3d_t ( 1.0 ,1.0,-1.0),scene);
	fillSide(3,vector3d_t(-1.0 ,1.0 ,1.0),
						vector3d_t (-1.0,-1.0 ,1.0),
						vector3d_t (-1.0 ,1.0,-1.0),scene);
	fillSide(4,vector3d_t(-1.0 ,1.0 ,1.0),
						vector3d_t ( 1.0 ,1.0 ,1.0),
						vector3d_t (-1.0,-1.0 ,1.0),scene);
	fillSide(5,vector3d_t(-1.0 ,1.0,-1.0),
						vector3d_t ( 1.0 ,1.0,-1.0),
						vector3d_t (-1.0,-1.0,-1.0),scene);
}

light_t *softLight_t::factory(paramMap_t &params,renderEnvironment_t &render)
{
	point3d_t from;
	color_t color(1,1,1);
	CFLOAT power=1.0;
	GFLOAT bias=0.1;
	int res=100,radius=1;

	params.getParam("from", from);
	params.getParam("color", color);
	params.getParam("power", power);
	params.getParam("res", res);
	params.getParam("radius", radius);
	params.getParam("bias", bias);

	// glow params
	CFLOAT gli=0, glo=0;
	int glt=0;
	params.getParam("glow_intensity", gli);
	params.getParam("glow_type", glt);
	params.getParam("glow_offset", glo);

	return new softLight_t(from, color, power, res, radius, bias, gli, glo, glt);
}

pluginInfo_t softLight_t::info()
{
	pluginInfo_t info;

	info.name="softlight";
	info.description="Shadow mapped point light";

	info.params.push_back(buildInfo<POINT>("from","Light position"));
	info.params.push_back(buildInfo<COLOR>("color","Light color"));
	info.params.push_back(buildInfo<FLOAT>("power",0,10000,1,"Light power"));
	info.params.push_back(buildInfo<INT>("res",64,65536,100,"Shadow map resolution"));
	info.params.push_back(buildInfo<INT>("radius",1,100,1,"Blur radius for the shadows"));
	info.params.push_back(buildInfo<FLOAT>("bias",0,100,0.1,"Minimun distance to \
				shadowing object. Used to avoid artifacts"));

	return info;
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("softlight",softLight_t::factory);
	std::cout<<"Registered softlight\n";
}

}
__END_YAFRAY
