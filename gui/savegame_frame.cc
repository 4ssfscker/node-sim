/*
 * savegame_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef _MSC_VER
#include <unistd.h>
#include <dirent.h>
#else
#include <io.h>
#include <direct.h>
#endif
#include <sys/stat.h>
#include <string.h>
#include <time.h>

#include "../pathes.h"

#include "../simdebug.h"
#include "../simwin.h"
#include "../simintr.h"
#include "../utils/simstring.h"

#include "components/list_button.h"
#include "savegame_frame.h"

#define DIALOG_WIDTH (360)


savegame_frame_t::savegame_frame_t(const char *suffix) :
	gui_frame_t("Load/Save") ,
	fnlabel("Filename"),
	button_frame(),
	scrolly(&button_frame)
{
	this->suffix = suffix;
	use_pak_extension = strcmp( suffix, ".sve" )==0;

#ifdef WIN32
	//CreateDirectoryA(SAVE_PATH);
	mkdir(SAVE_PATH);
#else
	mkdir(SAVE_PATH, 0700);
#endif

#ifndef _MSC_VER
	// find filenames
	DIR     *dir;                      /* Schnittstellen zum BS */
	struct  dirent  *entry;

	dir=opendir(SAVE_PATH_X ".");

	if(dir==NULL) {
		dbg->warning("savegame_frame_t::savegame_frame_t()","Couldn't read directory.");
	} else {
		do {
			entry=readdir(dir);
			if(entry!=NULL) {
				if(entry->d_name[0]!='.' ||  (entry->d_name[1]!='.' && entry->d_name[1]!=0)) {
					if(strncmp(entry->d_name + strlen(entry->d_name) - 4, suffix, 4) == 0) {
						if(!use_pak_extension) {
							add_file(entry->d_name, "");
						}
						else {
							// this is a savegame:
							// read also the pak extension
							loadsave_t test;
							char p[1024];
							sprintf( p, "%s%s", SAVE_PATH_X, entry->d_name );
							test.rd_open(p);
							add_file( entry->d_name, test.get_pak_extension() );
						}
					}
				}
			}
		}while(entry!=NULL);
		closedir(dir);
	}
#else
	struct _finddata_t entry;
	long hfind;

	char wild[32];
	tstrncpy(wild, SAVE_PATH_X "*", 32);
	strcat(wild, suffix);

	hfind = _findfirst( wild, &entry);
	if(hfind == -1) {
		dbg->warning("savegame_frame_t::savegame_frame_t()","Couldn't read directory.");
	} else {
		do{
			if(!use_pak_extension) {
				add_file(entry.name, "");
			}
			else {
				// this is a savegame:
				// read also the pak extension
				loadsave_t test;
				char p[1024];
				sprintf( p, "%s%s", SAVE_PATH_X, entry.name );
				test.rd_open(p);
				add_file( entry.name, test.get_pak_extension() );
			}
		} while(_findnext(hfind, &entry) == 0 );
	}
#endif
	// Text 'Game name'
	fnlabel.setze_pos (koord(10,12));
	fnlabel.setze_text ("Filename");
	add_komponente(&fnlabel);

	// Input box for game name
	tstrncpy(ibuf, "", 1);
	input.setze_text(ibuf, 58);
	input.add_listener(this);
	input.setze_pos(koord(75,8));
	input.setze_groesse(koord(DIALOG_WIDTH-75-10-10, 14));
	add_komponente(&input);

	// needs to be scrollable
	scrolly.setze_pos( koord(0,30) );
	scrolly.set_show_scroll_x(false);
	scrolly.set_size_corner(false);
	scrolly.setze_groesse( koord(DIALOG_WIDTH-12,30) );

	// The file entries
	slist_iterator_tpl <button_t *> iter1(deletes);
	slist_iterator_tpl <button_t *> iter2(buttons);
	slist_iterator_tpl <gui_label_t *> iter3(labels);
	int y = 0;

	while(iter1.next() && iter2.next() && iter3.next()) {
		button_t * button1 = iter1.get_current();
		button_t * button2 = iter2.get_current();
		gui_label_t * label = iter3.get_current();

		button1->setze_groesse(koord(14, 14));
		button1->setze_text("X");
		button1->setze_pos(koord(5, y));
		button1->set_tooltip("Delete this file.");

		button2->setze_pos(koord(25, y));
		button2->setze_groesse(koord(140, 14));

		label->setze_pos(koord(170, y+3));

		button1->add_listener(this);
		button2->add_listener(this);

		button_frame.add_komponente(button1);
		button_frame.add_komponente(button2);
		button_frame.add_komponente(label);

		y += 14;
	}
	button_frame.setze_groesse( koord( DIALOG_WIDTH-1, y ) );
	add_komponente(&scrolly);

	y += 10+30;
	divider1.setze_pos(koord(10,y));
	divider1.setze_groesse(koord(DIALOG_WIDTH-20,0));
	add_komponente(&divider1);

	y += 10;
	savebutton.setze_pos(koord(10,y));
	savebutton.setze_groesse(koord(BUTTON_WIDTH, 14));
	savebutton.setze_text("Ok");
	savebutton.setze_typ(button_t::roundbox);
	savebutton.add_listener(this);
	add_komponente(&savebutton);

	cancelbutton.setze_pos(koord(DIALOG_WIDTH-BUTTON_WIDTH-10,y));
	cancelbutton.setze_groesse(koord(BUTTON_WIDTH, 14));
	cancelbutton.setze_text("Cancel");
	cancelbutton.setze_typ(button_t::roundbox);
	cancelbutton.add_listener(this);
	add_komponente(&cancelbutton);

	setze_opaque(true);
	setze_fenstergroesse(koord(DIALOG_WIDTH, y + 40));
}



