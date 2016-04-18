
#include "shaderback.h"

using namespace std;

__BEGIN_YAFRAY

shader_Background_t::shader_Background_t(shader_t* in)
{
	input = in;
}

color_t shader_Background_t::operator() (const vector3d_t &dir, renderState_t &state, bool filtered) const
{
	PFLOAT u=0, v=0;
	spheremap(dir, u, v);
	surfacePoint_t sp(NULL,point3d_t(u, v, 0),point3d_t(u, v, 0),-dir,-dir,0,0,color_t(0,0,0),1.0);
	color_t color = input->stdoutColor(state, sp, -dir);
	return color;
}

background_t *shader_Background_t::factory(paramMap_t &params,renderEnvironment_t &render)
{

	string _inname;
	shader_t *input=NULL;
	const string *inname=&_inname;
	
	params.getParam("input",inname);
	input=render.getShader(*inname);
	if(input!=NULL)
		return new shader_Background_t(input);
	else //return NULL;
		return new shader_Background_t(input);
}

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("shaderback",shader_Background_t::factory);
	std::cout<<"Registered Shader Background\n";
}

}
__END_YAFRAY
