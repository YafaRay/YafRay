CXX=g++
AM_CXXFLAGS=-O3 -fomit-frame-pointer -ffast-math -Wall -D_PTHREADS -I../yafraycore
PLUGINFLAGS= -no-undefined -module -export-dynamic -avoid-version
PLUGINADD= ../yafraycore/libyafraycore.la

lib_LTLIBRARIES= libyafrayplugin.la

libyafrayplugin_la_SOURCES= interface_impl.cc interface_impl.h interface.h

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libyafrayplugin_la_LDFLAGS=$(PLUGINFLAGS)
libyafrayplugin_la_LIBADD=$(PLUGINADD)
