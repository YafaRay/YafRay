import shutil
import re
import os
import globalinfo

srcroot = os.getcwd()
prefix = ''

def init(args):
	global prefix
	prefix=args.get('prefix',os.getcwd() + os.sep + 'win32pak' )

def get_libpath(args): return prefix
def get_pluginpath(args): return prefix+ os.sep+"plugins"
def get_binpath(args): return prefix
def get_confpath(args): return prefix
def get_cxxflags(args):
	debug=args.get('debug',0)
	arch=args.get('arch','')
	flags = '/DWIN32 /D_WIN32 /D_USE_MATH_DEFINES /EHsc /DHAVE_CONFIG_H'
	if debug:
		flags+=' /Zi /GS /RTC1 /Yd /MTd'
	else:
#		flags+=' /Ox /MD /D_STATIC_CPPLIB'
		flags+=' /Ogitypb1 /Gs /MD /D_STATIC_CPPLIB'
	if arch!='':
		flags+=' /'+arch
	return flags

class exr(globalinfo.library):
	PATH = None
	C_ID = 'EXR'
	def present(args):
		if not zlib.present(args) : return False

		exr.PATH = args.get('exr_path',None)
		dyn = args.get('dynamic_exr',False)
		if exr.PATH is None and dyn : exr.PATH = srcroot + '\\..\\libs\\msvc\\openexr'
		if exr.PATH is None and not dyn : exr.PATH = srcroot + '\\..\\libs\\msvc\\openexr_static'
		if os.path.exists(exr.PATH + "\\include\\OpenEXR\\half.h"):
			return True
		else:
			return False
		
	def get_include(args): return zlib.get_include(args) + [ exr.PATH + "\\include\\OpenEXR" ]
	def get_libpath(args): return zlib.get_libpath(args) + [ exr.PATH + "\\lib" ]
	def get_libs(args): return zlib.get_libs(args) + ['IlmImf', 'Imath', 'Iex', 'Half']

class jpeg(globalinfo.library):
	PATH = srcroot + "\\..\\libs\\msvc\\jpeg"
	C_ID = 'JPEG'

	def present(args): return os.path.exists(jpeg.PATH + "\\include\\jpeglib.h")
	def get_include(args): return [jpeg.PATH + "\\include"]
	def get_libpath(args): return [jpeg.PATH + "\\lib"]
	def get_libs(args): return ['libjpeg']

class pthread(globalinfo.library):
	C_ID = 'PTHREAD'
	PATH = srcroot + "\\..\\libs\\msvc\\pthreads"

	def present(args): return os.path.exists(pthread.PATH + "\\include\\pthread.h")
	def get_include(args): return [pthread.PATH + "\\include"]
	def get_libpath(args): return [pthread.PATH + "\\lib"]
	def get_libs(args): return ['pthreadVC']

class zlib(globalinfo.library):
	C_ID = 'ZLIB'
	PATH = srcroot + "\\..\\libs\\msvc\\zlib"

	def present(args): return os.path.exists(zlib.PATH + "\\include\\zlib.h")
	def get_include(args): return [zlib.PATH + "\\include"]
	def get_libpath(args): return [zlib.PATH + "\\lib"]
	def get_libs(args): return ['zlib']

class dynload(globalinfo.library):
	C_ID = 'DL'

	def present(args): return False
	def get_include(args): return []
	def get_libpath(args): return []
	def get_libs(args): return []

def derive_shared_env(common_env):
	return common_env.Copy()

def derive_plugin_env(common_env):
	return common_env.Copy()

def derive_program_env(common_env):
	return common_env.Copy()

def package_rule(args):
	return ('win32installer',makewin32package)

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

def makewin32package(target,source,env):
	print 'building Win32 installer ...'
	pak=os.getcwd()+os.sep+'win32pak'
	template=pak+os.sep+'yafray_win_iss.tmpl'
	result=  pak+os.sep+'yafray_win.iss'
	if os.path.isfile(result):
		os.unlink(result)
	replaceInFile([ ['SRCROOT', pak] ], template, result)
