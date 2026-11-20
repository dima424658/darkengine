#pragma once

#include <types.h>

// Mission diagnostics are collected by the resource manager so failed binds
// can be deduplicated before the post-load summary is emitted.
EXTERN void MissionDiagnosticsBegin(const char *mission);
EXTERN void MissionDiagnosticsFinish(BOOL loaded);
EXTERN void MissionDiagnosticsFinalSummary(void);
EXTERN void MissionDiagnosticsIssue(const char *severity,
                                    const char *subsystem,
                                    const char *message);
