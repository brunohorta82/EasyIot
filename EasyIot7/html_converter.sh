gzip -k  ../html/devices.html
gzip -k  ../html/index.html   
gzip -k  ../html/node.html

xxd -i -n devices_html ../html/devices.html.gz > include/DevicesHtml.h 
sed -i "" "s/unsigned char index_html/unsigned char index_html PROGMEM/" include/DevicesHtml.h
echo "#include <Arduino.h>\n\r$(cat include/DevicesHtml.h)" > include/DevicesHtml.h 

xxd -i -n index_html ../html/index.html.gz > include/IndexHtml.h 
sed -i "" "s/unsigned char index_html/unsigned char index_html PROGMEM/" include/IndexHtml.h
echo "#include <Arduino.h>\n\r$(cat include/IndexHtml.h)" > include/IndexHtml.h 

xxd -i -n node_html ../html/node.html.gz > include/NodeHtml.h 
sed -i "" "s/unsigned char index_html/unsigned char index_html PROGMEM/" include/NodeHtml.h
echo "#include <Arduino.h>\n\r$(cat include/NodeHtml.h)" > include/NodeHtml.h 

rm -f  ../html/devices.html.gz
rm -f  ../html/index.html.gz   
rm -f  ../html/node.html.gz 