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
#include "scanner.h"
#include <deque>
#include <vector>
#include <cairo.h>
#include <cairo-pdf.h>
#include <cairo-svg.h>

using namespace std;

#ifdef _WIN32
#pragma comment( lib, "cairo.lib")

#endif

#define array_size(x) sizeof((x))/sizeof((x)[0])

//struct operator_handler;


struct rectangle
{
	double m_col{ 0.0 };
	double m_row{ 0.0 };
	double m_width{ DEFAULT_WIDTH };
	double m_height{ DEFAULT_HEIGHT };
};

struct point
{
	double x{ 0 };
	double y{ 0 };
	void clear()
	{
		x = y = 0;
	}
};

enum color_type
{
	device_gray,
	device_rgb,
	device_cmyk
};

struct color
{
	union
	{
		struct
		{
			double r, g, b, a;
		};
		struct
		{
			double c, m, y, k;
		};
		double m_values[4]{ 0 };
	};
	double m_opacity{ 1.0 };
	color_type m_type{ device_gray };
	color() = default;
	~color() = default;
};

struct gstate
{
	cairo_matrix_t m_ctm{ 0.0 };
	cairo_path_t *m_path{ nullptr };
	bool m_has_current_point{ false };
	point m_curpoint;
	color m_color;
	gstate() = default;
	~gstate()
	{
		if (m_path)
		{
			cairo_path_destroy(m_path);
		}
	}
};


#define MAX_OPERAND_STACK_SIZE 500

class processor : public common_class
{
	deque<operand> m_operand_stack;
	dictionary_container *m_dictionary;
	scanner& m_scanner;
	int m_procedure_counter{ 0 };
	cairo_t* m_cairo{ nullptr };
	cairo_surface_t* m_surface{ nullptr };
	rectangle m_bounding_box;
	uint32_t m_rand{ 1 };
	vector<gstate *> m_path_list;
	cairo_matrix_t m_ctm{0};
	bool m_has_current_point{ false };
	double m_scale{ 96.0/ 72.0 };
	double m_width{ DEFAULT_WIDTH };
	double m_height{ DEFAULT_HEIGHT };
	bool m_quit{ false };
	int32_t m_loop_count{ 0 };
	alloc_type m_alloc_type{ at_local };
	point m_current_point, m_last_moveto;
	color m_color;

	bool in_procedure()const
	{
		return m_procedure_counter > 0;
	}
	void temp_procedure(bool setting)
	{
		if (setting)
			++m_procedure_counter;
		else if (m_procedure_counter > 0)
			--m_procedure_counter;
	}
	void clear()
	{
		m_operand_stack.clear();
		m_dictionary->clear();
		for (auto* p : m_path_list)
		{
			delete p;
		}
		m_path_list.clear();
	}
	size_t stack_size() const
	{
		return m_operand_stack.size();
	}
	bool has_current_point() const
	{
		return m_has_current_point;
	}
	void push_number(double number, operand_type type);
	void push_operand(operand& op);
	void push_type(operand_type type);
	void push_name(const char *name, int32_t len, operand_type type);
	void push_string(const string &str, operand_type type);
	void do_name(const char* name, operand_type type);
	void create_array(operand_type type);
	void create_dictionary();
	int32_t counttomark();
	void pop();
	void pop(size_t count);
	operator_handler *search_operator_dictionary(const char* name);
	bool search_system_dictionary(const char* name, operand &value);
	void execute_operator(operator_handler* handler);
	void execute_procedure(operand &op);
	void read_dsc(const string& str);
	void do_dictionary_begin(operand& op);
	void do_dictionary_end();
	void do_load(operand& key);
	void do_dict(operand& op);
	
