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

#define __x_text_dsp_obj_cc 1

#include "x_text_dsp_obj.h"
#include "app_pkg.h"
#include "act_win.h"

#include "thread.h"

static int g_transInit = 1;
XtTranslations g_parsedTrans;

static char g_dragTrans[] =
  "#override\n\
   ~Shift<Btn2Down>: startDrag()\n\
   Shift<Btn2Up>: selectDrag()";

static XtActionsRec g_dragActions[] = {
  { "startDrag", (XtActionProc) drag },
  { "selectDrag", (XtActionProc) selectDrag }
};

static int stringPut (
  chid id,
  int size,
  char *string
) {

int stat;

  if ( size == 1 ) {

    string[39] = 0;

    stat = ca_put( DBR_STRING, id, string );

  }
  else {

    stat = ca_array_put( DBR_CHAR, strlen(string)+1, id, string );

  }

  return stat;

}

static void dropTransferProc (
  Widget w,
  XtPointer clientData,
  Atom *selType,
  Atom *type,
  XtPointer value,
  unsigned long *length,
  int format )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) clientData;
char *str = (char *) value;
int stat, ivalue, doPut;
double dvalue;
char string[XTDC_K_MAX+1], tmp[XTDC_K_MAX+1];

  if ( !axtdo ) return;

  if ( *type == XA_STRING ) {

    if ( str ) {

      switch ( axtdo->pvType ) {

      case DBR_FLOAT:
      case DBR_DOUBLE:

        doPut = 0;
        if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {
          if ( strlen( str ) > 2 ) {
            if ( ( strncmp( str, "0x", 2 ) != 0 ) &&
                 ( strncmp( str, "0X", 2 ) != 0 ) ) {
              strcpy( tmp, "0x" );
            }
            else {
              strcpy( tmp, "" );
            }
            Strncat( tmp, str, 15 );
            tmp[15] = 0;
	  }
	  else {
            strcpy( tmp, "0x" );
            Strncat( tmp, str, 15 );
            tmp[15] = 0;
          }
          if ( isLegalInteger(tmp) ) {
            doPut = 1;
            ivalue = strtol( tmp, NULL, 0 );
            dvalue = (double) ivalue;
	  }
	}
	else {
          if ( isLegalFloat(str) ) {
            doPut = 1;
            dvalue = atof( str );
          }
	}

        if ( doPut ) {

          if ( axtdo->pvExists ) {
#ifdef __epics__
            stat = ca_put( DBR_DOUBLE, axtdo->pvId, &dvalue );
#endif
          }
          else {
            axtdo->needUpdate = 1;
            axtdo->actWin->appCtx->proc->lock();
            axtdo->actWin->addDefExeNode( axtdo->aglPtr );
            axtdo->actWin->appCtx->proc->unlock();
          }

        }

        break;

      case DBR_SHORT:
      case DBR_LONG:
        if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {
          if ( strlen( str ) > 2 ) {
            if ( ( strncmp( str, "0x", 2 ) != 0 ) &&
                 ( strncmp( str, "0X", 2 ) != 0 ) ) {
              strcpy( tmp, "0x" );
            }
            else {
              strcpy( tmp, "" );
            }
            Strncat( tmp, str, 15 );
            tmp[15] = 0;
	  }
	  else {
            strcpy( tmp, "0x" );
            Strncat( tmp, str, 15 );
            tmp[15] = 0;
          }
	}
	else {
          strncpy( tmp, str, XTDC_K_MAX );
          tmp[XTDC_K_MAX] = 0;
	}
        if ( isLegalInteger(tmp) ) {
          ivalue = strtol( tmp, NULL, 0 );
          if ( axtdo->pvExists ) {
#ifdef __epics__
            stat = ca_put( DBR_LONG, axtdo->pvId, &ivalue );
#endif
          }
          else {
            axtdo->needUpdate = 1;
            axtdo->actWin->appCtx->proc->lock();
            axtdo->actWin->addDefExeNode( axtdo->aglPtr );
            axtdo->actWin->appCtx->proc->unlock();
          }

        }
        break;

      case DBR_STRING:
        strncpy( string, str, XTDC_K_MAX );
        string[XTDC_K_MAX] = 0;
        if ( axtdo->pvExists ) {
#ifdef __epics__
	  stat = stringPut( axtdo->pvId, axtdo->pvCount, string );
          //stat = ca_put( DBR_STRING, axtdo->pvId, string );
#endif
        }
        else {
          axtdo->needUpdate = 1;
          axtdo->actWin->appCtx->proc->lock();
          axtdo->actWin->addDefExeNode( axtdo->aglPtr );
          axtdo->actWin->appCtx->proc->unlock();
        }

        break;

      }

    }

  }

}

static void handleDrop (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo;
XmDropProcCallback ptr = (XmDropProcCallback) call;
XmDropTransferEntryRec transferEntries[2];
XmDropTransferEntry transferList;
int n;
Arg args[10];
Widget dc;

  n = 0;
  XtSetArg( args[n], XmNuserData, (XtPointer) &axtdo ); n++;
  XtGetValues( w, args, n );
  if ( !axtdo ) return;

  dc = ptr->dragContext;

  n = 0;
  if ( ptr->dropAction != XmDROP ) {
    XtSetArg( args[n], XmNtransferStatus, XmTRANSFER_FAILURE ); n++;
    XtSetArg( args[n], XmNnumDropTransfers, 0 ); n++;
  }
  else {
    transferEntries[0].target = XA_STRING;
    transferEntries[0].client_data = (XtPointer) axtdo;
    transferList = transferEntries;
    XtSetArg( args[n], XmNdropTransfers, transferList ); n++;
    XtSetArg( args[n], XmNnumDropTransfers, 1 ); n++;
    XtSetArg( args[n], XmNtransferProc, dropTransferProc ); n++;
  }

  //  XtSetArg( args[n], XmN ); n++;

  XmDropTransferStart( dc, args, n );

}

static void doBlink (
  void *ptr
) {

activeXTextDspClass *axtdo = (activeXTextDspClass *) ptr;

  if ( !axtdo->activeMode ) {
    if ( axtdo->isSelected() ) axtdo->drawSelectBoxCorners(); // erase via xor
    if ( axtdo->smartRefresh ) {
      axtdo->smartDrawAll();
    }
    else {
      axtdo->draw();
    }
    if ( axtdo->isSelected() ) axtdo->drawSelectBoxCorners();
  }
  else {
    axtdo->bufInvalidate();
    if ( axtdo->smartRefresh ) {
      axtdo->smartDrawAllActive();
    }
    else {
      axtdo->drawActive();
    }
  }

}

static void unconnectedTimeout (
  XtPointer client,
  XtIntervalId *id )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  if ( !axtdo->init ) {
    axtdo->needToDrawUnconnected = 1;
    axtdo->needRefresh = 1;
    axtdo->actWin->addDefExeNode( axtdo->aglPtr );
  }

  axtdo->unconnectedTimer = 0;

}

static void xtdoRestoreValue (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
Arg args[10];
int n, l;
char *buf;

  axtdo->actWin->appCtx->proc->lock();
  axtdo->needRefresh = 1;
  axtdo->actWin->addDefExeNode( axtdo->aglPtr );
  axtdo->actWin->appCtx->proc->unlock();

  n = 0;
  XtSetArg( args[n], XmNcursorPositionVisible, (XtArgVal) False ); n++;
  XtSetValues( axtdo->tf_widget, args, n );

  buf = XmTextGetString( axtdo->tf_widget );
  l = strlen(buf);
  XtFree( buf );

  axtdo->grabUpdate = 0;

}

static void xtdoCancelStr (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->editDialogIsActive = 0;

}

static void xtdoSetCpValue (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat;
unsigned int i, ii;
char tmp[XTDC_K_MAX+1];

  if ( axtdo->dateAsFileName ) {

    axtdo->cp.getDate( tmp, XTDC_K_MAX );
    tmp[XTDC_K_MAX] = 0;

    for ( i=0, ii=0; i<strlen(tmp)+1; i++ ) {

      if ( tmp[i] == '-' ) {
        // do nothing
      }
      else if ( tmp[i] == ' ' ) {
        axtdo->entryValue[ii] = '_';
        ii++;
      }
      else if ( tmp[i] == ':' ) {
        // do nothing
      }
      else {
        axtdo->entryValue[ii] = tmp[i];
        ii++;
      }

    }

  }
  else {

    axtdo->cp.getDate( axtdo->entryValue, XTDC_K_MAX );

  }

  strncpy( axtdo->curValue, axtdo->entryValue, XTDC_K_MAX );
  axtdo->curValue[XTDC_K_MAX] = 0;

  axtdo->editDialogIsActive = 0;

  if ( axtdo->pvExists ) {
#ifdef __epics__
    stat = stringPut( axtdo->pvId, axtdo->pvCount, (char *) &axtdo->curValue );
    //stat = ca_put( DBR_STRING, axtdo->pvId, &axtdo->curValue );
#endif
  }

  axtdo->actWin->appCtx->proc->lock();
  axtdo->needUpdate = 1;
  axtdo->actWin->addDefExeNode( axtdo->aglPtr );
  axtdo->actWin->appCtx->proc->unlock();

}

static void xtdoSetFsValue (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat;
char tmp[XTDC_K_MAX+1], name[XTDC_K_MAX+1], *tk;

  if ( axtdo->fileComponent != XTDC_K_FILE_FULL_PATH ) {

    axtdo->fsel.getSelection( tmp, XTDC_K_MAX );

    tk = strtok( tmp, "/" );
    if ( tk ) {
      strncpy( name, tk, XTDC_K_MAX );
      name[XTDC_K_MAX] = 0;
    }
    else {
      strcpy( name, "" );
    }
    while ( tk ) {

      tk = strtok( NULL, "/" );
      if ( tk ) {
        strncpy( name, tk, XTDC_K_MAX );
        name[XTDC_K_MAX] = 0;
      }

    }

    if ( axtdo->fileComponent == XTDC_K_FILE_NAME ) {

      strncpy( tmp, name, XTDC_K_MAX );
      tmp[XTDC_K_MAX] = 0;
      tk = strtok( tmp, "." );
      if ( tk ) {
        strncpy( name, tk, XTDC_K_MAX );
        name[XTDC_K_MAX] = 0;
      }

    }

    strncpy( axtdo->entryValue, name, XTDC_K_MAX );

  }
  else {

    axtdo->fsel.getSelection( axtdo->entryValue, XTDC_K_MAX );

  }

  strncpy( axtdo->curValue, axtdo->entryValue, XTDC_K_MAX );
  axtdo->curValue[XTDC_K_MAX] = 0;

  axtdo->editDialogIsActive = 0;

  if ( axtdo->pvExists ) {
#ifdef __epics__
    stat = stringPut( axtdo->pvId, axtdo->pvCount, (char *) &axtdo->curValue );
    //stat = ca_put( DBR_STRING, axtdo->pvId, &axtdo->curValue );
#endif
  }

  axtdo->actWin->appCtx->proc->lock();
  axtdo->needUpdate = 1;
  axtdo->actWin->addDefExeNode( axtdo->aglPtr );
  axtdo->actWin->appCtx->proc->unlock();

}

static void xtdoCancelKp (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->editDialogIsActive = 0;

}

static void xtdoSetKpDoubleValue (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat;

  axtdo->editDialogIsActive = 0;
  stat = ca_put( DBR_DOUBLE, axtdo->pvId, &axtdo->kpDouble );

}

static void xtdoSetKpIntValue (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat;

  axtdo->editDialogIsActive = 0;
  stat = ca_put( DBR_LONG, axtdo->pvId, &axtdo->kpInt );

}

static void xtdoSetValueChanged (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->widget_value_changed = 1;

  if ( axtdo->changeCallback ) {
    (*axtdo->changeCallback)( axtdo );
  }

}

static void xtdoGrabUpdate (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  if ( !axtdo->grabUpdate ) {

    //XSetInputFocus( axtdo->actWin->display(),
     // XtWindow(axtdo->actWin->executeWidget), RevertToNone, CurrentTime );

    XSetInputFocus( axtdo->actWin->display(),
     XtWindow(axtdo->tf_widget), RevertToNone, CurrentTime );

  }

  axtdo->grabUpdate = 1;

}

static void xtdoSetSelection (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int l;
char *buf;
Arg args[10];
int n;

  axtdo->widget_value_changed = 0;

  buf = XmTextGetString( axtdo->tf_widget );
  l = strlen(buf);
  XtFree( buf );

  n = 0;
  XtSetArg( args[n], XmNcursorPositionVisible, (XtArgVal) True ); n++;
  XtSetValues( axtdo->tf_widget, args, n );

  if ( axtdo->autoSelect ) {
    XmTextSetSelection( axtdo->tf_widget, 0, l,
    XtLastTimestampProcessed( axtdo->actWin->display() ) );
  }

  XmTextSetInsertionPosition( axtdo->tf_widget, l );

}

static void xtdoTextFieldToStringA (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat;
char string[XTDC_K_MAX+1];
char *buf;

  buf = XmTextGetString( axtdo->tf_widget );
  strncpy( axtdo->entryValue, buf, XTDC_K_MAX );
  axtdo->entryValue[XTDC_K_MAX] = 0;
  XtFree( buf );

  strncpy( axtdo->curValue, axtdo->entryValue, XTDC_K_MAX );
  axtdo->curValue[XTDC_K_MAX] = 0;
  strncpy( string, axtdo->entryValue, XTDC_K_MAX );
  string[XTDC_K_MAX] = 0;
  if ( axtdo->pvExists ) {
#ifdef __epics__
    stat = stringPut( axtdo->pvId, axtdo->pvCount, string );
    //stat = ca_put( DBR_STRING, axtdo->pvId, string );
#endif
  }
  else {
    axtdo->actWin->appCtx->proc->lock();
    axtdo->needUpdate = 1;
    axtdo->actWin->addDefExeNode( axtdo->aglPtr );
    axtdo->actWin->appCtx->proc->unlock();
  }

  XmTextSetInsertionPosition( axtdo->tf_widget, 0 );

  axtdo->grabUpdate = 0;

}

static void xtdoTextFieldToStringLF (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat;
char string[XTDC_K_MAX+1];
char *buf;
Arg args[10];
int n;

  n = 0;
  XtSetArg( args[n], XmNcursorPositionVisible, (XtArgVal) False ); n++;
  XtSetValues( axtdo->tf_widget, args, n );

  //XmTextSetInsertionPosition( axtdo->tf_widget, 0 );

  axtdo->grabUpdate = 0;

  if ( !axtdo->widget_value_changed ) return;
 
  buf = XmTextGetString( axtdo->tf_widget );
  strncpy( axtdo->entryValue, buf, XTDC_K_MAX );
  axtdo->entryValue[XTDC_K_MAX] = 0;
  XtFree( buf );

  strncpy( axtdo->curValue, axtdo->entryValue, XTDC_K_MAX );
  axtdo->curValue[XTDC_K_MAX] = 0;
  strncpy( string, axtdo->entryValue, XTDC_K_MAX );
  string[XTDC_K_MAX] = 0;
  if ( axtdo->pvExists ) {
#ifdef __epics__
    stat = stringPut( axtdo->pvId, axtdo->pvCount, string );
    //stat = ca_put( DBR_STRING, axtdo->pvId, string );
#endif
  }
  else {
    axtdo->needUpdate = 1;
    axtdo->actWin->appCtx->proc->lock();
    axtdo->actWin->addDefExeNode( axtdo->aglPtr );
    axtdo->actWin->appCtx->proc->unlock();
  }

}

