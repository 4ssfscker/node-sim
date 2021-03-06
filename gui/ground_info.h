/*
 * ground_info.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_ground_info_h
#define gui_ground_info_h

#include "gui_frame.h"
#include "components/gui_world_view_t.h"
#include "components/gui_textarea.h"
#include "../utils/cbuffer_t.h"
#include "../boden/grund.h"

/**
 * An adapter class to display info windows for ground (floor) objects
 *
 * @author Hj. Malthaner
 * @date 20-Nov-2001
 */
class grund_info_t : public gui_frame_t
{
protected:
	/**
	 * The ground we observe. The ground will delete this object
	 * if self deleted.
	 * @author Hj. Malthaner
	 */
	grund_t *gr;
	world_view_t view;
	static cbuffer_t gr_info;

public:
    grund_info_t(karte_t *welt, grund_t *gr);
    ~grund_info_t() { gr->entferne_grund_info(); }

	void zeichnen(koord pos, koord gr);
};

#endif
