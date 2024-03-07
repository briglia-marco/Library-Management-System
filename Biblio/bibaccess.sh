#!/bin/bash

options=$1

if [[ $options != "--query" && $options != "--loan" ]]; then
  echo "Utilizzo: $0 [--query|--loan] file1.log ... fileN.log"
  exit 1
fi

shift

files=("$@")
query="QUERY"
loan="LOAN"
querynum=0
loannum=0
querytot=0
loantot=0
querynumarr=()
loannumarr=()

for file in "${files[@]}"; do
    querynum=0
    loannum=0
    while read -r line; do
    if [[ $line == *$query* ]]; then
        num=$(echo $line | tr -dc '0-9')
        querynum=$((querynum + num))

    elif [[ $line == *$loan* ]]; then
        num=$(echo $line | tr -dc '0-9')
        loannum=$((loannum + num))
    fi
    done < $file
    querynumarr+=($querynum)
    loannumarr+=($loannum)
done

if [[ $options == "--query" ]]; then
    for((i=0;i<${#querynumarr[@]};i++)); do
        querytot=$((querytot + querynumarr[i]))
        echo "${files[$i]} ${querynumarr[$i]}"
    done
    echo "QUERY $querytot"
fi

if [[ $options == "--loan" ]]; then
    for((i=0;i<${#loannumarr[@]};i++)); do
        loantot=$((loantot + loannumarr[i]))
        echo "${files[$i]} ${loannumarr[$i]}"
    done
    echo "LOAN $loantot"
fi