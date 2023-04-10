/*
@Copyright Looking Glass Studios, Inc.
1996,1997,1998,1999,2000 Unpublished Work.
*/

// $Header: r:/t2repos/thief2/src/shock/shkcam.cpp,v 1.11 1999/01/19 17:21:06 XEMU Exp $

#include <shkcmapi.h>

#include <appagg.h>
#include <aggmemb.h>

#include <creatext.h>

#include <camera.h>
#include <playrobj.h>
#include <iobjsys.h>
#include <objpos.h>
#include <wrtype.h>
#include <objdef.h>
#include <physapi.h>
#include <rendprop.h>
#include <collprop.h>

#include <gen_bind.h>
#include <contexts.h>
#include <shkcntxt.h>

#include <shkovrly.h>
#include <shkovcst.h>
#include <shklbox.h>

#include <dynarray.h>
#include <simtime.h>
#include <dlist.h>
#include <findhack.h>  // all the abstracted lg_find stuff
#include <hashset.h>
#include <hshsttem.h>
#include <str.h>
#include <datapath.h>
#include <config.h>
#include <cfgdbg.h>
#include <mprintf.h>
#include <qt.h>

// Include these absolutely last
#include <dbmem.h>
#include <initguid.h>
#include <shkcmiid.h>

// why do i have to define this everywhere?
#define min(x, y) (((x)<(y))?(x):(y))

//------------------------------------------------------------
// Camera controls
//

const kMaxCameras = 10;
const float kCameraRadius = 0;
const float kDefaultCameraSpeed = 0.2;

// list for storing camera file info
class cFileName: public cDListNode<cFileName,1>
{
public:
   cStr m_fileName;
   cStr m_pathName;
};

class cFileNameList: public cDList<cFileName,1>
{
public:
   cFileName *Find(const char *pFileName) const
   {
      cFileName *pNode = GetFirst();
      
      while (pNode != NULL)
      {
         if (pNode->m_fileName == pFileName) 
            return pNode;
         pNode = pNode->GetNext();
      }
      return NULL;
   }
};

// for recording camera positions
struct sPositionSample
{
   mxs_vector m_pos;
   mxs_angvec m_orient;
};

struct sPositionTimeSample: public sPositionSample
{
   tSimTime m_time;
};

typedef cDynArray<sPositionTimeSample> tRawSampleArray;
typedef cDynArray<sPositionSample> tSampleArray;

const int kSamplesGrowSize = 100;
const int kRawSamplesGrowSize = 100;

// we store samples at this delta time
const tSimTime kSampleDelta = 25;

// for recording camera switches
struct sSwitchTimeSample
{
   int m_cameraNum;
   tSimTime m_time;
};
const int kSwitchesGrowSize = 10;

