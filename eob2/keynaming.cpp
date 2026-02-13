#include "draw.h"
#include "stringbuilder.h"

void addkey(stringbuilder& sb, int key) {
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