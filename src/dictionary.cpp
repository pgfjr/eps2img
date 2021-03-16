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

static int counter = 0;

void processor::do_name(const char* name, operand_type type)
{
	operand value;

	if (ot_name == type)
	{
		if (in_procedure())
		{
			push_name(name, -1, type);

			return;
		}
	}
	if( m_dictionary->find(name, value))
	{
		if (value.is_procedure())
		{
			execute_procedure(value);
		}
		else if (value.is_operator())
		{			
			execute_operator(value.m_operator);
		}
		else
		{
			push_operand(value);
		}
	}
	else
	{
		// search system dictionary
		if (search_system_dictionary(name, value))
		{
			if (value.is_operator())
			{
				
				execute_operator(value.m_operator);
			}
			else
			{
				push_operand(value);
			}
		}
		else
		{
			message("Undefined in --%s--", name);
		}		
	}
}

void processor::execute_procedure(operand &op)
{	
	{
		array_type* proc = op.as_array();

		if (proc)
		{
			for (operand& item : proc->m_data)
			{
				if (item.is_name())
				{
					if (item.m_object)
					{
						string_type* str = dynamic_cast<string_type*>(item.m_object);
						if (str)
						{
							do_name(str->data(), item.m_type);
						}
					}
				}
				else if (item.is_operator())
				{
					execute_operator(item.m_operator);
				}
				else if (item.is_marker_off())
				{
					if (ot_array_marker_off == item.m_type)
					{
						create_array(ot_array);
					}
					else if (ot_dictionary_marker_off == item.m_type)
					{
						create_dictionary();
					}
				}
				else
				{
					push_operand(item);
				}
			}
		}
	}
}

void processor::execute_operator(operator_handler* handler)
{
	size_t param_count = handler->m_param_count;
	size_t stack_size = m_operand_stack.size();

	if ( param_count > 0)
	{
		if (param_count > stack_size)
		{
			message("Stack underflow in --%s--. Stack size: %u. Required: %u", handler->m_name, stack_size, param_count);
		}
		else if (handler->m_numeric_param)
		{
			for (size_t i = 0; i < param_count; ++i)
			{
				if (!m_operand_stack[i].is_number())
				{
					message("Type mismatch in --%s--. Parameter %u is not numeric", handler->m_name, i);
				}
			}
		}
	}

	(this->*handler->func)(handler);
}

void processor::create_dictionary()
{
	int32_t index  = counttomark();

	if (NOTFOUND == index)
	{
		// procedure will not have this problem
		message("Missing marker '<<'");
	}
	else if ((index % 2) != 0)
	{
		message("Number of dictionary items must be even. Current size is %d", index);
	}
	else
	{
		dictionary_type* dct = new dictionary_type((size_t)index, m_alloc_type);

		if (!dct)
		{
			message("Not enough memory to create a dictionary object");
		}
		else
		{
			operand op(ot_dictionary);

			op.m_object = dct;

			for (int32_t start_index = 0; start_index < index; start_index += 2)
			{
				operand& value = m_operand_stack[start_index];
				operand& key = m_operand_stack[start_index + 1];

				dct->put(key, value);
			}
			// remove the items and marker from the stack

			pop(index + 1);

			push_operand(op);
		}
	}
}

void processor::do_def(operator_handler* handler)
{
	operand& value = m_operand_stack[0];
	operand& key = m_operand_stack[1];

	m_dictionary->put(key, value);

	pop(2);
}

void processor::do_dictionary_begin(operand& op)
{
	base_dictionary* dict = op.as_dictionary();

	if (dict)
	{
		m_dictionary->push(dict);

		pop();
	}
	else
	{
		message("Dictionary operand expected in --begin--");
	}
}

void processor::do_dictionary_end()
{
	if (!m_dictionary->pop())
	{
		message("Dictionary stack underflow in --end--");
	}
}

void processor::do_dictionary_ops(operator_handler* handler)
{
	switch (handler->m_op_id)
	{
	case op_id_bind:
		do_bind(m_operand_stack[0]);
		break;
	case op_id_begin:
		do_dictionary_begin(m_operand_stack[0]);
		break;
	case op_id_end:
		do_dictionary_end();
		break;
	case op_id_dict:
		do_dict(m_operand_stack[0]);
		break;
	case op_id_load:
		do_load(m_operand_stack[0]);
		break;
	case op_id_currentdict:
		{
			operand op = m_dictionary->currentdict();
			push_operand(op);
		}
		break;
	}
}

