#include "color.h"
#include "draw.h"
#include "io_stream.h"
#include "math.h"
#include "seqlink.h"
#include "slice.h"
#include "stringbuilder.h"

#ifndef __GNUC__
#pragma optimize("t", on)
#endif

extern "C" void	exit(int code);
extern "C" void* malloc(unsigned long size);
extern "C" void* realloc(void *ptr, unsigned long size);
extern "C" void	free(void* pointer);

using namespace draw;

// Default theme colors
color				colors::active;
color				colors::text;
color				colors::border;
color				colors::h1;
color				colors::h2;
color				colors::h3;
color				colors::special;
color				colors::tips::text;
color				colors::tips::back;
// Color context and font context
fnevent			draw::domodal, draw::pbackground, draw::pfinish, draw::ptips;
fnevent			draw::pbeforemodal, draw::pleavemodal, draw::psetfocus;
unsigned char  draw::alpha = 255;
color				draw::fore;
color				draw::fore_stroke;
int				draw::width, draw::height, draw::dialog_width = 500;
int		      draw::fsize = 32;
bool				draw::text_clipped, draw::control_hilited;
const sprite*	draw::font;
double			draw::linw = 1.0;
color*			draw::palt;
rect				draw::clipping;
hoti				draw::hot;
const void*		draw::hilite_object;
point				draw::hilite_position;
int				draw::hilite_size;
// Hot keys and menus
rect				sys_static_area;
// Locale draw variables
static draw::surface default_surface;
draw::surface*	draw::canvas = &default_surface;
point				draw::caret, draw::camera, draw::tips_caret, draw::tips_size;
bool			   line_antialiasing = true;
// Drag
point				draw::dragmouse;
// Metrics
sprite*			metrics::font;
sprite*			metrics::h1;
sprite*			metrics::h2;
sprite*			metrics::h3;
sprite*			metrics::small;
sprite*			metrics::icons;
int				metrics::padding = 2, metrics::border = 4;
//
static bool		break_modal;
static long		break_result;
static fnevent	next_scene;
awindowi			draw::awindow = {-1, -1, 800, 600, 160, WFMinmax | WFResize};

static void correct(int& x1, int& y1, int& x2, int& y2) {
	if(x1 > x2)
		iswap(x1, x2);
	if(y1 > y2)
		iswap(y1, y2);
}

static bool correct(int& x1, int& y1, int& x2, int& y2, const rect& clip, bool include_edge = true) {
	correct(x1, y1, x2, y2);
	if(x2 < clip.x1 || x1 >= clip.x2 || y2 < clip.y1 || y1 >= clip.y2)
		return false;
	if(x1 < clip.x1)
		x1 = clip.x1;
	if(y1 < clip.y1)
		y1 = clip.y1;
	if(include_edge) {
		if(x2 > clip.x2)
			x2 = clip.x2;
		if(y2 > clip.y2)
			y2 = clip.y2;
	} else {
		if(x2 >= clip.x2)
			x2 = clip.x2 - 1;
		if(y2 >= clip.y2)
			y2 = clip.y2 - 1;
	}
	return true;
}

static void set32(color* p, unsigned count) {
	color* p2;
	switch(alpha) {
	case 0:
		break;
	case 255:
#ifdef _MSC_VER
		_asm {
			mov edi, p
			mov eax, fore
			mov ecx, count
			rep stosd
		}
#else
		p2 = p + count;
		while(p < p2)
			*p++ = fore;
#endif
		break;
	case 128:
		p2 = p + count;
		while(p < p2) {
			p->r = (p->r + fore.r) >> 1;
			p->g = (p->g + fore.g) >> 1;
			p->b = (p->b + fore.b) >> 1;
			p++;
		}
		break;
	default:
		p2 = p + count;
		while(p < p2) {
			p->r = (p->r * (255 - draw::alpha) + fore.r * draw::alpha) >> 8;
			p->g = (p->g * (255 - draw::alpha) + fore.g * draw::alpha) >> 8;
			p->b = (p->b * (255 - draw::alpha) + fore.b * draw::alpha) >> 8;
			p++;
		}
		break;
	}
}

static void set32(color* p) {
	switch(alpha) {
	case 0:
		break;
	case 255:
		p[0] = fore;
		break;
	case 128:
		p->r = (p->r + fore.r) >> 1;
		p->g = (p->g + fore.g) >> 1;
		p->b = (p->b + fore.b) >> 1;
		break;
	default:
		p->r = (p->r * (255 - draw::alpha) + fore.r * draw::alpha) >> 8;
		p->g = (p->g * (255 - draw::alpha) + fore.g * draw::alpha) >> 8;
		p->b = (p->b * (255 - draw::alpha) + fore.b * draw::alpha) >> 8;
		break;
	}
}

static void set32x(unsigned char* d, int d_scan, int width, int height) {
	while(height-- > 0) {
		set32((color*)d, width);
		d += d_scan;
	}
}

static void set32h(unsigned char* d, int height) {
	while(height-- > 0) {
		set32((color*)d);
		d += canvas->scanline;
	}
}

