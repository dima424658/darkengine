#include <windows.h>
#include <initguid.h>
DEFINE_GUID(IID_CompatTest, 0x12345678, 0xabcd, 0x1234, 1,2,3,4,5,6,7,8);
extern "C" int CheckCompatC(const GUID *);
static_assert(sizeof(DWORD) == 4 && sizeof(LONG) == 4 && sizeof(ULONG) == 4);
static_assert(sizeof(HRESULT) == 4 && sizeof(GUID) == 16);
static_assert(sizeof(SIZE_T) == sizeof(void *));
class Counter : public IUnknown {
public:
 HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id, void **out) override {
  if (!out) return E_POINTER;
  *out = nullptr;
  if (!IsEqualGUID(id, IID_CompatTest)) return E_NOINTERFACE;
  *out = this;
  AddRef();
  return S_OK;
 }
 ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
 ULONG STDMETHODCALLTYPE Release() override { return --refs; }
 ULONG refs = 1;
};
int main() {
 Counter counter;
 void *out = nullptr;
 if (FAILED(counter.QueryInterface(IID_CompatTest, &out)) || out != &counter) return 1;
 if (counter.Release() != 1 || !FAILED(E_FAIL) || !SUCCEEDED(S_FALSE)) return 2;
 return CheckCompatC(&IID_CompatTest);
}
