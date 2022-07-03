/* vgac.c - vgainit, vgaputc */

#include <xinu.h>

#define	KBD_WIDTH	80
#define	KBD_HEIGHT	25

static	uint16	(*disp)[KBD_HEIGHT][KBD_WIDTH] = 0xB8000;
static	uint16	color = 0x0700;

local	uint16	get_cursor(void);
local	void	set_cursor(uint16);


/*------------------------------------------------------------------------
 *  vgainit  -  Initialize an empty vga display
 *------------------------------------------------------------------------
 */
void	vgainit(void)
{
	memset(*disp, 0, sizeof(*disp));
	set_cursor(0);
	(*disp)[0][0] = ' ' | color;
}


/*------------------------------------------------------------------------
 *  vgaputc  -  Write one character to a vga display
 *------------------------------------------------------------------------
 */
devcall	vgaputc(
	  char		ch
	)
{
	uint16		cursor_pos, row, col;
	uint16		i;

	cursor_pos = get_cursor();
	row = cursor_pos / KBD_WIDTH;
	col = cursor_pos % KBD_WIDTH;

	switch (ch) {
	case	TY_NEWLINE:
		row += 1;
		break;
	
	case	TY_RETURN:
		/* Move cursor to the beginning of the line and return */
		(*disp)[row][col] = 0;
		set_cursor(row * KBD_WIDTH);
		return OK;

	case	TY_TAB:
		col = (col / TAB_WIDTH + 1) * TAB_WIDTH;
		break;

	case	TY_BACKSP:
	case	TY_BACKSP2:
		/* Just ignore '\b' in output of process */
		return OK;

	default:
		(*disp)[row][col] = ch | color;
		col++;
		break;
	}

	/* Display cursor on the next place */
	if (col >= KBD_WIDTH) {
		col = 0;
		row++;
	}

	if (row == KBD_HEIGHT) {
		for (i = 1; i < KBD_HEIGHT; i++) {
			memcpy((*disp)[i - 1], (*disp)[i], sizeof(uint16[KBD_WIDTH]));
		}
		memset((*disp)[KBD_HEIGHT - 1], 0, sizeof(uint16[KBD_WIDTH]));
		row = KBD_HEIGHT - 1;
	}

	set_cursor(row * KBD_WIDTH + col);
	(*disp)[row][col] = ' ' | color;

	return OK;
}


/*------------------------------------------------------------------------
 *  vgaerase1  -  Erase one character to a vga display
 *------------------------------------------------------------------------
 */
devcall	vgaerase1(
	  bool8		invisible
	)
{
	uint16		cursor_pos, row, col;

	cursor_pos = get_cursor();
	row = cursor_pos / KBD_WIDTH;
	col = cursor_pos % KBD_WIDTH;

	/* Erase cursor first */
	(*disp)[row][col] = 0;

	/* Calulate position of the previous place */
	if (col != 0) {
		col--;
	} else {
		if (row != 0) {
			row--;
			col = KBD_WIDTH - 1;
		}
	}

	/* Replace it with cursor */
	set_cursor(row * KBD_WIDTH + col);
	(*disp)[row][col] = ' ' | color;

	/* For invisible characters, further erase ^ */
	if (invisible) {
		vgaerase1(FALSE);
	}

	return OK;
}


/*------------------------------------------------------------------------
 *  set_cursor  -  Set position of cursor
 *------------------------------------------------------------------------
 */
void	set_cursor(
	  uint16	pos
	)
{
	outb(0x3D4, 14);
	outb(0x3D5, pos >> 8);
	outb(0x3D4, 15);
	outb(0x3D5, pos);
}


/*------------------------------------------------------------------------
 *  get_cursor  -  Get position of cursor
 *------------------------------------------------------------------------
 */
uint16	get_cursor(void)
{
	uint16		pos;

	outb(0x3D4, 14);
	pos = inb(0x3D5) << 8;
	outb(0x3D4, 15);
	return pos | inb(0x3D5);
}