static void xtdoTextFieldToIntA (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int ivalue, stat;
char *buf, tmp[XTDC_K_MAX+1];

  buf = XmTextGetString( axtdo->tf_widget );
  strncpy( axtdo->entryValue, buf, XTDC_K_MAX );
  axtdo->entryValue[XTDC_K_MAX] = 0;
  XtFree( buf );

  if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {
    if ( strlen( axtdo->entryValue ) > 2 ) {
      if ( ( strncmp( axtdo->entryValue, "0x", 2 ) != 0 ) &&
           ( strncmp( axtdo->entryValue, "0X", 2 ) != 0 ) ) {
        strcpy( tmp, "0x" );
      }
      else {
        strcpy( tmp, "" );
      }
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }
    else {
      strcpy( tmp, "0x" );
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }
  }
  else {
    strncpy( tmp, axtdo->entryValue, XTDC_K_MAX );
    tmp[XTDC_K_MAX] = 0;
  }

  if ( isLegalInteger(tmp) ) {

    strncpy( axtdo->curValue, tmp, XTDC_K_MAX );
    axtdo->curValue[XTDC_K_MAX] = 0;

    ivalue = strtol( tmp, NULL, 0 );
    if ( axtdo->pvExists ) {
#ifdef __epics__
      stat = ca_put( DBR_LONG, axtdo->pvId, &ivalue );
#endif
    }
    else {
      axtdo->needUpdate = 1;
      axtdo->actWin->appCtx->proc->lock();
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      axtdo->actWin->appCtx->proc->unlock();
    }

    //XmTextSetInsertionPosition( axtdo->tf_widget, 0 );

  }

  axtdo->grabUpdate = 0;

}

static void xtdoTextFieldToIntLF (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int ivalue, stat;
char *buf;
Arg args[10];
int n;
char tmp[XTDC_K_MAX+1];

  n = 0;
  XtSetArg( args[n], XmNcursorPositionVisible, (XtArgVal) False ); n++;
  XtSetValues( axtdo->tf_widget, args, n );

  //XmTextSetInsertionPosition( axtdo->tf_widget, 0 );

  axtdo->grabUpdate = 0;

  if ( !axtdo->widget_value_changed ) return;

  buf = XmTextGetString( axtdo->tf_widget );
  strncpy( axtdo->entryValue, buf, XTDC_K_MAX );
  axtdo->entryValue[XTDC_K_MAX] = 0;
  XtFree( buf );

  if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {
    if ( strlen( axtdo->entryValue ) > 2 ) {
      if ( ( strncmp( axtdo->entryValue, "0x", 2 ) != 0 ) &&
           ( strncmp( axtdo->entryValue, "0X", 2 ) != 0 ) ) {
        strcpy( tmp, "0x" );
      }
      else {
        strcpy( tmp, "" );
      }
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }
    else {
      strcpy( tmp, "0x" );
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }
  }
  else {
    strncpy( tmp, axtdo->entryValue, XTDC_K_MAX );
    tmp[XTDC_K_MAX] = 0;
  }

  if ( isLegalInteger(tmp) ) {

    strncpy( axtdo->curValue, tmp, XTDC_K_MAX );
    axtdo->curValue[XTDC_K_MAX] = 0;

    ivalue = strtol( tmp, NULL, 0 );
    if ( axtdo->pvExists ) {
#ifdef __epics__
      stat = ca_put( DBR_LONG, axtdo->pvId, &ivalue );
#endif
    }
    else {
      axtdo->needUpdate = 1;
      axtdo->actWin->appCtx->proc->lock();
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      axtdo->actWin->appCtx->proc->unlock();
    }

  }

}

static void xtdoTextFieldToDoubleA (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat, ivalue, doPut;
double dvalue;
char *buf, tmp[XTDC_K_MAX+1];

  buf = XmTextGetString( axtdo->tf_widget );
  strncpy( axtdo->entryValue, buf, XTDC_K_MAX );
  axtdo->entryValue[XTDC_K_MAX] = 0;
  XtFree( buf );

  doPut = 0;

  if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {

    if ( strlen( axtdo->entryValue ) > 2 ) {
      if ( ( strncmp( axtdo->entryValue, "0x", 2 ) != 0 ) &&
           ( strncmp( axtdo->entryValue, "0X", 2 ) != 0 ) ) {
        strcpy( tmp, "0x" );
      }
      else {
        strcpy( tmp, "" );
      }
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }
    else {
      strcpy( tmp, "0x" );
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }

    if ( isLegalInteger(tmp) ) {
      doPut = 1;
      ivalue = strtol( tmp, NULL, 0 );
      dvalue = (double) ivalue;
    }

  }
  else {

    strncpy( tmp, axtdo->entryValue, XTDC_K_MAX );
    tmp[XTDC_K_MAX] = 0;

    if ( isLegalFloat(tmp) ) {
      doPut = 1;
      dvalue = atof( tmp );
    }

  }

  if ( doPut ) {

    strncpy( axtdo->curValue, tmp, XTDC_K_MAX );
    axtdo->curValue[XTDC_K_MAX] = 0;

    if ( axtdo->pvExists ) {
#ifdef __epics__
      stat = ca_put( DBR_DOUBLE, axtdo->pvId, &dvalue );
#endif
    }
    else {
      axtdo->needUpdate = 1;
      axtdo->actWin->appCtx->proc->lock();
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      axtdo->actWin->appCtx->proc->unlock();
    }

    //XmTextSetInsertionPosition( axtdo->tf_widget, 0 );

  }

  axtdo->grabUpdate = 0;

}

static void xtdoTextFieldToDoubleLF (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
int stat, ivalue, doPut;
double dvalue;
char *buf, tmp[XTDC_K_MAX+1];
Arg args[10];
int n;

  n = 0;
  XtSetArg( args[n], XmNcursorPositionVisible, (XtArgVal) False ); n++;
  XtSetValues( axtdo->tf_widget, args, n );

  //XmTextSetInsertionPosition( axtdo->tf_widget, 0 );

  axtdo->grabUpdate = 0;

  if ( !axtdo->widget_value_changed ) return;

  buf = XmTextGetString( axtdo->tf_widget );
  strncpy( axtdo->entryValue, buf, XTDC_K_MAX );
  axtdo->entryValue[XTDC_K_MAX] = 0;
  XtFree( buf );

  doPut = 0;

  if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {

    if ( strlen( axtdo->entryValue ) > 2 ) {
      if ( ( strncmp( axtdo->entryValue, "0x", 2 ) != 0 ) &&
           ( strncmp( axtdo->entryValue, "0X", 2 ) != 0 ) ) {
        strcpy( tmp, "0x" );
      }
      else {
        strcpy( tmp, "" );
      }
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }
    else {
      strcpy( tmp, "0x" );
      Strncat( tmp, axtdo->entryValue, 15 );
      tmp[15] = 0;
    }

    if ( isLegalInteger(tmp) ) {
      doPut = 1;
      ivalue = strtol( tmp, NULL, 0 );
      dvalue = (double) ivalue;
    }

  }
  else {

    strncpy( tmp, axtdo->entryValue, XTDC_K_MAX );
    tmp[XTDC_K_MAX] = 0;

    if ( isLegalFloat(tmp) ) {
      doPut = 1;
      dvalue = atof( tmp );
    }

  }

  if ( doPut ) {

    strncpy( axtdo->curValue, tmp, XTDC_K_MAX );
    axtdo->curValue[XTDC_K_MAX] = 0;

    if ( axtdo->pvExists ) {
#ifdef __epics__
      stat = ca_put( DBR_DOUBLE, axtdo->pvId, &dvalue );
#endif
    }
    else {
      axtdo->needUpdate = 1;
      axtdo->actWin->appCtx->proc->lock();
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      axtdo->actWin->appCtx->proc->unlock();
    }

  }

}

#ifdef __epics__

static void xtdo_monitor_connect_state (
  struct connection_handler_args arg )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) ca_puser(arg.chid);

  axtdo->actWin->appCtx->proc->lock();

  if ( axtdo->activeMode ) {

    if ( arg.op == CA_OP_CONN_UP ) {

      axtdo->pvType = (int) ca_field_type( axtdo->pvId );
      axtdo->pvCount = (int) ca_element_count( axtdo->pvId );

      if ( axtdo->pvType == DBR_CHAR ) {
        if ( axtdo->pvCount == 1 ) {
          axtdo->pvType = DBR_LONG;
	}
        else {
          axtdo->pvType = DBR_STRING;
	}
      }

      // if format is hex, force double/float type to long
      if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {
        if ( ( axtdo->pvType == DBR_DOUBLE ) ||
             ( axtdo->pvType == DBR_FLOAT ) ) {
          axtdo->pvType = DBR_LONG;
	}
      }

      axtdo->connection.setPvConnected( axtdo->pvId );

      if ( axtdo->connection.pvsConnected() ) {
        axtdo->needConnectInit = 1;
        axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      }

    }
    else {

      axtdo->connection.setPvDisconnected( axtdo->pvId );
      axtdo->fgColor.setDisconnected();
      axtdo->needRefresh = 1;
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );

    }

  }

  axtdo->actWin->appCtx->proc->unlock();

}

static void xtdo_monitor_sval_connect_state (
  struct connection_handler_args arg )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) ca_puser(arg.chid);

  axtdo->actWin->appCtx->proc->lock();

  if ( axtdo->activeMode ) {

    if ( arg.op == CA_OP_CONN_UP ) {

      axtdo->svalPvType = (int) ca_field_type( axtdo->svalPvId );
      axtdo->svalPvCount = (int) ca_element_count( axtdo->pvId );

      if ( axtdo->svalPvType == DBR_CHAR ) {
        if ( axtdo->svalPvCount == 1 ) {
          axtdo->svalPvType = DBR_LONG;
	}
        else {
          axtdo->svalPvType = DBR_STRING;
	}
      }

      axtdo->connection.setPvConnected( axtdo->svalPvId );

      if ( axtdo->connection.pvsConnected() ) {
        axtdo->needConnectInit = 1;
        axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      }

    }
    else {

      axtdo->connection.setPvDisconnected( axtdo->svalPvId );
      axtdo->fgColor.setDisconnected();
      axtdo->needRefresh = 1;
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );

    }

  }

  axtdo->actWin->appCtx->proc->unlock();

}

static void xtdo_monitor_fg_connect_state (
  struct connection_handler_args arg )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) ca_puser(arg.chid);

  axtdo->actWin->appCtx->proc->lock();

  if ( axtdo->activeMode ) {

    if ( arg.op == CA_OP_CONN_UP ) {

      axtdo->connection.setPvConnected( axtdo->fgPvId );

      if ( axtdo->connection.pvsConnected() ) {
        axtdo->needConnectInit = 1;
        axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      }

    }
    else {

      axtdo->connection.setPvDisconnected( axtdo->fgPvId );
      axtdo->fgColor.setDisconnected();
      axtdo->needRefresh = 1;
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );

    }

  }

  axtdo->actWin->appCtx->proc->unlock();

}

