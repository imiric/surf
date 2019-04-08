#!/bin/bash
# Bookmark URL on pinboard.in
#
# External requirements:
# - https://github.com/davatorium/rofi
# - https://github.com/stedolan/jq
# - https://pypi.org/project/pinboard/
# - https://joeyh.name/code/moreutils/

set -euo pipefail

wid="$1"
uri="$2"
title="$3"
prompt="$4"

bmarkfile="$HOME/.surf/bookmarks.json"

# Sort and deduplicate inputs while maintaining order
# See https://stackoverflow.com/a/20639730
stable_sort_uniq() {
    tr -s ' ' '\n' | cat -n | sort -k2 -k1n | uniq -f1 | sort -nk1,1 | cut -f2- | xargs
}

# Get suggested tags from Pinboard, merge, dedupe and pass them to rofi
rofiout="$(printf '%b' "$(pinboard suggest-tags --url "$uri" \
  | jq -r '.[0].popular + .[1].recommended | .[]' | stable_sort_uniq \
  | rofi -dmenu -mesg $uri -p $prompt -multi-select -format 's f' -location 7 \
      -lines 10 -click-to-exit --release -width 100% -m wid:$wid \
      -font 'Iosevka Term ss08 14' -sep ' ' -async-pre-read 0 \
      -theme-str 'window { children: [inputbar, message, listview]; } listview { scrollbar: false; }' \
)")"
# Cleanup rofi's custom -format output to append custom tags after suggested tags
tags="$(echo $(echo "$rofiout" | cut -d' ' -f1 | xargs) $(echo "$rofiout" | head -1 | cut -d' ' -f2-) | stable_sort_uniq)"

if [ -n "$tags" ]; then
  # '-private' and '-read' are treated as special tags to store a
  # private bookmark and an unread bookmark, respectively.
  # Note that `--read` is applied by default, and the '-read' tag
  # marks the bookmark as *unread*.
  pbargs=""
  [[ " $tags " == *" -private "* ]] && pbargs="${pbargs} --private"
  [[ " $tags " == *" -read "* ]] && pbargs="${pbargs} --unread" || pbargs="${pbargs} --read"
  tags=$(echo $tags | tr ' ' '\n' | grep -Ev '^\-(read|private)$' | xargs)
  pinboard add --url "$uri" --title "$title" --tags "$tags" $pbargs

  { # Update the bookmark file in the background
  # Get new bookmarks since last modified timestamp or Unix epoch (i.e. fetch all bookmarks)
  bmarkupdate=$(date -u -d "@$(stat -c%Y $bmarkfile 2>/dev/null || echo 0)")
  newbmarks=$(pinboard bookmarks --from_date="$bmarkupdate")
  # Merge new bookmarks with bookmark file, dedupe and sort in reverse timestamp order
  jq --argjson newBmarks "$newbmarks" \
     '$newBmarks + . | unique | sort_by(.time) | reverse' "$bmarkfile" | sponge "$bmarkfile"
  } &

  resultmsg="Bookmarked $uri with tags '$tags'"
  colors="#262626,#0087af"
else
  resultmsg="No tags provided; not bookmarking $uri"
  colors="#262626,#f9d72d"
fi

rofi -location 7 -width 100% -m "wid:$wid" -font 'Iosevka Term ss08 14' \
  -e "$resultmsg" -click-to-exit --release -color-normal "$colors"
