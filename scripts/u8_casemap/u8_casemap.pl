#!/usr/bin/perl

# DeaDBeeF - The Ultimate Music Player
# Copyright (C) 2009-2011 Oleksiy Yakovenko <waker@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

use utf8;

open F, "<UnicodeData.txt" or die "failed to open UnicodeData.txt";

print "struct u8_case_map_t {\n".
    "    const char *name;\n".
    "    const char *lower;\n".
    "};\n".
    "\%\%\n";

while (<F>) {
    if (/CAPITAL/ && /[^;];$/) {

        /^([^;]+);/;
        $uni_upper = $1;

        /;([^;]+);$/;
        $uni_lower = $1;

        $u8_upper = chr(hex($uni_upper));
        $u8_lower = chr(hex($uni_lower));

        utf8::encode ($u8_upper);
        utf8::encode ($u8_lower);

        print "$u8_upper, \"$u8_lower\"\n";
    }
}

close F;
