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

#define __button_cc 1

#include "button.h"
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

static void btc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeButtonClass *bto = (activeButtonClass *) client;

  bto->actWin->setChanged();

  bto->eraseSelectBoxCorners();
  bto->erase();

  bto->fgColorMode = bto->bufFgColorMode;
  if ( bto->fgColorMode == BTC_K_COLORMODE_ALARM )
    bto->fgColor.setAlarmSensitive();
  else
    bto->fgColor.setAlarmInsensitive();
  bto->fgColor.setColorIndex( bto->bufFgColor, bto->actWin->ci );

  bto->onColor.setColorIndex( bto->bufOnColor, bto->actWin->ci );

  bto->offColor.setColorIndex( bto->bufOffColor, bto->actWin->ci );

  bto->inconsistentColor.setColorIndex( bto->bufInconsistentColor,
   bto->actWin->ci );

  bto->topShadowColor = bto->bufTopShadowColor;
  bto->botShadowColor = bto->bufBotShadowColor;

  
  bto->controlPvName.setRaw( bto->controlBufPvName );
  bto->readPvName.setRaw( bto->readBufPvName );

  strncpy( bto->onLabel, bto->bufOnLabel, MAX_ENUM_STRING_SIZE );
  strncpy( bto->offLabel, bto->bufOffLabel, MAX_ENUM_STRING_SIZE );

  if ( strcmp( bto->labelTypeString, activeButtonClass_str3 ) == 0 )
    bto->labelType = BTC_K_PV_STATE;
  else
    bto->labelType = BTC_K_LITERAL;

  strncpy( bto->fontTag, bto->fm.currentFontTag(), 63 );
  bto->actWin->fi->loadFontTag( bto->fontTag );
  bto->fs = bto->actWin->fi->getXFontStruct( bto->fontTag );

  if ( strcmp( bto->buttonTypeStr, activeButtonClass_str4 ) == 0 ) {
    bto->toggle = 0;
    bto->buttonType = BTC_K_PUSH;
  }
  else {
    bto->toggle = 1;
    bto->buttonType = BTC_K_TOGGLE;
  }

  if ( strcmp( bto->_3DString, activeButtonClass_str5 ) == 0 )
    bto->_3D = 1;
  else
    bto->_3D = 0;

  if ( strcmp( bto->invisibleString, activeButtonClass_str6 ) == 0 )
    bto->invisible = 1;
  else
    bto->invisible = 0;

  strncpy( bto->id, bto->bufId, 31 );
  bto->downCallbackFlag = bto->bufDownCallbackFlag;
  bto->upCallbackFlag = bto->bufUpCallbackFlag;
  bto->activateCallbackFlag = bto->bufActivateCallbackFlag;
  bto->deactivateCallbackFlag = bto->bufDeactivateCallbackFlag;
  bto->anyCallbackFlag = bto->downCallbackFlag || bto->upCallbackFlag ||
   bto->activateCallbackFlag || bto->deactivateCallbackFlag;

  bto->x = bto->bufX;
  bto->sboxX = bto->bufX;

  bto->y = bto->bufY;
  bto->sboxY = bto->bufY;

  bto->w = bto->bufW;
  bto->sboxW = bto->bufW;

  bto->h = bto->bufH;
  bto->sboxH = bto->bufH;

  bto->updateDimensions();

}

static void btc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeButtonClass *bto = (activeButtonClass *) client;

  btc_edit_update ( w, client, call );
  bto->refresh( bto );

}

static void btc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeButtonClass *bto = (activeButtonClass *) client;

  btc_edit_update ( w, client, call );
  bto->ef.popdown();
  bto->operationComplete();

}

static void btc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeButtonClass *bto = (activeButtonClass *) client;

  bto->ef.popdown();
  bto->operationCancel();

}

static void btc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeButtonClass *bto = (activeButtonClass *) client;

  bto->ef.popdown();
  bto->operationCancel();
  bto->erase();
  bto->deleteRequest = 1;
  bto->drawAll();

}

#ifdef __epics__

static void bt_monitor_control_connect_state (
  struct connection_handler_args arg )
{

activeButtonClass *bto = (activeButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    bto->needCtlConnectInit = 1;

  }
  else {

    bto->controlValid = 0;
    bto->controlPvConnected = 0;
    bto->active = 0;
    bto->onColor.setDisconnected();
    bto->offColor.setDisconnected();
    bto->inconsistentColor.setDisconnected();
    bto->needDraw = 1;

  }

  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

static void bt_controlInfoUpdate (
  struct event_handler_args ast_args )
{

activeButtonClass *bto = (activeButtonClass *) ast_args.usr;
struct dbr_gr_enum controlRec = *( (dbr_gr_enum *) ast_args.dbr );

  bto->curControlV = controlRec.value;

  if ( !(bto->readExists) ) {

//      bto->active = 1;
//      bto->init = 1;

    bto->no_str = controlRec.no_str;

    if ( bto->no_str > 0 ) {
      strncpy( bto->stateString[0], controlRec.strs[0], MAX_ENUM_STRING_SIZE );
    }
    else {
      strncpy( bto->stateString[0], "?0?", MAX_ENUM_STRING_SIZE );
    }
    if ( bto->no_str > 1 ) {
    strncpy( bto->stateString[1], controlRec.strs[1], MAX_ENUM_STRING_SIZE );
    }
    else {
      strncpy( bto->stateString[1], "?1?", MAX_ENUM_STRING_SIZE );
    }

  }

  bto->needCtlInfoInit = 1;
  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

static void bt_controlUpdate (
  struct event_handler_args ast_args )
{

activeButtonClass *bto = (activeButtonClass *) ast_args.usr;

  bto->controlValid = 1;
  bto->curControlV = *( (short *) ast_args.dbr );
  bto->needCtlRefresh = 1;
  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

static void bt_monitor_read_connect_state (
  struct connection_handler_args arg )
{

activeButtonClass *bto = (activeButtonClass *) ca_puser(arg.chid);