class cShockCamera: 
   public cCTDelegating<IShockCamera>, 
   public cCTAggregateMemberControl<kCTU_Default>
{
private:
   eCameraEditType m_camMode;
   eCameraEditType m_camState;

   int m_numCameraFiles;       // number of cameras files for scene
   int m_numLoadedCameras; // number of cameras actually loaded for scene
   int m_cameraFileNum;
   int m_cameraFileNums[kMaxCameras];  // the file number associated with each camera number
   int m_currentCamera;
   cFileNameList m_fileNameList;
   cStr m_sceneName;
   ObjID m_camID;

   // camera data
   tSimTime m_startTime;
   tSimTime m_endTime;
   int m_numRawSamples;
   tRawSampleArray m_rawSamples;
   int m_numSamples[kMaxCameras];
   tSampleArray m_samples[kMaxCameras];
   cDynArray<sSwitchTimeSample> m_switchTimes;
   int m_numSwitches;
   int m_lastSwitch;
   float m_cameraSpeed;

   void ScanScene(const char *pszDataPath, const char *pszWhat);
   void ScanScene(const char *pszWhat);
   void ScanScenes(cFileNameList &sceneList, const char *pDataPath);
   HRESULT LoadScene(const char *pszDataPath, const char *pszWhat);
   HRESULT LoadScene(const char *pszWhat);
   HRESULT SaveScene(const char *pszWhat);
   HRESULT SaveSamples(THIS_ const char *pName);
   void AddSample(int cameraNum, const sPositionSample *pSample);
   HRESULT LoadSamples(THIS_ const char *pName);
   void LoadAllSamples(void);
   void DestroyAllSamples(void);
   HRESULT FindNextFreeCameraFile(THIS_);

   void RestoreDefaultCamera(THIS_);
   void SwitchToNewCamera(THIS_);

   void PlaybackSwitch(THIS_);
   HRESULT SaveSwitches(THIS_ const char *pName);
   HRESULT LoadSwitches(THIS_ const char *pName);
   void RecordSwitch(tSimTime time, int cameraNum);

   void UpdateCameraPos(THIS_);
   void MoveCameraToPlayer(THIS_);
   
   void StartSampling(THIS_);

   // debugging
   void DumpSceneInfo(THIS_);

public:

   cShockCamera(IUnknown* pOuter):
      m_camMode(kCameraNormal),
      m_camState(kCameraPlayback),
      m_numCameraFiles(0),
      m_numLoadedCameras(0),
      m_numRawSamples(0),
      m_camID(NULL),
      m_cameraSpeed(kDefaultCameraSpeed)
   {
      m_rawSamples.SetSize(kRawSamplesGrowSize);
      MI_INIT_AGGREGATION_1(pOuter, IShockCamera, kPriorityNormal, NULL);
   }
   
   STDMETHOD(SetLocation)(THIS_ const mxs_vector *pPos, const mxs_angvec *pAng);
   STDMETHOD_(BOOL, Attach)(THIS_ ObjID objID);
   STDMETHOD_(BOOL, Attach)(THIS_ const Label *pName);
   STDMETHOD_(BOOL, Attach)(THIS_ const char *pName);
   STDMETHOD(Detach)(THIS_);
   STDMETHOD_(ObjID, GetAttachObject)(THIS_);

// state is what the user defines the camera to do when we start a cut-scene
// mode is what the camera is actually doing right now
   STDMETHOD(SetEditState)(THIS_ eCameraEditType state);
   STDMETHOD(SetScene)(THIS_ const char *pSceneName);
   STDMETHOD(SetEditMode)(THIS_ eCameraEditType mode);
   STDMETHOD_(eCameraEditType, GetEditState)(THIS);
   STDMETHOD_(eCameraEditType, GetEditMode)(THIS);

   STDMETHOD(SwitchCamera)(int cameraNum);
   STDMETHOD_(int, GetNumCameras)(THIS);

   STDMETHOD(SetCameraSpeed)(float speed);
   STDMETHOD_(float, GetCameraSpeed)(void);

   STDMETHOD(SaveScene)(THIS) {return SaveScene(m_sceneName);}
   STDMETHOD(MakeScenes)(THIS);

   STDMETHOD(Frame)(THIS);
};

struct sSceneHeader
{
   int numCameras;
   int numSamples[kMaxCameras];
   int numSwitches;
};

STDMETHODIMP cShockCamera::SetLocation(const mxs_vector *pPos, const mxs_angvec *pAng)
{
   CameraSetLocation(PlayerCamera(), (mxs_vector*)pPos, (mxs_angvec*)pAng);
   CameraDetach(PlayerCamera());
   return S_OK;
}

STDMETHODIMP_(BOOL) cShockCamera::Attach(ObjID objID)
{
   CameraAttach(PlayerCamera(), objID);
   return TRUE;
}

STDMETHODIMP_(BOOL) cShockCamera::Attach(const Label *pName)
{
   ObjID objID;
   AutoAppIPtr_(ObjectSystem, pObjSys);

   if ((objID = pObjSys->GetObjectNamed((const char*)pName)) != OBJ_NULL)
   {
      CameraAttach(PlayerCamera(), objID);
      return TRUE;
   }
   return FALSE;
}

STDMETHODIMP_(BOOL) cShockCamera::Attach(const char *pName)
{
   return Attach((const Label*)pName);
}

STDMETHODIMP cShockCamera::Detach(void)
{
   CameraDetach(PlayerCamera());
   return S_OK;
}

STDMETHODIMP_(ObjID) cShockCamera::GetAttachObject(void)
{
   return CameraGetObjID(PlayerCamera());
}

