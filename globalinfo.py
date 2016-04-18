from types import *

package='yafray'
version='0.0.9'
config_file='config.h'

class staticMetaClass(type):
	def __new__(cls, classname, bases, cdict):
		for k,i in cdict.items():
			if type(i) is FunctionType : cdict[k] = staticmethod(i)
		return type.__new__(cls,classname,bases,cdict)

class library(object):
	__metaclass__ = staticMetaClass

