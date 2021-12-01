
arr=()
while IFS=$'\n' read -r line; 
  do
    arr+=($line)
  done < "../input.txt"

for i in "${arr[@]}";
do 
    echo "$i" 
done
