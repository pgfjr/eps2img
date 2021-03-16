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
#include <sstream>

void processor::pop(size_t count)
{
	if (count > m_operand_stack.size())
	{
		message("Stack underflow");
	}
	else
	{
		for (size_t i = 0; i < count; ++i)
		{
			m_operand_stack.pop_front();
		}
	}
}

void processor::pop()
{
	pop(1);
}

void processor::push_operand(operand& op)
{
		m_operand_stack.push_front(op);
}

void processor::push_type(operand_type type)
{
	operand op(type);

	push_operand(op);
}

void processor::push_number(double number, operand_type type)
{
	operand op(type);

	op.m_number = number;

	push_operand(op);
}

void processor::push_name(const char* name, int32_t len, operand_type type)
{
	string_type* str;

	if (len < 0)
	{
		str = new string_type(name, type);
	}
	else
	{
		str = new string_type(name, (size_t)len, type);
	}
	if (!str)
	{
		message("Not enough memory to allocate a string object");
	}
	else
	{
		operand op(type);

		op.m_object = str;

		push_operand(op);
	}
}

void processor::push_string(const string& str, operand_type type)
{
	const char* data = str.data();
	int32_t len = (int32_t)str.length();

	push_name(data, len, type);
}

int32_t processor::counttomark()
{
	int32_t i = 0;

	for (const auto& o : m_operand_stack)
	{
		if (o.is_marker_on())
		{
			return i;
		}
		++i;
	}

	return NOTFOUND;
}

void processor::create_array(operand_type type)
{
	int32_t index;

	index = counttomark();

	if (NOTFOUND == index)
	{
		// procedure will not have this problem
		message("Missing marker '['");
	}
	else
	{
		array_type *arr = new array_type(index, type);

		if (!arr)
		{
			message("Not enough memory to create an array/procedure object");
		}
		else
		{
			operand op(type);

			op.m_object = arr;

			if (ot_procedure == type)
			{
				op.m_exec = true;
			}
			for (int32_t i = 0, slot = index-1; i < index; ++i, --slot)
			{
				operand& tmp = m_operand_stack[i];

				arr->put(slot, tmp);
				
			}
			// remove the items and marker from the stack

			pop(index + 1);

			push_operand(op);
		}
	}
}

void processor::do_copy(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (!op.is_integer())
	{
		message("Type mismatch in --copy--. Integer parameter required");
	}
	else
	{
		size_t count = (size_t)op.m_number;
		size_t stack_size = m_operand_stack.size() - 1;

		if (count > stack_size)
		{
			message("Stack underflow in --copy--. Parameter(s) required: %u. Stack size: %u", count, stack_size);
		}
		m_operand_stack.pop_front();

		for (size_t i = 0, index = count - 1; i < count; ++i)
		{
			operand& tmp = m_operand_stack[index];
			
			push_operand(tmp);
		}
	}
}

void processor::do_exch()
{
	operand op = m_operand_stack[0];
	operand op2 = m_operand_stack[1];

	pop(2);

	push_operand(op);
	push_operand(op2);
}

void processor::do_stack_ops(operator_handler* handler)
{
	switch (handler->m_op_id)
	{
	case op_id_dup:
		{
			operand& op = m_operand_stack[0];

			push_operand(op);
		}
		break;
	case op_id_exch:
		do_exch();
		break;
	case op_id_pop:
		m_operand_stack.pop_front();
		break;
	case op_id_clear:
		m_operand_stack.clear();
		break;
	case op_id_mark:
		push_type(ot_array_marker_on); 
		break;
	case op_id_count:
		push_number(m_operand_stack.size(), ot_integer);
		break;
	case op_id_counttomark:
		{
			int32_t index = counttomark();

			if (NOTFOUND == index)
			{
				message("Marker not found in --counttomark--");
			}
			push_number(index, ot_integer);
		}
		break;
	case op_id_cleartomark:
		{
			int32_t index = counttomark();

			if (NOTFOUND == index)
			{
				message("Marker not found in --cleartomark--");
			}
			pop(index + 1);
		}
		break;
	case op_id_index:
	{
		operand& op = m_operand_stack[0];
		

		if (!op.is_integer())
		{
			message("Type mismatch in --index--. Parameter must be an integer");
		}
		else
		{
			int32_t stack_size = (int32_t)(m_operand_stack.size() - 1);
			int32_t index = (int32_t)op.m_number;

			if (index < 0)
			{
				message("Range check in --index--. Index is negative: %d", index);
			}
			else if (index >= stack_size)
			{
				message("Stack underflow in --index--. Index is out of range: %u", index);
			}
			else
			{
				pop(); // pop the index

				operand& op = m_operand_stack[index];

				push_operand(op);
			}
		}
	}
	break;
	}
}


void processor::do_pstack(operator_handler* handler)
{
	dump_stack();
}

void processor::do_print_n_pop(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	cout << op << endl;

	pop();
}

void processor::do_pop(operator_handler* handler)
{
	pop();
}

void processor::do_roll(operator_handler* handler)
{
	int32_t times;
	size_t count;

	if (!m_operand_stack[0].is_integer())
	{
		message("Type check in --roll--. Both parameters must be integers");
	}
	else if (!m_operand_stack[1].is_integer())
	{
		message("Type check in --roll--. Both parameters must be integers");
	}

	times = (int32_t)m_operand_stack[0].m_number;
	count = (size_t)m_operand_stack[1].m_number;

	if (count < 0)
	{
		message("Range check in --roll--.");
	}
	else if ((stack_size() - 2) < count)
	{
		message("Stack underflow in --roll--.");
	}

	pop(2);

	if (count > 0 && times != 0)
	{
		int32_t index = (int32_t)(stack_size() - count);

		if (times > 0)
		{
			for (int32_t i = 0; i < times; ++i)
			{
				std::rotate(m_operand_stack.begin(), m_operand_stack.begin() + 1, m_operand_stack.end() - index);
			}
		}
		else if (times < 0)
		{
			int32_t next = index + 1;

			times = abs(times);

			for (int32_t i = 0; i < times; ++i)
			{
				std::rotate(m_operand_stack.rbegin() + index, m_operand_stack.rbegin() + next, m_operand_stack.rend());
			}
		}
	}
}


void processor::do_aload(operator_handler* handler)
{
	if (m_operand_stack[0].is_array())
	{
		operand op = m_operand_stack[0];
		array_type* arr = op.as_array();

		if (arr)
		{
			pop();

			for (auto& i : arr->m_data)
			{
				push_operand(i);
			}

			push_operand(op);
		}
	}
}

void processor::do_print_top_stack(operator_handler* handler)
{
	cout << m_operand_stack[0] << endl;
	pop();
}

void processor::do_stack(operator_handler* handler)
{
	for (auto& op : m_operand_stack)
	{
		switch (op.m_type)
		{
		case ot_boolean:
		case ot_integer:
		case ot_real:
			cout << op << endl;
			break;
		case ot_hex_string:
		case ot_literal:
		case ot_name:
		case ot_text_string:
			{
				string_type *str = dynamic_cast<string_type*>(op.m_object);

				if (str)
				{
					cout << str->data() << endl;
				}
			}
			break;
		default:
			cout << "--nostringval--\n";
			break;
		}
	}
}