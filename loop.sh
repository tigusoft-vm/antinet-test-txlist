# just for quick dirty running of code while developing

while true ; do echo -e "\n\n\n\n" ; g++  --std=c++11   -Wall a.cpp -o a.bin && { date ; ./a.bin --simple ; } || { echo "ERROR" ; sleep 2 ; } ; sleep 0.3 ; done

