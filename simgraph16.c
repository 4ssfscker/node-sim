/*
 * Copyright (c) 2001 Hansj�rg Malthaner
 * hansjoerg.malthaner@gmx.de
 *
 * This file is part of the Simugraph engine and may not be used
 * in other projects without written permission of the author.
 *
 * Usage for Iso-Angband is granted.
 */

/* simgraph.c
 *
 * Versuch einer Graphic fuer Simulationsspiele
 * Hj. Malthaner, Aug. 1997
 *
 *
 * 3D, isometrische Darstellung
 *
 *
 *
 * 18.11.97 lineare Speicherung fuer Images -> hoehere Performance
 * 22.03.00 run l�ngen Speicherung fuer Images -> hoehere Performance
 * 15.08.00 dirty tile verwaltung fuer effizientere updates
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <zlib.h>

#include "simtypes.h"
#include "font.h"
#include "pathes.h"
#include "simconst.h"
#include "simsys.h"
#include "simmem.h"
#include "simdebug.h"
#include "besch/bild_besch.h"
#include "unicode.h"


#ifdef _MSC_VER
#	include <io.h>
#	include <direct.h>
#	define W_OK 2
#else
#	include <sys/stat.h>
#	include <fcntl.h>
#	include <unistd.h>
#endif

#include "simgraph.h"


/*
 * Hajo: RGB 1555
 */
typedef uint16 PIXVAL;


#ifdef USE_SOFTPOINTER
static int softpointer = -1;
#endif
static int standard_pointer = -1;


#ifdef USE_SOFTPOINTER
/*
 * Die Icon Leiste muss neu gezeichnet werden wenn der Mauszeiger darueber
 * schwebt
 */
int old_my = -1;
#endif


/* Flag, if we have Unicode font => do unicode (UTF8) support! *
 * @author prissi
 * @date 29.11.04
 */
static bool has_unicode = false;

#ifdef USE_SMALL_FONT
static font_type	large_font, small_font;
#else
static font_type	large_font;
#define small_font (large_font)
#endif

// needed for gui
int large_font_height = 10;

#define LIGHT_COUNT (14)
#define MAX_PLAYER (8)

#define RGBMAPSIZE (0x8000+LIGHT_COUNT+16)

/*
 * Hajo: mapping tables for RGB 555 to actual output format
 * plus the special (player, day&night) colors appended
 *
 * 0x0000 - 0x7FFF: RGB colors
 * 0x8000 - 0x800F: Player colors
 * 0x8010 -       : Day&Night special colors
 */
static PIXVAL rgbmap_day_night[RGBMAPSIZE];


/*
 * Hajo: same as rgbmap_day_night, but allways daytime colors
 */
static PIXVAL rgbmap_all_day[RGBMAPSIZE];


/**
 * Hajo:used by pixel copy functions, is one of rgbmap_day_night
 * rgbmap_all_day
 */
static PIXVAL *rgbmap_current = 0;


/*
 * Hajo: mapping table for specialcolors (AI player colors)
 * to actual output format - day&night mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_day_night[256];


/*
 * Hajo: mapping table for specialcolors (AI player colors)
 * to actual output format - all day mode
 * 16 sets of 16 colors
 */
static PIXVAL specialcolormap_all_day[256];


// offsets of first and second comany color
static uint8 player_offsets[MAX_PLAYER][2];


/*
 * Hajo: Image map descriptor struture
 */
struct imd {
	sint16 base_x; // min x offset
	sint16 base_y; // min y offset
	uint8 base_w; //  width
	uint8 base_h; // height

	sint16 x; // current (zoomed) min x offset
	sint16 y; // current (zoomed) min y offset
	uint8 w; // current (zoomed) width
	uint8 h; // current (zoomed) height

	uint16 len; // base image data size (used for allocation purposes only)

	uint8 recode_flags; // divers flags for recoding
	uint8 player_flags; // 128 = free/needs recode, otherwise coded to this color in player_data

	PIXVAL* data; // current data, zoomed and adapted to output format RGB 555 or RGB 565

	PIXVAL* zoom_data; // zoomed original data

	PIXVAL* base_data; // original image data

	PIXVAL* player_data; // current data coded for player1 (since many building belong to him)
};

// flags for recoding
#define FLAG_ZOOMABLE (1)
#define FLAG_PLAYERCOLOR (2)
#define FLAG_REZOOM (4)
#define FLAG_NORMAL_RECODE (8)

#define NEED_PLAYER_RECODE (128)


static sint16 disp_width  = 640;
static sint16 disp_height = 480;


/*
 * Image table
 */
static struct imd* images = NULL;

/*
 * Number of loaded images
 */
static unsigned int anz_images = 0;

/*
 * Number of allocated entries for images
 * (>= anz_images)
 */
static unsigned int alloc_images = 0;


/*
 * Output framebuffer
 */
static PIXVAL* textur = NULL;


/*
 * Hajo: Current clipping rectangle
 */
static struct clip_dimension clip_rect;


/*
 * Hajo: dirty tile management strcutures
 */
#define DIRTY_TILE_SIZE 16
#define DIRTY_TILE_SHIFT 4

static unsigned char *tile_dirty = NULL;
static unsigned char *tile_dirty_old = NULL;

static int tiles_per_line = 0;
static int tile_lines = 0;
static int tile_buffer_length = 0;


static int light_level = 0;
static int night_shift = -1;


/*
 * Hajo: speical colors during daytime
 */
static const uint8 day_lights[LIGHT_COUNT*3] = {
	0x57,	0x65,	0x6F, // Dark windows, lit yellowish at night
	0x7F,	0x9B,	0xF1, // Lighter windows, lit blueish at night
	0xFF,	0xFF,	0x53, // Yellow light
	0xFF,	0x21,	0x1D, // Red light
	0x01,	0xDD,	0x01, // Green light
	0x6B,	0x6B,	0x6B, // Non-darkening grey 1 (menus)
	0x9B,	0x9B,	0x9B, // Non-darkening grey 2 (menus)
	0xB3,	0xB3,	0xB3, // non-darkening grey 3 (menus)
	0xC9,	0xC9,	0xC9, // Non-darkening grey 4 (menus)
	0xDF,	0xDF,	0xDF, // Non-darkening grey 5 (menus)
	0xE3,	0xE3,	0xFF, // Nearly white light at day, yellowish light at night
	0xC1,	0xB1,	0xD1, // Windows, lit yellow
	0x4D,	0x4D,	0x4D, // Windows, lit yellow
	0xE1,	0x00,	0xE1, // purple light for signals
};


/*
 * Hajo: speical colors during nighttime
 */
static const uint8 night_lights[LIGHT_COUNT*3] = {
	0xD3,	0xC3,	0x80, // Dark windows, lit yellowish at night
	0x80,	0xC3,	0xD3, // Lighter windows, lit blueish at night
	0xFF,	0xFF,	0x53, // Yellow light
	0xFF,	0x21,	0x1D, // Red light
	0x01,	0xDD,	0x01, // Green light
	0x6B,	0x6B,	0x6B, // Non-darkening grey 1 (menus)
	0x9B,	0x9B,	0x9B, // Non-darkening grey 2 (menus)
	0xB3,	0xB3,	0xB3, // non-darkening grey 3 (menus)
	0xC9,	0xC9,	0xC9, // Non-darkening grey 4 (menus)
	0xDF,	0xDF,	0xDF, // Non-darkening grey 5 (menus)
	0xFF,	0xFF,	0xE3, // Nearly white light at day, yellowish light at night
	0xD3,	0xC3,	0x80, // Windows, lit yellow
	0xD3,	0xC3,	0x80, // Windows, lit yellow
	0xE1,	0x00,	0xE1, // purple light for signals
};


