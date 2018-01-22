./tests/notemp $1 $2 $3 $4 > log1

echo "scale=$4; obase=$3; a=$1; b=$2;
(a+=b); (a-=b); (a*=b); (a/=b);
(b+=a); (b=a-b); (b*=a); (b=a/b);
(a+=a); (a*=a); (a/=a); (a-=a);
quit" | bc -l > log2

diff log1 log2
