


cd ../Output_Dir_Mine
x=$(find . -type f -not -name "*.csv" -not -name "*.txt" | cut -d/ -f2 | sort | uniq -c)
echo $x

my_array=($(echo $x | tr "[\n\t]" " "))

declare -A dict

i=0
while [ $i -lt ${#my_array[@]} ];
do
    dict["${my_array[i+1]}"]=$((${my_array[i]}))
    i=$(($i+2))
 
done

cd ..
echo "file type , no_of_files" >> output_mine.csv


for i in "${!dict[@]}"
do
  echo "key  : $i"
  echo "value: ${dict[$i]}"
  echo "$i , ${dict[$i]}" >> output_mine.csv
done