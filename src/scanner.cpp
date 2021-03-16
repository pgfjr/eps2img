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

#include "scanner.h"

void scanner::clear()
{
	m_error.clear();
	m_quit = false;
}


void scanner::get_error(int& column, int& row, string& msg)
{
	column = m_column;
	row = m_row;
	msg = m_error;
}

bool scanner::read_file()
{
	return fgets(m_buffer, MAX_LINE_BUF, m_file) != nullptr;
}

bool scanner::read_stdin()
{
	if (m_show_prompt)
	{
		printf("> ");
	}
	return fgets(m_buffer, MAX_LINE_BUF, m_file) != nullptr;
}

bool scanner::get(uint8_t& ch)
{
	if (!m_quit)
	{
		if (!*m_curpos)
		{
			m_buffer[0] = 0;

			if (!(this->*read_file_ptr)())
			{
				ch = 0;

				if (feof(m_file))
				{
					m_quit = true;
				}
				return false;
			}
			m_curpos = m_buffer;
		}

		m_column = m_curpos - m_buffer;

		ch = *m_curpos;

		++m_curpos;

		return true;
	}

	ch = 0;

	return false;
}

void scanner::unget()
{
	if (m_curpos != &m_buffer[0])
	{
		--m_curpos;

		if ('\n' == *m_curpos)
		{
			--m_row;
		}
	}
}

uint8_t scanner::get()
{
	uint8_t ch;

	get(ch);

	return ch;
}

uint8_t scanner::peek() const
{
	return *m_curpos;
}

bool scanner::find_bounding_box(double& width, double& height)
{
	long curpos = ftell(m_file);
	bool result = false;

	while (!feof(m_file) && fgets(m_buffer, sizeof(m_buffer) - 1, m_file))
	{
		if (m_buffer[0] == '%' && m_buffer[1] == '%')
		{
			const char bbox[] = "BoundingBox: ";
			size_t len = sizeof(bbox) - 1;
			const char* p = &m_buffer[2];

			if (strncmp(p, bbox, len) == 0)
			{
				int32_t x1, y1, x2, y2;

				if (sscanf_s(p+len, "%d %d %d %d", &x1, &y1, &x2, &y2) == 4)
				{
					width = (double)(x2 - x1);
					height = (double)(y2 - y1);

					curpos = ftell(m_file);
					result = true;
					break;
				}
			}
		}
	}

	fseek(m_file, curpos, SEEK_SET);

	return result;
}

bool scanner::load_file(const char* input_file, double& width, double& height)
{
	m_file = fopen(input_file, "rb");

	if (!m_file)
	{
		m_error = "Unable to open file: ";

		m_error.append(input_file);

		return false;
	}

	if (read_file())
	{
		//char signature[] = { "%!PSAdobe" };
		char signature[] = { "%!PS" };

		if (strncmp(m_curpos, signature, sizeof(signature) - 1) == 0 )
		{
			m_eps = find_bounding_box(width, height);

			read_file_ptr = &scanner::read_file;

			return true;
		}
		else
		{
			m_error = "Input file is neither a PostScript nor an EPS file";

			return false;
		}
	}

	return false;
}

void scanner::clear_input()
{
	// skip the rest
	while (*m_curpos)
	{
		if ('\n' == *m_curpos)
			++m_row;

		++m_curpos;
	}
}

void scanner::do_comment()
{
	if ('%' == *m_curpos)
	{
		++m_curpos;
		m_token->m_string = m_curpos;
		m_token->m_type = ot_dsc;
	}
	else
	{
		m_token->m_type = ot_comment;
	}
	
	clear_input();
}

#define PS_DELIMITER " \t\n\r\f/{}[]()<>%"

bool scanner::is_delimiter(uint8_t ch)
{
	return strchr(PS_DELIMITER, ch) != nullptr;
}

size_t scanner::get_name(char* buf, size_t buf_len)
{
	size_t i = 0;
	uint8_t ch;

	while (get(ch) && i < buf_len)
	{
		if (is_delimiter(ch))
		{
			buf[i] = 0;
			unget();
			break;
		}
		else if (isgraph(ch))
		{
			buf[i] = (char)ch;
			++i;
		}
		else
		{
			message("Invalid char in name: %d", ch);
		}
	}

	buf[i] = 0;

	return i;
}


