/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

////////////////////////////////////////////////////////////////////////////////
// $Header: r:/t2repos/thief2/src/sim/wrbsp.cpp,v 1.11 2000/02/19 13:27:49 toml Exp $
//
// Run-time worldrep bsp tree
//

#include <lg.h>
#include <math.h>
#include <allocapi.h>
#include <hashpp.h>
#include <hshpptem.h>
#include <dynarray.h>
#include <wr.h>
#include <portal.h>

#include <media.h>
#include <bspdata.h>
#include <wrbsp.h>

#include <mprintf.h>
#include <memall.h>
#include <dbmem.h>   // must be last header! 

////////////////////////////////////////////////////////////////////////////////

cDynArray<wrBspNode> wrBspTree;

wrBspNode *g_wrBspTree = NULL;
int g_wrBspTreeSize = 0;

struct sPostSplit
{
   int cell_id;
   BspNode *n;
};

cDynArray<sPostSplit> gPostSplitList;

////////////////////////////////////////////////////////////////////////////////

cDynArray<PortalPlane *> gExtraPlaneList;

void ClearExtraPlanes()
{
   for (int i=0; i<gExtraPlaneList.Size(); i++)
   {
      if (gExtraPlaneList[i] != NULL)
      {
         delete gExtraPlaneList[i];
         gExtraPlaneList[i] = NULL;
      }
   }
   gExtraPlaneList.SetSize(0);
}

PortalPlane *AddExtraPlane(mxs_vector plane_norm, mxs_real plane_const)
{
   PortalPlane *pNewPlane = new PortalPlane;

   pNewPlane->normal = plane_norm;
   pNewPlane->plane_constant = plane_const;

   LGALLOC_PUSH_CREDIT();
   gExtraPlaneList.Append(pNewPlane);
   LGALLOC_POP_CREDIT();

   return pNewPlane;
}

////////////////////////////////////////////////////////////////////////////////

PortalPlane *FindPlaneInCell(mxs_vector *plane_norm, mxs_real plane_const, int cell_id)
{
   PortalCell *pCell = WR_CELL(cell_id);
   Assert_(pCell);

   mxs_vector norm_delta;
  
   for (int i=0; i<pCell->num_planes; i++)
   {
      if (fabs(plane_const - pCell->plane_list[i].plane_constant) < 0.00001)
      {
         mx_sub_vec(&norm_delta, &pCell->plane_list[i].normal, plane_norm);

         if (mx_mag2_vec(&norm_delta) < 0.00001)
            return &pCell->plane_list[i];
      }
   }

   return NULL;
}

BOOL FindPortalPlaneRec(mxs_vector *plane_norm, mxs_real plane_const, BspNode *n, PortalPlane **pPlane, BOOL *reversed)
{
   if (n->leaf)
   {
      if (n->cell_id > 0)
      {

         if ((*pPlane = FindPlaneInCell(plane_norm, plane_const, n->cell_id - 1)) != NULL)
         {
            *reversed = FALSE;
            return TRUE;
         }

         mxs_vector pnorm;
         mxs_real pconst;

         mx_scale_vec(&pnorm, plane_norm, -1.0);
         pconst = plane_const * -1.0;

         if ((*pPlane = FindPlaneInCell(&pnorm, pconst, n->cell_id - 1)) != NULL)
         {
            *reversed = TRUE;
            return TRUE;
         }

         return FALSE;
      }
      else
         return FALSE;
   }
   else
   {
      if (FindPortalPlaneRec(plane_norm, plane_const, n->inside, pPlane, reversed))
         return TRUE;

      return FindPortalPlaneRec(plane_norm, plane_const, n->outside, pPlane, reversed);
   }
}