// the players colors and colors for simple drawing operations
// each eight colors are corresponding to a player color
static const uint8 special_pal[224*3]=
{
	36, 75, 103,
	57, 94, 124,
	76, 113, 145,
	96, 132, 167,
	116, 151, 189,
	136, 171, 211,
	156, 190, 233,
	176, 210, 255,
	88, 88, 88,
	107, 107, 107,
	125, 125, 125,
	144, 144, 144,
	162, 162, 162,
	181, 181, 181,
	200, 200, 200,
	219, 219, 219,
	17, 55, 133,
	27, 71, 150,
	37, 86, 167,
	48, 102, 185,
	58, 117, 202,
	69, 133, 220,
	79, 149, 237,
	90, 165, 255,
	123, 88, 3,
	142, 111, 4,
	161, 134, 5,
	180, 157, 7,
	198, 180, 8,
	217, 203, 10,
	236, 226, 11,
	255, 249, 13,
	86, 32, 14,
	110, 40, 16,
	134, 48, 18,
	158, 57, 20,
	182, 65, 22,
	206, 74, 24,
	230, 82, 26,
	255, 91, 28,
	34, 59, 10,
	44, 80, 14,
	53, 101, 18,
	63, 122, 22,
	77, 143, 29,
	92, 164, 37,
	106, 185, 44,
	121, 207, 52,
	0, 86, 78,
	0, 108, 98,
	0, 130, 118,
	0, 152, 138,
	0, 174, 158,
	0, 196, 178,
	0, 218, 198,
	0, 241, 219,
	74, 7, 122,
	95, 21, 139,
	116, 37, 156,
	138, 53, 173,
	160, 69, 191,
	181, 85, 208,
	203, 101, 225,
	225, 117, 243,
	59, 41, 0,
	83, 55, 0,
	107, 69, 0,
	131, 84, 0,
	155, 98, 0,
	179, 113, 0,
	203, 128, 0,
	227, 143, 0,
	87, 0, 43,
	111, 11, 69,
	135, 28, 92,
	159, 45, 115,
	183, 62, 138,
	230, 74, 174,
	245, 121, 194,
	255, 156, 209,
	20, 48, 10,
	44, 74, 28,
	68, 99, 45,
	93, 124, 62,
	118, 149, 79,
	143, 174, 96,
	168, 199, 113,
	193, 225, 130,
	54, 19, 29,
	82, 44, 44,
	110, 69, 58,
	139, 95, 72,
	168, 121, 86,
	197, 147, 101,
	226, 173, 115,
	255, 199, 130,
	8, 11, 100,
	14, 22, 116,
	20, 33, 139,
	26, 44, 162,
	41, 74, 185,
	57, 104, 208,
	76, 132, 231,
	96, 160, 255,
	43, 30, 46,
	68, 50, 85,
	93, 70, 110,
	118, 91, 130,
	143, 111, 170,
	168, 132, 190,
	193, 153, 210,
	219, 174, 230,
	63, 18, 12,
	90, 38, 30,
	117, 58, 42,
	145, 78, 55,
	172, 98, 67,
	200, 118, 80,
	227, 138, 92,
	255, 159, 105,
	11, 68, 30,
	33, 94, 56,
	54, 120, 81,
	76, 147, 106,
	98, 174, 131,
	120, 201, 156,
	142, 228, 181,
	164, 255, 207,
	64, 0, 0,
	96, 0, 0,
	128, 0, 0,
	192, 0, 0,
	255, 0, 0,
	255, 64, 64,
	255, 96, 96,
	255, 128, 128,
	0, 128, 0,
	0, 196, 0,
	0, 225, 0,
	0, 240, 0,
	0, 255, 0,
	64, 255, 64,
	94, 255, 94,
	128, 255, 128,
	0, 0, 128,
	0, 0, 192,
	0, 0, 224,
	0, 0, 255,
	0, 64, 255,
	0, 94, 255,
	0, 106, 255,
	0, 128, 255,
	128, 64, 0,
	193, 97, 0,
	215, 107, 0,
	255, 128, 0,
	255, 128, 0,
	255, 149, 43,
	255, 170, 85,
	255, 193, 132,
	8, 52, 0,
	16, 64, 0,
	32, 80, 4,
	48, 96, 4,
	64, 112, 12,
	84, 132, 20,
	104, 148, 28,
	128, 168, 44,
	164, 164, 0,
	193, 193, 0,
	215, 215, 0,
	255, 255, 0,
	255, 255, 32,
	255, 255, 64,
	255, 255, 128,
	255, 255, 172,
	32, 4, 0,
	64, 20, 8,
	84, 28, 16,
	108, 44, 28,
	128, 56, 40,
	148, 72, 56,
	168, 92, 76,
	184, 108, 88,
	64, 0, 0,
	96, 8, 0,
	112, 16, 0,
	120, 32, 8,
	138, 64, 16,
	156, 72, 32,
	174, 96, 48,
	192, 128, 64,
	32, 32, 0,
	64, 64, 0,
	96, 96, 0,
	128, 128, 0,
	144, 144, 0,
	172, 172, 0,
	192, 192, 0,
	224, 224, 0,
	64, 96, 8,
	80, 108, 32,
	96, 120, 48,
	112, 144, 56,
	128, 172, 64,
	150, 210, 68,
	172, 238, 80,
	192, 255, 96,
	32, 32, 32,
	48, 48, 48,
	64, 64, 64,
	80, 80, 80,
	96, 96, 96,
	172, 172, 172,
	236, 236, 236,
	255, 255, 255,
	41, 41, 54,
	60, 45, 70,
	75, 62, 108,
	95, 77, 136,
	113, 105, 150,
	135, 120, 176,
	165, 145, 218,
	198, 191, 232,
};


/*
 * Hajo: tile raster width
 */
int tile_raster_width = 16;
static int base_tile_raster_width = 16;

/*
 * Hajo: Zoom factor
 */
static int zoom_factor = 1;


/* changes the raster width after loading */
int display_set_base_raster_width(int new_raster)
{
	int old = base_tile_raster_width;
	base_tile_raster_width = new_raster;
	tile_raster_width = new_raster / zoom_factor;
	return old;
}


/**
 * Rezooms all images
 * @author Hj. Malthaner
 */
static void rezoom(void)
{
	unsigned int n;

	for (n = 0; n < anz_images; n++) {
		if((images[n].recode_flags & FLAG_ZOOMABLE) != 0 && images[n].base_h > 0) {
			images[n].recode_flags |= FLAG_REZOOM;
		}
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;	// color will be set next time
	}
}


int get_zoom_factor()
{
	return zoom_factor;
}


void set_zoom_factor(int z)
{
	if (z > 0) {
		zoom_factor = z;
		tile_raster_width = base_tile_raster_width / z;
	}
	fprintf(stderr, "set_zoom_factor() : factor=%d\n", zoom_factor);

	rezoom();
}


#ifdef _MSC_VER
#	define inline _inline
#endif


static inline void mark_tile_dirty(const int x, const int y)
{
	const int bit = x + y * tiles_per_line;

#if 0
	assert(bit / 8 < tile_buffer_length);
#endif

	tile_dirty[bit >> 3] |= 1 << (bit & 7);
}


static inline void mark_tiles_dirty(const int x1, const int x2, const int y)
{
	int bit = y * tiles_per_line + x1;
	const int end = bit + x2 - x1;

	do {
#if 0
		assert(bit / 8 < tile_buffer_length);
#endif

		tile_dirty[bit >> 3] |= 1 << (bit & 7);
	} while (++bit <= end);
}


static inline int is_tile_dirty(const int x, const int y)
{
	const int bit = x + y * tiles_per_line;
	const int bita = bit >> 3;
	const int bitb = 1 << (bit & 7);

	return (tile_dirty[bita] | tile_dirty_old[bita]) & bitb;
}


/**
 * Markiert ein Tile as schmutzig, ohne Clipping
 * @author Hj. Malthaner
 */
static void mark_rect_dirty_nc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2)
{
	// floor to tile size
	x1 >>= DIRTY_TILE_SHIFT;
	y1 >>= DIRTY_TILE_SHIFT;
	x2 >>= DIRTY_TILE_SHIFT;
	y2 >>= DIRTY_TILE_SHIFT;

#if 0
	assert(x1 >= 0);
	assert(x1 < tiles_per_line);
	assert(y1 >= 0);
	assert(y1 < tile_lines);
	assert(x2 >= 0);
	assert(x2 < tiles_per_line);
	assert(y2 >= 0);
	assert(y2 < tile_lines);
#endif

	for (; y1 <= y2; y1++) mark_tiles_dirty(x1, x2, y1);
}


/**
 * Markiert ein Tile as schmutzig, mit Clipping
 * @author Hj. Malthaner
 */
void mark_rect_dirty_wc(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL x2, KOORD_VAL y2)
{
	// inside display?
	if (x2 >= 0 && y2 >= 0 && x1 < disp_width && y1 < disp_height) {
		if (x1 < 0) x1 = 0;
		if (y1 < 0) y1 = 0;
		if (x2 >= disp_width)  x2 = disp_width  - 1;
		if (y2 >= disp_height) y2 = disp_height - 1;
		mark_rect_dirty_nc(x1, y1, x2, y2);
	}
}


static uint8 player_night=0;
static uint8 player_day=0;
static void	activate_player_color(sint8 player_nr, bool daynight)
{
	// cahces the last settings
	if(!daynight) {
		if(player_day!=player_nr) {
			int i;
			player_day = player_nr;
			for(i=0;  i<8;  i++  ) {
				rgbmap_all_day[0x8000+i] = specialcolormap_all_day[player_offsets[player_day][0]+i];
				rgbmap_all_day[0x8008+i] = specialcolormap_all_day[player_offsets[player_day][1]+i];
			}
		}
		rgbmap_current = rgbmap_all_day;
	}
	else {
		// chaning colortable
		if(player_night!=player_nr) {
			int i;
			player_night = player_nr;
			for(i=0;  i<8;  i++  ) {
				rgbmap_day_night[0x8000+i] = specialcolormap_day_night[player_offsets[player_night][0]+i];
				rgbmap_day_night[0x8008+i] = specialcolormap_day_night[player_offsets[player_night][1]+i];
			}
		}
		rgbmap_current = rgbmap_day_night;
	}
}



/**
 * Convert image data to actual output data
 * @author Hj. Malthaner
 */
static void recode(void)
{
	unsigned n;

	for (n = 0; n < anz_images; n++) {
		// tut jetzt on demand recode_img() f�r jedes Bild einzeln
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;
	}
}


/**
 * Convert a certain image data to actual output data
 * @author Hj. Malthaner
 */
static void recode_img_src_target(KOORD_VAL h, PIXVAL *src, PIXVAL *target)
{
	if (h > 0) {
		do {
			unsigned char runlen = *target++ = *src++;

			// eine Zeile dekodieren
			do {
				// clear run is always ok

				runlen = *target++ = *src++;
				while (runlen--) {
					// now just convert the color pixels
					*target++ = rgbmap_day_night[*src++];
				}
			} while ((runlen = *target++ = *src++));
		} while (--h);
	}
}


/**
 * Handles the conversion of an image to the output color
 * @author prissi
 */
static void recode_normal_img(const unsigned int n)
{
	PIXVAL *src = images[n].base_data;

	// then use the right data
	if (zoom_factor > 1) {
		if (images[n].zoom_data != NULL) src = images[n].zoom_data;
	}
	if (images[n].data == NULL) {
		images[n].data = (PIXVAL*)guarded_malloc(sizeof(PIXVAL) * images[n].len);
	}
	// now do normal recode
	activate_player_color( 0, true );
	recode_img_src_target(images[n].h, src, images[n].data);
	images[n].recode_flags &= ~FLAG_NORMAL_RECODE;
}


#ifdef NEED_PLAYER_RECODE

/**
 * Convert a certain image data to actual output data for a certain player
 * @author prissi
 */
static void recode_img_src_target_color(KOORD_VAL h, PIXVAL *src, PIXVAL *target)
{
	if (h > 0) {
		do {
			unsigned char runlen = *target++ = *src++;
			// eine Zeile dekodieren

			do {
				// clear run is always ok

				runlen = *target++ = *src++;
				// now just convert the color pixels
				while (runlen--) {
					*target++ = rgbmap_day_night[*src++];
				}
				// next clea run or zero = end
			} while ((runlen = *target++ = *src++));
		} while (--h);
	}
}