void scanner::read_name()
{
	char* name = m_token->m_name;
	size_t len;

	// read only this much
	len = get_name(name, MAX_NAME_LEN + 2);

	if (len > MAX_NAME_LEN)
	{
		message("Name too long: %d. Max is %d: %s...", len, MAX_NAME_LEN, name);
	}

		m_token->m_type = ot_name;
}

void scanner::do_name()
{
	unget();

	read_name();
}

void scanner::do_literal()
{
	char* name = m_token->m_name;
	bool literal = true;
	size_t len;
	uint8_t ch;

	ch = peek();

	if ('/' == ch)
	{
		literal = false; // a constant, not a literal

		get(); // skip it
	}
	else if (0 == ch)
	{
		message("Unexpected end of file");
	}

	// read only this much
	len = get_name(name, MAX_NAME_LEN + 2);

	if (len > MAX_NAME_LEN)
	{
		message("Name too long: %d. Max is %d: %s...", len, MAX_NAME_LEN, name);
	}
	else if (0 == len)
	{
		if (literal)
		{
			name[0] = '/';
		}
		else
		{
			message("Missing name after constant //");
		}
	}

	if (literal)
	{
		m_token->m_type = ot_literal;
	}
	else
	{
		m_token->m_type = ot_constant;
	}
}

static operand_type get_number_type(const char* start, const char* end)
{
	while (start < end)
	{
		if ('.' == *start || 'e' == *start || 'E' == *start)
		{
			return operand_type::ot_real;
		}
		++start;
	}

	return operand_type::ot_integer;
}

void scanner::read_number()
{
	double value;
	char* endp;

	endp = m_curpos;

	value = strtod(m_curpos, &endp);

	if (is_delimiter((uint8_t)*endp))
	{

		m_token->m_type = get_number_type(m_curpos, endp);
		m_token->m_number = value;

		m_curpos = endp;

		return;
	}
	// base#number
	else if ('#' == *endp && isdigit((uint8_t)*m_curpos))
	{
		// if 'value' is an integer
		if (get_number_type(m_curpos, endp) == ot_integer)
		{
			int32_t base = (int32_t)value;

			if (base >= 2 && base <= 36)
			{
				value = strtol(endp+1, &endp, base);

				if (is_delimiter((uint8_t)*endp))
				{
					m_token->m_type = ot_integer;
					m_token->m_number = value;

					m_curpos = endp;

					return;
				}
			}
		}
	}

	read_name();	
}

void scanner::do_number()
{
	unget();

	read_number();
}

void scanner::try_number()
{
	char* curpos = m_curpos - 1;

	if ('+' == *curpos || '-' == *curpos)
	{
		++curpos;
	}
	if ('.' == *curpos)
	{
		++curpos;
	}

	if (isdigit((uint8_t)*curpos))
	{
		do_number();
	}
	else
	{
		do_name();
	}
}


void scanner::do_angular_on()
{
	uint8_t ch = peek();

	if ('<' == ch)
	{
		get(); // skip

		m_token->m_type = ot_dictionary_marker_on;
	}
	else if ('~' == ch)
	{
		message("Base 85 ASCII is not yet supported");
	}
	else
	{
		try
		{
			m_show_prompt = false;
			do_hex_string_on();
			m_show_prompt = true;
		}
		catch (const exception& ex)
		{
			m_show_prompt = true;
			throw ex;
		}
	}
}

void scanner::do_angular_off()
{
	uint8_t ch = peek();

	if ('>' == ch)
	{
		get(); // skip

		m_token->m_type = ot_dictionary_marker_off;
	}
	else
	{
		// closing '>' must be read by "do_hex_string_on()"

		message("Missing '<'");
	}
}

void scanner::do_tilde()
{
	if (peek() == '>') // ~> must be read by do_ascci_text_b85 (not yet supported)
	{
		message("Missing '<~");
	}
	else
	{
		do_name();
	}
}