  if ( arg.op == CA_OP_CONN_UP ) {

    bto->needReadConnectInit = 1;

  }
  else {

    bto->readValid = 0;
    bto->readPvConnected = 0;
    bto->active = 0;
    bto->onColor.setDisconnected();
    bto->offColor.setDisconnected();
    bto->inconsistentColor.setDisconnected();
    bto->needDraw = 1;

  }

  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

static void bt_readInfoUpdate (
  struct event_handler_args ast_args )
{

activeButtonClass *bto = (activeButtonClass *) ast_args.usr;
struct dbr_gr_enum readRec = *( (dbr_gr_enum *) ast_args.dbr );

  bto->curReadV = readRec.value;

  bto->no_str = readRec.no_str;

  if ( bto->no_str > 0 ) {
    strncpy( bto->stateString[0], readRec.strs[0], MAX_ENUM_STRING_SIZE );
  }
  else {
    strncpy( bto->stateString[0], "?", MAX_ENUM_STRING_SIZE );
  }
  if ( bto->no_str > 1 ) {
  strncpy( bto->stateString[1], readRec.strs[1], MAX_ENUM_STRING_SIZE );
  }
  else {
    strncpy( bto->stateString[1], "?", MAX_ENUM_STRING_SIZE );
  }

  bto->needReadInfoInit = 1;
  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

static void bt_readUpdate (
  struct event_handler_args ast_args )
{

activeButtonClass *bto = (activeButtonClass *) ast_args.usr;

  bto->readValid = 1;
  bto->curReadV = *( (short *) ast_args.dbr );
  bto->needReadRefresh = 1;
  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

static void bt_alarmUpdate (
  struct event_handler_args ast_args )
{

activeButtonClass *bto = (activeButtonClass *) ast_args.usr;
struct dbr_sts_enum statusRec;

  statusRec = *( (struct dbr_sts_enum *) ast_args.dbr );

  bto->fgColor.setStatus( statusRec.status, statusRec.severity );

  bto->needErase = 1;
  bto->needDraw = 1;
  bto->actWin->appCtx->proc->lock();
  bto->actWin->addDefExeNode( bto->aglPtr );
  bto->actWin->appCtx->proc->unlock();

}

#endif

activeButtonClass::activeButtonClass ( void ) {

  name = new char[strlen("activeButtonClass")+1];
  strcpy( name, "activeButtonClass" );
  deleteRequest = 0;
  selected = 0;
  strcpy( stateString[0], "" );
  strcpy( stateString[1], "" );

  strcpy( id, "" );
  downCallbackFlag = 0;
  upCallbackFlag = 0;
  activateCallbackFlag = 0;
  deactivateCallbackFlag = 0;
  anyCallbackFlag = 0;
  downCallback = NULL;
  upCallback = NULL;
  activateCallback = NULL;
  deactivateCallback = NULL;

  fgColorMode = BTC_K_COLORMODE_STATIC;

}

// copy constructor
activeButtonClass::activeButtonClass
 ( const activeButtonClass *source ) {

activeGraphicClass *bto = (activeGraphicClass *) this;

  bto->clone( (activeGraphicClass *) source );

  name = new char[strlen("activeButtonClass")+1];
  strcpy( name, "activeButtonClass" );
  strcpy( stateString[0], "" );
  strcpy( stateString[1], "" );

  deleteRequest = 0;

  strcpy( id, source->id );

  downCallbackFlag = source->downCallbackFlag;
  upCallbackFlag = source->upCallbackFlag;
  activateCallbackFlag = source->activateCallbackFlag;
  deactivateCallbackFlag = source->deactivateCallbackFlag;
  anyCallbackFlag = downCallbackFlag || upCallbackFlag ||
   activateCallbackFlag || deactivateCallbackFlag;
  downCallback = NULL;
  upCallback = NULL;
  activateCallback = NULL;
  deactivateCallback = NULL;

  fgCb = source->fgCb;
  onCb = source->onCb;
  offCb = source->offCb;
  inconsistentCb = source->inconsistentCb;
  topShadowCb = source->topShadowCb;
  botShadowCb = source->botShadowCb;

  strncpy( fontTag, source->fontTag, 63 );

  fs = actWin->fi->getXFontStruct( fontTag );

  fgColor.copy( source->fgColor );
  offColor.copy( source->offColor );
  onColor.copy( source->onColor );
  inconsistentColor.copy( source->inconsistentColor );

  topShadowColor = source->topShadowColor;
  botShadowColor = source->botShadowColor;

  controlPvName.copy( source->controlPvName );
  readPvName.copy( source->readPvName );

  strncpy( onLabel, source->onLabel, MAX_ENUM_STRING_SIZE );
  strncpy( offLabel, source->offLabel, MAX_ENUM_STRING_SIZE );

  labelType = source->labelType;

  buttonType = source->buttonType;
  if ( buttonType == BTC_K_TOGGLE )
    toggle = 1;
  else
    toggle = 0;

  _3D = source->_3D;
  invisible = source->invisible;

  updateDimensions();

}

int activeButtonClass::createInteractive (
  activeWindowClass *aw_obj,
  int _x,
  int _y,
  int _w,
  int _h ) {

  actWin = (activeWindowClass *) aw_obj;
  x = _x;
  y = _y;
  w = _w;
  h = _h;

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  onColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  offColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  inconsistentColor.setColorIndex( actWin->defaultOffsetColor,
   actWin->ci );
  topShadowColor = actWin->defaultTopShadowColor;
  botShadowColor = actWin->defaultBotShadowColor;

  strcpy( stateString[0], "" );
  strcpy( stateString[1], "" );

  strcpy( fontTag, actWin->defaultBtnFontTag );
  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );

  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 10;
    fontDescent = 5;
    fontHeight = fontAscent + fontDescent;
  }