/**
 * Handles the conversion of an image to the output color
 * @author prissi
 */
static void recode_color_img(const unsigned int n, const unsigned char player_nr)
{
	PIXVAL *src = images[n].base_data;

	images[n].player_flags = player_nr;
	// then use the right data
	if (zoom_factor > 1) {
		if (images[n].zoom_data != NULL) src = images[n].zoom_data;
	}
	// second recode modi, for player one (city and factory AI)
	if (images[n].player_data == NULL) {
		images[n].player_data = (PIXVAL*)guarded_malloc(sizeof(PIXVAL) * images[n].len);
	}
	// contains now the player color ...
	activate_player_color( player_nr, true );
	recode_img_src_target_color(images[n].h, src, images[n].player_data );
}

#endif


/**
 * Convert base image data to actual image size
 * @author prissi (to make this much faster) ...
 */
static void rezoom_img(const unsigned int n)
{
	// Hajo: may this image be zoomed
	if (n < anz_images && images[n].base_h > 0) {
		// we may need night conversion afterwards
		images[n].recode_flags &= ~FLAG_REZOOM;
		images[n].recode_flags |= FLAG_NORMAL_RECODE;
		images[n].player_flags = NEED_PLAYER_RECODE;

		// just restore original size?
		if (zoom_factor <= 1  ||  (images[n].recode_flags&FLAG_ZOOMABLE)==0) {
			// this we can do be a simple copy ...
			images[n].x = images[n].base_x;
			images[n].w = images[n].base_w;
			images[n].y = images[n].base_y;
			images[n].h = images[n].base_h;
			if (images[n].zoom_data != NULL) {
				guarded_free(images[n].zoom_data);
				images[n].zoom_data = NULL;
			}
			return;
		}

		// now we want to downsize the image
		// just divede the sizes
		images[n].x = images[n].base_x / zoom_factor;
		images[n].y = images[n].base_y / zoom_factor;
		images[n].w = (images[n].base_x + images[n].base_w) / zoom_factor - images[n].x;
		images[n].h = images[n].base_h / zoom_factor;

		if (images[n].h > 0 && images[n].w > 0) {
			// just recalculate the image in the new size
			unsigned char y_left = (images[n].base_y + zoom_factor - 1) % zoom_factor;
			unsigned char h = images[n].base_h;

			static PIXVAL line[512];
			PIXVAL *src = images[n].base_data;
			PIXVAL *dest, *last_dest;

			// decode/recode linewise
			unsigned int last_color = 255; // ==255 to keep compiler happy

			if (images[n].zoom_data == NULL) {
				// normal len is ok, since we are only skipping parts ...
				images[n].zoom_data = guarded_malloc(sizeof(PIXVAL) * images[n].len);
			}
			last_dest = dest = images[n].zoom_data;

			do { // decode/recode line
				unsigned int runlen;
				unsigned int color = 0;
				PIXVAL *p = line;
				const int imgw = images[n].base_x + images[n].base_w;

				// left offset, which was left by division
				runlen = images[n].base_x%zoom_factor;
				while (runlen--) *p++ = 0x73FE;

				// decode line
				runlen = *src++;
				color -= runlen;
				do {
					// clear run
					while (runlen--) *p++ = 0x73FE;
					// color pixel
					runlen = *src++;
					color += runlen;
					while (runlen--) *p++ = *src++;
				} while ((runlen = *src++));

				if (y_left == 0 || last_color < color) {
					// required; but if the following are longer, take them instead (aviods empty pixels)
					// so we have to set/save the beginning
					unsigned char step = 0;
					unsigned char i;

					if (y_left == 0) {
						last_dest = dest;
					} else {
						dest = last_dest;
					}

					// encode this line
					do {
						// check length of transparent pixels
						for (i = 0; line[step] == 0x73FE && step < imgw; i++, step += zoom_factor) {}
						*dest++ = i;
						// chopy for non-transparent
						for (i = 0; line[step] != 0x73FE && step < imgw; i++, step += zoom_factor) {
							dest[i + 1] = line[step];
						}
						*dest++ = i;
						dest += i;
					} while (step < imgw);
					*dest++ = 0; // mark line end
					if (y_left == 0) {
						// ok now reset countdown counter
						y_left = zoom_factor;
					}
					last_color = color;
				}

				// countdown line skip counter
				y_left--;
			} while (--h != 0);

			// something left?
			{
				const unsigned int zoom_len = dest - images[n].zoom_data;
				if (zoom_len > images[n].len) {
					printf("*** FATAL ***\nzoom_len (%i) > image_len (%i)", zoom_len, images[n].len);
					fflush(NULL);
					exit(0);
				}
				if (zoom_len == 0) images[n].h = 0;
			}
		} else {
			if (images[n].w <= 0) {
				// h=0 will be ignored, with w=0 there was an error!
				printf("WARNING: image%d w=0!\n", n);
			}
			images[n].h = 0;
		}
	}
}


int	display_set_unicode(int use_unicode)
{
	return has_unicode = (use_unicode != 0);
}


/**
 * Loads the fonts (true for large font)
 * @author prissi
 */
bool display_load_font(const char* fname, bool large)
{
	if (load_font(large ? &large_font : &small_font, fname)) {
		if (large) large_font_height = large_font.height;
		return true;
	} else {
		return false;
	}
}


#ifdef USE_SMALL_FONT

/* copy a font
 * @author prissi
 */
static void copy_font(font_type *dest_font, font_type *src_font)
{
	memcpy(dest_font, src_font, sizeof(font_type));
	dest_font->char_data = malloc(dest_font->num_chars * 16);
	memcpy(dest_font->char_data, src_font->char_data, 16 * dest_font->num_chars);
	dest_font->screen_width = malloc(dest_font->num_chars);
	memcpy(dest_font->screen_width, src_font->screen_width, dest_font->num_chars);
}


/* checks if a small and a large font exists;
 * if not the missing font will be emulated
 * @author prissi
 */
void display_check_fonts(void)
{
	// replace missing font, if none there (better than crashing ... )
	if (small_font.screen_width == NULL && large_font.screen_width != NULL) {
		printf("Warning: replacing small  with large font!\n");
		copy_font(&small_font, &large_font);
	}
	if (large_font.screen_width == NULL && small_font.screen_width != NULL) {
		printf("Warning: replacing large  with small font!\n");
		copy_font(&large_font, &small_font);
	}
}

#endif


sint16 display_get_width(void)
{
	return disp_width;
}


sint16 display_get_height(void)
{
	return disp_height;
}


sint16 display_set_height(KOORD_VAL h)
{
	sint16 old = disp_height;

	disp_height = h;
	return old;
}