static void XtextDspUpdate (
  struct event_handler_args ast_args
) {

class activeXTextDspClass *axtdo;
int ivalue, n, i;
short svalue;

  axtdo = (activeXTextDspClass *) ast_args.usr;

  axtdo->actWin->appCtx->proc->lock();

  if ( axtdo->activeMode ) {

    switch ( axtdo->pvType ) {

    case DBR_STRING:

      if ( axtdo->pvCount == 1 ) {

        strncpy( axtdo->curValue, (char *) ast_args.dbr, XTDC_K_MAX );
        axtdo->curValue[XTDC_K_MAX] = 0;

      }
      else {

	n = axtdo->pvCount;
	if ( n > XTDC_K_MAX ) n = XTDC_K_MAX;
	for ( i=0; i<n; i++ ) {
	  ( (char *) axtdo->curValue )[i] = ( (char *) ast_args.dbr )[i];
	}
	( (char *) axtdo->curValue )[i] = 0;

      }

      break;

    case DBR_FLOAT:
      axtdo->curDoubleValue = (double) *( (float *) ast_args.dbr );
      sprintf( axtdo->curValue, axtdo->format, axtdo->curDoubleValue );
      if ( !axtdo->noSval ) {
        if ( axtdo->nullDetectMode == 0 ) {
          if ( axtdo->curDoubleValue == axtdo->curSvalValue ) {
            axtdo->fgColor.setNull();
            axtdo->bufInvalidate();
	  }
	  else {
            axtdo->fgColor.setNotNull();
            axtdo->bufInvalidate();
	  }
	}
	else if ( axtdo->nullDetectMode == 1 ) {
          if ( axtdo->curSvalValue == 0 ) {
            axtdo->fgColor.setNull();
            axtdo->bufInvalidate();
	  }
	  else {
            axtdo->fgColor.setNotNull();
            axtdo->bufInvalidate();
	  }
	}
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      break;

    case DBR_DOUBLE:
      axtdo->curDoubleValue = *( (double *) ast_args.dbr );
      sprintf( axtdo->curValue, axtdo->format, axtdo->curDoubleValue );
      if ( !axtdo->noSval ) {
        if ( axtdo->nullDetectMode == 0 ) {
          if ( axtdo->curDoubleValue == axtdo->curSvalValue ) {
            axtdo->fgColor.setNull();
            axtdo->bufInvalidate();
	  }
	  else {
            axtdo->fgColor.setNotNull();
            axtdo->bufInvalidate();
	  }
	}
	else if ( axtdo->nullDetectMode == 1 ) {
          if ( axtdo->curSvalValue == 0 ) {
            axtdo->fgColor.setNull();
            axtdo->bufInvalidate();
	  }
	  else {
            axtdo->fgColor.setNotNull();
            axtdo->bufInvalidate();
	  }
	}
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      break;

    case DBR_SHORT:
      ivalue = (int) *( (short *) ast_args.dbr );
      sprintf( axtdo->curValue, axtdo->format, ivalue );
      break;

    case DBR_LONG:
      ivalue = *( (int *) ast_args.dbr );
      sprintf( axtdo->curValue, axtdo->format, ivalue );
      break;

    case DBR_ENUM:
      svalue = *( (short *) ast_args.dbr );
      if ( ( svalue >= 0 ) && ( svalue < axtdo->numStates ) ) {
        strncpy( axtdo->curValue, axtdo->stateString[svalue], XTDC_K_MAX );
	axtdo->curValue[XTDC_K_MAX] = 0;
        axtdo->entryState = (int) svalue;
      }
      else {
        strcpy( axtdo->curValue, "" );
      }

      break;

    default:
      strcpy( axtdo->curValue, "" );
      break;

    } // end switch

    if ( !blank( axtdo->curValue ) ) {
      if ( axtdo->showUnits && !blank( axtdo->units ) ) {
        Strncat( axtdo->curValue, " ", XTDC_K_MAX );
        Strncat( axtdo->curValue, axtdo->units, XTDC_K_MAX );
      }
    }

    axtdo->needUpdate = 1;
    axtdo->actWin->addDefExeNode( axtdo->aglPtr );

  }

  axtdo->actWin->appCtx->proc->unlock();

}

static void XtextDspSvalUpdate (
  struct event_handler_args ast_args
) {

class activeXTextDspClass *axtdo;

  axtdo = (activeXTextDspClass *) ast_args.usr;

  axtdo->actWin->appCtx->proc->lock();

  if ( axtdo->activeMode ) {

    switch ( axtdo->svalPvType ) {

    case DBR_LONG:

      axtdo->curSvalValue = (double) *( (int *) ast_args.dbr );

      if ( axtdo->nullDetectMode == 0 ) {
        if ( axtdo->curDoubleValue == axtdo->curSvalValue ) {
          axtdo->fgColor.setNull();
        }
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      else if ( axtdo->nullDetectMode == 1 ) {
        if ( axtdo->curSvalValue == 0 ) {
          axtdo->fgColor.setNull();
        }
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      else {
        axtdo->fgColor.setNotNull();
      }

      break;

    case DBR_FLOAT:

      axtdo->curSvalValue = (double) *( (float *) ast_args.dbr );

      if ( axtdo->nullDetectMode == 0 ) {
        if ( axtdo->curDoubleValue == axtdo->curSvalValue ) {
          axtdo->fgColor.setNull();
        }
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      else if ( axtdo->nullDetectMode == 1 ) {
        if ( axtdo->curSvalValue == 0 ) {
          axtdo->fgColor.setNull();
        }
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      else {
        axtdo->fgColor.setNotNull();
      }

      break;

    case DBR_DOUBLE:

      axtdo->curSvalValue = *( (double *) ast_args.dbr );

      if ( axtdo->nullDetectMode == 0 ) {
        if ( axtdo->curDoubleValue == axtdo->curSvalValue ) {
          axtdo->fgColor.setNull();
        }
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      else if ( axtdo->nullDetectMode == 1 ) {
        if ( axtdo->curSvalValue == 0 ) {
          axtdo->fgColor.setNull();
        }
        else {
          axtdo->fgColor.setNotNull();
        }
      }
      else {
        axtdo->fgColor.setNotNull();
      }

      break;

    default:
      break;

    } // end switch

    axtdo->noSval = 0;
    axtdo->bufInvalidate();
    axtdo->needRefresh = 1;
    axtdo->actWin->addDefExeNode( axtdo->aglPtr );

  }

  axtdo->actWin->appCtx->proc->unlock();

}

static void XtextDspFgUpdate (
  struct event_handler_args ast_args
) {

class activeXTextDspClass *axtdo;
double val;
int index;

  axtdo = (activeXTextDspClass *) ast_args.usr;

  axtdo->actWin->appCtx->proc->lock();

  if ( axtdo->activeMode ) {

    val = *( (double *) ast_args.dbr );
    index = axtdo->actWin->ci->evalRule( axtdo->fgColor.pixelIndex(), val );
    axtdo->fgColor.changeIndex( index, axtdo->actWin->ci );
    axtdo->bufInvalidate();
    axtdo->needRefresh = 1;
    axtdo->actWin->addDefExeNode( axtdo->aglPtr );

  }

  axtdo->actWin->appCtx->proc->unlock();

}

static void XtextAlarmUpdate (
  struct event_handler_args ast_args )
{

class activeXTextDspClass *axtdo;
struct dbr_sts_float floatStatusRec;
struct dbr_sts_double doubleStatusRec;
struct dbr_sts_short shortStatusRec;
struct dbr_sts_long longStatusRec;
struct dbr_sts_string stringStatusRec;
struct dbr_sts_enum enumStatusRec;
short svalue;

  axtdo = (activeXTextDspClass *) ast_args.usr;

  switch ( axtdo->pvType ) {

  case DBR_STRING:
    stringStatusRec = *( (struct dbr_sts_string *) ast_args.dbr );
    axtdo->fgColor.setStatus( stringStatusRec.status,
     stringStatusRec.severity );
    strncpy( axtdo->curValue, stringStatusRec.value, XTDC_K_MAX );
    axtdo->curValue[XTDC_K_MAX] = 0;
    break;

  case DBR_FLOAT:
    floatStatusRec = *( (struct dbr_sts_float *) ast_args.dbr );
    axtdo->fgColor.setStatus( floatStatusRec.status,
     floatStatusRec.severity );
    sprintf( axtdo->curValue, axtdo->format, floatStatusRec.value );
    break;

  case DBR_DOUBLE:
    doubleStatusRec = *( (struct dbr_sts_double *) ast_args.dbr );
    axtdo->fgColor.setStatus( doubleStatusRec.status,
     doubleStatusRec.severity );
    sprintf( axtdo->curValue, axtdo->format, doubleStatusRec.value );
    break;

  case DBR_SHORT:
    shortStatusRec = *( (struct dbr_sts_short *) ast_args.dbr );
    axtdo->fgColor.setStatus( shortStatusRec.status, shortStatusRec.severity );
    sprintf( axtdo->curValue, axtdo->format, shortStatusRec.value );
    break;

  case DBR_LONG:
    longStatusRec = *( (struct dbr_sts_long *) ast_args.dbr );
    axtdo->fgColor.setStatus( longStatusRec.status, longStatusRec.severity );
    sprintf( axtdo->curValue, axtdo->format, longStatusRec.value );
    break;

  case DBR_ENUM:
    enumStatusRec = *( (struct dbr_sts_enum *) ast_args.dbr );
    axtdo->fgColor.setStatus( enumStatusRec.status, enumStatusRec.severity );
    svalue = enumStatusRec.value;
    if ( ( svalue >= 0 ) && ( svalue < axtdo->numStates ) ) {
      strncpy( axtdo->curValue, axtdo->stateString[svalue], XTDC_K_MAX );
      axtdo->curValue[XTDC_K_MAX] = 0;
      axtdo->entryState = (int) svalue;
    }
    else {
      strcpy( axtdo->curValue, "" );
    }
    break;

  default:
    strcpy( axtdo->curValue, "" );
    break;

  } // end switch

  if ( !blank( axtdo->curValue ) ) {
    if ( axtdo->showUnits && !blank( axtdo->units ) ) {
      Strncat( axtdo->curValue, " ", XTDC_K_MAX );
      Strncat( axtdo->curValue, axtdo->units, XTDC_K_MAX );
    }
  }

  axtdo->needRefresh = 1;
  axtdo->actWin->appCtx->proc->lock();
  axtdo->actWin->addDefExeNode( axtdo->aglPtr );
  axtdo->actWin->appCtx->proc->unlock();

}

static void XtextDspInfoUpdate (
  struct event_handler_args ast_args )
{

int i, n;
short svalue;
class activeXTextDspClass *axtdo = (activeXTextDspClass *) ast_args.usr;
struct dbr_gr_float floatInfoRec;
struct dbr_gr_double doubleInfoRec;
struct dbr_sts_string stringInfoRec;
struct dbr_gr_short shortInfoRec;
struct dbr_gr_long longInfoRec;
struct dbr_gr_enum enumInfoRec;

  if ( ast_args.status == ECA_DISCONN ) {
    return;
  }

  switch ( axtdo->pvType ) {

  case DBR_STRING:

    if ( axtdo->pvCount == 1 ) {

      stringInfoRec = *( (dbr_sts_string *) ast_args.dbr );

      strcpy( axtdo->units, "" );
      axtdo->showUnits = 0;

      axtdo->fgColor.setStatus( stringInfoRec.status, stringInfoRec.severity );

    }
    else {

      n = axtdo->pvCount;
      if ( n > XTDC_K_MAX ) n = XTDC_K_MAX;
      for ( i=0; i<n; i++ ) {
        ( (char *) axtdo->curValue )[i] = ( (char *) ast_args.dbr )[i];
      }
      ( (char *) axtdo->curValue )[i] = 0;

    }

    break;

  case DBR_FLOAT:

    floatInfoRec = *( (dbr_gr_float *) ast_args.dbr );

    strncpy( axtdo->units, floatInfoRec.units, MAX_UNITS_SIZE );
    axtdo->units[MAX_UNITS_SIZE] = 0;

    if ( axtdo->limitsFromDb || axtdo->efPrecision.isNull() ) {
      axtdo->precision = floatInfoRec.precision;
    }

    axtdo->fgColor.setStatus( floatInfoRec.status, floatInfoRec.severity );

    axtdo->isDate = 0;

    axtdo->isFile = 0;

    break;

  case DBR_DOUBLE:

    doubleInfoRec = *( (dbr_gr_double *) ast_args.dbr );

    strncpy( axtdo->units, doubleInfoRec.units, MAX_UNITS_SIZE );
    axtdo->units[MAX_UNITS_SIZE] = 0;

    if ( axtdo->limitsFromDb || axtdo->efPrecision.isNull() ) {
      axtdo->precision = doubleInfoRec.precision;
    }

    axtdo->fgColor.setStatus( doubleInfoRec.status, doubleInfoRec.severity );

    axtdo->isDate = 0;

    axtdo->isFile = 0;

    break;

  case DBR_SHORT:

    shortInfoRec = *( (dbr_gr_short *) ast_args.dbr );

    strncpy( axtdo->units, shortInfoRec.units, MAX_UNITS_SIZE );
    axtdo->units[MAX_UNITS_SIZE] = 0;

    axtdo->fgColor.setStatus( shortInfoRec.status, shortInfoRec.severity );

    axtdo->isDate = 0;

    axtdo->isFile = 0;

    break;

  case DBR_LONG:

    longInfoRec = *( (dbr_gr_long *) ast_args.dbr );

    strncpy( axtdo->units, longInfoRec.units, MAX_UNITS_SIZE );
    axtdo->units[MAX_UNITS_SIZE] = 0;

    axtdo->fgColor.setStatus( longInfoRec.status, longInfoRec.severity );

    axtdo->isDate = 0;

    axtdo->isFile = 0;

    break;

  case DBR_ENUM:

    enumInfoRec = *( (dbr_gr_enum *) ast_args.dbr );

    strcpy( axtdo->units, "" );
    axtdo->showUnits = 0;

    n = enumInfoRec.no_str;

    for ( i=0; i<n; i++ ) {

      if ( axtdo->stateString[i] == NULL ) {
        axtdo->stateString[i] = new char[MAX_ENUM_STRING_SIZE+1];
      }

      strncpy( axtdo->stateString[i], enumInfoRec.strs[i],
       MAX_ENUM_STRING_SIZE );
      axtdo->stateString[i][MAX_ENUM_STRING_SIZE] = 0;

    }

    axtdo->numStates = n;

    svalue = enumInfoRec.value;
    if ( ( svalue >= 0 ) && ( svalue < axtdo->numStates ) ) {
      strncpy( axtdo->value, axtdo->stateString[svalue], XTDC_K_MAX );
      axtdo->value[XTDC_K_MAX] = 0;
      axtdo->entryState = (int) svalue;
    }
    else {
      strcpy( axtdo->value, "" );
    }

    strncpy( axtdo->curValue, axtdo->value, XTDC_K_MAX );
    axtdo->curValue[XTDC_K_MAX] = 0;

    axtdo->isWidget = 0;

    axtdo->fgColor.setStatus( enumInfoRec.status, enumInfoRec.severity );

    axtdo->isDate = 0;

    axtdo->isFile = 0;

    break;

  } // end switch ( pvType )

  axtdo->needInfoInit = 1;
  axtdo->actWin->appCtx->proc->lock();
  axtdo->actWin->addDefExeNode( axtdo->aglPtr );
  axtdo->actWin->appCtx->proc->unlock();

}

#endif

static void axtdc_value_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;
double dvalue;
int ivalue, stat, doPut;
short svalue;
char string[XTDC_K_MAX+1], tmp[XTDC_K_MAX+1];

  strncpy( axtdo->curValue, axtdo->entryValue, XTDC_K_MAX );
  axtdo->curValue[XTDC_K_MAX] = 0;

  switch ( axtdo->pvType ) {

  case DBR_FLOAT:
  case DBR_DOUBLE:

    doPut = 0;

    if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {

      if ( strlen( axtdo->entryValue ) > 2 ) {
        if ( ( strncmp( axtdo->entryValue, "0x", 2 ) != 0 ) &&
             ( strncmp( axtdo->entryValue, "0X", 2 ) != 0 ) ) {
          strcpy( tmp, "0x" );
        }
        else {
          strcpy( tmp, "" );
        }
        Strncat( tmp, axtdo->entryValue, 15 );
        tmp[15] = 0;
      }
      else {
        strcpy( tmp, "0x" );
        Strncat( tmp, axtdo->entryValue, 15 );
        tmp[15] = 0;
      }

      if ( isLegalInteger(tmp) ) {
        doPut = 1;
        ivalue = strtol( tmp, NULL, 0 );
        dvalue = (double) ivalue;
      }

    }
    else {

      if ( isLegalFloat(axtdo->entryValue) ) {
        doPut = 1;
        dvalue = atof( axtdo->entryValue );
      }

    }

    if ( doPut ) {
      if ( axtdo->pvExists ) {
#ifdef __epics__
        stat = ca_put( DBR_DOUBLE, axtdo->pvId, &dvalue );
#endif
      }
      else {
        axtdo->needUpdate = 1;
        axtdo->actWin->appCtx->proc->lock();
        axtdo->actWin->addDefExeNode( axtdo->aglPtr );
        axtdo->actWin->appCtx->proc->unlock();
      }

    }
    break;

  case DBR_SHORT:
  case DBR_LONG:

    if ( axtdo->formatType == XTDC_K_FORMAT_HEX ) {
      if ( strlen( axtdo->entryValue ) > 2 ) {
        if ( ( strncmp( axtdo->entryValue, "0x", 2 ) != 0 ) &&
             ( strncmp( axtdo->entryValue, "0X", 2 ) != 0 ) ) {
          strcpy( tmp, "0x" );
        }
        else {
          strcpy( tmp, "" );
        }
        Strncat( tmp, axtdo->entryValue, 15 );
        tmp[15] = 0;
      }
      else {
        strcpy( tmp, "0x" );
        Strncat( tmp, axtdo->entryValue, 15 );
        tmp[15] = 0;
      }
    }
    else {
      strncpy( tmp, axtdo->entryValue, XTDC_K_MAX );
      tmp[XTDC_K_MAX] = 0;
    }

    if ( isLegalInteger(tmp) ) {
      ivalue = strtol( tmp, NULL, 0 );
      if ( axtdo->pvExists ) {
#ifdef __epics__
        stat = ca_put( DBR_LONG, axtdo->pvId, &ivalue );
#endif
      }
      else {
        axtdo->needUpdate = 1;
        axtdo->actWin->appCtx->proc->lock();
        axtdo->actWin->addDefExeNode( axtdo->aglPtr );
        axtdo->actWin->appCtx->proc->unlock();
      }

    }
    break;

  case DBR_STRING:
    strncpy( string, axtdo->entryValue, XTDC_K_MAX );
    string[XTDC_K_MAX] = 0;
    if ( axtdo->pvExists ) {
#ifdef __epics__
      stat = stringPut( axtdo->pvId, axtdo->pvCount, string );
      //stat = ca_put( DBR_STRING, axtdo->pvId, string );
#endif
    }
    else {
      axtdo->needUpdate = 1;
      axtdo->actWin->appCtx->proc->lock();
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      axtdo->actWin->appCtx->proc->unlock();
    }

    break;

  case DBR_ENUM:
    svalue = (short) axtdo->entryState;
    if ( axtdo->pvExists ) {
#ifdef __epics__
      stat = ca_put( DBR_ENUM, axtdo->pvId, &svalue );
#endif
    }
    else {
      axtdo->needUpdate = 1;
      axtdo->actWin->appCtx->proc->lock();
      axtdo->actWin->addDefExeNode( axtdo->aglPtr );
      axtdo->actWin->appCtx->proc->unlock();
    }

    break;

  }

}

static void axtdc_value_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdc_value_edit_apply ( w, client, call );
  axtdo->textEntry.popdown();
  axtdo->editDialogIsActive = 0;

}

static void axtdc_value_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->textEntry.popdown();
  axtdo->editDialogIsActive = 0;

}

static void axtdc_edit_update (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->actWin->setChanged();

  axtdo->eraseSelectBoxCorners();
  axtdo->erase();

  strncpy( axtdo->value, axtdo->bufPvName, 39 );
  axtdo->value[39] = 0;
  strncpy( axtdo->curValue, axtdo->bufPvName, 39 );
  axtdo->curValue[39] = 0;

  strncpy( axtdo->pvName, axtdo->bufPvName, activeGraphicClass::MAX_PV_NAME );
  axtdo->pvName[activeGraphicClass::MAX_PV_NAME] = 0;
  axtdo->pvExpStr.setRaw( axtdo->pvName );

  axtdo->svalPvExpStr.setRaw( axtdo->bufSvalPvName );

  axtdo->fgPvExpStr.setRaw( axtdo->bufColorPvName );

  axtdo->defDir.setRaw( axtdo->bufDefDir );
  axtdo->pattern.setRaw( axtdo->bufPattern );

  strncpy( axtdo->fontTag, axtdo->fm.currentFontTag(), 63 );
  axtdo->fontTag[63] = 0;
  axtdo->actWin->fi->loadFontTag( axtdo->fontTag );
  axtdo->actWin->drawGc.setFontTag( axtdo->fontTag, axtdo->actWin->fi );

  axtdo->stringLength = strlen( axtdo->curValue );

  axtdo->fs = axtdo->actWin->fi->getXFontStruct( axtdo->fontTag );

  axtdo->updateFont( axtdo->curValue, axtdo->fontTag, &axtdo->fs,
   &axtdo->fontAscent, &axtdo->fontDescent, &axtdo->fontHeight,
   &axtdo->stringWidth );

  axtdo->useDisplayBg = axtdo->bufUseDisplayBg;

  axtdo->autoHeight = axtdo->bufAutoHeight;

  axtdo->formatType = axtdo->bufFormatType;

  axtdo->limitsFromDb = axtdo->bufLimitsFromDb;

  axtdo->changeValOnLoseFocus = axtdo->bufChangeValOnLoseFocus;

  axtdo->fastUpdate = axtdo->bufFastUpdate;

  axtdo->efPrecision = axtdo->bufEfPrecision;

  if ( axtdo->efPrecision.isNull() )
    axtdo->precision = 2;
  else
    axtdo->precision = axtdo->efPrecision.value();

  axtdo->fgColor.setConnectSensitive();

  axtdo->colorMode = axtdo->bufColorMode;

  axtdo->editable = axtdo->bufEditable;

  axtdo->isWidget = axtdo->bufIsWidget;

  axtdo->isDate = axtdo->bufIsDate;

  axtdo->isFile = axtdo->bufIsFile;

  axtdo->useKp = axtdo->bufUseKp;

  if ( axtdo->colorMode == XTDC_K_COLORMODE_ALARM )
    axtdo->fgColor.setAlarmSensitive();
  else
    axtdo->fgColor.setAlarmInsensitive();

  axtdo->fgColor.setColorIndex( axtdo->bufFgColor, axtdo->actWin->ci );
  axtdo->fgColor.setNullIndex ( axtdo->bufSvalColor, axtdo->actWin->ci );
  axtdo->bgColor = axtdo->bufBgColor;

  axtdo->nullDetectMode = axtdo->bufNullDetectMode;

  axtdo->smartRefresh = axtdo->bufSmartRefresh;

  axtdo->autoSelect = axtdo->bufAutoSelect;

  axtdo->updatePvOnDrop = axtdo->bufUpdatePvOnDrop;

  axtdo->useHexPrefix = axtdo->bufUseHexPrefix;

  axtdo->fileComponent = axtdo->bufFileComponent;

  axtdo->dateAsFileName = axtdo->bufDateAsFileName;

  axtdo->showUnits = axtdo->bufShowUnits;
  if ( axtdo->editable ) {
    axtdo->showUnits = 0;
  }

  axtdo->useAlarmBorder = axtdo->bufUseAlarmBorder;

  strncpy( axtdo->id, axtdo->bufId, 31 );
  axtdo->id[31] = 0;
  axtdo->changeCallbackFlag = axtdo->bufChangeCallbackFlag;
  axtdo->activateCallbackFlag = axtdo->bufActivateCallbackFlag;
  axtdo->deactivateCallbackFlag = axtdo->bufDeactivateCallbackFlag;
  axtdo->anyCallbackFlag = axtdo->changeCallbackFlag ||
   axtdo->activateCallbackFlag || axtdo->deactivateCallbackFlag;

  axtdo->x = axtdo->bufX;
  axtdo->sboxX = axtdo->bufX;

  axtdo->y = axtdo->bufY;
  axtdo->sboxY = axtdo->bufY;

  axtdo->w = axtdo->bufW;
  axtdo->sboxW = axtdo->bufW;

  axtdo->h = axtdo->bufH;
  axtdo->sboxH = axtdo->bufH;

  axtdo->updateDimensions();

  if ( axtdo->autoHeight && axtdo->fs ) {
    axtdo->h = axtdo->fontHeight;
    axtdo->sboxH = axtdo->h;
  }

  axtdo->stringY = axtdo->y + axtdo->fontAscent + axtdo->h/2 -
   axtdo->fontHeight/2;

  axtdo->alignment = axtdo->fm.currentFontAlignment();

  if ( axtdo->alignment == XmALIGNMENT_BEGINNING )
    axtdo->stringX = axtdo->x;
  else if ( axtdo->alignment == XmALIGNMENT_CENTER )
    axtdo->stringX = axtdo->x + axtdo->w/2 - axtdo->stringWidth/2;
  else if ( axtdo->alignment == XmALIGNMENT_END )
    axtdo->stringX = axtdo->x + axtdo->w - axtdo->stringWidth;

}

static void axtdc_edit_apply (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdc_edit_update( w, client, call );
  axtdo->refresh( axtdo );

}

static void axtdc_edit_ok (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdc_edit_update( w, client, call );
  axtdo->ef.popdown();
  axtdo->operationComplete();

}

static void axtdc_edit_cancel (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->ef.popdown();
  axtdo->operationCancel();

}

static void axtdc_edit_cancel_delete (
  Widget w,
  XtPointer client,
  XtPointer call )
{

activeXTextDspClass *axtdo = (activeXTextDspClass *) client;

  axtdo->ef.popdown();
  axtdo->operationCancel();
  axtdo->erase();
  axtdo->deleteRequest = 1;
  axtdo->drawAll();

}

activeXTextDspClass::activeXTextDspClass ( void ) {

int i;

  name = new char[strlen("activeXTextDspClass")+1];
  strcpy( name, "activeXTextDspClass" );
  strcpy( value, "" );

  editable = 0;
  smartRefresh = 0;
  isWidget = 0;
  useKp = 0;
  isDate = 0;
  isFile = 0;
  fileComponent = XTDC_K_FILE_FULL_PATH;
  dateAsFileName = 0;
  numStates = 0;
  entryState = 0;
  for ( i=0; i<MAX_ENUM_STATES; i++ ) {
    stateString[i] = NULL;
  }
  limitsFromDb = 1;
  changeValOnLoseFocus = 0;
  autoSelect = 0;
  updatePvOnDrop = 0;
  useHexPrefix = 1;
  fastUpdate = 0;

  efPrecision.setNull(1);
  precision = 3;

  activeMode = 0;

  strcpy( id, "" );

  changeCallbackFlag = 0;
  activateCallbackFlag = 0;
  deactivateCallbackFlag = 0;
  anyCallbackFlag = 0;
  changeCallback = NULL;
  activateCallback = NULL;
  deactivateCallback = NULL;

  nullDetectMode = 0;

  showUnits = 0;
  strcpy( units, "" );

  useAlarmBorder = 0;

  prevAlarmSeverity = 0;
  pvCount = svalPvCount = 1;

  connection.setMaxPvs( 3 );

  unconnectedTimer = 0;

  setBlinkFunction( (void *) doBlink );

}

// copy constructor
activeXTextDspClass::activeXTextDspClass
 ( const activeXTextDspClass *source ) {

activeGraphicClass *ago = (activeGraphicClass *) this;
int i;

  ago->clone( (activeGraphicClass *) source );

  name = new char[strlen("activeXTextDspClass")+1];
  strcpy( name, "activeXTextDspClass" );

  numStates = 0;
  entryState = 0;
  for ( i=0; i<MAX_ENUM_STATES; i++ ) {
    stateString[i] = NULL;
  }

  useDisplayBg = source->useDisplayBg;

  autoHeight = source->autoHeight;

  formatType = source->formatType;

  colorMode = source->colorMode;

  editable = source->editable;

  smartRefresh = source->smartRefresh;

  isWidget = source->isWidget;

  useKp = source->useKp;

  isDate = source->isDate;

  isFile = source->isFile;

  fileComponent = source->fileComponent;

  dateAsFileName = source->dateAsFileName;

  bgColor = source->bgColor;

  fgColor.copy(source->fgColor);

  strncpy( fontTag, source->fontTag, 63 );
  fontTag[63] = 0;
  strncpy( bufFontTag, source->bufFontTag, 63 );
  bufFontTag[63] = 0;

  fs = actWin->fi->getXFontStruct( fontTag );

  fgCb = source->fgCb;
  bgCb = source->bgCb;
  svalCb = source->svalCb;

  strncpy( value, source->value, XTDC_K_MAX );
  value[XTDC_K_MAX] = 0;

  alignment = source->alignment;

  stringLength = source->stringLength;
  fontAscent = source->fontAscent;
  fontDescent = source->fontDescent;
  fontHeight = source->fontHeight;
  stringWidth = source->stringWidth;
  stringY = source->stringY;
  stringX = source->stringX;

  strncpy( pvName, source->pvName, activeGraphicClass::MAX_PV_NAME );
  pvName[activeGraphicClass::MAX_PV_NAME] = 0;

  pvExpStr.copy( source->pvExpStr );
  svalPvExpStr.copy( source->svalPvExpStr );
  fgPvExpStr.copy( source->fgPvExpStr );

  defDir.copy( source->defDir );
  pattern.copy( source->pattern );

  limitsFromDb = source->limitsFromDb;
  changeValOnLoseFocus = source->changeValOnLoseFocus;
  autoSelect = source->autoSelect;
  updatePvOnDrop  = source->updatePvOnDrop;
  useHexPrefix = source->useHexPrefix;
  fastUpdate = source->fastUpdate;
  precision = source->precision;
  efPrecision = source->efPrecision;

  showUnits = source->showUnits;
  strcpy( units, "" );

  activeMode = 0;

  strcpy( id, source->id );

  changeCallbackFlag = source->changeCallbackFlag;
  activateCallbackFlag = source->activateCallbackFlag;
  deactivateCallbackFlag = source->deactivateCallbackFlag;
  anyCallbackFlag = changeCallbackFlag ||
   activateCallbackFlag || deactivateCallbackFlag;
  changeCallback = NULL;
  activateCallback = NULL;
  deactivateCallback = NULL;

  nullDetectMode = source->nullDetectMode;

  useAlarmBorder = source->useAlarmBorder;

  prevAlarmSeverity = 0;
  pvCount = svalPvCount = 1;

  connection.setMaxPvs( 3 );

  unconnectedTimer = 0;

  setBlinkFunction( (void *) doBlink );

}

activeXTextDspClass::~activeXTextDspClass ( void ) {

int i;

  if ( name ) delete name;

  if ( unconnectedTimer ) {
    XtRemoveTimeOut( unconnectedTimer );
    unconnectedTimer = 0;
  }

  for ( i=0; i<MAX_ENUM_STATES; i++ ) {
    if ( stateString[i] ) {
      stateString[i] = NULL;
      delete stateString[i];
    }
  }

  updateBlink( 0 );

}

int activeXTextDspClass::createInteractive (
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

  strcpy( value, "" );
  strcpy( pvName, "" );

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  fgColor.setNullIndex( actWin->defaultFg2Color, actWin->ci );
  bgColor = actWin->defaultBgColor;

  useDisplayBg = 1;

  autoHeight = 1;

  formatType = XTDC_K_FORMAT_NATURAL;

  colorMode = XTDC_K_COLORMODE_STATIC;

  editable = 0;
  smartRefresh = 0;
  isWidget = 0;
  useKp = 0;
  isDate = 0;
  isFile = 0;

  strcpy( fontTag, actWin->defaultFontTag );

  actWin->fi->loadFontTag( fontTag );

  fs = actWin->fi->getXFontStruct( fontTag );

  alignment = actWin->defaultAlignment;

  if ( fs ) {
    fontAscent = fs->ascent;
    fontDescent = fs->descent;
    fontHeight = fontAscent + fontDescent;
  }
  else {
    fontAscent = 0;
    fontDescent = 0;
    fontHeight = 0;
  }

  updateDimensions();

  this->draw();

  this->editCreate();

  return 1;

}

int activeXTextDspClass::save (
  FILE *f )
{

int index, stat;

  fprintf( f, "%-d %-d %-d\n", XTDC_MAJOR_VERSION, XTDC_MINOR_VERSION,
   XTDC_RELEASE );

  fprintf( f, "%-d\n", x );
  fprintf( f, "%-d\n", y );
  fprintf( f, "%-d\n", w );
  fprintf( f, "%-d\n", h );
  writeStringToFile( f, pvName );
  writeStringToFile( f, fontTag );
  fprintf( f, "%-d\n", useDisplayBg );
  fprintf( f, "%-d\n", alignment );
  index = fgColor.pixelIndex();
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );
  index = bgColor;
  actWin->ci->writeColorIndex( f, index );
  //fprintf( f, "%-d\n", index );
  fprintf( f, "%-d\n", formatType );
  fprintf( f, "%-d\n", colorMode );
  fprintf( f, "%-d\n", editable );
  fprintf( f, "%-d\n", autoHeight );

  fprintf( f, "%-d\n", isWidget );

  fprintf( f, "%-d\n", limitsFromDb );
  stat = efPrecision.write( f );

  // version 1.5.0
  writeStringToFile( f, id );
  fprintf( f, "%-d\n", changeCallbackFlag );
  fprintf( f, "%-d\n", activateCallbackFlag );
  fprintf( f, "%-d\n", deactivateCallbackFlag );

  // version 1.6.0
  if ( svalPvExpStr.getRaw() )
    writeStringToFile( f, svalPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  // version 1.7.0
  index = fgColor.nullIndex();
  fprintf( f, "%-d\n", index );

  fprintf( f, "%-d\n", nullDetectMode );

  // version 1.8.0
  if ( fgPvExpStr.getRaw() )
    writeStringToFile( f, fgPvExpStr.getRaw() );
  else
    writeStringToFile( f, "" );

  // version 1.9.0
  fprintf( f, "%-d\n", smartRefresh );

  // version 2.1.0
  fprintf( f, "%-d\n", useKp );

  // version 2.2.0
  fprintf( f, "%-d\n", changeValOnLoseFocus );

  // version 2.3.0
  fprintf( f, "%-d\n", fastUpdate );

  // version 2.3.0
  fprintf( f, "%-d\n", isDate );

  fprintf( f, "%-d\n", isFile );

  if ( defDir.getRaw() )
    writeStringToFile( f, defDir.getRaw() );
  else
    writeStringToFile( f, "" );

  if ( pattern.getRaw() )
    writeStringToFile( f, pattern.getRaw() );
  else
    writeStringToFile( f, "" );

  // version 2.5
  fprintf( f, "%-d\n", objType );

  // version 2.6
  fprintf( f, "%-d\n", autoSelect );

  // version 2.8
  fprintf( f, "%-d\n", updatePvOnDrop );

  // version 2.9
  fprintf( f, "%-d\n", useHexPrefix );

  // version 2.10
  fprintf( f, "%-d\n", fileComponent );
  fprintf( f, "%-d\n", dateAsFileName );

  // version 2.11
  fprintf( f, "%-d\n", showUnits );

  // version 2.12
  fprintf( f, "%-d\n", useAlarmBorder );

  return 1;

}

int activeXTextDspClass::createFromFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int r, g, b, index;
int major, minor, release;
int stat = 1;
char oneName[XTDC_K_MAX+1], onePv[activeGraphicClass::MAX_PV_NAME+1];
unsigned int pixel;

  this->actWin = _actWin;

  fscanf( f, "%d %d %d\n", &major, &minor, &release ); actWin->incLine();

  if ( major > XTDC_MAJOR_VERSION ) {
    postIncompatable();
    return 0;
  }

  fscanf( f, "%d\n", &x ); actWin->incLine();
  fscanf( f, "%d\n", &y ); actWin->incLine();
  fscanf( f, "%d\n", &w ); actWin->incLine();
  fscanf( f, "%d\n", &h ); actWin->incLine();

  this->initSelectBox();

  readStringFromFile( pvName, activeGraphicClass::MAX_PV_NAME+1, f );
   actWin->incLine();
  pvExpStr.setRaw( pvName );

  readStringFromFile( fontTag, 63+1, f ); actWin->incLine();
  fscanf( f, "%d\n", &useDisplayBg ); actWin->incLine();
  fscanf( f, "%d\n", &alignment ); actWin->incLine();

  if ( ( major > 2 ) || ( ( major == 2 ) && ( minor > 6 ) ) ) {

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    bufFgColor = index;
    fgColor.setColorIndex( bufFgColor, actWin->ci );

    actWin->ci->readColorIndex( f, &index );
    actWin->incLine(); actWin->incLine();
    bgColor = index;

  }
  else if ( major > 1 ) {

    fscanf( f, "%d\n", &index ); actWin->incLine();
    bufFgColor = index;
    fgColor.setColorIndex( bufFgColor, actWin->ci );

    fscanf( f, "%d\n", &index ); actWin->incLine();
    bgColor = index;

  }
  else {

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    bufFgColor = actWin->ci->pixIndex( pixel );
    fgColor.setColorIndex( bufFgColor, actWin->ci );

    fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
    if ( ( major < 2 ) && ( minor < 2 ) ) {
      r *= 256;
      g *= 256;
      b *= 256;
    }
    actWin->ci->setRGB( r, g, b, &pixel );
    bgColor = actWin->ci->pixIndex( pixel );

  }

  fscanf( f, "%d\n", &formatType ); actWin->incLine();
  fscanf( f, "%d\n", &colorMode ); actWin->incLine();
  fscanf( f, "%d\n", &editable ); actWin->incLine();

  if ( ( major > 1 ) || ( minor > 0 ) ) {
    fscanf( f, "%d\n", &autoHeight ); actWin->incLine();
  }
  else {
    autoHeight = 0;
  }

  if ( ( major > 1 ) || ( minor > 2 ) ) {
    fscanf( f, "%d\n", &isWidget ); actWin->incLine();
  }
  else {
    isWidget = 0;
  }

  if ( ( major > 1 ) || ( minor > 3 ) ) {
    fscanf( f, "%d\n", &limitsFromDb ); actWin->incLine();
    stat = efPrecision.read( f ); actWin->incLine();
    if ( limitsFromDb || efPrecision.isNull() )
      precision = 3;
    else
      precision = efPrecision.value();
  }
  else {
    limitsFromDb = 1;
    precision = 3;
    efPrecision.setValue( 3 );
  }

  if ( ( major > 1 ) || ( minor > 4 ) ) {
    readStringFromFile( this->id, 31+1, f ); actWin->incLine();
    fscanf( f, "%d\n", &changeCallbackFlag ); actWin->incLine();
    fscanf( f, "%d\n", &activateCallbackFlag ); actWin->incLine();
    fscanf( f, "%d\n", &deactivateCallbackFlag ); actWin->incLine();
    anyCallbackFlag = changeCallbackFlag ||
     activateCallbackFlag || deactivateCallbackFlag;
  }
  else {
    strcpy( this->id, "" );
    changeCallbackFlag = 0;
    activateCallbackFlag = 0;
    deactivateCallbackFlag = 0;
    anyCallbackFlag = 0;
  }

  if ( colorMode == XTDC_K_COLORMODE_ALARM )
    fgColor.setAlarmSensitive();
  else
    fgColor.setAlarmInsensitive();

  strncpy( value, pvName, 39 );
  value[39] = 0;

  if ( ( major > 1 ) || ( minor > 5 ) ) {

    readStringFromFile( onePv, activeGraphicClass::MAX_PV_NAME+1, f );
     actWin->incLine();
    svalPvExpStr.setRaw( onePv );

    if ( major > 1 ) {

      fscanf( f, "%d\n", &index ); actWin->incLine();
      bufSvalColor = index;
      fgColor.setNullIndex( bufSvalColor, actWin->ci );

    }
    else {

      fscanf( f, "%d %d %d\n", &r, &g, &b ); actWin->incLine();
      actWin->ci->setRGB( r, g, b, &pixel );
      bufSvalColor = actWin->ci->pixIndex( pixel );
      fgColor.setNullIndex( bufSvalColor, actWin->ci );

    }

  }
  else {

    svalPvExpStr.setRaw( "" );
    bufSvalColor = bufFgColor;

  }

  if ( ( major > 1 ) || ( minor > 6 ) ) {

    fscanf( f, "%d\n", &nullDetectMode ); actWin->incLine();

  }
  else {

    nullDetectMode = 0;

  }

  if ( ( major > 1 ) || ( minor > 7 ) ) {

    readStringFromFile( onePv, activeGraphicClass::MAX_PV_NAME+1, f );
     actWin->incLine();
    fgPvExpStr.setRaw( onePv );

  }
  else {

    fgPvExpStr.setRaw( "" );

  }

  if ( ( major > 1 ) || ( minor > 8 ) ) {

    fscanf( f, "%d\n", &smartRefresh ); actWin->incLine();

  }
  else {

    smartRefresh = 0;

  }

  if ( ( ( major == 1 ) && ( minor > 0 ) ) || ( major > 1 ) ) {

    fscanf( f, "%d\n", &useKp ); actWin->incLine();

  }
  else {

    useKp = 0;

  }

  if ( ( ( major == 2 ) && ( minor > 1 ) ) || ( major > 2 ) ) {

    fscanf( f, "%d\n", &changeValOnLoseFocus ); actWin->incLine();

  }
  else {

    changeValOnLoseFocus = 1; // older version behavior

  }

  if ( ( ( major == 2 ) && ( minor > 2 ) ) || ( major > 2 ) ) {

    fscanf( f, "%d\n", &fastUpdate ); actWin->incLine();

  }
  else {

    fastUpdate = 0; // older version behavior

  }

  if ( ( ( major == 2 ) && ( minor > 3 ) ) || ( major > 2 ) ) {

    fscanf( f, "%d\n", &isDate );
    fscanf( f, "%d\n", &isFile );

    readStringFromFile( oneName, XTDC_K_MAX+1, f ); actWin->incLine();
    defDir.setRaw( oneName );

    readStringFromFile( oneName, XTDC_K_MAX+1, f ); actWin->incLine();
    pattern.setRaw( oneName );

  }
  else {

    isDate = 0;
    isFile = 0;

  }

  if ( ( ( major == 2 ) && ( minor > 4 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &objType );
  }
  else {
    objType = -1;
  }

  if ( ( ( major == 2 ) && ( minor > 5 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &autoSelect );
  }
  else {
    autoSelect = 0;
  }

  if ( ( ( major == 2 ) && ( minor > 7 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &updatePvOnDrop );
  }
  else {
    updatePvOnDrop = 0;
  }

  if ( ( ( major == 2 ) && ( minor > 8 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &useHexPrefix );
  }
  else {
    useHexPrefix = 1;
  }

  if ( ( ( major == 2 ) && ( minor > 9 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &fileComponent );
    fscanf( f, "%d\n", &dateAsFileName );
  }
  else {
    fileComponent = XTDC_K_FILE_FULL_PATH;
    dateAsFileName = 0;
  }

  if ( ( ( major == 2 ) && ( minor > 10 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &showUnits );
  }
  else {
    showUnits = 0;
  }

  if ( editable ) {
    showUnits = 0;
  }

  if ( ( ( major == 2 ) && ( minor > 11 ) ) || ( major > 2 ) ) {
    fscanf( f, "%d\n", &useAlarmBorder );
  }
  else {
    useAlarmBorder = 0;
  }

  actWin->fi->loadFontTag( fontTag );
  actWin->drawGc.setFontTag( fontTag, actWin->fi );

  stringLength = strlen( value );

  fs = actWin->fi->getXFontStruct( fontTag );

  updateFont( value, fontTag, &fs, &fontAscent, &fontDescent, &fontHeight,
   &stringWidth );

  stringY = y + fontAscent + h/2 - fontHeight/2;

  if ( alignment == XmALIGNMENT_BEGINNING )
    stringX = x;
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

  return stat;

}

int activeXTextDspClass::importFromXchFile (
  FILE *f,
  char *name,
  activeWindowClass *_actWin )
{

int r, g, b, more;
int stat = 1;
char *tk, *gotData, *context, buf[255+1];
unsigned int pixel;

  r = 0xffff;
  g = 0xffff;
  b = 0xffff;

  this->actWin = _actWin;

  strcpy( value, "" );
  strcpy( pvName, "" );

  fgColor.setColorIndex( actWin->defaultTextFgColor, actWin->ci );
  bgColor = actWin->defaultBgColor;

  useDisplayBg = 1;

  autoHeight = 1;

  formatType = XTDC_K_FORMAT_NATURAL;

  colorMode = XTDC_K_COLORMODE_STATIC;

  editable = 0;
  smartRefresh = 0;
  isWidget = 0;
  useKp = 0;
  isDate = 0;
  isFile = 0;

  strcpy( fontTag, actWin->defaultFontTag );

  alignment = actWin->defaultAlignment;

  // continue until tag is <eod>

  do {

    gotData = getNextDataString( buf, 255, f );
    if ( !gotData ) {
      actWin->appCtx->postMessage( activeXTextDspClass_str3 );
      return 0;
    }

    context = NULL;

    tk = strtok_r( buf, " \t\n", &context );
    if ( !tk ) {
      actWin->appCtx->postMessage( activeXTextDspClass_str3 );
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
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        x = atol( tk );

      }
      else if ( strcmp( tk, "y" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        y = atol( tk );

      }
      else if ( strcmp( tk, "w" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        w = atol( tk );

      }
      else if ( strcmp( tk, "h" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        h = atol( tk );

      }
            
      else if ( strcmp( tk, "ctlpv" ) == 0 ) {

        tk = strtok_r( NULL, "\"", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        strncpy( pvName, tk, 28 );
        pvName[28] = 0;

      }
            
      else if ( strcmp( tk, "font" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        strncpy( fontTag, tk, 63 );
	fontTag[63] = 0;

      }
            
      else if ( strcmp( tk, "justify" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        alignment = atol( tk );

      }
            
      else if ( strcmp( tk, "red" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        r = atol( tk );

      }
            
      else if ( strcmp( tk, "green" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        g = atol( tk );

      }
            
      else if ( strcmp( tk, "blue" ) == 0 ) {

        tk = strtok_r( NULL, "\"\n", &context );
        if ( !tk ) {
          actWin->appCtx->postMessage( activeXTextDspClass_str3 );
          return 0;
        }

        b = atol( tk );

      }
            
    }

  } while ( more );

  actWin->ci->setRGB( r, g, b, &pixel );
  bufFgColor = actWin->ci->pixIndex( pixel );
  fgColor.setColorIndex( bufFgColor, actWin->ci );

  limitsFromDb = 1;
  changeValOnLoseFocus = 0;
  fastUpdate = 0;
  precision = 3;
  efPrecision.setValue( 3 );

  fgColor.setAlarmInsensitive();

  strncpy( value, pvName, 39 );
  value[39] = 0;

  pvExpStr.setRaw( pvName );

  actWin->fi->loadFontTag( fontTag );
  actWin->drawGc.setFontTag( fontTag, actWin->fi );

  stringLength = strlen( value );

  fs = actWin->fi->getXFontStruct( fontTag );

  updateFont( value, fontTag, &fs, &fontAscent, &fontDescent, &fontHeight,
   &stringWidth );

  y = y + fontDescent;

  this->initSelectBox(); // call after getting x,y,w,h

  if ( alignment == XmALIGNMENT_BEGINNING )
    stringX = x;
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

  stringY = y + fontAscent + h/2 - fontHeight/2;

  return stat;

}

int activeXTextDspClass::genericEdit ( void ) {

char title[32], *ptr;
int noedit;

  strcpy( title, "activeXTextDspClass" );
  if ( strcmp( this->getCreateParam(), "noedit" ) == 0 ) {
    noedit = 1;
    Strncat( title, ":noedit", 31 );
  }
  else {
    noedit = 0;
  }

  ptr = actWin->obj.getNameFromClass( title );
  if ( ptr ) {
    strncpy( title, ptr, 31 );
    title[31] = 0;
  }
  else {
    strncpy( title, activeXTextDspClass_str4, 31 );
    title[31] = 0;
  }

  Strncat( title, activeXTextDspClass_str5, 31 );
  title[31] = 0;

  strncpy( bufId, id, 31 );
  bufId[31] = 0;

  bufX = x;
  bufY = y;
  bufW = w;
  bufH = h;
  bufFgColor = fgColor.pixelIndex();
  bufBgColor = bgColor;
  strncpy( bufFontTag, fontTag, 63 );
  bufFontTag[63] = 0;
  bufUseDisplayBg = useDisplayBg;
  bufAutoHeight = autoHeight;
  bufFormatType = formatType;
  bufColorMode = colorMode;
  strncpy( bufValue, value, XTDC_K_MAX );
  bufValue[XTDC_K_MAX] = 0;
  strncpy( bufPvName, pvName, activeGraphicClass::MAX_PV_NAME );
  bufPvName[activeGraphicClass::MAX_PV_NAME] = 0;

  if ( fgPvExpStr.getRaw() ) {
    strncpy( bufColorPvName, fgPvExpStr.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
    bufColorPvName[activeGraphicClass::MAX_PV_NAME] = 0;
  }
  else {
    strcpy( bufColorPvName, "" );
  }

  if ( svalPvExpStr.getRaw() ) {
    strncpy( bufSvalPvName, svalPvExpStr.getRaw(),
     activeGraphicClass::MAX_PV_NAME );
    bufSvalPvName[activeGraphicClass::MAX_PV_NAME] = 0;
  }
  else {
    strcpy( bufSvalPvName, "" );
  }

  if ( defDir.getRaw() ) {
    strncpy( bufDefDir, defDir.getRaw(), XTDC_K_MAX );
    bufDefDir[XTDC_K_MAX] = 0;
  }
  else {
    strcpy( bufDefDir, "" );
  }

  if ( pattern.getRaw() ) {
    strncpy( bufPattern, pattern.getRaw(), XTDC_K_MAX );
    bufPattern[XTDC_K_MAX] = 0;
  }
  else {
    strcpy( bufPattern, "" );
  }

  bufSvalColor = fgColor.nullIndex();
  bufNullDetectMode = nullDetectMode;

  bufEditable = editable;
  bufSmartRefresh = smartRefresh;
  bufIsWidget = isWidget;
  bufUseKp = useKp;
  bufIsDate = isDate;
  bufDateAsFileName = dateAsFileName;
  bufIsFile = isFile;
  bufFileComponent = fileComponent;
  bufLimitsFromDb = limitsFromDb;
  bufChangeValOnLoseFocus = changeValOnLoseFocus;
  bufFastUpdate = fastUpdate;
  bufEfPrecision = efPrecision;
  bufChangeCallbackFlag = changeCallbackFlag;
  bufActivateCallbackFlag = activateCallbackFlag;
  bufDeactivateCallbackFlag = deactivateCallbackFlag;
  bufAutoSelect = autoSelect;
  bufUpdatePvOnDrop = updatePvOnDrop;
  bufUseHexPrefix = useHexPrefix;
  bufShowUnits = showUnits;
  bufUseAlarmBorder = useAlarmBorder;

  ef.create( actWin->top, actWin->appCtx->ci.getColorMap(),
   &actWin->appCtx->entryFormX,
   &actWin->appCtx->entryFormY, &actWin->appCtx->entryFormW,
   &actWin->appCtx->entryFormH, &actWin->appCtx->largestH,
   title, NULL, NULL, NULL );

  //ef.addTextField( activeXTextDspClass_str6, 35, bufId, 31 );
  ef.addTextField( activeXTextDspClass_str7, 35, &bufX );
  ef.addTextField( activeXTextDspClass_str8, 35, &bufY );
  ef.addTextField( activeXTextDspClass_str9, 35, &bufW );
  ef.addTextField( activeXTextDspClass_str10, 35, &bufH );
  ef.addTextField( activeXTextDspClass_str22, 35, bufPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addTextField( activeXTextDspClass_str74, 35, bufColorPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addTextField( activeXTextDspClass_str25, 35, bufSvalPvName,
   activeGraphicClass::MAX_PV_NAME );
  ef.addOption( activeXTextDspClass_str23, activeXTextDspClass_str24,
   &bufNullDetectMode );
  ef.addOption( activeXTextDspClass_str18,
   activeXTextDspClass_str19, &bufFormatType );
  ef.addToggle( activeXTextDspClass_str77, &bufUseHexPrefix );
  ef.addToggle( activeXTextDspClass_str20, &bufLimitsFromDb );
  ef.addTextField( activeXTextDspClass_str21, 35, &bufEfPrecision );
  ef.addToggle( activeXTextDspClass_str81, &bufShowUnits );
  ef.addToggle( activeXTextDspClass_str11, &bufAutoHeight );

  if ( !noedit ) {
    ef.addToggle( activeXTextDspClass_str27, &bufEditable );
  }
  else {
    bufEditable = editable = 0;
  }

  if ( !noedit ) {
    ef.addToggle( activeXTextDspClass_str67, &bufUseKp );
  }
  else {
    bufUseKp = useKp = 0;
  }

  ef.addToggle( activeXTextDspClass_str28, &bufSmartRefresh );
  ef.addToggle( activeXTextDspClass_str29, &bufIsWidget );

  if ( !noedit ) {
    ef.addToggle( activeXTextDspClass_str68, &bufChangeValOnLoseFocus );
    ef.addToggle( activeXTextDspClass_str75, &bufAutoSelect );
    ef.addToggle( activeXTextDspClass_str76, &bufUpdatePvOnDrop );
  }
  else {
    bufChangeValOnLoseFocus = changeValOnLoseFocus = 0;
    bufAutoSelect = autoSelect = 0;
    bufUpdatePvOnDrop = updatePvOnDrop = 0;
  }

  ef.addToggle( activeXTextDspClass_str69, &bufFastUpdate );

  if ( !noedit ) {
    ef.addToggle( activeXTextDspClass_str70, &bufIsDate );
    ef.addToggle( activeXTextDspClass_str80, &bufDateAsFileName );
    ef.addToggle( activeXTextDspClass_str71, &bufIsFile );
    ef.addOption( activeXTextDspClass_str78,
     activeXTextDspClass_str79, &bufFileComponent );
    ef.addTextField( activeXTextDspClass_str72, 35, bufDefDir, XTDC_K_MAX );
    ef.addTextField( activeXTextDspClass_str73, 35, bufPattern, XTDC_K_MAX );
  }
  else {
    bufIsDate = isDate = 0;
    bufIsFile = isFile = 0;
    fileComponent = XTDC_K_FILE_FULL_PATH;
    dateAsFileName = 0;
  }

  ef.addColorButton( activeXTextDspClass_str15, actWin->ci, &fgCb,
   &bufFgColor );
  ef.addToggle( activeXTextDspClass_str14, &bufColorMode );
  ef.addToggle( activeXTextDspClass_str82, &bufUseAlarmBorder );
  ef.addColorButton( activeXTextDspClass_str16, actWin->ci, &bgCb,
   &bufBgColor );
  ef.addToggle( activeXTextDspClass_str17, &bufUseDisplayBg );
  ef.addColorButton( activeXTextDspClass_str26, actWin->ci, &svalCb,
   &bufSvalColor );
  ef.addFontMenu( activeXTextDspClass_str12, actWin->fi, &fm, fontTag );
  fm.setFontAlignment( alignment );

  //ef.addToggle( activeXTextDspClass_str30, &bufActivateCallbackFlag );
  //ef.addToggle( activeXTextDspClass_str31, &bufDeactivateCallbackFlag );
  //ef.addToggle( activeXTextDspClass_str32, &bufChangeCallbackFlag );

  return 1;

}

int activeXTextDspClass::editCreate ( void ) {

  this->genericEdit();
  ef.finished( axtdc_edit_ok, axtdc_edit_apply, axtdc_edit_cancel_delete, this );
  actWin->currentEf = NULL;
  ef.popup();

  return 1;

}

int activeXTextDspClass::edit ( void ) {

  this->genericEdit();
  ef.finished( axtdc_edit_ok, axtdc_edit_apply, axtdc_edit_cancel, this );
  fm.setFontAlignment( alignment );
  actWin->currentEf = &ef;
  ef.popup();

  return 1;

}

int activeXTextDspClass::erase ( void ) {

XRectangle xR = { x, y, w, h };

  if ( activeMode || deleteRequest ) return 1;

  actWin->drawGc.addEraseXClipRectangle( xR );

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->drawGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    XDrawString( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.eraseGC(), stringX, stringY,
     value, stringLength );

  }
  else {

    XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.eraseGC(), x, y, w, h );

    XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.eraseGC(), x, y, w, h );

    XDrawImageString( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.eraseGC(), stringX, stringY,
     value, stringLength );

  }

  actWin->drawGc.removeEraseXClipRectangle();

  return 1;

}

int activeXTextDspClass::eraseActive ( void ) {

int len;

  if ( !init || !activeMode ) return 1;

  if ( isWidget ) return 1;

  if ( !bufInvalid && ( strlen(value) == strlen(bufValue) ) ) {
    if ( strcmp( value, bufValue ) == 0 ) return 1;
  }

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->executeGc.setFontTag( fontTag, actWin->fi );
  }

  len = strlen(bufValue);

  if ( bufInvalid ) {

    if ( colorMode == XTDC_K_COLORMODE_ALARM ) {

      if ( fgColor.getSeverity() != prevAlarmSeverity ) {

        if ( useAlarmBorder ) {

          actWin->executeGc.setLineWidth( 2 );
          actWin->executeGc.setLineStyle( LineSolid );

          XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
           actWin->executeGc.eraseGC(), x, y, w, h );

          actWin->executeGc.setLineWidth( 1 );

        }

      }

    }

  }

  if ( useDisplayBg ) {

    XDrawString( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), stringX, stringY,
     bufValue, len );

  }
  else {

    actWin->executeGc.saveFg();
    actWin->executeGc.saveBg();

    actWin->executeGc.setFG( actWin->ci->pix(bgColor) );
    actWin->executeGc.setBG( actWin->ci->pix(bgColor) );

    XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->executeGc.normGC(), x, y, w, h );

    XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->executeGc.normGC(), x, y, w, h );

    XDrawImageString( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), stringX, stringY,
     bufValue, len );

    actWin->executeGc.restoreFg();
    actWin->executeGc.restoreBg();

  }

  return 1;

}

int activeXTextDspClass::draw ( void ) {

XRectangle xR = { x, y, w, h };
int clipStat;
int blink = 0;

  if ( activeMode || deleteRequest ) return 1;

  actWin->drawGc.saveFg();
  actWin->drawGc.saveBg();

  clipStat = actWin->drawGc.addNormXClipRectangle( xR );

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->drawGc.setFontTag( fontTag, actWin->fi );
  }

  if ( useDisplayBg ) {

    actWin->drawGc.setFG( fgColor.pixelIndex(), &blink );
    actWin->drawGc.setBG( actWin->ci->pix(bgColor) );

    XDrawString( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), stringX, stringY,
     value, stringLength );

  }
  else {

    actWin->drawGc.setFG( actWin->ci->pix(bgColor) );

    XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), x, y, w, h );

    XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), x, y, w, h );

    actWin->drawGc.setFG( fgColor.pixelIndex(), &blink );
    actWin->drawGc.setBG( actWin->ci->pix(bgColor) );

    XDrawImageString( actWin->d, XtWindow(actWin->drawWidget),
     actWin->drawGc.normGC(), stringX, stringY,
     value, stringLength );

  }

  if ( clipStat & 1 ) actWin->drawGc.removeNormXClipRectangle();

  actWin->drawGc.restoreFg();
  actWin->drawGc.restoreBg();

  updateBlink( blink );

  return 1;

}

int activeXTextDspClass::drawActive ( void ) {

Arg args[10];
int n;
int blink = 0;

  if ( !init && !connection.pvsConnected() ) {
    if ( needToDrawUnconnected ) {
      actWin->executeGc.saveFg();
      //actWin->executeGc.setFG( fgColor.getDisconnected() );
      actWin->executeGc.setFG( fgColor.getDisconnectedIndex(), &blink );
      actWin->executeGc.setLineWidth( 2 );
      actWin->executeGc.setLineStyle( LineSolid );
      XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
       actWin->executeGc.normGC(), x, y, w, h );
      actWin->executeGc.restoreFg();
      needToEraseUnconnected = 1;
      updateBlink( blink );
    }
  }
  else if ( needToEraseUnconnected ) {
    actWin->executeGc.setLineWidth( 2 );
    actWin->executeGc.setLineStyle( LineSolid );
    XDrawRectangle( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.eraseGC(), x, y, w, h );
    needToEraseUnconnected = 0;
  }

  if ( !activeMode || !init ) return 1;

  if ( !bufInvalid && ( strlen(value) == strlen(bufValue) ) ) {
    if ( strcmp( value, bufValue ) == 0 ) return 1;
  }

  if ( isWidget ) {

    if ( tf_widget ) {

      if ( !grabUpdate || updatePvOnDrop ) {

        if ( bufInvalid ) {
          n = 0;
          XtSetArg( args[n], XmNvalue, (XtArgVal) value ); n++;
          if ( useAlarmBorder && ( colorMode == XTDC_K_COLORMODE_ALARM ) ) {
            XtSetArg( args[n], XmNforeground, fgColor.pixelColor() );
          }
          else {
            XtSetArg( args[n], XmNforeground, fgColor.getColor() ); n++;
          }
          if ( colorMode == XTDC_K_COLORMODE_ALARM ) {
            if ( fgColor.getSeverity() != prevAlarmSeverity ) {
              if ( fgColor.getSeverity() && useAlarmBorder ) {
                XtSetArg( args[n], XmNborderWidth, (XtArgVal) 2 ); n++;
                XtSetArg( args[n], XmNborderColor, fgColor.getColor() );
                 n++;
              }
              else {
                XtSetArg( args[n], XmNborderWidth, (XtArgVal) 0 ); n++;
              }
            }
          }
          XtSetValues( tf_widget, args, n );
        }
        else {
          XmTextFieldSetString( tf_widget, value );
        }

      }

    }

    strncpy( entryValue, value, XTDC_K_MAX );
    entryValue[XTDC_K_MAX] = 0;

    strncpy( bufValue, value, XTDC_K_MAX );
    bufValue[XTDC_K_MAX] = 0;


    if ( bufInvalid ) {
      bufInvalid = 0;
    }

    if ( fgColor.getSeverity() != prevAlarmSeverity ) {
      prevAlarmSeverity = fgColor.getSeverity();
    }

    return 1;

  }

  actWin->executeGc.saveFg();
  actWin->executeGc.saveBg();

  if ( strcmp( fontTag, "" ) != 0 ) {
    actWin->executeGc.setFontTag( fontTag, actWin->fi );
  }

  updateDimensions();

  if ( useDisplayBg ) {

    if ( useAlarmBorder && ( colorMode == XTDC_K_COLORMODE_ALARM ) ) {
      actWin->executeGc.setFG( fgColor.pixelIndex(), &blink );
    }
    else {
      actWin->executeGc.setFG( fgColor.getIndex(), &blink );
    }

    actWin->executeGc.setBG( actWin->ci->pix(bgColor) );

    XDrawString( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), stringX, stringY,
     value, stringLength );

  }
  else {

    actWin->executeGc.setFG( actWin->ci->pix(bgColor) );

    XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->executeGc.normGC(), x, y, w, h );

    XFillRectangle( actWin->d, XtWindow(actWin->drawWidget),
     actWin->executeGc.normGC(), x, y, w, h );

    if ( useAlarmBorder && ( colorMode == XTDC_K_COLORMODE_ALARM ) ) {
      actWin->executeGc.setFG( fgColor.pixelIndex(), &blink );
    }
    else {
      actWin->executeGc.setFG( fgColor.getIndex(), &blink );
    }

    actWin->executeGc.setBG( actWin->ci->pix(bgColor) );

    XDrawImageString( actWin->d, XtWindow(actWin->executeWidget),
     actWin->executeGc.normGC(), stringX, stringY, 
     value, stringLength );

  }

  if ( colorMode == XTDC_K_COLORMODE_ALARM ) {

    if ( fgColor.getSeverity() && useAlarmBorder ) {

      actWin->executeGc.setFG( fgColor.getIndex(), &blink );
      actWin->executeGc.setLineWidth( 2 );
      actWin->executeGc.setLineStyle( LineSolid );

      XDrawRectangle( actWin->d, XtWindow(actWin->drawWidget),
       actWin->executeGc.normGC(), x, y, w, h );

      actWin->executeGc.setLineWidth( 1 );

    }

  }

  actWin->executeGc.restoreFg();
  actWin->executeGc.restoreBg();

  strncpy( bufValue, value, XTDC_K_MAX );
  bufValue[XTDC_K_MAX] = 0;

  updateBlink( blink );

  if ( bufInvalid ) {
    bufInvalid = 0;
  }

  if ( fgColor.getSeverity() != prevAlarmSeverity ) {
    prevAlarmSeverity = fgColor.getSeverity();
  }

  return 1;

}

void activeXTextDspClass::bufInvalidate ( void ) {

  bufInvalid = 1;

}

int activeXTextDspClass::expand1st (
  int numMacros,
  char *macros[],
  char *expansions[] ) {

int stat;

  stat = pvExpStr.expand1st( numMacros, macros, expansions );
  stat = svalPvExpStr.expand1st( numMacros, macros, expansions );
  stat = fgPvExpStr.expand1st( numMacros, macros, expansions );
  stat = defDir.expand1st( numMacros, macros, expansions );
  stat = pattern.expand1st( numMacros, macros, expansions );

  return stat;

}

int activeXTextDspClass::expand2nd (
  int numMacros,
  char *macros[],
  char *expansions[] ) {

int stat;

  stat = pvExpStr.expand2nd( numMacros, macros, expansions );
  stat = svalPvExpStr.expand2nd( numMacros, macros, expansions );
  stat = fgPvExpStr.expand2nd( numMacros, macros, expansions );
  stat = defDir.expand2nd( numMacros, macros, expansions );
  stat = pattern.expand2nd( numMacros, macros, expansions );

  return stat;

}

int activeXTextDspClass::containsMacros ( void ) {

int result;

  result = pvExpStr.containsPrimaryMacros();
  if ( result ) return 1;

  result = svalPvExpStr.containsPrimaryMacros();
  if ( result ) return 1;

  result = fgPvExpStr.containsPrimaryMacros();
  if ( result ) return 1;

  result = defDir.containsPrimaryMacros();
  if ( result ) return 1;

  result = pattern.containsPrimaryMacros();
  if ( result ) return 1;

  return 0;

}

int activeXTextDspClass::activate (
  int pass,
  void *ptr )
{

int stat;
char callbackName[63+1];

  switch ( pass ) {

  case 1:

    deferredCount = 0;
    needConnectInit = needInfoInit = needErase = needDraw = needRefresh =
     needUpdate = 0;
    needToEraseUnconnected = 0;
    needToDrawUnconnected = 0;
    unconnectedTimer = 0;
    aglPtr = ptr;
    strcpy( curValue, "" );
    strcpy( value, "" );
    strcpy( bufValue, "" );
    updateDimensions();
    tf_widget = NULL;
    opComplete = 0;
    numStates = 0; // for enum type
    editDialogIsActive = 0;
    activeMode = 1;
    init = 0;
    curDoubleValue = 0.0;
    curSvalValue = 0.0;
    noSval = 1;
    grabUpdate = 0;
    pvExistCheck = 0;
    connection.init();
    pvId = svalPvId = fgPvId = NULL;
    prevAlarmSeverity = 0;
    pvCount = svalPvCount = 1;

    break;

  case 2:

    if ( !opComplete ) {

      if ( !unconnectedTimer ) {
        unconnectedTimer = appAddTimeOut( actWin->appCtx->appContext(),
         2000, unconnectedTimeout, this );
      }

      fgColor.setNotNull();

      if ( !pvExistCheck ) {

        pvExistCheck = 1;

        if ( pvExpStr.getExpanded() ) {
          if ( strcmp( pvExpStr.getExpanded(), "" ) != 0 ) {
            pvExists = 1;
            connection.addPv(); // must do this only once per pv
          }
          else {
            pvExists = 0;
          }
        }
        else {
          pvExists = 0;
        }

        if ( svalPvExpStr.getExpanded() ) {
          if ( strcmp( svalPvExpStr.getExpanded(), "" ) != 0 ) {
            svalPvExists = 1;
            connection.addPv(); // must do this only once per pv
          }
          else {
            svalPvExists = 0;
          }
        }
        else {
          svalPvExists = 0;
        }

        if ( fgPvExpStr.getExpanded() ) {
          if ( strcmp( fgPvExpStr.getExpanded(), "" ) != 0 ) {
            fgPvExists = 1;
            connection.addPv(); // must do this only once per pv
          }
          else {
            fgPvExists = 0;
          }
        }
        else {
          fgPvExists = 0;
        }

      }

#ifdef __epics__
      eventId = 0;
      alarmEventId = 0;
      svalEventId = 0;
      fgEventId = 0;
#endif

      if ( pvExists ) {

#ifdef __epics__
        stat = ca_search_and_connect( pvExpStr.getExpanded(), &pvId,
         xtdo_monitor_connect_state, this );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str33 );
          return 0;
        }

        if ( svalPvExists ) {

          stat = ca_search_and_connect( svalPvExpStr.getExpanded(), &svalPvId,
           xtdo_monitor_sval_connect_state, this );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str34 );
            return 0;
          }

        }

        if ( fgPvExists ) {

          stat = ca_search_and_connect( fgPvExpStr.getExpanded(), &fgPvId,
           xtdo_monitor_fg_connect_state, this );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str35 );
            return 0;
          }

        }