void processor::do_bind(operand &op)
{
	array_type* arr = op.as_array();

	if (arr)
	{
		for (auto& it : arr->m_data)
		{
			if (it.is_name())
			{
				string_type* str = dynamic_cast<string_type*>(it.m_object);

				if (str)
				{
					const char* name = str->data();
					operand value;
					
					if(search_system_dictionary(name, value))
					{						
						// replace

						it = value;
					}
				}
			}
			else if (it.is_procedure())
			{
				do_bind(it);
			}
		}
	}
	else if (!op.is_procedure())
	{
		message("Type mismatch in --bind--. Parameter must be a procedure");
	}
}

void processor::do_dict(operand& op)
{
	if (op.is_integer())
	{
		int32_t count = (int32_t)op.m_number;

		if (count < 0 || count > MAX_OBJECT_SIZE)
		{
			message("Range check in --dict--. Parameter size must be from 0 to %u.", MAX_OBJECT_SIZE);
		}
		else
		{
			dictionary_type *dict = new dictionary_type((size_t)count, m_alloc_type);

			if (dict)
			{
				operand op(ot_dictionary);

				op.m_object = dict;

				pop(); // pop count parameter

				push_operand(op);
			}
			else
			{
				message("Not enough memory to create a dictionary object");
			}
		}
	}
	else
	{
		message("Type mismatch in --dict--. Integer operand required.");
	}
}

void processor::do_load(operand& key)
{
	operand value;

	if(m_dictionary->find(key, value))
	{
		pop();

		push_operand(value);

		return;
	}
	else if (key.is_string_type())
	{
		const string_type* str = key.as_string();
		operand value;

		if (search_system_dictionary(str->data(), value))
		{
			pop();

			push_operand(value);
		}
		else
		{
			message("Undefined in --load-- (key '%s' not found)", str->data());
		}

	}
	else
	{
		message("Undefined in --load--. Key not found");
	}
}

void processor::do_where(operator_handler* handler)
{
	operand result(ot_boolean);
	operand key = m_operand_stack[0];
	base_dictionary *dict;

	pop();
	
	dict = m_dictionary->where(key);

	if (dict)
	{
		operand dict_op(ot_dictionary);

		dict_op.m_object = dict;

		push_operand(dict_op);

		result.m_bool = true;
	}
	else if (key.is_string_type())
	{
		string_type* str = key.as_string();

		if (str)
		{
			const char* name = str->data();
			operand value;

			if (search_system_dictionary(name, value))
			{
				system_dictionary* sys_dict = new system_dictionary(this);

				if (sys_dict)
				{
					operand dict_op(ot_dictionary);

					dict_op.m_object = sys_dict;

					push_operand(dict_op);
				}
				else
				{
					message("Not enough memory to create a dictionary object");
				}

				result.m_bool = true;
			}
			else
			{
				result.m_bool = false;
			}
		}
		else
		{
			// not possible since key name can't be null
			message("Type check in --where--");
		}
	}
	else
	{
		result.m_bool = false;		
	}

	push_operand(result);
}

void processor::do_repeat(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];

	if (op1.is_procedure() && op2.is_integer())
	{
		int32_t times = (int32_t)op2.m_number;

		if (times < 0)
		{
			message("Range check in --repeat--");
		}
		else if (0 == times)
		{
			pop(2);
		}
		else
		{
			operand proc = op1;

			pop(2);

			for (int32_t i = 0; i < times; ++i)
			{
				execute_procedure(proc);
			}
		}
	}
	else
	{
		message("Type check in --repeat--");
	}
}

