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

#pragma once
#include "data.h"

struct token
{
	char m_name[MAX_LINE_BUF + 1]{ 0 };
	operand_type m_type{ operand_type::ot_null };
	double m_number{ 0.0 };
	string m_string;
	token() : m_string()
	{
		m_string.reserve(512);
	}
	~token()
	{
	}
	void clear()
	{
		m_string.clear();
		m_type = operand_type::ot_null;
		m_name[0] = 0;
		m_number = 0.0;
	}
};

class scanner : public common_class
{
	token* m_token{ nullptr };
	FILE* m_file{ stdin };
	int m_column{ 0 };
	int m_row{ 0 };
	bool m_quit{ false };
	char m_buffer[MAX_LINE_BUF + 1]{ 0 };
	char* m_curpos{ nullptr };
	bool m_eps{ false };
	bool m_show_prompt{ true };
	void do_comment();
	bool is_delimiter(uint8_t ch);
	size_t get_name(char* buf, size_t buf_len);
	void read_name();
	void do_name();
	void do_literal();
	void read_number();
	void do_number();
	void try_number();
	void do_angular_on();
	void do_angular_off();
	void do_hex_string_on();
	void do_text_string_on();
	void do_tilde();
	void get_token_ex(token& tkn);	
	bool (scanner::*read_file_ptr)();
	bool read_file();
	bool read_stdin();
	bool find_bounding_box(double& width, double& height);
public:
	scanner() : common_class(), m_curpos(m_buffer), read_file_ptr(&scanner::read_stdin)
	{
	}
	~scanner()
	{
		clear();

		if (m_file && m_file != stdin)
		{
			fclose(m_file);
		}
	}
	void clear();
	bool get_token(token& tkn, bool& error);
	bool has_token(token& tkn);
	void get_error(int& column, int& row, string& msg);
	bool get(uint8_t& ch);
	void unget();
	uint8_t get();
	uint8_t peek() const;
	bool load_file(const char* input_file, double &width, double &height);
	bool is_interactive() const;
	bool is_eof() const;
	bool is_eps() const;
	void clear_input();
};