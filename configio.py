import os
import sys
from globalinfo import *

def write_conf(C):
	double_coords=0
	yafray_namespace='yafray'
	if double_coords :
		min_raydist="0.000000000005"
	else:
		min_raydist="0.00005"
	
	if sys.platform == 'win32':
		compiler='MSVC'
	else:
		compiler='GCC'

	if os.path.exists(config_file):
		print "Using config file: "+config_file
	else:
		print "Creating config file:"+config_file
		config=open(config_file,'w')
		config.write("//Config file header created by scons\n\n")
		config.write("#ifndef __CONFIG_H\n")
		config.write("#define __CONFIG_H\n")
		config.write("#define "+compiler+"\n")
		
		# !EXR!
		config.write("#define HAVE_EXR %d\n"%(C.exr.present));
		
		config.write("#define HAVE_JPEG %d\n"%(C.jpeg.present))
		config.write("#define HAVE_PTHREAD %d\n"%(C.pthread.present))
		config.write("#define HAVE_ZLIB %d\n"%(C.zlib.present))
		if sys.platform == 'darwin' :  # STUPID HARDCODED PATH FIXME
			config.write("#define LIBPATH \"/usr/local/lib\"\n")
		else:
			config.write("#define LIBPATH \"%s\"\n"%(C.libpath))
		config.write("#define MIN_RAYDIST %s\n"%(min_raydist))
		config.write("#define PACKAGE \"%s\"\n"%(package))
		config.write("#define VERSION \"%s\"\n"%(version))
		config.write("\n")
		config.write("#define __BEGIN_YAFRAY namespace "+yafray_namespace+" {\n");
		config.write("#define __END_YAFRAY }\n")
		config.write("\n")
		config.write("__BEGIN_YAFRAY\n");
		config.write("typedef float CFLOAT;\n");
		config.write("typedef float GFLOAT;\n");
		if double_coords:
			config.write("typedef double PFLOAT;\n");
		else:
			config.write("typedef float PFLOAT;\n");
		config.write("__END_YAFRAY\n");

		if sys.platform == 'win32' :
			config.write("#ifdef BUILDING_YAFRAYCORE\n")
			config.write("#define YAFRAYCORE_EXPORT __declspec(dllexport)\n")
			config.write("#else \n")
			config.write("#define YAFRAYCORE_EXPORT __declspec(dllimport)\n")
			config.write("#endif \n")

			config.write("#ifdef BUILDING_YAFRAYPLUGIN\n")
			config.write("#define YAFRAYPLUGIN_EXPORT __declspec(dllexport)\n")
			config.write("#else \n")
			config.write("#define YAFRAYPLUGIN_EXPORT __declspec(dllimport)\n")
			config.write("#endif \n")
		else :
			config.write("#define YAFRAYPLUGIN_EXPORT\n")
			config.write("#define YAFRAYCORE_EXPORT\n")

		config.write("#endif\n");
		config.close()
