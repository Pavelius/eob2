#pragma once

extern char console_text[];

void console(const char* format, ...);
void consolenl();
void consolev(const char* format, const char* format_param);
