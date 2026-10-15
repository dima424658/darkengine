#include <osm_emu/osm32env.h>

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

int main(int argc, char **argv)
{
   if (argc < 2 || argc > 4)
   {
      fprintf(stderr,
              "usage: osm32_entry_test module.osm [class|list] [message]\n");
      return 2;
   }

   osm32::cEnvironment host;
   osm32::cModule module;
   std::string error;
   if (!module.Load(argv[1], "probe", &host, &error))
   {
      fprintf(stderr, "%s: portable module load FAIL: %s\n", argv[1],
              error.c_str());
      return 1;
   }

   const std::vector<osm32::sGuestClass> &classes = module.Classes();
   if (classes.empty())
   {
      fprintf(stderr, "%s: portable module returned no classes\n", argv[1]);
      return 1;
   }

   if (argc == 3 && !strcmp(argv[2], "list"))
   {
      for (size_t i = 0; i < classes.size(); ++i)
         printf("%s\n", classes[i].name.c_str());
      return 0;
   }

   uint32_t script = 0;
   size_t factoryIndex = 0;
   for (; factoryIndex < classes.size(); ++factoryIndex)
   {
      if (argc >= 3 && strcmp(classes[factoryIndex].name.c_str(), argv[2]))
         continue;
      if (module.CreateScript(classes[factoryIndex], 1, &script, &error))
         break;
      if (error != "guest script factory returned null")
         break;
   }
   if (!script)
   {
      fprintf(stderr, "%s: guest factory FAIL for '%s': %s\n", argv[1],
              argc >= 3 ? argv[2] : "first constructible class",
              error.empty() ? "class not found" : error.c_str());
      return 1;
   }

   uint32_t classNameAddress;
   std::string className;
   if (!module.CallCom(script, 3, 0, 0, &classNameAddress, &error) ||
       !module.ReadString(classNameAddress, &className, &error))
   {
      fprintf(stderr, "%s: guest IScript::GetClassName FAIL: %s\n", argv[1],
              error.c_str());
      return 1;
   }

   osm32::sMessage message;
   message.from = 0;
   message.to = 1;
   message.name = argc == 4 ? argv[3] : "BeginScript";
   message.time = 0;
   message.flags = 0;
   osm32::sValue reply;
   reply.type = osm32::kValueInteger;
   reply.bits = 1;
   uint32_t messageResult;
   if (!module.ReceiveMessage(script, message, &reply, 0, &messageResult,
                              &error))
   {
      fprintf(stderr, "%s: guest IScript::ReceiveMessage FAIL: %s\n",
              argv[1], error.c_str());
      return 1;
   }
   if (argc < 4 &&
       (reply.type != osm32::kValueInteger || reply.bits != 1))
   {
      fprintf(stderr,
              "%s: guest IScript::ReceiveMessage discarded initial reply "
              "type=%u bits=%08x\n",
              argv[1], static_cast<unsigned>(reply.type), reply.bits);
      return 1;
   }

   uint32_t ignored;
   if (!module.CallCom(script, 2, 0, 0, &ignored, &error))
   {
      fprintf(stderr, "%s: guest IScript::Release FAIL: %s\n", argv[1],
              error.c_str());
      return 1;
   }

   printf("%s: module='%s' classes=%u factory='%s' instance='%s' "
          "%s=0x%08x PASS\n",
          argv[1], module.Name().c_str(),
          static_cast<unsigned>(classes.size()),
          classes[factoryIndex].name.c_str(), className.c_str(),
          message.name.c_str(),
          messageResult);
   return 0;
}
