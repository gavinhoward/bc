# Benchmarks

These are the results of benchmarks comparing this `bc` (at version `2.0.0`) and
GNU `bc` (at version `1.07.1`).

Note: all benchmarks were run four times, and the fastest run is the one shown.
Also, `[bc]` means whichever `bc` was being run, and the assumed working
directory is the root directory of this repository. Also, this `bc` was built at
`-O2`.

### Addition

The command used was:

```
tests/script.sh bc add.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: add.bc

real 2.08
user 1.01
sys 1.06
```

For this `bc`:

```
Running bc script: add.bc

real 1.29
user 1.24
sys 0.04
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: subtract.bc

real 2.12
user 1.12
sys 1.00
```

For this `bc`:

```
Running bc script: subtract.bc

real 1.33
user 1.31
sys 0.02
```

### Multiplication

The command used was:

```
tests/script.sh bc multiply.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: multiply.bc

real 5.74
user 3.95
sys 1.79
```

For this `bc`:

```
Running bc script: multiply.bc

real 2.59
user 2.55
sys 0.03
```

### Division

The command used was:

```
tests/script.sh bc divide.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: divide.bc

real 2.95
user 1.89
sys 1.06
```

For this `bc`:

```
Running bc script: divide.bc

real 1.78
user 1.78
sys 0.00
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 12.08
user 12.08
sys 0.00
```

For this `bc`:

```
real 0.81
user 0.81
sys 0.00
```

### Scripts

A script was created that contained the following:

```
#!/bin/sh

exe="$1"

for i in $(seq 0 1000); do
	printf '%d\n' "$i" | "$exe" -q tests/bc/scripts/timeconst.bc > /dev/null
done
```

where `tests/bc/scripts/timeconst.bc` is [this file][1]. The script was saved at
`../test.sh`.

The command used was:

```
time -p sh ../test.sh [bc]
```

For GNU `bc`:

```
real 1.30
user 1.22
sys 0.23
```

For this `bc`:

```
real 1.91
user 1.73
sys 0.38
```

[1]: https://github.com/torvalds/linux/blob/master/kernel/time/timeconst.bc