  updateDimensions();

  strcpy( onLabel, "" );
  strcpy( offLabel, "" );

  labelType = BTC_K_PV_STATE;
  buttonType = BTC_K_TOGGLE;
  toggle = 1;
  _3D = 1;
  invisible = 0;

  this->draw();

  this->editCreate();

  return 1;

}

int activeButtonClass::save (
  FILE *f )
{

int index;

  fprintf( f, "%-d %-d %-d\n", BTC_MAJOR_VERSION, BTC_MINOR_VERSION,
   BTC_RELEASE );

  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );

  index = fgColor.pixelIndex();
  fprintf( f, "%-d\n", index );

  fprintf( f, "%-d\n", fgColorMode );

  index = onColor.pixelIndex();
  fprintf( f, "%-d\n", index );

  index = offColor.pixelIndex();
  fprintf( f, "%-d\n", index );

  index = inconsistentColor.pixelIndex();
  fprintf( f, "%-d\n", index );

  index = topShadowColor;
  fprintf( f, "%-d\n", index );

  index = botShadowColor;
  fprintf( f, "%-d\n", index );

  if ( controlPvName.getRaw() )
    writeStringToFile( f, controlPvName.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( readPvName.getRaw() )
    writeStringToFile( f, readPvName.getRaw() );
  else
    writeStringToFile( f, "" );

  writeStringToFile( f, onLabel );
  writeStringToFile( f, offLabel );

  fprintf( f, "%-d\n", labelType );

  if ( toggle )
    buttonType = BTC_K_TOGGLE;
  else
    buttonType = BTC_K_PUSH;

  fprintf( f, "%-d\n", buttonType );

  fprintf( f, "%-d\n", _3D );

  fprintf( f, "%-d\n", invisible );

  writeStringToFile( f, fontTag );

  // version 1.3.0
  writeStringToFile( f, id );
  fprintf( f, "%-d\n", downCallbackFlag );
  fprintf( f, "%-d\n", upCallbackFlag );
  fprintf( f, "%-d\n", activateCallbackFlag );
  fprintf( f, "%-d\n", deactivateCallbackFlag );

  return 1;

}

int activeButtonClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int r, g, b, index;
int major, minor, release;
unsigned int pixel;
char oneName[39+1];

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release ); actWin->incLine();

  fscanf( f, "%d\n", &x ); actWin->incLine();
  fscanf( f, "%d\n", &y ); actWin->incLine();
  fscanf( f, "%d\n", &w ); actWin->incLine();
  fscanf( f, "%d\n", &h ); actWin->incLine();

  if ( major > 1 ) {

    fscanf( f, "%d\n", &index ); actWin->incLine();
    fgColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &fgColorMode ); actWin->incLine();

    if ( fgColorMode == BTC_K_COLORMODE_ALARM )
      fgColor.setAlarmSensitive();
    else
      fgColor.setAlarmInsensitive();

    fscanf( f, "%d\n", &index ); actWin->incLine();
    onColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &index ); actWin->incLine();
    offColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &index ); actWin->incLine();
    inconsistentColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &index ); actWin->incLine();
    topShadowColor = index;

    fscanf( f, "%d\n", &index ); actWin->incLine();
    botShadowColor = index;

  }
  else {

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    fgColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d\n", &fgColorMode ); actWin->incLine();

    if ( fgColorMode == BTC_K_COLORMODE_ALARM )
      fgColor.setAlarmSensitive();
    else
      fgColor.setAlarmInsensitive();

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    onColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    offColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    inconsistentColor.setColorIndex( index, actWin->ci );

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    topShadowColor = index;

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    index = actWin->ci->pixIndex( pixel );
    botShadowColor = index;

  }

  readStringFromFile( oneName, 39, f ); actWin->incLine();
  controlPvName.setRaw( oneName );

  readStringFromFile( oneName, 39, f ); actWin->incLine();
  readPvName.setRaw( oneName );

  readStringFromFile( onLabel, MAX_ENUM_STRING_SIZE, f ); actWin->incLine();

  readStringFromFile( offLabel, MAX_ENUM_STRING_SIZE, f ); actWin->incLine();

  fscanf( f, "%d\n", &labelType ); actWin->incLine();

  fscanf( f, "%d\n", &buttonType ); actWin->incLine();

  if ( buttonType == BTC_K_TOGGLE )
    toggle = 1;
  else
    toggle = 0;

  fscanf( f, "%d\n", &_3D ); actWin->incLine();

  fscanf( f, "%d\n", &invisible ); actWin->incLine();

  readStringFromFile( fontTag, 63, f ); actWin->incLine();

  if ( ( major > 1 ) || ( minor > 2 ) ) {
    readStringFromFile( this->id, 31, f ); actWin->incLine();
    fscanf( f, "%d\n", &downCallbackFlag ); actWin->incLine();
    fscanf( f, "%d\n", &upCallbackFlag ); actWin->incLine();
    fscanf( f, "%d\n", &activateCallbackFlag ); actWin->incLine();
    fscanf( f, "%d\n", &deactivateCallbackFlag ); actWin->incLine();
    anyCallbackFlag = downCallbackFlag || upCallbackFlag ||
     activateCallbackFlag || deactivateCallbackFlag;
  }
  else {
    strcpy( this->id, "" );
    downCallbackFlag = 0;
    upCallbackFlag = 0;
    activateCallbackFlag = 0;
    deactivateCallbackFlag = 0;
    anyCallbackFlag = 0;
  }

  this->initSelectBox();

  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );

  updateDimensions();

  return 1;

}

