libdir=@libdir@/yafray

CXX=g++
AM_CXXFLAGS=-O3 -fomit-frame-pointer -ffast-math -Wall -D_PTHREADS -I../yafraycore
PLUGINFLAGS= -no-undefined -module -export-dynamic -avoid-version
PLUGINADD= ../yafraycore/libyafraycore.la

lib_LTLIBRARIES= libhdri.la libsunsky.la libshaderback.la

libhdri_la_SOURCES= hdri.cc hdri.h \
HDRimage.cc HDRimage.h

libsunsky_la_SOURCES= sunsky.cc sunsky.h
libshaderback_la_SOURCES= shaderback.cc shaderback.h

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libhdri_la_LDFLAGS=$(PLUGINFLAGS)
libhdri_la_LIBADD=$(PLUGINADD)
libsunsky_la_LDFLAGS=$(PLUGINFLAGS)
libsunsky_la_LIBADD=$(PLUGINADD)
libshaderback_la_LDFLAGS=$(PLUGINFLAGS)
libshaderback_la_LIBADD=$(PLUGINADD)


install-data-local:
	rm $(libdir)/*.la $(libdir)/*.a || true


