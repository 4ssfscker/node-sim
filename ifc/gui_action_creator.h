/*
 * action_creator.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef gui_ifc_action_creator_h
#define gui_ifc_action_creator_h

#include "../gui/components/action_listener.h"
#include "../tpl/slist_tpl.h"
#include "gui_komponente.h"


/**
 * This interface must be implemented by all classes which want to
 * send actions, i.e. button presses
 * @author Hj. Malthaner
 */
class gui_komponente_action_creator_t : public gui_komponente_t
{
protected:
    /**
     * Our listeners.
     * @author Hj. Malthaner
     */
    slist_tpl <action_listener_t *> listeners;

	/**
	 * Inform all listeners that an action was triggered.
	 * @author Hj. Malthaner
	 */
	void call_listeners(value_t v)
	{
	    slist_iterator_tpl<action_listener_t *> iter (listeners);
	    while(iter.next() && !iter.get_current()->action_triggered(this,v));
	}

public:
    /**
     * Add a new listener to this text input field.
     * @author Hj. Malthaner
     */
    void add_listener(action_listener_t * l) { listeners.insert(l); }
};

#endif
