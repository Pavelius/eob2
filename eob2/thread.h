#pragma once

namespace io {
class thread {
	unsigned long s;
public:
	typedef void (*fnroutine)(void* v);
	constexpr thread() : s(0xFFFFFFFF) {}
	thread(fnroutine proc, void* param = 0) : s(0xFFFFFFFF) { start(proc, param); }
	~thread();
	void start(fnroutine proc, void* param = 0);
	void resume();
	void suspend();
	void join();
};
}
