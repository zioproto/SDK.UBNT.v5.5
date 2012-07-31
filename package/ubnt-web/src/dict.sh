#!/bin/sh
grep -r dict_translate . --exclude="*dict.sh*" --exclude="*.svn*" | sed \
	-e 's/dict_translate/\ndict_translate/g' | \
	grep dict_translate | sed \
	-e 's/^dict_translate("\([^|#]\+\)[|#]\?\(.*\)").*$/\1 => \2/' \
	-e 's/\(").*\?\) =>/ =>/' \
	-e 's/\(").*\?\)$//' \
	-e 's/\(.*\) => $/\1 => \1/' \
	-e 's/\\"/"/g' \
	| grep -v dict_translate | sort | uniq
