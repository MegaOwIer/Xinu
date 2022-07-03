/* vgac.c - vgainit, vgaputc */

#include <xinu.h>

#define	KBD_WIDTH	80
#define	KBD_HEIGHT	25

static	uint16	(*disp)[KBD_HEIGHT][KBD_WIDTH] = 0xB8000;
static	uint16	color = 0x0700;

local	uint16	get_cursor(void);
local	void	set_cursor(uint16);
local	bool8	vga_shellbanner(char);


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
	  char		ch,
	  bool8		force
	)
{
	uint16		cursor_pos, row, col;
	uint16		i;
	static	bool8	shell_banner = FALSE;

	cursor_pos = get_cursor();
	row = cursor_pos / KBD_WIDTH;
	col = cursor_pos % KBD_WIDTH;

	if (shell_banner && !force) {
		shell_banner = vga_shellbanner(ch);
		return OK;
	}

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

	case	TY_ESC:
		if (!shell_banner) {
			shell_banner = TRUE;
			vga_shellbanner(ch);
			return OK;
		}
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


/*------------------------------------------------------------------------
 *  vga_shellbanner  -  Processing shell banner in VT100
 *------------------------------------------------------------------------
 */
bool8	vga_shellbanner(char ch)
{
	static	char	buffer[64];
	static	char	*it = buffer;
	static	int16	color_map[] = {0x0, 0x4, 0x2, 0xE, 0x1, 0x5, 0x3, 0xF};

	int32		opcode;
	char		*i;

	*it++ = ch;

	if (ch != 'm') {
		return TRUE;
	}

	if (buffer[1] != '[') {
		goto INVALID_BANNER;
	}

	if (buffer[3] == 'm') {			/* length = 1 	*/
		opcode = buffer[2] - '0';
		if (opcode == 0) {
			color = 0x0700;
		} else if (opcode == 1) {
			color |= 0x0800;
		} else {
			goto INVALID_BANNER;
		}
	} else if (buffer[4] == 'm') {		/* length = 2	*/
		opcode = (buffer[2] - '0') * 10 + (buffer[3] - '0');
		if (opcode >= 30 && opcode <= 37) {
			color = (color & 0xF8FF) | (color_map[opcode - 30] << 8);
		} else if (opcode >= 40 && opcode <= 47) {
			color = (color & 0x0FFF) | (color_map[opcode - 40] << 12);
		} else {
			goto INVALID_BANNER;
		}
	} else {
		goto INVALID_BANNER;
	}

	it = buffer;
	return FALSE;


INVALID_BANNER:
	for (i = buffer; i != it; i++) {
		vgaputc(*i, TRUE);
	}
	return FALSE;
}