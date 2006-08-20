/*
 * simworld.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * simworld.cc
 *
 * Hauptklasse fuer Simutrans, Datenstruktur die alles Zusammenhaelt
 * Hansjoerg Malthaner, 1997
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "simdebug.h"
#include "boden/boden.h"
#include "boden/wasser.h"
#include "simplay.h"
#include "simfab.h"
#include "simconvoi.h"
#include "simcity.h"
#include "simskin.h"
#include "simcosts.h"
#include "simwin.h"
#include "simhalt.h"
#include "simversion.h"
#include "simticker.h"
#include "simcolor.h"

#include "simtime.h"
#include "simintr.h"
#include "simio.h"

#include "simimg.h"
#include "blockmanager.h"
#include "simvehikel.h"
#include "simworld.h"

#include "simwerkz.h"
#include "simtools.h"
#include "simsound.h"
#include "simsfx.h"

#include "simgraph.h"
#include "simdisplay.h"

#include "dings/zeiger.h"
#include "dings/baum.h"
#include "dings/signal.h"
#include "dings/oberleitung.h"
#include "dings/gebaeude.h"

#include "gui/welt.h"
#include "gui/karte.h"
#include "gui/map_frame.h"
#include "gui/optionen.h"
#include "gui/spieler.h"
#include "gui/werkzeug_waehler.h"
#include "gui/schedule_list.h"
#include "gui/messagebox.h"
#include "gui/loadsave_frame.h"
#include "gui/money_frame.h"
#include "gui/convoi_frame.h"
#include "gui/halt_list_frame.h"        //30-Dec-01     Markus Weber    Added
#include "gui/help_frame.h"

#include "dataobj/fahrplan.h"
#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/einstellungen.h"
#include "dataobj/umgebung.h"
#include "dataobj/tabfile.h"


#include "besch/grund_besch.h"

#include "bauer/brueckenbauer.h"
#include "bauer/tunnelbauer.h"
#include "bauer/factorybuilder.h"
#include "bauer/fabrikbauer.h"
#include "bauer/wegbauer.h"
#include "bauer/hausbauer.h"

// #include "adt/ifc/iterator.h"
// #include "adt/queue.h"
// #include "adt/vector.h"

#include "ifc/sync_steppable.h"

#ifdef AUTOTEST
#include "test/worldtest.h"
#include "test/testtool.h"
#endif



//#define DEMO
//#undef DEMO

const int karte_t::anz_spieler = 8;
const int karte_t::Z_PLAN      = 8;
const int karte_t::Z_GRID      = 0;
const int karte_t::Z_LINES     = 4;


void
karte_t::setze_scroll_multi(int n)
{
    einstellungen->setze_scroll_multi(n);
}


/* unused
void
karte_t::calc_hoehe(int x1, int y1, int x2, int y2)
{


    const double scale = 1.4;

    int xm = (x1+x2)/2;
    int ym = (y1+y2)/2;

    int xd = (int)(((x2 - x1)+1) * scale);

    if(xd < 16)
	xd = 1;

    if(x2-x1 <= 1 && y2-y1 <= 1)
	return;

    // 5 neue Punkte

    int h1 = (lookup_hgt(koord(x1, y1)) + lookup_hgt(koord(x2, y1))) / 2 +
             simrand( xd ) - (xd/2);

    int h2 = (lookup_hgt(koord(x1, y1)) + lookup_hgt(koord(x1, y2))) / 2 +
             simrand( xd ) - (xd/2);

    int h4 = (lookup_hgt(koord(x2, y1)) + lookup_hgt(koord(x2, y2))) / 2 +
             simrand( xd ) - (xd/2);

    int h5 = (lookup_hgt(koord(x1, y2)) + lookup_hgt(koord(x2, y2))) / 2 +
             simrand( xd ) - (xd/2);

    int h3 = (h1 + h2 + h4 + h5) / 4
              + (simrand( xd ) - (xd)/2);


    h1 = MIN(h1, 144) & 0xFFFFFFF0;
    h2 = MIN(h2, 144) & 0xFFFFFFF0;
    h3 = MIN(h3, 144) & 0xFFFFFFF0;
    h4 = MIN(h4, 144) & 0xFFFFFFF0;
    h5 = MIN(h5, 144) & 0xFFFFFFF0;


    hgt[xm + y1*gib_groesse()] = h1;
    hgt[xm+1 + y1*gib_groesse()] = h1;

    hgt[x1 + ym*gib_groesse()] = h2;
    hgt[x1 + (ym+1)*gib_groesse()] = h2;

    hgt[xm + ym*gib_groesse()] = h3;
    hgt[xm+1 + ym*gib_groesse()] = h3;
    hgt[xm + (ym+1)*gib_groesse()] = h3;
    hgt[xm+1 + (ym+1)*gib_groesse()] = h3;

    hgt[x2 + ym*gib_groesse()] = h4;
    hgt[x2 + (ym+1)*gib_groesse()] = h4;

    hgt[xm + y2*gib_groesse()] = h5;
    hgt[xm+1 + y2*gib_groesse()] = h5;


    calc_hoehe(x1, y1, xm, ym);
    calc_hoehe(xm+1, y1, x2, ym);
    calc_hoehe(x1, ym+1, xm, y2);
    calc_hoehe(xm+1, ym+1, x2, y2);
}
*/


/**
 * Read a heightfield from file
 * @param filename name of heightfield file
 * @author Hj. Malthaner
 */
void
karte_t::calc_hoehe_mit_heightfield(const cstring_t & filename)
{
  const int groesse = gib_groesse();


  FILE *file = fopen(filename, "rb");

  if(file) {
    char buf [256];
    int w, h;

    read_line(buf, 255, file);

    if(strncmp(buf, "P6", 2)) {
      dbg->fatal("karte_t::load_heightfield()",
		 "Heightfield has wrong image type %s", buf);
    }

    read_line(buf, 255, file);

    sscanf(buf, "%d %d", &w, &h);

    if(w != groesse || h != groesse) {
      dbg->fatal("karte_t::load_heightfield()",
		 "Heightfield has wrong size %s", buf);
    }

    read_line(buf, 255, file);

    if(strncmp(buf, "255", 2)) {
      dbg->fatal("karte_t::load_heightfield()",
		 "Heightfield has wrong color depth %s", buf);
    }

    for(int y=0; y<groesse; y++) {
      for(int x=0; x<groesse; x++) {
	int R = fgetc(file);
	int G = fgetc(file);
	int B = fgetc(file);


	setze_grid_hgt(koord(x,y),
		       ((R*2+G*3+B)/16 - 80) & 0xFFF0
		       );

      }

      if(is_display_init()) {
	display_progress(y/2, groesse+einstellungen->gib_anzahl_staedte()*4);
	display_flush(0, 0, 0, "", "");
      }
    }
  } else {
    perror("Error:");
  }
}

void
karte_t::calc_hoehe_mit_perlin()
{
    const int groesse = gib_groesse();

    for(int y=0; y<groesse; y++) {

	for(int x=0; x<groesse; x++) {
	  // Hajo: to Markus: replace the fixed values with your
          // settings. Amplitude is the top highness of the
          // montains, frequency is something like landscape 'roughness'
          // amplitude may not be greater than 160.0 !!!
          // please don't allow frequencies higher than 0.8 it'll
	  // break the AI's pathfinding. Frequency values of 0.5 .. 0.7
          // seem to be ok, less is boring flat, more is too crumbled
	  // the old defaults are given here: f=0.6, a=160.0
	  setze_grid_hgt(koord(x,y),
			 perlin_hoehe(x,y,
				      einstellungen->gib_map_roughness(),
				      einstellungen->gib_max_mountain_height()
				      )
			 );

	  // Niels: use this for fastest map creation
	  // setze_grid_hgt(koord(x, y), 64);
	}

	if(is_display_init()) {
	    display_progress(y/2, groesse+einstellungen->gib_anzahl_staedte()*4);
	    display_flush(0, 0, 0, "", "");
	} else {
	    print("%3d/%3d\r", y, groesse-1);
	    fflush(stdout);
	}
    }
    print("\n");
}


void karte_t::raise_clean(int x, int y, int h)
{
    const planquadrat_t *plan = lookup(koord(x,y));

    if(plan) {

	if(lookup_hgt(koord(x, y)) < h) {
	    setze_grid_hgt(koord(x, y), h);

	    raise_clean(x-1, y-1, h-16);
	    raise_clean(x  , y-1, h-16);
	    raise_clean(x+1, y-1, h-16);
	    raise_clean(x-1, y  , h-16);
	    // Punkt selbst hat schon die neue Hoehe
	    raise_clean(x+1, y  , h-16);
	    raise_clean(x-1, y+1, h-16);
	    raise_clean(x  , y+1, h-16);
	    raise_clean(x+1, y+1, h-16);

	}
    }
}

void karte_t::cleanup_karte()
{
    int i,j;

    int *hgts = new int [(gib_groesse()+1)*(gib_groesse()+1)];
    int *p = hgts;

    for(j=0; j<=gib_groesse(); j++) {
	for(i=0; i<=gib_groesse(); i++) {
	    *p++ = lookup_hgt(koord(i, j));
	}
    }

    p=hgts;
    for(j=0; j<=gib_groesse(); j++) {
	for(i=0; i<=gib_groesse(); i++) {
	    raise_clean(i,j, (*p++)+16);
	}
    }

    for(i=0; i<=gib_groesse(); i++) {
	lower_to(i, 0, grundwasser);
	lower_to(0, i, grundwasser);
	lower_to(i, gib_groesse(), grundwasser);
	lower_to(gib_groesse(), i, grundwasser);
    }

    for(i=0; i<=gib_groesse(); i++) {
	raise_to(i, 0, grundwasser);
	raise_to(0, i, grundwasser);
	raise_to(i, gib_groesse(), grundwasser);
	raise_to(gib_groesse(), i, grundwasser);
    }

    delete[] hgts;
}


/* Hajo's variant
void
karte_t::verteile_baeume(int dichte)
{
    for(j=0; j<gib_groesse(); j++) {
	for(i=0; i<gib_groesse(); i++) {
	    grund_t *gr = access(i, j)->gib_kartenboden();

	    if( simrand( dichte ) == 0 &&
		gr->ist_natur() &&
		!gr->ist_wasser() && gr->gib_hoehe() > grundwasser)
	    {
		if(baum_t::gib_anzahl_besch() > 0)
		    gr->obj_add( new baum_t(this, gr->gib_pos()) );
	    }
	}
    }
}
*/