STDMETHODIMP cShockCamera::SetScene(const char *pSceneName)
{
   m_sceneName.Empty();
   m_sceneName = pSceneName;
   return S_OK;
}

static void ChangeOverlays(BOOL camera)
{
   ShockLetterboxSet(camera);
}

STDMETHODIMP cShockCamera::SetEditMode(eCameraEditType mode)
{
   m_endTime = GetSimTime();

   if (mode == kCameraNormal)
   {
      RemoveIBHandler();
      InstallIBHandler (HK_GAME_MODE, UI_EVENT_KBD_RAW | UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE | UI_EVENT_JOY, FALSE);
   }
   else
   {
      RemoveIBHandler();
      InstallIBHandler (HK_RECORD_MODE, UI_EVENT_KBD_RAW | UI_EVENT_MOUSE | UI_EVENT_MOUSE_MOVE | UI_EVENT_JOY, FALSE);
   }
   switch(mode)
   {
   case kCameraNormal:
      if ((m_camMode == kCameraRecord) || (m_camMode == kCameraEdit) || (m_camMode == kCameraPlayback))
      {
         if (m_camMode == kCameraRecord)
            SaveSamples(m_sceneName);
         else if (m_camMode == kCameraEdit)
            SaveSwitches(m_sceneName);
         RestoreDefaultCamera();
      }
      ChangeOverlays(FALSE);
//      CloseCameras();
      break;
   case kCameraPlayback:
      if (FAILED(LoadScene(m_sceneName)))
      {
         ScanScene(m_sceneName);
         LoadAllSamples();
         LoadSwitches(m_sceneName);
      }
      SwitchCamera(0);
      ChangeOverlays(TRUE);
      break;
   case kCameraRecord:
      ScanScene(m_sceneName);
      if (m_numCameraFiles==kMaxCameras)
      {
         Warning(("Too many cameras for scene %s\n", m_sceneName));
         return E_FAIL;
      }
      else
      {
         FindNextFreeCameraFile();
         SwitchToNewCamera();
         MoveCameraToPlayer();
         StartSampling();
      }
      DumpSceneInfo();
      ChangeOverlays(TRUE);
      break;
   case kCameraEdit:
      ScanScene(m_sceneName);
      LoadAllSamples();
      m_numSwitches = 0;
      SwitchCamera(0);
      ChangeOverlays(TRUE);
      break;
   }
   m_camMode = mode;
   m_startTime = GetSimTime();
   return S_OK;
}

STDMETHODIMP cShockCamera::SetEditState(eCameraEditType state)
{
   m_camState = state;
   return S_OK;
}

STDMETHODIMP_(eCameraEditType) cShockCamera::GetEditMode(void)
{
   return m_camMode;
}

STDMETHODIMP_(eCameraEditType) cShockCamera::GetEditState(void)
{
   return m_camState;
}

STDMETHODIMP cShockCamera::Frame(void)
{
   switch (m_camMode)
   {
   case kCameraRecord:
      if ((m_numRawSamples)>=m_rawSamples.Size())
      {
         m_rawSamples.SetSize(m_rawSamples.Size()+kRawSamplesGrowSize);
         Assert_(m_numRawSamples<m_rawSamples.Size());
      }
      m_rawSamples[m_numRawSamples].m_pos = ObjPosGet(m_camID)->loc.vec;
      m_rawSamples[m_numRawSamples].m_orient = ObjPosGet(m_camID)->fac;
      m_rawSamples[m_numRawSamples].m_time = GetSimTime();
      ++m_numRawSamples;
      break;
   case kCameraPlayback:
      PlaybackSwitch();
   case kCameraEdit:
      UpdateCameraPos();
      break;
   default:
      break;
   }
   return S_OK;
}

