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
#include "scanner.h"
#include "processor.h"


class application
{
	string m_error;
	string m_output_file;
	bool run_loop(scanner &sc, double width, double height, bool is_interactive);
	bool create_output_filename(const char* filename, const char* output_file);
public:
	application() : m_error(), m_output_file()
	{
	}
	~application()
	{
	}
	string error() const
	{
		return m_error;
	}
	string output_file() const
	{
		return m_output_file;
	}
	bool convert(const char* filename, const char* output_file);
};