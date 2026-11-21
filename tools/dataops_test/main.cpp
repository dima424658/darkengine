#include <dataops_.h>
#include <keyprop.h>
#include <frobscrt.h>
#include <pickbase.h>
#include <lootprop.h>
#include <camera.h>
#include <linkbase.h>
#include <array>
#include <climits>
#include <cstdio>
#include <cstring>
#include <cstdlib>

// The isolated codec test supplies only the COM identity and fatal diagnostic.
const GUID IID_IUnknown = {};
extern "C" void _CriticalMsg(const char *message, const char *file, unsigned line)
{
   std::fprintf(stderr, "%s:%u: %s\n", file, line, message ? message : "critical error");
   std::abort();
}

static void Check(bool condition, const char *message)
{
   if (!condition)
   {
      std::fprintf(stderr, "%s\n", message);
      std::exit(1);
   }
}

// Layouts compared with the installed Thief II mission/save formats.
static_assert(sizeof(sFrobInfo) == 16);
static_assert(sizeof(sPickState) == 16);
static_assert(sizeof(sPickStage) == 12);
static_assert(sizeof(sPickCfg) == 36);
static_assert(sizeof(sAdvPickTransCfg) == 108);
static_assert(sizeof(sAdvPickStateCfg) == 52);
static_assert(sizeof(sAdvPickSoundCfg) == 648);
static_assert(sizeof(sLootProp) == 16);
static_assert(sizeof(Camera) == 36);
static_assert(sizeof(sLink) == 10);

static void TestMemoryFile()
{
   cDataOpsMemFile file;
   const std::array<unsigned char, 4> input = {1, 2, 3, 4};
   std::array<unsigned char, 4> output{};
   Check(file.Write(input.data(), input.size()) == input.size(), "initial write failed");
   Check(SUCCEEDED(file.Seek(kDataOpSeekFromStart, 0)), "rewind failed");
   Check(file.Read(output.data(), output.size()) == output.size() && output == input,
         "written bytes are not readable");
   Check(file.Read(output.data(), 1) == 0, "read past EOF succeeded");
   Check(file.Write(input.data(), -1) == 0, "negative write accepted");
   Check(file.Read(output.data(), -1) == 0, "negative read accepted");
   file.Seek(kDataOpSeekFromEnd, 100);
   Check(file.Tell() == 0, "seek before start wrapped");
   file.Seek(kDataOpSeekFromHere, UINT32_MAX);
   Check(file.Tell() == input.size(), "seek past EOF did not clamp");
   Check(FAILED(file.Seek(static_cast<eDataOpSeek>(99), 0)), "invalid seek accepted");

   std::array<unsigned char, 300> large;
   large.fill(77);
   file.Seek(kDataOpSeekFromStart, 0);
   Check(file.Write(large.data(), large.size()) == large.size(), "growth failed");
   file.Seek(kDataOpSeekFromStart, 1);
   Check(file.Write(input.data(), input.size()) == input.size(), "overwrite failed");
   file.Seek(kDataOpSeekFromEnd, 1);
   unsigned char last = 0;
   Check(file.Read(&last, 1) == 1 && last == 77, "overwrite lost trailing bytes");

   cDataOpsMemFile invalid(nullptr, -1);
   Check(invalid.Read(output.data(), 1) == 0, "invalid input length created readable data");
   cDataOpsMemFile empty(nullptr, 0);
   Check(empty.Write(input.data(), input.size()) == input.size(), "zero capacity did not grow");
}

static void TestFixedRecords()
{
   cFixedSizeDataOps ops(4);
   sDatum value = ops.New();
   const std::array<unsigned char, 6> bytes = {1, 2, 3, 4, 5, 6};
   std::memset(value.value, 99, 4);
   cDataOpsMemFile truncated(bytes.data(), 3);
   Check(FAILED(ops.Read(&value, &truncated, 4)), "truncated record accepted");
   Check(static_cast<unsigned char *>(value.value)[0] == 99, "failed read changed destination");
   cDataOpsMemFile input(bytes.data(), bytes.size());
   Check(FAILED(ops.Read(&value, &input, -1)), "negative size accepted");
   Check(FAILED(ops.Read(&value, &input, INT_MAX)), "unbounded size accepted");
   Check(input.Tell() == 0, "invalid size consumed input");
   Check(SUCCEEDED(ops.Read(&value, &input, 6)), "larger compatible record rejected");
   Check(input.Tell() == 6 && std::memcmp(value.value, bytes.data(), 4) == 0,
         "record prefix or consumption incorrect");
   cDataOpsMemFile output;
   Check(SUCCEEDED(ops.Write(value, &output)) && output.Tell() == 4, "fixed writer failed");
   Check(std::memcmp(output.GetBuffer(), bytes.data(), 4) == 0, "writer changed byte layout");
   ops.Delete(value);
}

static void TestPackedKey()
{
   static_assert(offsetof(sKeyInfo, region_mask) == 1);
   static_assert(offsetof(sKeyInfo, lock_id) == 5);
   struct HostLayout
   {
      unsigned char flag;
      unsigned int value;
   };
   static_assert(offsetof(HostLayout, value) == alignof(unsigned int));

   const unsigned char retail[] = {0, 1, 0, 0, 0, 1};
   cDataOpsMemFile input(retail, sizeof(retail));
   cClassDataOps<sKeyInfo> ops;
   sDatum value;
   Check(SUCCEEDED(ops.Read(&value, &input, sizeof(retail))), "retail key rejected");
   const sKeyInfo &key = *static_cast<sKeyInfo *>(value.value);
   Check(!key.master_bit && key.region_mask == 1 && key.lock_id == 1,
         "retail key fields shifted");
   cDataOpsMemFile output;
   Check(SUCCEEDED(ops.Write(value, &output)) && output.Tell() == sizeof(retail),
         "key writer used host padding");
   Check(std::memcmp(output.GetBuffer(), retail, sizeof(retail)) == 0,
         "key roundtrip changed retail bytes");
   ops.Delete(value);
}

int main()
{
   TestMemoryFile();
   TestFixedRecords();
   TestPackedKey();
   std::puts("PASS memory serialization and fixed-size records");
}