BOOL FindPortalPlane(BspNode *n, PortalPlane **pPlane, int *reversed)
{
   BOOL found = FALSE;

   mxs_vector plane_norm;
   mxs_vector rev_plane_norm;
   mxs_real   plane_const =  n->split_plane.d;
   mxs_real   rev_plane_const = -n->split_plane.d;

   mx_mk_vec(&plane_norm, n->split_plane.a, n->split_plane.b, n->split_plane.c);
   mx_mk_vec(&rev_plane_norm, -n->split_plane.a, -n->split_plane.b, -n->split_plane.c);

   if (!FindPortalPlaneRec(&plane_norm, plane_const, n, pPlane, reversed))
   {
      for (int i=0; i<wr_num_cells; i++)
      {
         if ((*pPlane = FindPlaneInCell(&plane_norm, plane_const, i)) != NULL)
         {
            *reversed = FALSE;
            found = TRUE;
            break;
         }

         if ((*pPlane = FindPlaneInCell(&rev_plane_norm, rev_plane_const, i)) != NULL)
         {
            *reversed = TRUE;
            found = TRUE;
            break;
         }
      }
   }
   else
      found = TRUE;

   // Add to our own plane list, if it wasn't found in the world
   if (!found)
   {
      *pPlane = AddExtraPlane(plane_norm, plane_const);
      *reversed = FALSE;

      found = TRUE;
   }

   return found;
}

////////////////////////////////////////////////////////////////////////////////

//
// Allocate a new leaf and initialize it
//
uint wrBspLeafCreate(uint parent_index, int cell_id)
{
   wrBspNode new_node;

   new_node.flags = 0;

   wrBspSetLeaf(&new_node);
   wrBspUnmark(&new_node);
   wrBspSetNormal(&new_node);

   wrSetParent(&new_node, parent_index);
   new_node.plane = NULL;

   new_node.cell_id = cell_id;

   LGALLOC_AUTO_CREDIT();
   return wrBspTree.Append(new_node);  
}

//
// Allocate a new node and initialize it
//  
uint wrBspNodeCreate(BspNode *n, int cell_id, uint parent_index)
{
   wrBspNode new_node;

   new_node.flags = 0;

   wrBspSetNode(&new_node);
   wrBspUnmark(&new_node);

   wrSetParent(&new_node, parent_index);

   BOOL reversed;
   FindPortalPlane(n, &new_node.plane, &reversed);

   AssertMsg(new_node.plane, "Unable to find matching splitplane!\n");

   if (reversed)
      wrBspSetReversed(&new_node);
   else
      wrBspSetNormal(&new_node);

   LGALLOC_AUTO_CREDIT();
   return wrBspTree.Append(new_node); 
}

//
// Search the tree for a leaf with the specified cell_id
// and return a pointer to the leaf
//
uint wrBspFindLeaf(int cell_id)
{
   for (int i=0; i<wrBspTree.Size(); i++)
   {
      if (wrBspIsLeaf(&wrBspTree[i]) && (wrBspTree[i].cell_id == cell_id))
         return i;
   }

   return WRBSP_INVALID;
}

//
// Point each cell at its leaf in the worldrep BSP
// 
void wrBspTreeRefCells(uint node_index)
{
   if (node_index == WRBSP_INVALID)
      return;

   if (wrBspTree.Size() == 0)
      return;

   wrBspNode *pCurNode = &wrBspTree[node_index];

   if (wrBspIsLeaf(pCurNode))
   {
      if (pCurNode->cell_id != -1)
         WR_CELL(pCurNode->cell_id)->leaf_index = node_index;
   }
   else
   {
      wrBspTreeRefCells(pCurNode->inside_index);
      wrBspTreeRefCells(pCurNode->outside_index);
   }
}

