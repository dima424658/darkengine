#include <splitpat.h>

#include <cstring>

void SplitPath(const char *path, char *drive, char *directory,
               char *name, char *extension)
{
    cPathSplitter splitter(path);
    const auto copy = [](char *destination, const char *source, int length)
    {
        if (destination)
        {
            if (source && length > 0)
                std::memcpy(destination, source, static_cast<size_t>(length));
            destination[source && length > 0 ? length : 0] = '\0';
        }
    };
    copy(drive, splitter.GetDrive(), splitter.GetDriveLen());
    copy(directory, splitter.GetDirectory(), splitter.GetDirectoryLen());
    copy(name, splitter.GetName(), splitter.GetNameLen());
    copy(extension, splitter.GetExtension(), splitter.GetExtensionLen());
}

BOOL cPathSplitter::DoSplitTo(eComponent)
{
    if (!pcszTarget)
        return FALSE;

    const size_t length = std::strlen(pcszTarget);
    const char *const begin = pcszTarget;
    const char *const end = begin + length;
    size_t driveEnd = 0;
    if (length >= 2 && begin[1] == ':')
        driveEnd = 2;

    size_t directoryEnd = driveEnd;
    for (size_t i = driveEnd; i < length; ++i)
        if (begin[i] == '/' || begin[i] == '\\')
            directoryEnd = i + 1;

    size_t extensionStart = length;
    for (size_t i = directoryEnd; i < length; ++i)
        if (begin[i] == '.' && i > directoryEnd)
            extensionStart = i;

    Drive.Found(begin, begin + driveEnd, static_cast<int>(length));
    Directory.Found(begin + driveEnd, begin + directoryEnd,
                    static_cast<int>(length));
    Name.Found(begin + directoryEnd, begin + extensionStart,
               static_cast<int>(length));
    Extension.Found(begin + extensionStart, end, static_cast<int>(length));
    nSignificantTargetLen = static_cast<unsigned>(length);
    LastSplitTo = kExtension;
    return TRUE;
}

BOOL cPathSplitter::GetSplit(cStr *drive, cStr *directory, cStr *name,
                             cStr *extension)
{
    if (!pcszTarget)
        return FALSE;
    if (drive) GetDrive(*drive);
    if (directory) GetDirectory(*directory);
    if (name) GetName(*name);
    if (extension) GetExtension(*extension);
    return TRUE;
}
