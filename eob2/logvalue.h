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

#include "variant.h"

#pragma once

namespace log {
struct valuei {
	const char* text;
	long		number;
	void*		data;
	variant	identifier;
	void clear() { memset(this, 0, sizeof(*this)); }
	const char* read(const char* p, stringbuilder& sb);
};
const char*		psval(const char* p, variant& v);
}