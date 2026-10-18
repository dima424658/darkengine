#include <filepath.h>
#include <filespec.h>
#include <fnamutil.h>
#include <platform_services.h>
#include <str.h>

#pragma pack(push, 8)
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fnmatch.h>
#include <limits.h>
#include <stdlib.h>
#include <string>
#include <vector>
#pragma pack(pop)

namespace fs = std::filesystem;

namespace
{
struct FindState
{
    std::vector<fs::path> entries;
    size_t next = 0;
};

std::string FoldAscii(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character)
                   {
                       return static_cast<char>(character >= 'A' && character <= 'Z'
                           ? character + ('a' - 'A') : character);
                   });
    return text;
}

std::string NativePath(const char *path)
{
    char resolved[PATH_MAX];
    PlatformResolvePath(path, resolved, sizeof(resolved));
    return resolved;
}

std::string DirectoryText(const fs::path &path)
{
    std::string result = path.lexically_normal().generic_string();
    if (!result.empty() && result.back() != '/')
        result.push_back('/');
    return result;
}

void SetOwnedName(cFileSpec &spec, const std::string &name, eFileSpecKind kind = kSpecKindFile)
{
    spec.Empty();
    spec.pName = strdup(name.c_str());
    spec.Kind = spec.pName ? kind : kSpecKindUnknown;
}

BOOL CopyAllocated(const std::string &value, char **output)
{
    if (!output)
        return FALSE;
    *output = strdup(value.c_str());
    return *output != nullptr;
}
}

BOOL __stdcall GetFullPath(const char *path, char **output)
{
    std::error_code error;
    fs::path full = fs::absolute(NativePath(path), error).lexically_normal();
    return error ? FALSE : CopyAllocated(full.generic_string(), output);
}

BOOL __stdcall AddFileSpec(const char *path, const char *file, char **output)
{
    fs::path child(NativePath(file));
    fs::path result = child.is_absolute() ? child : fs::path(NativePath(path)) / child;
    return CopyAllocated(result.lexically_normal().generic_string(), output);
}

BOOL __stdcall ComputeAnchoredPath(const char *anchor, const char *target, char **output)
{
    std::error_code error;
    fs::path base = fs::absolute(NativePath(anchor), error).lexically_normal();
    if (error)
        return FALSE;
    fs::path goal = fs::absolute(NativePath(target), error).lexically_normal();
    if (error)
        return FALSE;
    fs::path relative = fs::relative(goal, base, error);
    return CopyAllocated((error ? goal : relative).generic_string(), output);
}

BOOL __stdcall IsValidFileSpec(const char *path)
{
    return path && *path;
}

BOOL __stdcall IsValidFilePath(const char *path)
{
    return path != nullptr;
}

BOOL __stdcall ReduceDots(char *path)
{
    if (!path)
        return FALSE;
    const std::string reduced = fs::path(NativePath(path)).lexically_normal().generic_string();
    memmove(path, reduced.c_str(), reduced.size() + 1);
    return TRUE;
}

cFilePath::cFilePath() = default;
cFilePath::cFilePath(const cFilePath &other) : m_path(other.m_path) {}
cFilePath::cFilePath(const cFileSpec &spec) { FromFileSpec(spec.GetName()); }
cFilePath::cFilePath(const char *path) { FromText(path); }

cFilePath &cFilePath::operator=(const cFilePath &other)
{
    m_path = other.m_path;
    return *this;
}

cFilePath &cFilePath::operator=(const cFileSpec &spec)
{
    FromFileSpec(spec.GetName());
    return *this;
}

BOOL cFilePath::operator==(const cFilePath &other) const
{
    return m_path.Compare(other.m_path) == 0;
}

void cFilePath::FromText(const char *path)
{
    m_path = DirectoryText(fs::path(NativePath(path))).c_str();
}

void cFilePath::FromFileSpec(const char *file)
{
    FromText(fs::path(NativePath(file)).parent_path().generic_string().c_str());
}

void cFilePath::AsText(cStr &text) const
{
    text = m_path;
}

BOOL cFilePath::PathExists() const
{
    std::error_code error;
    return fs::is_directory(fs::path(NativePath(m_path)), error) && !error;
}

BOOL cFilePath::CreatePath() const
{
    std::error_code error;
    return (fs::create_directory(fs::path(NativePath(m_path)), error) || PathExists()) && !error;
}

