import os, subprocess
from pathlib import Path

# Import the current working construction
# environment to the `env` variable.
# alias of `env = DefaultEnvironment()`
Import('env')

print('====== START PRE SCRIPT')

# This code to is make the ulptool-pio scripts work. They must be written against an older
# version of the ESP32 tools and expect things to have different names and locations. A simple
# way to fix one of those errors is to copy the sdkconfig.h file to where ulptool-pio expects
# it to be.

# A different problem cannot be fixed this easily and had to be fixed in a ulptool-pio python
# script which is why we have our own fork of it.

# In ~/.platformio/packages/framework-arduinoespressif32/tools/sdk:
# mkdir -p include/config
# cp esp32/qout_qspi/include/sdkconfig.h include/config

platform = env.PioPlatform()
framework_dir = platform.get_package_dir("framework-arduinoespressif32")
to_path = Path(framework_dir, 'tools/sdk/include/config')
from_path = Path(framework_dir, f'tools/sdk/{env["BOARD_MCU"]}/qout_qspi/include/sdkconfig.h')

if from_path.exists():
    to_path.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(['cp', str(from_path), str(to_path)], capture_output=True, check=True)

# End of ulptool-pio hack

_VERSION_H = 'include/version.h'

commit_const = 'const char* commit_id = "unknown";'

result = subprocess.run(['git', 'show', '-s', '--format=%h'], capture_output=True, check=True)
commit_id = result.stdout.decode('ascii')
commit_id = commit_id.strip()

try:
    os.remove(_VERSION_H)
except:
    pass

result = subprocess.run(['git', 'status', '--porcelain'], capture_output=True, check=True)
repo_status = result.stdout.decode('ascii').strip()
if len(repo_status) > 0:
    repo_status = 'dirty'
else:
    repo_status = 'clean'

commit_const = f'#pragma once\nconst char* commit_id = "{commit_id} {repo_status}";'

with open(_VERSION_H, 'w') as version_h:
    version_h.write(commit_const)

print('====== END PRE SCRIPT')
