#ifndef gui_image_list_h
#define gui_image_list_h

#include "../../ifc/gui_action_creator.h"
#include "../../tpl/vector_tpl.h"
#include "../../simimg.h"


/**
 * Updated! class is used only for the vehicle dialog. SO I changed some things
 * for the new one::
 * - cannot select no-image fields any more
 * - numbers can be drawn ontop an images
 * - color bar can be added to the images
 *
 *
 * @author Volker Meyer
 * @date  09.06.2003
 *
 * Eine Komponenete die eine Liste von Bildern darstellt.
 * @author Hj. Malthaner
 * @version $Revision: 1.10 $
 */
class gui_image_list_t : public gui_komponente_action_creator_t
{
public:
    struct image_data_t {
	image_id		image;
	sint16		count;
	sint16		lcolor;	//0=none, 1=green, 2=red, 3 =yellow
	sint16		rcolor;
    };
    /**
     * Graphic layout:
     * size of borders around the whole area (there are no borders around
     * individual images)
     *
     * @author Volker Meyer
     * @date  07.06.2003
     */
    enum { BORDER = 4 };
private:
    vector_tpl<image_data_t> *images;

    koord grid;
    koord placement;

    /**
     * Rows or columns?
     * @author Volker Meyer
     * @date  20.06.2003
     */
    int use_rows;

    /**
     * Kennfarbe f�r Bilder (Spielerfarbe).
     * @author Hj. Malthaner
     */
    int color;

public:
    /**
     * Konstruktor, nimmt einen Vector von Bildnummern als Parameter.
     * @param bilder ein Vector mit den Nummern der Bilder
     * @author Hj. Malthaner
     */

    gui_image_list_t(vector_tpl<image_data_t> *images);
    /**
     * This set horizontal and vertical spacing for the images.
     * @author Volker Meyer
     * @date  20.06.2003
     */

    void set_grid(koord grid) { this->grid = grid; }
    /**
     * This set the offset for the images.
     * @author Volker Meyer
     * @date  20.06.2003
     */
    void set_placement(koord placement) { this->placement = placement; }

    void set_player_color(int color) { this->color = color; }
    /**
     * Events werden hiermit an die GUI-Komponenten
     * gemeldet
     * @author Hj. Malthaner
     */
    virtual void infowin_event(const event_t *ev);

    /**
     * Zeichnet die Bilder
     * @author Hj. Malthaner
     */
    virtual void zeichnen(koord offset) const;

    /**
     * Looks for the image at given position.
     * xpos and ypos relative to parent window.
     *
     * @author Volker Meyer
     * @date  07.06.2003
     */
    int index_at(koord parent_pos, int xpos, int ypos) const;

	void recalc_size();
};

#endif