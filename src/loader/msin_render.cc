/****************************************************************************
 *
 *      msin_render.cc: rendering construction parsing implementation
 *      This is part of the yafray package
 *      Copyright (C) 2002  Alejandro Conty Estévez
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

#include "msin.h"

__BEGIN_YAFRAY
#define WARNING cerr<<"[Warning]: "

string_value_t camera_attrs[]=
{
	{ "name", A_NAME },
	{ "resx", A_RESX },
	{ "resy", A_RESY },
	{ "focal", A_FOCAL },
	{ "name", A_NAME },
	{ "from", A_FROM },
	{ "to", A_TO },
	{ "up", A_UP },
	{ "aperture", A_APERTURE },
	{ "use_qmc", A_USE_QMC },
	{ "type", A_CAMTYPE },
	{ "dof_distance", A_DOFDIST},
	{ "bokeh_type", A_BOKEH},
	{ "bokeh_bias", A_BKHBIAS},
	{ "bokeh_rotation", A_BKHROT},
	{ "aspect_ratio", A_ASPECT},
	{ NULL , 0 }
};

lval_t join_camera( vector<lval_t>::iterator v )
{
	lval_t res;
	check_ast(v[1].ast,AST_LATTRDATA);
	camera_data_t *nc=new camera_data_t;
	nc->resx=100;
	nc->resy=100;
	nc->dfocal=1.0;
	nc->laperture = 0.0;
	nc->dof_use_qmc = false;
	nc->camtype = "perspective";
	nc->dofdist = 0.0;
	nc->bkhtype = "disk1";
	nc->bkhbias = "uniform";
	nc->bkh_ro = 0.0;
	nc->aspect = 1.0;
	list<attr_data_t *> &la=((lattr_data_t *)v[1].ast)->l;
	for(list<attr_data_t *>::iterator i=la.begin();i!=la.end();++i)
	{
		attr_data_t &attr=**i;
		int attr_id=string_value(camera_attrs,attr.I.c_str());
		switch(attr_id)
		{
			case A_NAME:
				if(attr.f)
					WARNING<<"Only a string value accepted for camera name\n";
				else
					nc->name=attr.D;
				break;
			case A_RESX:
				if(!attr.f)
					WARNING<<"Only a number accepted for camera resolution\n";
				else
					nc->resx=(int)attr.F;
				break;
			case A_RESY:
				if(!attr.f)
					WARNING<<"Only a number accepted for camera resolution\n";
				else
					nc->resy=(int)attr.F;
				break;
			case A_FOCAL:
				if(!attr.f)
					WARNING<<"Only a number accepted for camera focal distance\n";
				else
					nc->dfocal=attr.F;
				break;
			case A_APERTURE:
				if(!attr.f)
					WARNING << "Only a number accepted for camera aperture\n";
				else
					nc->laperture = attr.F;
				break;
			case A_USE_QMC:
				if (attr.f)
					WARNING << "Only a string accepted for camera use_qmc\n";
				else
					nc->dof_use_qmc = (attr.D=="on");
				break;
			case A_CAMTYPE:
				if (attr.f)
					WARNING << "Only a string accepted for camera type\n";
				else
					nc->camtype = attr.D;
				break;
			case A_DOFDIST:
				if (!attr.f)
					WARNING << "Only a number accepted for camera dof_distance\n";
				else
					nc->dofdist = attr.F;
				break;
			case A_BOKEH:
				if (attr.f)
					WARNING << "Only a string accepted for camera bokeh_type\n";
				else
					nc->bkhtype = attr.D;
				break;
			case A_BKHBIAS:
				if (attr.f)
					WARNING << "Only a string accepted for camera bokeh_bias\n";
				else
					nc->bkhbias = attr.D;
				break;
			case A_BKHROT:
				if (!attr.f)
					WARNING << "Only a number accepted for camera bokeh_rotation\n";
				else
					nc->bkh_ro = attr.F;
				break;
			case A_ASPECT:
				if (!attr.f)
					WARNING << "Only a number accepted for camera aspect_ratio\n";
				else
					nc->aspect = attr.F;
				break;
			default:
				WARNING<<"Unknown camera attr "<<attr.I<<endl;
		}
	}
	delete v[1].ast;
	for(int i=0;i<3;++i)
	{
		check_ast(v[3+i].ast,AST_POINT);
		point_data_t *p=(point_data_t *)v[3+i].ast;
		int attr=string_value(camera_attrs,p->label.c_str());
		switch(attr)
		{
			case A_FROM : nc->from=p->P;break;
			case A_TO : nc->to=p->P;break;
			case A_UP : nc->up=p->P;break;
			default :
				WARNING<<"Unknown camera attr "<<p->label<<endl;
		}
		delete p;
	}
	if(nc->name=="") WARNING<<"Camera with no name\n";
	res.ast=nc;
	return res;
}

__END_YAFRAY
