/*
 * thing_info.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_thing_info_h
#define gui_thing_info_h

#include "../simdebug.h"
#include "../simdings.h"
#include "gui_frame.h"
#include "components/gui_world_view_t.h"
#include "../utils/cbuffer_t.h"

/**
 * An adapter class to display info windows for things (objects)
 *
 * @author Hj. Malthaner
 * @date 22-Nov-2001
 */
class ding_infowin_t : public gui_frame_t
{
protected:
    world_view_t view;

    int calc_fensterhoehe_aus_info() const;

    static cbuffer_t buf;

    /**
     * The thing we observe. The thing will delete this object
     * if self deleted.
     * @author Hj. Malthaner
     */
    ding_t *ding;

public:
    ding_infowin_t(karte_t *welt, ding_t *ding);

    virtual ~ding_infowin_t() { ding->entferne_ding_info(); }

    /**
     * @return window title
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual const char * gib_name() const { return ding->gib_name(); }

    /**
     * @return the text to display in the info window
     *
     * @author Hj. Malthaner
     * @see simwin
     */
    virtual void info(cbuffer_t & buf) const { ding->info(buf); }

    /**
     * @return a pointer to the player who owns this thing
     *
     * @author Hj. Malthaner
     */
    virtual spieler_t* gib_besitzer() const { return ding->gib_besitzer(); }

    /**
     * @return the current map position
     *
     * @author Hj. Malthaner
     */
    virtual koord3d gib_pos() const { return ding->gib_pos(); }

  /**
   * komponente neu zeichnen. Die �bergebenen Werte beziehen sich auf
   * das Fenster, d.h. es sind die Bildschirmkoordinaten des Fensters
   * in dem die Komponente dargestellt wird.
   */
  virtual void zeichnen(koord pos, koord gr);
};


#endif