	size_t get_transform_params(double& x, double& y, double* values, bool pop_params);
public:
	processor(scanner& _scanner) : common_class(), m_scanner(_scanner),	
		m_operand_stack(), m_dictionary(), m_path_list()
	{
		cairo_matrix_t tmp = { m_scale, 0, 0, m_scale, 0, 0 };

		m_ctm = tmp;

		m_dictionary = new dictionary_container;

		if (!m_dictionary)
		{
			message("Not enough memory to create the dictionary stack");
		}
		srand((unsigned int)time(NULL));
	}
	~processor()
	{
		clear();
				
		if (m_dictionary)
		{
			delete m_dictionary;
		}
		{
			if (m_cairo)
			{
				cairo_destroy(m_cairo);
			}
			if (m_surface)
			{
				cairo_surface_destroy(m_surface);
			}
		}
	}	
	void clear_error()
	{
		common_class::clear();
		m_quit = false;
		m_error.clear();
	}
	bool quit() const
	{
		return m_quit == true;
	}
	bool init_graphics(double width, double height);
	bool save_file(const char* output_file);
	int32_t operator_dictionary_size();
	bool process_token(const token& tkn);
	void dump_stack();
	bool find_key(const char* name, operand& value);
	bool find_key(const operand& key, operand& value);
	void do_dictionary_ops(operator_handler* handler);
	void do_def(operator_handler* handler);
	void do_bind(operand &op);
	void do_pop(operator_handler* handler);
	void do_pstack(operator_handler* handler);
	void do_print_n_pop(operator_handler* handler);
	void do_copy(operator_handler* handler);
	void do_exch();
	void do_stack_ops(operator_handler* handler);
	void do_math_binary_ops(operator_handler* handler);
	void do_math_unary_ops(operator_handler* handler);
	void do_math_misc_ops(operator_handler* handler);
	void do_path_ops(operator_handler* handler);
	void do_clippath(operator_handler* handler);
	void do_roll(operator_handler* handler);
	void do_eq(operator_handler* handler);
	void do_ne(operator_handler* handler);
	void do_lt(operator_handler* handler);
	void do_le(operator_handler* handler);
	void do_gt(operator_handler* handler);
	void do_ge(operator_handler* handler);
	void do_true(operator_handler* handler);
	void do_false(operator_handler* handler);
	void do_if(operator_handler* handler);
	void do_ifelse(operator_handler* handler);
	void do_where(operator_handler* handler);
	void do_get(operator_handler* handler);
	void do_setrgbcolor(operator_handler* handler);
	void do_setlinejoin(operator_handler* handler);
	void do_setlinecap(operator_handler* handler);
	void do_gsave(operator_handler* handler);
	void do_grestore(operator_handler* handler);
	void do_set_graphics_state(operator_handler* handler);
	void do_get_graphics_state(operator_handler* handler);
	void do_matrix(operator_handler* handler);
	void do_rotate(operator_handler* handler);
	void do_scale(operator_handler* handler);
	void do_translate(operator_handler* handler);
	void do_invertmatrix(operator_handler* handler);
	void do_concatmatrix(operator_handler* handler);
	void do_setdash(operator_handler* handler);
	void do_setcmykcolor(operator_handler* handler);
	void do_currentrgbcolor(operator_handler* handler);
	void do_currentcmykcolor(operator_handler* handler);
	void do_currentgray(operator_handler* handler);
	
	void do_matrix_transform(operator_handler* handler);
	void do_showpage(operator_handler* handler);
	void do_repeat(operator_handler* handler);
	void do_for(operator_handler* handler);
	void do_findfont(operator_handler* handler);
	void do_scalefont(operator_handler* handler);
	void do_setfont(operator_handler* handler);
	void do_selectfont(operator_handler* handler);
	void do_show(operator_handler* handler);
	void do_charpath(operator_handler* handler);
	void do_string(operator_handler* handler);
	void do_stringwidth(operator_handler* handler);
	void do_put(operator_handler* handler);
	void do_flattenpath(operator_handler* handler);
	void do_cvs(operator_handler* handler);
	void do_cvx(operator_handler* handler);
	void do_aload(operator_handler* handler);
	void do_quit(operator_handler* handler);
	void do_length(operator_handler* handler);
	void do_save(operator_handler* handler);
	void do_restore(operator_handler* handler);
	void do_print_top_stack(operator_handler* handler);
	void do_stack(operator_handler* handler);
	void do_misc_ops(operator_handler* handler);
	void do_logic_misc_ops(operator_handler* handler);
	void do_array(operator_handler* handler);
	void do_astore(operator_handler* handler);
	void do_setglobal(operator_handler* handler);
	void do_replace_matrix(operator_handler* handler);
	void do_initmatrix(operator_handler* handler);
	void do_setmatrix(operator_handler* handler);
	void do_concat(operator_handler* handler);
	void do_setpagedevice(operator_handler* handler);
	void do_exec(operator_handler* handler);
	void do_currentfile(operator_handler* handler);
	void do_token(operator_handler* handler);
};

struct system_dictionary : public base_dictionary
{
	processor* m_processor{ nullptr };
	system_dictionary(processor* _processor) : base_dictionary(), m_processor(_processor)
	{
	}
	~system_dictionary()
	{
	}
	void put(const operand& key, const operand& value)
	{
		throw runtime_error("Write error in --put--");
	}
	bool find(const char* name, operand &value)
	{
		return m_processor->find_key(name, value);
	}	
	bool find(const operand& key, operand& value)
	{
		return m_processor->find_key(key, value);
	}
	bool get(const operand& key, operand &value)
	{
		return m_processor->find_key(key, value);
	}
	void write(ostream& os)
	{
		os << "-dict-";
	}
	int32_t size() const
	{
		return m_processor->operator_dictionary_size();
	}
	operand_type subtype() const
	{
		return ot_system_dictionary;
	}
	void clear()
	{
	}
	void clone()
	{
		addref();
	}
	bool key_exists(const char* name)
	{
		operand tmp;

		return m_processor->find_key(name, tmp);
	}
	bool key_exists(const operand& key)
	{
		operand tmp;

		return m_processor->find_key(key, tmp);
	}
};
