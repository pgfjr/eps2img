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

#pragma once
#include <string>
#include <iterator>
#include <iostream>
#include <iomanip>

#include <map>
#include <deque>
#include <forward_list>
#include <inttypes.h>
#include <stdarg.h>
#include "operator_id.h"

#pragma warning(disable : 4996)
#pragma warning(disable : 26812)

using namespace std;

#define NOTFOUND -1
#define MAX_NAME_LEN 127
#define MAX_LINE_BUF 256
#define MAX_OBJECT_SIZE 65536

// short bond paper

#define DEFAULT_WIDTH 612.0f // 612 pts = 8.5 inches
#define DEFAULT_HEIGHT 792.0f // 792 pts = 11 inches


//enum class operand_type
enum operand_type
{
	ot_null,	
	ot_array_marker_on,
	ot_array_marker_off,
	ot_boolean,
	ot_comment,
	ot_constant,
	ot_dictionary_marker_on,
	ot_dictionary_marker_off,
	ot_dsc,
	ot_file,
	ot_integer,
	ot_operator,
	ot_procedure_marker_on,
	ot_procedure_marker_off,
	ot_real,
	ot_composite,
	ot_ascii_text_b85,
	ot_array,
	ot_dictionary,
	ot_user_dictionary,
	ot_state_dictionary,
	ot_system_dictionary,
	ot_font,
	ot_hex_string,
	ot_literal,
	ot_name,
	ot_procedure,
	ot_save,
	
	ot_text_string
};

enum alloc_type
{
	at_local,
	at_global
};

class composite_object
{
	int m_refcount{ 1 };
protected:
	operand_type m_type;// { operand_type::ot_null };
	alloc_type m_alloc_type{ at_local };
public:
	composite_object(operand_type type, alloc_type _alloc_type) : m_type(type), m_alloc_type(_alloc_type)
	{
	}
	virtual ~composite_object()
	{
	}
	int addref()
	{
		return ++m_refcount;
	}
	int release()
	{
		if (0 == --m_refcount)
		{
			delete this;

			return 0;
		}

		return m_refcount;
	}
	alloc_type get_alloc_type() const
	{
		return m_alloc_type;
	}
	virtual void write(ostream& os) = 0;
	virtual int32_t size() const = 0;
	virtual void clear() = 0;

};

struct operator_handler;
class processor;
struct array_type;
struct string_type;
struct dictionary_type;
struct font_type;
struct base_dictionary;
class scanner;

struct operand
{
	union
	{
		bool   m_bool;
		double m_number;
		scanner* m_scanner;
		composite_object* m_object;
		operator_handler* m_operator;
		uint64_t m_dummy;
	};
	bool m_exec{ false };
	operand_type m_type{ ot_null };
	operand();
	operand(const operand& op);
	operand(operand_type type);
	operand(double value, bool is_real);
	~operand();
	void clear();
	operand& operator=(const operand& op);	
	void copy(const operand& op);
	bool is_array() const;
	bool is_array_type() const;
	bool is_bool() const;
	bool is_composite_type() const;
	bool is_dictionary() const;
	bool is_file() const;
	bool is_font() const;
	bool is_number() const;
	bool is_real() const;
	bool is_integer() const;
	bool is_literal() const;
	bool is_procedure() const;
	bool is_name() const;
	bool is_operator() const;
	bool is_marker_on() const;
	bool is_marker_off() const;
	bool is_save() const;
	bool is_string() const;
	bool is_string_type() const;
	bool is_text_string() const;
	bool is_matrix() const;
	bool is_matrix(double* values);
	int addref();
	array_type* as_array();
	base_dictionary* as_dictionary();
	string_type* as_string();
	void clone(const operand& src, alloc_type atype);
	
	friend ostream& operator<<(ostream &os, const operand& op);
};

