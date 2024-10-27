#pragma once

enum featn : unsigned char;

enum damagen : unsigned char {
	Bludgeon, Slashing, Piercing,
	Magic, Fire, Cold, Acid, Shock, Poison,
};
struct damagei {
	const char*	id;
	featn		resist, immunity, vulnerable;
};
