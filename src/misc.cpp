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

void processor::do_misc_ops(operator_handler* handler)
{
	switch (handler->m_op_id)
	{
	case op_id_exit:
		if (m_loop_count > 0)
			--m_loop_count;
		break;
	case op_id_start:
		break;
	case op_id_languagelevel:
		push_number(1, ot_integer);
		break;
	case op_id_product:
		{
			string product("EPS2IMG");

			push_string(product, ot_text_string);
		}
		break;
	case op_id_version:
	{
		string version("1.0");

		push_string(version, ot_text_string);
	}
		break;
	}
}

void processor::do_setglobal(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (!op.is_bool())
	{
		message("Type check in --%s--", handler->m_name);
	}
	else
	{
		m_alloc_type = op.m_bool ? at_global : at_local;

		pop();
	}
}

void processor::do_setpagedevice(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_dictionary())
	{
		base_dictionary* dct = op.as_dictionary();

		if (dct && dct->subtype() == ot_user_dictionary)
		{
			dictionary_type* d = static_cast<dictionary_type*>(dct);
			operand value;

			if (d->find("PageSize", value))
			{
				if (value.is_array())
				{
					const size_t array_size = 2;
					array_type* arr = value.as_array();

					if (arr->is_numeric() && arr->size() == array_size)
					{
						double v[array_size]{ 0 };

						if (arr->get_numbers(v, array_size))
						{
							if (v[0] > 0.0 && v[1] > 0.0)
							{
								m_width = v[0];
								m_height = v[1];
							}
							else
							{
								message("Range check in --%s--", handler->m_name);
							}
						}
					}
				}
			}
			pop();

			return;
		}
	}
		
	message("Type check in --%s--", handler->m_name);	

}

void processor::do_cvs(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	operand& op2 = m_operand_stack[1];

	if (op1.is_text_string())
	{
		string_type* str = op1.as_string();

		if (str)
		{
			size_t dest_len = (size_t)str->size();
			size_t src_len{ 0 };

			if (!op2.is_string_type())
			{
				char buf[128]{ 0 };


				if (op2.is_integer())
				{
					src_len = sprintf_s(buf, sizeof(buf) - 1, "%d", (int32_t)op2.m_number);
				}
				else if (op2.is_real())
				{
					src_len = sprintf_s(buf, sizeof(buf) - 1, "%f", (float)op2.m_number);
					src_len = trim_spaces(buf, src_len);
				}
				else if (op2.is_bool())
				{
					src_len = sprintf_s(buf, sizeof(buf) - 1, "%s", op2.m_bool ? "true" : "false");
				}
				else if (op2.is_operator())
				{
					const operator_handler* h = op2.m_operator;

					src_len = sprintf_s(buf, sizeof(buf) - 1, "%s", h->m_name);
				}
				else
				{
					src_len = sprintf_s(buf, sizeof(buf) - 1, "--nostringval--");
				}

				if (src_len <= dest_len)
				{
					operand result = op1;
					string& tmp = str->m_data;

					tmp.replace(0, src_len, buf);

					pop(2);

					push_operand(result);
				}
				else
				{
					message("Range check in --cvs--");
				}
			}
			else
			{
				string_type* src = op2.as_string();

				if (src)
				{
					src_len = (size_t)src->size();

					if (src_len <= dest_len)
					{
						operand result = op1;
						string& dest_str = str->m_data;
						string& tmp = src->m_data;

						dest_str.replace(0, src_len, tmp);

						pop(2);

						push_operand(result);
					}
					else
					{
						message("Range check in --cvs--");
					}
				}
				else
				{
					message("Type check in --cvs--");
				}
			}
		}
		else
		{
			message("Type check in --cvs--");
		}
	}
	else
	{
		message("Type check in --cvs--");
	}
}

void processor::do_cvx(operator_handler* handler)
{
	operand& op = m_operand_stack[0];
	operand_type type = op.m_type;

	switch (type)
	{
	case ot_array: // change to procedure
		op.m_type = ot_procedure;
		op.m_exec = true;
		break;
	case ot_literal:
		op.m_type = ot_name;
		op.m_exec = true;
		break;
	case ot_name: // unlikely since this only appears inside a procedure, but keep it anyway
		op.m_exec = true;
		break;
	case ot_text_string: // todo
		message("Unsupported parameter '()' in --%s--", handler->m_name);
		break;
	}

}

void processor::do_exec(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_name())
	{
		operand name = op;

		pop();

		if (name.m_object)
		{
			string_type* str = dynamic_cast<string_type*>(name.m_object);
			if (str)
			{
				do_name(str->data(), name.m_type);
			}
		}
	}
	else if (op.is_procedure())
	{
		operand name = op;

		pop();

		execute_procedure(name);
	}
	else if (op.is_text_string())
	{
		//todo
		message("Unsupported parameter '()' in --%s--", handler->m_name);
	}
}

void processor::do_token(operator_handler* handler)
{
	operand& op = m_operand_stack[0];

	if (op.is_file())
	{
		operand new_op(ot_boolean);
		scanner* scr = op.m_scanner;
		token tkn;

		pop();

		if (scr->has_token(tkn))
		{
			// pretend to be in a procedure to avoid executing names, except for constants
			temp_procedure(true);

			if (process_token(tkn))
			{
				new_op.m_bool = true;

				temp_procedure(false);
			}
			else
			{
				return;
			}
		}
		
		push_operand(new_op);		
	}
	else if (op.is_text_string())
	{
		//todo
		message("Unsupported parameter '()' in --%s--", handler->m_name);
	}
	else
	{
		message("Type check in --%s--", handler->m_name);
	}
}

void processor::do_currentfile(operator_handler* handler)
{
	operand op(ot_file);

	op.m_scanner = &m_scanner;

	push_operand(op);
}