struct array_type : public composite_object
{
	deque<operand> m_data;
	array_type() : composite_object(ot_array, at_local), m_data()
	{
	}
	array_type(size_t size, operand_type type) : composite_object(type, at_local), m_data()
	{
		m_data.resize(size);
	}
	array_type(size_t size, operand_type type, alloc_type _alloc_type) : composite_object(type, _alloc_type), m_data()
	{
		m_data.resize(size);
	}
	~array_type()
	{
		m_data.clear();
	}
	int32_t size() const
	{
		return (int32_t)m_data.size();
	}
	operand get(size_t index) const
	{
		if (index < m_data.size())
		{
			return m_data[index];
		}
		throw runtime_error("Range check in --get--");
	}
	void put(size_t index, operand &op)
	{
		if (index < m_data.size())
		{
			m_data[index] = op;
		}
		else
		{
			throw runtime_error("Range check in --put--");
		}
	}
	void put(operand& op)
	{
		m_data.push_back( op );
	}
	array_type& operator=(const array_type& src)
	{
		m_data = src.m_data;

		return *this;
	}
	void clear()
	{
		m_data.clear();
	}
	bool is_numeric()
	{
		if (size() > 0)
		{
			for (auto& op : m_data)
			{
				if (!op.is_number())
				{
					return false;
				}
			}

			return true;
		}
		return false;
	}
	bool is_matrix()
	{
		if (size() == 6 && is_numeric())
		{
			return true;
		}
		return false;
	}
	// call is_numeric before calling this function!
	size_t get_numbers(double* v, size_t count)
	{
		size_t array_size = (size_t)size();

		if (count > 0 && count <= array_size)
		{
			for (size_t i = 0; i < count; ++i)
			{
				v[i] = m_data[i].m_number;
			}

			return count;
		}

		return 0;
	}
	void write(ostream& os);
	array_type* clone(alloc_type atype);
};

struct string_type : public composite_object
{
	string m_data;
	string_type() : composite_object(ot_text_string, at_global), m_data()
	{
	}
	string_type(size_t size) : composite_object(ot_text_string, at_global), m_data(size, 0)
	{
	}
	string_type(const char *str) : composite_object(ot_text_string, at_global), m_data(str)
	{
	}
	string_type(const char* str, operand_type type) : composite_object(type, at_global), m_data(str)
	{
	}
	string_type(const char* str, size_t len, operand_type type) : composite_object(type, at_global), m_data(str, len)
	{
	}
	~string_type()
	{
		m_data.clear();
	}
	int32_t size() const
	{
		return (int32_t)m_data.size();
	}
	const char* data() const
	{
		return m_data.c_str();
	}
	char get(size_t index) const
	{
		if (index < m_data.size())
		{
			return m_data[index];
		}
		throw runtime_error("Range check in --get--");
	}
	void put(size_t index, int32_t ch)
	{
		if (index < m_data.size() && ( ch >= 0 && ch <= 255))
		{
			m_data[index] = (char)ch;
		}
		else
		{
			throw runtime_error("Range check in --put--");
		}
	}
	void put(char ch)
	{
		m_data.push_back( ch );
	}
	string_type& operator=(const string_type& src)
	{
		m_data = src.m_data;

		return *this;
	}
	int compare(const string_type* s2)
	{
		return m_data.compare(s2->m_data);
	}
	void clear()
	{
		m_data.clear();
	}
	void write(ostream& os);
};

struct base_dictionary : public composite_object
{
	base_dictionary() : composite_object(ot_dictionary, at_local)
	{
	}
	base_dictionary(alloc_type _alloc_type) : composite_object(ot_dictionary, _alloc_type)
	{
	}
	virtual ~base_dictionary()
	{
	}
	virtual void clear() = 0;
	virtual int32_t size() const = 0;
	virtual void write(ostream& os) = 0;
	