int activeButtonClass::importFromXchFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int r, g, b, more, param;
char *tk, *gotData, *context, buf[255+1];

  r = 0xffff;
  g = 0xffff;
  b = 0xffff;

  this->actWin = _actWin;

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  onColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  offColor.setColorIndex( actWin->defaultBgColor, actWin->ci );
  inconsistentColor.setColorIndex( actWin->defaultOffsetColor,
   actWin->ci );
  topShadowColor = actWin->defaultTopShadowColor;
  botShadowColor = actWin->defaultBotShadowColor;

  strcpy( stateString[0], "" );
  strcpy( stateString[1], "" );

  strcpy( fontTag, actWin->defaultBtnFontTag );
  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );

  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 10;
    fontDescent = 5;
    fontHeight = fontAscent + fontDescent;
  }

  updateDimensions();

  strcpy( onLabel, "" );
  strcpy( offLabel, "" );

  labelType = BTC_K_PV_STATE;
  buttonType = BTC_K_TOGGLE;
  toggle = 1;
  _3D = 1;
  invisible = 0;

  // continue until tag is <eod>

  do {

    gotData = getNextDataString( buf, 255, f );
    if ( !gotData ) {
      actWin->appCtx->postMessage( activeButtonClass_str57 );
      return 0;
    }

    context = NULL;

    tk = strtok_r( buf, " \t\n", &context );
    if ( !tk ) {
      actWin->appCtx->postMessage( activeButtonClass_str57 );
      return 0;
    }

    if ( strcmp( tk, "<eod>" ) == 0 ) {

      more = 0;

    }
    else {

      more = 1;

      if ( strcmp( tk, "x" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        x = atol( tk );

      }
      else if ( strcmp( tk, "y" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        y = atol( tk );

      }
      else if ( strcmp( tk, "w" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        w = atol( tk );

      }
      else if ( strcmp( tk, "h" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        h = atol( tk );

      }
            
      else if ( strcmp( tk, "ctlpv" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );

        if ( tk )
          controlPvName.setRaw( tk );
        else
          controlPvName.setRaw( "" );

      }
            
      else if ( strcmp( tk, "readpv" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );

        if ( tk )
          readPvName.setRaw( tk );
        else
          readPvName.setRaw( "" );

      }
            
      else if ( strcmp( tk, "truelabel" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );

        if ( tk ) {
          strncpy( onLabel, tk, MAX_ENUM_STRING_SIZE );
          onLabel[MAX_ENUM_STRING_SIZE] = 0;
	}
	else {
          strcpy( onLabel, "" );
	}

      }
            
      else if ( strcmp( tk, "falselabel" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );

        if ( tk ) {
          strncpy( offLabel, tk, MAX_ENUM_STRING_SIZE );
          offLabel[MAX_ENUM_STRING_SIZE] = 0;
	}
	else {
          strcpy( offLabel, "" );
	}

      }
            
      else if ( strcmp( tk, "font" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        strncpy( fontTag, tk, 63 );

      }
            
      else if ( strcmp( tk, "push" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        param = atol( tk );
        if ( param == 1 ) {
          buttonType = BTC_K_PUSH;
          toggle = 0;
	}
	else {
          buttonType = BTC_K_TOGGLE;
          toggle = 1;
	}

      }
            
      else if ( strcmp( tk, "3d" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        _3D = atol( tk );

      }
            
      else if ( strcmp( tk, "labelfrompv" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        param = atol( tk );
        if ( param == 1 ) {
          labelType = BTC_K_PV_STATE;
	}
	else {
          labelType = BTC_K_LITERAL;
	}

      }
            
      else if ( strcmp( tk, "invisible" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeButtonClass_str57 );
          return 0;
        }

        invisible = atol( tk );

      }
            
    }

  } while ( more );

  this->initSelectBox();

  fgColor.setAlarmInsensitive();

  actWin->fi->loadFontTag( fontTag );
  fs = actWin->fi->getXFontStruct( fontTag );

  updateDimensions();

  return 1;

}

int activeButtonClass::genericEdit ( void ) {

char title[32], *ptr;

  ptr = actWin->obj.getNameFromClass( "activeButtonClass" );
  if ( ptr )
    strncpy( title, ptr, 31 );
  else
    strncpy( title, activeButtonClass_str7, 31 );

  strncat( title, activeButtonClass_str8, 31 );

  strncpy( bufId, id, 31 );

  bufX = x;
  bufY = y;
  bufW = w;
  bufH = h;

  bufFgColor = fgColor.pixelIndex();
  bufFgColorMode = fgColorMode;

  bufOnColor = onColor.pixelIndex();

  bufOffColor = offColor.pixelIndex();

  bufInconsistentColor = inconsistentColor.pixelIndex();

  bufTopShadowColor = topShadowColor;
  bufBotShadowColor = botShadowColor;
  strncpy( bufFontTag, fontTag, 63 );

  if ( controlPvName.getRaw() )
    strncpy( controlBufPvName, controlPvName.getRaw(), 39 );
  else
    strncpy( controlBufPvName, "", 39 );

  if ( readPvName.getRaw() )
    strncpy( readBufPvName, readPvName.getRaw(), 39 );
  else
    strncpy( readBufPvName, "", 39 );

  strncpy( bufOnLabel, onLabel, MAX_ENUM_STRING_SIZE );
  strncpy( bufOffLabel, offLabel, MAX_ENUM_STRING_SIZE );

  bufDownCallbackFlag = downCallbackFlag;
  bufUpCallbackFlag = upCallbackFlag;
  bufActivateCallbackFlag = activateCallbackFlag;
  bufDeactivateCallbackFlag = deactivateCallbackFlag;

  if ( labelType == BTC_K_PV_STATE )
    strcpy( labelTypeString, activeButtonClass_str9 );
  else
    strcpy( labelTypeString, activeButtonClass_str10 );

  if ( toggle )
    strcpy( buttonTypeStr, activeButtonClass_str11 );
  else
    strcpy( buttonTypeStr, activeButtonClass_str12 );

  if ( _3D )
    strcpy( _3DString, activeButtonClass_str13 );
  else
    strcpy( _3DString, activeButtonClass_str14 );

  if ( invisible )
    strcpy( invisibleString, activeButtonClass_str15 );
  else
    strcpy( invisibleString, activeButtonClass_str16 );

  ef.create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  ef.addTextField( activeButtonClass_str17, 30, bufId, 31 );
  ef.addTextField( activeButtonClass_str18, 30, &bufX );
  ef.addTextField( activeButtonClass_str19, 30, &bufY );
  ef.addTextField( activeButtonClass_str20, 30, &bufW );
  ef.addTextField( activeButtonClass_str21, 30, &bufH );
  ef.addTextField( activeButtonClass_str22, 30, controlBufPvName, 39 );
  ef.addTextField( activeButtonClass_str23, 30, readBufPvName, 39 );
  ef.addOption( activeButtonClass_str24, activeButtonClass_str25, buttonTypeStr, 7 );
  ef.addOption( activeButtonClass_str26, activeButtonClass_str27, _3DString, 7 );
  ef.addOption( activeButtonClass_str28, activeButtonClass_str29, invisibleString, 7 );
  ef.addOption( activeButtonClass_str30, activeButtonClass_str31, labelTypeString, 15 );
  ef.addTextField( activeButtonClass_str32, 30, bufOnLabel, MAX_ENUM_STRING_SIZE );
  ef.addTextField( activeButtonClass_str33, 30, bufOffLabel, MAX_ENUM_STRING_SIZE );
  ef.addToggle( activeButtonClass_str34, &bufActivateCallbackFlag );
  ef.addToggle( activeButtonClass_str35, &bufDeactivateCallbackFlag );
  ef.addToggle( activeButtonClass_str36, &bufDownCallbackFlag );
  ef.addToggle( activeButtonClass_str37, &bufUpCallbackFlag );
  ef.addColorButton( activeButtonClass_str39, actWin->ci, &fgCb, &bufFgColor );
  ef.addOption( activeButtonClass_str40, activeButtonClass_str41, &bufFgColorMode );
  ef.addColorButton( activeButtonClass_str42, actWin->ci, &onCb, &bufOnColor );
  ef.addColorButton( activeButtonClass_str43, actWin->ci, &offCb, &bufOffColor );
  ef.addColorButton( activeButtonClass_str44, actWin->ci,
   &inconsistentCb, &bufInconsistentColor );
  ef.addColorButton( activeButtonClass_str45, actWin->ci, &topShadowCb, &bufTopShadowColor );
  ef.addColorButton( activeButtonClass_str46, actWin->ci, &botShadowCb, &bufBotShadowColor );

  ef.addFontMenu( activeButtonClass_str38, actWin->fi, &fm, fontTag );

  XtUnmanageChild( fm.alignWidget() ); // no alignment info

  return 1;

}

int activeButtonClass::editCreate ( void ) {

  this->genericEdit();
  ef.finished( btc_edit_ok, btc_edit_apply, btc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activeButtonClass::edit ( void ) {

  this->genericEdit();
  ef.finished( btc_edit_ok, btc_edit_apply, btc_edit_cancel, this );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activeButtonClass::erase ( void ) {

  if ( deleteRequest ) return 1;

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activeButtonClass::eraseActive ( void ) {

  if ( !init || !activeMode || invisible ) return 1;

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.eraseGC(), x, y, w, h );

  return 1;

}

int activeButtonClass::draw ( void ) {

int tX, tY;
XRectangle xR = { x, y, w, h };

  if ( deleteRequest ) return 1;

  actWin->drawGc.saveFg();

  actWin->drawGc.setFG( onColor.pixelColor() );

  XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, w, h );

  XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, w, h );

  if ( _3D ) {

  actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, x+w, y );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x, y, x, y+h );

   actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

   XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
    actWin->drawGc.normGC(), x, y+h, x+w, y+h );

   XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
    actWin->drawGc.normGC(), x+w, y, x+w, y+h );

  actWin->drawGc.setFG( actWin->ci->pix(topShadowColor) );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+1, y+1, x+w-1, y+1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+2, y+2, x+w-2, y+2 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+1, y+1, x+1, y+h-1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+2, y+2, x+2, y+h-2 );

  actWin->drawGc.setFG( actWin->ci->pix(botShadowColor) );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+1, y+h-1, x+w-1, y+h-1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+2, y+h-2, x+w-2, y+h-2 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+w-1, y+1, x+w-1, y+h-1 );

  XDrawLine( actWin->d, XtWindow(actWin->drawWidget),
   actWin->drawGc.normGC(), x+w-2, y+2, x+w-2, y+h-2 );

  }

  if ( fs ) {

    actWin->drawGc.addNormXClipRectangle( xR );

    actWin->drawGc.setFG( fgColor.pixelColor() );
    actWin->drawGc.setFontTag( fontTag, actWin->fi );

    tX = x + w/2;
    tY = y + h/2 - fontAscent/2;

    drawText( actWin->drawWidget, &actWin->drawGc, fs, tX, tY,
     XmALIGNMENT_CENTER, onLabel );

    actWin->drawGc.removeNormXClipRectangle();

  }

  actWin->drawGc.restoreFg();

  return 1;

}

int activeButtonClass::drawActive ( void ) {

int cV, rV, tX, tY;
XRectangle xR = { x, y, w, h };
char string[MAX_ENUM_STRING_SIZE+1];

  if ( !init || !activeMode || invisible ) return 1;

  cV = controlV;
  rV = readV;

  actWin->executeGc.saveFg();

  if ( controlExists && readExists ) {

    if ( ( cV != rV ) || !controlValid || !readValid ) {
      actWin->executeGc.setFG( inconsistentColor.getColor() );
    }
    else if ( cV == 0 ) {
      actWin->executeGc.setFG( offColor.getColor() );
    }
    else {
      actWin->executeGc.setFG( onColor.getColor() );
    }

  }
  else if ( readExists ) {

    cV = readV;

    if ( cV == 0 )
      actWin->executeGc.setFG( offColor.getColor() );
    else
      actWin->executeGc.setFG( onColor.getColor() );

  }
  else if ( controlExists ) {

    if ( cV == 0 )
      actWin->executeGc.setFG( offColor.getColor() );
    else
      actWin->executeGc.setFG( onColor.getColor() );

  }
  else if ( anyCallbackFlag ) {

    if ( cV != rV )
      actWin->executeGc.setFG( inconsistentColor.getColor() );
    else if ( cV == 0 )
      actWin->executeGc.setFG( offColor.getColor() );
    else
      actWin->executeGc.setFG( onColor.getColor() );

  }
  else {

    actWin->executeGc.setFG( inconsistentColor.getColor() );

  }

  XFillRectangle( actWin->d, XtWindow(actWin->executeWidget),
   actWin->executeGc.normGC(), x, y, w, h );

  XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
   actWin->executeGc.normGC(), x, y, w, h );

  if ( cV == 0 ) {

    if ( labelType == BTC_K_LITERAL ) {
      strncpy( string, offLabel, MAX_ENUM_STRING_SIZE );
    }
    else {
      strncpy( string, stateString[0], MAX_ENUM_STRING_SIZE );
    }

    if ( _3D ) {

    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x+w, y );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x, y+h );

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y+h, x+w, y+h );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w, y, x+w, y+h );

    // top
    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+1, y+1, x+w-1, y+1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+2, y+2, x+w-2, y+2 );

    // left
    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+1, y+1, x+1, y+h-1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+2, y+2, x+2, y+h-2 );

    // bottom
    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+1, y+h-1, x+w-1, y+h-1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+2, y+h-2, x+w-2, y+h-2 );

    // right
    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w-1, y+1, x+w-1, y+h-1 );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w-2, y+2, x+w-2, y+h-2 );

    }

  }
  else {

    if ( labelType == BTC_K_LITERAL ) {
      strncpy( string, onLabel, MAX_ENUM_STRING_SIZE );
    }
    else {
      strncpy( string, stateString[1], MAX_ENUM_STRING_SIZE );
    }

    if ( _3D ) {

    actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x+w, y );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y, x, y+h );

    // top

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