#endif

      }
      else if ( anyCallbackFlag ) {

        needInfoInit = 1;
        actWin->appCtx->proc->lock();
        actWin->addDefExeNode( aglPtr );
        actWin->appCtx->proc->unlock();

      }

      if ( anyCallbackFlag ) {

        if ( changeCallbackFlag ) {
          strncpy( callbackName, id, 63 );
	  callbackName[63] = 0;
          Strncat( callbackName, activeXTextDspClass_str36, 63 );
          callbackName[63] = 0;
          changeCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( activateCallbackFlag ) {
          strncpy( callbackName, id, 63 );
	  callbackName[63] = 0;
          Strncat( callbackName, activeXTextDspClass_str37, 63 );
          callbackName[63] = 0;
          activateCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( deactivateCallbackFlag ) {
          strncpy( callbackName, id, 63 );
	  callbackName[63] = 0;
          Strncat( callbackName, activeXTextDspClass_str38, 63 );
          callbackName[63] = 0;
          deactivateCallback =
           actWin->appCtx->userLibObject.getFunc( callbackName );
	}

        if ( activateCallback ) {
          (*activateCallback)( this );
        }

      }

      opComplete = 1;

    }

    break;

  case 3:
  case 4:
  case 5:
  case 6:
    break;

  } // end switch

  return 1;

}

