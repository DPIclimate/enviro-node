import subprocess

# Import the current working construction
# environment to the `env` variable.
# alias of `env = DefaultEnvironment()`
Import('env')

print('====== START PRE SCRIPT')

commit_const = 'const char* commit_id = "unknown";'
try:
    result = subprocess.run(['git', 'show', '-s', '--format=%h'], capture_output=True, check=True)
    commit_id = result.stdout.decode('ascii')
    commit_id = commit_id.strip()

    result = subprocess.run(['git', 'status', '--porcelain'], capture_output=True, check=True)
    repo_status = result.stdout.decode('ascii').strip()
    if len(repo_status) > 0:
        repo_status = 'dirty'
    else:
        repo_status = 'clean'

    commit_const = f'#pragma once\nconst char* commit_id = "{commit_id} {repo_status}";'

except:
    pass

print(commit_const)

with open('include/version.h', 'w') as version_h:
    version_h.write(commit_const)

print('====== END PRE SCRIPT')