//     XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
//      actWin->executeGc.normGC(), x, y, x+w, y );

//      actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

//      XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
//       actWin->executeGc.normGC(), x+1, y+1, x+w-1, y+1 );

    //left

//     actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

//     XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
//      actWin->executeGc.normGC(), x, y, x, y+h );

//      actWin->executeGc.setFG( actWin->ci->pix(botShadowColor) );

//      XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
//       actWin->executeGc.normGC(), x+1, y+1, x+1, y+h-1 );

    // bottom

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x, y+h, x+w, y+h );

    //right

    actWin->executeGc.setFG( actWin->ci->pix(topShadowColor) );

    XDrawLine( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), x+w, y, x+w, y+h );

    }

  }

  if ( fs ) {

    actWin->executeGc.addNormXClipRectangle( xR );

    actWin->executeGc.setFG( fgColor.getColor() );
    actWin->executeGc.setFontTag( fontTag, actWin->fi );

    tX = x + w/2;
    tY = y + h/2 - fontAscent/2;

    if ( labelType == BTC_K_LITERAL ) {

      drawText( actWin->executeWidget, &actWin->executeGc, fs, tX, tY,
       XmALIGNMENT_CENTER, string );

    }
    else {

      drawText( actWin->executeWidget, &actWin->executeGc, fs, tX, tY,
       XmALIGNMENT_CENTER, string );

    }

    actWin->executeGc.removeNormXClipRectangle();

  }

  actWin->executeGc.restoreFg();

  return 1;

}