/**
 * Holt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
int display_get_light(void)
{
	return light_level;
}


#if 0
/* Hajos variant */
static void calc_base_pal_from_night_shift(const int night)
{
	const int day = 4 - night;
	int i;

	for (i = 0; i < (1 << 15); i++) {
		int R;
		int G;
		int B;

		// RGB 555 input
		R = ((i & 0x7C00) >> 7) - night * 24;
		G = ((i & 0x03E0) >> 2) - night * 24;
		B = ((i & 0x001F) << 3) - night * 16;

		rgbmap_day_night[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// player 0 colors, unshaded
	for (i = 0; i < 4; i++) {
		const int R = day_pal[i * 3];
		const int G = day_pal[i * 3 + 1];
		const int B = day_pal[i * 3 + 2];

		rgbcolormap[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// player 0 colors, shaded
	for (i = 0; i < 4; i++) {
		const int R = day_pal[i * 3]     - night * 24;
		const int G = day_pal[i * 3 + 1] - night * 24;
		const int B = day_pal[i * 3 + 2] - night * 16;

		specialcolormap_day_night[i] = rgbmap_day_night[0x8000 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// Lights
	for (i = 0; i < LIGHT_COUNT; i++) {
		const int day_R =  day_lights[i] >> 16;
		const int day_G = (day_lights[i] >> 8) & 0xFF;
		const int day_B =  day_lights[i] & 0xFF;

		const int night_R =  night_lights[i] >> 16;
		const int night_G = (night_lights[i] >> 8) & 0xFF;
		const int night_B =  night_lights[i] & 0xFF;

		const int R = (day_R * day + night_R * night) >> 2;
		const int G = (day_G * day + night_G * night) >> 2;
		const int B = (day_B * day + night_B * night) >> 2;

		rgbmap_day_night[0x8004 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}


	// transform special colors
	for (i = 4; i < 32; i++) {
		int R = special_pal[i * 3]     - night * 24;
		int G = special_pal[i * 3 + 1] - night * 24;
		int B = special_pal[i * 3 + 2] - night * 16;

		specialcolormap_day_night[i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// convert to RGB xxx
	recode();
}
#endif


/* Tomas variant */
static void calc_base_pal_from_night_shift(const int night)
{
	const int day = 4 - night;
	unsigned int i;

	// constant multiplier 0,66 - dark night  255 will drop to 49, 55 to 10
	//                     0,7  - dark, but all is visible     61        13
	//                     0,73                                72        15
	//                     0,75 - quite bright                 80        17
	//                     0,8    bright                      104        22
#if 0
	const double RG_nihgt_multiplier = pow(0.73, night);
	const double B_nihgt_multiplier  = pow(0.82, night);
#endif

	const double RG_nihgt_multiplier = pow(0.75, night) * ((light_level + 8.0) / 8.0);
	const double B_nihgt_multiplier  = pow(0.83, night) * ((light_level + 8.0) / 8.0);

	for (i = 0; i < 0x8000; i++) {
		// (1<<15) this is total no of all possible colors in RGB555)
		// RGB 555 input
		int R = (i & 0x7C00) >> 7;
		int G = (i & 0x03E0) >> 2;
		int B = (i & 0x001F) << 3;
		// lines generate all possible colors in 555RGB code - input
		// however the result is in 888RGB - 8bit per channel

		R = (int)(R * RG_nihgt_multiplier);
		G = (int)(G * RG_nihgt_multiplier);
		B = (int)(B * B_nihgt_multiplier);

		rgbmap_day_night[i] = get_system_color(R, G, B);
	}

	// player color map (and used for map display etc.)
	for (i = 0; i < 224; i++) {
		const int R = (int)(special_pal[i*3 + 0] * RG_nihgt_multiplier);
		const int G = (int)(special_pal[i*3 + 1] * RG_nihgt_multiplier);
		const int B = (int)(special_pal[i*3 + 2] * B_nihgt_multiplier);

		specialcolormap_day_night[i] = get_system_color(R, G, B);
	}
	// special light colors (actually, only non-darkening greys should be used
	for(i=0;  i<LIGHT_COUNT;  i++  ) {
		specialcolormap_day_night[i+224] = get_system_color( day_lights[i*3 + 0], day_lights[i*3 + 1], 	day_lights[i*3 + 2] );
	}
	// init with black for forbidden colors
	for(i=224+LIGHT_COUNT;  i<256;  i++  ) {
		specialcolormap_day_night[i] = 0;
	}
	// default player colors
	for(i=0;  i<8;  i++  ) {
		rgbmap_day_night[0x8000+i] = specialcolormap_day_night[player_offsets[0][0]+i];
		rgbmap_day_night[0x8008+i] = specialcolormap_day_night[player_offsets[0][1]+i];
	}
	player_night = 0;

	// Lights
	for (i = 0; i < LIGHT_COUNT; i++) {
		const int day_R = day_lights[i*3+0];
		const int day_G = day_lights[i*3+1];
		const int day_B = day_lights[i*3+2];

		const int night_R = night_lights[i*3+0];
		const int night_G = night_lights[i*3+1];
		const int night_B = night_lights[i*3+2];

		const int R = (day_R * day + night_R * night) >> 2;
		const int G = (day_G * day + night_G * night) >> 2;
		const int B = (day_B * day + night_B * night) >> 2;

		rgbmap_day_night[0x8010 + i] =
			get_system_color(R > 0 ? R : 0, G > 0 ? G : 0, B > 0 ? B : 0);
	}

	// convert to RGB xxx
	recode();
}


/**
 * Setzt Helligkeitseinstellungen
 * @author Hj. Malthaner
 */
void display_set_light(int new_light_level)
{
	light_level = new_light_level;
	calc_base_pal_from_night_shift(night_shift);
	recode();
}


void display_day_night_shift(int night)
{
	if (night != night_shift) {
		night_shift = night;
		calc_base_pal_from_night_shift(night);
		mark_rect_dirty_nc(0, 32, disp_width - 1, disp_height - 1);
	}
}



// set first and second company color for player
void display_set_player_color_scheme(const int player, const COLOR_VAL col1, const COLOR_VAL col2 )
{
	if(player_offsets[player][0]!=col1  ||  player_offsets[player][1]!=col2) {
		// set new player colors
		player_offsets[player][0] = col1;
		player_offsets[player][1] = col2;
		if(player==player_day  ||  player==player_night) {
			// and recalculate map (and save it)
			calc_base_pal_from_night_shift(0);
			memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));
			if(night_shift!=0) {
				calc_base_pal_from_night_shift(night_shift);
			}
			player_day = player_night = player;
			recode();
		}
		mark_rect_dirty_nc(0, 32, disp_width - 1, disp_height - 1);
	}
}


/**
 * F�gt ein Image aus anderer Quelle hinzu
 */
void register_image(struct bild_t* bild)
{
	struct imd* image;

	/* valid image? */
	if (bild->len == 0 || bild->h == 0) {
		fprintf(stderr, "Warning: ignoring image %d because of missing data\n", anz_images);
		bild->bild_nr = -1;
		return;
	}

	if (anz_images >= 65535) {
		fprintf(stderr, "FATAL:\n*** Out of images (more than 65534!) ***\n");
		abort();
	}

	if (anz_images == alloc_images) {
		alloc_images += 128;
		images = guarded_realloc(images, sizeof(*images) * alloc_images);
	}

	bild->bild_nr = anz_images;
	image = &images[anz_images];
	anz_images++;

	image->x = bild->x;
	image->w = bild->w;
	image->y = bild->y;
	image->h = bild->h;

	image->base_x = bild->x;
	image->base_w = bild->w;
	image->base_y = bild->y;
	image->base_h = bild->h;

	image->len = bild->len;

	// allocate and copy if needed
	image->recode_flags = FLAG_NORMAL_RECODE | FLAG_REZOOM | (bild->zoomable & FLAG_ZOOMABLE);
#ifdef NEED_PLAYER_RECODE
	image->player_flags = NEED_PLAYER_RECODE;
#endif

	image->base_data = NULL;
	image->zoom_data = NULL;
	image->data = NULL;
	image->player_data = NULL;	// chaches data for one AI

	// since we do not recode them, we can work with the original data
	image->base_data = (PIXVAL*)(bild + 1);

	// does this image have color?
	if (bild->h > 0) {
		int h = bild->h;
		const PIXVAL *src = image->base_data;

		do {
			src++; // offset of first start
			do {
				PIXVAL runlen;

				for (runlen = *src++; runlen != 0; runlen--) {
					PIXVAL pix = *src++;

					if (pix>=0x8000  &&  pix<=0x800F) {
						image->recode_flags |= FLAG_PLAYERCOLOR;
						return;
					}
				}
				// next clear run or zero = end
			} while (*src++ != 0);
		} while (--h != 0);
	}
}


// prissi: query offsets
void display_get_image_offset(unsigned bild, int *xoff, int *yoff, int *xw, int *yw)
{
	if (bild < anz_images) {
		*xoff = images[bild].base_x;
		*yoff = images[bild].base_y;
		*xw   = images[bild].base_w;
		*yw   = images[bild].base_h;
	}
}


// prissi: canges the offset of an image
// we need it this complex, because the actual x-offset is coded into the image
void display_set_image_offset(unsigned bild, int xoff, int yoff)
{
	int h;
	PIXVAL* sp;

	if(bild >= anz_images) {
		fprintf(stderr, "Warning: display_set_image_offset(): illegal image=%d\n", bild);
		return;
	}

	assert(images[bild].base_h > 0);
	assert(images[bild].base_w > 0);

	h = images[bild].base_h;
	sp = images[bild].base_data;

	// avoid overflow
	if (images[bild].base_x + xoff < 0) {
		xoff = -images[bild].base_x;
	}
	images[bild].base_x += xoff;
	images[bild].base_y += yoff;
	// the offfset is hardcoded into the image
	while (h-- > 0) {
		// one line
		*sp += xoff;	// reduce number of transparent pixels (always first)
		do {
			// clear run + colored run + next clear run
			++sp;
			sp += *sp + 1;
		} while (*sp);
		sp++;
	}
	// need recoding
	images[anz_images].recode_flags |= FLAG_NORMAL_RECODE;
#ifdef NEED_PLAYER_RECODE
	images[anz_images].player_flags = NEED_PLAYER_RECODE;
#endif
	images[anz_images].recode_flags |= FLAG_REZOOM;
}


/**
 * Holt Maus X-Position
 * @author Hj. Malthaner
 */
int gib_maus_x(void)
{
	return sys_event.mx;
}


/**
 * Holt Maus y-Position
 * @author Hj. Malthaner
 */
int gib_maus_y(void)
{
	return sys_event.my;
}


/**
 * this sets width < 0 if the range is out of bounds
 * so check the value after calling and before displaying
 * @author Hj. Malthaner
 */
static int clip_wh(KOORD_VAL *x, KOORD_VAL *width, const KOORD_VAL min_width, const KOORD_VAL max_width)
{
	if (*x < min_width) {
		const KOORD_VAL xoff = min_width - *x;

		*width += *x-min_width;
		*x = min_width;

		if (*x + *width >= max_width) *width = max_width - *x;

		return xoff;
	} else if (*x + *width >= max_width) {
		*width = max_width - *x;
	}
	return 0;
}


/**
 * places x and w within bounds left and right
 * if nothing to show, returns FALSE
 * @author Niels Roest
 */
static int clip_lr(KOORD_VAL *x, KOORD_VAL *w, const KOORD_VAL left, const KOORD_VAL right)
{
	const KOORD_VAL l = *x;      // leftmost pixel
	const KOORD_VAL r = *x + *w; // rightmost pixel

	if (l > right || r < left) {
		*w = 0;
		return FALSE;
	}

	// there is something to show.
	if (l < left) {
		*w -= left - l;
		*x = left;
	}
	if (r > right) {
		*w -= r - right;
	}
	return *w > 0;
}


/**
 * Ermittelt Clipping Rechteck
 * @author Hj. Malthaner
 */
struct clip_dimension display_gib_clip_wh(void)
{
	return clip_rect;
}


/**
 * Setzt Clipping Rechteck
 * @author Hj. Malthaner
 */
void display_setze_clip_wh(KOORD_VAL x, KOORD_VAL y, KOORD_VAL w, KOORD_VAL h)
{
	clip_wh(&x, &w, 0, disp_width);
	clip_wh(&y, &h, 0, disp_height);

	clip_rect.x = x;
	clip_rect.y = y;
	clip_rect.w = w;
	clip_rect.h = h;

	clip_rect.xx = x + w;
	clip_rect.yy = y + h;
}



// ----------------- basic painting procedures ----------------


// scrolls horizontally, will ignore clipping etc.
void display_scroll_band(const KOORD_VAL start_y, const KOORD_VAL x_offset, const KOORD_VAL h)
{
	const PIXVAL* src = textur + start_y * disp_width + x_offset;
	PIXVAL* dst = textur + start_y * disp_width;
	size_t amount = sizeof(PIXVAL) * (h * disp_width - x_offset);

#ifdef USE_C
	memmove(dst, src, amount);
#else
	amount /= 4;
	asm volatile (
		"rep\n\t"
		"movsl\n\t"
		: "+D" (dst), "+S" (src), "+c" (amount)
	);
#endif
}


/************ display all kind of images from here on ********/


/**
 * Kopiert Pixel von src nach dest
 * @author Hj. Malthaner
 */
static inline void pixcopy(PIXVAL *dest, const PIXVAL *src, const unsigned int len)
{
	// for gcc this seems to produce the optimal code ...
	const PIXVAL *const end = dest + len;

	while (dest < end) *dest++ = *src++;
}


/**
 * Kopiert Pixel, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
static inline void colorpixcopy(PIXVAL *dest, const PIXVAL *src, const PIXVAL * const end)
{
	while (src < end) {
		*dest++ = rgbmap_current[*src++];
	}
}


/**
 * Zeichnet Bild mit horizontalem Clipping
 * @author Hj. Malthaner
 */
static void display_img_wc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			int runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen >= clip_rect.x && xpos <= clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx - xpos >= runlen ? runlen : clip_rect.xx - xpos);

					pixcopy(tp + xpos + left, sp + left, len - left);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/**
 * Zeichnet Bild ohne jedes Clipping
 */
static void display_img_nc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp)
{
	if (h > 0) {
		PIXVAL *tp = textur + xp + yp * disp_width;

		do { // zeilen dekodieren
			uint32 runlen = *sp++;
			PIXVAL *p = tp;

			// eine Zeile dekodieren
			do {
				// wir starten mit einem clear run
				p += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;
#if USE_C
#if 1
				{
					// "classic" C code (why is it faster!?!)
					const uint32 *ls;
					uint32 *ld;

					if (runlen & 1) {
						*p++ = *sp++;
					}

					ls = (const uint32 *)sp;
					ld = (uint32 *)p;
					runlen >>= 1;
					while (runlen--) {
						*ld++ = *ls++;
					}
					p = (PIXVAL*)ld;
					sp = (const PIXVAL*)ls;
				}
#else
				// some architectures: faster with inline of memory functions!
				memcpy( p, sp, runlen*sizeof(PIXVAL) );
				sp += runlen;
#endif
#else
				// this code is sometimes slower, mostly 5% faster, not really clear why and when (cache alignment?)
				asm volatile (
					"cld\n\t"
					// rep movsw and we would be finished, but we unroll
					// uneven number of words to copy
					"shrl %2\n\t"
					"jnc 0f\n\t"
					// Copy first word
					// *p++ = *sp++;
					"movsw\n\t"
					"0:\n\t"
					"negl %2\n\t"
					"addl $1f, %2\n\t"
					"jmp * %2\n\t"
					".p2align 2\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"

					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n\t"
					"movsd\n"

					"1:\n\t"

					: "+D" (p), "+S" (sp), "+r" (runlen)
					:
					: "cc"
				);
#endif
				runlen = *sp++;
			} while (runlen != 0);

			tp += disp_width;
		} while (--h != 0);
	}
}


/**
 * Zeichnet Bild mit verticalem clipping (schnell) und horizontalem (langsam)
 * @author prissi
 */
void display_img_aux(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const int dirty, bool use_player)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		PIXVAL *sp;
		KOORD_VAL h, reduce_h, skip_lines;

		if (use_player) {
			sp = images[n].player_data;
			if (sp == NULL) {
				printf("CImg %i failed!\n", n);
				return;
			}
		} else {
			if (images[n].recode_flags&FLAG_REZOOM) {
				rezoom_img(n);
				recode_normal_img(n);
			} else if (images[n].recode_flags&FLAG_NORMAL_RECODE) {
				recode_normal_img(n);
			}
			sp = images[n].data;
			if (sp == NULL) {
				printf("Img %i failed!\n", n);
				return;
			}
		}
		// now, since zooming may have change this image
		yp += images[n].y;
		h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		reduce_h = yp + h - clip_rect.yy;
		if (reduce_h > 0) h -= reduce_h;
		// still something to draw
		if (h <= 0) return;

		// vertically lines to skip (only bottom is visible
		skip_lines = clip_rect.y - (int)yp;
		if (skip_lines > 0) {
			if (skip_lines >= h) {
				// not visible at all
				return;
			}
			h -= skip_lines;
			yp += skip_lines;
			// now skip them
			while (skip_lines--) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += *sp + 1;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const KOORD_VAL x = images[n].x;
			const KOORD_VAL w = images[n].w;

			// use horzontal clipping or skip it?
			if (xp + x >= clip_rect.x && xp + x + w - 1 <= clip_rect.xx) {
				// marking change?
				if (dirty) mark_rect_dirty_nc(xp + x, yp, xp + x + w - 1, yp + h - 1);
				display_img_nc(h, xp, yp, sp);
			} else if (xp <= clip_rect.xx && xp + x + w > clip_rect.x) {
				display_img_wc(h, xp, yp, sp);
				// since height may be reduced, start marking here
				if (dirty) mark_rect_dirty_wc(xp + x, yp, xp + x + w - 1, yp + h - 1);
			}
		}
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * assumes height is ok and valid data are caluclated
 * @author hajo/prissi
 */
static void display_color_img_aux(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp)
{
	KOORD_VAL h = images[n].h;
	KOORD_VAL y = yp + images[n].y;
	KOORD_VAL yoff = clip_wh(&y, &h, clip_rect.y, clip_rect.yy);

	if (h > 0) { // clipping may have reduced it
		// color replacement needs the original data
		const PIXVAL *sp = (zoom_factor > 1 && images[n].zoom_data != NULL) ? images[n].zoom_data : images[n].base_data;
		PIXVAL *tp = textur + y * disp_width;

		// oben clippen
		while (yoff) {
			yoff--;
			do {
				// clear run + colored run + next clear run
				++sp;
				sp += *sp + 1;
			} while (*sp);
			sp++;
		}

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen

			int runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen >= clip_rect.x && xpos <= clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx-xpos > runlen ? runlen : clip_rect.xx - xpos);

					colorpixcopy(tp + xpos + left, sp + left, sp + len);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}


/**
 * Zeichnet Bild, ersetzt Spielerfarben
 * @author Hj. Malthaner
 */
void display_color_img(const unsigned n, const KOORD_VAL xp, const KOORD_VAL yp, const sint8 player_nr, const int daynight, const int dirty)
{
	if (n < anz_images) {
		// Hajo: since the colors for player 0 are already right,
		// only use the expensive replacement routine for colored images
		// of other players
		if ((daynight  ||  night_shift==0) && (player_nr<=0  || (images[n].recode_flags & FLAG_PLAYERCOLOR) == 0)) {
			display_img_aux(n, xp, yp, dirty, false);
			return;
		}

		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
		}

		{
			// first test, if there is a cached version (or we can built one ... )
			const unsigned char player_flag = images[n].player_flags&0x7F;
			if (daynight && (player_flag == 0 || player_flag == player_nr)) {
				if (images[n].player_flags== 128 || player_flag == 0) {
					recode_color_img(n, player_nr);
				}
				// ok, now we could use the same faster code as for the normal images
				display_img_aux(n, xp, yp, dirty, true);
				return;
			}
		}

		// prissi: now test if visible and clipping needed
		{
			const KOORD_VAL x = images[n].x;
			const KOORD_VAL y = images[n].y;
			const KOORD_VAL w = images[n].w;
			const KOORD_VAL h = images[n].h;

			if (h == 0 || xp > clip_rect.xx || yp + y > clip_rect.yy || xp + x + w <= clip_rect.x || yp + y + h <= clip_rect.y) {
				// not visible => we are done
				// happens quite often ...
				return;
			}

			if (dirty) {
				mark_rect_dirty_wc(xp + x, yp + y, xp + x + w - 1, yp + y + h - 1);
			}
		}

		// colors for 2nd company color
		if(player_nr>=0) {
			activate_player_color( player_nr, daynight );
		}
		else {
			// no player
			activate_player_color( 0, daynight );
		}
		display_color_img_aux(n, xp, yp );
	} // number ok
}



/* from here code for transparent images */
typedef void (*blend_proc)(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len);

// different masks needed for RGB 555 and RGB 565
#define ONE_OUT_16 (0x7bef)
#define TWO_OUT_16 (0x39E7)
#define ONE_OUT_15 (0x3DEF)
#define TWO_OUT_15 (0x1CE7)

static void pix_blend75_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*(((*src)>>2) & TWO_OUT_15)) + (((*dest)>>2) & TWO_OUT_15);
		dest++;
		src++;
	}
}
static void pix_blend75_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*(((*src)>>2) & TWO_OUT_16)) + (((*dest)>>2) & TWO_OUT_16);
		dest++;
		src++;
	}
}

