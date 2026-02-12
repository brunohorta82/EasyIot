#!/usr/bin/env bash
set -euo pipefail

# Resolve project root no matter where you run the script from.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB="$ROOT/webpanel"
INC="$ROOT/include"

JS_SRC="$WEB/js/index.js"
CSS_SRC="$WEB/css/styles.css"
HTML_SRC="$WEB/index.html"

JS_TMP="$WEB/js/index.tmp.js"
HTML_MIN="$WEB/index.min.html"
JS_MIN="$WEB/js/index.min.js"
CSS_MIN="$WEB/css/styles.min.css"

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Missing required command: $1" >&2
    exit 1
  fi
}

require_cmd html-minifier
require_cmd uglifyjs
require_cmd uglifycss
require_cmd gzip
require_cmd xxd
require_cmd sed

echo "== Building webpanel assets =="
echo "" # add space betwen lines
echo "ROOT: $ROOT"
echo "" # add space betwen lines

# 1) Prepare JS temp: drop first line, inject baseUrl line.
tail -n +2 "$JS_SRC" > "$JS_TMP"
{ printf 'let baseUrl = "";\n'; cat "$JS_TMP"; } > "$JS_TMP.new"
mv "$JS_TMP.new" "$JS_TMP"

# 2) Minify assets.
html-minifier --collapse-whitespace "$HTML_SRC" -o "$HTML_MIN"
uglifyjs --compress --mangle -o "$JS_MIN" "$JS_TMP"
uglifycss "$CSS_SRC" > "$CSS_MIN"

# 3) Gzip (deterministic: no original filename/timestamp in header).
gzip -kfn "$HTML_MIN"
gzip -kfn "$JS_MIN"
gzip -kfn "$CSS_MIN"

sed_inplace() {
  # macOS-friendly in-place sed.
  sed -i.bak "$1" "$2"
  rm -f "$2.bak"
}

prepend_include() {
  local file="$1"
  local tmp="${file}.tmp"
  { printf '#include <Arduino.h>\n'; cat "$file"; } > "$tmp"
  mv "$tmp" "$file"
}

# 4) Convert to PROGMEM headers.
xxd -i -n index_html "$HTML_MIN.gz" > "$INC/IndexHtml.h"
sed_inplace 's/\[\]/\[\] PROGMEM/' "$INC/IndexHtml.h"
sed_inplace 's/^unsigned char /const uint8_t /' "$INC/IndexHtml.h"
prepend_include "$INC/IndexHtml.h"

xxd -i -n index_js "$JS_MIN.gz" > "$INC/IndexJs.h"
sed_inplace 's/\[\]/\[\] PROGMEM/' "$INC/IndexJs.h"
sed_inplace 's/^unsigned char /const uint8_t /' "$INC/IndexJs.h"
prepend_include "$INC/IndexJs.h"

xxd -i -n styles_min_css "$CSS_MIN.gz" > "$INC/StylesMinCss.h"
sed_inplace 's/\[\]/\[\] PROGMEM/' "$INC/StylesMinCss.h"
sed_inplace 's/^unsigned char /const uint8_t /' "$INC/StylesMinCss.h"
prepend_include "$INC/StylesMinCss.h"

# 5) Cleanup temporary/minified artifacts.
rm -f "$JS_TMP" "$HTML_MIN" "$CSS_MIN" "$JS_MIN" \
  "$HTML_MIN.gz" "$CSS_MIN.gz" "$JS_MIN.gz"

echo "== Done. Generated: include/IndexHtml.h include/IndexJs.h include/StylesMinCss.h =="
echo "" # add space betwen lines
