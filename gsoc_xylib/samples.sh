#!/bin/bash

for i in test/*/[^R]*; do
	echo processing file $i ...
	#outpath=$(echo $i | sed -e 's@^test/\([^.]*\)\..*@out2/\1.txt@')
	outpath=$(echo $i | sed -e 's@^test/@out2/@')"_tr.txt"
	mkdir -p $(dirname $outpath)
	./xyconv -m $i $outpath && echo OK
done
