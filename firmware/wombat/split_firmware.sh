set -euo pipefail

cd $(dirname $0)
cd .pio/build/wombat

#split -b 64K -d firmware.bin wombat.

# Ver   Length  SHA-1 hash                               Commit hash
# 1.0.2 1229088 62f3a58758a1b2deb20fdc434f425733af0dd77d a30068bac9220191c15c3cad616fb969770a054e

VERSION=$(fgrep '_VERSION_NUM =' ~/src/enviro-node/firmware/wombat/gen_version_h.py | cut -d= -f2 | tr -d " '")
LENGTH=$(ls -l firmware.bin | cut -w -f5)
FILE_SHA1=$(shasum -a 1 -b firmware.bin | cut -w -f1)
COMMIT_HASH=$(git show --format=%H | head -1)
echo $VERSION $LENGTH $FILE_SHA1 $COMMIT_HASH >wombat.sha1

#rm wombat.tgz
#tar zcf wombat.tgz wombat.*
