// Shared implementation for the platform diagnostics API.  The including
// translation unit supplies PlatformDiagnosticTimestamp(),
// PlatformDiagnosticArchitecture(), and PlatformDiagnosticProcessId().

#include <cstdio>
#include <cstdlib>
#include <mutex>

namespace
{
std::mutex g_diagnosticsMutex;

void WriteDiagnosticsContext(FILE *report, const char *name)
{
   const char *value = std::getenv(name);
   if (value && value[0])
      std::fprintf(report, "%s: %s\n", name, value);
}
}

const char *PlatformDiagnosticsPath(void)
{
   const char *overridePath = std::getenv("DARK_DIAGNOSTICS_PATH");
   return overridePath && overridePath[0]
      ? overridePath : "darkengine-diagnostics.log";
}

void PlatformDiagnosticsStartSession(const char *application, int argc,
                                     const char *const *argv)
{
   std::lock_guard<std::mutex> lock(g_diagnosticsMutex);
   FILE *report = std::fopen(PlatformDiagnosticsPath(), "wb");
   if (!report)
      return;

   char timestamp[64] = {};
   char directory[1024] = {};
   PlatformDiagnosticTimestamp(timestamp, sizeof(timestamp), false);
   PlatformGetCurrentDirectory(directory, sizeof(directory));
   std::fprintf(report, "Dark Engine diagnostic report\n"
                        "=============================\n"
                        "Started: %s\nApplication: %s\n"
                        "Architecture: %s\nProcess: %llu\n"
                        "Game directory: %s\n",
                timestamp, application ? application : "(unknown)",
                PlatformDiagnosticArchitecture(),
                PlatformDiagnosticProcessId(),
                directory[0] ? directory : "(unknown)");
   WriteDiagnosticsContext(report, "DARK_GAME_PROFILE");
   WriteDiagnosticsContext(report, "DARK_CONTENT_MODE");
   WriteDiagnosticsContext(report, "DARK_FM_NAME");
   std::fprintf(report, "Arguments:");
   for (int i = 1; i < argc; ++i)
      std::fprintf(report, "\n  [%d] %s", i,
                   argv && argv[i] ? argv[i] : "");
   std::fprintf(report, "\n\nIssues and checkpoints\n"
                        "----------------------\n");
   std::fclose(report);
}

void PlatformDiagnosticsLog(const char *severity, const char *subsystem,
                            const char *message)
{
   std::lock_guard<std::mutex> lock(g_diagnosticsMutex);
   FILE *report = std::fopen(PlatformDiagnosticsPath(), "ab");
   if (!report)
      return;

   char timestamp[64] = {};
   PlatformDiagnosticTimestamp(timestamp, sizeof(timestamp), true);
   std::fprintf(report, "[%s] %-5s %-12s %s\n", timestamp,
                severity && severity[0] ? severity : "INFO",
                subsystem && subsystem[0] ? subsystem : "engine",
                message ? message : "");
   std::fclose(report);
}