void cShockCamera::ScanScene(const char *pszDataPath, const char *pszWhat)
{
   Datapath sDatapath;
   DatapathDir *pDatapathDir;
   char *pszFileName;
   cFileName *pCameraFile;
   cStr wildCardStr;
   int idx;

   DatapathClear(&sDatapath);
   DatapathAdd(&sDatapath, (char*)pszDataPath);
   wildCardStr.FmtStr("%s_*.cam", pszWhat);
   pDatapathDir = DatapathOpenDir(&sDatapath, (char*)((const char*)wildCardStr), DP_SCREEN_DOT);
   while (NULL!=(pszFileName = DatapathReadDir(pDatapathDir)))
   {
      pCameraFile = new cFileName;
      pCameraFile->m_fileName = pszFileName;
      (pCameraFile->m_fileName).MakeLower();
      if (((idx = (pCameraFile->m_fileName).Find('_')) == -1) || ((pCameraFile->m_fileName).GetLength()<=idx))
      {
         delete pCameraFile;
         continue;
      }
      if (m_fileNameList.Find((const char*)(pCameraFile->m_fileName)) == NULL)
      {
         pCameraFile->m_pathName = pszDataPath;
         m_fileNameList.Append(pCameraFile);
         // idx+1 is index of digit after _ in file name
         m_cameraFileNums[m_numCameraFiles] = atoi(&(pCameraFile->m_fileName[idx+1]));
         ++m_numCameraFiles;
      }
      else
         delete pCameraFile;
   }
   DatapathCloseDir(pDatapathDir);
   DatapathFree(&sDatapath);
}

void cShockCamera::AddSample(int cameraNum, const sPositionSample *pSample)
{
   tSampleArray *pSamples = &m_samples[cameraNum];

   if (pSamples->Size()<=m_numSamples[cameraNum])
      pSamples->Grow(kSamplesGrowSize);
   Assert_(pSamples->Size()>m_numSamples[cameraNum]);
   (*pSamples)[m_numSamples[cameraNum]++] = *pSample;
}

HRESULT cShockCamera::LoadSamples(const char *pFileName)
{
   FILE *pFile = fopen(pFileName, "r");
   sPositionSample sample;
   int i = 0;

   Assert_(m_numLoadedCameras<kMaxCameras-1);

   if (pFile == NULL)
   {
      Warning(("LoadSamples: bad file name %s\n", pFileName));
      return E_FAIL;
   }
   while (fscanf(pFile, "%f %f %f %d %d %d\n", &sample.m_pos.x, &sample.m_pos.y, &sample.m_pos.z, 
      &sample.m_orient.tx, &sample.m_orient.ty, &sample.m_orient.tz) == 6)
   {
      AddSample(m_numLoadedCameras, &sample);
   }
   if (m_samples[m_numLoadedCameras].Size()>m_numSamples[m_numLoadedCameras])
      m_samples[m_numLoadedCameras].SetSize(m_numSamples[m_numLoadedCameras]);
   fclose(pFile);
   ++m_numLoadedCameras;
   return S_OK;
}

void cShockCamera::DestroyAllSamples(void)
{
   for (int i=0; i<kMaxCameras; i++)
   {
      m_samples[i].SetSize(0);
      m_numSamples[i] = 0;
   }
   m_numLoadedCameras = 0;
}

void cShockCamera::LoadAllSamples(void)
{
   cStr fullName;
   cFileName *pCameraFile;

   DestroyAllSamples();
   pCameraFile = m_fileNameList.GetFirst();
   while (pCameraFile != NULL)
   {
      fullName.FmtStr("%s\\%s", (const char*)(pCameraFile->m_pathName), 
                      (const char*)(pCameraFile->m_fileName));
      m_fileNameList.Remove(pCameraFile);
      delete pCameraFile;
      LoadSamples((const char*)fullName);
      pCameraFile = m_fileNameList.GetFirst();
   }
}

