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


#include "data.h"

operand::operand() : m_dummy(0)
{
}

operand::operand(const operand& op) : m_dummy(0)
{
	copy(op);
}

operand::operand(operand_type type) : m_dummy(0)
{
	m_type = type;
}

operand::operand(double value, bool is_real)
{
	m_number = value;
	m_type = is_real ? ot_real : ot_integer;
}

operand::~operand()
{
	clear();
}

void operand::clear()
{
	if (m_type > ot_composite)
	{
		if (m_object)
		{
			m_object->release();
		}
	}
	m_type = ot_null;
	m_dummy = 0;
}

operand& operand::operator=(const operand& op)
{
	clear();

	copy(op);

	return *this;
}

void operand::copy(const operand& op)
{
	m_type = op.m_type;

	if (op.m_type > ot_composite)
	{
		if (op.m_object)
		{
			m_object = dynamic_cast<composite_object*>(op.m_object);

			if (m_object)
			{
				m_object->addref();
			}
		}
		else
		{
			m_object = nullptr;
		}
	}
	else
	{
		m_dummy = op.m_dummy;
	}
}

int operand::addref()
{
	if (m_type > ot_composite)
	{
		if (m_object)
		{
			if (m_object)
			{
				return m_object->addref();
			}
		}
	}
	return 0;
}

bool operand::is_array() const
{
	return (m_type == ot_array);
}

bool operand::is_array_type() const
{
	return (m_type == ot_array) || (m_type == ot_procedure);
}

bool operand::is_bool() const
{
	return (m_type == ot_boolean);
}

bool operand::is_composite_type() const
{
	return m_type > ot_composite;
}

bool operand::is_dictionary() const
{
	return (m_type == ot_dictionary);
}

bool operand::is_file() const
{
	return (m_type == ot_file);
}

bool operand::is_font() const
{
	return (m_type == ot_font);
}

bool operand::is_number() const
{
	return (m_type == ot_real) || (m_type == ot_integer);
}

bool operand::is_integer() const
{
	return (m_type == ot_integer);
}

bool operand::is_real() const
{
	return (m_type == ot_real);
}

bool operand::is_literal() const
{
	return (m_type == ot_literal);
}

bool operand::is_procedure() const
{
	return (m_type == ot_procedure);
}

bool operand::is_name() const
{
	return (m_type == ot_name);
}

bool operand::is_string() const
{
	return (m_type == ot_name) || (m_type == ot_hex_string) || (m_type == ot_text_string);
}

bool operand::is_text_string() const
{
	return (m_type == ot_text_string) || (m_type == ot_hex_string);
}

bool operand::is_string_type() const
{
	return is_text_string() || is_literal();
}

bool operand::is_matrix() const
{
	if (ot_array == m_type && m_object != nullptr)
	{
		array_type* arr = dynamic_cast<array_type*>(m_object);

		if (arr && arr->is_matrix())
		{
			return true;
		}
	}

	return false;
}

bool operand::is_matrix(double* values)
{
	if (ot_array == m_type && m_object != nullptr)
	{
		array_type* arr = dynamic_cast<array_type*>(m_object);

		if (arr && arr->is_matrix())
		{
			arr->get_numbers(values, 6);

			return true;
		}
	}

	return false;
}
bool operand::is_operator() const
{
	return (m_type == ot_operator);
}

bool operand::is_marker_on() const
{
	return (m_type == ot_dictionary_marker_on || m_type == ot_array_marker_on || m_type == ot_procedure_marker_on);
}

bool operand::is_marker_off() const
{
	return (m_type == ot_dictionary_marker_off || m_type == ot_array_marker_off || m_type == ot_procedure_marker_off);
}

bool operand::is_save() const
{
	return (m_type == ot_save);
}

array_type* operand::as_array()
{
	if (is_array_type())
	{
		if (m_object)
		{
			return dynamic_cast<array_type*>(m_object);
		}
	}
	return nullptr;
}

base_dictionary* operand::as_dictionary()
{
	if (is_dictionary())
	{
		if (m_object)
		{
			return dynamic_cast<base_dictionary*>(m_object);
		}
	}
	return nullptr;
}

string_type* operand::as_string()
{
	if (is_string_type())
	{
		if (m_object)
		{
			return dynamic_cast<string_type*>(m_object);
		}
	}
	return nullptr;
}

