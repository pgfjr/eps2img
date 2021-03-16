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

#include "application.h"

bool application::run_loop(scanner& sc, double width, double height, bool is_interactive)
{
	bool result = true;
	processor proc(sc);
	token tkn;

	if (!proc.init_graphics(width, height))
	{
		m_error = "Unable to initialize the graphics output";

		return false;
	}

	while (true)
	{
		bool error = false;

		if (sc.get_token(tkn, error))
		{
			if (!proc.process_token(tkn))
			{
				if (proc.has_error())
				{					
					if (is_interactive)
					{
						cout << proc.error() << endl;

						proc.clear_error();

						sc.clear_input();

						continue;
					}
					else
					{
						m_error = proc.error();

						result = false;

						break;
					}					
				}		
				else if (proc.quit())
				{
					break;
				}
			}
		}
		else if (sc.is_eof()) // normal exit
		{
			break;
		}
		else if (sc.has_error())
		{
			int column, row;

			sc.get_error(column, row, m_error);

			cout << m_error << endl;

			if (is_interactive)
			{
				m_error.clear();

				continue;
			}
			result = false;
			break;
		}
	}

	//cout << "\nDumping...\n";

	//proc.dump_stack();

	proc.save_file(m_output_file.c_str());

	return result;
}

bool application::create_output_filename(const char* filename, const char* output_file)
{
	if (output_file)
	{
		const char* ext = strrchr(output_file, '.');

		if (ext && strcmpi(ext + 1, "pdf") == 0)
		{
			m_output_file = output_file;
		}
		else
		{
			m_error = "Unknown or unsupported output file type. Output file extension must be '.pdf'";

			return false;
		}
	}
	else
	{
		size_t pos;

		if (filename)
		{
			m_output_file = filename;

			pos = m_output_file.find_last_of(".");

			if (pos == string::npos)
			{
				m_output_file.append(".pdf");
			}
			else
			{
				m_output_file.replace(pos + 1, string::npos, "pdf");
			}
		}
		else
		{
			m_output_file = "./test.pdf";
		}
	}

	return true;
}

bool application::convert(const char* filename, const char *output_file)
{
	double width = DEFAULT_WIDTH;
	double height = DEFAULT_HEIGHT;
	scanner sc;
	
	if (!create_output_filename(filename, output_file))
	{
		return false;
	}

	if (!filename)
	{
		return run_loop(sc, width, height, true);
	}
	else
	{
		if (!sc.load_file(filename, width, height))
		{
			m_error = sc.error();

			return false;
		}		

		if (width <= 0)
		{
			width = DEFAULT_WIDTH;
		}
		if (height <= 0)
		{
			height = DEFAULT_HEIGHT;
		}

		return run_loop(sc, DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
	}
}