savegame_frame_t::~savegame_frame_t()
{
	slist_iterator_tpl <button_t *> b_iter (buttons);
	while(b_iter.next()) {
		delete [] const_cast<char *>(b_iter.get_current()->gib_text());
		delete b_iter.get_current();
	}

	slist_iterator_tpl <gui_label_t *> l_iter (labels);
	while(l_iter.next()) {
		delete [] const_cast<char *>(l_iter.get_current()->get_text_pointer());
		delete l_iter.get_current();
	}

	slist_iterator_tpl <button_t *> s_iter (deletes);
	while(s_iter.next()) {
		delete s_iter.get_current();
	}
}



// sets the current filename in the input box
void
savegame_frame_t::set_filename(const char *fn)
{
	long len=strlen(fn);
	if(len>=4  &&  len-SAVE_PATH_X_LEN-3<58) {
		if(strncmp(fn,SAVE_PATH_X,SAVE_PATH_X_LEN)==0) {
			tstrncpy(input.gib_text(), fn+SAVE_PATH_X_LEN, len-SAVE_PATH_X_LEN-3 );
		}
		else {
			tstrncpy(input.gib_text(), fn, len-3 );
		}
	}
}



void
savegame_frame_t::add_file(const char *filename, const char *pak)
{
	button_t * button = new button_t();
	char * name = new char [strlen(filename)+10];
	char * date = new char[18+strlen(pak)+3];

	//DBG_MESSAGE("","sizeof(stat)=%d, sizeof(tm)=%d",sizeof(struct stat),sizeof(struct tm) );
	sprintf(name, SAVE_PATH_X "%s", filename);

	struct stat  sb;
	stat(name, &sb);

	// add pak extension
	size_t n = *pak!=0 ? sprintf( date, "%s - ", pak ) : 0;

	// add the time too
	struct tm *tm = localtime(&sb.st_mtime);
	if(tm) {
		strftime(date+n, 18, "%Y-%m-%d %H:%M", tm);
	}
	else {
		tstrncpy(date, "??.??.???? ??:??", 15);
	}

	sprintf(name, "%s", filename);
	name[strlen(name)-4] = '\0';
	button->setze_text(name);	// to avoid translation
	button->set_no_translate(true);

	// sort by date descending:
	unsigned int i;
	for(i = 0; i < buttons.count(); i++) {
		if(strcmp(labels.at(i)->get_text_pointer(), date) < 0) {
			break;
		}
	}
	buttons.insert(button, i);
	labels.insert(new gui_label_t(NULL), i);
	labels.at(i)->set_text_pointer(date);
	deletes.insert(new button_t, i);
}



/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool savegame_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	char buf[128];	// ok, since maximum length is 58

	if(komp == &input || komp == &savebutton) {
		// Save/Load Button or Enter-Key pressed
		//---------------------------------------

		tstrncpy(buf, SAVE_PATH_X, 128);
		strcat(buf, ibuf);
		strcat(buf, suffix);

		destroy_win(this);      //29-Oct-2001         Markus Weber    Close window

		action(buf);
	}
	else if(komp == &cancelbutton) {
		// Cancel-button pressed
		//-----------------------------
		destroy_win(this);      //29-Oct-2001         Markus Weber    Added   savebutton case

	}
	else {
		// File in list selected
		//--------------------------

		slist_iterator_tpl <button_t *> iter (buttons);
		slist_iterator_tpl <button_t *> iter2 (deletes);

		while(iter.next()) {
			iter2.next();
			if(komp == iter.get_current() || komp == iter2.get_current()) {
				destroy_win(this);
				intr_refresh_display( true );

				tstrncpy(buf, SAVE_PATH_X, 128);
				strcat(buf, iter.get_current()->gib_text());
				strcat(buf, suffix);

				if(komp == iter.get_current()) {
					action(buf);
				}
				else {
					del_action(buf);
				}
				break;
			}
		}
	}
	return true;
}




/**
 * Bei Scrollpanes _muss_ diese Methode zum setzen der Groesse
 * benutzt werden.
 * @author Hj. Malthaner
 */
void savegame_frame_t::setze_fenstergroesse(koord groesse)
{
	if(groesse.y>display_get_height()-64) {
		// too large ...
		groesse.y = display_get_height()-64;
		// position adjustment will be done automatically ... nice!
	}
	scrolly.setze_groesse( koord(groesse.x,groesse.y-30-40-8) );
	gui_frame_t::setze_fenstergroesse(groesse);
	divider1.setze_pos(koord(10,groesse.y-44));
	savebutton.setze_pos(koord(10,groesse.y-34));
	cancelbutton.setze_pos(koord(DIALOG_WIDTH-BUTTON_WIDTH-10,groesse.y-34));
}