//
// Recursively copies the CSG's BSP tree.  At each recursion it
// allocates the appropriate structure and copies it.  Returned
// from each level of recursion is a pointer to the element 
// created from the returning level of recursion.  Note that a 
// pointer from the current level of recursion is passed to the 
// next level of recursion so that children can have links to 
// their parents.
//
// When first calling, parent should be passed in as NULL.
//
// Also note that when this copy is taking place the cells
// have not yet been numbered or emitted.  I am assuming that
// they will be numbered according to the existing CSG BSP
// tree (inorder traversal). Additional splits due to high-
// poly-count-cells will go to dummy nodes, whose numbers 
// come *after* the original CSG BSP's cell's numbers.
// 
// Of course, for this BSP tree, there will be no dummy
// nodes -- splits will actually create nodes in the tree.
// But that's handled later.
//
uint wrBspTreeCopy(BspNode *node, uint parent_id)
{
   uint new_node_index;

   // If it's the first recursion, initialize the BSP tree
   if (parent_id == WRBSP_INVALID) 
   {
      wrBspTree.SetSize(0);
      g_wrBspTree = NULL;
      g_wrBspTreeSize = 0;

      ClearExtraPlanes();
   }

   if (node->ph) 
   {
      // Create leaf, but only assign a cell number if it's 
      // a non-solid (marked) leaf
      if (IS_MARKED(node))
         new_node_index = wrBspLeafCreate(parent_id, node->cell_id - 1);
      else
         new_node_index = WRBSP_INVALID;
   } 
   else 
   {
      uint ret_index; // to enforce order of evaulation of assignment

      new_node_index = wrBspNodeCreate(node, node->cell_id, parent_id);

      AssertMsg(new_node_index != WRBSP_INVALID, "Invalid cration node index");

      ret_index = wrBspTreeCopy(node->inside, new_node_index);
      wrBspTree[new_node_index].inside_index = ret_index;

      ret_index = wrBspTreeCopy(node->outside, new_node_index);
      wrBspTree[new_node_index].outside_index = ret_index;
   }

   if (parent_id == WRBSP_INVALID)
   {
      wrBspTreeApplyPostSplits();

      if (wrBspTree.Size() > 0)
         g_wrBspTree = &wrBspTree[0];
      else
         g_wrBspTree = NULL;

      g_wrBspTreeSize = wrBspTree.Size();

      mprintf("  %d extra bsp planes created\n", gExtraPlaneList.Size());
   }    

   return new_node_index;
}

//
// Deallocates a worldrep BSP tree
//
void wrBspTreeDeallocate()
{
   wrBspTree.SetSize(0);
   g_wrBspTree = NULL;
   g_wrBspTreeSize = 0;
   ClearExtraPlanes(); 
}

//
// Dumps the tree ever-so-ineligantly to the mono
//
void wrBspTreeDumpRec(uint node_index)
{
   if (node_index == WRBSP_INVALID)
      return;

   wrBspNode *pCurNode = &wrBspTree[node_index];

   if (wrBspIsLeaf(pCurNode))
   {
      mprintf("  LEAF  (%d)  %c\n", node_index, wrBspIsMarked(pCurNode) ? '*' : ' ');
      mprintf("    parent_id = %d\n", wrParentIndex(pCurNode));
      mprintf("    cell_id = %d\n", pCurNode->cell_id);
   }
   else
   {
      mprintf("  NODE  (%d)  %s\n", node_index, wrBspIsReversed(pCurNode) ? "rev" : "   ");
      mprintf("    %gx + %gy + %gz + %g\n", pCurNode->plane->normal.x, 
                                            pCurNode->plane->normal.y, 
                                            pCurNode->plane->normal.z,
                                            pCurNode->plane->plane_constant);
      wrBspTreeDumpRec(pCurNode->inside_index);
      wrBspTreeDumpRec(pCurNode->outside_index);
   }
}

void wrBspTreeDump()
{
   wrBspTreeDumpRec(WRBSP_HEAD);
}


////////////////////////////////////////////////////////////////////////////////

void wrBspTreeAddPostSplit(BspNode *n, int cell_id)
{
   sPostSplit postSplit;

   postSplit.cell_id = cell_id;
   postSplit.n = n;

   LGALLOC_AUTO_CREDIT();
   gPostSplitList.Append(postSplit);
}

void wrBspTreeApplyPostSplits()
{
   for (int i=0; i<gPostSplitList.Size(); i++)
   {
      uint split_index = wrBspFindLeaf(gPostSplitList[i].cell_id);

      AssertMsg1(split_index != WRBSP_INVALID, "Unable to find cell %d to split!\n", gPostSplitList[i].cell_id);

      wrBspNode *pSplitLeaf = &wrBspTree[split_index];

      BOOL reversed;
      FindPortalPlane(gPostSplitList[i].n, &pSplitLeaf->plane, &reversed);

      if (reversed)
         wrBspSetReversed(pSplitLeaf);
      else
         wrBspSetNormal(pSplitLeaf);

      // Convert to a node
      wrBspSetNode(pSplitLeaf);
      wrBspUnmark(pSplitLeaf);

      uint ret_index;

      ret_index = wrBspLeafCreate(split_index, gPostSplitList[i].cell_id);
      wrBspTree[split_index].inside_index = ret_index;

      ret_index = wrBspLeafCreate(split_index, gPostSplitList[i].n->cell_id - 1);
      wrBspTree[split_index].outside_index = ret_index;
   }

   gPostSplitList.SetSize(0);
}


