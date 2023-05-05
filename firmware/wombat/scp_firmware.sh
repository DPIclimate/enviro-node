set -euo pipefail

cd $(dirname $0)
if [ -z $"FTP_HOST" ]; then
    echo Environment variable FTP_HOST must be set
    exit 1
fi

./split_firmware.sh
ssh $FTP_HOST rm /srv/ftp/wombat.*
scp .pio/build/wombat/firmware.bin $FTP_HOST:/srv/ftp/wombat.bin
scp .pio/build/wombat/wombat.sha1 $FTP_HOST:/srv/ftp/wombat.sha1
ssh $FTP_HOST ls -l /srv/ftp
