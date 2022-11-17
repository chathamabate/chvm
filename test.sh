#!/bin/sh

path_len=0
line="hey"
hello=$line
line+=" $hello" 
line+=" $hello"
echo $line


if [ "hello" = "ello" ]; then
    echo "Hello world\n"
else echo "meh"; fi;

# Take a list of strings....
# Turn them into one printf command???
# Print each one actually...

x=10
# This actually works!!!!
if ((x - 9 == 1)); then
    echo "What\n"
fi

#for n in $b; do
#    printf "$n\n"
#done

# Takes a space separated list of strings
