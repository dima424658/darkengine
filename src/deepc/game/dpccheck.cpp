/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

#include <dpccheck.h>

#include <matrix.h>
#include <phcore.h>
#include <phmod.h>
#include <phmodobb.h>
#include <phmods.h>
#include <physapi.h>
#include <sphrcst.h>
#include <wrtype.h>

//////////////////////////////////////////////////////////////
//
// Test for sphere intersecting doors
//

BOOL DPCCheckSphereIntersectsDoor(mxs_vector &vec, float radius)
{
    mxs_vector norm_list[6];
    mxs_real   const_list[6];

    cPhysModel *pIterModel;
    cPhysOBBModel *pDoorModel;

    pIterModel = g_PhysModels.GetFirstActive();
    while (pIterModel)
    {
        if (pIterModel->IsDoor())
        {
            pDoorModel = (cPhysOBBModel *)pIterModel;

            if (mx_dist_vec(&vec, &pDoorModel->GetLocationVec()) < (radius + mx_mag_vec(&pDoorModel->GetEdgeLengths())))
            {
                pDoorModel->GetNormals(norm_list);
                pDoorModel->GetConstants(const_list, norm_list);

                BOOL inside = TRUE;

                for (int i=0; i<6 && inside; i++)
                {
                    mxs_real dist = mx_dot_vec(&vec, &norm_list[i]) - (const_list[i] + radius);

                    if (dist > 0.0)
                    {
                        inside = FALSE;
                        break;
                    }
                }

                if (inside)
                    return TRUE;
            }
        }

        pIterModel = g_PhysModels.GetNextActive(pIterModel);
    }
    return FALSE;
}

///////////////////////////////////////////////////
// Check start location for sphere
//

BOOL DPCCheckSphereLocation(const Location& testLoc, float radius)
{
    // Check start location
    if (!SphrSphereInWorld((Location*)(&testLoc), radius, 0))
        return FALSE;

    // Check start location against doors
    if (DPCCheckSphereIntersectsDoor((mxs_vector&)(testLoc.vec), radius))
        return FALSE;

    return TRUE;
}
