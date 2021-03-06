/*
 * schedule_list.cc
 *
 * Copyright (c) 1997 - 2004 Hansj�rg Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdio.h>

#include "messagebox.h"
#include "schedule_list.h"
#include "line_management_gui.h"
#include "gui_convoiinfo.h"

#include "../simcolor.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simconvoi.h"
#include "../simvehikel.h"
#include "../simwin.h"
#include "../simlinemgmt.h"
#include "../utils/simstring.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "components/list_button.h"
#include "halt_list_stats.h"


const char schedule_list_gui_t::cost_type[MAX_LINE_COST][64] =
{
	"Free Capacity",
	"Transported",
	"Revenue",
	"Operation",
	"Profit",
	"Convoys"
};

static const uint8 tabs_to_lineindex[7]={ simline_t::line, simline_t::trainline, simline_t::monorailline, simline_t::tramline, simline_t::truckline, simline_t::shipline, simline_t::airline };

const int schedule_list_gui_t::cost_type_color[MAX_LINE_COST] =
{
  COL_FREE_CAPACITY, COL_TRANSPORTED, COL_REVENUE, COL_OPERATION, COL_PROFIT, COL_VEHICLE_ASSETS
};

uint8 schedule_list_gui_t::statistic[MAX_LINE_COST]={
	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_CONVOIS
};

uint8 schedule_list_gui_t::statistic_type[MAX_LINE_COST]={
	STANDARD, STANDARD, MONEY, MONEY, MONEY, STANDARD
};

#define LINE_NAME_COLUMN_WIDTH ((BUTTON_WIDTH*3)+11+11)
#define SCL_HEIGHT (170)


// Hajo: 17-Jan-04: changed layout to make components fit into
// a width of 400 pixels -> original size was unuseable in 640x480

schedule_list_gui_t::schedule_list_gui_t(karte_t *welt,spieler_t *sp)
 : gui_frame_t("Line Management",sp),
 scrolly(&cont),
 scrolly_haltestellen(&cont_haltestellen),
 scl(gui_scrolled_list_t::select)
{
	this->welt = welt;
	this->sp = sp;
	capacity = load = 0;
	selection = -1;
	loadfactor = 0;

	button_t button_def;

	// init scrolled list
	scl.setze_groesse(koord(LINE_NAME_COLUMN_WIDTH-22, SCL_HEIGHT-14));
	scl.setze_pos(koord(0,1));
	scl.setze_highlight_color(sp->get_player_color1()+1);
	scl.request_groesse(scl.gib_groesse());
	scl.add_listener(this);

	// tab panel
	tabs.setze_pos(koord(11,5));
	tabs.setze_groesse(koord(LINE_NAME_COLUMN_WIDTH-22, SCL_HEIGHT));
	tabs.add_tab(&scl, translator::translate("All"));
	tabs.add_tab(&scl, translator::translate("Train"));
	tabs.add_tab(&scl, translator::translate("Monorail"));
	tabs.add_tab(&scl, translator::translate("Tram"));
	tabs.add_tab(&scl, translator::translate("Truck"));
	tabs.add_tab(&scl, translator::translate("Ship"));
	tabs.add_tab(&scl, translator::translate("Air"));
	tabs.add_listener(this);
	add_komponente(&tabs);

	// editable line name
	inp_name.add_listener(this);
	inp_name.setze_pos(koord(LINE_NAME_COLUMN_WIDTH, 14 + SCL_HEIGHT));
	inp_name.set_visible(false);
	add_komponente(&inp_name);

	// load display
	filled_bar.add_color_value(&loadfactor, COL_GREEN);
	filled_bar.setze_pos(koord(LINE_NAME_COLUMN_WIDTH, 6 + SCL_HEIGHT));
	filled_bar.set_visible(false);
	add_komponente(&filled_bar);

	// vonvois?
	cont.setze_groesse(koord(LINE_NAME_COLUMN_WIDTH, 40));

	// convoi list?
	scrolly.setze_pos(koord(LINE_NAME_COLUMN_WIDTH-11, 14 + SCL_HEIGHT+14+4+2*LINESPACE+2));
	scrolly.set_visible(false);
	add_komponente(&scrolly);
	setze_opaque(true);

	// halt list?
	cont_haltestellen.setze_groesse(koord(500, 40));
	scrolly_haltestellen.setze_pos(koord(0, 14 + SCL_HEIGHT+BUTTON_HEIGHT+2));
	scrolly_haltestellen.set_visible(false);
	add_komponente(&scrolly_haltestellen);
	setze_opaque(true);

	// normal buttons edit new remove
	bt_new_line.setze_pos(koord(11, 14 + SCL_HEIGHT));
	bt_new_line.setze_groesse(koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	bt_new_line.setze_typ(button_t::roundbox);
	bt_new_line.setze_text("New Line");
	add_komponente(&bt_new_line);
	bt_new_line.add_listener(this);

	bt_change_line.setze_pos(koord(11+BUTTON_WIDTH, 14 + SCL_HEIGHT));
	bt_change_line.setze_groesse(koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	bt_change_line.setze_typ(button_t::roundbox);
	bt_change_line.setze_text("Update Line");
	bt_change_line.set_tooltip("Modify the selected line");
	add_komponente(&bt_change_line);
	bt_change_line.add_listener(this);
	bt_change_line.disable();

	bt_delete_line.setze_pos(koord(11+2*BUTTON_WIDTH, 14 + SCL_HEIGHT));
	bt_delete_line.setze_groesse(koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	bt_delete_line.setze_typ(button_t::roundbox);
	bt_delete_line.setze_text("Delete Line");
	add_komponente(&bt_delete_line);
	bt_delete_line.add_listener(this);
	bt_delete_line.disable();

	//CHART
	chart.set_dimension(12, 1000);
	chart.setze_pos( koord(LINE_NAME_COLUMN_WIDTH+50,11) );
	chart.set_seed(0);
	chart.set_background(MN_GREY1);

	add_komponente(&chart);
	//CHART END

	// add filter buttons
	for (int i=0; i<MAX_LINE_COST; i++) {
		filterButtons[i].init(button_t::box_state,cost_type[i],koord(0,0), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
		filterButtons[i].add_listener(this);
		filterButtons[i].background = cost_type_color[i];
		add_komponente(filterButtons + i);
	}

	update_lineinfo( linehandle_t() );
	build_line_list(0);

	// resize button
	set_min_windowsize(koord(488, 400));
	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



bool schedule_list_gui_t::action_triggered(gui_komponente_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if (komp == &bt_change_line) {
		if (line.is_bound()) {
			create_win(-1, -1, new line_management_gui_t(welt, line, sp), w_info);
		}
	}
	else if (komp == &bt_new_line) {
		if (tabs.get_active_tab_index() > 0) {
			// create typed line
			uint8 type=tabs_to_lineindex[tabs.get_active_tab_index()];
			linehandle_t new_line = sp->simlinemgmt.create_line(type);
			create_win(-1, -1, new line_management_gui_t(welt, new_line, sp), w_info);
			update_lineinfo( new_line );
			build_line_list( tabs.get_active_tab_index() );
		}
		else {
			create_win(-1, -1, 120, new nachrichtenfenster_t(welt, translator::translate("Cannot create generic line!\nSelect line type by\nusing filter tabs.")), w_autodelete);
		}
	}
	else if (komp == &bt_delete_line) {
		if (line.is_bound()) {
			// remove elements from lists
			linehandle_t delete_line=line;
			update_lineinfo( linehandle_t() );
			sp->simlinemgmt.delete_line(delete_line);
			build_line_list(tabs.get_active_tab_index());
		}
	}
	else if (komp == &tabs) {
		update_lineinfo( linehandle_t() );
		build_line_list(tabs.get_active_tab_index());
	}
	else if (komp == &scl) {
		if (!line.is_bound()) {
			bt_change_line.disable();
			bt_delete_line.disable();
		}
		// get selected line
		linehandle_t new_line = linehandle_t();
		selection = scl.gib_selection();
		if ((selection >= 0) && (selection < (int)lines.count())) {
			new_line = lines.at(selection);
			bt_change_line.enable();
			bt_delete_line.enable();
		}
		update_lineinfo(new_line);
	}
	else {
		if (line.is_bound()) {
			for ( int i = 0; i<MAX_LINE_COST; i++) {
				if (komp == &filterButtons[i]) {
					filterButtons[i].pressed ^= 1;
					if(filterButtons[i].pressed) {
						chart.show_curve(i);
					}
					else {
						chart.hide_curve(i);
					}
					break;
				}
			}
		}
	}

	return true;
}



void schedule_list_gui_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen(pos, gr);
	if (line.is_bound()) {
		display(pos);
	}
}



void
schedule_list_gui_t::display(koord pos)
{
	int icnv = line->count_convoys();

	char buffer[8192];
	char ctmp[128];

	capacity = load = loadfactor = 0; // total capacity and load of line (=sum of all conv's cap/load)

	sint64 profit = line->get_finance_history(0,LINE_PROFIT);

	for (int i = 0; i<icnv; i++) {
		convoihandle_t cnv = line->get_convoy(i)->self;
		// we do not want to count the capacity of depot convois
		if (!cnv->in_depot()) {
			for (unsigned j = 0; j<cnv->gib_vehikel_anzahl(); j++) {
				capacity += cnv->gib_vehikel(j)->gib_fracht_max();
				load += cnv->gib_vehikel(j)->gib_fracht_menge();
			}
		}
	}

	// we check if cap is zero, since theoretically a
	// conv can consist of only 1 vehicle, which has no cap (eg. locomotive)
	// and we do not like to divide by zero, do we?
	if (capacity > 0) {
		loadfactor = (load * 100) / capacity;
	}

	switch(icnv) {
		case 0: strcpy(buffer, translator::translate("no convois") );
			break;
		case 1: strcpy(buffer, translator::translate("1 convoi") );
			break;
		default: sprintf(buffer, translator::translate("%d convois"), icnv);
			break;
	}
	int len=display_proportional(pos.x+LINE_NAME_COLUMN_WIDTH-5, pos.y+16+14+SCL_HEIGHT+14+4, buffer, ALIGN_LEFT, COL_BLACK, true );

	int len2 = display_proportional(pos.x+LINE_NAME_COLUMN_WIDTH-5, pos.y+16+14+SCL_HEIGHT+14+4+LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, COL_BLACK, true );
	money_to_string(ctmp, profit/100.0);
	len2 += display_proportional(pos.x+LINE_NAME_COLUMN_WIDTH+len2, pos.y+16+14+SCL_HEIGHT+14+4+LINESPACE, ctmp, ALIGN_LEFT, profit>=0?MONEY_PLUS:MONEY_MINUS, true );

	int rest_width = max( (gib_fenstergroesse().x-LINE_NAME_COLUMN_WIDTH)/2, max(len2,len) );
	number_to_string(ctmp, capacity);
	sprintf(buffer, translator::translate("Capacity: %s\nLoad: %d (%d%%)"), ctmp, load, loadfactor);
	display_multiline_text(pos.x + LINE_NAME_COLUMN_WIDTH + rest_width, pos.y+16 + 14 + SCL_HEIGHT + 14 +4 , buffer, COL_BLACK);
}



void schedule_list_gui_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	int rest_width = gib_fenstergroesse().x-LINE_NAME_COLUMN_WIDTH;
	int button_per_row=max(1,rest_width/(BUTTON_WIDTH+BUTTON_SPACER));
	int button_rows= MAX_LINE_COST/button_per_row + ((MAX_LINE_COST%button_per_row)!=0);

	scrolly.setze_groesse( koord(rest_width+11, get_client_windowsize().y-scrolly.gib_pos().y) );
	scrolly_haltestellen.setze_groesse( koord(LINE_NAME_COLUMN_WIDTH-11, get_client_windowsize().y-scrolly_haltestellen.gib_pos().y) );

	chart.setze_groesse(koord(rest_width-11-50, SCL_HEIGHT-11-14-(button_rows*(BUTTON_HEIGHT+BUTTON_SPACER))));
	inp_name.setze_groesse(koord(rest_width-11, 14));
	filled_bar.setze_groesse(koord(rest_width-11, 4));

	int y=SCL_HEIGHT-11-(button_rows*(BUTTON_HEIGHT+BUTTON_SPACER))+14;
	for (int i=0; i<MAX_LINE_COST; i++) {
		filterButtons[i].setze_pos( koord(LINE_NAME_COLUMN_WIDTH+(i%button_per_row)*(BUTTON_WIDTH+BUTTON_SPACER),y+(i/button_per_row)*(BUTTON_HEIGHT+BUTTON_SPACER))  );
	}
}



void schedule_list_gui_t::build_line_list(int filter)
{
	sp->simlinemgmt.sort_lines();	// to take care of renaming ...
DBG_MESSAGE("schedule_list_gui_t::build_line_list()","count=%i",sp->simlinemgmt.count_lines());
	if(sp->simlinemgmt.count_lines() > 0) {
		scl.clear_elements();
		lines.clear();
		sp->simlinemgmt.build_line_list( tabs_to_lineindex[filter], &lines );
		slist_iterator_tpl<linehandle_t> iter(lines);
		while( iter.next() ) {
			scl.append_element( iter.get_current()->get_name(), iter.get_current()->get_state_color() );
			if(line==iter.get_current()) {
				scl.setze_selection( scl.get_count()-1 );
			}
		}
	}
}




/* hides show components */
void schedule_list_gui_t::update_lineinfo(linehandle_t new_line)
{
	if(new_line.is_bound()) {
		// ok, this line is visible
		scrolly.set_visible(true);
		scrolly_haltestellen.set_visible(true);
		inp_name.setze_text(new_line->get_name(), 128);
		inp_name.set_visible(true);
		filled_bar.set_visible(true);

		// fill container with info of line's convoys
		// we do it here, since this list needs only to be
		// refreshed when the user selects a new line
		int i, icnv = 0;
		icnv = new_line->count_convoys();
		// display convoys of line
		cont.remove_all();
		int ypos = 5;
		for(i = 0;  i<icnv;  i++  ) {
			gui_convoiinfo_t *cinfo = new gui_convoiinfo_t(new_line->get_convoy(i)->self, i + 1);
			cinfo->setze_pos(koord(0, ypos));
			cinfo->setze_groesse(koord(400, 40));
			cont.add_komponente(cinfo);
			ypos += 40;
		}
		cont.setze_groesse(koord(500, ypos));

		if(icnv>0) {
			bt_delete_line.disable();
		}
		else {
			bt_delete_line.enable();
		}

		// fill haltestellen container with info of line's haltestellen
		cont_haltestellen.remove_all();
		ypos = 5;
		slist_tpl<koord> tmp; // stores koords of stops that are allready displayed
		for(i=0; i<new_line->get_fahrplan()->maxi(); i++) {
			const koord fahrplan_koord = new_line->get_fahrplan()->eintrag[i].pos.gib_2d();
			halthandle_t halt = haltestelle_t::gib_halt(welt, fahrplan_koord);
			if (halt.is_bound()) {
				// only add a haltestelle to the list, if it is not in the list allready
				if (!tmp.contains(fahrplan_koord)) {
					halt_list_stats_t *cinfo = new halt_list_stats_t(halt, i + 1);
					cinfo->setze_pos(koord(0, ypos));
					cinfo->setze_groesse(koord(500, 40));
					cont_haltestellen.add_komponente(cinfo);
					ypos += 40;
					tmp.append(fahrplan_koord);
				}
			}
		}
		cont_haltestellen.setze_groesse(koord(500, ypos));

		// chart
		chart.remove_curves();
		for(i=0; i<MAX_LINE_COST; i++)  {
			chart.add_curve(cost_type_color[i], new_line->get_finance_history(), MAX_LINE_COST, statistic[i], MAX_MONTHS, statistic_type[i], filterButtons[i].pressed, true);
			if(filterButtons[i].pressed) {
				chart.show_curve(i);
			}
		}
		chart.set_visible(true);
	}
	else if(line.is_bound()) {
		// previously a line was visible
		// thus the need to hide everything
		cont.remove_all();
		inp_name.set_visible(false);
		filled_bar.set_visible(false);
		scrolly.set_visible(false);
		scrolly_haltestellen.set_visible(false);
		inp_name.set_visible(false);
		filled_bar.set_visible(false);
		scl.setze_selection(-1);
		bt_delete_line.disable();
		bt_change_line.disable();
		for(int i=0; i<MAX_LINE_COST; i++)  {
			chart.hide_curve(i);
		}
		chart.set_visible(true);
	}
	line = new_line;
}