// load the entire scene (cameras & switches) from a single binary file
HRESULT cShockCamera::LoadScene(const char *pszDataPath, const char *pszWhat)
{
   Datapath sDatapath;
   DatapathDir *pDatapathDir;
   char *pszFileName;
   sSceneHeader header;
   sPositionSample sample;
   sSwitchTimeSample switchSample;
   FILE *pFile;
   cStr baseFileName;
   cStr fullFileName;

   DatapathClear(&sDatapath);
   DatapathAdd(&sDatapath, (char*)pszDataPath);
   baseFileName.FmtStr("%s.bcm", pszWhat);
   pDatapathDir = DatapathOpenDir(&sDatapath, (char*)((const char*)baseFileName), DP_SCREEN_DOT);
   if ((pszFileName = DatapathReadDir(pDatapathDir)) != NULL)
   {
      fullFileName.FmtStr("%s\\%s", pszDataPath, pszFileName);
      if ((pFile = fopen((const char*)fullFileName, "rb")) == NULL)
      {
         Warning(("Can't open file %s\n", (const char*)fullFileName));
         return E_FAIL;
      }
      DestroyAllSamples();
      int checkSum = fread((void*)&header, sizeof(sSceneHeader), 1, pFile);
      Assert_(checkSum == 1);
      for (int i=0; i<header.numCameras; i++)
      {
         m_numSamples[i] = 0;
         // this is an item-by-item read
         // we may want to turn into an array read
         for (int j=0; j<header.numSamples[i]; j++)
         {
            checkSum = fread((void*)&sample, sizeof(sPositionSample), 1, pFile);
            Assert_(checkSum == 1);
            AddSample(m_numLoadedCameras, &sample);
         }
         ++m_numLoadedCameras;
      }
      m_numSwitches = 0;
      m_lastSwitch = 0;
      for (i=0; i<header.numSwitches; i++)
      {
         checkSum = fread((void*)&switchSample, sizeof(sSwitchTimeSample), 1, pFile);
         Assert_(checkSum == 1);
         RecordSwitch(switchSample.m_time, switchSample.m_cameraNum);
      }
      fclose(pFile);
   }
   DatapathCloseDir(pDatapathDir);
   DatapathFree(&sDatapath);
   if (pszFileName == NULL)
      return E_FAIL;
   else
      return S_OK;
}

// save the entire scene (cameras & switches) to a single binary file
HRESULT cShockCamera::SaveScene(const char *pName)
{
   FILE *pFile;
   sSceneHeader header;
   cStr fileName;

   fileName.FmtStr(".\\cameras\\%s.bcm", pName);
   if ((pFile = fopen((const char*)fileName, "wb")) == NULL)
   {
      Warning(("Can't open file %s\n", (const char*)fileName));
      return E_FAIL;
   }
   header.numCameras = m_numLoadedCameras;
   for (int i=0; i<header.numCameras; i++)
      header.numSamples[i] = m_numSamples[i];
   header.numSwitches = m_numSwitches;
   int checkSum = fwrite((void*)&header, sizeof(sSceneHeader), 1, pFile);
   Assert_(checkSum == 1);
   for (i=0; i<header.numCameras; i++)
   {
      // this is an item-by-item write
      // we may want to turn into an array write
      for (int j=0; j<header.numSamples[i]; j++)
      {
         checkSum = fwrite((void*)&m_samples[i][j], sizeof(sPositionSample), 1, pFile);
         Assert_(checkSum == 1);
      }
   }
   for (i=0; i<header.numSwitches; i++)
   {
      checkSum = fwrite((void*)&m_switchTimes[i], sizeof(sSwitchTimeSample), 1, pFile);
      Assert_(checkSum == 1);
   }
   fclose(pFile);
   return S_OK;
}

HRESULT cShockCamera::LoadScene(const char *pName)
{
   static char szDataPath[PATH_MAX];
   cStr resCameraPath;

   // open stuff in your current dir
   resCameraPath.FmtStr(".\\cameras");
   if (SUCCEEDED(LoadScene((const char*)resCameraPath, pName)))
      return S_OK;

   // get stuff from res path
   if (config_get_raw("resname_base", szDataPath, sizeof(szDataPath)))
   {
      resCameraPath.FmtStr("%s\\cameras", szDataPath);
      return LoadScene((const char*)resCameraPath, pName);
   }
   return E_FAIL;
}

void cShockCamera::ScanScene(const char *pName)
{
   static char szDataPath[PATH_MAX];
   cStr resCameraPath;

   m_numCameraFiles = 0;
//   m_fileNameList.DestroyAll();
   while (m_fileNameList.GetFirst())
      delete m_fileNameList.Remove(m_fileNameList.GetFirst());

   // open stuff in your current dir
   resCameraPath.FmtStr(".\\cameras");
   ScanScene((const char*)resCameraPath, pName);

   // get stuff from res path
   if (config_get_raw("resname_base", szDataPath, sizeof(szDataPath)))
   {
      resCameraPath.FmtStr("%s\\cameras", szDataPath);
      ScanScene((const char*)resCameraPath, pName);
   }
}