void karte_t::verteile_baeume(int dichte)
{
  // at first deafault values for forest creation will be set
  // Base forest size - minimal size of forest - map independent
  unsigned char forest_base_size = 36;

  // Map size divisor - smaller it is the larger are individual forests
  unsigned char forest_map_size_divisor = 38;

  // Forest count divisor - smaller it is, the more forest are generated
  unsigned char forest_count_divisor = 16;

  // Forest boundary sharpenss: 0 - perfectly sharp boundaries, 20 - very blurred
  unsigned char forest_boundary_blur = 6;

  // Forest boundary thickness  - determines how thick will the boundary line be
  unsigned char forest_boundary_thickness = 2;

  // Determins how often are spare trees going to be planted (works inversly)
  unsigned char forest_inverse_spare_tree_density = 5;

  //	Number of trees on square 2 - minimal usable, 3 good, 5 very nice looking
  setze_max_no_of_trees_on_square (3);

  //now forest configuration data will be read form simuconf.tab
  tabfile_t simuconf;

  if(simuconf.open("simuconf.tab")) {
    tabfileobj_t contents;

    simuconf.read(contents);

    forest_base_size = contents.get_int("forest_base_size", 36);
    forest_map_size_divisor = contents.get_int("forest_map_size_divisor", 38);
    forest_count_divisor = contents.get_int("forest_count_divisor", 16);
    forest_boundary_blur = contents.get_int("forest_boundary_blur", 6);
    forest_boundary_thickness = contents.get_int("forest_boundary_thickness", 2);
    forest_inverse_spare_tree_density = contents.get_int("forest_inverse_spare_tree_density", 5);


    setze_max_no_of_trees_on_square (contents.get_int("max_no_of_trees_on_square", 3));

  } else {
    // nothing here, if reading fails, default values will remain
  }

  // now we can proceed to tree palnting routine itself
  // best forests results are produced if forest size is tied to map size -
  // but there is some nonlinearity to ensure good forests on small maps

  const unsigned int t_forest_size =
    (unsigned int)pow( gib_groesse()>>7 , 0.5)*forest_base_size + (gib_groesse()/forest_map_size_divisor);
  const unsigned char c_forest_count = gib_groesse()/forest_count_divisor;
  unsigned int  x_tree_pos, y_tree_pos, distance, tree_probability;
  unsigned char c2;

  for (unsigned char c1 = 0 ; c1 < c_forest_count ; c1++) {
    const unsigned int xpos_f = simrand(gib_groesse());
    const unsigned int ypos_f = simrand(gib_groesse());
    const unsigned int c_coef_x = 1+simrand(2);
    const unsigned int c_coef_y = 1+simrand(2);
    const unsigned int x_forest_boundary = t_forest_size*c_coef_x;
    const unsigned int y_forest_boundary = t_forest_size*c_coef_y;
    unsigned int i, j;

    for(j = 0; j < x_forest_boundary; j++) {
      for(i = 0; i < y_forest_boundary; i++) {

	x_tree_pos = (j-(t_forest_size>>1)); // >>1 works like 2 but is faster
	y_tree_pos = (i-(t_forest_size>>1));

	distance = 1 + ((int) sqrt( x_tree_pos*x_tree_pos/c_coef_x +
				    y_tree_pos*y_tree_pos/c_coef_y));

	tree_probability = (t_forest_size<<4) / distance; //is same as = ( 32 * (t_forest_size / 2) ) / distance

	for (c2 = 0 ; c2 < gib_max_no_of_trees_on_square() ; c2++) {
	  const unsigned int rating =
	    simrand(forest_boundary_blur) + 38 + c2*forest_boundary_thickness;

	  if (rating < tree_probability ) {
	    const koord pos ((short int)(xpos_f+x_tree_pos),
			     (short int)(ypos_f+y_tree_pos));

	    baum_t::plant_tree_on_coordinate(this,
					     pos,
					     gib_max_no_of_trees_on_square());
	  }
	}
      }
    }
  }

  for(j = 0; j<gib_groesse(); j++) {
    for(i = 0; i<gib_groesse(); i++) {
      //plant spare trees, (those with low preffered density)
      if(simrand(forest_inverse_spare_tree_density*dichte) < 100) {
	const koord pos (i,j);
	baum_t::plant_tree_on_coordinate(this, pos, 1);
      }
    }
  }
}




// karte_t methoden

void
karte_t::destroy()
{
    dbg->message("karte_t::destroy()", "destroying world");

    // hier nur entfernen, aber nicht l�schen
    ausflugsziele.clear();

    labels.clear();

    unsigned int i,j;

    // dinge aufr�umen
    if(plan != NULL) {
	for(j=0; j<(unsigned int)gib_groesse(); j++) {
	    for(i=0; i<(unsigned int)gib_groesse(); i++) {
		access(i, j)->destroy(NULL);
	    }
	}

	delete [] plan;
	plan = NULL;
    }
    // entfernt alle synchronen objekte aus der liste
    sync_list.clear();

    // gitter aufr�umen
    if(grid_hgts != NULL) {
	delete [] grid_hgts;
	grid_hgts = NULL;
    }
    // marker aufr�umen
    marker.init(0);
    dirty.init(0);


    // st�dte aufr�umen
    if(stadt != NULL) {
	for(i=0; i<stadt->get_size(); i++) {
            delete stadt->at(i);
	}

	delete stadt;
	stadt = NULL;
    }


    // spieler aufr�umen
    for(i=0; i<(unsigned int)anz_spieler ; i++) {
	if(spieler[i] != NULL) {
	    delete spieler[i];
	    spieler[i] = NULL;
	}
    }

    // alle convois aufr�umen
    while(convoi_list.count() > 0) {
	convoihandle_t cnv = convoi_list.at(0);
	rem_convoi(cnv);

	convoi_t *p = cnv.detach();
	delete p;
    }
    convoi_list.clear();

    // alle fabriken aufr�umen
    slist_iterator_tpl<fabrik_t*> fab_iter(fab_list);

    while(fab_iter.next()) {
	delete fab_iter.get_current();
    }

    fab_list.clear();

    // alle haltestellen aufr�umen
    haltestelle_t::destroy_all();

    // rail blocks aufraeumen
    blockmanager::gib_manager()->delete_all_blocks();

    dbg->message("karte_t::destroy()", "world destroyed");
}


/**
 * Zugriff auf das St�dte Array.
 * @author Hj. Malthaner
 */
void karte_t::add_stadt(stadt_t *s)
{
    einstellungen->setze_anzahl_staedte(einstellungen->gib_anzahl_staedte()+1);

    array_tpl <stadt_t *> * neu_stadt  = new array_tpl<stadt_t *> (einstellungen->gib_anzahl_staedte());

    for(unsigned int i=0; i<stadt->get_size() ; i++) {
	neu_stadt->at(i) = stadt->at(i);
    }


    neu_stadt->at(stadt->get_size()) = s;

    delete stadt;

    stadt = neu_stadt;
}


void
karte_t::init_felder()
{
    const int groesse = gib_groesse();

    plan   = new planquadrat_t[groesse*groesse];
    grid_hgts = new short[(groesse+1)*(groesse+1)];
    marker.init(groesse);
    dirty.init(groesse);
    stadt  = new array_tpl<stadt_t *> (einstellungen->gib_anzahl_staedte());

    blockmanager::gib_manager()->setze_welt_groesse( groesse );
    win_setze_welt( this );
    reliefkarte_t::gib_karte()->setze_welt(this);

    for(i=0; i<anz_spieler ; i++) {
        spieler[i] = new spieler_t(this, i*4);

    }
}

void
karte_t::init(einstellungen_t *sets)
{
    int i, j;

    if(is_display_init()) {
	display_show_pointer(false);
    }

    setze_ij_off(koord(0,0));

    x_off = y_off = 0;

    ticks = 0;
    letzter_monat = 0;
    letztes_jahr = 0;
    steps = 0;

    //grundwasser = -32;        29-Nov-01       Markus Weber    Changed

    destroy();

    intr_disable();

    einstellungen = sets;

    grundwasser = sets->gib_grundwasser();      //29-Nov-01     Markus Weber    Changed

    // wird gecached, um den Pointerzugriff zu sparen, da
    // die groesse _sehr_ oft referenziert wird
    cached_groesse = einstellungen->gib_groesse();
    cached_groesse2 = cached_groesse*cached_groesse;

    init_felder();

    for(j=0; j<=gib_groesse(); j++) {
	for(i=0; i<=gib_groesse(); i++) {
	    setze_grid_hgt(koord(i, j), 0);
	}
    }

    koord k;

    for(k.y=0; k.y<gib_groesse(); k.y++) {
	for(k.x=0; k.x<gib_groesse(); k.x++) {
	    access(k)->kartenboden_setzen( new boden_t(this, koord3d(k, 0) ), false);
	}
    }

    print("Creating landscape shape...\n");

    // calc_hoehe(0, 0, gib_groesse()-1, gib_groesse()-1);

    unsigned long old_seed = setsimrand( einstellungen->gib_karte_nummer() );
    if(einstellungen->heightfield.len() > 0) {
      calc_hoehe_mit_heightfield(einstellungen->heightfield);
    } else {
      calc_hoehe_mit_perlin();
    }
    // Wenn man den Zufall kaputtmacht, sollte man ihn auch wieder heil machen
    if(gib_groesse() != 64)
	setsimrand( old_seed );

    cleanup_karte();

    print("Creating landscape ground ...\n");

    for(k.y=0; k.y<gib_groesse(); k.y++) {
	for(k.x=0; k.x<gib_groesse(); k.x++) {

	    if(lookup_hgt(k) <= grundwasser &&
               lookup_hgt(k+koord(1,0)) <= grundwasser &&
	       lookup_hgt(k+koord(0,1)) <= grundwasser &&
	       lookup_hgt(k+koord(1,1)) <= grundwasser) {
		access(k)->kartenboden_setzen(new wasser_t(this, k), false);
	    }
	}
    }

    for(k.y=0; k.y<gib_groesse(); k.y++) {
	for(k.x=0; k.x<gib_groesse(); k.x++) {
	    grund_t *gr = lookup(k)->gib_kartenboden();
	    gr->sync_height();
	    gr->calc_bild();
	}
    }

    verteile_baeume(3);


    print("Creating cities ...\n");

    hausbauer_t::neue_karte();

    stadt_t::init_namen();
    array_tpl<koord> *pos = stadt_t::random_place(this, einstellungen->gib_anzahl_staedte() + 1);
    if( pos ) {

      // Ansicht auf erste Stadt zentrieren

      setze_ij_off(pos->at(0) + koord(-8, -8));

      for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	//	printf("City #%d at %d, %d\n",i+1, pos[i].x, pos[i].y);
	stadt->at(i) = new stadt_t(this, spieler[1], pos->at(i));


	if(is_display_init()) {
	    display_progress(gib_groesse()/2+i*2, gib_groesse()+einstellungen->gib_anzahl_staedte()*4);
	    display_flush(0, 0, 0, "", "");
	}
      }

	grund_t *gr = access(pos->at(einstellungen->gib_anzahl_staedte()))->gib_kartenboden();

	hausbauer_t::baue(this, NULL, gr->gib_pos(), 0, hausbauer_t::muehle_besch);

	delete pos;
	pos = NULL;
    }

	////////////////////////
	////////////////////////
	////////////////////////
	////////////////////////
	////////////////////////
    // fabrikbauer_t::verteile_industrie(this, gib_spieler(1), einstellungen->gib_industrie_dichte());
	try
	{
		factory_builder_t tmp (this, spieler[1]);

		//acceptable industry density is from 1-12
		//current variable ranges from 140 (hghest density) to 1220 (lowest density)
		//conversion needed
		const uint16 industry_density = ((800 / (1+ (einstellungen->gib_industrie_dichte() / 2))) > 1)?(800 / (1+(einstellungen->gib_industrie_dichte() / 2))):1;

		tmp.Build_Industries (industry_density);
		//dbg->message("factory_builder_t::Build_Industries()", "einstellungen->gib_industrie_dichte() = %i", einstellungen->gib_industrie_dichte());

	} catch (int error_from_factory_builder)
	{
		switch (error_from_factory_builder)
		{
			case 0 :
				dbg->message("factory_builder_t::Build_Industries()", "Cell size not set or set incorrectly");
				break;
			case 1 :
				dbg->message("factory_builder_t::Build_Industries()", "World size not set or 0");
				break;
			case 2 :
				dbg->message("factory_builder_t::Build_Industries()", "Cell size too small");
				break;
			case 100 :
				dbg->message("factory_builder_t::Build_Industries()", "Cannot find suppliers for some type of goods! Check if there are not some industry packs missing");
				break;
			case 101 :
				dbg->message("factory_builder_t::Build_Industries()", "Found no industries. Check the location of pak files!");
				break;
			case 102 :
				dbg->message("factory_builder_t::Build_Industries()", "Found no end consumers!. Check the location of pak files!");
				break;

			default:
				dbg->message("factory_builder_t::Build_Industries()", "Some exception occured during industry distribution process.");
				break;

		}

	}


	////////////////////////
	////////////////////////
	////////////////////////
	////////////////////////
	////////////////////////


    for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	// Hajo: do final init after world was loaded/created
        if(stadt->at(i)) {
	    stadt->at(i)->laden_abschliessen();
	}
    }

    print("Preparing startup ...\n");

    zeiger = new zeiger_t(this, koord3d(0,0,0), spieler[0]);

    mouse_funk = NULL;
    setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN);

    reliefkarte_t::gib_karte(this)->init();

