import shutil
import re
import os
import globalinfo

srcroot = os.getcwd()

def init(args): pass

def get_libpath(args): return os.getcwd()+'/osxpaknocvs/Package_Root/usr/local/lib'
def get_pluginpath(args): return os.getcwd()+'/osxpaknocvs/Package_Root/usr/local/lib/yafray'
def get_binpath(args): return os.getcwd()+'/osxpaknocvs/Package_Root/usr/local/bin'
def get_confpath(args): return os.getcwd()+'/osxpaknocvs/Package_Root/private/etc'
def get_cxxflags(args):
	debug=args.get('debug',0)
	flags='-Wall -DHAVE_CONFIG_H'
	if pthread.present(args) :
		flags+=' -D_PTHREADS'
	if debug:
		flags+=' -O3 -ffast-math -ggdb'
	else:
		flags+=' -O3 -ffast-math -fomit-frame-pointer'
	return flags

class exr(globalinfo.library):
	PATH = None
	C_ID = 'EXR'
	def present(args):
		exr.PATH = args.get('exr_path',None)
		dyn = args.get('dynamic_exr',False)
		if exr.PATH is None and dyn : exr.PATH = srcroot + '/../libs/osx/openexr'
		if exr.PATH is None and not dyn : exr.PATH = srcroot + '/../libs/osx/openexr_static'
		if os.path.exists(exr.PATH + "/include/OpenEXR/half.h"):
			return True
		else:
			return False
		
	def get_include(args): return [ exr.PATH + "/include/OpenEXR" ]
	def get_libpath(args): return [ exr.PATH + "/lib" ]
	def get_libs(args): return ['IlmImf', 'Imath', 'Iex', 'Half', 'z']

class jpeg(globalinfo.library):
	PATH = srcroot + "/../libs/osx/jpeg"
	C_ID = 'JPEG'

	def present(args): return os.path.exists(jpeg.PATH + "/include/jpeglib.h")
	def get_include(args): return [jpeg.PATH + "/include"]
	def get_libpath(args): return [jpeg.PATH + "/lib"]
	def get_libs(args): return ['jpeg']

class pthread(globalinfo.library):
	C_ID = 'PTHREAD'

	def present(args): return True
	def get_include(args): return []
	def get_libpath(args): return []
	def get_libs(args): return ['pthread']

class zlib(globalinfo.library):
	C_ID = 'ZLIB'

	def present(args): return True
	def get_include(args): return []
	def get_libpath(args): return []
	def get_libs(args): return ['z']

class dynload(globalinfo.library):
	C_ID = 'DL'

	def present(args): return True
	def get_include(args): return []
	def get_libpath(args): return []
	def get_libs(args): return ['dl']

def derive_shared_env(common_env):
	env=common_env.Copy(SHLINKFLAGS='-dynamic -dynamiclib -dylib ',SHLIBSUFFIX='.dylib')
	env.Append(CXXFLAGS=' -dynamic ')
	return env

def derive_plugin_env(common_env):
	env=common_env.Copy(SHLINKFLAGS='-Wall -bundle')
	env.Append(CXXFLAGS=' -dynamic ')
	return env

def derive_program_env(common_env):
	return common_env.Copy()

def package_rule(args):
	return ('osxpackage',makeosxpackage)

# AUXILIAR



def replaceInFile(remp, name, out):
	inf=open(name,'r')
	outf=open(out,'w')
	line=inf.readline();
	while line:
		for x in remp:
			line=re.sub(x[0],x[1],line)
		outf.write(line)
		line=inf.readline();
	outf.close();
	inf.close();

def removeFile(patt, dirname, names):
	for x in names:
		if re.compile(patt).search(x):
			if os.path.isdir(dirname+os.sep+x):
				shutil.rmtree(dirname+os.sep+x)
			else:
				os.unlink(dirname+os.sep+x)

def removeFiles(dir,patt):
	os.path.walk(dir,removeFile,patt)


def makeosxpackage(target,source,env):
	print 'building OS X package ...'
	removeFiles('osxpaknocvs','CVS')
	removeFiles('osxpaknocvs','sconsign')
	pak=os.getcwd()+'/osxpaknocvs'
	pakroot=pak+'/Package_Root'
	pakresources=pak+'/Resources'
	pakinfo=pak+'/yafray.info'
	pakdes=pak+'/yafraydes.plist'
	pakmaker='/Developer/Applications/Utilities/PackageMaker.app/Contents/MacOS/PackageMaker'
	pakresult=os.getcwd()+'/yafray-%s.pkg'%(globalinfo.version)
	pakresultstrip='yafray-%s.pkg'%(globalinfo.version)
	if os.path.isdir(pakresultstrip):
		shutil.rmtree(pakresultstrip)
	if os.path.isfile(pakresultstrip+'.zip'):
		os.unlink(pakresultstrip+'.zip')
	os.system(pakmaker+' -build -p '+pakresult+' -f '+pakroot+' -r '+pakresources+' -i '+pakinfo+' -d '+pakdes)
	os.system('zip -r '+pakresultstrip+'.zip '+pakresultstrip)
	#shutil.rmtree(pakresult)

if os.path.isdir('osxpak'):
	os.rename('osxpak','osxpaknocvs')
