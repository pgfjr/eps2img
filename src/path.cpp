/*
//  Copyright (c) 2020 Peter Frane Jr. All Rights Reserved.
//
//  Use of this source code is governed by the GPL v. 3.0 license that can be
//  found in the LICENSE file.
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
//  OF ANY KIND, either express or implied.
//
//
//  For inquiries, email the author at pfranejr AT hotmail.com
*/

#include "processor.h"

void processor::do_path_ops(operator_handler* handler)
{
	size_t param_count = handler->m_param_count;
	double v[6]{ 0.0 };
	double& x1 = v[0];
	double& y1 = v[1];
	double& x2 = v[2];
	double& y2 = v[3];
	double& x3 = v[4];
	double& y3 = v[5];

	if (param_count > 0)
	{
		for (size_t i = 0, j = param_count - 1; i < param_count; ++i, --j)
		{
			v[j] = m_operand_stack[i].m_number;
		}
	}
	switch (handler->m_op_id)
	{
	case op_id_newpath:
		cairo_new_path(m_cairo);
		m_current_point.clear();
		m_last_moveto.clear();
		m_has_current_point = false;
		break;
	case op_id_moveto:
		cairo_move_to(m_cairo, x1, y1);
		m_current_point.x = x1;
		m_current_point.y = y1;
		m_last_moveto = m_current_point;
		m_has_current_point = true;
		break;
	case op_id_lineto:
		if( has_current_point())
		{
			cairo_line_to(m_cairo, x1, y1);
			m_current_point.x = x1;
			m_current_point.y = y1;
		}
		else
		{
			message("No current point in --lineto--");
		}
		break;
	case op_id_curveto:
		if (has_current_point())
		{
			cairo_curve_to(m_cairo, x1, y1, x2, y2, x3, y3);
			m_current_point.x = x3;
			m_current_point.y = y3;
		}
		else
		{
			message("No current point in --curveto--");
		}
		break;
	case op_id_currentpoint:
		if (has_current_point())
		{
		
			push_number(m_current_point.x, ot_real);
			push_number(m_current_point.y, ot_real);
		}
		else
		{
			message("No current point in --currentpoint--");
		}
		break;
	case op_id_closepath:
		cairo_close_path(m_cairo);
		m_current_point = m_last_moveto;
		break;
	case op_id_rmoveto:
		if (has_current_point())
		{
			cairo_rel_move_to(m_cairo, x1, y1);
			m_current_point.x += x1;
			m_current_point.y += y1;
			m_last_moveto = m_current_point;
		}
		else
		{
			message("No current point in --rmoveto--");
		}
		break;
	case op_id_rlineto:
		if (has_current_point())
		{
			cairo_rel_line_to(m_cairo, x1, y1);
			m_current_point.x += x1;
			m_current_point.y += y1;
		}
		else
		{
			message("No current point in --rlineto--");
		}
		break;
	case op_id_rcurveto:
		if (has_current_point())
		{
		
			cairo_rel_curve_to(m_cairo, x1, y1, x2, y2, x3, y3);
			m_current_point.x += x3;
			m_current_point.y += y3;

		}
		else
		{
			message("No current point in --rcurveto--");
		}
		break;
	case op_id_stroke:
		cairo_stroke(m_cairo);
		m_current_point.clear();
		m_last_moveto.clear();
		m_has_current_point = false;
		break;
	case op_id_fill:
		cairo_fill(m_cairo);
		m_current_point.clear();
		m_last_moveto.clear();
		m_has_current_point = false;
		break;
	case op_id_eofill:
		cairo_set_fill_rule(m_cairo, CAIRO_FILL_RULE_EVEN_ODD);
		cairo_fill(m_cairo);
		cairo_set_fill_rule(m_cairo, CAIRO_FILL_RULE_WINDING);
		m_current_point.clear();
		m_last_moveto.clear();
		m_has_current_point = false;
		break;
	case op_id_arc:
		cairo_arc(m_cairo, v[0], v[1], v[2], __deg2rad(v[3]), __deg2rad(v[4]));
		break;
	case op_id_arcn:
		cairo_arc_negative(m_cairo, v[0], v[1], v[2], __deg2rad(v[3]), __deg2rad(v[4]));
		break;
	case op_id_rectfill:
		do_gsave(handler);
		cairo_rectangle(m_cairo, v[0], v[1], v[2], v[3]);
		cairo_fill(m_cairo);
		do_grestore(handler);
		break;
	case op_id_rectstroke:
		do_gsave(handler);
		cairo_rectangle(m_cairo, v[0], v[1], v[2], v[3]);
		cairo_stroke(m_cairo);
		do_grestore(handler);
		break;

	case op_id_clip:
		cairo_clip(m_cairo);
		break;
	case op_id_erasepage:
		cairo_save(m_cairo);
		cairo_set_source_rgb(m_cairo, 1.0, 1.0, 1.0);
		cairo_paint(m_cairo);
		cairo_restore(m_cairo);
		break;
	}
	if (param_count > 0)
	{
		pop(param_count);
	}
}

void processor::do_flattenpath(operator_handler* handler)
{
	if (has_current_point())
	{
		// if cairo is not in an error state
		if (cairo_status(m_cairo) != CAIRO_STATUS_NO_MEMORY)
		{
			cairo_path_t* path = cairo_copy_path_flat(m_cairo);

			if (cairo_status(m_cairo) == CAIRO_STATUS_SUCCESS)
			{
				cairo_new_path(m_cairo);

				cairo_append_path(m_cairo, path);

				cairo_path_destroy(path);

				return;
			}
			else if (cairo_status(m_cairo) == CAIRO_STATUS_NO_MEMORY)
			{
				message("Not enough memory to create a flattened path");
			}
			
			else
			{
				goto error_msg;
			}
		}
		else
		{
			goto error_msg;
		}
	}
	else
	{
		return;
	}
error_msg:
	message("Internal error in --%s--. The graphics backend is in an unknown error state", handler->m_name);
}

void processor::do_clippath(operator_handler* handler)
{
	if (cairo_has_current_point(m_cairo) == 0)
	{
		double width = m_width;// m_bounding_box.m_width;
		double height = m_height;// m_bounding_box.m_height;


		cairo_move_to(m_cairo, 0, 0);
		cairo_rel_line_to(m_cairo, width, 0);
		cairo_rel_line_to(m_cairo, 0, height);
		cairo_rel_line_to(m_cairo, -width, 0);
		cairo_close_path(m_cairo);
	}
	cairo_clip_preserve(m_cairo);
}