static void raw832(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height, const color* pallette) {
	const int cbd = 4;
	while(height-- > 0) {
		unsigned char* p1 = d;
		unsigned char* sb = s;
		unsigned char* se = s + width;
		while(sb < se) {
			*((color*)p1) = pallette[*sb++];
			p1 += cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

static void raw832m(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height, const color* pallette) {
	const int cbd = 4;
	while(height-- > 0) {
		unsigned char* p1 = d;
		unsigned char* sb = s;
		unsigned char* se = s + width;
		while(sb < se) {
			*((color*)p1) = pallette[*sb++];
			p1 -= cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

static void raw32(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
	const int cbs = 3;
	const int cbd = 4;
	if(width <= 0)
		return;
	if(!alpha)
		return;
	if(alpha == 255) {
		while(height-- > 0) {
			unsigned char* sb = s;
			unsigned char* se = s + width * cbs;
			unsigned char* p1 = d;
			while(sb < se) {
				p1[0] = sb[0];
				p1[1] = sb[1];
				p1[2] = sb[2];
				sb += cbs;
				p1 += cbd;
			}
			s += s_scan;
			d += d_scan;
		}
	} else {
		while(height-- > 0) {
			unsigned char* sb = s;
			unsigned char* se = s + width * cbs;
			unsigned char* p1 = d;
			while(sb < se) {
				p1[0] = (((int)p1[0] * (255 - alpha)) + ((sb[0]) * (alpha))) >> 8;
				p1[1] = (((int)p1[1] * (255 - alpha)) + ((sb[1]) * (alpha))) >> 8;
				p1[2] = (((int)p1[2] * (255 - alpha)) + ((sb[2]) * (alpha))) >> 8;
				sb += cbs;
				p1 += cbd;
			}
			s += s_scan;
			d += d_scan;
		}
	}
}

static void raw32m(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
	const int cbs = 3;
	const int cbd = 4;
	if(width <= 0)
		return;
	while(height-- > 0) {
		unsigned char* sb = s;
		unsigned char* se = s + width * cbs;
		unsigned char* p1 = d;
		while(sb < se) {
			p1[0] = sb[0];
			p1[1] = sb[1];
			p1[2] = sb[2];
			sb += cbs;
			p1 -= cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

// (00) end of line
// (01 - 7F) draw count of pixels
// (80, XX, AA) draw count of XX bytes of alpha AA pixels
// (81 - 9F, AA) draw count of (B-0xC0) bytes of alpha AA pixels
// (A0, XX) skip count of XX pixels
// A1 - FF skip count of (b-0xB0) pixels
// each pixel has b,g,r value
static void rle32(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha) {
	const int cbs = 3;
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			d = p1;
			if(--h == 0)
				break;
		} else if(c <= 0x9F) {
			unsigned char ap, cb;
			// count
			if(c <= 0x7F) {
				cb = c;
				ap = 0xFF;
			} else if(c == 0x80) {
				cb = *s++;
				ap = *s++;
			} else {
				cb = c - 0x80;
				ap = *s++;
			}
			// clip left invisible part
			if(d + cb * cbd <= s1 || d > s2) {
				d += cb * cbd;
				s += cb * cbs;
				continue;
			} else if(d < s1) {
				unsigned char sk = (s1 - d) / cbd;
				d += sk * cbd;
				s += sk * cbs;
				cb -= sk;
			}
			// visible part
			if(ap == 0xFF && alpha == 0xFF) {
				do {
					if(d >= s2)
						break;
					d[0] = s[0];
					d[1] = s[1];
					d[2] = s[2];
					s += cbs;
					d += cbd;
				} while(--cb);
			} else {
				ap = (ap * alpha) / 256;
				do {
					if(d >= s2)
						break;
					d[0] = (((int)d[0] * (255 - ap)) + ((s[0]) * (ap))) >> 8;
					d[1] = (((int)d[1] * (255 - ap)) + ((s[1]) * (ap))) >> 8;
					d[2] = (((int)d[2] * (255 - ap)) + ((s[2]) * (ap))) >> 8;
					s += cbs;
					d += cbd;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				s += cb * cbs;
				d += cb * cbd;
			}
		} else {
			if(c == 0xA0)
				d += (*s++) * cbd;
			else
				d += (c - 0xA0) * cbd;
		}
	}
}

static void rle32m(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha) {
	const int cbs = 3;
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			d = p1;
			if(--h == 0)
				break;
		} else if(c <= 0x9F) {
			unsigned char ap, cb;
			// count
			if(c <= 0x7F) {
				cb = c;
				ap = 0xFF;
			} else if(c == 0x80) {
				cb = *s++;
				ap = *s++;
			} else {
				cb = c - 0x80;
				ap = *s++;
			}
			// clip left invisible part
			if(d - (cb * cbd) >= s2 || d < s1) {
				s += cb * cbs;
				d -= cb * cbd;
				continue;
			} else if(d >= s2) {
				unsigned char sk = 1 + (d - s2) / cbd;
				d -= sk * cbd;
				s += sk * cbs;
				cb -= sk;
				if(!cb)
					continue;
			}
			// visible part
			if(ap == 0xFF && alpha == 0xFF) {
				// no alpha or modification
				do {
					if(d < s1)
						break;
					d[0] = s[0];
					d[1] = s[1];
					d[2] = s[2];
					s += cbs;
					d -= cbd;
				} while(--cb);
			} else {
				// alpha channel
				ap = (ap * alpha) / 256;
				do {
					if(d < s1)
						break;
					d[0] = (((int)d[0] * (255 - ap)) + ((s[0]) * (ap))) >> 8;
					d[1] = (((int)d[1] * (255 - ap)) + ((s[1]) * (ap))) >> 8;
					d[2] = (((int)d[2] * (255 - ap)) + ((s[2]) * (ap))) >> 8;
					d -= cbd;
					s += cbs;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				d -= cb * cbd;
				s += cb * cbs;
			}
		} else {
			if(c == 0xA0)
				d -= (*s++) * cbd;
			else
				d -= (c - 0xA0) * cbd;
		}
	}
}

static void rle832(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha, const color* pallette) {
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			if(--h == 0)
				break;
			d = p1;
		} else if(c <= 0x9F) {
			unsigned char ap = alpha, cb;
			bool need_correct_s = false;
			// count
			if(c <= 0x7F) {
				need_correct_s = true;
				cb = c;
			} else if(c == 0x80) {
				cb = *s++;
				ap >>= 1;
			} else {
				cb = c - 0x80;
				ap >>= 1;
			}
			// clip left invisible part
			if(d + cb * cbd <= s1 || d > s2) {
				d += cb * cbd;
				if(need_correct_s)
					s += cb;
				continue;
			} else if(d < s1) {
				unsigned char sk = (s1 - d) / cbd;
				d += sk * cbd;
				if(need_correct_s)
					s += sk;
				cb -= sk;
			}
			// visible part
			if(ap == alpha) {
				if(ap == 0xFF) {
					do {
						if(d >= s2)
							break;
						*((color*)d) = pallette[*s++];
						d += cbd;
					} while(--cb);
				} else {
					do {
						if(d >= s2)
							break;
						unsigned char* s1 = (unsigned char*)&pallette[*s++];
						d[0] = (((int)d[0] * (255 - ap)) + ((s1[0]) * (ap))) >> 8;
						d[1] = (((int)d[1] * (255 - ap)) + ((s1[1]) * (ap))) >> 8;
						d[2] = (((int)d[2] * (255 - ap)) + ((s1[2]) * (ap))) >> 8;
						d += cbd;
					} while(--cb);
				}
			} else if(ap == 0x7F) {
				do {
					if(d >= s2)
						break;
					d[0] >>= 1;
					d[1] >>= 1;
					d[2] >>= 1;
					d += cbd;
				} while(--cb);
			} else {
				ap = 255 - ap;
				do {
					if(d >= s2)
						break;
					d[0] = (((int)d[0] * ap)) >> 8;
					d[1] = (((int)d[1] * ap)) >> 8;
					d[2] = (((int)d[2] * ap)) >> 8;
					d += cbd;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				if(need_correct_s)
					s += cb;
				d += cb * cbd;
			}
		} else {
			if(c == 0xA0)
				d += (*s++) * cbd;
			else
				d += (c - 0xA0) * cbd;
		}
	}
}

static void rectfx(int x1, int y1, int x2, int y2) {
	if(x1 < clipping.x1)
		x1 = clipping.x1;
	if(x2 > clipping.x2) {
		x2 = clipping.x2;
		if(x1 == x2)
			return;
	}
	if(x1 > x2)
		return;
	if(y1 < clipping.y1)
		y1 = clipping.y1;
	if(y2 > clipping.y2) {
		y2 = clipping.y2;
		if(y1 == y2)
			return;
	}
	if(y1 > y2)
		return;
	set32x(canvas->ptr(x1, y1), canvas->scanline, x2 - x1 + 1, y2 - y1 + 1);
}

static void alc832(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha) {
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	auto fr = fore.b;
	auto fg = fore.g;
	auto fb = fore.r;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			if(--h == 0)
				break;
			d = p1;
		} else if(c <= 0x9F) {
			unsigned char cb;
			bool need_correct_s = false;
			// count
			if(c <= 0x9F) {
				need_correct_s = true;
				cb = c;
			} else if(c == 0xA0)
				cb = *s++;
			else
				cb = c - 0xA0;
			// clip left invisible part
			if(d + cb * cbd <= s1 || d > s2) {
				d += cb * cbd;
				if(need_correct_s)
					s += cb;
				continue;
			} else if(d < s1) {
				unsigned char sk = (s1 - d) / cbd;
				d += sk * cbd;
				if(need_correct_s)
					s += sk;
				cb -= sk;
			}
			// visible part
			do {
				if(d >= s2)
					break;
				auto ap = *s++;
				if(ap == 255)
					*((color*)d) = fore;
				else {
					d[0] = (((int)d[0] * (256 - ap)) + ((fr) * (ap))) >> 8;
					d[1] = (((int)d[1] * (256 - ap)) + ((fg) * (ap))) >> 8;
					d[2] = (((int)d[2] * (256 - ap)) + ((fb) * (ap))) >> 8;
				}
				d += cbd;
			} while(--cb);
			// right clip part
			if(cb) {
				if(need_correct_s)
					s += cb;
				d += cb * cbd;
			}
		} else if(c == 0xA0)
			d += (*s++) * cbd;
		else
			d += (c - 0xA0) * cbd;
	}
}

static void rle832m(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha, const color* pallette) {
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			d = p1;
			if(--h == 0)
				break;
		} else if(c <= 0x9F) {
			unsigned char ap = alpha, cb;
			bool need_correct_s = false;
			// count
			if(c <= 0x7F) {
				need_correct_s = true;
				cb = c;
			} else if(c == 0x80) {
				cb = *s++;
				ap >>= 1;
			} else {
				cb = c - 0x80;
				ap >>= 1;
			}
			// clip left invisible part
			if(d - (cb * cbd) >= s2 || d < s1) {
				d -= cb * cbd;
				if(need_correct_s)
					s += cb;
				continue;
			} else if(d >= s2) {
				unsigned char sk = (d - s2) / cbd;
				d -= sk * cbd;
				if(need_correct_s)
					s += sk;
				cb -= sk;
			}
			// visible part
			if(ap == alpha) {
				if(ap == 0xFF) {
					do {
						if(d < s1)
							break;
						*((color*)d) = pallette[*s++];
						d -= cbd;
					} while(--cb);
				} else {
					do {
						if(d < s1)
							break;
						unsigned char* s1 = (unsigned char*)&pallette[*s++];
						d[0] = (((int)d[0] * (255 - ap)) + ((s1[0]) * (ap))) >> 8;
						d[1] = (((int)d[1] * (255 - ap)) + ((s1[1]) * (ap))) >> 8;
						d[2] = (((int)d[2] * (255 - ap)) + ((s1[2]) * (ap))) >> 8;
						d -= cbd;
					} while(--cb);
				}
			} else if(ap == 0x7F) {
				do {
					if(d < s1)
						break;
					d[0] >>= 1;
					d[1] >>= 1;
					d[2] >>= 1;
					d -= cbd;
				} while(--cb);
			} else {
				ap = 255 - ap;
				do {
					if(d < s1)
						break;
					d[0] = (((int)d[0] * ap)) >> 8;
					d[1] = (((int)d[1] * ap)) >> 8;
					d[2] = (((int)d[2] * ap)) >> 8;
					d -= cbd;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				if(need_correct_s)
					s += cb;
				d -= cb * cbd;
			}
		} else {
			if(c == 0xA0)
				d -= (*s++) * cbd;
			else
				d -= (c - 0xA0) * cbd;
		}
	}
}

static void alc32(unsigned char* d, int d_scan, const unsigned char* s, int height, const unsigned char* clip_x1, const unsigned char* clip_x2, color c1, bool italic) {
	const int cbs = 3;
	const int cbd = 4;
	unsigned char* p = d;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			d += d_scan;
			clip_x1 += d_scan;
			clip_x2 += d_scan;
			if(italic && (height & 1) != 0)
				d -= cbd;
			p = d;
			if(--height == 0)
				break;
		} else if(c <= 0x7F) {
			// clip left invisible part
			if(p + (c * cbd) <= clip_x1 || p > clip_x2) {
				p += c * cbd;
				s += c * cbs;
				continue;
			} else if(p < clip_x1) {
				unsigned char sk = (clip_x1 - p) / cbd;
				p += sk * cbd;
				s += sk * cbs;
				c -= sk;
			}
			// visible part
			if(alpha == 255) {
				do {
					if(p >= clip_x2)
						break;
					p[0] = ((p[0] * (255 - s[0])) + (c1.b * (s[0]))) >> 8;
					p[1] = ((p[1] * (255 - s[1])) + (c1.g * (s[1]))) >> 8;
					p[2] = ((p[2] * (255 - s[2])) + (c1.r * (s[2]))) >> 8;
					p += cbd;
					s += cbs;
				} while(--c);
			} else {
				do {
					if(p >= clip_x2)
						break;
					unsigned char b = (s[0] * alpha) >> 8;
					unsigned char g = (s[1] * alpha) >> 8;
					unsigned char r = (s[2] * alpha) >> 8;
					p[0] = ((p[0] * (255 - b)) + (c1.b * b)) >> 8;
					p[1] = ((p[1] * (255 - g)) + (c1.g * g)) >> 8;
					p[2] = ((p[2] * (255 - r)) + (c1.r * r)) >> 8;
					p += cbd;
					s += cbs;
				} while(--c);
			}
			// right clip part
			if(c) {
				p += c * cbd;
				s += c * cbs;
			}
		} else {
			if(c == 0x80)
				p += (*s++) * cbd;
			else
				p += (c - 0x80) * cbd;
		}
	}
}

static unsigned char* skip_v3(unsigned char* s, int h) {
	const int cbs = 1;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x9F) {
			if(c <= 0x7F)
				s += c * cbs;
			else {
				if(c == 0x80)
					c = *s++;
				else
					c -= 0x80;
				s++;
				s += c * cbs;
			}
		} else if(c == 0xA0)
			s++;
	}
}