static void pix_blend50_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>1) & ONE_OUT_15) + (((*dest)>>1) & ONE_OUT_15);
		dest++;
		src++;
	}
}
static void pix_blend50_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>1) & ONE_OUT_16) + (((*dest)>>1) & ONE_OUT_16);
		dest++;
		src++;
	}
}

static void pix_blend25_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>2) & TWO_OUT_15) + (3*(((*dest)>>2) & TWO_OUT_15));
		dest++;
		src++;
	}
}
static void pix_blend25_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (((*src)>>2) & TWO_OUT_16) + (3*(((*dest)>>2) & TWO_OUT_16));
		dest++;
		src++;
	}
}

static void pix_outline75_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*((colour>>2) & TWO_OUT_15)) + (((*dest)>>2) & TWO_OUT_15);
		dest++;
	}
}
static void pix_outline75_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = (3*((colour>>2) & TWO_OUT_16)) + (((*dest)>>2) & TWO_OUT_16);
		dest++;
	}
}

static void pix_outline50_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>1) & ONE_OUT_15) + (((*dest)>>1) & ONE_OUT_15);
		dest++;
	}
}
static void pix_outline50_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>1) & ONE_OUT_16) + (((*dest)>>1) & ONE_OUT_16);
		dest++;
	}
}

static void pix_outline25_15(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>2) & TWO_OUT_15) + (3*(((*dest)>>2) & TWO_OUT_15));
		dest++;
	}
}
static void pix_outline25_16(PIXVAL *dest, const PIXVAL *src, const PIXVAL colour, const PIXVAL len)
{
	const PIXVAL *const end = dest + len;
	while (dest < end) {
		*dest = ((colour>>2) & TWO_OUT_16) + (3*(((*dest)>>2) & TWO_OUT_16));
		dest++;
	}
}

