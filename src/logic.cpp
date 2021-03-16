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

void processor::do_eq(operator_handler* handler)
{
	operand& second = m_operand_stack[0];
	operand& first = m_operand_stack[1];
	operand eq(ot_boolean);
	bool result = false;

	if (first.is_number() && second.is_number())
	{
		result = first.m_number == second.m_number;
	}
	else if (first.is_string_type() && second.is_string_type())
	{
		string_type* s1 = first.as_string();
		string_type* s2 = second.as_string();

		if (s1 && s2)
		{
			result = s1->compare(s2) == 0;
		}
	}
	else if (first.m_type == second.m_type)
	{		
		if (first.is_composite_type() && first.m_object && second.m_object)
		{
			result = first.m_object == second.m_object;
		}
		else
		{
			result = first.m_dummy == second.m_dummy;
		}		
	}

	eq.m_bool = result;

	pop(2);

	push_operand(eq);
}

void processor::do_ne(operator_handler* handler)
{
	do_eq(handler);

	// flip the result

	m_operand_stack[0].m_bool = !m_operand_stack[0].m_bool;
}

void processor::do_lt(operator_handler* handler)
{
	operand& second = m_operand_stack[0];
	operand& first = m_operand_stack[1];
	
	operand op(ot_boolean);

	if (first.is_number() && second.is_number())
	{
		op.m_bool = first.m_number < second.m_number;
	}
	else if (first.is_text_string() && second.is_text_string())
	{
		string_type* s1 = first.as_string();
		string_type* s2 = second.as_string();

		if (s1 && s2)
		{
			op.m_bool = s1->compare(s2) < 0;
		}
		else
		{
			op.m_bool = false;
		}
	}
	else
	{
		message("Type mismatch in --lt--. Parameters must be both numbers or both strings.");
	}

	pop(2);

	push_operand(op);

}

void processor::do_le(operator_handler* handler)
{
	operand& second = m_operand_stack[0];
	operand& first = m_operand_stack[1];

	operand op(ot_boolean);

	if (first.is_number() && second.is_number())
	{
		op.m_bool = first.m_number <= second.m_number;
	}
	else if (first.is_text_string() && second.is_text_string())
	{
		string_type* s1 = first.as_string();
		string_type* s2 = second.as_string();

		if (s1 && s2)
		{
			op.m_bool = s1->compare(s2) <= 0;
		}
		else
		{
			op.m_bool = false;
		}
	}
	else
	{
		message("Type mismatch in --lt--. Parameters must be both numbers or both strings.");
	}

	pop(2);

	push_operand(op);

}

void processor::do_gt(operator_handler* handler)
{
	operand& second = m_operand_stack[0];
	operand& first = m_operand_stack[1];

	operand op(ot_boolean);

	if (first.is_number() && second.is_number())
	{
		op.m_bool = first.m_number > second.m_number;
	}
	else if (first.is_text_string() && second.is_text_string())
	{
		string_type* s1 = first.as_string();
		string_type* s2 = second.as_string();

		if (s1 && s2)
		{
			op.m_bool = s1->compare(s2) > 0;
		}
		else
		{
			op.m_bool = false;
		}
	}
	else
	{
		message("Type mismatch in --lt--. Parameters must be both numbers or both strings.");
	}

	pop(2);

	push_operand(op);

}

void processor::do_ge(operator_handler* handler)
{
	operand& second = m_operand_stack[0];
	operand& first = m_operand_stack[1];

	operand op(ot_boolean);

	if (first.is_number() && second.is_number())
	{
		op.m_bool = first.m_number >= second.m_number;
	}
	else if (first.is_text_string() && second.is_text_string())
	{
		string_type* s1 = first.as_string();
		string_type* s2 = second.as_string();

		if (s1 && s2)
		{
			op.m_bool = s1->compare(s2) >= 0;
		}
		else
		{
			op.m_bool = false;
		}
	}
	else
	{
		message("Type mismatch in --lt--. Parameters must be both numbers or both strings.");
	}

	pop(2);

	push_operand(op);

}

void processor::do_true(operator_handler* handler)
{
	operand op(ot_boolean);

	op.m_bool = true;

	push_operand(op);
}

void processor::do_false(operator_handler* handler)
{
	operand op(ot_boolean);

	op.m_bool = false;

	push_operand(op);
}

void processor::do_if(operator_handler* handler)
{
	operand& proc = m_operand_stack[0];
	operand& condition = m_operand_stack[1];

	if (!proc.is_procedure() || !condition.is_bool())
	{
		message("Type check in --if--.");
	}
	else
	{
		operand pr = m_operand_stack[0];
		operand con = m_operand_stack[1];

		pop(2);

		if (con.m_bool)
		{
			execute_procedure(pr);
		}
	}
}

void processor::do_ifelse(operator_handler* handler)
{
	operand& proc2 = m_operand_stack[0];
	operand& proc1 = m_operand_stack[1];
	operand& condition = m_operand_stack[2];

	if (!proc2.is_procedure() || !proc1.is_procedure() || !condition.is_bool())
	{
		message("Type check in --ifelse--.");
	}
	else
	{
		operand pr2 = m_operand_stack[0];
		operand pr1 = m_operand_stack[1];
		operand con = m_operand_stack[2];

		pop(3);

		if (con.m_bool)
		{
			execute_procedure(pr1);
		}
		else
		{
			execute_procedure(pr2);
		}
	}
}

void processor::do_logic_misc_ops(operator_handler* handler)
{
	operand op1, op2;
	bool is_number{ false };
	int32_t num1{ 0 }, num2{ 0 };
	bool b1{ false }, b2{ false };

	if (2 == handler->m_param_count)
	{
		op2 = m_operand_stack[0];
		op1 = m_operand_stack[1];

		if (op1.is_integer() && op2.is_integer())
		{
			is_number = true;

			num1 = (int32_t)op1.m_number;
			num2 = (int32_t)op2.m_number;
		}
		else if (op1.is_bool() && op2.is_bool())
		{
			b1 = op1.m_bool;
			b2 = op2.m_bool;
		}
		else
		{
			message("Type check in --%s--", handler->m_name);
		}

		pop(2);
	}
	else if (1 == handler->m_param_count)
	{
		op1 = m_operand_stack[0];

		if (op1.is_integer())
		{
			is_number = true;

			num1 = (int32_t)op1.m_number;
		}
		else if (op1.is_bool())
		{
			b1 = op1.m_bool;
		}
		else
		{
			message("Type check in --%s--", handler->m_name);
		}
		pop();
	}
	if (is_number)
	{
		operand result(ot_integer);

		switch (handler->m_op_id)
		{
		case op_id_and:
			result.m_number = (num1 & num2);
			break;
		case op_id_not:
			result.m_number = ~num1;
			break;
		case op_id_or:
			result.m_number = (num1 | num2);
			break;
		case op_id_xor:
			result.m_number = (num1 ^ num2);
			break;
		case op_id_bitshift:
			if (num2 > 0)
			{
				result.m_number = (num1 << num2);
			}
			else if (num2 < 0)
			{
				result.m_number = (num1 >> (-num2));
			}
			break;
		}
		push_operand(result);
	}
	else
	{
		operand result(ot_boolean);

		switch (handler->m_op_id)
		{
		case op_id_and:
			result.m_bool = (b1 && b2) ? true : false;
			break;
		case op_id_not:
			result.m_bool = !b1;
			break;
		case op_id_or:
			result.m_bool = (!b1 && !b2) ? false : true;
			break;
		case op_id_xor:
			result.m_bool = (b1 == b2) ? false : true;
			break;
		}
		push_operand(result);
	}
}