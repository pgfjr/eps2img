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

/*
struct operator_handler
{
	const char* m_name;
	short m_param_count;
	bool m_numeric_param;
	operator_id m_op_id;
	void (processor::* func)(const char *name, short m_param_count, operator_id m_op_id);
};
*/
static operator_handler handlers[] =
{
	{"=",  1, false, op_id_print_top_stack,&processor::do_print_top_stack },
	{"==",  1, false, op_id_print_n_pop,&processor::do_print_n_pop },
	{"abs", 1, true, op_id_abs,&processor::do_math_unary_ops},
	{"add", 2, true, op_id_add,&processor::do_math_binary_ops},
	{"aload",  1, false, op_id_aload,&processor::do_aload },
	{"and",  2, false, op_id_and,&processor::do_logic_misc_ops },
	{"arc", 5, true, op_id_arc,&processor::do_path_ops},
	{"arcn", 5, true, op_id_arcn,&processor::do_path_ops},
	{"array", 1, true, op_id_array,&processor::do_array},
	{"astore", 2, false, op_id_astore,&processor::do_astore},
	{"atan", 2, true, op_id_atan,&processor::do_math_binary_ops},
	{"begin", 1, false, op_id_begin, &processor::do_dictionary_ops},
	{"bind", 1, false, op_id_bind,&processor::do_dictionary_ops},
	{"bitshift",  2, false, op_id_bitshift,&processor::do_logic_misc_ops },
	{"ceiling", 1, true, op_id_ceiling,&processor::do_math_unary_ops},
	{"charpath",  2, false, op_id_charpath,&processor::do_charpath },
	{"clear", 0, false, op_id_clear,&processor::do_stack_ops},
	{"cleartomark", 1, false, op_id_cleartomark,&processor::do_stack_ops},
	{"clip",  0, false, op_id_clip,&processor::do_path_ops},
	{"clippath",  0, false, op_id_clippath,&processor::do_clippath},
	{"closepath", 0, false, op_id_closepath,&processor::do_path_ops},
	{"concat",  1, false, op_id_concat,&processor::do_concat },
	{"concatmatrix",  3, false, op_id_concatmatrix,&processor::do_concatmatrix },
	{"copy", 1, true, op_id_copy,&processor::do_copy},
	{"cos", 1, true, op_id_cos,&processor::do_math_unary_ops},
	{"count", 0, false, op_id_count,&processor::do_stack_ops},
	{"counttomark", 1, false, op_id_counttomark,&processor::do_stack_ops},
	{"currentcmykcolor",  0, false, op_id_currentcmykcolor,&processor::do_currentcmykcolor},
	{"currentdict", 0, false, op_id_currentdict,&processor::do_dictionary_ops},
	{"currentfile",  0, false, op_id_currentfile,&processor::do_currentfile},
	{"currentflat",  0, false, op_id_currentflat,&processor::do_get_graphics_state},
	{"currentgray",  0, false, op_id_currentgray,&processor::do_currentgray},
	{"currentlinecap",  0, false, op_id_currentlinecap,&processor::do_get_graphics_state },
	{"currentlinejoin",  0, false, op_id_currentlinejoin,&processor::do_get_graphics_state },
	{"currentlinewidth",  0, false, op_id_currentlinewidth,&processor::do_get_graphics_state },
	{"currentmatrix",  1, false, op_id_currentmatrix,&processor::do_replace_matrix },
	{"currentmiterlimit",  0, false, op_id_currentmiterlimit,&processor::do_get_graphics_state },
	{"currentpoint", 0, false, op_id_currentpoint,&processor::do_path_ops},
	{"currentrgbcolor",  0, false, op_id_currentrgbcolor,&processor::do_currentrgbcolor},
	{"curveto", 6, true, op_id_curveto,&processor::do_path_ops},
	{"cvs",  2, false, op_id_cvs,&processor::do_cvs },
	{"cvx",  1, false, op_id_cvx,&processor::do_cvx },
	{"def", 2, false, op_id_def, &processor::do_def},
	{"defaultmatrix",  1, false, op_id_defaultmatrix,&processor::do_replace_matrix },
	{"dict", 1, true, op_id_dict, &processor::do_dictionary_ops},
	{"div", 2, true, op_id_div,&processor::do_math_binary_ops},
	{"dtransform", 2, false, op_id_dtransform,&processor::do_matrix_transform},
	{"dup", 1, false, op_id_dup,&processor::do_stack_ops},
	{"end", 0, false, op_id_end, &processor::do_dictionary_ops},
	{"eofill",  0, false, op_id_eofill,&processor::do_path_ops},
	{"eq",  2, false, op_id_eq,&processor::do_eq},
	{"erasepage",  0, false, op_id_erasepage,&processor::do_path_ops},
	{"exch", 2, false, op_id_exch,&processor::do_stack_ops},
	{"exec", 1, false, op_id_exec,&processor::do_exec},
	{"exit",  0, false, op_id_exit,&processor::do_misc_ops },
	{"exp", 1, true, op_id_exp,&processor::do_math_binary_ops},
	{"fill", 0, false, op_id_fill,&processor::do_path_ops},
	{"findfont",  1, false, op_id_findfont,&processor::do_findfont },
	{"flattenpath",  0, false, op_id_flattenpath,&processor::do_flattenpath},
	{"floor", 1, true, op_id_floor,&processor::do_math_unary_ops},
	{"for",  4, false, op_id_for,&processor::do_for },
	{"ge",  2, false, op_id_ge,&processor::do_ge},
	{"get",  2, false, op_id_get,&processor::do_get },
	{"get",  2, false, op_id_get,&processor::do_get},
	{"grestore",  0, false, op_id_grestore,&processor::do_grestore },
	{"gsave",  0, false, op_id_gsave,&processor::do_gsave },
	{"gt",  2, false, op_id_gt,&processor::do_gt},
	{"identmatrix",  1, false, op_id_identmatrix,&processor::do_replace_matrix },
	{"idiv", 2, true, op_id_idiv,&processor::do_math_binary_ops},	
	{"idtransform", 2, false, op_id_idtransform,&processor::do_matrix_transform},
	{"if",  2, false, op_id_if,&processor::do_if},
	{"ifelse",  3, false, op_id_ifelse,&processor::do_ifelse},
	{"index", 2, false, op_id_index,&processor::do_stack_ops},
	{"initmatrix",  0, false, op_id_initmatrix,&processor::do_initmatrix },
	{"invertmatrix",  2, false, op_id_invertmatrix,&processor::do_invertmatrix },
	{"itransform", 2, false, op_id_itransform,&processor::do_matrix_transform},
	{"languagelevel", 0, false, op_id_languagelevel,&processor::do_misc_ops},
	{"le",  2, false, op_id_le,&processor::do_le},
	{"length",  1, false, op_id_length,&processor::do_length },
	{"lineto", 2, true, op_id_lineto,&processor::do_path_ops},
	{"ln", 1, true, op_id_ln,&processor::do_math_unary_ops},
	{"load", 1, false, op_id_load, &processor::do_dictionary_ops},
	{"log", 1, true, op_id_log,&processor::do_math_unary_ops},
	{"lt",  2, false, op_id_lt,&processor::do_lt},
	{"mark", 0, false, op_id_mark,&processor::do_stack_ops},
	{"matrix",  0, false, op_id_matrix,&processor::do_matrix },
	{"mod", 2, true, op_id_mod,&processor::do_math_binary_ops},
	{"moveto", 2, true, op_id_moveto,&processor::do_path_ops},
	{"mul", 2, true, op_id_mul,&processor::do_math_binary_ops},
	{"neg", 1, true, op_id_neg,&processor::do_math_unary_ops},
	{"newpath", 0, false, op_id_newpath,&processor::do_path_ops},
	{"not",  1, false, op_id_not,&processor::do_logic_misc_ops },
	{"or",  2, false, op_id_or,&processor::do_logic_misc_ops },
	{"pop",  1, false, op_id_pop,&processor::do_pop},
	{"product", 0, false, op_id_product,&processor::do_misc_ops},
	{"pstack", 0, false, op_id_pstack,&processor::do_pstack},
	{"put",  3, false, op_id_put,&processor::do_put },
	{"quit",  0, false, op_id_quit,&processor::do_quit },
	{"rand", 0, false, op_id_rand,&processor::do_math_misc_ops},
	{"rcurveto", 6, true, op_id_rcurveto,&processor::do_path_ops},
	{"rectfill", 4, true, op_id_rectfill,&processor::do_path_ops},
	{"rectstroke", 4, true, op_id_rectstroke,&processor::do_path_ops},
	{"repeat",  2, false, op_id_repeat,&processor::do_repeat },
	{"restore",  1, false, op_id_restore,&processor::do_restore },
	{"rlineto", 2, true, op_id_rlineto,&processor::do_path_ops},
	{"rmoveto", 2, true, op_id_rmoveto,&processor::do_path_ops},
	{"roll",  2, true, op_id_roll,&processor::do_roll},
	{"rotate",  1, false, op_id_rotate,&processor::do_rotate},
	{"round", 1, true, op_id_round,&processor::do_math_unary_ops},
	{"rrand", 0, false, op_id_rrand,&processor::do_math_misc_ops},
	{"save",  0, false, op_id_save,&processor::do_save },
	{"scale",  2, false, op_id_scale,&processor::do_scale},
	{"scalefont",  1, true, op_id_scalefont,&processor::do_scalefont },
	{"selectfont",  2, false, op_id_selectfont,&processor::do_selectfont },
	{"setcmybcolor",  4, true, op_id_setcmykcolor,&processor::do_setcmykcolor}, // use cmyk
	{"setcmykcolor",  4, true, op_id_setcmykcolor,&processor::do_setcmykcolor},
	{"setdash",  2, false, op_id_setdash,&processor::do_setdash},
	{"setflat",  1, true, op_id_setflat,&processor::do_set_graphics_state},
	{"setfont",  1, false, op_id_setfont,&processor::do_setfont },
	{"setglobal", 1, false, op_id_setglobal,&processor::do_setglobal },
	{"setgray",  1, true, op_id_setgray,&processor::do_set_graphics_state},
	{"setlinecap",  1, true, op_id_setlinecap,&processor::do_setlinecap},
	{"setlinejoin",  1, true, op_id_setlinejoin,&processor::do_setlinejoin},
	{"setlinewidth",  1, true, op_id_setlinewidth,&processor::do_set_graphics_state},
	{"setmatrix",  1, false, op_id_setmatrix,&processor::do_setmatrix },
	{"setmiterlimit",  1, true, op_id_setmiterlimit,&processor::do_set_graphics_state},
	{"setpagedevice",  1, false, op_id_setpagedevice,&processor::do_setpagedevice },
	{"setrgbcolor",  3, true, op_id_setrgbcolor,&processor::do_setrgbcolor},
	{"show",  1, false, op_id_show,&processor::do_show },
	{"showpage",  0, false, op_id_showpage,&processor::do_showpage },
	{"sin", 1, true, op_id_sin,&processor::do_math_unary_ops},
	{"sqrt", 1, true, op_id_sqrt,&processor::do_math_unary_ops},
	{"srand", 1, true, op_id_srand,&processor::do_math_misc_ops},
	{"stack",  1, false, op_id_stack,&processor::do_stack },
	{"start",  0, false, op_id_start,&processor::do_misc_ops },
	{"string",  1, true, op_id_string,&processor::do_string },
	{"stringwidth",  1, false, op_id_stringwidth,&processor::do_stringwidth },
	{"stroke", 0, false, op_id_stroke,&processor::do_path_ops},
	{"sub", 2, true, op_id_sub,&processor::do_math_binary_ops},
	{"token",  1, false, op_id_token,&processor::do_token },
	{"transform",  2, false, op_id_transform,&processor::do_matrix_transform},
	{"translate",  2, false, op_id_translate,&processor::do_translate},
	{"truncate", 1, true, op_id_truncate,&processor::do_math_unary_ops},
	{"version", 0, false, op_id_version,&processor::do_misc_ops },
	{"where",  1, false, op_id_where,&processor::do_where},
	{"xor",  2, false, op_id_xor,&processor::do_logic_misc_ops },

};