HRESULT cShockCamera::SaveSamples(const char *pszWhat)
{
   cStr fileName;
   tSimTime t;
   int currentSample;
   tSimTime deltaT;
   mxs_vector deltaPos;
   mxs_vector pos;
   mxs_angvec orient;
   float frac;
   mxs_matrix orientMat;
   quat quat1, quat2, quat;

   fileName.FmtStr(".\\cameras\\%s_%d.cam", pszWhat, m_cameraFileNum);
   FILE *pFile = fopen(fileName, "w");
   if (pFile == NULL)
   {
      Warning(("SaveSamples::Can't open file %s for writing\n"));
      return E_FAIL;
   }

   if (m_numRawSamples>0)
      for (t=m_startTime, currentSample=0; t<m_endTime; t+=kSampleDelta)
      {
         while (((currentSample+1)<m_numRawSamples) && (m_rawSamples[currentSample+1].m_time<=t))
            ++currentSample;
         if ((currentSample+1)<m_numRawSamples)
         {
            Assert_(m_rawSamples[currentSample+1].m_time>t);
            // interpolate position
            deltaT = t-m_rawSamples[currentSample].m_time;
            frac = float(deltaT)/float(m_rawSamples[currentSample+1].m_time-m_rawSamples[currentSample].m_time);
            mx_sub_vec(&deltaPos, &m_rawSamples[currentSample+1].m_pos, &m_rawSamples[currentSample].m_pos);
            mx_scaleeq_vec(&deltaPos, frac);
            mx_add_vec(&pos, &m_rawSamples[currentSample].m_pos, &deltaPos);
            // intepolate orientation
            // convert to quaternion
            mx_ang2mat(&orientMat, &m_rawSamples[currentSample].m_orient);
            quat_from_matrix(&quat1, &orientMat);
            mx_ang2mat(&orientMat, &m_rawSamples[currentSample+1].m_orient);
            quat_from_matrix(&quat2, &orientMat);
            // interpolate in quat land
            quat_slerp(&quat, &quat1, &quat2, frac);
            // back to angvec
            quat_to_matrix(&orientMat, &quat);
            mx_mat2ang(&orient, &orientMat);
         }
         else
         {
            mx_copy_vec(&pos, &m_rawSamples[currentSample].m_pos);
            orient.tx = m_rawSamples[currentSample].m_orient.tx;
            orient.ty = m_rawSamples[currentSample].m_orient.ty;
            orient.tz = m_rawSamples[currentSample].m_orient.tz;
         }
         fprintf(pFile, "%f %f %f %d %d %d\n", pos.x, pos.y, pos.z, orient.tx, orient.ty, orient.tz);
      }
   fclose(pFile);
   return S_OK;
}

// scan the set of loaded camera file numbers to find the next free one
HRESULT cShockCamera::FindNextFreeCameraFile(void)
{
   BOOL found;
   int i;

   if (m_numCameraFiles == kMaxCameras)
      return E_FAIL;
   for (m_cameraFileNum = 0; m_cameraFileNum<kMaxCameras; m_cameraFileNum++)
   {
      found = FALSE;
      for (i=0; (i<m_numCameraFiles) && !found; i++)
         if (m_cameraFileNums[i] == m_cameraFileNum)
            found = TRUE;
      if (!found)
         break;
   }
   Assert_(!found);
   return S_OK;
}

void cShockCamera::RestoreDefaultCamera(void)
{
//   PhysCreateDefaultPlayer(PlayerObject());
   ObjSetRenderType(PlayerObject(), kRenderNormally);
   CameraAttach(PlayerCamera(), PlayerObject());

   AutoAppIPtr(ObjectSystem);
   if (m_camID != OBJ_NULL)
      pObjectSystem->Destroy(m_camID);
   m_camID = OBJ_NULL;
}

void cShockCamera::StartSampling(void)
{
   m_numRawSamples = 0;
}

