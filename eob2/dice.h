#pragma once

struct dice {
	char c, d, b, m;
	int	maximum() const;
	int	minimum() const;
	int	roll() const;
	explicit operator bool() const { return c || b; }
};