void operand::clone(const operand& src, alloc_type atype)
{
	clear();

	if (src.is_composite_type())
	{
		composite_object* obj = static_cast<composite_object*>(src.m_object);
		
		if (obj)
		{
			m_object = obj;
			
			if (src.is_array_type())
			{
				array_type* arr = dynamic_cast<array_type*>(obj);

				if (arr)
				{
					m_object = arr->clone(atype);

					if (!m_object)
					{
						throw runtime_error("Not enough memory to duplicate an array");
					}
				}
			}
			else if (src.is_dictionary())
			{
				base_dictionary* base = dynamic_cast<base_dictionary*>(obj);

				if (base )
				{
					if (base->subtype() == ot_user_dictionary)
					{
						dictionary_type* dict = dynamic_cast<dictionary_type*>(base);

						if (dict)
						{
							m_object = dict->clone(atype);

							if (!m_object)
							{
								throw runtime_error("Not enough memory to duplicate a dictionary");
							}
						}
						else
						{
							//todo
							m_object->addref();
						}
					}
					else
					{
						m_object->addref();
					}
				}
			}
			else
			{
				m_object->addref();
			}
		}
	}
	else
	{
		m_dummy = src.m_dummy;
	}

	m_type = src.m_type;
}

ostream& operator<<(ostream& os, const operand& op)
{
	switch (op.m_type)
	{
	case ot_null:
		os << "null";
		break;
	case ot_array_marker_on:
	case ot_dictionary_marker_on:
		os << "-mark-";
		break;
	case ot_array_marker_off:
		os << ']';
		break;
	case ot_boolean:
		os << (op.m_bool ? "true" : "false");
		break;
	case ot_dictionary_marker_off:
		os << ">>";
		break;
	case ot_integer:
		os << (int)op.m_number;
		break;
	case ot_operator:
		if (op.m_operator)
		{			
			os << "--" << op.m_operator->m_name << "--";
		}
		break;
	case ot_real:		
		{
			char buf[32];
			size_t len;

			len = sprintf_s(buf, sizeof(buf) - 1, "%f", (float)op.m_number);
			trim_spaces(buf, len);

			os << buf;
		}
		break;
	case ot_array:
		if (op.m_object)
		{
			os << '[';

			op.m_object->write(os);

			os << ']';
		}
		else
		{
			os << "[]";
		}
		break;
	case ot_dictionary:
	case ot_font:
		os << "-dict-";
		break;
	case ot_file: // not supported
		os << "-file-";
		break;
	case ot_procedure:
		if (op.m_object)
		{
			os << '{';

			op.m_object->write(os);
			
			os << '}';
		}
		else
		{
			os << "{}";
		}
		break;
	case ot_save:
		os << "-save-";
		break;
	case ot_hex_string:
	case ot_literal:
	case ot_text_string:
	case ot_name:
		if (op.m_object)
		{
			((string_type*)op.m_object)->write(os);
		}
		else
		{
			os << "()";
		}
		break;
	}
	return os;
}

void dictionary_type::insert(dictionary& dict, string& key, const operand& value)
{
	try
	{
		auto it = dict.find(key);

		if (it == dict.end())
		{
			dict[key] = value;
		}
		else // replace
		{
			it->second = value;
		}
	}
	catch (...)
	{
		throw;
	}
}

void dictionary_type::insert(const operand& key, const operand& value)
{
	switch (key.m_type)
	{
	case ot_hex_string:
	case ot_literal:
	case ot_text_string:
		if( key.m_object)
		{
			string_type* str = dynamic_cast<string_type*>(key.m_object);

			if (str)
			{
				insert(m_data, str->m_data, value);
			}
		}
		break;
	default:
		{
		string str;

			switch (key.m_type)
			{
			case ot_boolean:
			case ot_integer:
			case ot_real:
				to_string(str, key);
				insert(m_data2, str, value);
				break;
			}
		}
		break;
	}
}

operand* dictionary_type::find(const char* name)
{
	auto it = m_data.find(string(name));

	if (it != m_data.end())
	{
		return &(it->second);
	}
	return nullptr;
}

bool dictionary_type::key_exists(const char* name)
{
	return find(name) != nullptr;
}

bool dictionary_type::key_exists(const operand& key)
{
	return find(key) != nullptr;
}

void dictionary_type::to_string(string& str, const operand& key)
{
	char buf[32]{ 0 };

	switch (key.m_type)
	{
	case ot_boolean:
		str = (key.m_bool) ? "true" : "false";
		break;
	case ot_integer:
		sprintf_s(buf, sizeof(buf) - 1, "%d", (int32_t)key.m_number);
		str = buf;
		break;
	case ot_real:
		sprintf_s(buf, sizeof(buf) - 1, "%f", (float)key.m_number);
		str = buf;
		break;
	}
}

