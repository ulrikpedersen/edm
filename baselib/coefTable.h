//  edm - extensible display manager

//  Copyright (C) 1999 John W. Sinclair

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.

//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#ifndef __coefTable_h
#define __coefTable_h 1

#include "act_grf.h"
#include "entry_form.h"
#include "tableObj.h"

#include "pv_factory.h"

#define COEFTABLEC_MAJOR_VERSION 4
#define COEFTABLEC_MINOR_VERSION 0
#define COEFTABLEC_RELEASE 0

#ifdef __coefTable_cc

#include "coefTable.str"

static void coefTablec_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

static void coefTablec_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

static void coefTablec_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

static void coefTablec_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

static void coefTablec_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

static void coefTable_readUpdate (
  ProcessVariable *pv,
  void *userarg );

static void coefTable_monitor_read_connect_state (
  ProcessVariable *pv,
  void *userarg );

#endif

class activeCoefTableClass : public activeGraphicClass {

private:

friend void coefTablec_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void coefTablec_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void coefTablec_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void coefTablec_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void coefTablec_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call );

friend void coefTable_readUpdate (
  ProcessVariable *pv,
  void *userarg );

friend void coefTable_monitor_read_connect_state (
  ProcessVariable *pv,
  void *userarg );

static const int MaxLabelSize=1023;

typedef struct editBufTag {
// edit buffer
  int bufX;
  int bufY;
  int bufW;
  int bufH;
  int bufFgColor;
  int bufBgColor;
  int bufOddBgColor;
  int bufEvenBgColor;
  int bufTopShadowColor;
  int bufBotShadowColor;
  colorButtonClass fgCb;
  colorButtonClass bgCb;
  colorButtonClass oddBgCb;
  colorButtonClass evenBgCb;
  colorButtonClass topCb;
  colorButtonClass botCb;
  char bufReadPvName[PV_Factory::MAX_PV_NAME+1];
  char bufLabels[MaxLabelSize+1];
  efInt bufEfFirstEle;
  efInt bufEfNumEle;
} editBufType, *editBufPtr;

editBufPtr eBuf;

int opComplete;

Widget frameWidget;

tableClass table;

int arraySize;
int bufInvalid;

fontMenuClass fm;
char fontTag[63+1];
XFontStruct *fs;

// Font metrics
int fontAscent;
int fontDescent;
int fontHeight;
int stringLength;
int stringWidth;

ProcessVariable *readPvId;
int initialReadConnection;

expStringClass readPvExpStr;
expStringClass labelsExpStr;

efInt efFirstEle, efNumEle;
int firstEle, numEle;

int readExists;

int readPvConnected, firstReadUpdate, init, active, activeMode;

pvColorClass fgColor, bgColor, oddBgColor, evenBgColor, topShadowColor, botShadowColor;

int needConnectInit, needUpdate, needDraw;

public:

activeCoefTableClass ( void );

activeCoefTableClass
 ( const activeCoefTableClass *source );

~activeCoefTableClass ( void );

char *objName ( void ) {

  return name;

}

int createInteractive (
  activeWindowClass *aw_obj,
  int x,
  int y,
  int w,
  int h );

int save (
  FILE *f );

int createFromFile (
  FILE *fptr,
  char *name,
  activeWindowClass *actWin );

int genericEdit ( void );

int edit ( void );

int editCreate ( void );

int draw ( void );

int erase ( void );

int drawActive ( void );

int eraseActive ( void );

int activate ( int pass, void *ptr );

int deactivate ( int pass );

int expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] );

int expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] );

int containsMacros ( void );

int createTableWidgets ( void );

int checkResizeSelectBox (
  int _x,
  int _y,
  int _w,
  int _h );

int checkResizeSelectBoxAbs (
  int _x,
  int _y,
  int _w,
  int _h );

void executeDeferred ( void );

void changeDisplayParams (
  unsigned int flag,
  char *fontTag,
  int alignment,
  char *ctlFontTag,
  int ctlAlignment,
  char *btnFontTag,
  int btnAlignment,
  int textFgColor,
  int fg1Color,
  int fg2Color,
  int offsetColor,
  int bgColor,
  int topShadowColor,
  int botShadowColor );

void changePvNames (
  int flag,
  int numCtlPvs,
  char *ctlPvs[],
  int numReadbackPvs,
  char *readbackPvs[],
  int numNullPvs,
  char *nullPvs[],
  int numVisPvs,
  char *visPvs[],
  int numAlarmPvs,
  char *alarmPvs[] );

void map ( void );

void unmap ( void );

void getPvs (
  int max,
  ProcessVariable *pvs[],
  int *n );

char *crawlerGetFirstPv ( void );

char *crawlerGetNextPv ( void );

};

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeCoefTableClassPtr ( void );
void *clone_activeCoefTableClassPtr ( void * );

#ifdef __cplusplus
}
#endif

#endif
