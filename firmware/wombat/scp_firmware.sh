set -euo pipefail

cd "$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
if [ -z $"SSH_HOST" ]; then
    echo Environment variable SSH_HOST must be set
    exit 1
fi

./split_firmware.sh
ssh $SSH_HOST rm /srv/ftp/wombat.*
scp .pio/build/wombat/firmware.bin $SSH_HOST:/srv/ftp/wombat.bin
scp .pio/build/wombat/wombat.sha1 $SSH_HOST:/srv/ftp/wombat.sha1
ssh $SSH_HOST ls -l /srv/ftp
ssh $SSH_HOST cat /srv/ftp/wombat.sha1
