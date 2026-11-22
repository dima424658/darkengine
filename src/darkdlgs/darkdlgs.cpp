#include <stdio.h>

#include "darkdlgs.h"
#include <platform_services.h>
#include <status.h>

void DarkDlgsUnavailable(const char *title)
{
   char message[128];
   snprintf(message, sizeof(message), "%s: editor dialog unavailable", title);
   Status(message);
   if (!PlatformBatchMode())
      fprintf(stderr, "DromEd: %s\n", message);
}

// The original provider receives the application aggregate at startup.
void LGAPI DarkDlgsSetAggregate(IUnknown *)
{
}

IObjEditor *LGAPI ConstructObjEditor(const sObjEditorDesc *, ObjID)
{
   DarkDlgsUnavailable("Object properties");
   return NULL;
}

void LGAPI DoLinkEditor(const sLinkEditorDesc *, ObjID, ObjID, RelationID)
{
   DarkDlgsUnavailable("Link editor");
}

BOOL LGAPI DisplayLinkDialog(sLink &)
{
   DarkDlgsUnavailable("Create link");
   return FALSE;
}

void LGAPI DoHierarchy(const sObjTreeEditorDesc *, ObjID)
{
   DarkDlgsUnavailable("Object hierarchy");
}

IActReactEditors *LGAPI ExportActReactEditors(void)
{
   DarkDlgsUnavailable("Act/React");
   return NULL;
}
