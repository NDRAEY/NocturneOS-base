nm -Cl -n $1 | awk '{ if ($2 != "a") print; }' > kernel.map