void processor::do_for(operator_handler* handler)
{
	operand& op4 = m_operand_stack[0];
	operand& op3 = m_operand_stack[1];
	operand& op2 = m_operand_stack[2];
	operand& op1 = m_operand_stack[3];

	if (op1.is_number() && op2.is_number() && op3.is_number() && op4.is_procedure())
	{
		operand_type initial_type = op1.m_type;
		operand_type increment_type = op2.m_type;
		double initial = op1.m_number;
		double increment = op2.m_number;
		double limit = op3.m_number;
		operand proc = op4;
		
		{
			if (0.0 == increment)
			{
				message("Infinite loop in --for--. Increment is 0");
			}
			else
			{
				double control = initial;
				operand_type new_type = initial_type == increment_type ? initial_type : ot_real;
				int32_t loop_count = ++m_loop_count;

				pop(handler->m_param_count);

				if (increment > 0.0)
				{
					while (control <= limit && loop_count <= m_loop_count)
					{
						push_number(control, new_type);
			
						execute_procedure(proc);
						
						control += increment;
					}					
				}
				else if (increment < 0.0)
				{
					while (control >= limit && loop_count <= m_loop_count)
					{
						push_number(control, new_type);

						execute_procedure(proc);

						control += increment;
					}					
				}
			}			
		}
	}
	else
	{
		message("Type check in --for--");
	}
}

static int get_count = 0;

void processor::do_get(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];

	if (op2.is_array() && op1.is_integer())
	{
		array_type* arr = op2.as_array();

		if (arr)
		{
			operand result;

			result = arr->get((size_t)op1.m_number);

			pop(handler->m_param_count);

			push_operand(result);
		}
		else
		{
			pop(handler->m_param_count);
		}
	}
	else if (op2.is_text_string() && op1.is_integer())
	{
		string_type* str = op2.as_string();

		if (str)
		{
			operand result(ot_integer);

			result.m_number = str->get((int32_t)op1.m_number);

			pop(handler->m_param_count);

			push_operand(result);
		}
		else
		{
			pop(handler->m_param_count);
		}
	}
	else if (op2.is_dictionary())
	{
		base_dictionary* dict = op2.as_dictionary();

		if (dict)
		{
			operand value;

			if( dict->get(op1, value))
			{
				pop(handler->m_param_count);

				push_operand(value);				
			}
			else
			{
				message("Undefined in --get--");
			}
		}
		else
		{
			pop(handler->m_param_count);
		}
	}
	else
	{
		message("Type check in --get--");
	}
}

void processor::do_put(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];
	operand& op3 = m_operand_stack[2];

	if (op3.is_array() && op2.is_integer())
	{
		array_type* arr = op3.as_array();

		if (arr)
		{
			arr->put((size_t)op2.m_number, op1);
		}
		pop(handler->m_param_count);
	}
	else if (op3.is_text_string() && op2.is_integer() && op1.is_integer())
	{
		string_type* str = op3.as_string();

		if (str)
		{
			str->put((size_t)op2.m_number, (int32_t)op1.m_number);
		}
		pop(handler->m_param_count);
	}
	else if (op3.is_dictionary())
	{
		base_dictionary* dict = op3.as_dictionary();

		if (dict)
		{
			dict->put(op2, op1);
		}
		pop(handler->m_param_count);
	}
	else
	{
		message("Type check in --put--");
	}
}

void processor::do_length(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_composite_type())
	{
		composite_object* obj = op.m_object;

		if (op.m_object)
		{
			operand result(ot_integer);

			result.m_number = obj->size();

			pop();

			push_operand(result);
		}
		else
		{
			pop();
		}
	}
	else
	{
		message("Type check in --length--");
	}
}


void processor::do_save(operator_handler* handler)
{
	dictionary_container* cur_dict = new dictionary_container;

	if (!cur_dict)
	{
		message("Not enough memory to save the current state");
	}
	try
	{
		operand op(ot_save);

		*cur_dict = *m_dictionary;

		op.m_object = cur_dict;

		push_operand(op);

		do_gsave(handler);
	}
	catch (const exception& ex)
	{
		if (cur_dict)
		{
			delete cur_dict;
		}

		throw ex;
	}
}

void processor::do_restore(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_save())
	{
		dictionary_container* old_dct = dynamic_cast<dictionary_container *>(op.m_object);

		if (old_dct)
		{
			m_dictionary->release();

			m_dictionary = old_dct;

			m_dictionary->addref(); // addref; old_dct will be destroyed below
			
			pop();

			do_grestore(handler);			
		}
		else
		{
			message("Type check in --restore--");
		}
	}
	else
	{
		message("Type check in --restore--");
	}
}