int activeButtonClass::activate (
  int pass,
  void *ptr )
{

int stat, opStat;
char callbackName[63+1];

  switch ( pass ) {

  case 1:

    aglPtr = ptr;
    needCtlConnectInit = needCtlInfoInit = needCtlRefresh =
     needReadConnectInit = needReadInfoInit = needReadRefresh =
     needErase = needDraw = 0;
    init = 0;
    opComplete = 0;
    controlValid = 0;
    readValid = 0;
    controlV = 0;

#ifdef __epics__
    controlEventId = readEventId = alarmEventId = 0;
#endif

    controlPvConnected = readPvConnected = active = 0;
    activeMode = 1;

    if ( !controlPvName.getExpanded() ||
       ( strcmp( controlPvName.getExpanded(), "" ) == 0 ) ) {
      controlExists = 0;
    }
    else {
      controlExists = 1;
    }

    if ( !readPvName.getExpanded() ||
       ( strcmp( readPvName.getExpanded(), "" ) == 0 ) ) {
      readExists = 0;
    }
    else {
      readExists = 1;
    }

    break;

  case 2:

    if ( !opComplete ) {

      if ( anyCallbackFlag ) {

        if ( downCallbackFlag ) {
          strncpy( callbackName, id, 63 );
          strncat( callbackName, "Down", 63 );
          downCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( upCallbackFlag ) {
          strncpy( callbackName, id, 63 );
          strncat( callbackName, "Up", 63 );
          upCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( activateCallbackFlag ) {
          strncpy( callbackName, id, 63 );
          strncat( callbackName, "Activate", 63 );
          activateCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( deactivateCallbackFlag ) {
          strncpy( callbackName, id, 63 );
          strncat( callbackName, "Deactivate", 63 );
          deactivateCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( activateCallback ) {
          (*activateCallback)( this );
        }

      }

      opStat = 1;

#ifdef __epics__

      if ( controlExists ) {
        stat = ca_search_and_connect( controlPvName.getExpanded(),
         &controlPvId, bt_monitor_control_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeButtonClass_str47 );
          opStat = 0;
        }
      }

      if ( readExists ) {
        stat = ca_search_and_connect( readPvName.getExpanded(), &readPvId,
         bt_monitor_read_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeButtonClass_str48 );
          opStat = 0;
        }
      }

      if ( !( opStat & 1 ) ) opComplete = 1;

#endif

      if ( !controlExists && !readExists ) {
        init = 1;
        active = 1;
        onColor.setConnected();
        offColor.setConnected();
        inconsistentColor.setConnected();
        controlV = readV = 0;
      }

      return opStat;

    }

    break;

  case 3:
  case 4:
  case 5:
  case 6:

    break;

  }

  return 1;

}

int activeButtonClass::deactivate (
  int pass
) {

int stat;

  active = 0;
  activeMode = 0;

  if ( pass == 1 ) {

  if ( deactivateCallback ) {
    (*deactivateCallback)( this );
  }

#ifdef __epics__

  controlEventId = 0;
  readEventId = 0;
  alarmEventId = 0;

  if ( controlExists ) {
    stat = ca_clear_channel( controlPvId );
    if ( stat != ECA_NORMAL )
      printf( activeButtonClass_str49 );
  }

  if ( readExists ) {
    stat = ca_clear_channel( readPvId );
    if ( stat != ECA_NORMAL )
      printf( activeButtonClass_str50 );
  }

#endif

  }

  return 1;

}

void activeButtonClass::updateDimensions ( void )
{

  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 10;
    fontDescent = 5;
    fontHeight = fontAscent + fontDescent;
  }

}

void activeButtonClass::btnUp (
  int x,
  int y,
  int buttonState,
  int buttonNumber )
{

short value;
int stat;

  if ( !active ) return;

  if ( !ca_write_access( controlPvId ) ) return;

  if ( buttonNumber != 1 ) return;

  if ( toggle ) return;

  value = 0;

  if ( !controlExists ) controlV = 0;

  if ( upCallback ) {
    (*upCallback)( this );
  }

#ifdef __epics__
  if ( !controlExists ) return;
  stat = ca_put( DBR_ENUM, controlPvId, &value );
#endif

}

void activeButtonClass::btnDown (
  int x,
  int y,
  int buttonState,
  int buttonNumber )
{

short value;
int stat;

  if ( !active ) return;

  if ( controlExists ) {
    if ( !ca_write_access( controlPvId ) ) return;
  }

  if ( buttonNumber != 1 ) return;

  if ( toggle ) {
    if ( controlV == 0 ) {
      value = 1;
      if ( !controlExists ) controlV = 1;
      if ( downCallback ) {
        (*downCallback)( this );
      }
    }
    else {
      value = 0;
      if ( !controlExists ) controlV = 0;
      if ( upCallback ) {
        (*upCallback)( this );
      }
    }
  }
  else {
    value = 1;
    if ( !controlExists ) controlV = 1;
    if ( downCallback ) {
      (*downCallback)( this );
    }
  }

#ifdef __epics__
  if ( !controlExists ) return;
  stat = ca_put( DBR_ENUM, controlPvId, &value );
#endif

}

void activeButtonClass::pointerIn (
  int _x,
  int _y,
  int buttonState )
{

  if ( !active ) return;

  if ( !ca_write_access( controlPvId ) ) {
    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_NO );
  }
  else {
    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_DEFAULT );
  }

  activeGraphicClass::pointerIn( _x, _y, buttonState );

}