#ifndef DEMO
    for(i = 0; i < 6; i++) {
	spieler[i + 2]->automat = umgebung_t::automaten[i];
    }
#endif

    intr_enable();

    if(is_display_init()) {
	display_show_pointer(true);
    }
}

karte_t::karte_t()
{
    setze_dirty();

    einstellungen_t * sets = new einstellungen_t();

    if(umgebung_t::testlauf) {
	sets->setze_groesse(256);
	sets->setze_anzahl_staedte(16);
	sets->setze_industrie_dichte(800);
	sets->setze_verkehr_level(7);
	sets->setze_karte_nummer( 33 );

    } else {
	sets->setze_groesse(64);
	sets->setze_anzahl_staedte(1);
	sets->setze_industrie_dichte(800);
	sets->setze_verkehr_level(7);
	sets->setze_karte_nummer( 33 );

    }

    stadt = NULL;
    zeiger = NULL;
    plan = NULL;
    grid_hgts = NULL;
    einstellungen = NULL;

    for(i=0; i<anz_spieler ; i++) {
	spieler[i] = NULL;
    }

    init(sets);
}

karte_t::~karte_t()
{
    destroy();

    if(einstellungen) {
        delete einstellungen;
        einstellungen = NULL;
    }
}

bool karte_t::can_lower_plan_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));

    if(plan == 0 || !is_plan_height_changeable(x, y)) {
	return false;
    }

    int hmax = plan->gib_kartenboden()->gib_hoehe();
    //
    // irgendwo ein Tunnel vergraben?
    //
    while(h < hmax) {
	if(plan->gib_boden_in_hoehe(h)) {
	    return false;
	}
	h += 16;
    }
    return true;
}

bool karte_t::can_raise_plan_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));

    if(plan == 0 || !is_plan_height_changeable(x, y)) {
	return false;
    }

    int hmin = plan->gib_kartenboden()->gib_hoehe();
    //
    // irgendwo eine Br�cke im Weg?
    //
    while(h > hmin) {
	if(plan->gib_boden_in_hoehe(h)) {
	    return false;
	}
	h -= 16;
    }
    return true;
}


bool karte_t::is_plan_height_changeable(int x, int y) const
{
    const planquadrat_t *plan = lookup(koord(x,y));
    bool ok = true;

    if(plan != NULL) {
	grund_t *gr = plan->gib_kartenboden();

	ok = gr->ist_natur() || gr->ist_wasser();

	for(int i=0; ok && i<gr->gib_top(); i++) {
	    const ding_t *dt = gr->obj_bei(i);
	    if(dt != NULL) {
		ok =
		    dt->gib_typ() != ding_t::gebaeude &&    // Bohrinsel!
		    dt->gib_typ() != ding_t::gebaeude_alt &&    // Bohrinsel!
		    dt->gib_typ() != ding_t::zeiger &&
		    dt->gib_typ() != ding_t::automobil &&
		    dt->gib_typ() != ding_t::waggon &&
		    dt->gib_typ() != ding_t::schiff &&
		    dt->gib_typ() != ding_t::bruecke;
	    }
	}
    }

    return ok;
}


bool karte_t::can_raise_to(int x, int y, int h) const
{
//    printf("karte_t::can_raise_to %d %d %d\n", x, y, h);

    const planquadrat_t *plan = lookup(koord(x,y));
    bool ok = true;		// annahme, es geht, pruefung ob nicht

    if(plan) {
	if(lookup_hgt(koord(x, y)) < h) {
	    ok =
		// Nachbar-Planquadrate testen
		can_raise_plan_to(x-1,y-1, h) &&
		can_raise_plan_to(x,y-1, h)   &&
		can_raise_plan_to(x-1,y, h)   &&
		can_raise_plan_to(x,y, h)     &&
		// Nachbar-Gridpunkte testen
		can_raise_to(x-1, y-1, h-16) &&
		can_raise_to(x  , y-1, h-16) &&
		can_raise_to(x+1, y-1, h-16) &&
		can_raise_to(x-1, y  , h-16) &&
		can_raise_to(x+1, y  , h-16) &&
		can_raise_to(x-1, y+1, h-16) &&
		can_raise_to(x  , y+1, h-16) &&
		can_raise_to(x+1, y+1, h-16);
	}
    } else {
	ok = false;
    }
    return ok;
}

bool karte_t::can_raise(int x, int y) const
{
//    printf("karte_t::can_raise %d %d\n", x, y);

    if(ist_in_gittergrenzen(x, y)) {
	return can_raise_to(x, y, lookup_hgt(koord(x, y))+16);
    } else {
	return true;
    }
}

int karte_t::raise_to(int x, int y, int h)
{
//    printf("karte_t::raise_to %d %d %d\n", x, y, h);

    planquadrat_t *plan = access(x,y);

    int n = 0;

    if(ist_in_gittergrenzen(x,y)) {
	if(lookup_hgt(koord(x, y)) < h) {
	    setze_grid_hgt(koord(x, y), h);

	    n = 1;

	    n += raise_to(x-1, y-1, h-16);
	    n += raise_to(x  , y-1, h-16);
	    n += raise_to(x+1, y-1, h-16);
	    n += raise_to(x-1, y  , h-16);

	    n += raise_to(x, y, h);

	    n += raise_to(x+1, y  , h-16);
	    n += raise_to(x-1, y+1, h-16);
	    n += raise_to(x  , y+1, h-16);
	    n += raise_to(x+1, y+1, h-16);

	    if(plan)
		plan->angehoben(this);

	    if((plan = access(x-1,y)) != NULL)
		plan->angehoben(this);

	    if((plan = access(x,y-1)) != NULL)
		plan->angehoben(this);

	    if((plan = access(x-1,y-1)) != NULL)
		plan->angehoben(this);
	}
    }
    return n;
}

int karte_t::raise(koord pos)
{
//    printf("karte_t::raise %d %d\n", x, y);

    bool ok = can_raise(pos.x, pos.y);

    int n = 0;

    if(ok && ist_in_gittergrenzen(pos.x, pos.y)) {
	n = raise_to(pos.x, pos.y, lookup_hgt(pos)+16);
    }
    return n;
}

bool karte_t::can_lower_to(int x, int y, int h) const
{
    const planquadrat_t *plan = lookup(koord(x,y));
    bool ok = true;		// annahme, es geht, pruefung ob nicht

    if(plan) {
	if(lookup_hgt(koord(x, y)) > h) {
	    ok =
		// Nachbar-Planquadrate testen
		can_lower_plan_to(x-1,y-1, h) &&
		can_lower_plan_to(x,y-1, h)   &&
		can_lower_plan_to(x-1,y, h)   &&
		can_lower_plan_to(x,y, h)     &&
		// Nachbar-Gridpunkte testen
		can_lower_to(x-1, y-1, h+16) &&
		can_lower_to(x  , y-1, h+16) &&
		can_lower_to(x+1, y-1, h+16) &&
		can_lower_to(x-1, y  , h+16) &&
		can_lower_to(x+1, y  , h+16) &&
		can_lower_to(x-1, y+1, h+16) &&
		can_lower_to(x  , y+1, h+16) &&
		can_lower_to(x+1, y+1, h+16);
	}
    } else {
	ok = false;
    }
    return ok;
}

bool karte_t::can_lower(int x, int y) const
{
    if(x >= 0 && y >= 0 && x <= gib_groesse() && y<= gib_groesse()) {
	return can_lower_to(x, y, lookup_hgt(koord(x, y))-16);
    } else {
	return true;
    }
}

int karte_t::lower_to(int x, int y, int h)
{
    planquadrat_t *plan = access(x,y);

    int n = 0;

    if(ist_in_gittergrenzen(x,y)) {
	if(lookup_hgt(koord(x, y)) > h) {
	    setze_grid_hgt(koord(x, y), h);

	    n = 1;

	    n += lower_to(x-1, y-1, h+16);
	    n += lower_to(x  , y-1, h+16);
	    n += lower_to(x+1, y-1, h+16);
	    n += lower_to(x-1, y  , h+16);

	    n += lower_to(x, y, h);

	    n += lower_to(x+1, y  , h+16);
	    n += lower_to(x-1, y+1, h+16);
	    n += lower_to(x  , y+1, h+16);
	    n += lower_to(x+1, y+1, h+16);

	    if(plan)
		plan->abgesenkt( this );

	    if((plan = access(x-1,y)) != NULL)
		plan->abgesenkt( this );

	    if((plan = access(x,y-1)) != NULL)
		plan->abgesenkt( this );

	    if((plan = access(x-1,y-1)) != NULL)
		plan->abgesenkt( this );

	}
    }

    return n;
}


int karte_t::lower(koord pos)
{
    bool ok = can_lower(pos.x, pos.y);

    int n = 0;

    if(ok && ist_in_gittergrenzen(pos.x, pos.y)) {
	n = lower_to(pos.x, pos.y, lookup_hgt(pos)-16);
    }

    return n;
}

