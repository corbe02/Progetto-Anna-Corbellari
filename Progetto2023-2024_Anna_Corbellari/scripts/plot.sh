#!/bin/bash

# Check if GNU Plot is installed
if ! command -v gnuplot &> /dev/null; then
    echo "GNU Plot is not installed. Please install it before running this script."
    exit 1
fi

# Check if two input files are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <dataset.csv> <centroids.csv>"
    exit 1
fi


dataset="$1"
centroids="$2"

#Print the two series using gnuplot as a dotting plot
gnuplot -p -e "set datafile separator ','; plot '$dataset' using 1:2 with points pointtype 7 pointsize 0.5 linecolor rgb 'blue' title 'Dataset', '$centroids' using 1:2 with points pointtype 7 pointsize 2 linecolor rgb 'red' title 'Centroids'; set key below;replot"
