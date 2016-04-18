libdir=@libdir@/yafray

CXX=g++
AM_CXXFLAGS=-O3 -fomit-frame-pointer -ffast-math -Wall -D_PTHREADS -I../yafraycore
#CXXFLAGS=-O3 -ggdb -Wall -D_PTHREADS -I../yafraycore
PLUGINFLAGS= -no-undefined -module -export-dynamic -avoid-version
PLUGINADD= ../yafraycore/libyafraycore.la

lib_LTLIBRARIES= libpathlight.la libsunlight.la libpointlight.la libphotonlight.la\
libhemilight.la libspotlight.la libsoftlight.la libarealight.la libglobalphotonlight.la \
libspherelight.la

libpathlight_la_SOURCES= pathlight.cc pathtools.cc pathlight.h pathtools.h
libsunlight_la_SOURCES= sunlight.cc sunlight.h
libpointlight_la_SOURCES= pointlight.cc pointlight.h
libphotonlight_la_SOURCES= photonlight.cc photonlight.h
libglobalphotonlight_la_SOURCES= globalphotonlight.cc globalphotonlight.h
libhemilight_la_SOURCES= hemilight.cc hemilight.h
libspotlight_la_SOURCES= spotlight.cc spotlight.h
libsoftlight_la_SOURCES= softlight.cc softlight.h
libarealight_la_SOURCES= arealight.cc arealight.h
libspherelight_la_SOURCES= spherelight.cc spherelight.h

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libpathlight_la_LDFLAGS=$(PLUGINFLAGS)
libpathlight_la_LIBADD=$(PLUGINADD)
libsunlight_la_LDFLAGS=$(PLUGINFLAGS)
libsunlight_la_LIBADD=$(PLUGINADD)
libpointlight_la_LDFLAGS=$(PLUGINFLAGS)
libpointlight_la_LIBADD=$(PLUGINADD)
libphotonlight_la_LDFLAGS=$(PLUGINFLAGS)
libphotonlight_la_LIBADD=$(PLUGINADD)
libglobalphotonlight_la_LDFLAGS=$(PLUGINFLAGS)
libglobalphotonlight_la_LIBADD=$(PLUGINADD)
libhemilight_la_LDFLAGS=$(PLUGINFLAGS)
libhemilight_la_LIBADD=$(PLUGINADD)
libspotlight_la_LDFLAGS=$(PLUGINFLAGS)
libspotlight_la_LIBADD=$(PLUGINADD)
libsoftlight_la_LDFLAGS=$(PLUGINFLAGS)
libsoftlight_la_LIBADD=$(PLUGINADD)
libarealight_la_LDFLAGS=$(PLUGINFLAGS)
libarealight_la_LIBADD=$(PLUGINADD)
libspherelight_la_LDFLAGS=$(PLUGINFLAGS)
libspherelight_la_LIBADD=$(PLUGINADD)

install-data-local:
	rm $(libdir)/*.la $(libdir)/*.a || true


