// Portable execution and ABI wrapper for legacy 32-bit OSM modules.

#include <osm_emu/osm32rt.h>

#include <string.h>

namespace
{

const uint32_t kStackBase = 0x70000000;
const uint32_t kStackSize = 0x00100000;
const uint32_t kStop = 0xffff0000;

bool Fail(std::string *error, const char *message)
{
   if (error)
      *error = message;
   return false;
}

} // namespace

namespace osm32
{

sValue::sValue()
 : type(kValueUndefined),
   bits(0)
{
   vector[0] = vector[1] = vector[2] = 0.0f;
}

cModule::cModule()
 : m_environment(0),
   m_module(0),
   m_callDepth(0)
{
}

cModule::~cModule()
{
   Unload();
}

bool cModule::Call(uint32_t function, const uint32_t *arguments,
                   size_t argumentCount, uint32_t *result,
                   std::string *error)
{
   if (!function)
      return Fail(error, "cannot call a null guest function");
   const unsigned kStackSlice = 0x00010000;
   const unsigned kMaximumCallDepth = kStackSize / kStackSlice;
   if (m_callDepth >= kMaximumCallDepth)
      return Fail(error, "guest call nesting exhausted the interpreter stack");

   const bool nested = m_callDepth != 0;
   const sCpuState savedState = m_cpu.State();
   ++m_callDepth;
   m_cpu.State() = sCpuState();
   m_cpu.State().eip = function;
   m_cpu.State().reg[kEsp] = kStackBase + kStackSize -
                             (m_callDepth - 1) * kStackSlice;
   bool succeeded = true;
   for (size_t i = argumentCount; i != 0; --i)
      if (!m_cpu.Push32(arguments[i - 1], error))
      {
         succeeded = false;
         break;
      }
   if (succeeded && !m_cpu.Push32(kStop, error))
      succeeded = false;
   if (succeeded && m_cpu.Run(kStop, 10000000, error) != cCpu::kRunStopped)
      succeeded = false;
   const uint32_t returnValue = m_cpu.State().reg[kEax];
   --m_callDepth;
   if (nested)
      m_cpu.State() = savedState;
   if (succeeded && result)
      *result = returnValue;
   return succeeded;
}

bool cModule::CopyString(const char *value, uint32_t *address,
                         std::string *error)
{
   if (!value)
      value = "";
   const size_t length = strlen(value) + 1;
   if (length > 0xffffffffu)
      return Fail(error, "guest string is too large");
   *address = m_environment->AllocGuest(static_cast<uint32_t>(length), error);
   if (!*address)
      return false;
   if (!m_cpu.Memory().Load(*address, value, length, error))
   {
      m_environment->FreeGuest(*address, 0);
      *address = 0;
      return false;
   }
   return true;
}

bool cModule::ReadString(uint32_t address, std::string *value,
                         std::string *error) const
{
   value->clear();
   for (unsigned i = 0; i < 0x00100000; ++i)
   {
      uint8_t character;
      if (!m_cpu.Memory().Read8(address + i, &character, error))
         return false;
      if (!character)
         return true;
      value->push_back(static_cast<char>(character));
   }
   return Fail(error, "unterminated or excessively long guest string");
}

bool cModule::CallCom(uint32_t object, unsigned slot,
                      const uint32_t *arguments, size_t argumentCount,
                      uint32_t *result, std::string *error)
{
   uint32_t vtable, function;
   if (!object ||
       !m_cpu.Memory().Read32(object, &vtable, error) ||
       !m_cpu.Memory().Read32(vtable + slot * 4, &function, error))
      return false;

   std::vector<uint32_t> callArguments(argumentCount + 1);
   callArguments[0] = object;
   for (size_t i = 0; i < argumentCount; ++i)
      callArguments[i + 1] = arguments[i];
   return Call(function, &callArguments[0], callArguments.size(), result,
               error);
}

bool cModule::WriteValue(uint32_t address, const sValue &value,
                         std::vector<uint32_t> *allocations,
                         std::string *error)
{
   uint32_t data = value.bits;
   if (value.type == kValueString)
   {
      if (!CopyString(value.text.c_str(), &data, error))
         return false;
      allocations->push_back(data);
   }
   else if (value.type == kValueVector)
   {
      data = m_environment->AllocGuest(12, error);
      if (!data)
         return false;
      allocations->push_back(data);
      if (!m_cpu.Memory().Load(data, value.vector, 12, error))
         return false;
   }
   return m_cpu.Memory().Write32(address, data, error) &&
          m_cpu.Memory().Write32(address + 4,
                                 static_cast<uint32_t>(value.type), error);
}

bool cModule::ReadValue(uint32_t address, sValue *value,
                        std::string *error) const
{
   uint32_t type;
   if (!m_cpu.Memory().Read32(address, &value->bits, error) ||
       !m_cpu.Memory().Read32(address + 4, &type, error))
      return false;
   if (type > kValueObject)
      return Fail(error, "guest returned an invalid multiparm type");
   value->type = static_cast<eValueType>(type);
   value->text.clear();
   if (value->type == kValueString)
      return ReadString(value->bits, &value->text, error);
   if (value->type == kValueVector)
   {
      uint32_t bits[3];
      if (!m_cpu.Memory().Read32(value->bits, &bits[0], error) ||
          !m_cpu.Memory().Read32(value->bits + 4, &bits[1], error) ||
          !m_cpu.Memory().Read32(value->bits + 8, &bits[2], error))
         return false;
      memcpy(value->vector, bits, sizeof(bits));
   }
   return true;
}

void cModule::FreeAllocations(const std::vector<uint32_t> &allocations)
{
   for (size_t i = allocations.size(); i != 0; --i)
      m_environment->FreeGuest(allocations[i - 1], 0);
}

bool cModule::ReceiveMessage(uint32_t script, const sMessage &message,
                             sValue *reply, uint32_t debugAction,
                             uint32_t *result, std::string *error)
{
   // VC6 x86 layout: cCTUnaggregated base (8 bytes), sPersistent vptr
   // (4 bytes), followed by the public sScrMsg fields.
   const uint32_t kBaseMessageSize = 56;
   if (message.extension.size() > 0xffffffffu - kBaseMessageSize)
      return Fail(error, "guest message extension is too large");
   const uint32_t messageSize = kBaseMessageSize +
      static_cast<uint32_t>(message.extension.size());
   const uint32_t messageAddress = m_environment->AllocGuest(messageSize,
                                                              error);
   const uint32_t replyAddress = m_environment->AllocGuest(8, error);
   if (!messageAddress || !replyAddress)
   {
      if (messageAddress) m_environment->FreeGuest(messageAddress, 0);
      if (replyAddress) m_environment->FreeGuest(replyAddress, 0);
      return false;
   }

   std::vector<uint32_t> allocations;
   allocations.push_back(messageAddress);
   allocations.push_back(replyAddress);
   std::vector<uint8_t> empty(messageSize, 0);
   uint32_t nameAddress;
   if (!m_cpu.Memory().Load(messageAddress, &empty[0], empty.size(), error) ||
       !CopyString(message.name.c_str(), &nameAddress, error))
   {
      FreeAllocations(allocations);
      return false;
   }
   allocations.push_back(nameAddress);
   if (!message.extension.empty() &&
       !m_cpu.Memory().Load(messageAddress + kBaseMessageSize,
                            &message.extension[0], message.extension.size(),
                            error))
   {
      FreeAllocations(allocations);
      return false;
   }
   for (size_t i = 0; i < message.extensionStrings.size(); ++i)
   {
      const sMessage::sStringPatch &patch = message.extensionStrings[i];
      if (patch.offset > message.extension.size() ||
          message.extension.size() - patch.offset < 4)
      {
         FreeAllocations(allocations);
         return Fail(error, "guest message string patch is out of range");
      }
      uint32_t textAddress;
      if (!CopyString(patch.value.c_str(), &textAddress, error))
      {
         FreeAllocations(allocations);
         return false;
      }
      allocations.push_back(textAddress);
      if (!m_cpu.Memory().Write32(messageAddress + kBaseMessageSize +
                                  patch.offset, textAddress, error))
      {
         FreeAllocations(allocations);
         return false;
      }
   }
   for (size_t i = 0; i < message.extensionValues.size(); ++i)
   {
      const sMessage::sValuePatch &patch = message.extensionValues[i];
      if (patch.offset > message.extension.size() ||
          message.extension.size() - patch.offset < 8)
      {
         FreeAllocations(allocations);
         return Fail(error, "guest message value patch is out of range");
      }
      if (!WriteValue(messageAddress + kBaseMessageSize + patch.offset,
                      patch.value, &allocations, error))
      {
         FreeAllocations(allocations);
         return false;
      }
   }
   std::vector<uint32_t> bufferAddresses(message.extensionBuffers.size(), 0);
   for (size_t i = 0; i < message.extensionBuffers.size(); ++i)
   {
      const sMessage::sBufferPatch &patch = message.extensionBuffers[i];
      if (patch.offset > message.extension.size() ||
          message.extension.size() - patch.offset < 4 ||
          !patch.destination || !patch.capacity ||
          patch.capacity > 0x00100000u)
      {
         FreeAllocations(allocations);
         return Fail(error, "guest message buffer patch is invalid");
      }
      uint32_t length = 0;
      while (length < patch.capacity && patch.destination[length]) ++length;
      if (length == patch.capacity)
      {
         FreeAllocations(allocations);
         return Fail(error, "native message buffer is unterminated");
      }
      const uint32_t bufferAddress =
         m_environment->AllocGuest(patch.capacity, error);
      if (!bufferAddress)
      {
         FreeAllocations(allocations);
         return false;
      }
      allocations.push_back(bufferAddress);
      std::vector<uint8_t> buffer(patch.capacity, 0);
      if (length)
         memcpy(&buffer[0], patch.destination, length);
      if (!m_cpu.Memory().Load(bufferAddress, &buffer[0], buffer.size(),
                               error) ||
          !m_cpu.Memory().Write32(messageAddress + kBaseMessageSize +
                                  patch.offset, bufferAddress, error))
      {
         FreeAllocations(allocations);
         return false;
      }
      bufferAddresses[i] = bufferAddress;
   }

   bool written = m_cpu.Memory().Write32(messageAddress + 12,
                                          static_cast<uint32_t>(message.from),
                                          error) &&
                  m_cpu.Memory().Write32(messageAddress + 16,
                                          static_cast<uint32_t>(message.to),
                                          error) &&
                  m_cpu.Memory().Write32(messageAddress + 20, nameAddress,
                                          error) &&
                  m_cpu.Memory().Write32(messageAddress + 24, message.time,
                                          error) &&
                  m_cpu.Memory().Write32(messageAddress + 28,
                                          static_cast<uint32_t>(message.flags),
                                          error);
   for (unsigned i = 0; written && i < 3; ++i)
      written = WriteValue(messageAddress + 32 + i * 8, message.data[i],
                           &allocations, error);
   // The script manager supplies an initial reply value.  In particular,
   // frob dispatch starts with integer true so scripts can veto an action by
   // explicitly changing it.  Preserve that in/out value across the guest
   // boundary instead of replacing it with undefined before every call.
   sValue emptyReply;
   const sValue &initialReply = reply ? *reply : emptyReply;
   if (written)
      written = WriteValue(replyAddress, initialReply, &allocations, error);

   const uint32_t arguments[] = { messageAddress, replyAddress, debugAction };
   const bool called = written &&
      CallCom(script, 4, arguments, 3, result, error);
   bool read = called && (!reply || ReadValue(replyAddress, reply, error));
   for (size_t i = 0; read && i < message.extensionBuffers.size(); ++i)
   {
      const sMessage::sBufferPatch &patch = message.extensionBuffers[i];
      bool terminated = false;
      for (uint32_t j = 0; j < patch.capacity; ++j)
      {
         uint8_t value;
         if (!m_cpu.Memory().Read8(bufferAddresses[i] + j, &value, error))
         {
            read = false;
            break;
         }
         patch.destination[j] = static_cast<char>(value);
         if (!value)
         {
            terminated = true;
            break;
         }
      }
      if (read && !terminated)
      {
         patch.destination[patch.capacity - 1] = '\0';
         read = Fail(error, "guest message buffer is unterminated");
      }
   }
   FreeAllocations(allocations);
   return called && read;
}

bool cModule::ReadClasses(std::string *error)
{
   uint32_t nameAddress;
   if (!CallCom(m_module, 3, 0, 0, &nameAddress, error) ||
       !ReadString(nameAddress, &m_name, error))
      return false;

   const uint32_t iterator = m_environment->AllocGuest(4, error);
   if (!iterator)
      return false;
   if (!m_cpu.Memory().Write32(iterator, 0, error))
   {
      m_environment->FreeGuest(iterator, 0);
      return false;
   }

   uint32_t descriptor;
   const uint32_t argument[] = { iterator };
   if (!CallCom(m_module, 4, argument, 1, &descriptor, error))
   {
      m_environment->FreeGuest(iterator, 0);
      return false;
   }

   m_classes.clear();
   while (descriptor)
   {
      if (m_classes.size() == 65536)
      {
         m_environment->FreeGuest(iterator, 0);
         return Fail(error, "guest class iteration did not terminate");
      }
      uint32_t moduleName, className, baseName, factory;
      if (!m_cpu.Memory().Read32(descriptor, &moduleName, error) ||
          !m_cpu.Memory().Read32(descriptor + 4, &className, error) ||
          !m_cpu.Memory().Read32(descriptor + 8, &baseName, error) ||
          !m_cpu.Memory().Read32(descriptor + 12, &factory, error))
      {
         m_environment->FreeGuest(iterator, 0);
         return false;
      }
      sGuestClass item;
      item.factory = factory;
      if (!ReadString(moduleName, &item.module, error) ||
          !ReadString(className, &item.name, error) ||
          !ReadString(baseName, &item.base, error))
      {
         m_environment->FreeGuest(iterator, 0);
         return false;
      }
      m_classes.push_back(item);
      if (!CallCom(m_module, 5, argument, 1, &descriptor, error))
      {
         m_environment->FreeGuest(iterator, 0);
         return false;
      }
   }

   uint32_t ignored;
   const bool ended = CallCom(m_module, 6, argument, 1, &ignored, error);
   const bool freed = m_environment->FreeGuest(iterator, error);
   return ended && freed;
}

bool cModule::Load(const char *path, const char *moduleName,
                   iEnvironment *environment, std::string *error)
{
   Unload();
   if (!environment)
      return Fail(error, "OSM runtime requires a host environment");
   m_environment = environment;
   m_cpu.SetHostCalls(environment);

   if (!m_image.Load(path, error) ||
       !environment->Prepare(&m_image, &m_cpu.Memory(), error) ||
       !m_cpu.Memory().Map(m_image.LoadedBase(), m_image.ImageSize(),
                           kMemRead | kMemWrite | kMemExecute, error) ||
       !m_cpu.Memory().Load(m_image.LoadedBase(), m_image.Data(),
                            m_image.ImageSize(), error) ||
       !m_cpu.Memory().Map(kStackBase, kStackSize,
                           kMemRead | kMemWrite, error))
   {
      Unload();
      return false;
   }

   const uint32_t entryArguments[] = { m_image.LoadedBase(), 1, 0 };
   uint32_t result;
   if (!Call(m_image.EntryPoint(), entryArguments, 3, &result, error))
   {
      Unload();
      return false;
   }

   const uint32_t init = m_image.FindExport("_ScriptModuleInit@20");
   uint32_t guestName = 0;
   const uint32_t moduleOut = environment->AllocGuest(4, error);
   if (!init || !moduleOut || !CopyString(moduleName, &guestName, error) ||
       !m_cpu.Memory().Write32(moduleOut, 0, error))
   {
      if (guestName) environment->FreeGuest(guestName, 0);
      if (moduleOut) environment->FreeGuest(moduleOut, 0);
      Unload();
      return init ? false : Fail(error, "OSM has no ScriptModuleInit export");
   }

   const uint32_t initArguments[] = {
      guestName,
      environment->ScriptManagerAddress(),
      environment->PrintAddress(),
      environment->AllocatorAddress(),
      moduleOut
   };
   const bool initialized = Call(init, initArguments, 5, &result, error) &&
                            result &&
                            m_cpu.Memory().Read32(moduleOut, &m_module, error) &&
                            m_module;
   environment->FreeGuest(guestName, 0);
   environment->FreeGuest(moduleOut, 0);
   if (!initialized)
   {
      if (error && error->empty())
         *error = "OSM ScriptModuleInit failed";
      Unload();
      return false;
   }

   if (!ReadClasses(error))
   {
      Unload();
      return false;
   }
   if (error) error->clear();
   return true;
}

bool cModule::CreateScript(const sGuestClass &scriptClass, int32_t objectId,
                           uint32_t *script, std::string *error)
{
   if (!m_environment || !scriptClass.factory)
      return Fail(error, "invalid guest script factory");
   uint32_t className;
   if (!CopyString(scriptClass.name.c_str(), &className, error))
      return false;
   const uint32_t arguments[] = {
      className, static_cast<uint32_t>(objectId)
   };
   std::string callError;
   const bool called = Call(scriptClass.factory, arguments, 2, script,
                            &callError);
   std::string freeError;
   const bool freed = m_environment->FreeGuest(className, &freeError);
   if (!called || !freed)
   {
      if (error)
         *error = called ? freeError : callError;
      return false;
   }
   if (!*script)
      return Fail(error, "guest script factory returned null");
   return true;
}

void cModule::Unload()
{
   if (m_module && m_environment)
   {
      uint32_t ignored;
      std::string ignoredError;
      CallCom(m_module, 2, 0, 0, &ignored, &ignoredError);
   }
   m_module = 0;
   m_callDepth = 0;
   m_classes.clear();
   m_name.clear();
   m_environment = 0;
   m_cpu.SetHostCalls(0);
   m_cpu = cCpu();
   m_image = cImage();
}

} // namespace osm32