BOOL cFilePath::SetCurrentPath() const
{
    std::error_code error;
    fs::current_path(fs::path(NativePath(m_path)), error);
    return !error;
}

BOOL cFilePath::AddRelativePath(const cFilePath &relative)
{
    fs::path addition(NativePath(relative.GetPathName()));
    if (addition.is_absolute())
        return FALSE;
    m_path = DirectoryText(fs::path(NativePath(m_path)) / addition).c_str();
    return TRUE;
}

BOOL cFilePath::MakeFullPath()
{
    cStr full;
    if (!GetFullPath(full))
        return FALSE;
    FromText(full);
    return TRUE;
}

BOOL cFilePath::GetFullPath(cStr &result, const cFilePath &base) const
{
    fs::path path(NativePath(m_path));
    if (!path.is_absolute())
        path = fs::path(NativePath(base.GetPathName())) / path;
    std::error_code error;
    path = fs::absolute(path, error).lexically_normal();
    if (error)
        return FALSE;
    result = DirectoryText(path).c_str();
    return TRUE;
}

BOOL cFilePath::MakeFullPath(const cFilePath &base)
{
    cStr full;
    if (!GetFullPath(full, base))
        return FALSE;
    FromText(full);
    return TRUE;
}

BOOL sFindContext::DoFindFirst(tFindState what, const cStr &spec, cStr &result)
{
    delete static_cast<FindState *>(PlatformFindContext.hContext);
    PlatformFindContext.hContext = nullptr;
    Finding = kFindingNone;

    fs::path search(NativePath(spec));
    fs::path directory = search.parent_path();
    if (directory.empty())
        directory = ".";
    const std::string pattern = search.filename().string();
    auto *state = new FindState;
    std::error_code error;
    for (fs::directory_iterator it(directory, error), end; !error && it != end; it.increment(error))
    {
        const bool isDirectory = it->is_directory(error);
        if (error)
            break;
        if ((what == kFindingPaths) != isDirectory)
            continue;
        const std::string name = it->path().filename().string();
        const std::string foldedPattern = FoldAscii(pattern);
        const std::string foldedName = FoldAscii(name);
        if (fnmatch(foldedPattern.c_str(), foldedName.c_str(), 0) == 0)
            state->entries.push_back(it->path().filename());
    }
    if (error || state->entries.empty())
    {
        delete state;
        return FALSE;
    }
    std::sort(state->entries.begin(), state->entries.end());
    result = state->entries[0].generic_string().c_str();
    state->next = 1;
    PlatformFindContext.hContext = state;
    Finding = what;
    return TRUE;
}

BOOL sFindContext::DoFindNext(cStr &result)
{
    auto *state = static_cast<FindState *>(PlatformFindContext.hContext);
    if (!state || state->next >= state->entries.size())
        return FALSE;
    result = state->entries[state->next++].generic_string().c_str();
    return TRUE;
}

BOOL cFilePath::FindFirst(cFileSpec &spec, sFindContext &context) const
{
    return FindFirst(spec, context, "*");
}

BOOL cFilePath::FindFirst(cFileSpec &spec, sFindContext &context, const char *wildcard) const
{
    cStr found;
    cStr search(m_path);
    search += wildcard ? wildcard : "*";
    if (!context.DoFindFirst(sFindContext::kFindingFiles, search, found))
        return FALSE;
    spec.Set(*this, found);
    return TRUE;
}

BOOL cFilePath::FindNext(cFileSpec &spec, sFindContext &context) const
{
    cStr found;
    if (!context.DoFindNext(found))
        return FALSE;
    spec.Set(*this, found);
    return TRUE;
}

BOOL cFilePath::FindFirst(cFilePath &path, sFindContext &context) const
{
    cStr found;
    cStr search(m_path);
    search += "*";
    if (!context.DoFindFirst(sFindContext::kFindingPaths, search, found))
        return FALSE;
    path.FromText(DirectoryText(fs::path(NativePath(m_path)) / NativePath(found)).c_str());
    return TRUE;
}

BOOL cFilePath::FindNext(cFilePath &path, sFindContext &context) const
{
    cStr found;
    if (!context.DoFindNext(found))
        return FALSE;
    path.FromText(DirectoryText(fs::path(NativePath(m_path)) / NativePath(found)).c_str());
    return TRUE;
}

