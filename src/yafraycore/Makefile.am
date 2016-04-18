
CXX=g++
#CXXFLAGS=-pg -fprofile-arcs -O3 -Wall @GTKCFLAGS@ @PIXCFLAGS@
AM_CXXFLAGS=-O3 -fomit-frame-pointer -ffast-math -Wall -D_PTHREADS
#CXXFLAGS=-ggdb -O3 -Wall -D_PTHREADS @GTKCFLAGS@ @PIXCFLAGS@

lib_LTLIBRARIES= libyafraycore.la

libyafraycore_la_SOURCES= bound.cc bound.h\
buffer.cc buffer.h\
camera.cc camera.h\
color.cc color.h\
filter.cc filter.h\
light.h\
matrix4.cc matrix4.h\
mesh.cc mesh.h\
reference.cc reference.h\
output.h\
scene.cc scene.h\
threadedscene.cc threadedscene.h\
forkedscene.cc forkedscene.h\
ipc.cc ipc.h\
ccthreads.cc ccthreads.h\
noise.cc noise.h\
background.cc background.h\
sphere.cc sphere.h\
surface.h\
texture.cc texture.h\
targaIO.cc targaIO.h\
triangle.cc triangle.h\
triangletools.cc triangletools.h\
tools.cc tools.h\
vector3d.cc vector3d.h\
object3d.cc object3d.h\
photon.cc photon.h\
params.cc params.h\
yafsystem.cc yafsystem.h\
renderblock.cc renderblock.h

LIBTOOL_DEPS = @LIBTOOL_DEPS@

libyafraycore_la_LIBADD=@PIXLIBS@ @ZLIBS@ @PTLIBS@ @JPEGLIBS@
libyafraycore_la_LDFLAGS=-no-undefined -dynamic -avoid-version

