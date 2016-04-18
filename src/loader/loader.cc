/****************************************************************************
 *
 *      loader.cc: Yafray file loader (main function) implementation
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

#include "parser.h"
#include "msin.h"
#include "render.h"
#include "ipc.h"

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#ifdef HAVE_ZLIB
#include<zlib.h>
#endif

#ifdef MSVC
#define NOMINMAX
#include<windows.h>
#include<winunistd.h>
bool useZ=false;
#else
#include<unistd.h>
#endif

//#include <getopt.h>


using namespace yafray;

#define WARNING cerr<<"[Warning]: "
#define ERRORMSG cerr<<"[Error]: "

string path[]=
{
	"./",
	"/etc/",
	"/usr/local/etc/",
	"/usr/etc/",
	""
};
		
#define GRAMAR_FILE "gram.yafray"

using namespace std;
#ifdef WIN32 
string find_path ()
{
	HKEY	hkey;
//	DWORD	dwDisposition;
	DWORD dwType, dwSize;

//	if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, TEXT("Software\\YafRay Team\\YafRay"),
//	 0, NULL, 0, 0, NULL, &hkey, &dwDisposition)!=ERROR_SUCCESS)
	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"Software\\YafRay Team\\YafRay",0,KEY_READ,&hkey)==ERROR_SUCCESS)
	{
		dwType = REG_EXPAND_SZ;
	 	dwSize = MAX_PATH;
		DWORD dwStat;

		char *pInstallDir=(char *)malloc(MAX_PATH);

  		dwStat=RegQueryValueEx(hkey, TEXT("InstallDir"), 
			NULL, NULL,(LPBYTE)pInstallDir, &dwSize);
		
		if (dwStat == NO_ERROR) 
			return string(pInstallDir);
		else
			ERRORMSG << "Couldn't READ \'InstallDir\' value.\n";
		
		free(pInstallDir);
		RegCloseKey(hkey);
	}	
	else
		ERRORMSG << "Couldn't FOUND registry key.\n";

	cout <<"\n\nPlease fix your registry Maybe you need add/modify\n\n" 
				<< " HKEY_LOCAL_MACHINE\\Software\\YafRay Team\\YafRay\\InstallDir\n\n"
				<< "key at registry. You can use \"regedit.exe\" for modify it at "
				<< "your own risk.\n"
				<< "If you are unsure, reinstall YafRay\n";

	return string("");

}

FILE *find_gramar(string thePath)
{

	if (thePath!="")
	{
		cout << "Trying open grammar " << thePath+"\\"+GRAMAR_FILE << endl;
		thePath+="\\";
		thePath+=GRAMAR_FILE;
		FILE *it=fopen(thePath.c_str(),"rb");
		if (it!=NULL) return it;
		ERRORMSG<<"Couldn't open grammar.\n";
	}
	
	return NULL;
}
#else

FILE *find_gramar(string unused)
{
	for(int i=0;path[i]!="";++i)
	{
		string file=path[i]+GRAMAR_FILE;
		FILE *it=fopen(file.c_str(),"rb");
		if(it!=NULL) return it;
	}
	ERRORMSG<<"Couldn't find "<<GRAMAR_FILE<<" in any of these directories:\n";
	for(int i=0;path[i]!="";++i)
		cerr<<path[i]<<endl;
	return NULL;
}
#endif

int main(int argc,char **argv)
{
	cout<<"Starting YafRay ..."<<endl;
	int cpus=1;
	string strategy("threaded");
	string thePath;
	float scxmin=-2,scxmax=2,scymin=-2,scymax=2;

	while((2+2)==4)
	{
		int c=getopt(argc,argv,"zvr:c:p:s:");
		if(c==-1) break;
		switch(c)
		{
			case 's' : strategy=optarg;break;
			case 'c' : cpus=atoi(optarg);break;
#ifdef HAVE_ZLIB
			case 'z' : useZ=1;break;
#else
			case 'z' : ERRORMSG<<"Yafray was compiled without zlib support\n";break;
#endif
			case 'v' :{ cout << "Yet Another Free RayTracer version " << VERSION << endl;return 1;}break;					 
//#ifdef WIN32
			case 'p' : thePath=optarg;break;
//#endif
			case 'r' :
							 {
								 string num;
								 int i;
								 for(i=0;optarg[i];++i) if(optarg[i]==':') break; else num+=optarg[i];
								 scxmin=atof(num.c_str());num="";
								 if(optarg[i]!=0) for(i++;optarg[i];++i) if(optarg[i]==':') break; else num+=optarg[i];
								 scxmax=atof(num.c_str());num="";
								 if(optarg[i]!=0) for(i++;optarg[i];++i) if(optarg[i]==':') break; else num+=optarg[i];
								 scymin=atof(num.c_str());num="";
								 if(optarg[i]!=0) for(i++;optarg[i];++i) if(optarg[i]==':') break; else num+=optarg[i];
								 scymax=atof(num.c_str());
								 cout<<"Rendering region: "<<scxmin<<" "<<scxmax<<" "<<scymin<<" "<<scymax<<endl;
							 }break;
			default : ERRORMSG<<"Unknown option "<<(char)c<<endl;
		}
	}
	if(cpus<1) cpus=1;
								 
        if ((strategy != "threaded" && strategy != "fork" &&strategy != "mono") ||
            (optind>=argc))
	{
		cerr<<"Usage: yafray [options] <file to render>\n";
		cerr<<"\nOptions :\n";
		cerr<<"\t-s Render using the specified strategy.  Valid values are\n";
		cerr<<"\t\t\"threaded\": Multi-threaded (default)\n";
		cerr<<"\t\t\"mono\": Single process\n";
		cerr<<"\t\t\"fork\": Multi-process\n\n";
		cerr<<"\t-c N\tNumber of threads/processes to use\n";
#ifdef HAVE_ZLIB
		cerr<<"\t-z\tUse Net optimized\n\n";
#endif
#ifdef WIN32
		cerr<<"\t-p PATH\tYafRay's installation path\n\n";
#else
		cerr<<"\t-p <PATH>\tSpecify alternative plugin path\n\n";
#endif
		cerr<<"\t-r min_x:max_x:min_y:max_y\tRender region, values between -1 and 1\n";
		cerr<<"\t                          \twhole image is -r -1:1:-1:1\n\n";
		cerr<<"\t-v\tYafRay Version\n\n";
		return 1;
	}
#ifdef HAVE_ZLIB
	inputGzip_t input(argv[optind]);
#else
	inputFile_t input(argv[optind]);
#endif

	if(input.null())
	{
		cerr<<"Could not open input file\n";
		return 1;
	}
	
	cout<<"Loading grammar ..."<<endl;
	parser_t<lval_t>::mapJoins_t joins;

	joins["join_copy"]=join_copy;
	joins["join_shader"]=join_shader;
	joins["join_copy1"]=join_copy1;
	joins["join_lmod"]=join_lmod;
	joins["join_empty_lmod"]=join_empty_lmod;
	joins["join_lattr"]=join_lattr;
	joins["join_empty_lattr"]=join_empty_lattr;
	joins["join_color"]=join_color;
	joins["join_colorA"]=join_colorA;
	joins["join_attr"]=join_attr;
	joins["join_attrf"]=join_attrf;
	joins["join_null"]=join_null;
	joins["join_litem"]=join_litem;
	joins["join_empty_litem"]=join_empty_litem;
	joins["join_label"]=join_label;
	joins["join_llabel"]=join_llabel;
	joins["join_empty_llabel"]=join_empty_llabel;
	//joins["join_tdata"]=join_tdata;
	joins["join_point"]=join_point;
	joins["join_lpoint"]=join_lpoint;
	joins["join_empty_lpoint"]=join_empty_lpoint;
	joins["join_points"]=join_points;
	joins["join_face"]=join_face;
	joins["join_lface"]=join_lface;
	joins["join_empty_lface"]=join_empty_lface;
	joins["join_faces"]=join_faces;
	joins["join_mesh"]=join_mesh;
	joins["join_oattr"]=join_oattr;
	joins["join_loattr"]=join_loattr;
	joins["join_empty_loattr"]=join_empty_loattr;
	joins["join_object"]=join_object;
	joins["join_camera"]=join_camera;
	joins["join_sphere"]=join_sphere;
	joins["join_generic"]=join_generic;
	joins["join_transform"]=join_transform;
	
	parser_t<lval_t>::mapToks_t toks;

	toks[">"]='>';
	toks["<"]='<';
	toks["/"]='/';
	toks["="]='=';
	toks["T_IDE"]=T_IDE;
	toks["T_LITE"]=T_LITE;
	toks["T_FLOAT"]=T_FLOAT;
	toks["T_ATTR"]=T_ATTR;
	toks["T_MODULATOR"]=T_MODULATOR;
	toks["T_SHADER"]=T_SHADER;
	toks["T_TEXTURE"]=T_TEXTURE;
	toks["T_POINTS"]=T_POINTS;
	toks["T_FACES"]=T_FACES;
	toks["T_MESH"]=T_MESH;
	toks["T_OBJECT"]=T_OBJECT;
	toks["T_CAMERA"]=T_CAMERA;
	toks["T_RENDER"]=T_RENDER;
	toks["T_SPHERE"]=T_SPHERE;
	toks["T_SCENE"]=T_SCENE;
	toks["T_SLIGHT"]=T_SLIGHT;
	toks["T_FILTER"]=T_FILTER;
	toks["T_LIGHT"]=T_LIGHT;
	toks["T_TRANS"]=T_TRANS;
	toks["T_BACKG"]=T_BACKG;
	toks["T_REDEFINE"]=T_REDEFINE;

#ifdef WIN32
	if (thePath=="") thePath=find_path();
#endif
	
	FILE *in=find_gramar(thePath);
	if(in==NULL) return 1;
	

	mlex_t lex;
	preprocessor_t preprocesor;
	
	parser_t<lval_t> sintax(in,toks,joins,preprocesor);

	if(sintax.error)
	{
		ERRORMSG<<"Corrupted grammar definition\n";
		return 1;
	}
	cerr<<"Starting parser ...\n";
	

	lex.setInput(&input);
	preprocesor.setInput(&lex);
	
	render_t::strategy_t scene_strat;
	if (strategy == "threaded") 
		scene_strat = render_t::THREAD;
	else if (strategy == "fork")
		scene_strat = render_t::FORK;
	else if (strategy == "mono")
		scene_strat = render_t::MONO;
	else
		scene_strat = render_t::THREAD;

	// FORCE
	//scene_strat = render_t::MONO;

	if(sintax.parse())
	{
		cerr<<"Parsing OK\n\n";
#ifdef WIN32
		render_t render(cpus, scene_strat, thePath+"\\plugins");
#else
		string plugPath;
		if(thePath.length() > 0) plugPath = thePath;
		else plugPath = string(LIBPATH) + "/yafray";
		render_t render(cpus, scene_strat, plugPath);
#endif
		render.setRegion(scxmin,scxmax,scymin,scymax);
		render.call(sintax.result().ast);
		delete sintax.result().ast;
	}
  else
  {
    cerr<<"Failed\n";
    cerr<<"Error at line "<<sintax.line()<<" column "<<sintax.col()
            <<" token "<<sintax.text()<<"\n";
  }
	return 0;
}