static unsigned char* skip_v4(unsigned char* s, int h) {
	const int cbs = 1;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x9F)
			s += c * cbs;
		else if(c == 0xA0)
			s++;
	}
}

static unsigned char* skip_rle32(unsigned char* s, int h) {
	const int cbs = 3;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x9F) {
			if(c <= 0x7F)
				s += c * cbs;
			else {
				if(c == 0x80)
					c = *s++;
				else
					c -= 0x80;
				s++;
				s += c * cbs;
			}
		} else if(c == 0xA0)
			s++;
	}
}

static unsigned char* skip_alc(unsigned char* s, int h) {
	const int cbs = 3;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x7F)
			s += c * cbs;
		else if(c == 0x80)
			s++;
	}
}

static void scale_line_32(unsigned char* dst, unsigned char* src, int sw, int tw) {
	const int cbd = 4;
	int NumPixels = tw;
	int IntPart = (sw / tw) * cbd;
	int FractPart = sw % tw;
	int E = 0;
	while(NumPixels-- > 0) {
		*((unsigned*)dst) = *((unsigned*)src);
		dst += cbd;
		src += IntPart;
		E += FractPart;
		if(E >= tw) {
			E -= tw;
			src += cbd;
		}
	}
}

static bool corrects(const surface& dc, int& x, int& y, int& width, int& height) {
	if(x + width > dc.width)
		width = dc.width - x;
	if(y + height > dc.height)
		height = dc.height - y;
	if(width <= 0 || height <= 0)
		return false;
	return true;
}

static bool correctb(int& x1, int& y1, int& w, int& h, int& ox) {
	int x11 = x1;
	int x2 = x1 + w;
	int y2 = y1 + h;
	if(!correct(x1, y1, x2, y2, draw::clipping))
		return false;
	ox = x1 - x11;
	w = x2 - x1;
	h = y2 - y1;
	return true;
}

static void scale32(
	unsigned char* d, int d_scan, int d_width, int d_height,
	unsigned char* s, int s_scan, int s_width, int s_height) {
	if(!d_width || !d_height || !s_width || !s_height)
		return;
	const int cbd = 4;
	int NumPixels = d_height;
	int IntPart = (s_height / d_height) * s_scan;
	int FractPart = s_height % d_height;
	int E = 0;
	unsigned char* PrevSource = 0;
	while(NumPixels-- > 0) {
		if(s == PrevSource)
			memcpy(d, d - d_scan, d_width * cbd);
		else {
			scale_line_32(d, s, s_width, d_width);
			PrevSource = s;
		}
		d += d_scan;
		s += IntPart;
		E += FractPart;
		if(E >= d_height) {
			E -= d_height;
			s += s_scan;
		}
	}
}

static void cpy(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height, int bytes_per_pixel) {
	if(height <= 0 || width <= 0)
		return;
	int width_bytes = width * bytes_per_pixel;
	do {
		memcpy(d, s, width_bytes);
		s += s_scan;
		d += d_scan;
	} while(--height);
}

static void cpy32a(unsigned char* d, int d_scan,
	const unsigned char* s, int s_scan,
	int width, int height, unsigned char ap) {
	if(height <= 0 || width <= 0)
		return;
	do {
		auto sb = (color*)s;
		auto pe = sb + width;
		for(auto d2 = (color*)d; sb < pe; sb++) {
			d2->r = (((int)d2->r * (255 - ap)) + ((sb->r) * (ap))) >> 8;
			d2->g = (((int)d2->g * (255 - ap)) + ((sb->g) * (ap))) >> 8;
			d2->b = (((int)d2->b * (255 - ap)) + ((sb->b) * (ap))) >> 8;
			d2++;
		}
		s += s_scan;
		d += d_scan;
	} while(--height);
}

static void cpy32t(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
	if(height <= 0 || width <= 0)
		return;
	do {
		color* d2 = (color*)d;
		color* sb = (color*)s;
		color* se = sb + width;
		while(sb < se) {
			if(sb->a == 0xFF) {
				d2++;
				sb++;
			} else
				*d2++ = *sb++;
		}
		s += s_scan;
		d += d_scan;
	} while(--height);
}

//static void cpy32t(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
//	if(height <= 0 || width <= 0)
//		return;
//	do {
//		color* d2 = (color*)d;
//		color* sb = (color*)s;
//		color* se = sb + width;
//		while(sb < se) {
//			if(!sb->a) {
//				d2++;
//				sb++;
//			} else if(sb->a == 0xFF)
//				*d2++ = *sb++;
//			else {
//				auto ap = sb->a;
//				d2->r = (((int)d2->r * (255 - ap)) + ((sb->r) * (ap))) >> 8;
//				d2->g = (((int)d2->g * (255 - ap)) + ((sb->g) * (ap))) >> 8;
//				d2->b = (((int)d2->b * (255 - ap)) + ((sb->b) * (ap))) >> 8;
//				d2++; sb++;
//			}
//		}
//		s += s_scan;
//		d += d_scan;
//	} while(--height);
//}

int draw::getbpp() {
	return canvas ? canvas->bpp : 1;
}

int draw::getwidth() {
	return canvas ? canvas->width : 0;
}

int draw::getheight() {
	return canvas ? canvas->height : 0;
}

unsigned char* draw::ptr(int x, int y) {
	return canvas ? (canvas->bits + y * canvas->scanline + x * canvas->bpp / 8) : 0;
}

void draw::pixel(int x, int y) {
	if(x >= clipping.x1 && x < clipping.x2 && y >= clipping.y1 && y < clipping.y2)
		*((color*)((char*)canvas->bits + y * canvas->scanline + x * 4)) = fore;
}

void draw::pixel(int x, int y, unsigned char a) {
	if(x < clipping.x1 || x >= clipping.x2 || y < clipping.y1 || y >= clipping.y2 || a == 0xFF)
		return;
	color* p = (color*)ptr(x, y);
	if(a == 0)
		*p = fore;
	else {
		p->b = (((unsigned)p->b * (a)) + (fore.b * (255 - a))) >> 8;
		p->g = (((unsigned)p->g * (a)) + (fore.g * (255 - a))) >> 8;
		p->r = (((unsigned)p->r * (a)) + (fore.r * (255 - a))) >> 8;
		p->a = 0;
	}
}

