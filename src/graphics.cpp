/*
//  Copyright (c) 2020 Peter Frane Jr. All Rights Reserved.
//
//  Use of this source code is governed by the GPL v. 3.0 license that can be
//  found in the LICENSE file.
//
//  This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
//  OF ANY KIND, either express or implied.
//
//  For inquiries, email the author at pfranejr AT hotmail.com
*/


#include "processor.h"

static double __clamp(double v)
{
	if (v < 0.0)
		return 0.0;
	else if (v > 1.0)
		return 1.0;
	return v;
}

void processor::do_showpage(operator_handler* handler)
{
	cairo_show_page(m_cairo);
}

void processor::do_setrgbcolor(operator_handler* handler)
{
	double b = __clamp(m_operand_stack[0].m_number);
	double g = __clamp(m_operand_stack[1].m_number);
	double r = __clamp(m_operand_stack[2].m_number);

	pop(3);

	cairo_set_source_rgb(m_cairo, r, g, b);

	m_color.m_type = device_rgb;
	m_color.r = r;
	m_color.g = g;
	m_color.b = b;
}

void processor::do_setcmykcolor(operator_handler* handler)
{
	double k = __clamp(m_operand_stack[0].m_number);
	double y = __clamp(m_operand_stack[1].m_number);
	double m = __clamp(m_operand_stack[2].m_number);
	double c = __clamp(m_operand_stack[3].m_number);
	

	double r, g, b;

	pop(4);

	
	r = 1.0 - min(1.0, c + k);
	g = 1.0 - min(1.0, m + k);
	b = 1.0 - min(1.0, y + k);

	cairo_set_source_rgb(m_cairo, r, g, b);

	m_color.m_type = device_cmyk;
	m_color.c = c;
	m_color.m = m;
	m_color.y = y;
	m_color.k = k;
}

void processor::do_setlinejoin(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (!op.is_integer())
	{
		message("Type check in --%s--", handler->m_name);
	}
	else
	{
		int32_t value = (int32_t)op.m_number;

		if (value < 0 || value > 2)
		{
			message("Range check in --%s--", handler->m_name);
		}
		cairo_set_line_join(m_cairo, (cairo_line_join_t)value);

		pop();
	}
}

void processor::do_setlinecap(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (!op.is_integer())
	{
		message("Type check in --%s--", handler->m_name);
	}
	else
	{
		int32_t value = (int32_t)op.m_number;

		if (value < 0 || value > 2)
		{
			message("Range check in --%s--", handler->m_name);
		}
		cairo_set_line_cap(m_cairo, (cairo_line_cap_t)value);

		pop();

	}
}

void processor::do_setdash(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];

	if (op1.is_number() && op2.is_array())
	{
		double offset = op1.m_number;
		array_type* arr = op2.as_array();

		if (arr)
		{
			const size_t array_size = arr->size();

			if (arr->is_numeric())
			{
				vector<double> v(array_size);
				size_t zero = 0;

				for (size_t i = 0; i < array_size; ++i)
				{
					operand tmp = arr->get(i);

					if (tmp.m_number < 0.0)
					{
						message("Range check in --setdash--");						
					}
					// count the zeros
					else if (0.0 == tmp.m_number)
					{
						++zero;
					}
					v[i] = tmp.m_number;
				}

				// array values cannot be all zeros
				if (array_size == zero)
				{
					message("Range check in --setdash--");
				}

				pop(2);

				cairo_set_dash(m_cairo, v.data(), (int)array_size, offset);

				return;
			}
			else if (array_size == 0)
			{
				pop(2);

				cairo_set_dash(m_cairo, nullptr, 0, offset);

				return;
			}
		}		
	}

	message("Type check in --%s--", handler->m_name);
}

void processor::do_gsave(operator_handler* handler)
{
	gstate* gs = new gstate;

	if (!gs)
	{
		message("Memory allocation error in --%s--", handler->m_name);
	}
	try
	{

		gs->m_ctm = m_ctm;
		gs->m_color = m_color;

		if (has_current_point())
		{
			cairo_path_t* p = cairo_copy_path(m_cairo);

			if (cairo_status(m_cairo) == CAIRO_STATUS_SUCCESS)
			{
				gs->m_has_current_point = true;

				gs->m_path = p;

				gs->m_curpoint = m_current_point;
			}
			else if (cairo_status(m_cairo) == CAIRO_STATUS_NO_MEMORY)
			{
				message("Memory allocation error in --%s--", handler->m_name);
			}
			else
			{
				message("Graphics backend returned an unknown error");
			}
		}
		
		m_path_list.push_back(gs);
		
		cairo_save(m_cairo);
	}
	catch (const exception& ex)
	{
		if (gs)
		{
			delete gs;
		}

		throw ex;
	}	
}