	virtual bool find(const char* name, operand& value) = 0;
	virtual bool find(const operand& key, operand& value) = 0;
	virtual bool key_exists(const char* name) = 0;
	virtual bool key_exists(const operand& key) = 0;
	virtual void put(const operand& key, const operand& value) = 0;
	virtual void clone() = 0;
	virtual bool get(const operand& key, operand& value) = 0;
	virtual operand_type subtype() const = 0;
};

using dictionary = map<string, operand>;

struct dictionary_type : public base_dictionary
{
	dictionary m_data;
	dictionary m_data2; // for non-string keys

	size_t m_max_size{ 65536 };

	dictionary_type() : base_dictionary(), m_data(), m_data2()
	{
	}
	dictionary_type(alloc_type _alloc_type) : base_dictionary(_alloc_type), m_data(), m_data2()
	{
	}
	dictionary_type(size_t max_size, alloc_type _alloc_type) : base_dictionary(_alloc_type), m_data(), m_data2(), m_max_size(max_size)
	{
	}	
	~dictionary_type()
	{
		clear();
	}
	int32_t size() const
	{
		return (int32_t)(m_data.size()+m_data2.size());
	}
	void clear()
	{
		m_data.clear();
		m_data2.clear();
	}
	
	bool find(const char* name, operand &value);
	bool find(const operand& key, operand& value);
	void clone();
	dictionary_type* clone(alloc_type atype);
	void write(ostream& os);
	
	bool get(const operand& key, operand &value)
	{
		return find(key, value);
	}
	dictionary_type& operator=(const dictionary_type& dct)
	{
		m_data = dct.m_data;

		m_data2 = dct.m_data2;

		return *this;
	}
	void put(const operand& key, const operand& value)
	{
		insert(key, value);
	}
	operand_type subtype() const
	{
		return ot_user_dictionary;
	}
	bool key_exists(const char* name);
	bool key_exists(const operand& key);
protected:
	void insert(dictionary& dict, string& key, const operand& value);
	void to_string(string& str, const operand& key);		
	operand *find(const char* name);
	operand* find(const operand& key);
	void insert(const operand& key, const operand& value);
};


using user_dictionary = deque<base_dictionary*>;

// dictionary_stack
struct dictionary_container : public composite_object
{
	user_dictionary m_dictionary_stack;
	dictionary_type m_local_dictionary;
	base_dictionary* m_current_dictionary{ nullptr };

