#!/usr/bin/env python3
"""Build before testing, and reject Vulkan validation errors even on exit zero."""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path, help='Configured standalone renderer test build')
parser.add_argument('--jobs', type=int, default=8)
options = parser.parse_args()
folder = options.build.resolve()
environment = os.environ.copy()
subprocess.run(['cmake', '--build', str(folder), '-j', str(options.jobs)], check=True, env=environment)
subprocess.run(['ctest', '--test-dir', str(folder), '--output-on-failure', '--no-tests=error'], check=True, env=environment)
log = (folder / 'Testing/Temporary/LastTest.log').read_text(errors='replace')
errors = [line for line in log.splitlines() if any(marker in line for marker in ('Validation Error', 'VUID-', 'SYNC-HAZARD'))]
if errors:
    print('\n'.join(errors))
    raise SystemExit('Renderer checks failed: Vulkan validation errors were reported')
print('PASS fresh build, renderer tests and validation log scan')