operand* dictionary_type::find(const operand& key)
{
	switch (key.m_type)
	{
	case ot_boolean:
	case ot_integer:
	case ot_real:
		{
			string str;

			to_string(str, key);

			auto it = m_data2.find(str);

			if (it != m_data2.end())
			{
				return &(it->second);
			}
		}
		break;
	case ot_hex_string:
	case ot_literal:
	case ot_text_string:
		if (key.m_object)
		{
			string_type* str = dynamic_cast<string_type*>(key.m_object);

			if (str)
			{
				auto it = m_data.find(str->m_data);

				if (it != m_data.end())
				{
					return &(it->second);
				}
			}
		}		
		break;
	}

	return nullptr;
}

bool dictionary_type::find(const char* name, operand& value)
{
	operand* result = find(name);

	if (result)
	{
		value = *result;

		return true;
	}
	return false;
}

bool dictionary_type::find(const operand& key, operand& value)
{
	operand* result = find(key);

	if (result)
	{
		value = *result;

		return true;
	}
	return false;
}

void dictionary_type::clone()
{
	for (auto& it : m_data)
	{
		it.second.addref();
	}
	for (auto& it : m_data2)
	{
		it.second.addref();
	}
}

dictionary_type* dictionary_type::clone(alloc_type atype)
{
	dictionary_type* dict = new dictionary_type(atype);

	if (!dict)
	{
		return nullptr;
	}
	try
	{
		for (auto& it : m_data)
		{
			operand tmp;

			tmp.clone(it.second, atype);

			dict->m_data[it.first] = tmp;
		}
		for (auto& it : m_data2)
		{
			operand tmp;

			tmp.clone(it.second, atype);

			dict->m_data2[it.first] = tmp;
		}

		return dict;
	}
	catch (...)
	{
		delete dict;

		return nullptr;
	}
}


void array_type::write(ostream& os)
{
	size_t count = size();
	size_t i = 0;

	for (const auto& v : m_data)
	{
		os << v;

		if (i + 1 < count)
		{
			os << ' ';
		}
		++i;
	}
}

array_type* array_type::clone(alloc_type atype)
{
	array_type* tmp = new array_type((size_t)size(), m_type, atype);

	if (!tmp)
	{
		return nullptr;
	}
	try
	{
		size_t index = 0;

		for (const auto& it : m_data)
		{
			operand op;

			op.clone(it, atype);
			tmp->put(index, op);
			++index;
		}

		return tmp;
	}
	catch (...)
	{
		delete tmp;

		return nullptr;
	}
}

void string_type::write(ostream& os)
{
	if (ot_text_string == m_type)
	{
		size_t len = m_data.size();
		const char* str = m_data.data();
		char buf[20];

		os << '(';

		for (size_t i = 0; i < len; ++i)
		{
			if (isprint((uint8_t)str[i]))
			{
				os << str[i];
			}
			else
			{
				sprintf_s(buf, sizeof(buf) - 1, "\\%03o", (uint8_t)m_data[i]);

				os << buf;
			}
		}
		os << ')';
	}
	else if (ot_hex_string == m_type)
	{
		size_t len = m_data.size();
		char buf[20];

		os << '(';

		for (size_t i = 0; i < len; ++i)
		{
			sprintf_s(buf, sizeof(buf) - 1, "\\%03o", (uint8_t)m_data[i]);

			os << buf;
		}
		os << ')';
	}
	else if (ot_literal == m_type)
	{
		os << '/' << m_data;
	}
	else if (ot_name == m_type)
	{
		os << m_data;
	}
}

void dictionary_type::write(ostream& os)
{
	for (auto& v : m_data)
	{
		cout << v.first << " -> ";
		cout << v.second << '\n';
	}
	for (auto& v : m_data2)
	{
		cout << v.first << " -> ";
		cout << v.second << '\n';
	}
}

// used only for floats!
size_t trim_spaces(char* buf, size_t len)
{
	size_t new_size = len;

	for (size_t i = len - 1; i > 0; --i)
	{
		size_t prev = i - 1;
		if (buf[i] == '0')
		{
			if (isdigit((uint8_t)buf[prev]))
			{
				--new_size;
				continue;
			}
		}
		if (buf[prev] == '.' || buf[prev] == ',')
			break;
		// not a float
		else
			return new_size;
	}
	buf[new_size] = 0;

	return new_size;
}

