#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

tail -n +2 ../webpanel/js/index.js > ../webpanel/js/index.tmp.js
{ printf 'let baseUrl = "";\n'; cat ../webpanel/js/index.tmp.js; } > ../webpanel/js/index.tmp.new.js
mv ../webpanel/js/index.tmp.new.js ../webpanel/js/index.tmp.js

html-minifier --collapse-whitespace ../webpanel/index.html -o ../webpanel/index.min.html
uglifyjs  --compress --mangle -o ../webpanel/js/index.min.js ../webpanel/js/index.tmp.js
uglifycss ../webpanel/css/styles.css > ../webpanel/css/styles.min.css

gzip -k  ../webpanel/index.min.html   
gzip -k  ../webpanel/js/index.min.js
gzip -k  ../webpanel/css/styles.min.css

xxd -i -n index_html ../webpanel/index.min.html.gz > ../include/IndexHtml.h 
sed -i "" "s/\[\]/\[\] PROGMEM/" ../include/IndexHtml.h
{ printf '#include <Arduino.h>\n'; cat ../include/IndexHtml.h; } > ../include/IndexHtml.h.new
mv ../include/IndexHtml.h.new ../include/IndexHtml.h

xxd -i -n index_js ../webpanel/js/index.min.js.gz > ../include/IndexJs.h 
sed -i "" "s/\[\]/\[\] PROGMEM/" ../include/IndexJs.h
{ printf '#include <Arduino.h>\n'; cat ../include/IndexJs.h; } > ../include/IndexJs.h.new
mv ../include/IndexJs.h.new ../include/IndexJs.h

xxd -i -n styles_min_css ../webpanel/css/styles.min.css.gz > ../include/StylesMinCss.h 
sed -i "" "s/\[\]/\[\] PROGMEM/" ../include/StylesMinCss.h
{ printf '#include <Arduino.h>\n'; cat ../include/StylesMinCss.h; } > ../include/StylesMinCss.h.new
mv ../include/StylesMinCss.h.new ../include/StylesMinCss.h

rm -f ../webpanel/js/index.tmp.js
rm -f  ../webpanel/index.min.html   
rm -f  ../webpanel/css/styles.min.css
rm -f  ../webpanel/js/index.min.js
rm -f  ../webpanel/index.min.html.gz   
rm -f  ../webpanel/js/index.min.js.gz 
rm -f  ../webpanel/css/styles.min.css.gz 