bool
karte_t::ebne_planquadrat(koord pos, int hgt)
{
    koord offsets[] = {koord(0,0), koord(1,0), koord(0,1), koord(1,1)};
    bool ok = true;

    for(int i=0; i<4; i++) {
	koord p = pos + offsets[i];

	if(lookup_hgt(p) > hgt) {

	    if(can_lower_to(p.x, p.y, hgt)) {
		lower_to(p.x, p.y, hgt);
	    } else {
		ok = false;
	    }

	} else if(lookup_hgt(p) < hgt) {

	    if(can_raise_to(p.x, p.y, hgt)) {
		raise_to(p.x, p.y, hgt);
	    } else {
		ok = false;
	    }
	}
    }
    return ok;
}


void
karte_t::setze_maus_funktion(int (* funktion)(spieler_t *, karte_t *, koord),
                             int zeiger_bild,
			     int zeiger_versatz,
			     int ok_sound /*= 0*/,
			     int ko_sound /*= 0*/)
{
    setze_maus_funktion(
	(int (*)(spieler_t *, karte_t *, koord, long))funktion,
	zeiger_bild, zeiger_versatz, 0, ok_sound, ko_sound);
}


void
karte_t::setze_maus_funktion(int (* funktion)(spieler_t *, karte_t *, koord,
					      long),
                             int zeiger_bild,
			     int zeiger_versatz,
			     long param,
			     int ok_sound /*= 0*/,
			     int ko_sound /*= 0*/)
{
    // gibt es eien neue funktion ?
    if(funktion != NULL) {
	// gab es eine alte funktion ?
	if(mouse_funk != NULL) {
	    mouse_funk(spieler[0], this, EXIT, mouse_funk_param);
	}

	struct sound_info info;
	info.index = SFX_SELECT;
	info.volume = 255;
	info.pri = 0;
	sound_play(info);

	mouse_funk = funktion;
	mouse_funk_param = param;
	zeiger->setze_yoff(zeiger_versatz);
	zeiger->setze_bild(0, zeiger_bild);

	mouse_funk(spieler[0], this, INIT, mouse_funk_param);
    }

    mouse_funk_ok_sound = ok_sound;
    mouse_funk_ko_sound = ko_sound;
}


void karte_t::new_mountain(int x, int y, int w, int h, int t)
{
    int i,j,n;

    dbg->message("karte_t::new_mountain()",
		 "New mountain %d+%d, %d+%d, %d",x,w,y,h,t);

    for(n=0; n<abs(t); n++){
	for(i=x; i<=x+w; i++) {
	    for(j=y; j<=y+h; j++) {
		if(t < 0) {
		    lower(koord(i,j));
		} else {
		    raise(koord(i,j));
		}
	    }
	}
    }
}


planquadrat_t *
karte_t::access(const int i, const int j)
{
    if(ist_in_kartengrenzen(i, j)) {
	return &plan[i + j*gib_groesse()];
    }
    return NULL;
}



/**
 * Sets grid height. Synchronized map grounds with
 * new height. Never set grid_hgts manually, always
 * use this method.
 * @author Hj. Malthaner
 */
void karte_t::setze_grid_hgt(koord k, short hgt)
{
  if(ist_in_gittergrenzen(k.x, k.y)) {
    grid_hgts[k.x + k.y*(cached_groesse+1)] = hgt;
  }
}


int
karte_t::min_hgt(const koord pos) const
{
    const int h1 = lookup_hgt(pos);
    const int h2 = lookup_hgt(pos+koord(1, 0));
    const int h3 = lookup_hgt(pos+koord(1, 1));
    const int h4 = lookup_hgt(pos+koord(0, 1));

    return min(min(h1,h2), min(h3,h4));
}


int
karte_t::max_hgt(const koord pos) const
{
    const int h1 = lookup_hgt(pos);
    const int h2 = lookup_hgt(pos+koord(1, 0));
    const int h3 = lookup_hgt(pos+koord(1, 1));
    const int h4 = lookup_hgt(pos+koord(0, 1));

    return max(max(h1,h2), max(h3,h4));
}


// -------- Verwaltung von Fabriken -----------------------------

bool
karte_t::add_fab(fabrik_t *fab)
{
    assert(fab != NULL);
    fab_list.insert( fab );
    return true;
}

bool
karte_t::rem_fab(fabrik_t *fab)
{
    assert(fab != NULL);
    return fab_list.remove( fab );
}

int
karte_t::gib_fab_index(fabrik_t *fab)
{
    return fab_list.index_of(fab);
}


fabrik_t *
karte_t::gib_fab(int index)
{
    return (fabrik_t *)fab_list.at(index);
}

const slist_tpl<fabrik_t *> &
karte_t::gib_fab_list() const
{
    return fab_list;
}

void
karte_t::add_ausflugsziel(gebaeude_t *gb)
{
    assert(gb != NULL);
    ausflugsziele.insert( gb );
}

const slist_tpl<gebaeude_t*> &
karte_t::gib_ausflugsziele() const
{
    return ausflugsziele;
}


void karte_t::add_label(koord pos)
{
    if(!labels.contains(pos)) {
	labels.append(pos);
    }
}

void karte_t::remove_label(koord pos)
{
    labels.remove(pos);
}


const slist_tpl<koord> &
karte_t::gib_label_list() const
{
    return labels;
}


const slist_tpl<convoihandle_t> &
karte_t::gib_convoi_list() const
{
    return convoi_list;
}


bool
karte_t::add_convoi(convoihandle_t &cnv)
{
    assert(cnv.is_bound());
    convoi_list.insert( cnv );
    return true;
}


bool
karte_t::rem_convoi(convoihandle_t &cnv)
{
    return convoi_list.remove( cnv );
}


fabrik_t *
karte_t::get_random_fab() const
{
    const int anz = fab_list.count();
    fabrik_t *result = NULL;

    if(anz > 0) {
        const int end = simrand( anz );
	result = (fabrik_t *)fab_list.at(end);

    }
    return result;
}


// -------- Verwaltung von Staedten -----------------------------

stadt_t *
karte_t::suche_naechste_stadt(const koord pos) const
{
    int min_dist = 99999999;
    stadt_t * best = NULL;

    for(int n=0; n<einstellungen->gib_anzahl_staedte(); n++) {
	if( stadt->at(n) ) {
	    const koord k = stadt->at(n)->gib_pos();

	    const int dist = (pos.x-k.x)*(pos.x-k.x) + (pos.y-k.y)*(pos.y-k.y);

	    if(dist < min_dist) {

//		printf("dist %d  stadt %d\n", dist, n);
		min_dist = dist;
		best = stadt->at(n);
	    }
	}
    }
    return best;
}

stadt_t *
karte_t::suche_naechste_stadt(const koord pos, const stadt_t *letzte) const
{
    int min_dist = 99999999;
    stadt_t * best = NULL;
    const koord letzte_pos = letzte->gib_pos();
    const int letzte_dist = (letzte_pos.x-pos.x)*(letzte_pos.x-pos.x) + (letzte_pos.y-pos.y)*(letzte_pos.y-pos.y);
    bool letzte_gefunden = false;

    for(int n=0; n<einstellungen->gib_anzahl_staedte(); n++) {
	if( stadt->at(n) ) {
	    const koord k = stadt->at(n)->gib_pos();

	    const int dist = (pos.x-k.x)*(pos.x-k.x) + (pos.y-k.y)*(pos.y-k.y);

	    if(stadt->at(n) == letzte) {
		letzte_gefunden = true;
	    }
	    else if(letzte_gefunden ? dist >= letzte_dist : dist > letzte_dist) {
		if(dist < min_dist) {
//		    printf("dist %d  stadt %d\n", dist, n);
		    min_dist = dist;
		    best = stadt->at(n);
		}
	    }
	}
    }
    return best;
}


/**
 * suche Haltestellen fuer einen Passagier/fracht
 * um den Punkt k herum - k selbst wird nicht gepr�ft!!!
 * liefert vector gefundener Haltestellen zur�ck
 * @author Hj. Malthaner
 */
vector_tpl<halthandle_t> &
karte_t::suche_nahe_haltestellen(const koord k,
                                 const int radius,
                                 const int mitte_wh,
				 uint32 max_anzahl) const
{
    static vector_tpl<halthandle_t> halt_list(32);

    halthandle_t halt;
    koord p;


    max_anzahl = MIN(31, max_anzahl);

    halt_list.clear();

    for(int rad=1; rad<radius; rad ++) {
	// obere zeile
	p.y = k.y - rad;

	for(p.x=k.x-rad; p.x<=k.x+rad; p.x++) {
	    halt = haltestelle_t::gib_halt(this, p);

	    if(halt.is_bound()) {
		halt_list.append_unique( halt );
		if(halt_list.get_count() >= max_anzahl) {
		    return halt_list;
		}
	    }
	}

	// untere zeile
	p.y = k.y + rad;

	for(p.x=k.x-rad; p.x<=k.x+rad; p.x++) {
	    halt = haltestelle_t::gib_halt(this, p);

	    if(halt.is_bound()) {
		halt_list.append_unique( halt );
		if(halt_list.get_count() >= max_anzahl) {
		    return halt_list;
		}
	    }
	}

        // rand links und rechts

	for(p.y=k.y-rad+1; p.y<=k.y+rad-1; p.y++) {
	    p.x = k.x-rad;
	    halt = haltestelle_t::gib_halt(this, p);

	    if(halt.is_bound()) {
		halt_list.append_unique( halt );
		if(halt_list.get_count() >= max_anzahl) {
		    return halt_list;
		}
	    }

	    p.x = k.x+rad;
	    halt = haltestelle_t::gib_halt(this, p);
	    if(halt.is_bound()) {
		halt_list.append_unique( halt );
		if(halt_list.get_count() >= max_anzahl) {
		    return halt_list;
		}
	    }
	}
    }

    if(mitte_wh != 0) {
        // untere extra-zeile

	p.y = k.y + radius;

	for(p.x = k.x-radius; p.x<k.x+radius+1; p.x++) {
	    halt = haltestelle_t::gib_halt(this, p);
	    if(halt.is_bound()) {
		halt_list.append_unique( halt );
		if(halt_list.get_count() >= max_anzahl) {
		    return halt_list;
		}
	    }
	}


	// rechte extra zeile
        p.x = k.x + radius;

	for(p.y = k.y-radius; p.y<k.y+radius; p.y++) {
	    halt = haltestelle_t::gib_halt(this, p);
	    if(halt.is_bound()) {
		halt_list.append_unique( halt );
		if(halt_list.get_count() >= max_anzahl) {
		    return halt_list;
		}
	    }
	}

    }

    return halt_list;
}



// -------- Verwaltung von synchronen Objekten ------------------

bool
karte_t::sync_add(sync_steppable *obj)
{
    assert(obj != NULL);

    sync_list.insert( obj );

    return true;
}