void cShockCamera::SwitchToNewCamera(void)
{
   AutoAppIPtr(ObjectSystem);
   cStr camName;

   // destroy existing camera
   if (m_camID != OBJ_NULL)
      pObjectSystem->Destroy(m_camID);

   // create camera object
   m_camID = pObjectSystem->BeginCreate(ROOT_ARCHETYPE, kObjectConcrete);
   
   // setup object
   PhysRegisterSphere(m_camID, 1, kPMCF_Default, kCameraRadius);
   PhysSetGravity(m_camID, FALSE);
   PhysSetBaseFriction(m_camID, 1);
   PhysSetMass(m_camID, 100);

   pObjectSystem->EndCreate(m_camID);

   camName.FmtStr("%s_%d", m_sceneName, m_cameraFileNum);
   pObjectSystem->NameObject(m_camID, camName);

   Attach(m_camID);

   ObjSetRenderType(PlayerObject(), kRenderNotAtAll);

   ObjSetCollisionType(m_camID, COLLISION_NONE);

// this causes a whole bunch 'o warnings
// we may have to go deal at some stage if the player proves to be a problem here
//   PhysDeregisterModel(PlayerObject());
}

void cShockCamera::DumpSceneInfo(void)
{
   mprintf("Scene %s: %d cameras\n", (const char*)m_sceneName, m_numCameraFiles);
   for (int i=0; i<m_numCameraFiles; i++)
      mprintf(" %d,", m_cameraFileNums[i]);
   mprintf("\n");
}

STDMETHODIMP cShockCamera::SetCameraSpeed(float speed)
{
   m_cameraSpeed = speed;
   return S_OK;
}

STDMETHODIMP_(float) cShockCamera::GetCameraSpeed(void)
{
   return m_cameraSpeed;
}

void cShockCamera::UpdateCameraPos(void)
{
   Assert_(m_currentCamera<m_numLoadedCameras);
   Assert_(m_camID != OBJ_NULL);

   int delta = GetSimTime()-m_startTime;
   Assert_(m_samples[m_currentCamera].Size()>0);
   int index = min(delta/kSampleDelta, m_samples[m_currentCamera].Size()-1);
   ObjPosUpdate(m_camID, &m_samples[m_currentCamera][index].m_pos, &m_samples[m_currentCamera][index].m_orient);

   /*
   mprintf("delta %d, id %d, loc: %g %g %g\n", delta, index,
      m_samples[m_currentCamera][index].m_pos.x,
      m_samples[m_currentCamera][index].m_pos.y,
      m_samples[m_currentCamera][index].m_pos.z);
*/
}

void cShockCamera::MoveCameraToPlayer(void)
{
   ObjPos *pObjPos = ObjPosGet(PlayerObject());
   mxs_vector *pPos = &(pObjPos->loc.vec);
   mxs_angvec *pAng = &(pObjPos->fac);

   // place object
   ObjPosUpdate(m_camID, pPos, pAng);
}

void cShockCamera::PlaybackSwitch(void)
{
   if (m_numSwitches>0)
   {
      while ((m_lastSwitch<m_numSwitches) && (m_switchTimes[m_lastSwitch].m_time<GetSimTime()-m_startTime))
         SwitchCamera(m_switchTimes[m_lastSwitch++].m_cameraNum);
   }
}

HRESULT cShockCamera::SaveSwitches(const char *pszWhat)
{
   cStr fileName;

   fileName.FmtStr(".\\cameras\\%s.swt", pszWhat);
   FILE *pFile = fopen(fileName, "w");
   if (pFile == NULL)
   {
      Warning(("SaveSwitches: can't open switch file %s\n", pszWhat));
      return E_FAIL;
   }
   for (int i=0; i<m_numSwitches; i++)
      fprintf(pFile, "%d %d\n", m_switchTimes[i].m_time, m_switchTimes[i].m_cameraNum);
   fclose(pFile);
   return S_OK;
}