int activeButtonClass::getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus )
{

  *drag = 0;

  if ( controlExists ) {
    *focus = 1;
    *up = 1;
    *down = 1;
  }
  else {
    *focus = 0;
    *up = 0;
    *down = 0;
  }

  return 1;

}

int activeButtonClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat;

  stat = controlPvName.expand1st( numMacros, macros, expansions );

  stat = readPvName.expand1st( numMacros, macros, expansions );

  return stat;

}

int activeButtonClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] )
{

int stat;

  stat = controlPvName.expand2nd( numMacros, macros, expansions );

  stat = readPvName.expand2nd( numMacros, macros, expansions );

  return stat;

}

int activeButtonClass::containsMacros ( void ) {

int result;

  result = controlPvName.containsPrimaryMacros();

  if ( result ) return result;

  result = readPvName.containsPrimaryMacros();

  return result;

}

void activeButtonClass::executeDeferred ( void ) {

int stat, ncc, nci, ncr, nrc, nri, nrr, ne, nd;
short rv, cv;
char msg[79+1];

  if ( actWin->isIconified ) return;

  actWin->appCtx->proc->lock();
  ncc = needCtlConnectInit; needCtlConnectInit = 0;
  nci = needCtlInfoInit; needCtlInfoInit = 0;
  ncr = needCtlRefresh; needCtlRefresh = 0;
  nrc = needReadConnectInit; needReadConnectInit = 0;
  nri = needReadInfoInit; needReadInfoInit = 0;
  nrr = needReadRefresh; needReadRefresh = 0;
  ne = needErase; needErase = 0;
  nd = needDraw; needDraw = 0;
  rv = curReadV;
  cv = curControlV;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

#ifdef __epics__

  if ( ncc ) {

    if ( ca_field_type(controlPvId) != DBR_ENUM ) {
      strncpy( msg, actWin->obj.getNameFromClass( "activeButtonClass" ),
       79 );
      strncat( msg, activeButtonClass_str51, 79 );
      actWin->appCtx->postMessage( msg );
      controlPvConnected = 0;
      active = 0;
      return;
    }

    stat = ca_get_callback( DBR_GR_ENUM, controlPvId,
     bt_controlInfoUpdate, (void *) this );

  }

  if ( nci ) {

    if ( !controlEventId ) {

      stat = ca_add_masked_array_event( DBR_ENUM, 1, controlPvId,
       bt_controlUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
       &controlEventId, DBE_VALUE );
      if ( stat != ECA_NORMAL )
        printf( activeButtonClass_str52 );

    }

    if ( !(readExists) ) {

      if ( !alarmEventId ) {

        stat = ca_add_masked_array_event( DBR_STS_ENUM, 1, controlPvId,
         bt_alarmUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &alarmEventId, DBE_ALARM );
        if ( stat != ECA_NORMAL )
          printf( activeButtonClass_str53 );

      }

    }

    controlPvConnected = 1;

    if ( readPvConnected || !readExists ) {
      onColor.setConnected();
      offColor.setConnected();
      inconsistentColor.setConnected();
      init = 1;
      active = 1;
      eraseActive();
      readV = rv;
      controlV = cv;
      drawActive();
    }

  }

  if ( ncr ) {

    eraseActive();
    readV = rv; 
    controlV = cv;
    drawActive();

  }

  if ( nrc ) {

    if ( ca_field_type(readPvId) != DBR_ENUM ) {
      strncpy( msg, actWin->obj.getNameFromClass( "activeButtonClass" ),
       79 );
      strncat( msg, activeButtonClass_str54, 79 );
      actWin->appCtx->postMessage( msg );
      readPvConnected = 0;
      active = 0;
      return;
    }

    stat = ca_get_callback( DBR_GR_ENUM, readPvId,
     bt_readInfoUpdate, (void *) this );

  }

  if ( nri ) {

    if ( !readEventId ) {

      stat = ca_add_masked_array_event( DBR_ENUM, 1, readPvId,
       bt_readUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
       &readEventId, DBE_VALUE );
      if ( stat != ECA_NORMAL )
        printf( activeButtonClass_str55 );

    }

    if ( !alarmEventId ) {

      stat = ca_add_masked_array_event( DBR_STS_ENUM, 1, readPvId,
       bt_alarmUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
       &alarmEventId, DBE_ALARM );
      if ( stat != ECA_NORMAL )
        printf( activeButtonClass_str56 );

    }

    readPvConnected = 1;

    if ( controlPvConnected || !controlExists ) {
      onColor.setConnected();
      offColor.setConnected();
      inconsistentColor.setConnected();
      init = 1;
      active = 1;
      eraseActive();
      controlV = cv;
      readV = rv;
      drawActive();
    }

  }

#endif

  if ( nrr ) {

    eraseActive();
    controlV = cv;
    readV = rv;
    drawActive();

  }

  if ( ne ) {

    eraseActive();

  }

  if ( nd ) {

    drawActive();

  }

}