void processor::do_grestore(operator_handler* handler)
{
	// empty if there was no prior 'gsave' call
	if (!m_path_list.empty())
	{
		gstate *gs = m_path_list.back();

		m_ctm = gs->m_ctm;

		m_color = gs->m_color;

		m_current_point = gs->m_curpoint;

		m_has_current_point = gs->m_has_current_point;

		cairo_restore(m_cairo);

		cairo_new_path(m_cairo);

		if (m_has_current_point)
		{
			// todo
			cairo_move_to(m_cairo, m_current_point.x, m_current_point.y);
		}

		if (gs->m_path)
		{
			cairo_append_path(m_cairo, gs->m_path);
			
		}
		
		
		m_path_list.pop_back();

		delete gs;		
	}	
	else
	{
		cairo_restore(m_cairo);
	}
}

void processor::do_currentrgbcolor(operator_handler* handler)
{
	operand op(ot_real);
	double r{ 0 }, g{ 0 }, b{ 0 };

	if ((m_color.m_type == device_rgb) || (m_color.m_type == device_gray))
	{
		r = m_color.r;		
		g = m_color.g;
		b = m_color.b;		
	}
	else
	{
		r = 1.0 - min(1.0, m_color.c + m_color.k);
		g = 1.0 - min(1.0, m_color.m + m_color.k);
		b = 1.0 - min(1.0, m_color.y + m_color.k);
	}

	op.m_number = r;
	push_operand(op);

	op.m_number = g;
	push_operand(op);

	op.m_number = b;
	push_operand(op);
}

void processor::do_currentcmykcolor(operator_handler* handler)
{
	operand op(ot_real);
	double c{ 0 }, m{ 0 }, y{ 0 }, k{ 0 };

	if ((m_color.m_type == device_cmyk))
	{
		c = m_color.c;		

		m = m_color.m;		

		y = m_color.y;		

		k = m_color.k;
		
	}
	else if(m_color.m_type == device_rgb)
	{
		c = (1.0 - m_color.r);

		m = (1.0 - m_color.g);
		
		y = (1.0 - m_color.b);
	}
	else
	{
		k = (1.0 - m_color.r);
	}

	op.m_number = c;
	push_operand(op);

	op.m_number = m;
	push_operand(op);

	op.m_number = y;
	push_operand(op);

	op.m_number = k;
	push_operand(op);
}

void processor::do_currentgray(operator_handler* handler)
{
	operand op(ot_real);

	if (m_color.m_type == device_gray)
	{
		op.m_number = m_color.r;		
	}
	else if (m_color.m_type == device_rgb)
	{
		op.m_number = ((0.3 * m_color.r) + (0.59 * m_color.g) + (0.11 * m_color.b));
	}
	else 
	{
		op.m_number = 1.0 - min(1.0, (0.3 * m_color.c) + (0.59 * m_color.m) + (0.11 * m_color.y) + m_color.k);
	}
	push_operand(op);
}

void processor::do_get_graphics_state(operator_handler* handler)
{
	operand op(ot_real);

	switch (handler->m_op_id)
	{
	case op_id_currentflat:
		op.m_number = cairo_get_tolerance(m_cairo);
		push_operand(op);
	break;
	case op_id_currentlinewidth:
		op.m_number = cairo_get_line_width(m_cairo);
		push_operand(op);
		break;
	case op_id_currentlinecap:
		op.m_number = (double) (int32_t)cairo_get_line_cap(m_cairo);
		op.m_type = ot_integer;
		push_operand(op);
		break;
	case op_id_currentlinejoin:
		op.m_number = (double)(int32_t)cairo_get_line_join(m_cairo);
		op.m_type = ot_integer;
		push_operand(op);
		break;
	case op_id_currentmiterlimit:
		op.m_number = cairo_get_miter_limit(m_cairo);		
		push_operand(op);
		break;
	}
}

void processor::do_set_graphics_state(operator_handler* handler)
{
	double value{ 0 };

	if (handler->m_numeric_param)
	{
		value = m_operand_stack[0].m_number;
	}

	switch (handler->m_op_id)
	{
	case op_id_setmiterlimit:
		if (value >= 1.0)
		{
			cairo_set_miter_limit(m_cairo, value);
			pop();
		}
		else
		{
			message("Range check in --%s--", handler->m_name);
		}
		break;
	case op_id_setlinewidth:
		if (value >= 0.0)
		{
			
			cairo_set_line_width(m_cairo, value);

		}
		pop();
		break;
	case op_id_setflat:
		if (value > 0.0)
		{
			cairo_set_tolerance(m_cairo, value);
		}
		else
		{
			cairo_set_tolerance(m_cairo, 0.1);
		}
		pop();
		break;
	case op_id_setgray:
		value = __clamp(value);
		cairo_set_source_rgb(m_cairo, value, value, value);
		m_color.m_type = device_gray;
		m_color.r = m_color.g = m_color.b = value;
		pop();
		break;
	}
}

