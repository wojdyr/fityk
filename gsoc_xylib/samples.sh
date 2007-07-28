#!/bin/bash
# convert all of the sample files in ./sample to ASCII plain format in ./output

for i in test/*/[^R]* 
do
	echo "processing file $i ..."
	outpath=$(echo $i | sed -e 's@^test/@output/@')"_tr.txt"
	mkdir -p $(dirname $outpath)
	./xyconv -m $i $outpath && echo OK
done
