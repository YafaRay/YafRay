libdir=@libdir@/yafray

CXX=g++
AM_CXXFLAGS=-O3 -fomit-frame-pointer -ffast-math -Wall -D_PTHREADS -I../yafraycore
PLUGINFLAGS= -no-undefined -module -export-dynamic -avoid-version
PLUGINADD= ../yafraycore/libyafraycore.la

lib_LTLIBRARIES= libbasicshaders.la libbasicblocks.la libmixblocks.la libblendershader.la

libbasicshaders_la_SOURCES= basicshaders.cc basicshaders.h
libbasicblocks_la_SOURCES= basictex.cc basictex.h \
basicblocks.cc basicblocks.h jpeg.cc bbreg.cc\
imagebackg.cc imagebackg.h
libmixblocks_la_SOURCES= mixblocks.cc mixblocks.h
libblendershader_la_SOURCES=blendershader.cc blendershader.h

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libbasicshaders_la_LDFLAGS=$(PLUGINFLAGS)
libbasicshaders_la_LIBADD=$(PLUGINADD)
libbasicblocks_la_LDFLAGS=$(PLUGINFLAGS)
libbasicblocks_la_LIBADD=$(PLUGINADD) @JPEGLIBS@
libmixblocks_la_LDFLAGS=$(PLUGINFLAGS)
libmixblocks_la_LIBADD=$(PLUGINADD)
libblendershader_la_LDFLAGS=$(PLUGINFLAGS)
libblendershader_la_LIBADD=$(PLUGINADD)

install-data-local:
	rm $(libdir)/*.la $(libdir)/*.a || true


