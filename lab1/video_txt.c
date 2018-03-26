#include <minix/drivers.h>
#include <sys/video.h>
#include <sys/mman.h>
#include <assert.h>
#include "vt_info.h"
#include "video_txt.h"

/* Private global variables */

static char *video_mem; /* Address to which VRAM is mapped */

static unsigned scr_width; /* Width of screen in columns */
static unsigned scr_lines; /* Height of screen in lines */

void vt_fill(char ch, char attr) {
	char *vram_ptr;
	for (vram_ptr = video_mem; vram_ptr < video_mem + scr_width * scr_lines * 2;
			vram_ptr++) {
		*vram_ptr++ = ch;
		*vram_ptr = attr;
	}
}

void vt_blank() {
	vt_fill(BLANK_SCR, BLANK_SCR);
}

int vt_print_char(char ch, char attr, int r, int c) {
	if ((c >= scr_width) || (r >= scr_lines)) {
		return 1;
	} else {
		char *ptr = video_mem;
		ptr += 2 * (r * scr_width + c);
		*ptr++ = ch;
		*ptr = attr;
	}
	return 0;
}

int vt_print_string(char *str, char attr, int r, int c) {
	if ((c >= scr_width) || (r >= scr_lines)) {
		return 1;
	} else {
		while (*str != 0) {
			if (vt_print_char(*str, attr, r, c) == 1) {
				if (r >= scr_lines)
					return 1; /* no more available rows, finishes */
				c = 0; /* column 0*/
				r++; /* go to next available row */
			} else {
				str++;
				c++;
			}
		}
	}
	return 0;
}

int vt_print_int(int num, char attr, int r, int c) {

	char str[80]; /* String containing the integer digits */
	char *p = str; /* Character pointer */
	int len = 0; /* String length */

	/* Prints '-' character if number is negative */
	if (num < 0) {
		num = (int) (-1) * (num);
		if (vt_print_char(MINUS_SIGN, attr, r, c++) == 1)/* Adds one to column because "-" sign*/
			return 1;
	}

	while (num != 0) {
		*p = (num % 10) + 0x30; /* Convert least significant digit to character code adding 48 to remainder*/
		len++; /* Increments length */
		num = num / 10; /* Removes least significant digit */
		p++;
	}
	*p = 0; /* Null terminator */

	char *q;
	p = str;
	q = str + len - 1;
	while (p < q) { /* Reverses the string */
		char temp = *p;
		*p = *q;
		*q = temp;
		p++;
		q--;
	}
	/* Prints string */
	if (vt_print_string(str, attr, r, c) == 1)
		return 1;
	return 0;

}

int vt_draw_frame(int width, int height, char attr, int r, int c) {
	int hor_lines = width - 2; // excluding corners
	int ver_lines = height - 2;
	int i;

	if ((c + width) > scr_width || (r + height) > scr_lines)
		return 1;
	else {
		vt_print_char(UL_CORNER, attr, r, c++);
		for (i = 0; i < hor_lines; i++)
			vt_print_char(HOR_BAR, attr, r, c++);
		vt_print_char(UR_CORNER, attr, r++, c);

		for (i = 0; i < ver_lines; i++)
			vt_print_char(VERT_BAR, attr, r++, c);
		vt_print_char(LR_CORNER, attr, r, c--);

		for (i = 0; i < hor_lines; i++)
			vt_print_char(HOR_BAR, attr, r, c--);
		vt_print_char(LL_CORNER, attr, r--, c);

		for (i = 0; i < ver_lines; i++)
			vt_print_char(VERT_BAR, attr, r--, c);
	}
	return 0;
}

/*
 * THIS FUNCTION IS FINALIZED, do NOT touch it
 */

char *vt_init(vt_info_t *vi_p) {

	int r;
	struct mem_range mr;

	/* Allow memory mapping */

	mr.mr_base = (phys_bytes)(vi_p->vram_base);
	mr.mr_limit = mr.mr_base + vi_p->vram_size;

	/**
	 * sys_privctl() to grant a process the permission to map
	 * a given address range you can use MINIX 3 kernel call
	 */
	if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr)))
		panic("video_txt: sys_privctl (ADD_MEM) failed: %d\n", r);

	/* Map memory */

	/**
	 * vm_map_phys() returns the virtual address
	 * (of the first physical memory position)
	 * on which the physical address range was mapped
	 */
	video_mem = vm_map_phys(SELF, (void *) mr.mr_base, vi_p->vram_size);

	if (video_mem == MAP_FAILED)
		panic("video_txt couldn't map video memory");

	/* Save text mode resolution */

	scr_lines = vi_p->scr_lines;
	scr_width = vi_p->scr_width;

	return video_mem;
}
