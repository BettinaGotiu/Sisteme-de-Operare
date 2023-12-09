#!/bin/bash

# Verifica daca script-ul a primit exact un argument
if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <caracter>"
    exit 1
fi

# Caracterul primit ca argument
caracter="$1"

# Contor pentru propozitiile corecte
contor=0

# Parcurge fiecare fisier cu extensia .txt din directorul specificat
for fisier in director1/*.txt; do
    if [ -f "$fisier" ]; then
        # Citeste linii din fiecare fisier pana la end-of-file
        while IFS= read -r linie; do
            # Verifica daca linia este o propozitie corecta
            if echo "$linie" | grep -e "^[A-Z][a-zA-Z0-9 ,.!]*[.!?]$" | grep -v ", si" | grep -v ",si"; then
                # Verifica daca nu contine virgula (,) inainte de "si"
                if ! echo "$linie" | grep -qE ", si"; then
                    contor=$((contor + 1))
                fi
            fi
        done < "$fisier"
    fi
done

# Afiseaza rezultatul final
echo "$contor"