int activeXTextDspClass::deactivate (
  int pass
) {

int stat;

  if ( pass == 1 ) {

  activeMode = 0;

  if ( unconnectedTimer ) {
    XtRemoveTimeOut( unconnectedTimer );
    unconnectedTimer = 0;
  }

  updateBlink( 0 );

  if ( kp.isPoppedUp() ) {
    kp.popdown();
  }

  if ( cp.isPoppedUp() ) {
    cp.popdown();
  }

  if ( fsel.isPoppedUp() ) {
    fsel.popdown();
  }

  if ( textEntry.formIsPoppedUp() ) {
    textEntry.popdown();
    editDialogIsActive = 0;
  }

  if ( deactivateCallback ) {
    (*deactivateCallback)( this );
  }

#ifdef __epics__

  if ( pvExists ) {

    if ( pvId ) {
      stat = ca_clear_channel( pvId );
      pvId = NULL;
      if ( stat != ECA_NORMAL ) printf( activeXTextDspClass_str41 );
    }

  }

  if ( svalPvExists ) {

    if ( svalPvId ) {
      stat = ca_clear_channel( svalPvId );
      svalPvId = NULL;
      if ( stat != ECA_NORMAL ) printf( activeXTextDspClass_str42 );
    }

  }

  if ( fgPvExists ) {

    if ( fgPvId ) {
      stat = ca_clear_channel( fgPvId );
      fgPvId = NULL;
      if ( stat != ECA_NORMAL ) printf( activeXTextDspClass_str43 );
    }

  }

#endif

  }
  else if ( pass == 2 ) {

  if ( tf_widget ) {
    XtDestroyWidget( tf_widget );
    tf_widget = NULL;
  }

  strcpy( value, pvName );
  strcpy( curValue, pvName );
  updateDimensions();

  }

  return 1;

}

