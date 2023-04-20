/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

//
//  The new list-based pseudo-script dialog
//  Presents data as a series of sub-dialogs accesible from a simple list:
//  eg.  Header
//       Step 01
//       Step 02
//       Step 03
//       ...
//

#include <lg.h>
#include <psdialog.h>
#include <simpwrap.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 


// The default pseudo-script field format
// This is what will appear in the sub-dialogs that open when you choose to edit a step
//
static sFieldDesc gStepFieldDesc[] =
{
   { "Response Step", kFieldTypeEnum,   FieldLocation(sAIPsdScrAct, type), kFieldFlagNone, 0, FieldNames(g_ppszAIPsdScrActions) }, \
   { "   Argument 1", kFieldTypeString, FieldLocation(sAIPsdScrAct, args[0])                                                    }, \
   { "   Argument 2", kFieldTypeString, FieldLocation(sAIPsdScrAct, args[1])                                                    }, \
   { "   Argument 3", kFieldTypeString, FieldLocation(sAIPsdScrAct, args[2])                                                    }
};

static sStructDesc gStepStructDesc = StructDescBuild(sAIPsdScrAct, kStructFlagNone, gStepFieldDesc);

// Arguments:
//   title        - The dialog's caption 
//   maxSteps     - The number of steps to display
//   headerStruct - Format of the header, created with StructDescBuild()
//   data         - Pointer to the entire data structure
//   steps        - Pointer to the pseudo-script array within the data structure
//
IStructEditor* NewPseudoScriptDialog (const char* title, int maxSteps, sStructDesc* headerStruct, void* data, sAIPsdScrAct* steps)
{
    int i;
    int numMenuSlots = maxSteps + 2;
    char **responseMenu = (char **) malloc(sizeof(char*) * numMenuSlots);
    sStructEditorDesc headerDesc = { "Header", kStructEditAllButtons };
    sStructEditorDesc stepDesc = { "Step", kStructEditAllButtons };
    BOOL result = FALSE;

    //  Add the "done" choice at the beginning
    //
    responseMenu[0] = (char*) malloc(sizeof(char) * 16);
    sprintf(responseMenu[0], "-- DONE --");
    
    //  Put the header in as an option
    //
    responseMenu[1] = (char*) malloc(sizeof(char) * 16);
    sprintf(responseMenu[1], "Header");
    
    //  Build the list of response steps
    //
    for (i = 2; i < numMenuSlots; i++)
    {
        responseMenu[i] = (char*) malloc(sizeof(char) * 64);
        if (i-1 < 10)
            sprintf(responseMenu[i], "Step 0%d: %.54s", i-1, g_ppszAIPsdScrActions[steps[i-2].type]);
        else
            sprintf(responseMenu[i], "Step %d: %.54s", i-1, g_ppszAIPsdScrActions[steps[i-2].type]);
    }
    
    //  Display the list, and let the user choose
    //
    int choice = PickFromStringList(title, (const char* const*)responseMenu, numMenuSlots);
    for (i = 0; i < numMenuSlots; i++)
       free (responseMenu[i]);
    free (responseMenu);

    //  Done
    //
    if (choice == 0)
        return NULL;
    
    //  Bring up the header dialog
    //
    else if (choice == 1)
        return CreateStructEditor(&headerDesc, headerStruct, data);
    
    //  Bring up the specific dialog for step chosen
    //
    else if (choice >= 2 && choice < numMenuSlots)
        return CreateStructEditor(&stepDesc, &gStepStructDesc, &steps[choice - 2]);

    return NULL;
}

