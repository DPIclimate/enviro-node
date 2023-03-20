import os, subprocess

# Import the current working construction
# environment to the `env` variable.
# alias of `env = DefaultEnvironment()`
Import('env')

print('====== START PRE SCRIPT')

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

print(commit_const)

with open(_VERSION_H, 'w') as version_h:
    version_h.write(commit_const)

print('====== END PRE SCRIPT')
