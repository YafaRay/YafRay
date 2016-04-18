
// Compile with g++ -o test -Wall test.cc external.cc -ldl


#include "external.h"
#include <dlfcn.h>
#include <iostream>

using namespace std;
using namespace yafray;

int main()
{
	void *handle=dlopen(".libs/libyafrayplugin.so",RTLD_NOW);
	if(handle==NULL)
	{
		cerr<<"dlerror: "<<dlerror()<<endl;
		return 1;
	}
	cout<<"Plugin loaded"<<endl;
	yafrayConstructor *constructor;

	constructor=(yafrayConstructor *)dlsym(handle,YAFRAY_SYMBOL);
	if(constructor==NULL)
	{
		cerr<<"dlerror: "<<dlerror()<<endl;
		return 1;
	}
	cout<<"constructor OK"<<endl;

	yafrayInterface_t *yafray=constructor(1,"/usr/local/lib/yafray");
	cout<<"Got yafray!"<<endl;
	

	paramMap_t params;
	params["type"]=parameter_t("sunsky");
	params["name"]=parameter_t("backg");
	params["turbidity"]=parameter_t(40);
	params["from"]=parameter_t(point3d_t(-2,-3,1));

	yafray->addBackground(params);

	params.clear();
	params["name"]=parameter_t("camara");
	params["resx"]=parameter_t(640);
	params["resy"]=parameter_t(480);
	params["focal"]=parameter_t(1.038292);
	params["from"]=parameter_t(point3d_t(7.229358,-11.235083,2.854462));
	params["to"]=parameter_t(point3d_t(6.732925,-10.485328,2.816943));
	params["up"]=parameter_t(point3d_t(6.963828,-10.886383,4.153296));

	yafray->addCamera(params);

	params.clear();
	list<paramMap_t> lparams;
	
	params["type"]=parameter_t("generic");
	params["name"]=parameter_t("blender_default");
	params["color"]=parameter_t(1.0,1.0,1.0);
	params["hard"]=parameter_t(50);

	yafray->addShader(params,lparams);

	
	vector<point3d_t> *verts=new vector<point3d_t>;
	verts->push_back(point3d_t(30.513753,30.938159,0.003869));
	verts->push_back(point3d_t(30.513753,-30.124275,0.003869));
	verts->push_back(point3d_t(-30.548681,-30.124272,0.003869));
	verts->push_back(point3d_t(-30.548675,30.938164,0.003869));

	vector<int> faces;
	faces.push_back(0);//first face
	faces.push_back(3);
	faces.push_back(2);
	faces.push_back(2);//second face
	faces.push_back(1);
	faces.push_back(0);

	vector<string> shaders(1);
	shaders[0]="blender_default";
	vector<int>    faceshader;// no shaders per face

	yafray->addObject_trimesh("obj",verts,faces,NULL,NULL,shaders,faceshader,0,
			true,true,true,false,false,color_t(0,0,0),color_t(0,0,0),1.0);
	
	params.clear();
	params["camera_name"]=parameter_t("camara");
	params["AA_passes"]=parameter_t(4);
	params["AA_minsamples"]=parameter_t(4);
	params["raydepth"]=parameter_t(5);
	params["AA_threshold"]=parameter_t(0.03);
	params["background_name"]=parameter_t("backg");
	params["outfile"]=parameter_t("salida.tga");
	
	yafray->render(params);
	
	delete yafray;
	cout<<"Yafray destroyed"<<endl;
	dlclose(handle);
	cout<<"Plugin closed"<<endl;
	return 0;
}