void scanner::do_hex_string_on()
{
	int counter = 0;
	string& str = m_token->m_string;
	uint8_t buf[3] = { 0 };
	uint8_t ch;

	str.clear();

	while (get(ch))
	{
		if (isxdigit(ch))
		{
			buf[counter] = ch;
			++counter;

			if (2 == counter)
			{
				uint8_t val = (uint8_t)strtol((char*)buf, nullptr, 16);

				counter = 0;

				str.push_back((char)val);
			}
		}
		else if ('>' == ch)
		{
			if (1 == counter)
			{
				uint8_t val;

				buf[1] = '0';

				val = (uint8_t)strtol((char*)buf, nullptr, 16);

				str.push_back((char)val);
			}

			m_token->m_type = ot_hex_string;

			return;
		}
		else
		{
			if (!isspace(ch))
			{
				message("Character %c is not a hex number", ch);
			}
		}
	}

	message("Unexpected end of file");
}

/*
* TODO
\n line feed (LF)
\r carriage return (CR)
\t horizontal tab
\b backspace
\f form feed
\\ backslash
\( left parenthesis
\) right parenthesis
\ddd character code ddd (octal)
*/
void scanner::do_text_string_on()
{
	string& str = m_token->m_string;
	int paren = 1;
	uint8_t ch;

	str.clear();

	m_show_prompt = false;

	while (get(ch))
	{
		if ('(' == ch)
		{
			++paren;
			str.push_back('(');
		}
		else if (')' == ch)
		{
			--paren;

			if (0 == paren)
			{
				m_token->m_type = ot_text_string;

				m_show_prompt = true;

				return;
			}
			else
			{
				str.push_back(')');
			}
		}
		else if( isgraph(ch))
		{
			if (ch != '\\')
			{
				str.push_back((char)ch);
			}
		}
		else
		{
			if (isspace(ch))
			{
				str.push_back(' '); 
			}
			else
			{
				m_show_prompt = true;

				message("Invalid char: %d (%c)", ch, ch);
			}
		}
	}

	m_show_prompt = true;

	message("Text string has no matching ')");
}

void scanner::get_token_ex(token& tkn)
{
	uint8_t ch;

	tkn.clear();

	m_token = &tkn;

	while (get(ch))
	{		
		if (isalpha(ch))
		{
			do_name();
		}
		else if (isdigit(ch))
		{
			do_number();
		}
		else if ('/' == ch)
		{
			do_literal();
		}
		else if (isgraph(ch))
		{
			switch (ch)
			{			
			case '(':
				do_text_string_on();
				break;
			case ')':
				message("Missing ')'");
				break;
			case '{':
				m_token->m_type = ot_procedure_marker_on;
				break;
			case '}':
				m_token->m_type = ot_procedure_marker_off;
				break;
			case '[':
				m_token->m_type = ot_array_marker_on;
				break;
			case ']':
				m_token->m_type = ot_array_marker_off;
				break;
			case '<':
				do_angular_on();
				break;
			case '>':
				do_angular_off();
				break;
			case '%':
				do_comment();
				if (ot_comment == m_token->m_type)
				{
					continue;
				}
				break;
			case '.':
			case '-':
			case '+':
				try_number();
				break;
			case '~':
				do_tilde();
				break;
			default:
				do_name();
				break;
			}			
		}
		else
		{
			if (isspace(ch))
			{
				if ('\n' == ch)
				{
					++m_row;
				}
				continue;
			}
			else
			{
				message("Invalid char: %d (%c)", ch, ch);				
			}
		}
		break;
	}
}

bool scanner::is_interactive() const
{
	return m_file == stdin;
}

bool scanner::is_eof() const
{
	return feof(m_file) != 0;
}

bool scanner::is_eps() const
{
	return m_eps;
}

bool scanner::get_token(token& tkn, bool& error)
{
	try
	{
		if (!m_quit)
		{
			get_token_ex(tkn);

			return true;
		}
	}
	catch (const exception& ex)
	{
		m_error = ex.what();

		error = true;
	}

	return false;
}

bool scanner::has_token(token& tkn)
{
	if (is_eof() || !*m_curpos)
	{
		return false;
	}
	else
	{
		while (isspace(*m_curpos))
		{
			++m_curpos;
		}
		if (!*m_curpos || '%' == *m_curpos)
		{
			return false;
		}
		else
		{
			bool error;

			return get_token(tkn, error);
		}
	}
	
}