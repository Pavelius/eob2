#pragma once

namespace io {
class thread {
	unsigned long s;
public:
	typedef void (*fnroutine)(void* param);
	constexpr thread() : s(0xFFFFFFFF) {}
	thread(fnroutine proc, void* param = 0) : s(0xFFFFFFFF) { start(proc, param); }
	~thread() { close(); }
	constexpr explicit operator bool() const { return s != 0xFFFFFFFF; }
	void close();
	void join();
	void resume();
	void start(fnroutine proc, void* param = 0);
	void suspend();
};
}
