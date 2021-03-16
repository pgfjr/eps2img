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

// todo: use the one in math.h
#define M_PI 3.14159265358979323846

double __deg2rad(double v)
{
	return v * (M_PI / 180.0);
}

double __rad2deg(double v)
{ return v * 180.0 / M_PI; 
}

static int subcount = 0;

void processor::do_math_binary_ops(operator_handler* handler)
{
	operand op2 = m_operand_stack[0];
	operand op1 = m_operand_stack[1];
	double result{ 0 };
	operand_type new_type;

	new_type = (op1.m_type == op2.m_type) ? op1.m_type : ot_real;

	switch (handler->m_op_id)
	{
	case op_id_add:
		result = op1.m_number + op2.m_number;
		break;
	case op_id_sub:
		result = op1.m_number - op2.m_number;
		break;
	case op_id_mul:
		result = op1.m_number * op2.m_number;
		break;
	case op_id_div:
		if (0.0 == op2.m_number)
		{
			message("Zero divisor in -div-");
		}
		result = op1.m_number / op2.m_number;
		new_type = ot_real;
		break;
	case op_id_idiv:
		if (ot_integer == new_type)
		{
			if (0.0 == op2.m_number)
			{
				message("Zero divisor in -idiv-");
			}
			result = (int32_t)(int32_t(op1.m_number) / int32_t(op2.m_number));
		}
		else
		{
			message("Type mismatch in -idiv-. Both parameters must be integers");
		}
		break;
	case op_id_mod:
		if (ot_integer == new_type)
		{
			if (0.0 == op2.m_number)
			{
				message("Zero divisor in -mod-");
			}
			result = (int32_t)int32_t(op1.m_number) % int32_t(op2.m_number);
		}
		else
		{
			message("Type mismatch in -mod-. Both parameters must be integers");
		}
		break;
	case op_id_atan:
		result = atan2(__deg2rad(op1.m_number), __deg2rad(op2.m_number));
		result = __rad2deg(result);
		new_type = ot_real;
		break;
	case op_id_exp:
		result = pow(op1.m_number, op2.m_number);
		new_type = ot_real;
		break;
	}
	pop(2);

	op1.m_type = new_type;
	op1.m_number = result;

	push_operand(op1);
}


void processor::do_math_unary_ops(operator_handler* handler)
{
	operand& op = m_operand_stack[0];
	double& number = op.m_number;
	
	switch (handler->m_op_id)
	{
	case op_id_sqrt:
		number = sqrt(number);
		op.m_type = ot_real;
		break;
	case op_id_ln:
		number = log(number);
		op.m_type = ot_real;
		break;
	case op_id_log:
		number = log10(number);		
		op.m_type = ot_real;
		break;
	case op_id_sin:
		number = sin(__deg2rad(number));
		op.m_type = ot_real;
		break;
	case op_id_cos:
		number = cos(__deg2rad(number));
		op.m_type = ot_real;
		break;
	case op_id_abs:
		number = abs(number);
		break;
	case op_id_neg:
		number = -number;
		break;
	case op_id_ceiling:
		number = ceil(number);
		break;
	case op_id_floor:
		number = floor(number);
		break;
	case op_id_round:
		number = round(number);
		break;
	case op_id_truncate:
		number = trunc(number);
		break;
	}
}

void processor::do_math_misc_ops(operator_handler* handler)
{
	const int rand_max = (int)(pow(2, 31) - 1);
	switch (handler->m_op_id)
	{
	case op_id_rand:
		
		push_number((int32_t)(rand() %  (2147483648 - 1)), ot_integer);
		
		break;
	case op_id_srand:
		{
			operand& op = m_operand_stack[0];

			if (!op.is_integer())
			{
				message("Type check in --srand--. Parameter must be an integer");
			}
			m_rand = (unsigned)abs(op.m_number);
			srand(m_rand);
			pop(1);
		}
		break;
	case op_id_rrand:
		push_number(m_rand, ot_integer);
		break;
	}
}