static int compare_name(const void* s1, const void* s2)
{
	const operator_handler* p1 = (operator_handler*)s1;
	const operator_handler* p2 = (operator_handler*)s2;

	return strcmp(p1->m_name, p2->m_name);
}

operator_handler* processor::search_operator_dictionary(const char* name)
{
	size_t table_size = array_size(handlers);
	operator_handler key = { name, 0, false, op_id_null, nullptr };
	

	return (operator_handler*)bsearch(&key, handlers, table_size, sizeof(handlers[0]), compare_name);	
}

bool processor::search_system_dictionary(const char* name, operand& value)
{
	operator_handler* h = search_operator_dictionary(name);

	if (h)
	{
		value.m_type = ot_operator;
		value.m_operator = h;

		return true;
	}
	else
	{
		switch (*name)
		{
		case 'f':
			if (strcmp(name, "false") == 0)
			{
				value.m_type = ot_boolean;
				value.m_bool = false;
				return true;
			}
			break;
		case 't':
			if (strcmp(name, "true") == 0)
			{
				value.m_type = ot_boolean;
				value.m_bool = true;
				return true;
			}
			break;
		case 'n':
			if (strcmp(name, "null") == 0)
			{
				value.m_type = ot_null;
				value.m_dummy = 0;
				return true;
			}
			break;
		}
		return false;
	}
}

bool processor::find_key(const char* name, operand& value)
{
	return search_system_dictionary(name, value);
}

bool processor::find_key(const operand& key, operand& value)
{
	if (key.is_string_type())
	{
		const string_type* str = dynamic_cast<string_type*>(key.m_object);

		if (str)
		{
			const char* name = str->data();

			return search_system_dictionary(name, value);
		}
	}

	return false;
}

int32_t processor::operator_dictionary_size()
{
	return (int32_t)array_size(handlers);
}