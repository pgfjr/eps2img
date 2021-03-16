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

#include "application.h"


int main(int argc, char* argv[])
{
	cout << "EPS2IMG (c) 2020 Peter Frane Jr. All Rights Reserved\n";
	cout << "Distributed under a GPL 3.0 license\n\n";

	if (argc < 2)
	{
		cout << "\nUsage: eps2img input_file [output_file.pdf]\n";
		cout << "\n       Where 'input_file' is an EPS file regardless of file extension (i.e., .EPS or .PS).\n\n";

		return 1;
	}
	else
	{
		const char* output_file = argc > 2 ? argv[2] : nullptr;
		application app;

		if (app.convert(argv[1], output_file))
		{
			cout << "\nSuccess (" << app.output_file() << ")\n";

			return 0;
		}
		else
		{
			cout << app.error() << endl;

			return 1;
		}
	}
}