bool
karte_t::sync_remove(sync_steppable *obj)	// entfernt alle dinge == obj aus der Liste
{
    assert(obj != NULL);

    return sync_list.remove( obj );
}

void
karte_t::sync_prepare()
{
    slist_iterator_tpl<sync_steppable *> iter (sync_list);

    while(iter.next()) {
	sync_steppable *ss = iter.get_current();
	ss->sync_prepare();
    }
}

void
karte_t::sync_step(const long delta_t)
{

  slist_iterator_tpl<sync_steppable*> iter (sync_list);

  // Hajo: we use a slight hack here to remove the current
  // object from the list without wrecking the iterator

  bool ok = iter.next();

  while(ok) {
    sync_steppable *ss = iter.get_current();


    // unsigned long T0 = get_current_time_millis();

    // Hajo: advance iterator, so that we can remove the current object
    // safely
    ok = iter.next();

    if(ss->sync_step(delta_t) == false) {
      sync_list.remove(ss);
      delete ss;
    }

    // unsigned long T1 = get_current_time_millis();

    // if(T1-T0 > 40) {
    //    dbg->warning("karte_t::sync_step()", "%p needed %d ms for step", ss, T1-T0);
    // }

  }

  ticks += delta_t;

  // Hajo: ein weiterer Frame
  thisFPS++;
}


void
karte_t::neuer_monat()
{
    letzter_monat ++;

    dbg->message("karte_t::neuer_monat()",
		 "Month %d has started", letzter_monat);

    slist_iterator_tpl<fabrik_t*> iter (fab_list);
    int i;

    while(iter.next()) {
	fabrik_t * fab = iter.get_current();
	fab->neuer_monat();
    }

    INT_CHECK("simworld 1278");

    for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	stadt->at(i)->neuer_monat();
	INT_CHECK("simworld 1282");
    }

    // spieler
    for(i=0; i<anz_spieler; i++) {
	if(spieler[i] != NULL) {
	    spieler[i]->neuer_monat();
	}
    }

    INT_CHECK("simworld 1289");
}


void karte_t::neues_jahr()
{
    letztes_jahr ++;

    dbg->message("karte_t::neues_jahr()",
		 "Year %d has started", letztes_jahr);

    slist_iterator_tpl<convoihandle_t> iter (convoi_list);

    while(iter.next()) {
	convoihandle_t cnv = iter.get_current();
	cnv->neues_jahr();
    }

    for(int i=0; i<anz_spieler; i++) {
	gib_spieler(i)->neues_jahr();
    }
}


#define GRUPPEN 14
static long step_group_times[GRUPPEN];


void karte_t::step(const long delta_t)
{
    const int step_group = steps % GRUPPEN;
    const int step_group_step = steps / GRUPPEN;
    unsigned int i;

    for(i=0; i<GRUPPEN; i++) {
	step_group_times[i] += delta_t;
    }

    const long delta_t_sum = step_group_times[step_group];
    step_group_times[step_group] = 0;

    int check_counter = 0;


    for(i=step_group; i < cached_groesse2; i+=GRUPPEN) {

	plan[i].step(delta_t_sum, step_group_step);

	if(((++check_counter) & 63) == 0) {
	    INT_CHECK("simworld 1076");
	    interactive_update();
	}
    }

    steps ++;

    if(ticks > ((letzter_monat + 1)) << ticks_bits_per_tag) {
	// neuer monat

	neuer_monat();

	if(letzter_monat % 12 == 0) {
            neues_jahr();
	}
    }
}


void
karte_t::blick_aendern(event_t *ev)
{
  const int raster = get_tile_raster_width();
  const int xmask = raster - 1;
  const int ymask = raster/2 - 1;

  int xd = -x_off/2;
  int yd = -y_off/2;

  xd += (ev->mx - ev->cx) * einstellungen->gib_scroll_multi();
  yd += (ev->my - ev->cy) * einstellungen->gib_scroll_multi();

  const int dx = (xd & ~xmask) / (raster/2);
  const int dy = (yd & ~ymask) / (raster/4);

  x_off = -(xd & xmask)*2;
  y_off = -(yd & ymask)*2;

  // koord(i_off + (dx+dy), j_off + (dy-dx));
  setze_ij_off(gib_ij_off() + koord(dx+dy, dy-dx));


  if ((ev->mx - ev->cx) != 0 || (ev->my - ev->cy) != 0) {
    intr_refresh_display( true );
    display_move_pointer(ev->cx, ev->cy);
  }
}


// nordhang = 3
// suedhang = 12

int
karte_t::calc_hang_typ(const koord pos) const
{
  if(ist_in_kartengrenzen(pos.x, pos.y)) {

    short * p = &grid_hgts[pos.x + pos.y*(cached_groesse+1)];

    const int h1 = *p;
    const int h2 = *(p+1);
    const int h3 = *(p+cached_groesse+2);
    const int h4 = *(p+cached_groesse+1);

    const int mini = min(min(h1,h2), min(h3,h4));

    return ((h1>mini)<<3) +
      ((h2>mini)<<2) +
      ((h3>mini)<<1) +
      ((h4>mini));
  } else {
    return 0;  // Hajo: Annahme: ausserhalb der Karte ist alles Flach
  }
}

bool
karte_t::ist_wasser(koord pos, koord dim) const
{
    koord k;

    for(k.x = pos.x; k.x < pos.x + dim.x; k.x++) {
	for(k.y = pos.y; k.y < pos.y + dim.y; k.y++) {
	    if(lookup_hgt(k) > gib_grundwasser()) {
		return false;
	    }
	}
    }
    return true;
}

bool
karte_t::ist_platz_frei(koord pos, int w, int h, int *last_y) const
{
    koord k;

    if(pos.x < 0 || pos.y < 0 || pos.x+w >= (int)cached_groesse || pos.y + h >= (int)cached_groesse) {
	return false;
    }
    //
    // ACHTUNG: Schleifen sind mit finde_plaetze koordiniert, damit wir ein
    // paar Abfragen einsparen k�nnen bei h > 1!
    // V. Meyer
    for(k.y=pos.y+h-1; k.y>=pos.y; k.y--) {
	for(k.x=pos.x; k.x<pos.x+w; k.x++) {
	    const grund_t *gr = lookup( k )->gib_kartenboden();

	    if(gr->gib_grund_hang() != hang_t::flach ||
	       !gr->ist_natur() ||
	       /*gr->ist_wasser() || *//*Wasser liefert als ist_natur() false */
	       gr->kann_alle_obj_entfernen(NULL) != NULL)
	    {
		if(last_y) {
		    *last_y = k.y;
		}
		return false;
	    }
	}
    }
    return true;
}

slist_tpl<koord> *
karte_t::finde_plaetze(int w, int h) const
{
    slist_tpl<koord> * list = new slist_tpl<koord>();
    koord start;
    int last_y;

    for(start.x = 0; start.x < (short)(cached_groesse - w); start.x++) {
	for(start.y = 0; start.y < (short)(cached_groesse - h); start.y++) {
	    if(ist_platz_frei(start, w, h, &last_y)) {
		list->insert(start);
	    }
	    else {
		// Optimiert f�r gr��ere Felder, hehe!
		// Die Idee: wenn bei 2x2 die untere Reihe nicht geht, k�nnen
		// wir gleich 2 tifer weitermachen! V. Meyer
		start.y = last_y;
	    }
	}
    }
    return list;
}


/**
 * Play a sound, but only if near enoungh.
 * Sounds are muted by distance and clipped completely if too far away.
 *
 * @author Hj. Malthaner
 */
bool
karte_t::play_sound_area_clipped(koord pos, sound_info info)
{
    const int center = display_get_width() >> 7;
    const int dist = ABS((pos.x-center) - gib_ij_off().x) + ABS((pos.y-center) - gib_ij_off().y);

    if(dist < 25) {
	if(info.volume > dist*9 + 10) {
	    info.volume -= dist*9;
	} else {
	    info.volume = 8;
	}

	sound_play(info);
    }

    return dist < 25;
}



//void
//karte_t::beenden()    // 02-Nov-2001  Markus Weber    Added parameter

void
karte_t::beenden(bool quit_simutrans)
{
    doit = false;
    m_quit_simutrans = quit_simutrans; // 02-Nov-200    Markus Weber    Added
}

void
karte_t::speichern(const char *filename)
{
    display_show_pointer(false);

#ifndef DEMO
    loadsave_t  file;

    dbg->message("karte_t::speichern()", "saving game to '%s'", filename);


    if(!file.wr_open(filename, true)) {
	create_win(-1, -1, new nachrichtenfenster_t(this, "Kann Spielstand\nnicht speichern.\n"), w_autodelete);
	perror("Error");
    } else {
	speichern(&file);
	file.close();
	create_win(-1, -1, 30, new nachrichtenfenster_t(this, "Spielstand wurde\ngespeichert!\n"), w_autodelete);
    }
#endif

    display_show_pointer(true);
}


void
karte_t::speichern(loadsave_t *file)
{
    int i,j;

    einstellungen->rdwr(file);

    file->rdwr_int(ticks, " ");
    file->rdwr_int(letzter_monat, " ");
    file->rdwr_int(letztes_jahr, "\n");

    for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	stadt->at(i)->rdwr(file);
    }

    blockmanager *bm = blockmanager::gib_manager();
    bm->rdwr(this, file);

    for(j=0; j<gib_groesse(); j++) {
	for(i=0; i<gib_groesse(); i++) {
	    access(i, j)->rdwr(this, file);
	}
	display_progress(j, gib_groesse());
	display_flush(0, 0, 0, "", "");
    }

    for(j=0; j<=gib_groesse(); j++) {
	for(i=0; i<=gib_groesse(); i++) {
            int hgt = lookup_hgt(koord(i, j));
	    file->rdwr_int(hgt, "\n");
	}
    }

    int fabs = fab_list.count();

    file->rdwr_int(fabs, "\n");

    slist_iterator_tpl<fabrik_t*> fiter ( fab_list );
    while(fiter.next()) {
	(fiter.get_current())->rdwr(file);
    }

    slist_iterator_tpl<convoihandle_t> citer ( convoi_list );

    while(citer.next()) {
	(citer.get_current())->rdwr(file);
    }

    file->wr_obj_id("Ende Convois");

    for(i=0; i<8 ; i++) {
	spieler[i]->rdwr(file);
    }

    i = gib_ij_off().x;
    j = gib_ij_off().y;
    file->rdwr_delim("View ");
    file->rdwr_int(i, " ");
    file->rdwr_int(j, "\n");

    translator::rdwr( file );

    display_speichern( file->gib_file() );
}