void cFilePath::FindDone(sFindContext &context) const
{
    delete static_cast<FindState *>(context.PlatformFindContext.hContext);
    context.PlatformFindContext.hContext = nullptr;
    context.Finding = sFindContext::kFindingNone;
}

BOOL cFilePath::GetRootDir(const char *, cStr &result) { result = "/"; return TRUE; }
BOOL cFilePath::GetVolumeName(const char *, cStr &result) { result = "/"; return TRUE; }
eFileSystemKind cFilePath::GetFileSystemKind() const { return kUnknownFileSystem; }
BOOL cFilePath::GetFileSystemName(const char *, cStr &result) { result = "POSIX"; return TRUE; }
ulong cFilePath::GetVolumeSerialNumber(const char *) { return 0; }
int cFilePath::GetCaseConventions(const char *) { return kNameCasePreserved | kNameCaseSensitive; }
ulong cFilePath::GetMaxLegalComponentLen(const char *) { return 255; }
BOOL cFilePath::IsValidChar(const char *, char character) { return character != '/' && character != '\0'; }
BOOL cFilePath::IsValidComponentName(const char *, const char *name) { return name && *name && !strchr(name, '/'); }

cFilePath cFileSpec::AnchorPath;

cFileSpec::cFileSpec(const cFileSpec &other) { pName = other.pName ? strdup(other.pName) : nullptr; Kind = other.Kind; }
cFileSpec::cFileSpec(const sFileDesc &other) { pName = other.pName ? strdup(other.pName) : nullptr; Kind = other.Kind; }
cFileSpec::cFileSpec(const char *name) { pName = nullptr; Kind = kSpecKindUnknown; SetOwnedName(*this, NativePath(name)); }
cFileSpec::cFileSpec(const cFilePath &path, const char *name) { pName = nullptr; Kind = kSpecKindUnknown; Set(path, name); }
cFileSpec::cFileSpec(eFileSpecKind kind, const char *name) { pName = name ? strdup(name) : nullptr; Kind = pName ? kind : kSpecKindUnknown; }
cFileSpec::~cFileSpec() { free(pName); }

cFileSpec &cFileSpec::operator=(const sFileDesc &other)
{
    if (this != &other)
    {
        char *copy = other.pName ? strdup(other.pName) : nullptr;
        free(pName);
        pName = copy;
        Kind = other.Kind;
    }
    return *this;
}

void cFileSpec::Empty() { free(pName); pName = nullptr; Kind = kSpecKindUnknown; }

void cFileSpec::Set(const cFilePath &path, const char *name)
{
    SetOwnedName(*this, (fs::path(NativePath(path.GetPathName())) / NativePath(name)).lexically_normal().generic_string());
}

void cFileSpec::SetRelativePath(const char *path) { SetOwnedName(*this, NativePath(path)); }
void cFileSpec::GetRelativePath(cStr &result) const { result = pName ? pName : ""; }
BOOL cFileSpec::IsRelativePath() const { return pName && fs::path(NativePath(pName)).is_relative(); }

const char *cFileSpec::GetFileName() const
{
    if (!pName)
        return nullptr;
    const char *slash = strrchr(pName, '/');
    const char *backslash = strrchr(pName, '\\');
    const char *last = slash > backslash ? slash : backslash;
    return last ? last + 1 : pName;
}

void cFileSpec::GetFileName(cStr &result) const { result = GetFileName(); }
void cFileSpec::GetFileRoot(cStr &result) const { result = fs::path(NativePath(GetFileName())).stem().string().c_str(); }
void cFileSpec::GetFileExtension(cStr &result) const { result = fs::path(NativePath(GetFileName())).extension().string().c_str(); }

const char *cFileSpec::GetFileExtension() const
{
    const char *name = GetFileName();
    const char *dot = name ? strrchr(name, '.') : nullptr;
    return dot ? dot : (name ? name + strlen(name) : nullptr);
}

void cFileSpec::SetFileExtension(const char *extension)
{
    fs::path path(NativePath(pName));
    path.replace_extension(extension ? extension : "");
    SetOwnedName(*this, path.generic_string());
}

void cFileSpec::SetFilePath(const cFilePath &path) { Set(path, GetFileName()); }

