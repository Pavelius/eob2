#include "nameable.h"
#include "variant.h"

#pragma once

struct randomizeri : nameable {
	variants		chance;
	variant			random() const;
	static int		total(const variants& elements);
};

int random_total(const variants& elements);

variant	random_variant(const variants& elements);
variant	random_equal(const variants& elements);
variant single(variant v);