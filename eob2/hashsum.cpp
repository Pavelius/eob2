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

#include "hashsum.h"
#include "nameable.h"

unsigned long hashsum(const char* format) {
	if(!format)
		return 0;
	unsigned long total = 0;
	for(auto i = 0; format[i]; i++)
		total += ((unsigned char)format[i]) * (i + 1);
	return total;
}

unsigned long hashsum(array& source) {
	unsigned long total = 0;
	int index = 1;
	auto pe = source.end();
	auto size = source.size();
	for(auto p = source.begin(); p < pe; p += size)
		total += hashsum(((nameable*)p)->id) * (index++);
	return total;
}