CXX=g++
#CXXFLAGS=-pg -fprofile-arcs -O3 -Wall @GTKCFLAGS@ @PIXCFLAGS@
AM_CXXFLAGS=-O3 -fomit-frame-pointer -ffast-math -Wall -D_PTHREADS 
#CXXFLAGS=-ggdb -O3 -Wall -D_PTHREADS @GTKCFLAGS@ @PIXCFLAGS@
INCLUDES = -I../yafraycore

bin_PROGRAMS= yafray

yafray_SOURCES= gram_lex.cc gram_lex.h\
gram_loader.cc gram_loader.h\
parser.h lex.h\
mlex.cc mlex.h\
msin_general.cc msin.h\
msin_object.cc msin_shader.cc\
loader.cc\
msin_render.cc\
render.cc render.h


yafray_LDADD= @GTKLIBS@ @PIXLIBS@ @ZLIBS@ @PTLIBS@ @JPEGLIBS@ \
../yafraycore/libyafraycore.la -ldl

#LDFLAGS= @YAFLDFLAGS@

#yafray_LDFLAGS= -fprofile-arcs -pg

sysconf_DATA= ../gram.yafray