size_t processor::get_transform_params(double& x, double& y, double* values, bool pop_params)
{
	operand& op1 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];
	cairo_matrix_t tmp{ 0 };

	if (op1.is_number() && op2.is_number())
	{
		y = op1.m_number;
		x = op2.m_number;

		if (pop_params)
		{
			pop(2);
		}
		return 2;
	}
	else if (op1.is_matrix(values))
	{
		if (stack_size() >= 3)
		{
			operand& op3 = m_operand_stack[2];

			if (op2.is_number() && op3.is_number())
			{
				y = op2.m_number;
				x = op3.m_number;

				if (pop_params)
				{
					pop(3);
				}
				return 3;
			}
		}
	}

	return 0;
}


void processor::do_matrix_transform(operator_handler* handler)
{
	cairo_matrix_t mtx = m_ctm;
	double x{ 0 }, y{ 0 };
	size_t count;

	count = get_transform_params(x, y, (double*)&mtx, true);

	if (0 == count)
	{
		const char* name = handler->m_name;

		message("Type check in --%s--. Valid format: x y %s or x y matrix %s", name, name, name);
	}
	else	
	{
		switch (handler->m_op_id)
		{
		case op_id_idtransform:
			cairo_matrix_invert(&mtx);//fall through
		case op_id_dtransform:
			cairo_matrix_transform_distance(&mtx, &x, &y);
			break;
		case op_id_itransform:
			cairo_matrix_invert(&mtx);//fall through
		case op_id_transform:
			cairo_matrix_transform_point(&mtx, &x, &y);
			break;
		}

		push_number(x, ot_real);

		push_number(y, ot_real);
	}
}


void processor::do_setmatrix(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_matrix((double *)&m_ctm))
	{
		cairo_set_matrix(m_cairo, &m_ctm);

		pop();
	}
	else
	{
		message("Type check in --%s--", handler->m_name);
	}	
}

void processor::do_concat(operator_handler* handler)
{
	operand& op = m_operand_stack[0];
	cairo_matrix_t mtx;

	if (op.is_matrix((double*)&mtx))
	{
		cairo_matrix_t tmp;

		cairo_get_matrix(m_cairo, &tmp);

		cairo_matrix_multiply(&tmp, &mtx, &tmp);
		
		cairo_set_matrix(m_cairo, &tmp);

		pop();
	}
	else
	{
		message("Type check in --%s--", handler->m_name);
	}
}

void processor::do_replace_matrix(operator_handler* handler)
{
	operand &op = m_operand_stack[0];

	if (!op.is_matrix())
	{
		message("Type check in --%s--", handler->m_name);
	}
	else
	{
		array_type* arr = op.as_array();

		if (!arr)
		{
			message("Type check in --%s--", handler->m_name);
		}
		else
		{
			// overwrite the matrix on top of the stack			
			const size_t matrix_size = 6;
			double values[6] = { 1.0, 0, 0, 1.0, 0, 0 };			

			switch (handler->m_op_id)
			{
			case op_id_currentmatrix:
				memcpy_s(values, sizeof(values), &m_ctm, sizeof(m_ctm));
				break;
			case op_id_defaultmatrix:
				values[0] = values[3] = m_scale;
				break;
			case op_id_identmatrix:
			default:
				break;
			}
			for (size_t i = 0; i < matrix_size; ++i)
			{
				arr->m_data[i].m_number = values[i];
				arr->m_data[i].m_type = ot_real;
			}
		}
	}
}

void processor::do_initmatrix(operator_handler* handler)
{
	cairo_matrix_t mtx1 = { m_scale, 0, 0, m_scale, 0, 0 };

	m_ctm = mtx1;

	cairo_set_matrix(m_cairo, &mtx1);
}

void processor::do_matrix(operator_handler* handler)
{
	const size_t matrix_size = 6;
	double mtx[6] = { 1.0, 0, 0, 1.0, 0, 0 };

	array_type* arr = new array_type(matrix_size, ot_array, at_local);

	if (!arr)
	{
		message("Out of memory in --%s--", handler->m_name);
	}
	else
	{
		operand new_op(ot_array);

		for (size_t i = 0; i < matrix_size; ++i)
		{
			operand op(mtx[i], true);
			arr->put(i, op);
		}

		new_op.m_object = arr;

		push_operand(new_op);
	}
}