////////////////////////////////////////////////////////////////////////////////

typedef struct
{
   int cell_id;
   int plane_id;
} sCellPlane;

typedef cHashTableFunctions<long> LongHashFunctions;
typedef cHashTable<long, sCellPlane, LongHashFunctions> cCellPlaneTable;

cCellPlaneTable gCellPlaneTable;

#ifdef _MSC_VER
template cCellPlaneTable;
#endif

void wrBspTreeWrite(PortalReadWrite write)
{
   int i, j;
   sCellPlane cellplane;

   gCellPlaneTable.Clear();

   // First build a table of plane pointers to cell-plane ids
   for (i=0; i<wr_num_cells; i++)
   {
      PortalCell *pCell = WR_CELL(i);
      Assert_(pCell);

      for (j=0; j<pCell->num_planes; j++)
      {
         cellplane.cell_id = i;
         cellplane.plane_id = j;

         gCellPlaneTable.Set((long)(&pCell->plane_list[j]), cellplane);
      }
   }

   // Write out our extra planes
   int size = gExtraPlaneList.Size();

   write(&size, sizeof(int), 1);
   for (i=0; i<size; i++)
   {
      PortalPlane* plane = gExtraPlaneList[i]; 
      // You kids are so cute, writing out each field like this
      write(&plane->normal,sizeof(plane->normal),1); 
      write(&plane->plane_constant,sizeof(plane->plane_constant),1); 
   }
      
   // Then write out our array, replacing plane pointers will cellplanes
   write(&g_wrBspTreeSize, sizeof(int), 1);

   for (i=0; i<g_wrBspTreeSize; i++)
   {
      write(&g_wrBspTree[i].parent_index, sizeof(uint), 1);

      cellplane.cell_id = -1;
      cellplane.plane_id = -1;

      if (g_wrBspTree[i].plane)
      {
         if (!gCellPlaneTable.Lookup((long)g_wrBspTree[i].plane, &cellplane))
         {
            // Must be an extra plane
            for (int j=0; j<gExtraPlaneList.Size(); j++)
            {
               if (g_wrBspTree[i].plane == gExtraPlaneList[j])
               {
                  cellplane.plane_id = j;
                  break;
               }
            }
         }

         AssertMsg(cellplane.plane_id != -1, "Unable to match plane pointer in BSP save");
      }

      write(&cellplane, sizeof(sCellPlane), 1);

      write(&g_wrBspTree[i].inside_index, sizeof(uint), 1);
      write(&g_wrBspTree[i].outside_index, sizeof(uint), 1);
   }

   // And clean up

   gCellPlaneTable.Clear();
}


void wrBspTreeRead(PortalReadWrite read)
{
   sCellPlane cellplane;
   int i;

   int size;
   
   read(&size, sizeof(int), 1);

   gExtraPlaneList.SetSize(size); 
   for (i=0; i<size; i++)
   {
      PortalPlane* plane = new PortalPlane; 
      read(&plane->normal,sizeof(plane->normal),1);
      read(&plane->plane_constant,sizeof(plane->plane_constant),1);

      gExtraPlaneList[i] = plane; 
   }

   read(&g_wrBspTreeSize, sizeof(int), 1);

   LGALLOC_PUSH_CREDIT();
   wrBspTree.SetSize(g_wrBspTreeSize);
   LGALLOC_POP_CREDIT();

   if (g_wrBspTreeSize > 0)
      g_wrBspTree = &wrBspTree[0];
   else
      g_wrBspTree = NULL;

   for (i=0; i<g_wrBspTreeSize; i++)
   {
      read(&g_wrBspTree[i].parent_index, sizeof(uint), 1);

      read(&cellplane, sizeof(sCellPlane), 1);

      if (cellplane.cell_id == -1)
      {
         if (cellplane.plane_id == -1)
            g_wrBspTree[i].plane = NULL;
         else
            g_wrBspTree[i].plane = gExtraPlaneList[cellplane.plane_id];
      }
      else
         g_wrBspTree[i].plane = &WR_CELL(cellplane.cell_id)->plane_list[cellplane.plane_id];
   
      read(&g_wrBspTree[i].inside_index, sizeof(uint), 1);
      read(&g_wrBspTree[i].outside_index, sizeof(uint),1);
   }

}

















