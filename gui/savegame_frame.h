/*
 * savegame_frame.h
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef gui_savegame_frame_h
#define gui_savegame_frame_h


#include "../tpl/slist_tpl.h"
#include "ifc/action_listener.h"
#include "gui_frame.h"
#include "gui_scrollpane.h"
#include "gui_container.h"
#include "components/gui_textinput.h"
#include "components/gui_divider.h"     // 30-Oct-2001      Markus Weber    Added
#include "gui_label.h"                  // 31-Oct-2001      Markus Weber    Added
#include "button.h"                     // 29-Oct-2001      Markus Weber    Added


class button_t;

class savegame_frame_t : public gui_frame_t, action_listener_t
{
private:
    slist_tpl <button_t *> buttons;
    slist_tpl <button_t *> deletes;
    slist_tpl <gui_label_t *> labels;

	char ibuf[64];
	gui_textinput_t input;
	gui_divider_t divider1;                               // 30-Oct-2001  Markus Weber    Added
	button_t savebutton;                                  // 29-Oct-2001  Markus Weber    Added
	button_t cancelbutton;                               // 29-Oct-2001  Markus Weber    Added
	gui_label_t fnlabel;        //filename                // 31-Oct-2001  Markus Weber    Added
	gui_container_t button_frame;
	gui_scrollpane_t scrolly;

    /**
     * Filename suffix, i.e. ".sve", must be four characters
     * @author Hj. Malthaner
     */
    const char *suffix;

    void add_file(const char *filename);
protected:

    /**
     * Name des Spieles in der Datei.
     * @aparam filename Name der Spielstandsdatei
     * @author Hansj�rg Malthaner
     */
    const char * gib_spiel_name(const char *filename);


    /**
     * Aktion, die nach Knopfdruck gestartet wird.
     * @author Hansj�rg Malthaner
     */
    virtual void action(const char *filename) = 0;

    /**
     * Aktion, die nach X-Knopfdruck gestartet wird.
     * @author Volker Meyer
     */
    virtual void del_action(const char *filename) = 0;

public:
    /**
     * Konstruktor.
     * @param suffix Filename suffix, i.e. ".sve", must be four characters
     * @author Hj. Malthaner
     */
    savegame_frame_t(const char *suffix);

    ~savegame_frame_t();

    /**
     * This method is called if an action is triggered
     * @author Hj. Malthaner
     */
    bool action_triggered(gui_komponente_t *komp);

    /**
     * Setzt die Fenstergroesse
     * @author (Mathew Hounsell)
     * @date   11-Mar-2003
     */
    virtual void setze_fenstergroesse(koord groesse);
};

#endif
