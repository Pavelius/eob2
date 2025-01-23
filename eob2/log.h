/////////////////////////////////////////////////////////////////////////
// 
// Copyright 2024 Pavel Chistyakov
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

typedef void (*fnoutput)(const char* format);
typedef void(*fnread)(const char* url);

namespace log {
struct contexti {
	const char* url;
	const char* file;
	const char* header;
	void clear();
};
extern contexti context;
extern bool allowparse;
extern fnoutput	print_proc;
void			close();
bool			checksym(const char* p, char sym);
void			errorp(const char* position, const char* format, ...);
extern int		errors;
void			errorv(const char* position, const char* format, const char* format_param);
bool			errorpresent();
void			print(const char* format, ...);
void			println();
void			printv(const char* format);
void			printv(const char* format, const char* format_param);
const char*		read(const char* url, bool error_if_not_exist = true);
void			readf(fnread proc, const char* url, const char* filter);
void			readlf(fnread proc, const char* url, const char* filter);
const char*		skipws(const char* p);
const char*		skipwscr(const char* p);
}