void activeXTextDspClass::updateDimensions ( void )
{

  stringLength = strlen( value );

  if ( fs ) {
    stringWidth = XTextWidth( fs, value, stringLength );
  }
  else {
    stringWidth = 0;
  }

  stringY = y + fontAscent + h/2 - fontHeight/2;

  stringX = x;

  if ( alignment == XmALIGNMENT_BEGINNING ) {
    // no change
  }
  else if ( alignment == XmALIGNMENT_CENTER )
    stringX = x + w/2 - stringWidth/2;
  else if ( alignment == XmALIGNMENT_END )
    stringX = x + w - stringWidth;

}

void activeXTextDspClass::btnUp (
  int x,
  int y,
  int buttonState,
  int buttonNumber )
{

}

void activeXTextDspClass::btnDown (
  int x,
  int y,
  int buttonState,
  int buttonNumber )
{

char selectString[XTDC_K_MAX+1], bufDir[XTDC_K_MAX+1], bufPat[XTDC_K_MAX+1];
int i;

  if ( !editable || isWidget || !ca_write_access( pvId ) ) return;

  if ( buttonNumber != 1 ) return;

  if ( editDialogIsActive ) return;

  teX = x + actWin->xPos();
  teY = this->y + actWin->yPos() + h;
  teW = w;
  teH = h;
  teLargestH = 600;

  if ( useKp ) {

    if ( ( pvType == DBR_FLOAT ) || ( pvType == DBR_DOUBLE ) ) {
      if ( formatType == XTDC_K_FORMAT_HEX ) {
        kp.createHex( actWin->top, teX, teY, "", &kpDouble,
         (void *) this,
         (XtCallbackProc) xtdoSetKpDoubleValue,
         (XtCallbackProc) xtdoCancelKp );
      }
      else {
        kp.create( actWin->top, teX, teY, "", &kpDouble,
         (void *) this,
         (XtCallbackProc) xtdoSetKpDoubleValue,
         (XtCallbackProc) xtdoCancelKp );
      }
      editDialogIsActive = 1;
      return;
    }
    else if ( ( pvType == DBR_SHORT ) || ( pvType == DBR_LONG ) ) {
      if ( formatType == XTDC_K_FORMAT_HEX ) {
        kp.createHex( actWin->top, teX, teY, "", &kpInt,
         (void *) this,
         (XtCallbackProc) xtdoSetKpIntValue,
         (XtCallbackProc) xtdoCancelKp );
      }
      else {
        kp.create( actWin->top, teX, teY, "", &kpInt,
         (void *) this,
         (XtCallbackProc) xtdoSetKpIntValue,
         (XtCallbackProc) xtdoCancelKp );
      }
      editDialogIsActive = 1;
      return;
    }
    else if ( pvType == DBR_STRING ) {

      if ( isFile ) {

        if ( defDir.getExpanded() ) {
          strncpy( bufDir, defDir.getExpanded(), XTDC_K_MAX );
          bufDir[XTDC_K_MAX] = 0;
	}
        else {
          strcpy( bufDir, "" );
	}

        if ( pattern.getExpanded() ) {
          strncpy( bufPat, pattern.getExpanded(), XTDC_K_MAX );
          bufPat[XTDC_K_MAX] = 0;
	}
        else {
          strcpy( bufPat, "" );
	}

        fsel.create( actWin->top, teX, teY,
         bufDir, bufPat,
         (void *) this,
         (XtCallbackProc) xtdoSetFsValue,
         (XtCallbackProc) xtdoCancelStr );
        editDialogIsActive = 1;
        return;

      }
      else if ( isDate ) {

        cp.create( actWin->top, teX, teY, entryValue, XTDC_K_MAX,
         (void *) this,
         (XtCallbackProc) xtdoSetCpValue,
         (XtCallbackProc) xtdoCancelStr );
        cp.setDate( curValue );
        editDialogIsActive = 1;
        return;

      }

    }

  }

  strncpy( entryValue, value, XTDC_K_MAX );
  entryValue[XTDC_K_MAX] = 0;

  textEntry.create( actWin->top, &teX, &teY, &teW, &teH, &teLargestH, "",
  NULL, NULL, NULL );

#ifdef __epics__

  if ( pvType != DBR_ENUM ) {
    textEntry.addTextField( activeXTextDspClass_str44, 25, entryValue,
     XTDC_K_MAX );
  }
  else {
    strcpy( selectString, "" );
    for ( i=0; i<numStates; i++ ) {
      Strncat( selectString, stateString[i], XTDC_K_MAX );
      selectString[XTDC_K_MAX] = 0;
      if ( i != numStates-1 ) {
        Strncat( selectString, "|", XTDC_K_MAX );
        selectString[XTDC_K_MAX] = 0;
      }
    }
    textEntry.addOption( activeXTextDspClass_str45, selectString, &entryState );
  }

#endif

  textEntry.finished( axtdc_value_edit_ok, axtdc_value_edit_apply,
   axtdc_value_edit_cancel, this );

  textEntry.popup();
  editDialogIsActive = 1;

}

