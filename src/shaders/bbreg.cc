
#include "basicblocks.h"
#include "basictex.h"
#include "imagebackg.h"

__BEGIN_YAFRAY

extern "C"
{
	
YAFRAYPLUGIN_EXPORT void registerPlugin(renderEnvironment_t &render)
{
	render.registerFactory("float2color", floatToColor_t::factory);
	render.registerFactory("color2float", colorToFloat_t::factory);
	render.registerFactory("colorband", colorBandNode_t::factory);
	render.registerFactory("coords", coordsNode_t::factory);
	render.registerFactory("mul", mulNode_t::factory);
	render.registerFactory("sin", sinNode_t::factory);
	render.registerFactory("phong", phongNode_t::factory);
	
	render.registerFactory("clouds", cloudsNode_t::factory);
	render.registerFactory("marble", marbleNode_t::factory);
	render.registerFactory("wood", woodNode_t::factory);
	render.registerFactory("RGB", rgbNode_t::factory);
	render.registerFactory("HSV", hsvNode_t::factory);
	render.registerFactory("conetrace", coneTraceNode_t::factory);
	render.registerFactory("fresnel", fresnelNode_t::factory);
	render.registerFactory("image", imageNode_t::factory);
	render.registerFactory("gobo", goboNode_t::factory);
	render.registerFactory("voronoi", voronoiNode_t::factory);
	render.registerFactory("musgrave", musgraveNode_t::factory);
	render.registerFactory("distorted_noise", distortedNoiseNode_t::factory);
	render.registerFactory("gradient", gradientNode_t::factory);
	render.registerFactory("random_noise", randomNoiseNode_t::factory);
	
	render.registerFactory("clouds", textureClouds_t::factory);
	render.registerFactory("marble", textureMarble_t::factory);
	render.registerFactory("wood", textureWood_t::factory);
	render.registerFactory("image", textureImage_t::factory);
	render.registerFactory("voronoi", textureVoronoi_t::factory);
	render.registerFactory("musgrave", textureMusgrave_t::factory);
	render.registerFactory("distorted_noise", textureDistortedNoise_t::factory);
	render.registerFactory("gradient", textureGradient_t::factory);
	render.registerFactory("random_noise", textureRandomNoise_t::factory);

	render.registerFactory("image", imageBackground_t::factory);
	std::cout << "Registered basicblocks\n";
}

}

__END_YAFRAY