static void linew(int x1, int y1, double wd) {
	int x0 = caret.x, y0 = caret.y;
	int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx - dy, e2, x2, y2; /* error value e_xy */
	float ed = dx + dy == 0 ? 1 : sqrt((float)dx * dx + (float)dy * dy);
	for(wd = (wd + 1) / 2; ; ) {                                    /* pixel loop */
		draw::pixel(x0, y0, (unsigned char)imax((int)0, (int)(255 * (iabs(err - dx + dy) / ed - wd + 1))));
		e2 = err; x2 = x0;
		if(2 * e2 >= -dx) {                                            /* x step */
			for(e2 += dy, y2 = y0; e2 < ed * wd && (y1 != y2 || dx > dy); e2 += dx)
				draw::pixel(x0, y2 += sy, (unsigned char)imax((int)0, (int)(255 * (iabs(e2) / ed - wd + 1))));
			if(x0 == x1) break;
			e2 = err; err -= dy; x0 += sx;
		}
		if(2 * e2 <= dy) {                                             /* y step */
			for(e2 = dx - e2; e2 < ed * wd && (x1 != x2 || dx < dy); e2 += dy)
				draw::pixel(x2 += sx, y0, (unsigned char)imax((int)0, (int)(255 * (iabs(e2) / ed - wd + 1))));
			if(y0 == y1) break;
			err += dx; y0 += sy;
		}
	}
}

void draw::line(int xt, int yt) {
	int x0 = caret.x, y0 = caret.y, x1 = xt, y1 = yt;
	if(linw != 1.0)
		linew(x1, y1, linw);
	else if(caret.x == x1) {
		if(correct(x0, y0, x1, y1, clipping, false))
			set32h(canvas->ptr(x0, y0), y1 - y0 + 1);
	} else if(caret.y == y1) {
		if(correct(x0, y0, x1, y1, clipping, false))
			set32x(canvas->ptr(x0, y0), canvas->scanline, x1 - x0 + 1, 1);
	} else if(line_antialiasing) {
		int x0 = caret.x, y0 = caret.y;
		int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = dx - dy, e2, x2; // error value e_xy
		int ed = dx + dy == 0 ? 1 : isqrt(dx * dx + dy * dy);
		for(;;) {
			pixel(x0, y0, alpha * iabs(err - dx + dy) / ed);
			e2 = err; x2 = x0;
			if(2 * e2 >= -dx) {// x step
				if(x0 == x1)
					break;
				if(e2 + dy < ed)
					pixel(x0, y0 + sy, alpha * (e2 + dy) / ed);
				err -= dy; x0 += sx;
			}
			if(2 * e2 <= dy) {// y step
				if(y0 == y1)
					break;
				if(dx - e2 < ed)
					pixel(x2 + sx, y0, alpha * (dx - e2) / ed);
				err += dx; y0 += sy;
			}
		}
	} else {
		int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = dx + dy, e2;
		for(;;) {
			pixel(x0, y0, alpha);
			e2 = 2 * err;
			if(e2 >= dy) {
				if(x0 == x1) break;
				err += dy; x0 += sx;
			}
			if(e2 <= dx) {
				if(y0 == y1) break;
				err += dx; y0 += sy;
			}
		}
	}
	caret.x = xt;
	caret.y = yt;
}

