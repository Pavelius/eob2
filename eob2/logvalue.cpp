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

#include "logvalue.h"

const char* log::valuei::read(const char* p, stringbuilder& sb) {
	clear();
	if(*p == '\"') {
		sb.clear(); p = sb.psstr(p + 1, *p);
		text = szdup(sb.begin());
	} else if(*p == '-' || isnum(*p)) {
		auto minus = false;
		if(*p == '-') {
			minus = true;
			p++;
		}
		p = psnum(p, number);
		if(minus)
			number = -number;
	} else if(ischa(p[0])) {
		sb.clear(); p = sb.psidf(p);
		identifier = (const char*)sb.begin();
		if(identifier) {
			int bonus; p = psbon(p, bonus);
			identifier.counter = bonus;
			data = identifier.getpointer();
		} else
			text = szdup(sb.begin());
	}
	return p;
}

const char* log::psval(const char* p, variant& v) {
	if(!ischa(p[0]))
		return p;
	char temp[128]; stringbuilder sb(temp);
	sb.clear(); p = sb.psidf(p);
	v = (const char*)sb.begin();
	int bonus; p = psbon(p, bonus);
	v.counter = bonus;
	return p;
}