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

void processor::dump_stack()
{
	for (const auto& o : m_operand_stack)
	{
		cout << o << endl;
	}
}

void processor::read_dsc(const string& str)
{
	const char b[] = { "BoundingBox:" };
	size_t len = sizeof(b) - 1;

	if (str.compare(0, len, b) == 0)
	{
		const char* values = str.c_str() + len;
		int dim[4]{ 0 };

		//todo: check the range
		if (sscanf_s(values, "%d %d %d %d", &dim[0], &dim[1], &dim[2], &dim[3]) == 4)
		{
			int width = dim[2] - dim[0];
			int height = dim[3] - dim[1];

			if (width > 0 && height > 0)
			{
				m_bounding_box.m_col = dim[0];
				m_bounding_box.m_row = dim[1];
				m_bounding_box.m_width = width;
				m_bounding_box.m_height = height;
			}
		}
		else
		{
			while (isspace((uint8_t)*values))
				++values;

			// value is (atend)

			if (*values != '(')
			{
				message("Invalid BoundingBox values. Must be 4 integers or '(atend)'");
			}
		}
	}
}

bool processor::process_token(const token& tkn)
{
	try
	{
		switch (tkn.m_type)
		{
		case ot_array_marker_on:
		case ot_dictionary_marker_on:
			push_type(tkn.m_type);
			break;
		case ot_array_marker_off:
			if (in_procedure())
			{
				push_type(tkn.m_type);
			}
			else
			{
				create_array(ot_array);
			}
			break;
		case ot_constant:
		case ot_name:
			do_name(tkn.m_name, tkn.m_type);
			break;
		case ot_dictionary_marker_off:
			if (in_procedure())
			{
				push_type(tkn.m_type);
			}
			else
			{
				create_dictionary();
			}
			break;
		case ot_dsc:
			read_dsc(tkn.m_string);			
			break;
		case ot_integer:
		case ot_real:
			push_number(tkn.m_number, tkn.m_type);
			break;
		case ot_procedure_marker_on:
			push_type(tkn.m_type);
			++m_procedure_counter;
			break;
		case ot_procedure_marker_off:
			create_array(ot_procedure);
			--m_procedure_counter;
			break;
		//case ot_system_dictionary:
			//break;
		case ot_hex_string:
		case ot_text_string:
			push_string(tkn.m_string, tkn.m_type);
			break;
		case ot_literal:
			push_name(tkn.m_name, -1, tkn.m_type);
			break;
		}

		return !m_quit;
	}
	catch (const exception& ex)
	{
		// if the error was not thrown by this class
		if (!has_error())
		{
			m_error = ex.what();

			set_error(true);
		}

		return false;
	}
}

void processor::do_quit(operator_handler* handler)
{
	m_quit = true;
}

bool processor::init_graphics(double width, double height)
{
	double scale = 96.0 / 72.0;

	cairo_rectangle_t rc{ 0, 0, width*scale, height * scale };


	cairo_surface_t* surface = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &rc);

	if (!surface)
	{
		return false;
	}
	else
	{
		cairo_t* cr = cairo_create(surface);

		if (!cr || cairo_status(cr) == CAIRO_STATUS_NO_MEMORY)
		{
			cairo_surface_destroy(surface);
		
			return false;
		}
		else
		{

			m_surface = surface;
			m_cairo = cr;

			m_width = width;
			m_height = height;

			return true;
		}
	}
}

bool processor::save_file(const char* output_file)
{
	cairo_surface_t* pdf_surface;

	pdf_surface = cairo_pdf_surface_create(output_file, m_width, m_height);

	//pdf_surface = cairo_svg_surface_create(output_file, m_width, m_height);

	if (!pdf_surface)
	{
		cout << "Unable to create the file: " << output_file << '\n';

		return false;
	}
	else
	{
		const cairo_matrix_t mtx = { 1.0, 0, 0, -1.0, 0, m_height };
		cairo_t* cr = cairo_create(pdf_surface);


		cairo_set_matrix(cr, &mtx);

		cairo_set_source_surface(cr, m_surface, 0, 0);

		cairo_paint(cr);

		cairo_destroy(cr);

		cairo_surface_destroy(pdf_surface);

		return true;
	}
}