// will kept the actual values
static blend_proc blend[3];
static blend_proc outline[3];

static void display_img_blend_wc(KOORD_VAL h, const KOORD_VAL xp, const KOORD_VAL yp, const PIXVAL *sp, int colour,
	blend_proc p )
{
	if (h > 0) {
		PIXVAL *tp = textur + yp * disp_width;

		do { // zeilen dekodieren
			int xpos = xp;

			// bild darstellen
			int runlen = *sp++;

			do {
				// wir starten mit einem clear run
				xpos += runlen;

				// jetzt kommen farbige pixel
				runlen = *sp++;

				// Hajo: something to display?
				if (xpos + runlen >= clip_rect.x && xpos <= clip_rect.xx) {
					const int left = (xpos >= clip_rect.x ? 0 : clip_rect.x - xpos);
					const int len  = (clip_rect.xx - xpos >= runlen ? runlen : clip_rect.xx - xpos);
					(*p)(tp + xpos + left, sp + left, colour, len - left);
				}

				sp += runlen;
				xpos += runlen;
			} while ((runlen = *sp++));

			tp += disp_width;
		} while (--h);
	}
}

/**
 * draws the transparent outline of an image
 * @author kierongreen
 */
void display_img_blend(const unsigned n, const KOORD_VAL xp, KOORD_VAL yp, const PLAYER_COLOR_VAL color_index, const int daynight, const int dirty)
{
	if (n < anz_images) {
		// need to go to nightmode and or rezoomed?
		PIXVAL *sp;
		KOORD_VAL h, reduce_h, skip_lines;

		if (images[n].recode_flags&FLAG_REZOOM) {
			rezoom_img(n);
			recode_normal_img(n);
		} else if (images[n].recode_flags&FLAG_NORMAL_RECODE) {
			recode_normal_img(n);
		}
		sp = images[n].data;

		// now, since zooming may have change this image
		yp += images[n].y;
		h = images[n].h; // may change due to vertical clipping

		// in the next line the vertical clipping will be handled
		// by that way the drawing routines must only take into account the horizontal clipping
		// this should be much faster in most cases

		// must the height be reduced?
		reduce_h = yp + h - clip_rect.yy;
		if (reduce_h > 0) h -= reduce_h;
		// still something to draw
		if (h <= 0) return;

		// vertically lines to skip (only bottom is visible
		skip_lines = clip_rect.y - (int)yp;
		if (skip_lines > 0) {
			if (skip_lines >= h) {
				// not visible at all
				return;
			}
			h -= skip_lines;
			yp += skip_lines;
			// now skip them
			while (skip_lines--) {
				do {
					// clear run + colored run + next clear run
					sp++;
					sp += *sp + 1;
				} while (*sp);
				sp++;
			}
			// now sp is the new start of an image with height h
		}

		// new block for new variables
		{
			// needed now ...
			const KOORD_VAL x = images[n].x;
			const KOORD_VAL w = images[n].w;
			// get the real color
			const PIXVAL color = specialcolormap_all_day[color_index & 0xFF];
			// we use function pointer for the blend runs for the moment ...
			blend_proc pix_blend = (color_index&OUTLINE_FLAG) ? outline[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ] : blend[ (color_index&TRANSPARENT_FLAGS)/TRANSPARENT25_FLAG - 1 ];

			// use horzontal clipping or skip it?
			if (xp + x >= clip_rect.x && xp + x + w - 1 <= clip_rect.xx) {
				// marking change?
				if (dirty) mark_rect_dirty_nc(xp + x, yp, xp + x + w - 1, yp + h - 1);
				display_img_blend_wc( h, xp, yp, sp, color, pix_blend );
			} else if (xp <= clip_rect.xx && xp + x + w > clip_rect.x) {
				display_img_blend_wc( h, xp, yp, sp, color, pix_blend );
				// since height may be reduced, start marking here
				if (dirty) mark_rect_dirty_wc(xp + x, yp, xp + x + w - 1, yp + h - 1);
			}
		}
	}
}



/**
 * the area of this image need update
 * @author Hj. Malthaner
 */
void display_mark_img_dirty(unsigned bild, int xp, int yp)
{
	if (bild < anz_images) {
		mark_rect_dirty_wc(
			xp + images[bild].x,
			yp + images[bild].y,
			xp + images[bild].x + images[bild].w - 1,
			yp + images[bild].y + images[bild].h - 1
		);
	}
}


/**
 * Zeichnet ein Pixel
 * @author Hj. Malthaner
 */
static void display_pixel(KOORD_VAL x, KOORD_VAL y, PIXVAL color)
{
	if (x >= clip_rect.x && x <= clip_rect.xx && y >= clip_rect.y && y <= clip_rect.yy) {
		PIXVAL* const p = textur + x + y * disp_width;

		*p = color;
		mark_tile_dirty(x >> DIRTY_TILE_SHIFT, y >> DIRTY_TILE_SHIFT);
	}
}


/**
 * Zeichnet gefuelltes Rechteck
 */
static void display_fb_internal(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, int color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	if (clip_lr(&xp, &w, cL, cR) && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		int dx = disp_width - w;
		const PIXVAL colval = specialcolormap_all_day[color & 0xFF];
		const unsigned int longcolval = (colval << 16) | colval;

		if (dirty) mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		do {
			unsigned int count = w;
#ifdef USE_C
			uint32 *lp;

			if (count & 1) *p++ = longcolval;
			count >>= 1;
			lp = p;
			while (count-- != 0) {
				*lp++ = longcolval;
			}
			p = lp;
#else
			asm volatile (
				"cld\n\t"
				// uneven words to copy?
				// if(w&1)
				"testb $1,%%cl\n\t"
				"je 0f\n\t"
				// set first word
				"stosw\n\t"
				"0:\n\t"
				// now we set long words ...
				"shrl %%ecx\n\t"
				"rep\n\t"
				"stosl"
				: "+D" (p), "+c" (count)
				: "a" (longcolval)
				: "cc"
			);
#endif

			p += dx;
		} while (--h != 0);
	}
}


