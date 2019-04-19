#!/bin/bash

set -e

CSS_URL="$1"
TEMP="temp.css"

curl -o "$TEMP" "$CSS_URL"
NAME="$(cat "$TEMP" | grep 'font-family' | head -n 1 | cut -d "'" -f 2)"
echo name is $NAME

extract() {
  local input="$1"
  local pattern="$2"
  echo "$input" | sed "s/$pattern/\\1/"
}

mkdir -p "$NAME"
cp "$TEMP" "$NAME"/"$TEMP"
ideal_filename=''
for token in $(cat "$TEMP" | grep -o '\(local\|url\)([^)]*)' | grep -v ' ') ; do
  if echo "$token" | grep -q "local('[a-zA-Z-]*')" ; then
    ideal_filename="$(extract "$token" "[^(]*('\([^']*\)')")"
  fi
  if echo "$token" | grep -q "^url(" ; then
    suffix="$(extract "$token" ".*\.\([a-z0-9]*\))")"
    font_filename="$(extract "$token" ".*\/\([^)]*\))")"
    url="$(extract "$token" ".*(\([^)]*\))")"
    if [ -n "$ideal_filename" ]; then
      font_filename="${ideal_filename}.${suffix}"
      ideal_filename=""  # we used it up
    fi
    curl -o "${NAME}/${font_filename}" "$url"
    sed -i "s~$url~${font_filename}~" "${NAME}/${TEMP}"
  fi
done
mv "${NAME}/${TEMP}" "${NAME}/${NAME}.css"
rm "$TEMP"
