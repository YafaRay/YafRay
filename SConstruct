import os
import sys
import configio
import globalinfo

prefix=ARGUMENTS.get('prefix','/usr/local')

ficheros = {
'darwin' : 'darwin-settings',
'linux2' : 'linux-settings',
'win32' : 'win32-settings',
'sunos5' : 'sunos5-settings'
}

fichero = ficheros[sys.platform]

if fichero != None :
	confer = __import__(fichero)
else: exit("Arch not supported")

A = ARGUMENTS
confer.init(A)

class conflib:
	def __init__(self,C):
		self.c_id = C.C_ID
		self.present = C.present(A)
		if self.present :
			self.include = C.get_include(A)
			self.libpath = C.get_libpath(A)
			self.libs    = C.get_libs(A)
		else:
			self.include = []
			self.libpath = []
			self.libs    = []
			
class config:
	cxxflags   = confer.get_cxxflags(A)
	libpath    = confer.get_libpath(A)
	pluginpath = confer.get_pluginpath(A)
	binpath    = confer.get_binpath(A)
	confpath   = confer.get_confpath(A)
	rootpath   = Dir('.').srcnode().abspath

for name,val in confer.__dict__.items():
	if type(val) is globalinfo.staticMetaClass:
		config.__dict__[name] = conflib(val)

def print_config(C):
	for name,val in C.__dict__.items():
		if name[:2] != "__" :
			if val.__class__ is conflib:
				for name2,val2 in val.__dict__.items():
					print "%s.%s = "%(name,name2), val2
			else:
				print name, val

configio.write_conf(config)

common_env=Environment(ENV=os.environ, CXXFLAGS = config.cxxflags);
common_env.SConsignFile(config.rootpath + os.sep + 'scons-signatures')
shared_env=confer.derive_shared_env(common_env)
plugin_env=confer.derive_plugin_env(common_env)
program_env=confer.derive_program_env(common_env)

Export('config')
Export('shared_env')
Export('plugin_env')
Export('program_env')

SConscript(['src/SConscript'])

Alias('install',['install_core',
								 'install_lights',
								 'install_shaders',
								 'install_backgs',
								 'install_interface',
								 'install_loader'])

target, func = confer.package_rule(A)
if target != None:
	Depends(target,'install')
	Command(target,'install',func)
	Default('install')

