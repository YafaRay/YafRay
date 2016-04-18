/****************************************************************************
 *
 *                      scene.h: Scene manipulation and rendering api
 *      This is part of the yafray package
 *      Copyright (C) 2002 Alejandro Conty Estevez
 *
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2.1 of the License, or (at your option) any later version.
 *
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 *
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library; if not, write to the Free Software
 *      Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#ifndef __FORKEDSCENE_H
#define __FORKEDSCENE_H

#ifdef HAVE_CONFIG_H
#include<config.h>
#endif

#include "scene.h"

__BEGIN_YAFRAY
class forkedscene_t : public scene_t
{
 public:
    virtual void render(colorOutput_t &out);

 protected:

    typedef enum{OVERSAMPLE, REPEATFIRST, EXIT} state_t;

    // Storage for children
    int sndpipe, rcvpipe;
    int resx,resy;
    int childnum;

    void doChild(colorOutput_t &out);

};

__END_YAFRAY

#endif // __FORKEDSCENE_H