int activeButtonClass::setProperty (
  char *prop,
  int *value )
{

  if ( strcmp( prop, "controlValue" ) == 0 ) {

    curControlV = (short) *value;
    needCtlRefresh = 1;
    actWin->appCtx->proc->lock();
    actWin->addDefExeNode( aglPtr );
    actWin->appCtx->proc->unlock();
    return 1;

  }
  else if ( strcmp( prop, "readValue" ) == 0 ) {

    curReadV = (short) *value;
    needReadRefresh = 1;
    actWin->appCtx->proc->lock();
    actWin->addDefExeNode( aglPtr );
    actWin->appCtx->proc->unlock();
    return 1;

  }

  return 0;

}

char *activeButtonClass::firstDragName ( void ) {

  dragIndex = 0;
  return dragName[dragIndex];

}

char *activeButtonClass::nextDragName ( void ) {

  if ( dragIndex < (int) ( sizeof(dragName) / sizeof(char *) ) - 1 ) {
    dragIndex++;
    return dragName[dragIndex];
  }
  else {
    return NULL;
  }

}

char *activeButtonClass::dragValue (
  int i ) {

  if ( !i ) {
    return controlPvName.getExpanded();
  }
  else {
    return readPvName.getExpanded();
  }

}

void activeButtonClass::changeDisplayParams (
  unsigned int _flag,
  char *_fontTag,
  int _alignment,
  char *_ctlFontTag,
  int _ctlAlignment,
  char *_btnFontTag,
  int _btnAlignment,
  int _textFgColor,
  int _fg1Color,
  int _fg2Color,
  int _offsetColor,
  int _bgColor,
  int _topShadowColor,
  int _botShadowColor )
{

  if ( _flag & ACTGRF_TEXTFGCOLOR_MASK )
    fgColor.setColorIndex( _textFgColor, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    onColor.setColorIndex( _bgColor, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    offColor.setColorIndex( _bgColor, actWin->ci );

  if ( _flag & ACTGRF_OFFSETCOLOR_MASK )
    inconsistentColor.setColorIndex( _offsetColor, actWin->ci );

  if ( _flag & ACTGRF_TOPSHADOWCOLOR_MASK )
    topShadowColor = _topShadowColor;

  if ( _flag & ACTGRF_BOTSHADOWCOLOR_MASK )
    botShadowColor = _botShadowColor;

  if ( _flag & ACTGRF_BTNFONTTAG_MASK ) {
    strncpy( fontTag, _btnFontTag, 63 );
    fontTag[63] = 0;
    actWin->fi->loadFontTag( fontTag );
    fs = actWin->fi->getXFontStruct( fontTag );
    updateDimensions();
  }

}

void activeButtonClass::changePvNames (
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
  char *alarmPvs[] )
{

  if ( flag & ACTGRF_CTLPVS_MASK ) {
    if ( numCtlPvs ) {
      controlPvName.setRaw( ctlPvs[0] );
    }
  }

  if ( flag & ACTGRF_READBACKPVS_MASK ) {
    if ( numReadbackPvs ) {
      readPvName.setRaw( readbackPvs[0] );
    }
  }

}

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeButtonClassPtr ( void ) {

activeButtonClass *ptr;

  ptr = new activeButtonClass;
  return (void *) ptr;

}

void *clone_activeButtonClassPtr (
  void *_srcPtr )
{

activeButtonClass *ptr, *srcPtr;

  srcPtr = (activeButtonClass *) _srcPtr;

  ptr = new activeButtonClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif
