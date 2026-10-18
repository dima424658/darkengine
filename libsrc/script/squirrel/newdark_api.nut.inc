static const char kNewDarkSquirrelApi[] = R"SQAPI(
// NewDark Squirrel API bootstrap.
//
// The native engine bridge provides:
//   __dark_call(serviceName, methodName, argsArray)
//   __dark_get_self()
//   __dark_get_message()
//   __dark_get_userparams()
//
// This file installs the API-reference service globals as Squirrel objects.

class vector {
  x = 0.0;
  y = 0.0;
  z = 0.0;

  constructor(a = 0.0, b = null, c = null) {
    if (typeof a == "table" || typeof a == "instance") {
      x = ("x" in a) ? a.x.tofloat() : 0.0;
      y = ("y" in a) ? a.y.tofloat() : 0.0;
      z = ("z" in a) ? a.z.tofloat() : 0.0;
    } else if (b == null && c == null) {
      x = a.tofloat();
      y = a.tofloat();
      z = a.tofloat();
    } else {
      x = a.tofloat();
      y = (b == null ? 0.0 : b.tofloat());
      z = (c == null ? 0.0 : c.tofloat());
    }
  }

  function _add(o) { o = vector(o); return vector(x + o.x, y + o.y, z + o.z); }
  function _sub(o) { o = vector(o); return vector(x - o.x, y - o.y, z - o.z); }
  function _unm() { return vector(-x, -y, -z); }
  function _mul(o) {
    if (typeof o == "integer" || typeof o == "float") return vector(x * o, y * o, z * o);
    o = vector(o);
    return vector(x * o.x, y * o.y, z * o.z);
  }
  function _div(o) {
    if (typeof o == "integer" || typeof o == "float") return vector(x / o, y / o, z / o);
    o = vector(o);
    return vector(x / o.x, y / o.y, z / o.z);
  }
  function _tostring() { return x + "," + y + "," + z; }
  function LengthSquared() { return x * x + y * y + z * z; }
  function Length() { return sqrt(LengthSquared()); }
  function Normalize() {
    local len = Length();
    if (len > 0) {
      x /= len;
      y /= len;
      z /= len;
    }
    return this;
  }
  function GetNormalized() { return vector(this).Normalize(); }
  function Dot(o) { o = vector(o); return x * o.x + y * o.y + z * o.z; }
  function Cross(o) { o = vector(o); return vector(y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x); }
}

class sLink {
  id = 0;
  source = 0;
  dest = 0;
  flavor = 0;

  constructor(linkId = 0) {
    if (typeof linkId == "table" || typeof linkId == "instance") {
      id = ("id" in linkId) ? linkId.id : 0;
      source = ("source" in linkId) ? linkId.source : (("from" in linkId) ? linkId.from : 0);
      dest = ("dest" in linkId) ? linkId.dest : (("to" in linkId) ? linkId.to : 0);
      flavor = ("flavor" in linkId) ? linkId.flavor : (("kind" in linkId) ? linkId.kind : 0);
    } else {
      id = linkId;
      local data = LinkTools.LinkGet(linkId);
      if (data != null) {
        source = ("source" in data) ? data.source : (("from" in data) ? data.from : 0);
        dest = ("dest" in data) ? data.dest : (("to" in data) ? data.to : 0);
        flavor = ("flavor" in data) ? data.flavor : (("kind" in data) ? data.kind : 0);
      }
    }
  }

  function From() { return source; }
  function To() { return dest; }
  function Kind() { return flavor; }
  function ID() { return id; }
}

class sLinkSet {
  ids = null;
  cursor = 0;

  constructor(values = null) {
    ids = [];
    if (values != null) {
      foreach (value in values) ids.append(value);
    }
  }

  function AnyLinksLeft() { return cursor < ids.len(); }
  function Link() { return AnyLinksLeft() ? ids[cursor] : 0; }
  function NextLink() { cursor++; }
  function _nexti(prev) {
    if (prev == null) return ids.len() > 0 ? 0 : null;
    local next = prev + 1;
    return next < ids.len() ? next : null;
  }
  function _get(index) { return ids[index]; }
  function len() { return ids.len(); }
}

class int_ref {
  value = 0;
  constructor(v = 0) { value = v; }
  function tointeger() { return value.tointeger(); }
  function tofloat() { return value.tofloat(); }
  function tostring() { return value.tostring(); }
  function _tostring() { return tostring(); }
}
class float_ref extends int_ref { constructor(v = 0.0) { value = v; } }
class string extends int_ref {
  constructor(v = "") { value = v; }
  function _add(other) { return value + other; }
}

// Instances cannot be enumerated by sq_next. Serialize value types explicitly;
// engine-owned callbacks use a separate retained-handle contract.
function __dark_to_host(value) {
  if (value instanceof vector) return { x = value.x, y = value.y, z = value.z, __sq_type = "vector" };
  if (value instanceof int_ref) return { value = value.value };
  if (value instanceof sLink) return { id = value.id, source = value.source, dest = value.dest, flavor = value.flavor };
  if (typeof value == "array") {
    local result = [];
    foreach (item in value) result.append(__dark_to_host(item));
    return result;
  }
  if (typeof value == "table") {
    local result = {};
    foreach (key, item in value) result[key] <- __dark_to_host(item);
    return result;
  }
  return value;
}

function __dark_invoke(serviceName, methodName, args) {
  local result = __dark_call(serviceName, methodName, __dark_to_host(args));
  if (typeof result == "table" && "__sq_outputs" in result) {
    foreach (output in result.__sq_outputs) {
      local target = args[output.index];
      if (target != null) foreach (key, value in output.value) {
        if (key != "__sq_type") target[key] = value;
      }
    }
    result = result.value;
  }
  return __dark_from_host(result);
}

function __dark_from_host(value) {
  if (typeof value == "table" && "__sq_type" in value) {
    if (value.__sq_type == "vector") return vector(value);
    if (value.__sq_type == "linkset") return sLinkSet(value.ids);
    if (value.__sq_type == "link") return sLink(value);
  }
  return value;
}

function object(value) {
  if (typeof value == "string") return Object.Named(value);
  if (typeof value == "integer" || typeof value == "float") return value.tointeger();
  if (typeof value == "instance" && "tointeger" in value) return value.tointeger();
  return 0;
}

function __dark_make_method(serviceName, methodName) {
  return function(...) {
    return __dark_invoke(serviceName, methodName, vargv);
  };
}

function __dark_install_service(serviceName, methods) {
  local service = {};
  foreach (methodName in methods) {
    service[methodName] <- __dark_make_method(serviceName, methodName);
  }
  getroottable()[serviceName] <- service;
}

class SqRootScript {
  self = 0;

  function GetClassName() { return typeof this; }
  function message() { return __dark_get_message(); }
  function MessageIs(name) { local m = message(); return m && m.message.tolower() == name.tolower(); }
  function BlockMessage() { return __dark_invoke("SqRootScript", "BlockMessage", []); }
  function Reply(value = null) { return __dark_invoke("SqRootScript", "Reply", [value]); }
  function ReplyWithObj(value = null) { return __dark_invoke("SqRootScript", "ReplyWithObj", [value]); }
  function SendMessage(to, msg, data = null, data2 = null, data3 = null) { return __dark_invoke("SqRootScript", "SendMessage", [to, msg, data, data2, data3]); }
  function PostMessage(to, msg, data = null, data2 = null, data3 = null) { return __dark_invoke("SqRootScript", "PostMessage", [to, msg, data, data2, data3]); }
  function SetOneShotTimer(a, b, c = null, d = null) { return __dark_invoke("SqRootScript", "SetOneShotTimer", [a, b, c, d]); }
  function KillTimer(handle) { return __dark_invoke("SqRootScript", "KillTimer", [handle]); }
  function GetTime() { return __dark_invoke("SqRootScript", "GetTime", []); }
  function HasProperty(prop) { return __dark_invoke("SqRootScript", "HasProperty", [prop]); }
  function GetProperty(prop, field = null) { return __dark_invoke("SqRootScript", "GetProperty", [prop, field]); }
  function SetProperty(...) { return __dark_invoke("SqRootScript", "SetProperty", vargv); }
  function IsDataSet(name) { return __dark_invoke("SqRootScript", "IsDataSet", [name]); }
  function GetData(name) { return __dark_invoke("SqRootScript", "GetData", [name]); }
  function SetData(name, value = null) { return __dark_invoke("SqRootScript", "SetData", [name, value]); }
  function ClearData(name) { return __dark_invoke("SqRootScript", "ClearData", [name]); }
  function ObjID(name) { return __dark_invoke("SqRootScript", "ObjID", [name]); }
  function linkkind(name) { return __dark_invoke("SqRootScript", "linkkind", [name]); }
  function LinkDest(id) { return __dark_invoke("SqRootScript", "LinkDest", [id]); }
  function userparams() { return __dark_get_userparams(); }
}

// Overlay base class. Methods are implemented by user script subclasses.
class IDarkOverlayHandler { self = 0; }

function __dark_overlay_callback(handler) {
  if (handler == null || (typeof handler != "instance" && typeof handler != "table")) return null;
  return { __sq_type = "callback", id = __dark_retain_callback(handler) };
}

__dark_install_service("Version", ["GetAppName", "GetVersion", "IsEditor", "GetGame", "GetGamsys", "GetMap", "GetCurrentFM", "GetCurrentFMPath", "FMizeRelativePath", "FMizePath", "GetModPath"]);
)SQAPI"
R"SQAPI(__dark_install_service("Engine", ["ConfigIsDefined", "ConfigGetInt", "ConfigGetFloat", "ConfigGetRaw", "BindingGetFloat", "FindFileInPath", "IsRunningDX6", "GetCanvasSize", "GetAspectRatio", "GetFog", "SetFog", "GetFogZone", "SetFogZone", "GetWeather", "SetWeather", "PortalRaycast", "ObjRaycast", "SetEnvMapZone"]);
__dark_install_service("Object", ["BeginCreate", "EndCreate", "Create", "Destroy", "Exists", "SetName", "GetName", "Named", "AddMetaProperty", "RemoveMetaProperty", "HasMetaProperty", "InheritsFrom", "IsTransient", "SetTransience", "Position", "Facing", "Teleport", "IsPositionValid", "FindClosestObjectNamed", "AddMetaPropertyToMany", "RemoveMetaPropertyFromMany", "RenderedThisFrame", "ObjectToWorld", "WorldToObject", "CalcRelTransform", "Archetype"]);
__dark_install_service("Property", ["Get", "Set", "SetSimple", "SetLocal", "Add", "Remove", "CopyFrom", "Possessed"]);
__dark_install_service("Physics", ["SubscribeMsg", "UnsubscribeMsg", "LaunchProjectile", "SetVelocity", "GetVelocity", "ControlVelocity", "StopControlVelocity", "SetGravity", "GetGravity", "HasPhysics", "IsSphere", "IsOBB", "ControlCurrentLocation", "ControlCurrentRotation", "ControlCurrentPosition", "DeregisterModel", "PlayerMotionSetOffset", "Activate", "ValidPos", "IsRope", "GetClimbingObject"]);
__dark_install_service("Link", ["Create", "Destroy", "AnyExist", "GetAll", "GetOne", "BroadcastOnAllLinks", "BroadcastOnAllLinksData", "CreateMany", "DestroyMany", "GetAllInherited", "GetAllInheritedSingle"]);
__dark_install_service("LinkTools", ["LinkKindNamed", "LinkKindName", "LinkGet", "LinkGetData", "LinkSetData"]);
__dark_install_service("Listener", ["SubscribeLink", "UnsubscribeLink"]);
__dark_install_service("ActReact", ["React", "Stimulate", "GetReactionNamed", "GetReactionName", "SubscribeToStimulus", "UnsubscribeToStimulus", "BeginContact", "EndContact", "SetSingleSensorContact"]);
__dark_install_service("Data", ["GetString", "GetObjString", "DirectRand", "RandInt", "RandFlt0to1", "RandFltNeg1to1"]);
__dark_install_service("AI", ["MakeGotoObj", "MakeGotoObjLoc", "MakeFrobObjWith", "MakeFrobObj", "GetAlertLevel", "SetMinimumAlert", "ClearGoals", "SetScriptFlags", "ClearAlertness", "Signal", "StartConversation"]);
__dark_install_service("Sound", ["PlayAtLocation", "PlayAtObject", "Play", "PlayAmbient", "PlaySchemaAtLocation", "PlaySchemaAtObject", "PlaySchema", "PlaySchemaAmbient", "PlayEnvSchema", "PlayVoiceOver", "Halt", "HaltSchema", "HaltSpeech", "PreLoad"]);
__dark_install_service("AnimTexture", ["ChangeTexture"]);
__dark_install_service("PGroup", ["SetActive"]);
__dark_install_service("Camera", ["StaticAttach", "DynamicAttach", "CameraReturn", "ForceCameraReturn", "GetCameraParent", "IsRemote", "GetPosition", "GetFacing", "CameraToWorld", "WorldToCamera", "LockMovement"]);
__dark_install_service("Light", ["Set", "SetMode", "Activate", "Deactivate", "Subscribe", "Unsubscribe", "GetMode"]);
__dark_install_service("Door", ["CloseDoor", "OpenDoor", "GetDoorState", "ToggleDoor", "SetBlocking", "GetSoundBlocking"]);
__dark_install_service("Damage", ["Damage", "Slay", "Resurrect"]);
__dark_install_service("Container", ["Add", "Remove", "MoveAllContents", "StackAdd", "IsHeld"]);
__dark_install_service("Quest", ["SubscribeMsg", "UnsubscribeMsg", "Set", "Get", "Exists", "Delete", "GetAllVars", "BinSet", "BinGet", "BinSetTable", "BinGetTable", "BinExists", "BinDelete"]);
__dark_install_service("Puppet", ["PlayMotion"]);
__dark_install_service("Locked", ["IsLocked"]);
__dark_install_service("Key", ["TryToUseKey"]);
__dark_install_service("Networking", ["Broadcast", "SendToProxy", "TakeOver", "GiveTo", "IsPlayer", "IsMultiplayer", "SetProxyOneShotTimer", "FirstPlayer", "NextPlayer", "Suspend", "Resume", "HostedHere", "IsProxy", "LocalOnly", "IsNetworking", "Owner"]);
__dark_install_service("CD", ["SetBGM", "SetTrack"]);
__dark_install_service("Debug", ["MPrint", "Command", "Break", "Log"]);
__dark_install_service("DarkGame", ["KillPlayer", "EndMission", "FadeToBlack", "FoundObject", "ConfigIsDefined", "ConfigGetInt", "ConfigGetFloat", "BindingGetFloat", "GetAutomapLocationVisited", "SetAutomapLocationVisited", "SetNextMission", "GetCurrentMission", "RespawnPlayer", "FadeIn"]);
__dark_install_service("DarkUI", ["TextMessage", "ReadBook", "InvItem", "InvWeapon", "InvSelect", "IsCommandBound", "DescribeKeyBinding"]);
__dark_install_service("PickLock", ["Ready", "UnReady", "StartPicking", "FinishPicking", "CheckPick", "DirectMotion"]);
__dark_install_service("DrkInv", ["CapabilityControl", "AddSpeedControl", "RemoveSpeedControl"]);
__dark_install_service("DrkPowerups", ["TriggerWorldFlash", "ObjTryDeploy", "CleanseBlood"]);
__dark_install_service("PlayerLimbs", ["Equip", "UnEquip", "StartUse", "FinishUse"]);
__dark_install_service("Weapon", ["Equip", "UnEquip", "IsEquipped", "StartAttack", "FinishAttack"]);
__dark_install_service("Bow", ["Equip", "UnEquip", "IsEquipped", "StartAttack", "FinishAttack", "AbortAttack", "SetArrow"]);
__dark_install_service("DarkOverlay", ["GetBitmap", "FlushBitmap", "GetBitmapSize", "WorldToScreen", "GetObjectScreenBounds", "CreateTOverlayItem", "CreateTOverlayItemFromBitmap", "DestroyTOverlayItem", "UpdateTOverlayAlpha", "UpdateTOverlayPosition", "UpdateTOverlaySize", "DrawBitmap", "DrawSubBitmap", "SetTextColorFromStyle", "SetTextColor", "GetStringSize", "DrawString", "DrawLine", "FillTOverlay", "BeginTOverlayUpdate", "EndTOverlayUpdate", "DrawTOverlayItem"]);
DarkOverlay.SetHandler <- function(handler) { return __dark_invoke("DarkOverlay", "SetHandler", [handler == null ? null : __dark_overlay_callback(handler)]); };
DarkOverlay.AddHandler <- function(handler) { return __dark_invoke("DarkOverlay", "AddHandler", [__dark_overlay_callback(handler)]); };
DarkOverlay.RemoveHandler <- function(handler) { return __dark_invoke("DarkOverlay", "RemoveHandler", [__dark_overlay_callback(handler)]); };

function GetAPIVersion() { return 10; }
function GetDarkGame() { return 0; }
function IsEditor() { return 0; }

TRUE <- true;
FALSE <- false;
S_OK <- 0;

eDarkGame <- { kDarkGameThief = 0, kDarkGameShock = 1, kDarkGameThief2 = 2 };
eDoorStatus <- { kDoorClosed = 0, kDoorOpen = 1, kDoorClosing = 2, kDoorOpening = 3, kDoorHalt = 4, kDoorHalted = 4, kDoorNoDoor = 5 };
eGoalState <- { kGoalIncomplete = 0, kGoalComplete = 1, kGoalInactive = 2, kGoalFailed = 3 };
eTweqType <- { kTweqTypeScale = 0, kTweqTypeRotate = 1, kTweqTypeJoints = 2, kTweqTypeModels = 3, kTweqTypeDelete = 4, kTweqTypeEmitter = 5, kTweqTypeFlicker = 6, kTweqTypeLock = 7, kTweqTypeAll = 8, kTweqTypeNull = 9 };
eTweqDirection <- { kTweqDirForward = 0, kTweqDirReverse = 1 };
eTweqDo <- { kTweqDoDefault = 0, kTweqDoActivate = 1, kTweqDoHalt = 2, kTweqDoReset = 3, kTweqDoContinue = 4, kTweqDoForward = 5, kTweqDoReverse = 6 };
eSlayResult <- { kSlayNormal = 0, kSlayNoEffect = 1, kSlayTerminate = 2, kSlayDestroy = 3 };
eDamageResult <- { kDamageTerminate = 0, kDamageNoWound = 1, kDamageWound = 2, kDamageSlay = 3 };
eContainsEvent <- { kContainQueryAdd = 0, kContainQueryCombine = 1, kContainAdd = 2, kContainRemove = 3, kContainCombine = 4 };
eAIActionPriority <- { kLowPriorityAction = 0, kNormalPriorityAction = 1, kHighPriorityAction = 2 };
eAIMode <- { kAIM_Asleep = 0, kAIM_SuperEfficient = 1, kAIM_Efficient = 2, kAIM_Normal = 3, kAIM_Combat = 4, kAIM_Dead = 5, kAIM_Num = 6 };
eAIScriptSpeed <- { kSlow = 0, kNormalSpeed = 1, kFast = 2 };
eEnvSoundLoc <- { kEnvSoundOnObj = 0, kEnvSoundAtObjLoc = 1, kEnvSoundAmbient = 2 };
eQuestDataType <- { kQuestDataMission = 0, kQuestDataCampaign = 1, kQuestDataUnknown = 2 };
ePhysScriptMsgType <- { kNoMsg = 0, kCollisionMsg = 1, kContactMsg = 2, kEnterExitMsg = 4, kFellAsleepMsg = 8, kWokeUpMsg = 16, kMadePhysMsg = 256, kMadeNonPhysMsg = 512, kAllMsgs = 7 };
ePhysCollisionType <- { kCollNone = 0, kCollTerrain = 1, kCollObject = 2 };
ePhysContactType <- { kContactNone = 0, kContactFace = 1, kContactEdge = 2, kContactVertex = 4, kContactSphere = 8, kContactSphereHat = 16, kContactOBB = 32, kContactTerrain = 7, kContactObject = 56 };
ePhysContact <- { kContactCreate = 0, kContactDestroy = 1 };
ePhysEnterExit <- { kEnter = 0, kExit = 1 };
ePhysMessageResult <- { kPM_StatusQuo = 0, kPM_Nothing = 1, kPM_Bounce = 2, kPM_Slay = 3, kPM_NonPhys = 4 };
eRelationListenMsg <- { kListenLinkModify = 1, kListenLinkBirth = 2, kListenLinkDeath = 4, kListenLinkPostMortem = 8 };

)SQAPI";
