#!/bin/sh
#
# conf_fix.sh - Run "configure" in an environment where autotools is disabled.
#
# Copyright (C) 2001  Southern Storm Software, Pty Ltd.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

TEMPDIR="/tmp/cf$$"

mkdir "$TEMPDIR"
export PATH="$TEMPDIR:$PATH"

echo '#!/bin/sh
exit 1' >"$TEMPDIR/automake"
echo '#!/bin/sh
exit 1' >"$TEMPDIR/autoconf"
echo '#!/bin/sh
exit 1' >"$TEMPDIR/autoheader"
echo '#!/bin/sh
exit 1' >"$TEMPDIR/aclocal"

chmod +x "$TEMPDIR/automake"
chmod +x "$TEMPDIR/autoconf"
chmod +x "$TEMPDIR/autoheader"
chmod +x "$TEMPDIR/aclocal"

./configure $*
STATUS="$?"

rm -rf "$TEMPDIR"

exit "$STATUS"