void
karte_t::laden(const char *filename)
{
    display_show_pointer(false);

#ifndef DEMO
    loadsave_t file;

    dbg->message("karte_t::laden", "loading game from '%s'", filename);

    if(!file.rd_open(filename)) {

        if(file.get_version() == -1) {
	    create_win(-1, -1, new nachrichtenfenster_t(this, "WRONGSAVE"), w_autodelete);
        }
        else {
	    create_win(-1, -1, new nachrichtenfenster_t(this, "Kann Spielstand\nnicht laden.\n"), w_autodelete);
        }
    } else if(file.get_version() < 81009) {
	create_win(-1, -1, new nachrichtenfenster_t(this, "WRONGSAVE"), w_autodelete);

    } else {
	destroy_all_win();

	dbg->message("karte_t::laden()","Savegame version is %d", file.get_version());

	laden(&file);
	file.close();
	steps_bis_jetzt = steps-4;
	create_win(-1, -1, 30, new nachrichtenfenster_t(this, "Spielstand wurde\ngeladen!\n"), w_autodelete);
    }
#endif

    display_show_pointer(true);
}

void
karte_t::laden(loadsave_t *file)
{
    char buf[80];
    int i,j;

    setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN);

    intr_disable();

    // alte werte merken fuer neuen Zeiger
    const int bild = zeiger->gib_bild();
    const int yoff = zeiger->gib_yoff();


    // gibt alles frei

    // Zeiger entfernen
    zeiger->setze_pos(koord3d::invalid);     // war nicht in welt enthalten
    delete zeiger;
    zeiger = NULL;

    // Datenstrukturen loeschen
    destroy();


    // jetzt geht das laden los

    einstellungen->rdwr(file);

    // wird gecached, um den Pointerzugriff zu sparen, da
    // die groesse _sehr_ oft referenziert wird
    cached_groesse = einstellungen->gib_groesse();
    cached_groesse2 = cached_groesse*cached_groesse;


    //12-Jan-02     Markus Weber added
    grundwasser = einstellungen->gib_grundwasser();


    init_felder();
    hausbauer_t::neue_karte();

    file->rdwr_int(ticks, " ");
    file->rdwr_int(letzter_monat, " ");
    file->rdwr_int(letztes_jahr, "\n");
    steps = 0;

    for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	stadt->at(i) = new stadt_t(this, file);

	// printf("Loading city %s\n", stadt->get(i)->gib_name());
    }

    blockmanager *bm = blockmanager::gib_manager();
    bm->setze_welt_groesse( gib_groesse() );
    bm->rdwr(this, file);

    for(j=0; j<gib_groesse(); j++) {
	for(i=0; i<gib_groesse(); i++) {
//	    fprintf(stderr,"Lade Planquadrat (%d, %d).\n",i,j);

	    if(file->is_eof()) {
		dbg->fatal("karte_t::laden()",
			   "Savegame file mangled (too short)!");
	    }
	    access(i, j)->rdwr(this, file);
	}
	display_progress(j, gib_groesse());
	display_flush(0, 0, 0, "", "");
    }

    for(j=0; j<=gib_groesse(); j++) {
	for(i=0; i<=gib_groesse(); i++) {
	    int hgt;
	    file->rdwr_int(hgt, "\n");
	    setze_grid_hgt(koord(i, j), hgt);
	}
    }

    if(file->get_version() < 80005) {
	while( true ) {
	    file->rd_obj_id(buf, 79);

    //	printf("%s", buf);

	    if(strequ(buf, "Ende Fablist")) {
		break;
	    } else {
		// liste in gleicher reihenfolge wie vor dem speichern wieder aufbauen
		fab_list.append( new fabrik_t(this, file) );
	    }
	}
    } else {
	int fabs;

	file->rdwr_int(fabs, "\n");

	for(i = 0; i < fabs; i++) {
	    // liste in gleicher reihenfolge wie vor dem speichern wieder aufbauen
	    fab_list.append( new fabrik_t(this, file) );
	}
    }

    dbg->message("karte_t::laden()", "%d factories loaded", fab_list.count());

    while( true ) {
        file->rd_obj_id(buf, 79);

//	printf("%s", buf);

	if(strequ(buf, "Ende Convois")) {
	    break;
	} else {
	    convoi_t *cnv = new convoi_t(this, file);
	    convoi_list.insert( cnv->self );
	    sync_add( cnv );
	}
    }

    dbg->message("karte_t::laden()", "%d convois/trains loaded", convoi_list.count());

    // reinit pointer with new pointer object and old values
    zeiger = new zeiger_t(this, koord3d(0,0,0), spieler[0]);	// Zeiger ist spezialobjekt
    zeiger->setze_bild(0, bild);
    zeiger->setze_yoff( yoff );

    // jetzt k�nnen die spieler geladen werden
    for(i=0; i<8 ; i++) {
	spieler[i]->rdwr(file);
    }
    for(i = 0; i < 6; i++) {
	umgebung_t::automaten[i] = spieler[i + 2]->automat;
    }

    // nachdem die welt jetzt geladen ist k�nnen die Blockstrecken neu
    // angelegt werden
    bm->laden_abschliessen();

    // auch die fabrikverbindungen k�nnen jetzt neu init werden
    for(i=0; i<einstellungen->gib_anzahl_staedte(); i++) {
	stadt->at(i)->laden_abschliessen();
    }


    file->rdwr_delim("View ");
    file->rdwr_int(i, " ");
    file->rdwr_int(j, "\n");
    dbg->message("karte_t::laden()", "Setting view to %d,%d", i, j);
    setze_ij_off(koord(i, j));

    translator::rdwr( file );

    display_laden( file->gib_file());

    for(j=0; j<gib_groesse(); j++) {
	for(i=0; i<gib_groesse(); i++) {

	  const planquadrat_t *plan = lookup(koord(i,j));
	  const int boden_count = plan->gib_boden_count();

	  for(int schicht=0; schicht<boden_count; schicht++) {

	    grund_t *gr = plan->gib_boden_bei(schicht);

	    gr->calc_bild();

	    for(int n=0; n<gr->gib_top(); n++) {
	      ding_t *d = gr->obj_bei(n);

	      if(d) {
		d->laden_abschliessen();

		/* Hajo: not needed anymore
		switch(d->gib_typ()) {
		case ding_t::gebaeudefundament:
		  d->setze_bild(0, grund_besch_t::fundament->gib_bild(calc_hang_typ(koord(i,j))));
		  break;
		case ding_t::signal:
		  dynamic_cast<signal_t *>(d)->calc_bild();
		  break;
		case ding_t::oberleitung:
		  dynamic_cast<oberleitung_t *>(d)->calc_bild();
		  break;
		default:
		  // Hajo: do nothing
		  break;
		}
		*/
	      }
	    }
	  }
	}
    }


    // Reliefkarte an neue welt anpassen
    win_setze_welt( this );
    reliefkarte_t::gib_karte( this )->init();

    setze_dirty();

    intr_enable();
}

// Hilffunktionen zum Speichern

int
karte_t::sp2num(spieler_t *sp)
{
    for(int i=0; i<8; i++) {
	if(spieler[i] == sp) {
	    return i;
	}
    }
    return -1;
}

// ende karte_t methoden



/**
 * Creates a map from a heightfield
 * @param sets game settings
 * @author Hansj�rg Malthaner
 */
void karte_t::load_heightfield(einstellungen_t *sets)
{
  FILE *file = fopen(sets->heightfield, "rb");

  if(file) {
    char buf [256];
    int w, h;

    read_line(buf, 255, file);

    if(strncmp(buf, "P6", 2)) {
      dbg->error("karte_t::load_heightfield()",
		 "Heightfield has wrong image type %s", buf);
      create_win(-1, -1, new nachrichtenfenster_t(this, "\nHeightfield has wrong image type.\n"), w_autodelete);
      return;
    }

    read_line(buf, 255, file);

    sscanf(buf, "%d %d", &w, &h);

    if(w != h) {
      dbg->error("karte_t::load_heightfield()",
		 "Heightfield isn't square!");
      create_win(-1, -1, new nachrichtenfenster_t(this, "\nHeightfield isn't square.\n"), w_autodelete);
      return;
    }

    sets->setze_groesse(w);

    read_line(buf, 255, file);

    if(strncmp(buf, "255", 2)) {
      dbg->fatal("karte_t::load_heightfield()",
		 "Heightfield has wrong color depth %s", buf);
    }


    // create map
    init(sets);
  }
  else {
    dbg->error("karte_t::load_heightfield()",
	       "Cant open file '%s'", sets->heightfield.chars());

    create_win(-1, -1, new nachrichtenfenster_t(this, "\nCan't open heightfield file.\n"), w_autodelete);
  }
}


// Hilfmethoden fuer Interaktion

void warte_auf_taste(event_t *ev)
{
    do {
	win_get_event(ev);
    }while( !(ev->ev_class == EVENT_KEYBOARD && ev->ev_code < 256));
}



static void warte_auf_mausklick_oder_taste(event_t *ev)
{
    do {
	win_get_event(ev);
    }while( !(IS_LEFTRELEASE(ev) ||
	      (ev->ev_class == EVENT_KEYBOARD && ev->ev_code < 256)));
}


static void do_pause() {
    display_fillbox_wh(display_get_width()/2-100, display_get_height()/2-50,
                               200,100, MN_GREY2, true);
    display_ddd_box(display_get_width()/2-100, display_get_height()/2-50,
                                200,100, MN_GREY4, MN_GREY0);
    display_ddd_box(display_get_width()/2-92, display_get_height()/2-42,
                                200-16,100-16, MN_GREY0, MN_GREY4);

    display_proportional(display_get_width()/2, display_get_height()/2-5,
                                     translator::translate("GAME PAUSED"),
                                     ALIGN_MIDDLE, SCHWARZ, false);

    // Pause: warten auf die n�chste Taste
    event_t ev;
    display_flush_buffer();
    warte_auf_mausklick_oder_taste(&ev);
    intr_set_last_time(get_current_time_millis());
}

void karte_t::zentriere_auf(koord k)
{
  setze_ij_off(k + koord(-5,-5));
}


ding_t *
karte_t::gib_zeiger() const
{
    return zeiger;
}


spieler_t *
karte_t::gib_spieler(int n)
{
    assert(n>=0 && n<anz_spieler);
    assert(spieler[n] != NULL);
    return spieler[n];
}