void display_fillbox_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_fillbox_wh_clip(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL color, int dirty)
{
	display_fb_internal(xp, yp, w, h, color, dirty, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet vertikale Linie
 * @author Hj. Malthaner
 */
static void display_vl_internal(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty, KOORD_VAL cL, KOORD_VAL cR, KOORD_VAL cT, KOORD_VAL cB)
{
	if (xp >= cL && xp <= cR && clip_lr(&yp, &h, cT, cB)) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const PIXVAL colval =specialcolormap_all_day[color & 0xFF];

		if (dirty) mark_rect_dirty_nc(xp, yp, xp, yp + h - 1);

		do {
			*p = colval;
			p += disp_width;
		} while (--h != 0);
	}
}


void display_vline_wh(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty)
{
	display_vl_internal(xp, yp, h, color, dirty, 0, disp_width, 0, disp_height);
}


void display_vline_wh_clip(const KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL h, const PLAYER_COLOR_VAL color, int dirty)
{
	display_vl_internal(xp, yp, h, color, dirty, clip_rect.x, clip_rect.xx, clip_rect.y, clip_rect.yy);
}


/**
 * Zeichnet rohe Pixeldaten
 */
void display_array_wh(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, const COLOR_VAL *arr)
{
	const int arr_w = w;
	const KOORD_VAL xoff = clip_wh(&xp, &w, clip_rect.x, clip_rect.xx);
	const KOORD_VAL yoff = clip_wh(&yp, &h, clip_rect.y, clip_rect.yy);

	if (w > 0 && h > 0) {
		PIXVAL *p = textur + xp + yp * disp_width;
		const COLOR_VAL *arr_src = arr;

		mark_rect_dirty_nc(xp, yp, xp + w - 1, yp + h - 1);

		if (xp == clip_rect.x) arr_src += xoff;
		if (yp == clip_rect.y) arr_src += yoff * arr_w;

		do {
			unsigned int ww = w;

			do {
				*p++ = specialcolormap_all_day[*arr_src++];
			} while (--ww > 0);
			arr_src += arr_w - w;
			p += disp_width - w;
		} while (--h != 0);
	}
}


// unicode save moving in strings
int get_next_char(const char* text, int pos)
{
	if (has_unicode) {
		return utf8_get_next_char((const utf8*)text, pos);
	} else {
		return pos + 1;
	}
}


int get_prev_char(const char* text, int pos)
{
	if (pos <= 0) return 0;
	if (has_unicode) {
		return utf8_get_prev_char((const utf8*)text, pos);
	} else {
		return pos - 1;
	}
}


/* proportional_string_width with a text of a given length
 * extended for universal font routines with unicode support
 * @author Volker Meyer
 * @date  15.06.2003
 * @author prissi
 * @date 29.11.04
 */
int display_calc_proportional_string_len_width(const char *text, int len, bool use_large_font)
{
	font_type *fnt = use_large_font ? &large_font : &small_font;
	unsigned int c, width = 0;
	int w;

#ifdef UNICODE_SUPPORT
	if (has_unicode) {
		unsigned short iUnicode;
		int	iLen = 0;

		// decode char; Unicode is always 8 pixel (so far)
		while (iLen < len) {
			iUnicode = utf8_to_utf16(text + iLen, &iLen);
			if (iUnicode == 0) return width;
			w = fnt->screen_width[iUnicode];
			if (w == 0) {
				// default width for missing characters
				w = fnt->screen_width[0];
			}
			width += w;
		}
	} else {
#endif
		while (*text != 0 && len > 0) {
			c = (unsigned char)*text;
			width += fnt->screen_width[c];
			text++;
			len--;
		}
#ifdef UNICODE_SUPPORT
	}
#endif
	return width;
}


/* @ see get_mask() */
static const unsigned char byte_to_mask_array[9] = { 0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00 };

/* Helper: calculates mask for clipping *
 * Attention: xL-xR must be <=8 !!!
 * @author priss
 * @date  29.11.04
 */
static unsigned char get_h_mask(const int xL, const int xR, const int cL, const int cR)
{
	// do not mask
	unsigned char mask;

	// check, if there is something to display
	if (xR < cL || xL >= cR) return 0;
	mask = 0xFF;
	// check for left border
	if (xL < cL && xR >= cL) {
		// Left border clipped
		mask = byte_to_mask_array[cL - xL];
	}
	// check for right border
	if (xL < cR && xR >= cR) {
		// right border clipped
		mask &= ~byte_to_mask_array[cR - xL];
	}
	return mask;
}


/*
 * len parameter added - use -1 for previous bvbehaviour.
 * completely renovated for unicode and 10 bit width and variable height
 * @author Volker Meyer, prissi
 * @date  15.06.2003, 2.1.2005
 */
int display_text_proportional_len_clip(KOORD_VAL x, KOORD_VAL y, const char *txt, int align, const PLAYER_COLOR_VAL color_index, int dirty, bool use_large_font, int len, bool use_clipping)
{
	font_type *fnt = use_large_font ? &large_font : &small_font;
	KOORD_VAL cL, cR, cT, cB;
	uint32 c;
	int	iTextPos = 0; // pointer on text position: prissi
	int char_width_1, char_width_2; // 1 is char only, 2 includes room
	int screen_pos;
	const uint8 *char_data;
	const uint8 *p;
	KOORD_VAL yy = y + fnt->height;
	KOORD_VAL x0;	// store the inital x (for dirty marking)
	KOORD_VAL y0, y_offset, char_height;	// real y for display with clipping
	bool v_clip;
	unsigned char mask1, mask2;	// for horizontal clipping
	const PIXVAL color = specialcolormap_all_day[color_index & 0xFF];
#ifndef USE_C
	// faster drawing with assembler
	const unsigned long color2 = (color << 16) | color;
#endif

	// TAKE CARE: Clipping area may be larger than actual screen size ...
	if (use_clipping) {
		cL = clip_rect.x;
		cR = clip_rect.xx;
		cT = clip_rect.y;
		cB = clip_rect.yy;
	} else {
		cL = 0;
		cR = disp_width;
		cT = 0;
		cB = disp_height;
	}
	// don't know len yet ...
	if (len < 0) len = 0x7FFF;

	// adapt x-coordinate for alignment
	switch (align) {
		case ALIGN_LEFT:
			// nothing to do
			break;

		case ALIGN_MIDDLE:
			x -= display_calc_proportional_string_len_width(txt, len, use_large_font) / 2;
			break;

		case ALIGN_RIGHT:
			x -= display_calc_proportional_string_len_width(txt, len, use_large_font);
			break;
	}

	// still something to display?
	if (x > cR || y > cB || y + fnt->height <= cT) {
		// nothing to display
		return 0;
	}

	// x0 contains the startin x
	x0 = x;
	y0 = y;
	y_offset = 0;
	char_height = fnt->height;
	v_clip = false;
	// calculate vertical y clipping parameters
	if (y < cT) {
		y0 = cT;
		y_offset = cT - y;
		v_clip = TRUE;
	}
	if (yy > cB) {
		char_height -= yy - cB;
		v_clip = TRUE;
	}

	// big loop, char by char
	while (iTextPos < len && txt[iTextPos] != 0) {
		int h;
		uint8 char_yoffset;

#ifdef UNICODE_SUPPORT
		// decode char
		if (has_unicode) {
			c = utf8_to_utf16(txt + iTextPos, &iTextPos);
		} else {
#endif
			c = (unsigned char)txt[iTextPos++];
#ifdef UNICODE_SUPPORT
		}
#endif
		// print unknown character?
		if (c > fnt->num_chars || fnt->screen_width[c] == 0) {
			c = 0;
		}

		// get the data from the font
		char_data = fnt->char_data + CHARACTER_LEN * c;
		char_width_1 = char_data[CHARACTER_LEN-1];
		char_yoffset = char_data[CHARACTER_LEN-2];
		char_width_2 = fnt->screen_width[c];
		if (char_width_1>8) {
			mask1 = get_h_mask(x, x + 8, cL, cR);
			// we need to double mask 2, since only 2 Bits are used
			mask2 = get_h_mask(x + 8, x + char_width_1, cL, cR);
			mask2 &= 0xF0;
		} else {
			// char_width_1<= 8: call directly
			mask1 = get_h_mask(x, x + char_width_1, cL, cR);
			mask2 = 0;
		}
		// do the display

		screen_pos = (y0+char_yoffset) * disp_width + x;

		p = char_data + y_offset+char_yoffset;
		for (h = y_offset+char_yoffset; h < char_height; h++) {
			unsigned int dat = *p++ & mask1;
			PIXVAL* dst = textur + screen_pos;

#ifdef USE_C
			if (dat != 0) {
				if (dat & 0x80) dst[0] = color;
				if (dat & 0x40) dst[1] = color;
				if (dat & 0x20) dst[2] = color;
				if (dat & 0x10) dst[3] = color;
				if (dat & 0x08) dst[4] = color;
				if (dat & 0x04) dst[5] = color;
				if (dat & 0x02) dst[6] = color;
				if (dat & 0x01) dst[7] = color;
			}
#else
			// assemble variant of the above, using table and string instructions:
			// optimized for long pipelines ...
#			include "text_pixel.c"
#endif
			screen_pos += disp_width;
		}

		// extra four bits for overwidth characters (up to 12 pixel supported for unicode)
		if (char_width_1 > 8 && mask2 != 0) {
			p = char_data + y_offset/2+12;
			screen_pos = y0 * disp_width + x + 8;
			for (h = y_offset; h < char_height; h++) {
				unsigned int char_dat = *p;
				PIXVAL* dst = textur + screen_pos;
				if(h&1) {
					uint8 dat = (char_dat<<4) & mask2;
					if (dat != 0) {
						if (dat & 0x80) dst[0] = color;
						if (dat & 0x40) dst[1] = color;
						if (dat & 0x20) dst[2] = color;
						if (dat & 0x10) dst[3] = color;
					}
					p++;
				}
				else {
					uint8 dat = char_dat & mask2;
					if (dat != 0) {
						if (dat & 0x80) dst[0] = color;
						if (dat & 0x40) dst[1] = color;
						if (dat & 0x20) dst[2] = color;
						if (dat & 0x10) dst[3] = color;
					}
				}
				screen_pos += disp_width;
			}
		}
		// next char: screen width
		x += char_width_2;
	}

	if (dirty) {
		// here, because only now we know the lenght also for ALIGN_LEFT text
		mark_rect_dirty_wc(x0, y, x - 1, y + 10 - 1);
	}
	// warning: aktual len might be longer, due to clipping!
	return x - x0;
}


/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void display_ddd_box(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color)
{
	display_fillbox_wh(x1, y1,         w, 1, tl_color, TRUE);
	display_fillbox_wh(x1, y1 + h - 1, w, 1, rd_color, TRUE);

	h -= 2;

	display_vline_wh(x1,         y1 + 1, h, tl_color, TRUE);
	display_vline_wh(x1 + w - 1, y1 + 1, h, rd_color, TRUE);
}


/**
 * Zeichnet schattiertes Rechteck
 * @author Hj. Malthaner
 */
void display_ddd_box_clip(KOORD_VAL x1, KOORD_VAL y1, KOORD_VAL w, KOORD_VAL h, PLAYER_COLOR_VAL tl_color, PLAYER_COLOR_VAL rd_color)
{
	display_fillbox_wh_clip(x1, y1,         w, 1, tl_color, TRUE);
	display_fillbox_wh_clip(x1, y1 + h - 1, w, 1, rd_color, TRUE);

	h -= 2;

	display_vline_wh_clip(x1,         y1 + 1, h, tl_color, TRUE);
	display_vline_wh_clip(x1 + w - 1, y1 + 1, h, rd_color, TRUE);
}


// if width equals zero, take default value
void display_ddd_proportional(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = large_font_height / 2 + 1;

	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt - 1, width, 1,              ddd_farbe + 1, dirty);
	display_fillbox_wh(xpos - 2, ypos - halfheight - hgt,     width, halfheight * 2, ddd_farbe,     dirty);
	display_fillbox_wh(xpos - 2, ypos + halfheight - hgt,     width, 1,              ddd_farbe - 1, dirty);

	display_vline_wh(xpos - 2,         ypos - halfheight - hgt - 1, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh(xpos + width - 3, ypos - halfheight - hgt - 1, halfheight * 2 + 1, ddd_farbe - 1, dirty);

	display_text_proportional_len_clip(xpos + 2, ypos - halfheight + 1, text, ALIGN_LEFT, text_farbe, FALSE, true, -1, true);
}



/**
 * display text in 3d box with clipping
 * @author: hsiegeln
 */
void display_ddd_proportional_clip(KOORD_VAL xpos, KOORD_VAL ypos, KOORD_VAL width, KOORD_VAL hgt, PLAYER_COLOR_VAL ddd_farbe, PLAYER_COLOR_VAL text_farbe, const char *text, int dirty)
{
	int halfheight = large_font_height / 2 + 1;

	display_fillbox_wh_clip(xpos - 2, ypos - halfheight - 1 - hgt, width, 1,              ddd_farbe + 1, dirty);
	display_fillbox_wh_clip(xpos - 2, ypos - halfheight - hgt,     width, halfheight * 2, ddd_farbe,     dirty);
	display_fillbox_wh_clip(xpos - 2, ypos + halfheight - hgt,     width, 1,              ddd_farbe - 1, dirty);

	display_vline_wh_clip(xpos - 2,         ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe + 1, dirty);
	display_vline_wh_clip(xpos + width - 3, ypos - halfheight - 1 - hgt, halfheight * 2 + 1, ddd_farbe - 1, dirty);

	display_text_proportional_len_clip(xpos + 2, ypos - 5 + (12 - large_font_height) / 2, text, ALIGN_LEFT, text_farbe, FALSE, true, -1, true);
}



/**
 * Zaehlt Vorkommen ein Buchstabens in einem String
 * @author Hj. Malthaner
 */
int count_char(const char *str, const char c)
{
	int count = 0;

	while (*str != '\0') count += (*str++ == c);
	return count;
}


/**
 * Zeichnet einen mehrzeiligen Text
 * @author Hj. Malthaner
 *
 * Better performance without copying text!
 * @author Volker Meyer
 * @date  15.06.2003
 */
void display_multiline_text(KOORD_VAL x, KOORD_VAL y, const char *buf, PLAYER_COLOR_VAL color)
{
	if (buf != NULL && *buf != '\0') {
		char *next;

		do {
			next = strchr(buf, '\n');
			display_text_proportional_len_clip(
				x, y, buf,
				ALIGN_LEFT, color, TRUE,
				true, next != NULL ? next - buf : -1, true
			);
			buf = next + 1;
			y += LINESPACE;
		} while (next != NULL);
	}
}


//#define DEBUG_FLUSH_BUFFER

/**
 * copies only the changed areas to the screen using the "tile dirty buffer"
 * To get large changes, actually the current and the previous one is used.
 */
void display_flush_buffer(void)
{
	int x, y;
	unsigned char* tmp;

#ifdef USE_SOFTPOINTER
	if (softpointer != -1) {
		ex_ord_update_mx_my();
		display_color_img(standard_pointer, sys_event.mx, sys_event.my, 0, false, true);
	}
	old_my = sys_event.my;
#endif

#ifdef DEBUG_FLUSH_BUFFER
	for (y = 0; y < tile_lines - 1; y++) {
		x = 0;

		do {
			if (is_tile_dirty(x, y)) {
				const int xl = x;
				do {
					x++;
				} while(x < tiles_per_line && is_tile_dirty(x, y));

				display_vline_wh((xl << DIRTY_TILE_SHIFT) - 1, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, 80, FALSE);
				display_vline_wh(x << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE, 80, FALSE);
				display_fillbox_wh(xl << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, (x - xl) << DIRTY_TILE_SHIFT, 1, 80, FALSE);
				display_fillbox_wh(xl << DIRTY_TILE_SHIFT, (y << DIRTY_TILE_SHIFT) + DIRTY_TILE_SIZE - 1, (x - xl) << DIRTY_TILE_SHIFT, 1, 80, FALSE);
			}
			x++;
		} while (x < tiles_per_line);
	}
	dr_textur(0, 0, disp_width, disp_height);
#else
	for (y = 0; y < tile_lines; y++) {
		x = 0;

		do {
			if (is_tile_dirty(x, y)) {
				const int xl = x;
				do {
					x++;
				} while (x < tiles_per_line && is_tile_dirty(x, y));

				dr_textur(xl << DIRTY_TILE_SHIFT, y << DIRTY_TILE_SHIFT, (x - xl) << DIRTY_TILE_SHIFT, DIRTY_TILE_SIZE);
			}
			x++;
		} while (x < tiles_per_line);
	}
#endif

	dr_flush();

	// swap tile buffers
	tmp = tile_dirty_old;
	tile_dirty_old = tile_dirty;
	tile_dirty = tmp;
	memset(tile_dirty, 0, tile_buffer_length);
}


/**
 * Bewegt Mauszeiger
 * @author Hj. Malthaner
 */
void display_move_pointer(KOORD_VAL dx, KOORD_VAL dy)
{
	move_pointer(dx, dy);
}


/**
 * Schaltet Mauszeiger sichtbar/unsichtbar
 * @author Hj. Malthaner
 */
void display_show_pointer(int yesno)
{
#ifdef USE_SOFTPOINTER
	softpointer = yesno;
#else
	show_pointer(yesno);
#endif
}


/**
 * mouse pointer image
 * @author prissi
 */
void display_set_pointer(int pointer)
{
	standard_pointer = pointer;
}


/**
 * mouse pointer image
 * @author prissi
 */
void display_show_load_pointer(int loading)
{
#ifdef USE_SOFTPOINTER
	softpointer = !loading;
#else
	set_pointer(loading);
#endif
}


/**
 * Initialises the graphics module
 * @author Hj. Malthaner
 */
int simgraph_init(KOORD_VAL width, KOORD_VAL height, int use_shm, int do_sync, int full_screen)
{
	int parameter[2];
	int i;

	parameter[0] = use_shm;
	parameter[1] = do_sync;

	dr_os_init(parameter);

	// make sure it something of 16 (also better for caching ... )
	width = (width + 15) & 0x7FF0;

	if (dr_os_open(width, height, 16, full_screen)) {

		disp_width = width;
		disp_height = height;

		textur = dr_textur_init();

		// init, load, and check fonts
		large_font.screen_width = NULL;
		large_font.char_data = NULL;
		display_load_font(FONT_PATH_X "prop.fnt", true);
#ifdef USE_SMALL_FONT
		small_font.screen_width = NULL;
		small_font.char_data = NULL;
		display_load_font(FONT_PATH_X "4x7.hex", false);
		display_check_fonts();
#endif
	} else {
		puts("Error  : can't open window!");
		exit(-1);
	}

	// allocate dirty tile flags
	tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
	tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

	tile_dirty     = guarded_malloc(tile_buffer_length);
	tile_dirty_old = guarded_malloc(tile_buffer_length);

	memset(tile_dirty,     255, tile_buffer_length);
	memset(tile_dirty_old, 255, tile_buffer_length);

	// init player colors
	for(i=0;  i<MAX_PLAYER;  i++  ) {
		player_offsets[i][0] = i*8;
		player_offsets[i][1] = i*8+24;
	}

	display_setze_clip_wh(0, 0, disp_width, disp_height);
	display_day_night_shift(0);

	// Hajo: Calculate daylight rgbmap and save it for unshaded tile drawing
	calc_base_pal_from_night_shift(0);
	memcpy(rgbmap_all_day, rgbmap_day_night, RGBMAPSIZE * sizeof(PIXVAL));
	memcpy(specialcolormap_all_day, specialcolormap_day_night, 256 * sizeof(PIXVAL));

	// find out bit depth
	{
		uint32 c = get_system_color( 0, 255, 0 );
		while((c&1)==0) {
			c >>= 1;
		}
		if(c==31) {
			// 15 bit per pixel
			blend[0] = pix_blend25_15;
			blend[1] = pix_blend50_15;
			blend[2] = pix_blend75_15;
			outline[0] = pix_outline25_15;
			outline[1] = pix_outline50_15;
			outline[2] = pix_outline75_15;
		}
		else {
			blend[0] = pix_blend25_16;
			blend[1] = pix_blend50_16;
			blend[2] = pix_blend75_16;
			outline[0] = pix_outline25_16;
			outline[1] = pix_outline50_16;
			outline[2] = pix_outline75_16;
		}
	}

	printf("Init done.\n");
	fflush(NULL);

	return TRUE;
}


/**
 * Prueft ob das Grafikmodul schon init. wurde
 * @author Hj. Malthaner
 */
int is_display_init(void)
{
	return textur != NULL;
}



// delete all images above a certain number ...
// (mostly needed when changing climate zones)
void display_free_all_images_above( unsigned above )
{
	while( above<anz_images) {
		anz_images--;
		if(images[anz_images].zoom_data!=NULL) {
			guarded_free( images[anz_images].zoom_data );
		}
		if(images[anz_images].data!=NULL) {
			guarded_free( images[anz_images].data );
		}
	}
}



/**
 * Schliest das Grafikmodul
 * @author Hj. Malthaner
 */
int simgraph_exit()
{
	guarded_free(tile_dirty);
	guarded_free(tile_dirty_old);
	display_free_all_images_above(0);
	guarded_free(images);

	tile_dirty = tile_dirty_old = NULL;
	images = NULL;

	return dr_os_close();
}


/* changes display size
 * @author prissi
 */
void simgraph_resize(KOORD_VAL w, KOORD_VAL h)
{
	if (disp_width != w || disp_height != h) {
		disp_width = (w + 15) & 0x7FF0;
		disp_height = h;

		guarded_free(tile_dirty);
		guarded_free(tile_dirty_old);

		dr_textur_resize(&textur, disp_width, disp_height, 16);

		tiles_per_line     = (disp_width  + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_lines         = (disp_height + DIRTY_TILE_SIZE - 1) / DIRTY_TILE_SIZE;
		tile_buffer_length = (tile_lines * tiles_per_line + 7) / 8;

		tile_dirty     = guarded_malloc(tile_buffer_length);
		tile_dirty_old = guarded_malloc(tile_buffer_length);

		memset(tile_dirty,     255, tile_buffer_length);
		memset(tile_dirty_old, 255, tile_buffer_length);

		display_setze_clip_wh(0, 0, disp_width, disp_height);
	}
}


/**
 * Speichert Screenshot
 * @author Hj. Malthaner
 */
void display_snapshot()
{
	static int number = 0;

	char buf[80];

#ifdef WIN32
	mkdir(SCRENSHOT_PATH);
#else
	mkdir(SCRENSHOT_PATH, 0700);
#endif

	do {
		sprintf(buf, SCRENSHOT_PATH_X "simscr%02d.bmp", number++);
	} while (access(buf, W_OK) != -1);

	dr_screenshot(buf);
}



/**
 * zeichnet linie von x,y nach xx,yy
 * von Hajo
 **/
void display_direct_line(const KOORD_VAL x, const KOORD_VAL y, const KOORD_VAL xx, const KOORD_VAL yy, const PLAYER_COLOR_VAL color)
{
	int i, steps;
	int xp, yp;
	int xs, ys;

	const int dx = xx - x;
	const int dy = yy - y;
	const PIXVAL colval = specialcolormap_all_day[color & 0xFF];

	steps = (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
	if (steps == 0) steps = 1;

	xs = (dx << 16) / steps;
	ys = (dy << 16) / steps;

	xp = x << 16;
	yp = y << 16;

	for (i = 0; i <= steps; i++) {
		display_pixel(xp >> 16, yp >> 16, colval);
		xp += xs;
		yp += ys;
	}
}
