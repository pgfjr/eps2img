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

void processor::do_array(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];
	

	if (op1.is_integer())
	{
		int size = (int) op1.m_number;

		if (size < 0 || size > MAX_OBJECT_SIZE)
		{
			message("Range check in --%s--. Array size must be 0 to %u. Current size: %d", handler->m_name, MAX_OBJECT_SIZE, size);
		}
		else
		{
			array_type* arr = new array_type(size, ot_array, alloc_type::at_local); // todo: check alloc_type

			if (arr)
			{
				operand opr(ot_array);

				opr.m_object = arr;


				op1 = opr;
			}
			else
			{
				message("Out of memory in --%s--", handler->m_name);
			}
		}
	}
	else
	{
		message("Type check in --%s--", handler->m_name);
	}
}

void processor::do_astore(operator_handler* handler)
{
	operand& op1 = m_operand_stack[0];

	if (!op1.is_array())
	{
		message("Type check in --%s--", handler->m_name);
	}
	else
	{
		array_type* arr = op1.as_array();

		if (arr)
		{
			const size_t array_size = arr->size();

			if (array_size == 0)
			{
				return;
			}
			else
			{
				const size_t _stack_size = stack_size();

				// stack size must be at least the size of the array
				if ((_stack_size - 1) < array_size) // exclude the array from the size
				{
					message("Stack underflow in --%s--", handler->m_name);
				}
				for (size_t i = 0, index = array_size - 1; i < array_size; ++i, --index)
				{
					// exchange position
					do_exch();

					//put the top item on the array

					arr->put(index, m_operand_stack[0]);

					// remove the top item
					pop();
				}
			}
		}
		else
		{
			// unlikely is is_array() above succeeds but check anyway
			message("Type check in --%s--", handler->m_name);
		}
	}
}