void karte_t::bewege_zeiger(const event_t *ev)
{
    if(zeiger) {
        const int rw1 = get_tile_raster_width();
	const int rw2 = rw1/2;
	const int rw4 = rw1/4;

	const int scale = rw1/64;

	int i_alt=zeiger->gib_pos().x;
	int j_alt=zeiger->gib_pos().y;

	int screen_y = ev->my - y_off - rw2;
	int screen_x = (ev->mx - x_off - rw2 - display_get_width()/2) / 2;

	if(zeiger->gib_yoff() == Z_PLAN) {
	    screen_y -= rw4;
	} else if(zeiger->gib_yoff() == Z_LINES) {
	    screen_y -= rw4/2;
	}


	// berechnung der basis feldkoordinaten in i und j

	/*  this would calculate raster i,j koordinates if there was no height
	 *  die formeln stehen hier zur erinnerung wie sie in der urform aussehen

	 int base_i = (screen_x+screen_y)/2;
	 int base_j = (screen_y-screen_x)/2;

	 int raster_base_i = (int)floor(base_i / 16.0);
	 int raster_base_j = (int)floor(base_j / 16.0);

	 */

	// iterative naeherung fuer zeigerposition
	// iterative naehreung an gelaendehoehe

	int hgt = lookup_hgt(koord(i_alt, j_alt));

	const int i_off = gib_ij_off().x;
	const int j_off = gib_ij_off().y;

	for(int n = 0; n < 2; n++) {

	    const int base_i = (screen_x+screen_y+hgt*scale)/2;
	    const int base_j = (screen_y-screen_x+hgt*scale)/2;

	    i = ((int)floor(base_i/(double)rw4)) + i_off;
	    j = ((int)floor(base_j/(double)rw4)) + j_off;


	    if(zeiger->gib_yoff() == Z_GRID) {
		hgt = lookup_hgt(koord(i, j));
	    } else {
		const planquadrat_t *plan = lookup(koord(i,j));
		if(plan != NULL) {
		    hgt = plan->gib_kartenboden()->gib_hoehe();
		} else {
		    hgt = grundwasser;
		}
	    }

	    if(hgt < grundwasser) {
		hgt = grundwasser;
	    }
	}

	// rueckwaerttransformation um die zielkoordinaten
	// mit den mauskoordinaten zu vergleichen

	int neu_x = ((i-i_off) - (j-j_off))*rw2;
	int neu_y = ((i-i_off) + (j-j_off))*rw4;

	neu_x += display_get_width()/2 + rw2;
	neu_y += rw1;


	// pr�fe richtung d.h. welches nachbarfeld ist am naechsten

	if(ev->mx-x_off < neu_x) {
	    zeiger->setze_richtung(ribi_t::west);
	} else {
	    zeiger->setze_richtung(ribi_t::nord);
	}


	// zeiger bewegen

	if(i >= 0 && j >= 0 && i<gib_groesse() && j<gib_groesse() && (i != i_alt || j != j_alt)) {

	    i_alt = i;
	    j_alt = j;

	    const koord3d pos = lookup(koord(i,j))->gib_kartenboden()->gib_pos();
	    zeiger->setze_pos(pos);
	}
    }
}

void
karte_t::interactive_event(event_t &ev)
{
    struct sound_info click_sound;

    click_sound.index = SFX_SELECT;
    click_sound.volume = 255;
    click_sound.pri = 0;


    if(ev.ev_class == EVENT_KEYBOARD) {
	dbg->message("karte_t::interactive_event()",
		     "Keyboard event with code %d '%c'",
		     ev.ev_code, ev.ev_code);

	switch(ev.ev_code) {
	case ',':
	    sound_play(click_sound);
	    set_time_multi(get_time_multi()-1);
	    break;
	case '.':
	    sound_play(click_sound);
	    set_time_multi(get_time_multi()+1);
	    break;
	case '!':
	    sound_play(click_sound);
            umgebung_t::show_names = !umgebung_t::show_names;
	    setze_dirty();
	    break;
	case '"':
	    sound_play(click_sound);
	    gebaeude_t::hide = !gebaeude_t::hide;
	    setze_dirty();
	    break;
	case '#':
	    sound_play(click_sound);
	    boden_t::toggle_grid();
	    setze_dirty();
	    break;
	case 167:    // Hajo: '�'
	    verteile_baeume(3);
	    break;
	case 'a':
	    setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN);
	    break;
	case 'b':
	    setze_maus_funktion(wkz_blocktest, skinverwaltung_t::signalzeiger->gib_bild_nr(0), Z_PLAN);
	    break;
	case 'c':
	    sound_play(click_sound);
	    display_snapshot();
	    create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Screenshot\ngespeichert.\n"), w_autodelete);
            break;
	case 'd':
	    setze_maus_funktion(wkz_lower, skinverwaltung_t::downzeiger->gib_bild_nr(0), Z_GRID);
	    break;
	case 'e':
	    setze_maus_funktion(wkz_electrify_block,
				skinverwaltung_t::oberleitung->gib_bild_nr(8),
				Z_PLAN);
	    break;
	case 'f': /* OCR: Finances */
	    sound_play(click_sound);
	    create_win(-1, -1, -1, new money_frame_t(gib_spieler(0)), w_autodelete, magic_finances_t);
	    break;
	case 'g':
	    if(skinverwaltung_t::senke) {
		setze_maus_funktion(wkz_senke, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN);
	    }
	    break;

	case 'I':
	  setze_maus_funktion(wkz_build_industries, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN);
	    break;

	case 'k':
	    create_win(272,160, -1, new ki_kontroll_t(this), w_info, magic_ki_kontroll_t);
	    break;
	case 'l':
	    if(wegbauer_t::leitung_besch) {
		setze_maus_funktion(wkz_wegebau, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN,
		    (long)wegbauer_t::leitung);
	    }
	    break;
	case 'm':
	    create_win(-1, -1, -1, new map_frame_t(this), w_info, magic_reliefmap);
	    break;
	case 'M':
	    setze_maus_funktion(wkz_marker, skinverwaltung_t::belegtzeiger->gib_bild_nr(0), Z_PLAN);
	    break;
	case 'p':
	    sound_play(click_sound);
	    do_pause();
	    break;
	case 'r':
	    setze_maus_funktion(wkz_remover, skinverwaltung_t::killzeiger->gib_bild_nr(0), Z_PLAN, SFX_REMOVER, SFX_FAILURE);
	    break;
	case 's':
	    setze_maus_funktion(wkz_wegebau, skinverwaltung_t::strassenzeiger->gib_bild_nr(0), Z_PLAN,
		(long)wegbauer_t::strasse);
	    break;
	case 't':
	    setze_maus_funktion(wkz_wegebau, skinverwaltung_t::schienenzeiger->gib_bild_nr(0), Z_PLAN,
		(long)wegbauer_t::schiene);
	    break;
	case 'u':
	    setze_maus_funktion(wkz_raise, skinverwaltung_t::upzeiger->gib_bild_nr(0), Z_GRID);
	    break;
	case 'w':
	    create_win(272,160, -1,
		       new schedule_list_gui_t(this),
		       w_info,
		       magic_schedule_list_gui_t);
	    break;
	case '+':
	    display_set_light(display_get_light()+1);
	    break;
	case '-':
	    display_set_light(display_get_light()-1);
	    break;

	case 'C':
	    sound_play(click_sound);
	    setze_maus_funktion(wkz_add_city, skinverwaltung_t::stadtzeiger->gib_bild_nr(0), Z_GRID);
	    break;
	case 'Q':
	case 'X':
	    sound_play(click_sound);
	    destroy_all_win();
	    //beenden();        // 02-Nov-2001  Markus Weber    Function has a new parameter
            beenden(false);
	    break;
	case 'P':
	    if(wegbauer_t::leitung_besch) {
		setze_maus_funktion(wkz_pumpe, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN);
	    }
	    break;
	case 'L':
	    sound_play(click_sound);
	    create_win(new loadsave_frame_t(this, true), w_info, magic_load_t);
	    break;
#ifdef AUTOTEST
	case 'R':
	    setze_maus_funktion(tst_railtest, skinverwaltung_t::undoc_zeiger->gib_bild_nr(0), Z_PLAN);
	    break;
#endif
	case 'S':
	    sound_play(click_sound);
	    create_win(new loadsave_frame_t(this, false), w_info, magic_save_t);
	    break;

#ifdef AUTOTEST
	case 'T':
	    {
		worldtest_t tester (this, "test.log");
		tester.check_consistency();
	    }
	    break;
#endif
	case 'V':
	    sound_play(click_sound);
	    create_win(-1, -1, -1, new convoi_frame_t(gib_spieler(0), this), w_autodelete, magic_convoi_t);
	    break;
	case '9':
	    setze_ij_off(gib_ij_off()+koord::nord);
	    break;
	case '1':
	    setze_ij_off(gib_ij_off()+koord::sued);
	    break;
	case '7':
	    setze_ij_off(gib_ij_off()+koord::west);
	    break;
	case '3':
	    setze_ij_off(gib_ij_off()+koord::ost);
	    break;

	case '6':
	    setze_ij_off(gib_ij_off()+koord(+1,-1));
	    break;
	case '2':
	    setze_ij_off(gib_ij_off()+koord(+1,+1));
	    break;
	case '8':
	    setze_ij_off(gib_ij_off()+koord(-1,-1));
	    break;
	case '4':
	    setze_ij_off(gib_ij_off()+koord(-1,+1));
	    break;


	case '<':
	  set_zoom_factor(get_zoom_factor() > 1 ? get_zoom_factor()-1 : 1);
	  setze_dirty();
	  break;

	case '>':
	  set_zoom_factor(get_zoom_factor() < 4 ? get_zoom_factor()+1 : 4);
	  setze_dirty();
	  break;

	case 8:
	case 27:
	case 127:
	    sound_play(click_sound);
	    destroy_all_win();
	    break;
	case KEY_F1:
	    create_win(new help_frame_t("general.txt"), w_autodelete, magic_none);

	    break;
	case 0:
	    // just ignore the key
	    break;
	default:
	    dbg->message("karte_t::interactive_event()",
			 "key `%c` is not bound to a function.",ev.ev_code);

	    create_win(new help_frame_t("keys.txt"), w_autodelete, magic_none);
	}
    }

    if (IS_LEFTRELEASE(&ev)) {

	if(ev.my < 32) {
	    switch( ev.mx/32 ) {
	    case 0:
		sound_play(click_sound);
		create_win(240, 120, -1, new optionen_gui_t(this), w_info,
			   magic_optionen_gui_t);
		break;
	    case 1:
		sound_play(click_sound);
		create_win(-1, -1, -1, new map_frame_t(this), w_info, magic_reliefmap);
		break;
	    case 2:
		setze_maus_funktion(wkz_abfrage, skinverwaltung_t::fragezeiger->gib_bild_nr(0), Z_PLAN);
		break;
	    case 3:
		setze_maus_funktion(wkz_raise, skinverwaltung_t::upzeiger->gib_bild_nr(0), Z_GRID);
		break;
	    case 4:
		setze_maus_funktion(wkz_lower, skinverwaltung_t::downzeiger->gib_bild_nr(0), Z_GRID);
		break;
	    case 5:
                {
		    werkzeug_waehler_t *wzw =
                        new werkzeug_waehler_t(this, skinverwaltung_t::schienen_werkzeug, "RAILTOOLS");

		    wzw->setze_hilfe_datei("railtools.txt");

		    wzw->setze_werkzeug(0, wkz_wegebau, skinverwaltung_t::schienenzeiger->gib_bild_nr(0), Z_PLAN, (long)wegbauer_t::schiene, SFX_JACKHAMMER, SFX_FAILURE);
		    wzw->setze_werkzeug(1, wkz_signale, skinverwaltung_t::signalzeiger->gib_bild_nr(0), Z_LINES, SFX_GAVEL, SFX_FAILURE);
		    if(tunnelbauer_t::schienentunnel)
			wzw->setze_werkzeug(2, &tunnelbauer_t::baue, skinverwaltung_t::schienentunnelzeiger->gib_bild_nr(0), Z_PLAN, (long)weg_t::schiene, SFX_JACKHAMMER, SFX_FAILURE);
		    if(hausbauer_t::bahnhof_besch)
			wzw->setze_werkzeug(4, wkz_bahnhof, skinverwaltung_t::bahnhofzeiger->gib_bild_nr(0), Z_PLAN, SFX_GAVEL, SFX_FAILURE);
		    if(hausbauer_t::bahn_depot_besch)
			wzw->setze_werkzeug(5, wkz_bahndepot, skinverwaltung_t::bahndepotzeiger->gib_bild_nr(0), Z_PLAN, SFX_GAVEL, SFX_FAILURE);
		    wzw->setze_werkzeug(6, wkz_schienenkreuz, skinverwaltung_t::kreuzungzeiger->gib_bild_nr(0), Z_PLAN, SFX_JACKHAMMER, SFX_FAILURE);
		    wzw->setze_werkzeug(3, wkz_electrify_block, skinverwaltung_t::oberleitung->gib_bild_nr(8), Z_PLAN, SFX_JACKHAMMER, SFX_FAILURE);

		    sound_play(click_sound);

		    wzw->zeige_info(magic_railtools);
                }
		break;
	    case 6:
                {
		    werkzeug_waehler_t *wzw =
                        new werkzeug_waehler_t(this, skinverwaltung_t::strassen_werkzeug, "ROADTOOLS");

		    wzw->setze_hilfe_datei("roadtools.txt");

		    wzw->setze_werkzeug(0,
					wkz_wegebau,
					skinverwaltung_t::strassenzeiger->gib_bild_nr(0),
					Z_PLAN,
					(long)wegbauer_t::strasse,
					SFX_JACKHAMMER,
					SFX_FAILURE);

		    if(hausbauer_t::bushalt_besch)
			wzw->setze_werkzeug(1, wkz_bushalt, skinverwaltung_t::bushaltzeiger->gib_bild_nr(0), Z_PLAN, SFX_JACKHAMMER, SFX_FAILURE);
		    if(tunnelbauer_t::strassentunnel)
			wzw->setze_werkzeug(2, &tunnelbauer_t::baue, skinverwaltung_t::strassentunnelzeiger->gib_bild_nr(0), Z_PLAN, (long)weg_t::strasse, SFX_JACKHAMMER, SFX_FAILURE);
		    if(hausbauer_t::frachthof_besch)
			wzw->setze_werkzeug(4, wkz_frachthof, skinverwaltung_t::frachthofzeiger->gib_bild_nr(0), Z_PLAN, SFX_JACKHAMMER, SFX_FAILURE);
		    if(hausbauer_t::str_depot_besch)
			wzw->setze_werkzeug(5, wkz_strassendepot, skinverwaltung_t::strassendepotzeiger->gib_bild_nr(0), Z_PLAN, SFX_GAVEL, SFX_FAILURE);
		    wzw->setze_werkzeug(6, wkz_schienenkreuz, skinverwaltung_t::kreuzungzeiger->gib_bild_nr(0), Z_PLAN, SFX_JACKHAMMER, SFX_FAILURE);

		    sound_play(click_sound);

		    wzw->zeige_info(magic_roadtools);
                }
		break;
	    case 7:
                {
		    werkzeug_waehler_t *wzw =
                        new werkzeug_waehler_t(this, skinverwaltung_t::schiffs_werkzeug, "SHIPTOOLS");

		    wzw->setze_hilfe_datei("shiptools.txt");

		    if(hausbauer_t::dock_besch)
			wzw->setze_werkzeug(0, wkz_dockbau, skinverwaltung_t::anlegerzeiger->gib_bild_nr(0), Z_PLAN, SFX_DOCK, SFX_FAILURE);
		    if(hausbauer_t::sch_depot_besch) {
    			wzw->setze_werkzeug(4, wkz_schiffdepot_ns, skinverwaltung_t::werftNSzeiger->gib_bild_nr(0), Z_PLAN, SFX_DOCK, SFX_FAILURE);
			wzw->setze_werkzeug(5, wkz_schiffdepot_ow, skinverwaltung_t::werftOWzeiger->gib_bild_nr(0), Z_PLAN, SFX_DOCK, SFX_FAILURE);
		    }
		    sound_play(click_sound);

		    wzw->zeige_info(magic_shiptools);
                }
		break;
	    case 8:
		if(hausbauer_t::post_besch)
		    setze_maus_funktion(wkz_post, skinverwaltung_t::postzeiger->gib_bild_nr(0),  Z_PLAN);
		break;
	    case 9:
		setze_maus_funktion(wkz_remover, skinverwaltung_t::killzeiger->gib_bild_nr(0), Z_PLAN, SFX_REMOVER, SFX_FAILURE);
		break;
	    case 10:
		brueckenbauer_t::create_menu(this);
		break;
	    case 11:
		setze_maus_funktion(wkz_marker, skinverwaltung_t::belegtzeiger->gib_bild_nr(0), Z_PLAN);
		break;
	    case 12:
		break;
	    case 13:
		break;
	    case 14:
	      sound_play(click_sound);
	      setze_maus_funktion(wkz_add_city,
				  skinverwaltung_t::stadtzeiger->gib_bild_nr(0),
				  Z_GRID);
		break;
	    case 15:
		// Pause: warten auf die n�chste Taste
		do_pause();
		setze_dirty();
		break;
            case 16: // Station-List-Button    // 29-Dec-01    Markus Weber Added
		sound_play(click_sound);
		create_win(-1, -1, -1, new halt_list_frame_t(gib_spieler(0)), w_autodelete, magic_halt_list_t);
                break;
	    case 17:
		sound_play(click_sound);
		create_win(-1, -1, -1, new convoi_frame_t(gib_spieler(0), this), w_autodelete, magic_convoi_t);
		break;
	    case 18:
		sound_play(click_sound);
		create_win(-1, -1, -1, new money_frame_t(gib_spieler(0)), w_autodelete, magic_finances_t);
		break;
	    case 19:
		sound_play(click_sound);
                display_snapshot();
		create_win(-1, -1, 60, new nachrichtenfenster_t(this, "Screenshot\ngespeichert.\n"), w_autodelete);
		break;
	    }
	} else {
	    if(mouse_funk != NULL) {
		koord pos (i,j);

		dbg->message("karte_t::interactive_event(event_t &ev)",
			     "calling a tool");
		const int ok = mouse_funk(spieler[0], this, pos, mouse_funk_param);

		struct sound_info info;
		info.volume = 255;
		info.pri = 0;

		if(ok) {
		    if(mouse_funk_ok_sound) {
			info.index = mouse_funk_ok_sound;
			sound_play(info);
		    }
		} else {
		    if(mouse_funk_ko_sound) {
			info.index = mouse_funk_ko_sound;
			sound_play(info);
		    }
		}
	    }
	}
    }

    if(ev.ev_class == EVENT_SYSTEM) {
        // hellmade 15.05.2002
        // Beenden des Programms wenn das Fenster geschlossen wird.
        switch(ev.ev_code) {
	case SYSTEM_QUIT:
            sound_play(click_sound);
	    destroy_all_win();
	    beenden(true);
	    break;
	}
    }

    INT_CHECK("simworld 2117");
}

