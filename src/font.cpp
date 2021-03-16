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

struct base_font_table
{
	const char* m_key;
	const char* m_value;
	slant_type m_slant_type;
	bool m_bold;
};

const char* font_times = "Times New Roman";
const char* font_arial = "Arial";
const char* font_courier = "Courier New";

const base_font_table font_table[] =
{
	{"Times-Roman", font_times, slant_type_normal, false},
	{"Times-Italic", font_times, slant_type_italic, false},
	{"Times-Bold", font_times, slant_type_normal, true},
	{"Times-BoldItalic", font_times, slant_type_italic, true},

	{"Helvetica", font_arial, slant_type_normal, false},
	{"Helvetica-Oblique", font_arial, slant_type_oblique, false},
	{"Helvetica-Bold", font_arial, slant_type_normal, true},
	{"Helvetica-BoldOblique", font_arial, slant_type_oblique, true},

	{"Courier", font_courier, slant_type_normal, false},
	{"Courier-Oblique", font_courier, slant_type_oblique, false},
	{"Courier-Bold", font_courier, slant_type_normal, true},
	{"Courier-BoldOblique", font_courier, slant_type_oblique, true},

	{"Symbol", "Symbol", slant_type_normal, false}
};

const base_font_table *find_font_facename(const char* name)
{
	size_t table_size = array_size(font_table);

	for (size_t i = 0; i < table_size; ++i)
	{
		if (strcmp(font_table[i].m_key, name) == 0)
		{
			return &font_table[i];
		}
	}

	// use times roman
	return &font_table[0];
}

void processor::do_findfont(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_string_type())
	{
		string_type* str = op.as_string();

		if (str)
		{
			const base_font_table* font_obj = find_font_facename(str->m_data.c_str());
			font_type* fnt = new font_type;

			if (!fnt)
			{
				message("Not enough memory to create a font object");
			}
			else
			{
				operand new_op(ot_font);

				fnt->m_name = font_obj->m_value;
				fnt->m_bold = font_obj->m_bold;
				fnt->m_slant_type = font_obj->m_slant_type;

				new_op.m_object = fnt;

				pop();

				push_operand(new_op);
			}
		}
		else
		{
			message("Missing font name in --%s--", handler->m_name);
		}
	}
	else
	{
		message("Type check in --%s--", handler->m_name);
	}
}

void processor::do_scalefont(operator_handler* handler)
{
	operand& size = m_operand_stack[0];
	
	if (!size.is_number())
	{
		goto error_msg;
	}
	else
	{
		double point_size = size.m_number;

		if (point_size < 0)
		{
			message("Range check in --%s--", handler->m_name);
		}
		else
		{
			operand& font = m_operand_stack[1];

			if (!font.is_font())
			{
				goto error_msg;
			}
			else
			{
				font_type* fnt = dynamic_cast<font_type*>(font.m_object);

				if (!fnt)
				{
					goto error_msg;
				}
				else
				{
					// change the size

					fnt->m_point_size = point_size;

					pop(); // pop the point size
				}
			}
			return;
		}
	}
error_msg:
	message("Type check in --%s--", handler->m_name);
}

void processor::do_setfont(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_font())
	{
		font_type* fnt = dynamic_cast<font_type*>(op.m_object);

		if (!fnt)
		{
			goto error_msg;
		}
		else
		{
			// change the size
			cairo_font_slant_t slant = (cairo_font_slant_t)fnt->m_slant_type;
			cairo_font_weight_t weight = fnt->m_bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL;

			cairo_select_font_face(m_cairo, fnt->m_name.c_str(), slant, weight);

			cairo_set_font_size(m_cairo, fnt->m_point_size );

			pop();

			return;
		}
	}
error_msg:
	message("Type check in --%s--", handler->m_name);
}

void processor::do_selectfont(operator_handler* handler)
{
	operand& size = m_operand_stack[0];
	operand& font_name = m_operand_stack[1];

	if (size.is_number() && font_name.is_string_type())
	{
		// make sure 'size' is correct to prevent 'do_scalefont' from throwing an exception
		if (size.m_number < 0)
		{
			message("Range check in --%s--", handler->m_name);
		}

		try
		{
			// exchange position
			do_exch();

			do_findfont(handler);

			//swap again

			do_exch();

			do_scalefont(handler);

			do_setfont(handler);
		}
		catch (...)
		{
			do_exch();

			throw;
		}
	}
	else
	{
		message("Type check in --selectfont--");
	}
}

void processor::do_show(operator_handler* handler)
{
	if (!has_current_point())
	{
		message("No current point in --%s--", handler->m_name);
	}
	else
	{
		operand& op = m_operand_stack[0];

		if (op.is_text_string())
		{
			string_type* str = op.as_string();

			if (str)
			{
				double x = 1.0, y = -1.0;
				cairo_matrix_t tmp;

				cairo_get_matrix(m_cairo, &tmp);

				cairo_scale(m_cairo, x, y);

				cairo_show_text(m_cairo, str->m_data.c_str());

				cairo_set_matrix(m_cairo, &tmp);
			}

			pop();
		}
	}
}

void processor::do_charpath(operator_handler* handler)
{
	if (!has_current_point())
	{
		message("No current point in --%s--", handler->m_name);
	}
	else
	{
		operand& op1 = m_operand_stack[0];
		operand& op2 = m_operand_stack[1];

		// op1 not used; no difference in output in GS
		if (op1.is_bool() && op2.is_text_string())
		{
			string_type* str = op2.as_string();

			if (str)
			{
				double x = 1.0, y = -1.0;
				cairo_matrix_t tmp;

				cairo_get_matrix(m_cairo, &tmp);

				cairo_scale(m_cairo, x, y);

				cairo_text_path(m_cairo, str->m_data.c_str());

				cairo_set_matrix(m_cairo, &tmp);
			}

			pop(handler->m_param_count);
		}
	}
}

void processor::do_string(operator_handler* handler)
{
	if (m_operand_stack[0].is_integer())
	{
		int32_t len = (int32_t)m_operand_stack[0].m_number;

		if (len < 0 || len > MAX_OBJECT_SIZE)
		{
			message("Range check in --string--");
		}
		else
		{
			try
			{
				string_type* str = new string_type((size_t)len);
				
				if (str)
				{
					operand op(ot_text_string);

					op.m_object = str;

					pop();

					push_operand(op);
				}
				else
				{
					message("Not enough memory to create a string object");
				}
			}
			catch (const exception &ex)
			{
				throw ex;
			}			
		}
	}
	else
	{
		message("Type check in --string--");
	}
}

void processor::do_stringwidth(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];

	if (op1.is_text_string())
	{
		string_type* str = op1.as_string();

		if (str)
		{
			cairo_text_extents_t extent = { 0 };
			
			cairo_text_extents(m_cairo, str->m_data.c_str(), &extent);

			pop();

			push_number(extent.x_advance, ot_real);
			push_number(extent.y_advance, ot_real);
		}
		else
		{
			pop();
		}
	}
	else
	{
		message("Type check in --stringwidth--");
	}
}