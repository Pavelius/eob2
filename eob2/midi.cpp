#include "io_stream.h"

#pragma comment(lib, "winmm.lib")

namespace {
#pragma pack(push, 1)
struct mid_header {
	unsigned int id; // identifier "MThd"
	unsigned int size; // always 6 in big-endian format
	unsigned short format; // big-endian format
	unsigned short tracks; // number of tracks, big-endian
	unsigned short ticks; // number of ticks per quarter note, big-endian
};
struct mid_track {
	unsigned int id; // identifier "MTrk"
	unsigned int length; // track length, big-endian
};
#pragma pack(pop)
struct trk {
	mid_track* track;
	unsigned char* buf;
	unsigned char last_event;
	unsigned int absolute_time;
};
struct evt {
	unsigned char* data;
	unsigned char event;
	unsigned int absolute_time;
};
}

static volatile bool midi_need_close;
static volatile bool midi_need_repeat;
static unsigned int current_time = 0;

static unsigned long read_var_long(unsigned char* buf, unsigned int* bytesread) {
	unsigned long var = 0;
	unsigned char c;
	*bytesread = 0;
	do {
		c = buf[(*bytesread)++];
		var = (var << 7) + (c & 0x7f);
	} while(c & 0x80);
	return var;
}

static inline unsigned short midi_bytes_short(unsigned short in) {
	return ((in << 8) | (in >> 8));
}

static inline unsigned long midi_bytes_long(unsigned long in) {
	unsigned short *p;
	p = (unsigned short*)&in;
	return ((((unsigned long)midi_bytes_short(p[0])) << 16) | (unsigned long)midi_bytes_short(p[1]));
}

static struct evt get_next_event(const trk* track) {
	unsigned char* buf;
	struct evt e;
	unsigned int bytesread;
	unsigned int time;
	buf = track->buf;
	time = read_var_long(buf, &bytesread);
	buf += bytesread;
	e.absolute_time = track->absolute_time + time;
	e.data = buf;
	e.event = *e.data;
	return e;
}

static int is_track_end(const struct evt* e) {
	if(e->event == 0xff) // meta-event?
		if(*(e->data + 1) == 0x2f) // track end?
			return 1;
	return 0;
}

void midi_repeat(bool value) {
	midi_need_repeat = value;
}

#ifdef _WIN32

//#define USE_WIN_HEADER

#ifdef USE_WIN_HEADER

#include <windows.h>
#include <mmsystem.h>

#else

#include "slice.h"

#define HANDLE void*
#define HMIDISTRM HANDLE
#define HMIDIOUT HANDLE
#define DWORD unsigned long
#define DWORD_PTR unsigned long
#define WINMMAPI extern "C" __declspec(dllimport)
#define MMRESULT unsigned int
#define WINAPI __stdcall
#define CALLBACK __stdcall

#define MMSYSERR_NOERROR    0                    /* no error */

#define MM_MOM_OPEN         0x3C7 /* MIDI output */
#define MM_MOM_CLOSE        0x3C8
#define MM_MOM_DONE         0x3C9

#define MOM_OPEN        MM_MOM_OPEN
#define MOM_CLOSE       MM_MOM_CLOSE
#define MOM_DONE        MM_MOM_DONE

#define MEVT_SHORTMSG       ((unsigned char)0x00)    /* parm = shortmsg for midiOutShortMsg */
#define MEVT_TEMPO          ((unsigned char)0x01)    /* parm = new tempo in microsec/qn     */
#define MEVT_NOP            ((unsigned char)0x02)    /* parm = unused; does nothing         */

#define MIDIPROP_SET        0x80000000L
#define MIDIPROP_GET        0x40000000L

#define MIDIPROP_TIMEDIV    0x00000001L
#define MIDIPROP_TEMPO      0x00000002L

#define CALLBACK_TYPEMASK   0x00070000l    /* callback type mask */
#define CALLBACK_NULL       0x00000000l    /* no callback */
#define CALLBACK_WINDOW     0x00010000l    /* dwCallback is a HWND */
#define CALLBACK_TASK       0x00020000l    /* dwCallback is a HTASK */
#define CALLBACK_FUNCTION   0x00030000l    /* dwCallback is a FARPROC */

#define INFINITE            0xFFFFFFFF  // Infinite timeout

struct MIDIEVENT {
	DWORD       dwDeltaTime;          /* Ticks since last event */
	DWORD       dwStreamID;           /* Reserved; must be zero */
	DWORD       dwEvent;              /* Event type and parameters */
	DWORD       dwParms[1];           /* Parameters if this is a long event */
};
struct MIDIPROPTIMEDIV {
	DWORD       cbStruct;
	DWORD       dwTimeDiv;
};
struct MIDIHDR {
	char*       lpData;               /* pointer to locked data block */
	DWORD       dwBufferLength;       /* length of data in data block */
	DWORD       dwBytesRecorded;      /* used for input only */
	DWORD*		dwUser;               /* for client's use */
	DWORD       dwFlags;              /* assorted flags (see defines) */
	struct midihdr_tag *lpNext;   /* reserved for driver */
	DWORD*		reserved;             /* reserved for driver */
	DWORD       dwOffset;             /* Callback offset into buffer */
	DWORD*      dwReserved[8];        /* Reserved for MMSYSTEM */
};