void
karte_t::interactive_update()
{
    event_t ev;
    bool swallowed = false;

    const long frame_time = get_frame_time();

    do {
	win_poll_event(&ev);

	if(ev.ev_class != EVENT_NONE &&
	   ev.ev_class != IGNORE_EVENT) {

	  if(IS_RIGHTCLICK(&ev)) {
	    display_show_pointer(false);
	  } else if(IS_RIGHTRELEASE(&ev)) {
	    display_show_pointer(true);
	  } else if(ev.ev_class == EVENT_DRAG && ev.ev_code == MOUSE_RIGHTBUTTON) {
	    blick_aendern(&ev);
	  }

	  swallowed = check_pos_win(&ev);

	  if(ev.button_state == 0 && ev.ev_class == EVENT_MOVE) {
	    bewege_zeiger(&ev);
	  }
	}

	INT_CHECK("simworld 2630");

    } while(ev.button_state != 0);


    set_frame_time(frame_time);

    if (!swallowed) {
	interactive_event(ev);
    }
}



//void
//karte_t::interactive()        //02-Nov-2001   Markus Weber    returns true, if simutrans should be unloaded

bool
karte_t::interactive()
{
    sleep_time = 4000;

    doit = true;

    long now = get_current_time_millis();

    long last_step_time = now;
    long this_step_time = 0;

    steps_bis_jetzt = steps;

    ticker_t::get_instance()->add_msg("Welcome to Simutrans, a game created by Hj. Malthaner.", BLAU);

    do {
	// check if we need to play a new midi file
	check_midi();

//	printf("karte_t::step()\n");

	INT_CHECK("simworld 2135");
	interactive_update();
       	simusleep(sleep_time);

	INT_CHECK("simworld 2139");
        interactive_update();
       	simusleep(sleep_time);

	INT_CHECK("simworld 2142");
        interactive_update();

	// welt steppen
	this_step_time = get_current_time_millis();
	step((long)(this_step_time-last_step_time));
	last_step_time = this_step_time;

	INT_CHECK("simworld 2151");
	interactive_update();
	simusleep(sleep_time);

	INT_CHECK("simworld 2155");
        interactive_update();

        for(int n=0; n<einstellungen->gib_anzahl_staedte(); n++) {
	  stadt->at(n)->step();

	  INT_CHECK("simworld 2396");
	  interactive_update();
	}

	INT_CHECK("simworld 2166");
	interactive_update();
	simusleep(sleep_time);

	INT_CHECK("simworld 2170");
	interactive_update();
	spieler[steps & 7]->step();

	INT_CHECK("simworld 2174");
	interactive_update();
	simusleep(sleep_time);

	const long t = get_current_time_millis();

	if(t > now+1000) {
	    last_simloops = steps - steps_bis_jetzt;

//          printf("%d simloops, %d sleep in der letzten sekunde.\n", dt, sleep_time);

	    const int soll = 5;

            if(last_simloops < soll) {

		// Hajo: emergency case, try to get it running,
		// reduce idle tile to 0
		if(last_simloops <= 1) {
		    sleep_time = 0;
		}

		if(sleep_time > 10) {
		    sleep_time -= sleep_time/4;
		}
		else {
		    increase_frame_time();
		}
	    }

            if(last_simloops > soll) {
		if(!reduce_frame_time()) {
		    if(sleep_time == 0) {
			sleep_time = 10;
		    } else {
			sleep_time += sleep_time/4;
		    }
		} else {
		    if(sleep_time > 10) {
			sleep_time -= sleep_time/4;
		    }
		}
	    }

	    now = t;
	    steps_bis_jetzt = steps;
	    lastFPS = thisFPS;
	    thisFPS = 0;
	}

    }while(doit);

    return m_quit_simutrans;      // 02-Nov-2001    Markus Weber    Added
}