BOOL cFileSpec::MakeFullPath()
{
    cStr full;
    if (!GetFullPath(full))
        return FALSE;
    SetOwnedName(*this, full.BufIn());
    return TRUE;
}

BOOL cFileSpec::GetFullPath(cStr &result) const
{
    if (!pName)
        return FALSE;
    std::error_code error;
    fs::path full = fs::absolute(NativePath(pName), error).lexically_normal();
    if (error)
        return FALSE;
    result = full.generic_string().c_str();
    return TRUE;
}

BOOL cFileSpec::GetFullPath(cStr &result, const cFilePath &base) const
{
    if (!pName)
        return FALSE;
    fs::path path(NativePath(pName));
    if (path.is_relative())
        path = fs::path(NativePath(base.GetPathName())) / path;
    std::error_code error;
    path = fs::absolute(path, error).lexically_normal();
    if (error)
        return FALSE;
    result = path.generic_string().c_str();
    return TRUE;
}

BOOL cFileSpec::MakeFullPath(const cFilePath &base)
{
    cStr full;
    if (!GetFullPath(full, base))
        return FALSE;
    SetOwnedName(*this, full.BufIn());
    return TRUE;
}

BOOL cFileSpec::MakeAnchoredPath(const cFilePath &base)
{
    cStr relative;
    if (!base.ComputeAnchoredPath(cFilePath(pName), relative))
        return FALSE;
    SetOwnedName(*this, relative.BufIn());
    return TRUE;
}

BOOL cFileSpec::FileExists() const
{
    std::error_code error;
    return pName && fs::is_regular_file(NativePath(pName), error) && !error;
}

BOOL cFileSpec::UnlinkFile() const
{
    std::error_code error;
    return pName && fs::remove(NativePath(pName), error) && !error;
}

ulong cFileSpec::GetModificationTime() const
{
    std::error_code error;
    const auto time = fs::last_write_time(NativePath(pName), error);
    if (error)
        return 0;
    const auto systemTime = std::chrono::time_point_cast<std::chrono::seconds>(
        time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    return static_cast<ulong>(systemTime.time_since_epoch().count());
}

BOOL cFileSpec::operator==(const sFileDesc &other) const
{
    return Kind == other.Kind && ((!pName && !other.pName) || (pName && other.pName && strcmp(pName, other.pName) == 0));
}

int cFileSpec::Compare(const sFileDesc &other) const
{
    if (Kind != other.Kind)
        return static_cast<int>(Kind) - static_cast<int>(other.Kind);
    return strcmp(pName ? pName : "", other.pName ? other.pName : "");
}

void cFileSpec::SetAnchorPath(const cFilePath &path) { AnchorPath = path; }

void cFileSpec::GetNameString(cStr &result, eFileSpecNameStringStyle style) const
{
    if (style == kNameOnlyNameStyle)
        result = GetFileName();
    else if (style == kAnchorRelativeNameStyle)
        GetNameString(result, AnchorPath);
    else
        result = pName ? pName : "";
}

void cFileSpec::GetNameString(cStr &result, const cFilePath &relativeTo) const
{
    cStr full;
    if (GetFullPath(full) && relativeTo.ComputeAnchoredPath(cFilePath(full), result))
        return;
    result = pName ? pName : "";
}

void cFileSpec::GetNameString(cStr &result, int desiredLength) const
{
    result = pName ? pName : "";
    if (desiredLength > 3 && result.GetLength() > desiredLength)
    {
        const std::string value(result);
        result = (value.substr(0, desiredLength / 2 - 1) + "..." +
                  value.substr(value.size() - (desiredLength - desiredLength / 2 - 2))).c_str();
    }
}

BOOL cFileSpec::IsValidFileSpec() const { return Kind == kSpecKindFile && ::IsValidFileSpec(pName); }
BOOL cFileSpec::IsValidFileSpec(const char *path) { return ::IsValidFileSpec(path); }
BOOL cFileSpec::NameMatches(const char *pattern)
{
    if (!pName || !pattern)
        return FALSE;
    const std::string foldedPattern = FoldAscii(pattern);
    const std::string foldedName = FoldAscii(GetFileName());
    return fnmatch(foldedPattern.c_str(), foldedName.c_str(), 0) == 0;
}
