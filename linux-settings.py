import shutil
import re
import os
import globalinfo

srcroot = os.getcwd()
prefix = ''

def init(args): 
	global prefix
	prefix = args.get('prefix','/usr/local')

def get_libpath(args): return prefix+"/lib"
def get_pluginpath(args): return prefix+"/lib/yafray"
def get_binpath(args): return prefix+"/bin"
def get_confpath(args): return prefix+"/etc"
def get_cxxflags(args):
	debug=args.get('debug',0)
	arch=args.get('arch','')
	tune=args.get('tune','')
	flags='-Wall -DHAVE_CONFIG_H -D_PTHREADS'
	if debug:
		flags+=' -O3 -ffast-math -ggdb'
	else:
		flags+=' -O3 -ffast-math -fomit-frame-pointer'
	if arch!='':
		flags+=' -march='+arch
	if tune!='':
		flags+=' -mtune='+tune
	return flags

class exr(globalinfo.library):
	PATH = '/usr'
	C_ID = 'EXR'
	def present(args):
		exr.PATH = args.get('exr_path','/usr')
		if exr.PATH != None and os.path.exists(exr.PATH + "/include/OpenEXR/half.h"):
			return True
		else:
			return False
		
	def get_include(args): return [ exr.PATH + "/include/OpenEXR" ]
	def get_libpath(args): return [ exr.PATH + "/lib" ]
	def get_libs(args): return ['IlmImf', 'Imath', 'Iex', 'Half']

class jpeg(globalinfo.library):
	C_ID = 'JPEG'

	def present(args): return os.path.exists("/usr/include/jpeglib.h")
	def get_include(args): return []
	def get_libpath(args): return []
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
	return common_env.Copy()

def derive_plugin_env(common_env):
	return common_env.Copy()

def derive_program_env(common_env):
	return common_env.Copy()

def package_rule(args):
	return (None, None)