void processor::do_rotate(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	double angle;

	if (op1.is_number())
	{
		angle = __deg2rad(op1.m_number);

		cairo_rotate(m_cairo, angle);

		cairo_matrix_rotate(&m_ctm, angle);

		pop();
	}
	else if (op1.is_matrix())
	{
		if (m_operand_stack.size() >= 2)
		{
			operand& op2 = m_operand_stack[1];

			if (op2.is_number())
			{
				const size_t matrix_size = 6;
				array_type* arr = op1.as_array();
				double values[matrix_size];

				angle = __deg2rad(op2.m_number);

				arr->get_numbers(values, matrix_size);

				cairo_matrix_rotate((cairo_matrix_t *)values, angle);

				// overwrite array values
				for (size_t i = 0; i < matrix_size; ++i)
				{
					arr->m_data[i].m_number = values[i];
					arr->m_data[i].m_type = ot_real;
				}

				// exchange angle and matrix
				do_exch();

				pop(); // angle
			}
			else
			{
				message("Type check in --%s--", handler->m_name);
			}
		}
		else
		{
			message("Stack underflow in --%s--", handler->m_name);
		}
	}
	else
	{
		message("Type check in --%s--", handler->m_name);
	}
}

void processor::do_scale(operator_handler* handler)
{
	const size_t matrix_size = 6;
	double values[matrix_size];
	double x{ 0 }, y{ 0 };
	size_t count;

	count = get_transform_params(x, y, values, false);

	if (0 == count)
	{
		const char* name = handler->m_name;

		message("Type check in --%s--. Valid format: x y %s or x y matrix %s", name, name, name);
	}
	else if (2 == count)
	{
		cairo_scale(m_cairo, x, y);

		cairo_matrix_scale(&m_ctm, x, y);
		
		pop(2);
	}
	else if( 3 == count )
	{
		operand& op1 = m_operand_stack[0];
		array_type* arr = op1.as_array();

		cairo_matrix_scale((cairo_matrix_t*)values, x, y);

		// overwrite array values
		for (size_t i = 0; i < matrix_size; ++i)
		{
			arr->m_data[i].m_number = values[i];
			arr->m_data[i].m_type = ot_real;
		}

		//swap positions of matrix and x and y

		do_exch();

		pop(); // pop y

		do_exch();

		pop(); // pop x
	}
}

void processor::do_translate(operator_handler* handler)
{
	const size_t matrix_size = 6;
	double values[matrix_size];
	double x{ 0 }, y{ 0 };
	size_t count;

	count = get_transform_params(x, y, values, false);

	if (0 == count)
	{
		const char* name = handler->m_name;

		message("Type check in --%s--. Valid format: x y %s or x y matrix %s", name, name, name);
	}
	else if (2 == count)
	{
		cairo_translate(m_cairo, x, y);

		cairo_matrix_translate(&m_ctm, x, y);

		pop(2);
	}
	else if (3 == count)
	{
		operand& op1 = m_operand_stack[0];
		array_type* arr = op1.as_array();

		cairo_matrix_translate((cairo_matrix_t*)values, x, y);

		// overwrite array values
		for (size_t i = 0; i < matrix_size; ++i)
		{
			arr->m_data[i].m_number = values[i];
			arr->m_data[i].m_type = ot_real;
		}

		//swap positions of matrix and x and y

		do_exch();

		pop(); // pop y

		do_exch();

		pop(); // pop x
	}
}

void processor::do_invertmatrix(operator_handler* handler)
{
	const size_t matrix_size = 6;
	operand& op2 = m_operand_stack[0];
	operand& op1 = m_operand_stack[1];
	double mtx1[matrix_size]{ 0 };

	if (op1.is_matrix(mtx1) && op2.is_matrix())
	{
		array_type* arr = op2.as_array();

		cairo_matrix_invert((cairo_matrix_t *)&mtx1);

		
		// overwrite array values
		for (size_t i = 0; i < matrix_size; ++i)
		{
			arr->m_data[i].m_number = mtx1[i];
			arr->m_data[i].m_type = ot_real;
		}

		do_exch();

		pop();
	}
	else
	{
		message("Type check in --%s--. Valid format: matrix matrix %s", handler->m_name);
	}
}

void processor::do_concatmatrix(operator_handler* handler)
{
	const size_t matrix_size = 6;
	operand& op3 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];
	operand& op1 = m_operand_stack[2];
	double mtx1[matrix_size]{ 0 };
	double mtx2[matrix_size]{ 0 };
	double mtx3[matrix_size]{ 0 };

	if (op1.is_matrix(mtx1) && op2.is_matrix(mtx2) && op3.is_matrix())
	{
		array_type* arr = op3.as_array();

		cairo_matrix_multiply((cairo_matrix_t*)&mtx3, (cairo_matrix_t*)&mtx1, (cairo_matrix_t*)&mtx2);

		// overwrite array values
		for (size_t i = 0; i < matrix_size; ++i)
		{
			arr->m_data[i].m_number = mtx3[i];
			arr->m_data[i].m_type = ot_real;
		}

		do_exch();

		pop();

		do_exch();

		pop();
	}
	else
	{
		message("Type check in --%s--. Valid format: matrix matrix matrix %s", handler->m_name);
	}
}