void activeXTextDspClass::pointerIn (
  int _x,
  int _y,
  int buttonState )
{

  if ( !init ) return;

  if ( !ca_write_access( pvId ) ) {

    if ( isWidget ) {
      XtVaSetValues( tf_widget,
       XmNeditable, (XtArgVal) False,
       XmNcursorPositionVisible, (XtArgVal) False,
       NULL );
    }

    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_NO );

  }
  else {

    actWin->cursor.set( XtWindow(actWin->executeWidget), CURSOR_K_DEFAULT );

  }

  if ( !isWidget ) {
    activeGraphicClass::pointerIn( _x, _y, buttonState );
  }

}

int activeXTextDspClass::getButtonActionRequest (
  int *up,
  int *down,
  int *drag,
  int *focus )
{

  if ( pvExists && editable ) {
    *down = 1;
    *focus = 1;
  }
  else {
    *down = 0;
    *focus = 0;
  }

  *up = 0;
  *drag = 0;

  return 1;

}

static void drag (
   Widget w,
   XEvent *e,
   String *params,
   Cardinal numParams )
{

class activeXTextDspClass *atdo;
int stat;

  XtVaGetValues( w, XmNuserData, &atdo, NULL );

  stat = atdo->startDrag( w, e );

}

static void selectDrag (
   Widget w,
   XEvent *e,
   String *params,
   Cardinal numParams )
{

class activeXTextDspClass *atdo;
int stat;
XButtonEvent *be = (XButtonEvent *) e;

  XtVaGetValues( w, XmNuserData, &atdo, NULL );

  stat = atdo->selectDragValue( atdo->x + be->x, atdo->y + be->y );

}

void activeXTextDspClass::executeDeferred ( void ) {

int n, stat, numCols, width, csrPos;
int nc, ni, nu, nr, nd, ne;
Arg args[10];
unsigned int bg, pixel;
XmFontList textFontList = NULL;
Cardinal numImportTargets;
Atom importList[2];

  if ( actWin->isIconified ) return;

  if ( !fastUpdate ) {
    actWin->appCtx->proc->lock();
    if ( !needConnectInit && !needInfoInit && !needRefresh ) {
      deferredCount--;
      if ( deferredCount > 0 ) {
        actWin->appCtx->proc->unlock();
        return;
      }
      deferredCount = actWin->appCtx->proc->halfSecCount;
    }
    actWin->appCtx->proc->unlock();
  }

  actWin->appCtx->proc->lock();
  nc = needConnectInit; needConnectInit = 0;
  ni = needInfoInit; needInfoInit = 0;
  nr = needRefresh; needRefresh = 0;
  nu = needUpdate; needUpdate = 0;
  nd = needDraw; needDraw = 0;
  ne = needErase; needErase = 0;
  strncpy( value, curValue, XTDC_K_MAX );
  value[XTDC_K_MAX] = 0;
  actWin->remDefExeNode( aglPtr );
  actWin->appCtx->proc->unlock();

  if ( !activeMode ) return;

#ifdef __epics__

  if ( nc ) {

    switch ( pvType ) {

    case DBR_STRING:

      if ( pvCount == 1 ) {

        stat = ca_get_callback( DBR_GR_STRING, pvId,
         XtextDspInfoUpdate, (void *) this );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str46 );
        }

      }
      else {

        stat = ca_array_get_callback( DBR_CHAR, pvCount, pvId,
         XtextDspInfoUpdate, (void *) this );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str46 );
        }

      }

      break;

    case DBR_FLOAT:
    case DBR_DOUBLE:

      stat = ca_get_callback( DBR_GR_DOUBLE, pvId,
       XtextDspInfoUpdate, (void *) this );
      if ( stat != ECA_NORMAL ) {
        printf( activeXTextDspClass_str47 );
      }

      break;

    case DBR_SHORT:
    case DBR_LONG:

      stat = ca_get_callback( DBR_GR_LONG, pvId,
       XtextDspInfoUpdate, (void *) this );
      if ( stat != ECA_NORMAL ) {
        printf( activeXTextDspClass_str48 );
      }

      break;

    case DBR_ENUM:

      stat = ca_get_callback( DBR_GR_ENUM, pvId,
       XtextDspInfoUpdate, (void *) this );
      if ( stat != ECA_NORMAL ) {
        printf( activeXTextDspClass_str49 );
      }
       break;

    } // end switch

    bufInvalidate();

  }
#endif

  if ( ni ) {

#ifdef __epics__
    if ( fgPvExists ) {
      if ( !fgEventId ) {
        stat = ca_add_masked_array_event( DBR_DOUBLE, 1, fgPvId,
         XtextDspFgUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &fgEventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str50 );
        }
      }
    }
#endif

    if ( pvExists ) {

    switch ( pvType ) {

    case DBR_STRING:

      sprintf( format, "%%s" );

#ifdef __epics__
      if ( !eventId ) {

        if ( pvCount == 1 ) {

          stat = ca_add_masked_array_event( DBR_STRING, 1, pvId,
           XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0,
           (float) 0.0, &eventId, DBE_VALUE );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str51 );
          }

        }
        else {

          stat = ca_add_masked_array_event( DBR_CHAR, pvCount, pvId,
           XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0,
           (float) 0.0, &eventId, DBE_VALUE );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str51 );
          }

        }

      }

      if ( !alarmEventId ) {

        if ( pvCount == 1 ) {

          stat = ca_add_masked_array_event( DBR_STS_STRING, 1, pvId,
           XtextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
           (float) 0.0, &alarmEventId, DBE_ALARM );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str52 );
          }

	}

      }