void draw::linet(int x1, int y1) {
	int x0 = caret.x, y0 = caret.y;
	int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2; // error value e_xy
	for(;;) {
		if((x0 + y0) & 1)
			pixel(x0, y0);
		if(x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if(e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
		if(e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
	}
	caret.x = x1;
	caret.y = y1;
}

void draw::rectb() {
	line(caret.x + (width - 1), caret.y);
	line(caret.x, caret.y + (height - 1));
	line(caret.x - (width - 1), caret.y);
	line(caret.x, caret.y - (height - 1));
}

void draw::rectb3d() {
	auto push_caret = caret;
	auto push_alpha = alpha;
	line(caret.x + width, caret.y);
	line(caret.x, caret.y + height);
	line(caret.x - width, caret.y);
	line(caret.x, caret.y - height);
	setoffset(1, 1);
	alpha = alpha / 2;
	line(caret.x + width, caret.y);
	line(caret.x, caret.y + height);
	alpha = alpha / 2;
	line(caret.x - width, caret.y);
	line(caret.x, caret.y - height);
	alpha = push_alpha;
	caret = push_caret;
}

void draw::rectf() {
	rectpush push;
	int x1 = caret.x, y1 = caret.y, x2 = caret.x + width, y2 = caret.y + height;
	if(correct(x1, y1, x2, y2, clipping))
		set32x(ptr(x1, y1), canvas->scanline, x2 - x1, y2 - y1);
}

static void rectfpt(int xc1, int yc1, int xc2, int yc2, int r) {
	if(xc1 - r >= clipping.x2 || xc2 + r < clipping.x1 || yc1 - r >= clipping.y2 || yc2 + r < clipping.y1)
		return;
	xc1 += r; xc2 -= r;
	yc1 += r; yc2 -= r + 1;
	int x = -r, y = 0, err = 2 - 2 * r, yp = -1000;
	do {
		if(yp != y) {
			yp = y;
			//rectf({xc1 + x, yc1 - y, xc1, yc1 - y + 1});
			//rectf({xc2 - x, yc1 - y, xc2, yc1 - y + 1});
			if(y != 0) {
				//rectf({xc1 + x, yc2 + y, xc1, yc2 + y + 1});
				//rectf({xc2 - x, yc2 + y, xc2, yc2 + y + 1});
			}
		}
		r = err;
		if(r <= y)
			err += ++y * 2 + 1;
		if(r > x || err > y)
			err += ++x * 2 + 1;
	} while(x < 0);
}

void draw::rectfe(rect rc, int r) {
	rectfpt(rc.x1, rc.y1, rc.x2, rc.y2, r);
	//rectf({rc.x1 + r, rc.y1, rc.x2 - r, rc.y1 + r});
	//rectf({rc.x1, rc.y1 + r + 1, rc.x1 + r, rc.y2 - r});
	//rectf({rc.x2 - r, rc.y1 + r + 1, rc.x2, rc.y2 - r});
	//rectf({rc.x1 + r, rc.y2 - r, rc.x2 - r, rc.y2});
	//rectf({rc.x1 + r, rc.y1 + r, rc.x2 - r, rc.y2 - r});
}

void draw::setpos(int x, int y) {
	caret.x = x;
	caret.y = y;
}

void draw::setpos(int x, int y, int w, int h) {
	caret.x = x;
	caret.y = y;
	width = w;
	height = h;
}

void draw::setoffset(int x, int y) {
	caret.x += x; width -= x * 2;
	caret.y += y; height -= y * 2;
}

void draw::rectx() {
	linet(caret.x, caret.y + height);
	linet(caret.x + width, caret.y);
	linet(caret.x, caret.y - height);
	linet(caret.x - width, caret.y);
}

void draw::rectfocus() {
	rectpush push;
	setoffset(1, 1);
	rectx();
}

void draw::gradv(const color c1, const color c2, int skip) {
	if(!canvas)
		return;
	if(skip > height)
		skip = height;
	float k3 = (float)height;
	if(!k3)
		return;
	int y0 = caret.y;
	int y2 = caret.y + height;
	auto pf = fore;
	for(int y1 = caret.y + skip; y1 < y2; y1++) {
		auto k2 = (float)(y1 - y0) / k3;
		auto k1 = 1.00f - k2;
		fore.r = (unsigned char)(c1.r * k1 + c2.r * k2);
		fore.g = (unsigned char)(c1.g * k1 + c2.g * k2);
		fore.b = (unsigned char)(c1.b * k1 + c2.b * k2);
		if(y1 >= clipping.y2 || y1 < clipping.y1)
			continue;
		set32((color*)canvas->ptr(caret.x, y1), width);
	}
	fore = pf;
}

void draw::gradh(const color c1, const color c2, int skip) {
	if(!canvas)
		return;
	if(skip > width)
		skip = width;
	float k3 = (float)width;
	if(!k3)
		return;
	int x0 = caret.x;
	int x2 = caret.x + width;
	auto pf = fore;
	for(int x1 = caret.x + skip; x1 < x2; x1++) {
		auto k2 = (float)(x0 - x1) / k3;
		auto k1 = 1.00f - k2;
		fore.r = (unsigned char)(c1.r * k1 + c2.r * k2);
		fore.g = (unsigned char)(c1.g * k1 + c2.g * k2);
		fore.b = (unsigned char)(c1.b * k1 + c2.b * k2);
		if(x1 >= clipping.x2 || x1 < clipping.x1)
			continue;
		set32h(canvas->ptr(x1, caret.y), height);
	}
	fore = pf;
}

static void set32fl(int x, int y, int w) {
	if(y < clipping.y1 || y >= clipping.y2)
		return;
	auto x2 = x + w;
	if(x < clipping.x1)
		x = clipping.x1;
	if(x2 > clipping.x2)
		x2 = clipping.x2;
	w = x2 - x;
	if(w > 0)
		set32((color*)canvas->ptr(x, y), w);
}

void draw::circlef(int r) {
	int xm = caret.x, ym = caret.y;
	if(xm - r >= clipping.x2 || xm + r < clipping.x1 || ym - r >= clipping.y2 || ym + r < clipping.y1)
		return;
	int x = -r, y = 0, err = 2 - 2 * r, y2 = -1000;
	do {
		if(y2 != y) {
			y2 = y;
			set32fl(xm + x, ym + y, x * -2);
			if(y != 0)
				set32fl(xm + x, ym - y, x * -2);
		}
		r = err;
		if(r <= y)
			err += ++y * 2 + 1;
		if(r > x || err > y)
			err += ++x * 2 + 1;
	} while(x < 0);
}

void draw::circle(int r) {
	int xm = caret.x, ym = caret.y;
	int x = r, y = 0; // II. quadrant from bottom left to top right
	int x2, e2, err = 2 - 2 * r; // error of 1.step
	r = 1 - err;
	for(;;) {
		int i = 255 * iabs(err + 2 * (x + y) - 2) / r; // get blend value of pixel
		pixel(xm + x, ym - y, i); // I. Quadrant
		pixel(xm + y, ym + x, i); // II. Quadrant
		pixel(xm - x, ym + y, i); // III. Quadrant
		pixel(xm - y, ym - x, i); // IV. Quadrant
		if(x == 0)
			break;
		e2 = err; x2 = x; // remember values
		if(err > y) { // x step
			int i = 255 * (err + 2 * x - 1) / r; // outward pixel
			if(i < 255) {
				pixel(xm + x, ym - y + 1, i);
				pixel(xm + y - 1, ym + x, i);
				pixel(xm - x, ym + y - 1, i);
				pixel(xm - y + 1, ym - x, i);
			}
			err -= --x * 2 - 1;
		}
		if(e2 <= x2--) { // y step
			int i = 255 * (1 - 2 * y - e2) / r;
			if(i < 255) {
				pixel(xm + x2, ym - y, i);
				pixel(xm + y, ym + x2, i);
				pixel(xm - x2, ym + y, i);
				pixel(xm - y, ym - x2, i);
			}
			err -= --y * 2 - 1;
		}
	}
}

void draw::fhexagon() {
	const double cos_30 = 0.86602540378;
	point points[6] = {
		{(short)(caret.x + fsize), caret.y},
		{(short)(caret.x + fsize / 2), (short)(caret.y + fsize * cos_30)},
		{(short)(caret.x - fsize / 2), (short)(caret.y + fsize * cos_30)},
		{(short)(caret.x - fsize), caret.y},
		{(short)(caret.x - fsize / 2), (short)(caret.y - fsize * cos_30)},
		{(short)(caret.x + fsize / 2), (short)(caret.y - fsize * cos_30)},
	};
	auto push_caret = caret;
	caret = points[0];
	for(auto i = 1; i < 6; i++)
		line(points[i].x, points[i].y);
	line(points[0].x, points[0].y);
	caret = push_caret;
}

void draw::hexagon() {
	const double cos_30 = 0.86602540378;
	point points[6] = {
		{(short)(caret.x + fsize * cos_30), (short)(caret.y - fsize / 2)},
		{(short)(caret.x + fsize * cos_30), (short)(caret.y + fsize / 2)},
		{(short)caret.x, (short)(caret.y + fsize)},
		{(short)(caret.x - fsize * cos_30), (short)(caret.y + fsize / 2)},
		{(short)(caret.x - fsize * cos_30), (short)(caret.y - fsize / 2)},
		{(short)caret.x, (short)(caret.y - fsize)},
	};
	auto push_caret = caret;
	caret = points[0];
	for(auto i = 1; i < 6; i++)
		line(points[i].x, points[i].y);
	line(points[0].x, points[0].y);
	caret = push_caret;
}

void draw::setclip(rect rcn) {
	rect rc = draw::clipping;
	if(rc.x1 < rcn.x1)
		rc.x1 = rcn.x1;
	if(rc.y1 < rcn.y1)
		rc.y1 = rcn.y1;
	if(rc.x2 > rcn.x2)
		rc.x2 = rcn.x2;
	if(rc.y2 > rcn.y2)
		rc.y2 = rcn.y2;
	draw::clipping = rc;
}

static void intersect_rect(rect& r1, const rect& r2) {
	if(!r1.intersect(r2))
		return;
	if(hot.mouse.in(r2)) {
		if(r2.y1 > r1.y1)
			r1.y1 = r2.y1;
		if(r2.x1 > r1.x1)
			r1.x1 = r2.x1;
		if(r2.y2 < r1.y2)
			r1.y2 = r2.y2;
		if(r2.x2 < r1.x2)
			r1.x2 = r2.x2;
	} else {
		if(hot.mouse.y > r2.y2 && r2.y2 > r1.y1)
			r1.y1 = r2.y2;
		else if(hot.mouse.y < r2.y1 && r2.y1 < r1.y2)
			r1.y2 = r2.y1;
		else if(hot.mouse.x > r2.x2 && r2.x2 > r1.x1)
			r1.x1 = r2.x2;
		else if(hot.mouse.x < r2.x1 && r2.x1 < r1.x2)
			r1.x2 = r2.x1;
	}
}

bool draw::ishilite(const rect& rc) {
	if(hot.key == InputNoUpdate)
		return false;
	intersect_rect(sys_static_area, rc);
	if(!hot.mouse.in(clipping))
		return false;
	if(hot.mouse.in(rc)) {
		hot.hilite = rc;
		return true;
	}
	return false;
}

int	draw::aligned(int x, int width, unsigned flags, int dx) {
	switch(flags & AlignMask) {
	case AlignRightBottom:
	case AlignRightCenter:
	case AlignRight: return x + width - dx;
	case AlignCenterBottom:
	case AlignCenterCenter:
	case AlignCenter: return x + (width - dx) / 2;
	default: return x;
	}
}

static int alignedh1(const char* string, unsigned state) {
	int ty;
	switch(state & AlignMask) {
	case AlignCenterCenter:
	case AlignRightCenter:
	case AlignLeftCenter:
		if(state & TextSingleLine)
			ty = draw::texth();
		else
			ty = draw::texth(string, width);
		return (height - ty) / 2;
	case AlignCenterBottom:
	case AlignRightBottom:
	case AlignLeftBottom:
		if(state & TextSingleLine)
			ty = draw::texth();
		else
			ty = draw::texth(string, width);
		return height - ty;
	default:
		return 0;
	}
}

int draw::alignedh(const rect& rc, const char* string, unsigned state) {
	int ty;
	switch(state & AlignMask) {
	case AlignCenterCenter:
	case AlignRightCenter:
	case AlignLeftCenter:
		if(state & TextSingleLine)
			ty = draw::texth();
		else
			ty = draw::texth(string, rc.width());
		return (rc.height() - ty) / 2;
	case AlignCenterBottom:
	case AlignRightBottom:
	case AlignLeftBottom:
		if(state & TextSingleLine)
			ty = draw::texth();
		else
			ty = draw::texth(string, rc.width());
		return rc.y2 - ty;
	default:
		return 0;
	}
}

const char* draw::skiptr(const char* string) {
	// skiping trail symbols
	for(; *string && *string == 0x20; string++);
	if(*string == 13) {
		string++;
		if(*string == 10)
			string++;
	} else if(*string == 10) {
		string++;
		if(*string == 13)
			string++;
	}
	return string;
}

int draw::textw(rect& rc, const char* string) {
	int w1 = rc.width();
	rc.y2 = rc.y1;
	rc.x2 = rc.x1;
	while(string[0]) {
		int c = textbc(string, w1);
		if(!c)
			break;
		int m = textw(string, c);
		if(rc.width() < m)
			rc.x2 = rc.x1 + m;
		rc.y2 += texth();
		string = skiptr(string + c);
	}
	return rc.height();
}

void draw::textas(const char* string) {
	int mx = 0, my = 0;
	while(string[0]) {
		int c = textbc(string, width);
		if(!c)
			break;
		int m = textw(string, c);
		if(mx < m)
			mx = m;
		my += texth();
		string = skiptr(string + c);
	}
	width = mx;
	height = my;
}

int draw::texth(const char* string, int width) {
	int dy = texth();
	int y1 = 0;
	while(*string) {
		int c = textbc(string, width);
		if(!c)
			break;
		y1 += dy;
		string = skiptr(string + c);
	}
	return y1;
}

int draw::textw(const char* string, int count) {
	if(!font)
		return 0;
	int x1 = 0;
	const pma* pk = 0;//font->getheader("KRN");
	// unsigned char s0 = 0x0;
	if(count == -1) {
		const char *s1 = string;
		if(pk) {
			while(*s1) {
				// unsigned char sr = *((unsigned char*)s1);
				x1 += textw(szget(&s1, codepage::W1251));
				// s0 = sr;
			}
		} else {
			while(*s1)
				x1 += textw(szget(&s1, codepage::W1251));
		}
	} else {
		const char *s1 = string;
		const char *s2 = string + count;
		if(pk) {
			while(s1 < s2) {
				// unsigned char sr = *((unsigned char*)s1);
				x1 += textw(szget(&s1, codepage::W1251));
				// s0 = sr;
			}
		} else {
			while(s1 < s2)
				x1 += textw(szget(&s1, codepage::W1251));
		}
	}
	return x1;
}

void draw::text(const char* string, int count, unsigned flags) {
	if(!font)
		return;
	auto dy = texth();
	if(caret.y >= clipping.y2 || caret.y + dy < clipping.y1)
		return;
	if(count == -1)
		count = zlen(string);
	const char *s1 = string;
	const char *s2 = string + count;
	auto push_caret = caret;
	// unsigned char s0 = 0x0;
	while(s1 < s2) {
		int sm = *s1++;
		if(sm >= 0x21)
			glyph(sm, flags);
		caret.x += textw(sm);
	}
	caret = push_caret;
}

void draw::textc(const char* string, int count, unsigned flags) {
	auto push_clip = clipping;
	setclip({caret.x, caret.y, caret.x + width, caret.y + texth()});
	auto w = textw(string, count);
	text_clipped = w > width;
	auto push_caret = caret;
	caret.x = aligned(caret.x, width, flags, w);
	text(string, count, flags);
	caret = push_caret;
	clipping = push_clip;
	caret.y += texth();
}

int draw::textbc(const char* string, int width, bool* line_feed) {
	if(!font)
		return 0;
	int p = -1;
	int w = 0;
	const char* s1 = string;
	while(true) {
		unsigned s = *s1++;
		if(s == 0x20 || s == 9)
			p = s1 - string;
		else if(s == 0) {
			p = s1 - string - 1;
			break;
		} else if(s == 10 || s == 13) {
			p = s1 - string;
			if(line_feed)
				*line_feed = true;
			break;
		}
		w += textw(s);
		if(w > width)
			break;
	}
	if(p == -1)
		p = s1 - string;
	return p;
}

void draw::texta(const char* string, unsigned state) {
	if(!string || string[0] == 0)
		return;
	auto push_caret = caret;
	auto y2 = caret.y + height;
	caret.y += alignedh1(string, state);
	// auto y1 = caret.y;
	if(state & TextSingleLine) {
		auto push_clip = clipping; setclip(getrect());
		caret.x = aligned(caret.x, width, state, draw::textw(string));
		text(string, -1, state);
		clipping = push_clip;
	} else {
		auto dy = texth();
		while(caret.y < y2) {
			int c = textbc(string, width);
			if(!c)
				break;
			int w = textw(string, c);
			caret.x = aligned(push_caret.x, width, state, w);
			text(string, c, state);
			caret.y += dy;
			string = skiptr(string + c);
		}
	}
	caret = push_caret;
}

int draw::hittest(int x, int hit_x, const char* p, int lenght) {
	if(hit_x < x)
		return -2;
	int index = 0;
	int syw = 0;
	while(index < lenght) {
		syw = draw::textw(*p++);
		if(hit_x <= x + 1 + syw / 2)
			break;
		x += syw;
		index++;
	}
	if(index == lenght) {
		if(hit_x > x + syw)
			index = -3;
	}
	return index;
}

int draw::hittest(rect rc, const char* string, unsigned state, point pt) {
	int w1 = rc.width();
	int dy = texth();
	int x1 = rc.x1;
	int y1 = rc.y1 + draw::alignedh(rc, string, state);
	auto p = (const char*)string;
	if(state & TextSingleLine) {
		auto c = zlen(string);
		auto w1 = textw(string, c);
		auto x0 = aligned(x1, rc.width(), state, w1);
		return hittest(x0, pt.x, string, c);
	} else {
		while(y1 < rc.y2) {
			int c = textbc(p, w1);
			if(!c)
				break;
			int w = textw(p, c);
			auto x0 = aligned(x1, w1, state, w);
			if(pt.y >= y1 && pt.y < y1 + dy) {
				if(pt.x >= x0 && pt.x < x0 + w)
					return p - string + draw::hittest(x0, pt.x, p, c);
				if(pt.x < x0)
					return -2;
				return -3;
			}
			y1 += dy;
			p = draw::skiptr(p + c);
		}
	}
	return -1;
}

void draw::image(int x, int y, const sprite* e, int id, int flags) {
	int x2, y2;
	color* pal;
	if(!e)
		return;
	const sprite::frame& f = e->get(id);
	if(!f.offset)
		return;
	if(!canvas)
		return;
	if(flags & ImageMirrorH) {
		x2 = x;
		if((flags & ImageNoOffset) == 0)
			x2 += f.ox;
		x = x2 - f.sx;
	} else {
		if((flags & ImageNoOffset) == 0)
			x -= f.ox;
		x2 = x + f.sx;
	}
	if(flags & ImageMirrorV) {
		y2 = y;
		if((flags & ImageNoOffset) == 0)
			y2 += f.oy;
		y = y2 - f.sy;
	} else {
		if((flags & ImageNoOffset) == 0)
			y -= f.oy;
		y2 = y + f.sy;
	}
	unsigned char* s = (unsigned char*)e + f.offset;
	if(y2<clipping.y1 || y>clipping.y2 || x2<clipping.x1 || x>clipping.x2)
		return;
	if(y < clipping.y1) {
		if((flags & ImageMirrorV) == 0) {
			switch(f.encode) {
			case sprite::ALC: s = skip_alc(s, clipping.y1 - y); break;
			case sprite::ALC8: s = skip_v4(s, clipping.y1 - y); break;
			case sprite::RAW: s += (clipping.y1 - y) * f.sx * 3; break;
			case sprite::RAW8: s += (clipping.y1 - y) * f.sx; break;
			case sprite::RLE8: s = skip_v3(s, clipping.y1 - y); break;
			case sprite::RLE: s = skip_rle32(s, clipping.y1 - y); break;
			default: break;
			}
		}
		y = clipping.y1;
	}
	if(y2 > clipping.y2) {
		if(flags & ImageMirrorV) {
			switch(f.encode) {
			case sprite::ALC: s = skip_alc(s, y2 - clipping.y2); break;
			case sprite::ALC8: s = skip_v4(s, y2 - clipping.y2); break;
			case sprite::RAW: s += (y2 - clipping.y2) * f.sx * 3; break;
			case sprite::RAW8: s += (y2 - clipping.y2) * f.sx; break;
			case sprite::RLE8: s = skip_v3(s, y2 - clipping.y2); break;
			case sprite::RLE: s = skip_rle32(s, y2 - clipping.y2); break;
			default: break;
			}
		}
		y2 = clipping.y2;
	}
	if(y >= y2)
		return;
	int wd = (flags & ImageMirrorV) ? -canvas->scanline : canvas->scanline;
	int sy = (flags & ImageMirrorV) ? y2 - 1 : y;
	switch(f.encode) {
	case sprite::RAW:
		if(x < clipping.x1) {
			if((flags & ImageMirrorH) == 0)
				s += (clipping.x1 - x) * 3;
			x = clipping.x1;
		}
		if(x2 > clipping.x2) {
			if(flags & ImageMirrorH)
				s += (x2 - clipping.x2) * 3;
			x2 = clipping.x2;
		}
		if(x >= x2)
			return;
		if(flags & ImageMirrorH)
			raw32m(ptr(x2 - 1, sy), wd, s, f.sx * 3,
				x2 - x,
				y2 - y);
		else
			raw32(ptr(x, sy), wd, s, f.sx * 3,
				x2 - x,
				y2 - y);
		break;
	case sprite::RAW8:
		if(x < clipping.x1) {
			s += clipping.x1 - x;
			x = clipping.x1;
		}
		if(x2 > clipping.x2)
			x2 = clipping.x2;
		if(x >= x2)
			return;
		if(!f.pallette || (flags & ImagePallette))
			pal = draw::palt;
		else
			pal = (color*)e->ptr(f.pallette);
		if(!pal)
			return;
		if(flags & ImageMirrorH)
			raw832m(ptr(x2 - 1, y), wd, s, f.sx,
				x2 - x,
				y2 - y,
				pal);
		else
			raw832(ptr(x, y), wd, s, f.sx, x2 - x, y2 - y, pal);
		break;
	case sprite::RLE8:
		if(!f.pallette || (flags & ImagePallette))
			pal = draw::palt;
		else
			pal = (color*)e->ptr(f.pallette);
		if(!pal)
			return;
		if(flags & ImageMirrorH)
			rle832m(ptr(x2 - 1, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha, pal);
		else
			rle832(ptr(x, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha, pal);
		break;
	case sprite::RLE:
		if(flags & ImageMirrorH)
			rle32m(ptr(x2 - 1, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha);
		else
			rle32(ptr(x, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha);
		break;
	case sprite::ALC:
		if(flags & TextBold)
			alc32(ptr(x, sy - 1), wd, s, y2 - y,
				ptr(clipping.x1, sy - 1),
				ptr(clipping.x2, sy - 1),
				fore, (flags & TextItalic) != 0);
		if(flags & TextBold)
			alc32(ptr(x, sy - 1), wd, s, y2 - y,
				ptr(clipping.x1, sy - 1),
				ptr(clipping.x2, sy - 1),
				fore, (flags & TextItalic) != 0);
		alc32(ptr(x, sy), wd, s, y2 - y,
			ptr(clipping.x1, sy), ptr(clipping.x2, sy),
			fore, (flags & TextItalic) != 0);
		break;
	case sprite::ALC8:
		if(flags & ImageMirrorH) {
		} else
			alc832(ptr(x, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha);
		break;
	default:
		break;
	}
}

void draw::image(const sprite* e, int id, int flags, color* pal) {
	auto pal_push = draw::palt;
	draw::palt = pal;
	image(caret.x, caret.y, e, id, flags | ImagePallette);
	draw::palt = pal_push;
}

static void raw32cr(int x, int y, int width, int x0, int y0, unsigned char* s, unsigned scanline) {
	if(y >= clipping.y2 || y < clipping.y1 || x >= clipping.x2)
		return;
	auto x2 = x + width;
	if(x2 < clipping.x1)
		return;
	if(x < clipping.x1)
		x = clipping.x1;
	if(x2 > clipping.x2)
		x2 = clipping.x2;
	width = x2 - x;
	if(width <= 0)
		return;
	const auto cbs = 3;
	const auto cbd = 4;
	auto p1 = ptr(x, y);
	auto sb = s + ((y - y0) * scanline + (x - x0)) * cbs;
	auto se = sb + width * cbs;
	while(sb < se) {
		p1[0] = sb[0];
		p1[1] = sb[1];
		p1[2] = sb[2];
		sb += cbs;
		p1 += cbd;
	}
}

void draw::imager(int xm, int ym, const sprite* e, int id, int r) {
	if(!e)
		return;
	auto& f = e->get(id);
	if(!f.offset)
		return;
	if(r > f.sx / 2)
		r = f.sx / 2;
	if(r > f.sy / 2)
		r = f.sy / 2;
	if(xm - r >= clipping.x2 || xm + r < clipping.x1 || ym - r >= clipping.y2 || ym + r < clipping.y1)
		return;
	auto sx = f.sx;
	auto x1 = xm - f.sx / 2;
	auto y1 = ym - f.sy / 2;
	unsigned char* s = (unsigned char*)e + f.offset;
	int x = -r, y = 0, err = 2 - 2 * r, y2 = -1000;
	do {
		if(y2 != y) {
			y2 = y;
			raw32cr(xm + x, ym + y, -2 * x, x1, y1, s, sx);
			if(y != 0)
				raw32cr(xm + x, ym - y, -2 * x, x1, y1, s, sx);
		}
		r = err;
		if(r <= y)
			err += ++y * 2 + 1;
		if(r > x || err > y)
			err += ++x * 2 + 1;
	} while(x < 0);
}

void surface::blend(const surface& source, int alpha) {
	if((bpp != 32 && bpp != source.bpp) || height != source.height || width != source.width)
		return;
	cpy32a(ptr(0, 0), scanline,
		const_cast<surface&>(source).ptr(0, 0), source.scanline,
		width, height, alpha);
}

void draw::blit(surface& ds, int x1, int y1, int w, int h, unsigned flags, const surface& ss, int xs, int ys) {
	if(ss.bpp != ds.bpp)
		return;
	int ox;
	if(!correctb(x1, y1, w, h, ox))
		return;
	if(h > ss.height)
		h = ss.height;
	if(w > ss.width)
		w = ss.width;
	if(flags & ImageTransparent)
		cpy32t(
			ds.ptr(x1, y1), ds.scanline,
			const_cast<surface&>(ss).ptr(xs, ys) + ox * 4, ss.scanline,
			w, h);
	else
		cpy(
			ds.ptr(x1, y1), ds.scanline,
			const_cast<surface&>(ss).ptr(xs, ys) + ox * 4, ss.scanline,
			w, h, 4);
}

static void str832s(int x, int y, const unsigned char* s, int h) {
	auto x1 = x;
	auto y2 = y + h;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			x = x1;
			if(++y >= y2)
				break;
		} else if(c <= 0x9F) {
			rectfx(x - 1, y - 1, x + c, y + 1);
			s += c;
			x += c;
		} else if(c == 0xA0)
			x += (*s++);
		else
			x += (c - 0xA0);
	}
}

void draw::stroke(int x, int y, const sprite* e, int id, int flags) {
	if(!e || !canvas)
		return;
	const sprite::frame& f = e->get(id);
	if(!f.offset)
		return;
	if((flags & ImageNoOffset) == 0) {
		x -= f.ox;
		y -= f.oy;
	}
	switch(f.encode) {
	case sprite::ALC8: case sprite::RLE8: str832s(x, y, e->ptr(f.offset), f.sy); break;
	default: break;
	}
}

void draw::blit(surface& dest, int x, int y, int width, int height, unsigned flags, const surface& source, int x_source, int y_source, int width_source, int height_source) {
	if(width == width_source && height == height_source) {
		blit(dest, x, y, width, height, flags, source, x_source, y_source);
		return;
	}
	if(source.bpp != dest.bpp)
		return;
	if(!corrects(dest, x, y, width, height))
		return;
	if(!corrects(source, x_source, y_source, width_source, height_source))
		return;
	int ox;
	if(!correctb(x_source, y_source, width, height, ox))
		return;
	scale32(
		dest.ptr(x, y), dest.scanline, width, height,
		const_cast<surface&>(source).ptr(x_source, y_source) + ox * 4, source.scanline, width_source, height_source);
}

const pma* pma::getheader(const char* id) const {
	auto p = this;
	while(p->name[0]) {
		if(p->name[0] == id[0]
			&& p->name[1] == id[1]
			&& p->name[2] == id[2])
			return p;
		p = (pma*)((char*)p + p->size);
	}
	return 0;
}

const char* pma::getstring(int id) const {
	auto p = getheader("STR");
	if(!p || id > p->count)
		return "";
	return (char*)p + ((unsigned*)((char*)p + sizeof(*p)))[id];
}

int pma::find(const char* name) const {
	for(int i = 1; i <= count; i++) {
		if(szcmp(getstring(i), name) == 0)
			return i;
	}
	return 0;
}

int sprite::ganim(int index, int tick) const {
	if(!cicles)
		return 0;
	cicle* c = gcicle(index);
	if(!c->count)
		return 0;
	return c->start + tick % c->count;
}

//int sprite::glyph(unsigned sym) const {
//	// First interval (latin plus number plus ASCII)
//	unsigned* pi = (unsigned*)edata();
//	unsigned* p2 = pi + esize() / sizeof(unsigned);
//	unsigned n = 0;
//	while(pi < p2) {
//		if(sym >= pi[0] && sym <= pi[1])
//			return sym - pi[0] + n;
//		n += pi[1] - pi[0] + 1;
//		pi += 2;
//	}
//	return '?' - 0x21; // Unknown symbol is question mark
//}

rect sprite::frame::getrect(int x, int y, unsigned flags) const {
	int x2, y2;
	if(!offset)
		return{0, 0, 0, 0};
	if(flags & ImageMirrorH) {
		x2 = x;
		if((flags & ImageNoOffset) == 0)
			x2 += ox;
		x = x2 - sx;
	} else {
		if((flags & ImageNoOffset) == 0)
			x -= ox;
		x2 = x + sx;
	}
	if(flags & ImageMirrorV) {
		y2 = y;
		if((flags & ImageNoOffset) == 0)
			y2 += oy;
		y = y2 - sy;
	} else {
		if((flags & ImageNoOffset) == 0)
			y -= oy;
		y2 = y + sy;
	}
	return{x, y, x2, y2};
}

surface::plugin* surface::plugin::first;

surface::surface() : width(0), height(0), scanline(0), bpp(32), bits(0) {
}

surface::surface(int width, int height, int bpp) : surface() {
	resize(width, height, bpp, true);
}

surface::plugin::plugin(const char* name, const char* filter) : name(name), filter(filter), next(0) {
	seqlink(this);
}

unsigned char* surface::ptr(int x, int y) {
	return bits + y * scanline + x * (bpp / 8);
}

surface::~surface() {
	resize(0, 0, 0, true);
}

unsigned char* surface::allocator(unsigned char* bits, unsigned size) {
	if(!size) {
		if(bits)
			free(bits);
		bits = 0;
	} else {
		if(!bits)
			bits = (unsigned char*)malloc(size);
		else
			bits = (unsigned char*)realloc(bits, size);
	}
	return bits;
}

void surface::resize(int width, int height, int bpp, bool alloc_memory) {
	if(this->width == width && this->height == height && this->bpp == bpp)
		return;
	this->bpp = bpp;
	this->width = width;
	this->height = height;
	this->scanline = color_scanline(width, bpp);
	if(alloc_memory)
		bits = allocator(bits, height ? (height + 1) * scanline : 0);
}

void surface::flipv() {
	color_flipv(bits, scanline, height);
}

void surface::convert(int new_bpp, color* pallette) {
	if(bpp == new_bpp) {
		bpp = iabs(new_bpp);
		return;
	}
	auto old_scanline = scanline;
	scanline = color_scanline(width, new_bpp);
	if(iabs(new_bpp) <= bpp)
		color_convert(bits, width, height, new_bpp, 0, bits, bpp, pallette, old_scanline);
	else {
		unsigned char* new_bits = (unsigned char*)allocator(0, (height + 1) * scanline);
		color_convert(
			new_bits, width, height, new_bpp, pallette,
			bits, bpp, pallette, old_scanline);
		allocator(bits, 0);
		bits = new_bits;
	}
	bpp = iabs(new_bpp);
}

static unsigned char* rotate32(unsigned char* src, int width, int height) {
	auto m = sizeof(color);
	auto dst = (unsigned char*)malloc(height * width * m);
	for(auto x = 0; x < width; x++) {
		for(auto y = 0; y < height; y++) {
			auto p1 = dst + (x * height + y) * m;
			auto p2 = src + (y * width + x) * m;
			*((color*)p1) = *((color*)p2);
		}
	}
	return dst;
}

void surface::rotate() {
	if(iabs(bpp) != 32)
		return;
	auto pn = rotate32(bits, width, height);
	free(bits);
	bits = pn;
	iswap(width, height);
	scanline = color_scanline(width, bpp);
}

void draw::key2str(stringbuilder& sb, int key) {
	if(key & Ctrl)
		sb.add("Ctrl+");
	if(key & Alt)
		sb.add("Alt+");
	if(key & Shift)
		sb.add("Shift+");
	key = key & CommandMask;
	switch(key) {
	case KeyDown: sb.add("Down"); break;
	case KeyDelete: sb.add("Del"); break;
	case KeyEnd: sb.add("End"); break;
	case KeyEnter: sb.add("Enter"); break;
	case KeyHome: sb.add("Home"); break;
	case KeyLeft: sb.add("Left"); break;
	case KeyPageDown: sb.add("Page Down"); break;
	case KeyPageUp: sb.add("Page Up"); break;
	case KeyRight: sb.add("Right"); break;
	case KeyUp: sb.add("Up"); break;
	case F1: sb.add("F1"); break;
	case F2: sb.add("F2"); break;
	case F3: sb.add("F3"); break;
	case F4: sb.add("F4"); break;
	case F5: sb.add("F5"); break;
	case F6: sb.add("F6"); break;
	case F7: sb.add("F7"); break;
	case F8: sb.add("F8"); break;
	case F9: sb.add("F9"); break;
	case F10: sb.add("F10"); break;
	case F11: sb.add("F11"); break;
	case F12: sb.add("F12"); break;
	case KeySpace: sb.add("Space"); break;
	case KeyEscape: sb.add("Esc"); break;
	default:
		if(key >= 0x20) {
			char temp[2] = {(char)upper_symbol(key), 0};
			sb.add(temp);
		}
		break;
	}
}

void draw::execute(fnevent proc, long value, long value2, const void* object) {
	domodal = proc;
	// Это важно, так как мы никогда не обнуляем эту переменную при исполнении не стандартной команды.
	// Если не делать, будет зацикливание.
	hot.key = 0;
	hot.param = value;
	hot.param2 = value2;
	hot.object = object;
}

static void standart_domodal() {
	if(draw::ptips)
		draw::ptips();
	draw::hot.key = draw::rawinput();
	if(!draw::hot.key)
		exit(0);
}

static void beforemodal() {
	caret = {0, 0};
	width = getwidth();
	height = getheight();
	hilite_object = 0;
	hilite_position.clear();
	hilite_size = 0;
	hot.hilite.clear();
	if(hot.key == InputNeedUpdate)
		hot.key = InputUpdate;
	else
		domodal = standart_domodal;
	if(hot.mouse.x < 0 || hot.mouse.y < 0)
		sys_static_area.clear();
	else
		sys_static_area = {0, 0, draw::getwidth(), draw::getheight()};
	tips_caret.clear();
	tips_size.clear();
}

bool draw::ismodal() {
	beforemodal();
	if(pbeforemodal)
		pbeforemodal();
	if(!break_modal && !next_scene)
		return true;
	break_modal = false;
	if(pleavemodal)
		pleavemodal();
	return break_modal;
}

void draw::breakmodal(long result) {
	break_modal = true;
	break_result = result;
}

void draw::buttoncancel() {
	breakmodal(0);
}

void draw::buttonok() {
	breakmodal(1);
}

void draw::buttonparam() {
	breakmodal(hot.param);
}

long draw::getresult() {
	return break_result;
}

void draw::cbsetint() {
	auto p = (int*)hot.object;
	*p = hot.param;
}

void draw::cbsetsht() {
	auto p = (short*)hot.object;
	*p = (short)hot.param;
}

void draw::cbsetptr() {
	auto p = (void**)hot.object;
	*p = (void*)hot.param;
}

void draw::strokeout(fnevent proc, int dx) {
	rectpush push;
	if(!dx)
		dx = metrics::border;
	caret.x -= dx;
	caret.y -= dx;
	width += dx * 2;
	height += dx * 2;
	proc();
}

void draw::strokeborder() {
	auto push_fore = fore;
	fore = colors::border;
	rectb();
	fore = push_fore;
}

void draw::strokeactive() {
	auto push_fore = fore;
	fore = colors::active;
	rectb();
	fore = push_fore;
}

void draw::strokeline() {
	rectpush push;
	auto push_fore = fore;
	fore = colors::border;
	line(caret.x + width, caret.y);
	fore = push_fore;
}

void draw::vertical(fnevent proc) {
	auto push_caret = caret;
	auto push_width = width;
	auto push_height = height;
	proc();
	caret = push_caret;
	if(height)
		caret.y += height + metrics::padding + metrics::border * 2;
	height = push_height;
	width = push_width;
}

void draw::setneedupdate() {
	hot.key = InputNeedUpdate;
}

void draw::initialize(const char* title) {
	draw::width = 320;
	draw::font = metrics::font;
	draw::fore = colors::text;
	draw::fore_stroke = colors::border;
	draw::create(awindow.x, awindow.y, awindow.width, awindow.height, awindow.flags, 32);
	if(title)
		draw::setcaption(title);
}

void draw::paintstart() {
	if(pbackground)
		pbackground();
}

void draw::paintfinish() {
	if(pfinish)
		pfinish();
}

void* draw::scene(fnevent proc) {
	while(ismodal()) {
		paintstart();
		if(proc)
			proc();
		paintfinish();
		domodal();
	}
	return (void*)getresult();
}

void draw::scene() {
	scene(0);
}

bool draw::button(const char* title, unsigned key, fnbutton proc, bool vertical) {
	auto push_width = width;
	auto push_height = height;
	control_hilited = proc(title);
	if(vertical) {
		width = push_width;
		if(!height)
			return false;
		caret.y += height + metrics::padding;
	} else {
		height = push_height;
		if(!width)
			return false;
		caret.x += width + metrics::padding;
	}
	return (key && hot.key == key)
		|| (hot.key == MouseLeft && control_hilited && !hot.pressed);
}

void draw::fire(bool run, fnevent proc, long value, long value2, const void* object) {
	if(!proc)
		return;
	if(run)
		execute(proc, value, value2, object);
}

void draw::set(int x, int y) {
	caret.x = x - camera.x;
	caret.y = y - camera.y;
}

bool draw::isclipped(int size) {
	rect rc = {caret.x - size, caret.y - size, caret.x + size, caret.y + size};
	return !rc.intersect(clipping);
}

void run_next_scene() {
	while(next_scene) {
		auto p = next_scene;
		next_scene = 0;
		p();
	}
}

void set_next_scene(fnevent v) {
	if(!next_scene)
		next_scene = v;
}

bool is_next_scene() {
	return next_scene != 0;
}
