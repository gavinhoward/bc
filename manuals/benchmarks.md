# Benchmarks

These are the results of benchmarks comparing this `bc` (at version `2.0.0`) and
GNU `bc` (at version `1.07.1`).

Note: all benchmarks were run four time, and the fastest run is the one shown.
Also, `[bc]` means whichever `bc` was being run, and the assumed working
directory is the root directory of this repository. Also, this `bc` was built at
`-O2`.

### Addition

The command used was:

```
tests/script bc add.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: add.bc

real 0.80
user 0.37
sys 0.43
```

For this `bc`:

```
Running bc script: add.bc

real 0.56
user 0.55
sys 0.00
```

### Subtraction

The command used was:

```
tests/script bc subtract.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: subtract.bc

real 0.82
user 0.40
sys 0.42
```

For this `bc`:

```
Running bc script: subtract.bc

real 0.57
user 0.55
sys 0.01
```

### Multiplication

The command used was:

```
tests/script bc multiply.bc 0 1 1 [bc]
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
tests/script bc divide.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: divide.bc

real 2.89
user 1.62
sys 1.26
```

For this `bc`:

```
Running bc script: divide.bc

real 2.24
user 2.22
sys 0.01
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
