#pragma once

#ifndef __DARKDLGS_H
#define __DARKDLGS_H

#include <comtools.h>
#include <isdesced.h>
#include <linkedit.h>
#include <objedbas.h>
#include <otreetyp.h>
#include <simpwrap.h>

struct IActReactEditors;
struct sLink;

void LGAPI DarkDlgsSetAggregate(IUnknown *pUnk);
IStructEditor *LGAPI ConstructStructEditor(const sStructEditorDesc *eddesc,
                                          const sStructDesc *sdesc, void *editme);
IObjEditor *LGAPI ConstructObjEditor(const sObjEditorDesc *desc, ObjID obj);
void LGAPI DoLinkEditor(const sLinkEditorDesc *desc, ObjID src, ObjID dest, RelationID flavor);
BOOL LGAPI DisplayLinkDialog(sLink &link);
void LGAPI DoHierarchy(const sObjTreeEditorDesc *desc, ObjID focus);
int LGAPI DoSimpleMenu(const char *title, const menustring *list, int count);
IActReactEditors *LGAPI ExportActReactEditors(void);

bool DarkDlgsModalActive();
void DrawDarkDlgs();
void UpdateDarkDlgs();
void CloseDarkDlgs();

#endif