HRESULT cShockCamera::LoadSwitches(const char *pszWhat)
{
   cStr fileName;
   tSimTime time;
   int cameraNum;

   m_numSwitches = 0;
   m_lastSwitch = 0;
   fileName.FmtStr(".\\cameras\\%s.swt", pszWhat);
   FILE *pFile = fopen(fileName, "r");
   if (pFile == NULL)
   {
      Warning(("LoadSwitches: can't open switch file %s\n", (const char*)fileName));
      return E_FAIL;
   }
   while (fscanf(pFile, "%d %d", &time, &cameraNum) == 2)
      RecordSwitch(time, cameraNum);
   fclose(pFile);
   return S_OK;
}

void cShockCamera::RecordSwitch(tSimTime time, int cameraNum)
{
   // if we're paused, overwrite the last switch done at same time
   if ((m_numSwitches>0) && (m_switchTimes[m_numSwitches-1].m_time == time))
      --m_numSwitches;
   // grow the array if we need to
   if (m_numSwitches==m_switchTimes.Size())
      m_switchTimes.Grow(kSwitchesGrowSize);
   Assert_(m_numSwitches<m_switchTimes.Size());
   m_switchTimes[m_numSwitches].m_time = time;
   m_switchTimes[m_numSwitches].m_cameraNum = cameraNum;
   ++m_numSwitches;
}

STDMETHODIMP cShockCamera::SwitchCamera(int cameraNum)
{
   if (cameraNum>=m_numLoadedCameras)
   {
      Warning(("SwitchCamera: bad camera num %d\n", m_currentCamera));
      return E_FAIL;
   }
   SwitchToNewCamera();
   m_currentCamera = cameraNum;
   UpdateCameraPos();
   if (m_camMode == kCameraEdit)
      RecordSwitch(GetSimTime()-m_startTime, cameraNum);
   return S_OK;
}

STDMETHODIMP_(int) cShockCamera::GetNumCameras(void)
{
   return m_numLoadedCameras;
}

void cShockCamera::ScanScenes(cFileNameList &sceneList, const char *pDataPath)
{
   Datapath sDatapath;
   DatapathDir *pDatapathDir;
   cStr wildCardStr;
   char *pFileName;
   cFileName *pSceneFile;
   int idx;

   DatapathClear(&sDatapath);
   DatapathAdd(&sDatapath, (char*)pDataPath);
   wildCardStr = "*.cam";
   pDatapathDir = DatapathOpenDir(&sDatapath, (char*)((const char*)wildCardStr), DP_SCREEN_DOT);
   while (NULL!=(pFileName = DatapathReadDir(pDatapathDir)))
   {
      pSceneFile = new cFileName;
      pSceneFile->m_fileName = pFileName;
      if ((idx = pSceneFile->m_fileName.Find('_')) == -1)
      {
         Warning(("Camera file %s, bad name format\n", (const char*)(pSceneFile->m_fileName)));
         delete pSceneFile;
         continue;
      }
      pSceneFile->m_fileName.SetLength(idx);
      pSceneFile->m_fileName.MakeLower();
      if (sceneList.Find((const char*)(pSceneFile->m_fileName)) == NULL)
         sceneList.Append(pSceneFile);
      else
         delete pSceneFile;
   }
   DatapathCloseDir(pDatapathDir);
   DatapathFree(&sDatapath);
}

STDMETHODIMP cShockCamera::MakeScenes(void)
{
   cFileNameList sceneList;
   cStr fullName;
   char dataPath[PATH_MAX];
   cStr resCameraPath;
   cFileName *pSceneFile;

   ScanScenes(sceneList, ".\\cameras");
   if (config_get_raw("resname_base", dataPath, sizeof(dataPath)))
   {
      resCameraPath.FmtStr("%s\\cameras", dataPath);
      ScanScenes(sceneList, (const char*)resCameraPath);
   }
   while ((pSceneFile = sceneList.GetFirst()) != NULL)
   {
      ScanScene(pSceneFile->m_fileName);
      LoadAllSamples();
      LoadSwitches(pSceneFile->m_fileName);
      SaveScene(pSceneFile->m_fileName);
      sceneList.Remove(pSceneFile);
      delete pSceneFile;
   }
   return S_OK;
}

void ShockCameraCreate()
{
   AutoAppIPtr(Unknown); 
   new cShockCamera(pUnknown); 
}
