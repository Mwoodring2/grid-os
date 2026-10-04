"""Compile meaningful boot policy tests; requires g++ or an MSVC developer shell."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import sys
root = Path(__file__).resolve().parents[1]
compiler = shutil.which('g++') or shutil.which('cl')
if not compiler:
    print('Host tests unavailable: use g++ or an MSVC Developer PowerShell.')
    sys.exit(2)
with tempfile.TemporaryDirectory(prefix='grid-host-tests-') as temp:
    for suite in ('image_policy_test', 'boot_manager_test', 'installed_app_test', 'grid_effects_test'):
        binary = Path(temp) / (suite + ('.exe' if sys.platform == 'win32' else ''))
        includes = [root/'tests/stubs', root/'include']
        source = root/'tests'/f'{suite}.cpp'
        if Path(compiler).stem.lower() == 'cl':
            args = [compiler, '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX']
            args += [f'/I{p}' for p in includes] + [str(source), f'/Fe:{binary}']
        else:
            args = [compiler, '-std=c++17', '-Wall', '-Wextra', '-Werror']
            args += [f'-I{p}' for p in includes] + [str(source), '-o', str(binary)]
        subprocess.run(args, cwd=temp, check=True)
        subprocess.run([str(binary)], check=True)