	dictionary_container() : composite_object(ot_save, at_global), m_dictionary_stack(),
		m_local_dictionary(),
		m_current_dictionary(&m_local_dictionary)
	{
		m_local_dictionary.addref(); // prevent deletion
	}
	dictionary_container(alloc_type _alloc_type) : composite_object(ot_save, _alloc_type), m_dictionary_stack(),
		m_local_dictionary(),
		m_current_dictionary(&m_local_dictionary)
	{
		m_local_dictionary.addref(); // prevent deletion
	}
	~dictionary_container()
	{
		clear();
	}
	void write(ostream& os)
	{
	}
	int32_t size() const
	{
		return 0;
	}
	void clear()
	{
		for (auto it : m_dictionary_stack)
		{
			it->release();
		}

		m_dictionary_stack.clear();

		m_local_dictionary.clear();

		m_current_dictionary = &m_local_dictionary;
	}
	operand_type subtype() const
	{
		return ot_state_dictionary;
	}
	dictionary_container& operator=(const dictionary_container& src)
	{

		for (auto it : src.m_dictionary_stack)
		{
			if (it->subtype() == ot_user_dictionary)
			{
				dictionary_type* dict = dynamic_cast<dictionary_type*>(it);

				if (dict)// can't be null
				{
					dictionary_type* new_dict = dict->clone(at_local); // todo

					if (!new_dict)
					{
						throw runtime_error("Unable to save the current state");
					}

					m_dictionary_stack.push_back(new_dict);
				}
			}
			else
			{
				base_dictionary* base = it;

				base->addref();

				m_dictionary_stack.push_back(base);
			}
		}

		m_local_dictionary.clear();

		for (auto it : src.m_local_dictionary.m_data)
		{
			operand value;

			value.clone(it.second, at_local);

			m_local_dictionary.m_data[it.first] = value;
		}

		for (auto it : src.m_local_dictionary.m_data2)
		{
			operand value;

			value.clone(it.second, at_local);

			m_local_dictionary.m_data2[it.first] = value;
		}
		
		m_current_dictionary = get_current_dictionary();

		return *this;
	}
	operand currentdict()
	{
		operand op(ot_dictionary);

		op.m_object = m_current_dictionary;

		return op;
	}
	void put(const operand& key, const operand& value)
	{
		m_current_dictionary->put(key, value);
	}
	base_dictionary* where(const operand& key)
	{
		for (auto it : m_dictionary_stack)
		{
			if( it->key_exists(key))
			{
				return it;
			}
		}

		if (m_local_dictionary.key_exists(key))
		{
			return &m_local_dictionary;
		}

		return nullptr;
	}
	bool find(const char* name, operand& value)
	{
		for (auto it : m_dictionary_stack)
		{
			if (it->find(name, value))
			{
				return true;
			}
		}
		if (m_local_dictionary.find(name, value))
		{
			return true;
		}
		return false;
	}
	bool find(const operand &key, operand& value)
	{
		for (auto it : m_dictionary_stack)
		{
			if (it->find(key, value))
			{
				return true;
			}
		}
		if (m_local_dictionary.find(key, value))
		{
			return true;
		}
		return false;
	}
	void push(base_dictionary* dict)
	{

		m_dictionary_stack.push_front(dict);

		dict->addref();

		m_current_dictionary = dict;
	}
	bool pop()
	{
		if (!m_dictionary_stack.empty())
		{
			base_dictionary* dict = m_dictionary_stack.front();
						
			m_dictionary_stack.pop_front();

			dict->release();

			m_current_dictionary = get_current_dictionary();

			return true;
		}

		return false;
	}
	protected:
		base_dictionary* get_current_dictionary()
		{
			if (!m_dictionary_stack.empty())
				return m_dictionary_stack.front();
			return &m_local_dictionary;
		}
};


class common_class
{
protected:
	string m_error;
	bool m_error_flag{ false };
public:
	common_class() : m_error()
	{}
	virtual ~common_class()
	{}
	void clear()
	{
		m_error.clear();
		m_error_flag = false;
	}
	void set_error(bool flag)
	{
		m_error_flag = flag;
	}
	string& error()
	{
		return m_error;
	}
	void message(const char* format, ...)
	{
		char buffer[256]{ 0 };
		va_list args;

		va_start(args, format);
		vsprintf_s(buffer, sizeof(buffer) - 1, format, args);
		va_end(args);

		m_error = buffer;

		m_error_flag = true;

		throw runtime_error(m_error);
	}
	bool has_error() const
	{
		return m_error_flag == true;
	}	
};

struct operator_handler
{
	const char* m_name;
	size_t m_param_count;
	bool m_numeric_param;
	operator_id m_op_id;
	void (processor::* func)(operator_handler* handler);
};

enum slant_type
{
	slant_type_normal,
	slant_type_italic,
	slant_type_oblique
};

struct font_type : public composite_object
{
	string m_name;
	double m_point_size{ 1.0 };
	double m_matrix[6]{ 0.0 };
	bool m_use_point{ true };
	slant_type m_slant_type{ slant_type_normal };
	bool m_bold{ false };

	font_type() : composite_object(ot_font, at_global), m_name()
	{
	}
	~font_type()
	{
	}
	int32_t size() const
	{
		return 0;// not applicable
	}
	void clear()
	{
		m_name.clear();
	}
	void write(ostream& os)
	{
		os << "-dict-";
	}
};

double __deg2rad(double v);
double __rad2deg(double v);
size_t trim_spaces(char* buf, size_t len);