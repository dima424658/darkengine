#include <lg.h>

#pragma pack(push, 8)
#include <vector>
#pragma pack(pop)

extern "C" int LGAPI _AppMain(int argc, const char *argv[]);

extern "C" {
int _g_referenceEntryPoint = 0;
}

int main(int argc, char *argv[])
{
    std::vector<const char *> arguments(argv, argv + argc);
    return _AppMain(argc, arguments.data());
}
