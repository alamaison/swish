set -eu
if [ "${BASH_VERSINFO[0]}" -ge 3 ]; then
    set -o pipefail
fi

IFS=$'\n\t'

KEYFILE=fixture_hostkey
chgrp -v 545 $KEYFILE
chmod -v 600 $KEYFILE
