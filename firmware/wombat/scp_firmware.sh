set -euo pipefail

cd "$(cd -P -- "$(dirname -- "$0")" && pwd -P)"
if [ -z $"SSH_HOST" ]; then
    echo Environment variable SSH_HOST must be set
    exit 1
fi

if [ -z $"DEST_DIR" ]; then
    echo Environment variable DEST_DIR must be set
    exit 1
fi

./split_firmware.sh
scp .pio/build/wombat/firmware.bin $SSH_HOST:$DEST_DIR/wombat.bin
scp .pio/build/wombat/wombat.sha1 $SSH_HOST:$DEST_DIR
scp data/sdi12defn.json $SSH_HOST:$DEST_DIR
ssh $SSH_HOST ls -l $DEST_DIR
ssh $SSH_HOST cat $DEST_DIR/wombat.sha1