WINMMAPI MMRESULT WINAPI midiOutOpen(HMIDIOUT* phmo, unsigned int uDeviceID, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI midiOutClose(HMIDIOUT hmo);
WINMMAPI MMRESULT WINAPI midiOutShortMsg(HMIDIOUT hmo, DWORD dwMsg);

WINMMAPI MMRESULT WINAPI midiOutReset(HMIDIOUT hmo);
WINMMAPI MMRESULT WINAPI midiOutPrepareHeader(HMIDIOUT hmo, MIDIHDR* pmh, unsigned int cbmh);
WINMMAPI MMRESULT WINAPI midiOutUnprepareHeader(HMIDIOUT hmo, MIDIHDR* pmh, unsigned int cbmh);

WINMMAPI MMRESULT WINAPI midiStreamOpen(HMIDISTRM* phms, unsigned int* puDeviceID, DWORD cMidi, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI midiStreamProperty(HMIDISTRM hms, unsigned char* lppropdata, DWORD dwProperty);
WINMMAPI MMRESULT WINAPI midiStreamPause(HMIDISTRM hms);
WINMMAPI MMRESULT WINAPI midiStreamRestart(HMIDISTRM hms);
WINMMAPI MMRESULT WINAPI midiStreamOut(HMIDISTRM hms, MIDIHDR* pmh, unsigned int cbmh);
WINMMAPI MMRESULT WINAPI midiStreamClose(HMIDISTRM hms);

WINMMAPI int WINAPI CloseHandle(HANDLE hObject);
WINMMAPI HANDLE WINAPI CreateEventA(void* lpEventAttributes, int bManualReset, int bInitialState, const char* lpName);

WINMMAPI int WINAPI SetEvent(HANDLE hEvent);
WINMMAPI void WINAPI Sleep(unsigned dwMilliseconds);
WINMMAPI DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);

#endif

const unsigned int music_buffer_size = 512 * 12;

static HANDLE music_event;
static HMIDISTRM out = 0;
static unsigned music_buffer[music_buffer_size];

void midi_sleep(unsigned milliseconds) {
	if(milliseconds)
		Sleep(milliseconds);
}

static void CALLBACK midi_play_callback(HMIDIOUT out, unsigned int msg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
	switch(msg) {
	case MOM_DONE:
		if(music_event)
			SetEvent(music_event);
		break;
	}
}

static unsigned int get_buffer_ex9(struct trk* tracks, unsigned int ntracks, unsigned int* out, unsigned int* outlen) {
	MIDIEVENT e, *p;
	unsigned int streamlen = 0;
	unsigned int i;

	if(!tracks || !out || !outlen)
		return 0;

	*outlen = 0;

	while(true) {
		unsigned int time = (unsigned int)-1;
		unsigned int idx = -1;
		struct evt evt;
		unsigned char c;

		if(((streamlen + 3) * sizeof(unsigned int)) >= music_buffer_size)
			break;

		// get the next event
		for(i = 0; i < ntracks; i++) {
			evt = get_next_event(&tracks[i]);
			if(!(is_track_end(&evt)) && (evt.absolute_time < time)) {
				time = evt.absolute_time;
				idx = i;
			}
		}

		// if idx == -1 then all the tracks have been read up to the end of track mark
		if(idx == -1)
			break; // we're done

		e.dwStreamID = 0; // always 0

		evt = get_next_event(&tracks[idx]);

		tracks[idx].absolute_time = evt.absolute_time;
		e.dwDeltaTime = tracks[idx].absolute_time - current_time;
		current_time = tracks[idx].absolute_time;

		if(!(evt.event & 0x80)) { // running mode
			unsigned char last = tracks[idx].last_event;
			c = *evt.data++; // get the first data byte
			e.dwEvent = ((unsigned long)MEVT_SHORTMSG << 24) |
				((unsigned long)last) |
				((unsigned long)c << 8);
			if(!((last & 0xf0) == 0xc0 || (last & 0xf0) == 0xd0)) {
				c = *evt.data++; // get the second data byte
				e.dwEvent |= ((unsigned long)c << 16);
			}

			p = (MIDIEVENT*)&out[streamlen];
			*p = e;

			streamlen += 3;

			tracks[idx].buf = evt.data;
		} else if(evt.event == 0xff) { // meta-event
			evt.data++; // skip the event byte
			unsigned char meta = *evt.data++; // read the meta-event byte
			unsigned int len;

			switch(meta) {
			case 0x51: // only care about tempo events
			{
				unsigned char a, b, c;
				len = *evt.data++; // get the length byte, should be 3
				a = *evt.data++;
				b = *evt.data++;
				c = *evt.data++;

				e.dwEvent = ((unsigned long)MEVT_TEMPO << 24) |
					((unsigned long)a << 16) |
					((unsigned long)b << 8) |
					((unsigned long)c << 0);

				p = (MIDIEVENT*)&out[streamlen];
				*p = e;

				streamlen += 3;
			}
			break;
			default: // skip all other meta events
				len = *evt.data++; // get the length byte
				evt.data += len;
				break;
			}

			tracks[idx].buf = evt.data;
		} else if((evt.event & 0xf0) != 0xf0) { // normal command
			tracks[idx].last_event = evt.event;
			evt.data++; // skip the event byte
			c = *evt.data++; // get the first data byte
			e.dwEvent = ((unsigned long)MEVT_SHORTMSG << 24) |
				((unsigned long)evt.event << 0) |
				((unsigned long)c << 8);
			if(!((evt.event & 0xf0) == 0xc0 || (evt.event & 0xf0) == 0xd0)) {
				c = *evt.data++; // get the second data byte
				e.dwEvent |= ((unsigned long)c << 16);
			}

			p = (MIDIEVENT*)&out[streamlen];
			*p = e;

			streamlen += 3;

			tracks[idx].buf = evt.data;
		}

	}

	*outlen = streamlen * sizeof(unsigned int);

	return 1;
}

void midi_open() {
	if(out)
		return;
	unsigned int device = 0;
	midiStreamOpen(&out, &device, 1, (DWORD)midi_play_callback, 0, CALLBACK_FUNCTION);
}

static void midi_play(unsigned ticks, trk* tracks, unsigned ntracks, unsigned* streambuf, unsigned streambufsize) {
	if(music_event)
		return;
	music_event = CreateEventA(0, 0, 0, 0);
	if(music_event) {
		MIDIPROPTIMEDIV prop;
		prop.cbStruct = sizeof(MIDIPROPTIMEDIV);
		prop.dwTimeDiv = ticks;
		if(midiStreamProperty(out, (unsigned char*)&prop, MIDIPROP_SET | MIDIPROP_TIMEDIV) == MMSYSERR_NOERROR) {
			MIDIHDR mhdr;
			mhdr.lpData = (char*)streambuf;
			mhdr.dwBufferLength = mhdr.dwBytesRecorded = streambufsize;
			mhdr.dwFlags = 0;
			if(midiOutPrepareHeader((HMIDIOUT)out, &mhdr, sizeof(MIDIHDR)) == MMSYSERR_NOERROR) {
				if(midiStreamRestart(out) == MMSYSERR_NOERROR) {
					unsigned int streamlen = 0;
					get_buffer_ex9(tracks, ntracks, streambuf, &streamlen);
					while(streamlen > 0 && !midi_need_close) {
						mhdr.dwBytesRecorded = streamlen;
						if(midiStreamOut(out, &mhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR)
							break;
						WaitForSingleObject(music_event, INFINITE);
						if(midi_need_close)
							break;
						get_buffer_ex9(tracks, ntracks, streambuf, &streamlen);
					}
					midiOutReset((HMIDIOUT)out);
				}
				midiOutUnprepareHeader((HMIDIOUT)out, &mhdr, sizeof(MIDIHDR));
			}
		}
		CloseHandle(music_event);
		music_event = 0;
	}
	current_time = 0;
	midi_need_close = false;
}

bool midi_busy() {
	return music_event != 0;
}

void midi_music_stop() {
	if(music_event) {
		midi_need_close = true;
		midi_need_repeat = false;
		SetEvent(music_event);
	}
}

void midi_play_raw(void* mid_data) {

	if(!mid_data || music_event)
		return;

	midi_open();

	midi_need_repeat = true;

	while(midi_need_repeat) {

		current_time = 0;
		auto midibuf = (unsigned char*)mid_data;
		auto hdr = (mid_header*)mid_data;
		midibuf += sizeof(struct mid_header);
		int ntracks = midi_bytes_short(hdr->tracks);

		auto tracks = new trk[ntracks];
		if(tracks) {
			for(auto i = 0; i < ntracks; i++) {
				tracks[i].track = (struct mid_track*)midibuf;
				tracks[i].buf = midibuf + sizeof(struct mid_track);
				tracks[i].absolute_time = 0;
				tracks[i].last_event = 0;
				midibuf += sizeof(struct mid_track) + midi_bytes_long(tracks[i].track->length);
			}
			memset(music_buffer, 0, sizeof(unsigned int) * music_buffer_size);
			midi_play(midi_bytes_short(hdr->ticks), tracks, ntracks, music_buffer, music_buffer_size);
			delete[] tracks;
		} else
			midi_need_repeat = false;

	}
	
}

#else

void midi_open() {
}

void midi_play_raw(void* mid_data) {
}

bool midi_busy() {
	return false;
}

void midi_music_stop() {
}

#endif // _WIN32