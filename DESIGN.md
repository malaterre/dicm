I could not decide what to do with stream_filemetaelements so I left it as an
option. I do not believe most people actually care about the internal of the
file meta information.

Advanced user (and some filter) can turn it on. This makes it trivial to
reproduce behavior of tools such as dcdump/dcmdump or dicomdump.


---
START_DATASET
END_DATASET
---

// Short term
dicm -h
```
#dicm -print_format text -i input.dcm > output.txt
#dicm -print_format text -show_private -i input.dcm > output.txt
#dicm -print_format json -i input.dcm > output.json
#dicm -print_format xml  -i input.dcm > output.xml

dicm -i input.dcm -f dcmdump output.txt
dicm -i input.dcm -f dcm output.dcm
dicm -i input.dcm -f xml output.xml
dicm -i input.dcm -f json output.json
dicm -i input.dcm -f json --experimental output.json
dicm -i input.dcm -f json --show_private output.json
dicm -i input.dcm -f json --show_pdb --show_csa --show_asn1 output.json
dicm -f dcm - < input.dcm > output.dcm

// Long term:
dicm -i input.dcm -f group-length-recalc output.json

dicm -i input.dcm -f group-length-recalc output.json
dicm -i input.dcm -f group-length-create output.json
dicm -i input.dcm -f group-length-remove output.json

dicm -i input.dcm -f ts-implicit output.json
dicm -i input.dcm -f ts-explicit output.json
```
