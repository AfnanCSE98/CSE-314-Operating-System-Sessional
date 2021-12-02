#!/usr/bin/env bash
# assuming input file and working dir resides in the current directory
inputfile=""
working_dir=$pwd

if [ $# = "0" ]; then

	echo "No argument from cmd!!working directory name (optional) and input file name must be given"

elif [ $# = "1" ]; then
	if [[ $1 == *".txt"* ]]; then 
  	find ./ -type f -name $1 | grep -q $1 > /dev/null && inputfile=$1 
	else
		echo "Provide an input file name"
    #searching working_dir in parent dir
		find ../ -type d -name $1 | grep -q $1 > /dev/null && working_dir=`find ../ -type d -name $1` 
	fi

elif [ $# = "2" ]; then
	if [[ "$2" == *".txt"* ]]; then 
    find ./ -type f -name $2 | grep -q $2 > /dev/null && inputfile=$2
	else
		echo "Provide an input file name"
	fi	

	find ./ -type d -name $1 | grep -q $1 > /dev/null && working_dir=`find ./ -type d -name $1` 

else
	echo "Only working directory name (optional) and input file name should be given"
fi


echo "Working Directory : $working_dir"
echo "Input file : $inputfile"

#chk if wrong input file was given
while [ "$inputfile" == "" ] ; do 
echo "Provide valid input file"
read name 
if [[ $name == *".txt"* ]]; then
  find ./ -type f -name $name | grep -q $name > /dev/null && inputfile=$name
fi
done 


#read input file and store in arr
arr=()
while read -r line; 
  do
    x=$(echo $line | tr -d '\r')
    arr+=($x)
  done < $inputfile


#output directory
mkdir -p ../output_dir_1705098

declare -A dict

no_of_ignored=0;

for fullfile in $(find $working_dir -type f | sort -nr | cut -d: -f2- );
do
  filename=$(basename -- "$fullfile")
  extension="${filename##*.}"
  fname="${filename%.*}"
  
  #if extension not allowed,continue
  allowed=1
  for i in "${!arr[@]}"
  do 

    if [[ "${arr[i]}" == "$extension" ]]; then 
      allowed=0
      break
    fi 
  done

  if [ $allowed -eq 0 ]; then
    no_of_ignored=$(($no_of_ignored+1))
    continue
  fi 

  #store files in output_dir  
  if [[ "$fname" == "$extension" ]]; #file has no extension
  then
      mkdir -p ../output_dir_1705098/Others
      cp $fullfile ../output_dir_1705098/Others
      if [ ! -e "../output_dir_1705098/Others/desc_others.txt" ] ; then
        touch "../output_dir_1705098/Others/desc_others.txt"
      fi
      #append filepath
      x=${PWD##*/} 
      y=${fullfile:1}
      echo "$x$y" >> "../output_dir_1705098/Others/desc_others.txt"
      
  else
      mkdir -p ../output_dir_1705098/$extension
      cp $fullfile ../output_dir_1705098/$extension
      if [ ! -e "../output_dir_1705098/$extension/desc_${extension}.txt" ] ; then
        touch "../output_dir_1705098/$extension/desc_${extension}.txt"
      fi
      #append filepath
      x=${PWD##*/} 
      y=${fullfile:1}
      
      echo "$x$y" >> ../output_dir_1705098/$extension/desc_${extension}.txt
    
  fi

done

#creating CSV file

cd ../output_dir_1705098
x=$(find . -type f | cut -d/ -f2 | sort | uniq -c)
#echo $x

my_array=($(echo $x | tr "[\n\t]" " "))

i=0
while [ $i -lt ${#my_array[@]} ];
do
    dict["${my_array[i+1]}"]=$((${my_array[i]}-1))
    i=$(($i+2))
 
done

cd ..
cp /dev/null output_1705098.csv #clearing csv file
echo "file type , no_of_files" >> output_1705098.csv


for i in "${!dict[@]}"
do
  #echo "key  : $i"
  #echo "value: ${dict[$i]}"
  echo "$i , ${dict[$i]}" >> output_1705098.csv
done

echo "ignored , $no_of_ignored" >> output_1705098.csv