#endif

      break;

    case DBR_FLOAT:

      switch( formatType ) {
      case XTDC_K_FORMAT_FLOAT:
        sprintf( format, "%%.%-df", precision );
        break;
      case XTDC_K_FORMAT_EXPONENTIAL:
        sprintf( format, "%%.%-de", precision );
        break;
      default:
        sprintf( format, "%%.%-df", precision );
        break;
      } // end switch( formatType )
  
#ifdef __epics__

      if ( !eventId ) {
        stat = ca_add_masked_array_event( DBR_FLOAT, 1, pvId,
         XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &eventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str53 );
        }
      }

      if ( !alarmEventId ) {
        stat = ca_add_masked_array_event( DBR_STS_FLOAT, 1, pvId,
         XtextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &alarmEventId, DBE_ALARM );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str54 );
        }
      }

      if ( svalPvExists ) {
        if ( !svalEventId ) {
          stat = ca_add_masked_array_event( svalPvType, 1, svalPvId,
           XtextDspSvalUpdate, (void *) this, (float) 0.0, (float) 0.0,
           (float) 0.0, &svalEventId, DBE_VALUE );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str55 );
          }
        }
      }

#endif

      break;

    case DBR_DOUBLE:

      switch( formatType ) {
      case XTDC_K_FORMAT_FLOAT:
        sprintf( format, "%%.%-df", precision );
        break;
      case XTDC_K_FORMAT_EXPONENTIAL:
        sprintf( format, "%%.%-de", precision );
        break;
      default:
        sprintf( format, "%%.%-df", precision );
        break;
      } // end switch( formatType )

#ifdef __epics__

      if ( !eventId ) {
        stat = ca_add_masked_array_event( DBR_DOUBLE, 1, pvId,
         XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &eventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str56 );
        }
      }

      if ( !alarmEventId ) {
        stat = ca_add_masked_array_event( DBR_STS_DOUBLE, 1, pvId,
         XtextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &alarmEventId, DBE_ALARM );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str57 );
        }
      }

      if ( svalPvExists ) {
        if ( !svalEventId ) {
          stat = ca_add_masked_array_event( svalPvType, 1, svalPvId,
           XtextDspSvalUpdate, (void *) this, (float) 0.0, (float) 0.0,
           (float) 0.0, &svalEventId, DBE_VALUE );
          if ( stat != ECA_NORMAL ) {
            printf( activeXTextDspClass_str58 );
          }
        }
      }

#endif

      break;

    case DBR_SHORT:

      switch( formatType ) {
      case XTDC_K_FORMAT_DECIMAL:
        sprintf( format, "%%-d" );
        break;
      case XTDC_K_FORMAT_HEX:
        if ( useHexPrefix ) {
          sprintf( format, "0x%%-X" );
	}
	else {
          sprintf( format, "%%-X" );
	}
        break;
      default:
        sprintf( format, "%%-d" );
        break;
      } // end switch( formatType )

#ifdef __epics__

      if ( !eventId ) {
        stat = ca_add_masked_array_event( DBR_SHORT, 1, pvId,
         XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &eventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str59 );
        }
      }

      if ( !alarmEventId ) {
        stat = ca_add_masked_array_event( DBR_STS_SHORT, 1, pvId,
         XtextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &alarmEventId, DBE_ALARM );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str60 );
        }
      }

      break;

#endif

    case DBR_LONG:

      switch( formatType ) {
      case XTDC_K_FORMAT_DECIMAL:
        sprintf( format, "%%-d" );
        break;
      case XTDC_K_FORMAT_HEX:
        if ( useHexPrefix ) {
          sprintf( format, "0x%%-X" );
	}
	else {
          sprintf( format, "%%-X" );
	}
        break;
      default:
        sprintf( format, "%%-d" );
        break;
      } // end switch( formatType )

#ifdef __epics__

      if ( !eventId ) {
        stat = ca_add_masked_array_event( DBR_LONG, 1, pvId,
         XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &eventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str61 );
        }
      }

      if ( !alarmEventId ) {
        stat = ca_add_masked_array_event( DBR_STS_LONG, 1, pvId,
         XtextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &alarmEventId, DBE_ALARM );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str62 );
        }
      }

#endif

      break;

    case DBR_ENUM:

      sprintf( format, "%%s" );

#ifdef __epics__

      if ( !eventId ) {
        stat = ca_add_masked_array_event( DBR_ENUM, 1, pvId,
         XtextDspUpdate, (void *) this, (float) 0.0, (float) 0.0, (float) 0.0,
         &eventId, DBE_VALUE );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str63 );
        }
      }

      if ( !alarmEventId ) {
        stat = ca_add_masked_array_event( DBR_STS_ENUM, 1, pvId,
         XtextAlarmUpdate, (void *) this, (float) 0.0, (float) 0.0,
         (float) 0.0, &alarmEventId, DBE_ALARM );
        if ( stat != ECA_NORMAL ) {
          printf( activeXTextDspClass_str64 );
        }
      }

#endif

      break;

    default:
      sprintf( format, "%%s" );
      break;

    } // end switch ( pvType )

    }
    else {

      pvType = DBR_STRING;
      pvCount = 1;
      sprintf( format, "%%s" );

    }

    if ( isWidget ) {

      if ( fontTag ) {
        actWin->fi->getTextFontList( fontTag, &textFontList );
      }
      else {
        textFontList = NULL;
      }

      if ( fs ) {
        width = XTextWidth( fs, "1", 1 );
        numCols = (int) ( (float) ( w / width ) + 0.5 );
        if ( numCols < 1 ) numCols = 1;
      }
      else {
        numCols = 6;
      }

      strncpy( entryValue, value, XTDC_K_MAX );
      entryValue[XTDC_K_MAX] = 0;
      csrPos = strlen(entryValue);

      widget_value_changed = 0;

      if ( useDisplayBg )
        bg = actWin->executeGc.getBaseBG();
      else
        bg = actWin->ci->pix(bgColor);

      if ( !tf_widget ) {

      if ( g_transInit ) {
        g_transInit = 0;
        g_parsedTrans = XtParseTranslationTable( g_dragTrans );
      }
      actWin->appCtx->addActions( g_dragActions, XtNumber(g_dragActions) );

      if ( useAlarmBorder && ( colorMode == XTDC_K_COLORMODE_ALARM ) ) {
        pixel = fgColor.pixelColor();
      }
      else {
        pixel = fgColor.getColor();
      }

      tf_widget = XtVaCreateManagedWidget( "", xmTextFieldWidgetClass,
       actWin->executeWidget,
       XmNx, x,
       XmNy, y-3,
       XmNforeground, pixel,
       XmNbackground, bg,
       XmNhighlightThickness, 0,
       XmNwidth, w,
       //XmNcolumns, (short) numCols,
       XmNvalue, entryValue,
       XmNmaxLength, (short) XTDC_K_MAX,
       XmNpendingDelete, True,
       XmNmarginHeight, 0,
       XmNfontList, textFontList,
       XmNtranslations, g_parsedTrans,
       XmNuserData, this,
       XmNcursorPositionVisible, False,
       NULL );

      if ( textFontList ) XmFontListFree( textFontList );

      if ( !editable ) {

        n = 0;
        XtSetArg( args[n], XmNeditable, (XtArgVal) False ); n++;
        XtSetArg( args[n], XmNnavigationType, (XtArgVal) XmNONE ); n++;
        //XtSetArg( args[n], XmNcursorPositionVisible, (XtArgVal) False ); n++;
        XtSetValues( tf_widget, args, n );

      }
      else {

        //XmTextSetInsertionPosition( tf_widget, csrPos );

        XtAddCallback( tf_widget, XmNfocusCallback,
         xtdoSetSelection, this );

        XtAddCallback( tf_widget, XmNmotionVerifyCallback,
         xtdoGrabUpdate, this );

        XtAddCallback( tf_widget, XmNvalueChangedCallback,
         xtdoSetValueChanged, this );

        if ( updatePvOnDrop ) {

	  // change drop behavior

	  importList[0] = XA_STRING;
          numImportTargets = 1;
	  n = 0;
	  XtSetArg( args[n], XmNimportTargets, importList ); n++;
	  XtSetArg( args[n], XmNnumImportTargets, numImportTargets ); n++;
	  XtSetArg( args[n], XmNdropProc, handleDrop ); n++;
	  XmDropSiteUpdate( tf_widget, args, n );

	}

        switch ( pvType ) {

        case DBR_STRING:

          XtAddCallback( tf_widget, XmNactivateCallback,
           xtdoTextFieldToStringA, this );

          if ( changeValOnLoseFocus ) {
            XtAddCallback( tf_widget, XmNlosingFocusCallback,
             xtdoTextFieldToStringLF, this );
	  }
	  else {
            XtAddCallback( tf_widget, XmNlosingFocusCallback,
             xtdoRestoreValue, this );
	  }

          break;

        case DBR_SHORT:
        case DBR_LONG:

          XtAddCallback( tf_widget, XmNactivateCallback,
           xtdoTextFieldToIntA, this );

          if ( changeValOnLoseFocus ) {
            XtAddCallback( tf_widget, XmNlosingFocusCallback,
             xtdoTextFieldToIntLF, this );
	  }
	  else {
            XtAddCallback( tf_widget, XmNlosingFocusCallback,
             xtdoRestoreValue, this );
	  }

          break;

        case DBR_FLOAT:
        case DBR_DOUBLE:

          XtAddCallback( tf_widget, XmNactivateCallback,
           xtdoTextFieldToDoubleA, this );

          if ( changeValOnLoseFocus ) {
            XtAddCallback( tf_widget, XmNlosingFocusCallback,
             xtdoTextFieldToDoubleLF, this );
	  }
	  else {
            XtAddCallback( tf_widget, XmNlosingFocusCallback,
             xtdoRestoreValue, this );
	  }

          break;

        } // end switch

      }

      } // end if ( !tf_widget )

    } // end if ( isWidget )

    fgColor.setConnected();
    init = 1;

    bufInvalidate();
    eraseActive();
    drawActive();

  }

  if ( nr ) {

    bufInvalidate();
    eraseActive();
    if (smartRefresh) {
      smartDrawAllActive();
    }
    else {
      drawActive();
    }

  }

  if ( nu ) {

    eraseActive();
    if (smartRefresh) {
      smartDrawAllActive();
    }
    else {
      drawActive();
    }

    if ( changeCallback ) {
      (*changeCallback)( this );
    }

  }

  if ( ne ) {

    eraseActive();

  }

  if ( nd ) {

    drawActive();

  }

}

int activeXTextDspClass::getProperty (
  char *prop,
  int bufSize,
  char *_value )
{

int l;
char *buf;

  if ( strcmp( prop, activeXTextDspClass_str65 ) == 0 ) {

    if ( !tf_widget ) {

      l = strlen(curValue);
      if ( l > bufSize ) l = bufSize;

      strncpy( _value, curValue, l );
      _value[l] = 0;

    }
    else {

      buf = XmTextGetString( tf_widget );

      l = strlen(buf);
      if ( l > bufSize ) l = bufSize;

      strncpy( _value, buf, l );
      _value[l] = 0;

      XtFree( buf );

    }

    return 1;

  }
  else if ( strcmp( prop, activeXTextDspClass_str66 ) == 0 ) {

    if ( !tf_widget ) {
      strncpy( _value, "", bufSize );
      _value[bufSize] = 0;
      return 0;
    }

    buf = XmTextGetString( tf_widget );

    l = strlen(buf);
    if ( l > bufSize ) l = bufSize;

    strncpy( _value, buf, l );
    _value[l] = 0;

    XtFree( buf );

    return 1;

  }

  return 0;

}

char *activeXTextDspClass::firstDragName ( void ) {

  dragIndex = 0;
  return dragName[dragIndex];

}

char *activeXTextDspClass::nextDragName ( void ) {

  if ( dragIndex < (int) ( sizeof(dragName) / sizeof(char *) ) - 1 ) {
    dragIndex++;
    return dragName[dragIndex];
  }
  else {
    return NULL;
  }

}

char *activeXTextDspClass::dragValue (
  int i ) {

  if ( i == 0 ) {
    return pvExpStr.getExpanded();
  }
  else {
    return svalPvExpStr.getExpanded();
  }

}

void activeXTextDspClass::changeDisplayParams (
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

  if ( _flag & ACTGRF_FG2COLOR_MASK )
    fgColor.setNullIndex( _fg2Color, actWin->ci );

  if ( _flag & ACTGRF_BGCOLOR_MASK )
    bgColor = _bgColor;

  if ( _flag & ACTGRF_ALIGNMENT_MASK )
    alignment = _alignment;

  if ( _flag & ACTGRF_FONTTAG_MASK ) {

    strcpy( fontTag, _fontTag );
    actWin->fi->loadFontTag( fontTag );
    fs = actWin->fi->getXFontStruct( fontTag );

    if ( fs ) {
      fontAscent = fs->ascent;
      fontDescent = fs->descent;
      fontHeight = fontAscent + fontDescent;
    }
    else {
      fontAscent = 0;
      fontDescent = 0;
      fontHeight = 0;
    }

    updateDimensions();

  }

}

void activeXTextDspClass::changePvNames (
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

int changed = 0;

  if ( editable ) {

    if ( flag & ACTGRF_CTLPVS_MASK ) {

      if ( numCtlPvs ) {

        changed = 1;

        strncpy( value, ctlPvs[0], 39 );
        value[39] = 0;
        strncpy( curValue, ctlPvs[0], 39 );
        curValue[39] = 0;

        strncpy( pvName, ctlPvs[0], activeGraphicClass::MAX_PV_NAME );
        pvName[activeGraphicClass::MAX_PV_NAME] = 0;
        pvExpStr.setRaw( pvName );

      }

    }

  }
  else {

    if ( flag & ACTGRF_READBACKPVS_MASK ) {

      if ( numReadbackPvs ) {

        changed = 1;

        strncpy( value, readbackPvs[0], 39 );
        value[39] = 0;
        strncpy( curValue, readbackPvs[0], 39 );
        curValue[39] = 0;

        strncpy( pvName, readbackPvs[0], activeGraphicClass::MAX_PV_NAME );
        pvName[activeGraphicClass::MAX_PV_NAME] = 0;
        pvExpStr.setRaw( pvName );

      }

    }

  }

  if ( changed ) {

    stringLength = strlen( curValue );

    updateFont( curValue, fontTag, &fs,
     &fontAscent, &fontDescent, &fontHeight,
     &stringWidth );

    stringY = y + fontAscent + h/2 - fontHeight/2;

    if ( alignment == XmALIGNMENT_BEGINNING )
      stringX = x;
    else if ( alignment == XmALIGNMENT_CENTER )
      stringX = x + w/2 - stringWidth/2;
    else if ( alignment == XmALIGNMENT_END )
      stringX = x + w - stringWidth;

    updateDimensions();

  }

  if ( flag & ACTGRF_NULLPVS_MASK ) {
    if ( numNullPvs ) {
      svalPvExpStr.setRaw( nullPvs[0] );
    }
  }

}

#ifdef __cplusplus
extern "C" {
#endif

void *create_activeXTextDspClassPtr ( void ) {

activeXTextDspClass *ptr;

  ptr = new activeXTextDspClass;
  return (void *) ptr;

}

void *clone_activeXTextDspClassPtr (
  void *_srcPtr )
{

activeXTextDspClass *ptr, *srcPtr;

  srcPtr = (activeXTextDspClass *) _srcPtr;

  ptr = new activeXTextDspClass( srcPtr );

  return (void *) ptr;

}

#ifdef __cplusplus
}
#endif
