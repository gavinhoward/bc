# Benchmarks

These are the results of benchmarks comparing this `bc` (at version `2.1.0`) and
GNU `bc` (at version `1.07.1`).

Note: all benchmarks were run four times, and the fastest run is the one shown.
Also, `[bc]` means whichever `bc` was being run, and the assumed working
directory is the root directory of this repository. Also, this `bc` was built at
`-O2`.

Note: some mistakes were made when updating these benchmarks for `2.1.0`.
First, I did not update this `bc`'s version in this file. Second, I ran this
`bc` after compiling with `clang` when the GNU `bc` was almost certainly
compiled with `gcc`. Those mistakes have been fixed.

### Addition

The command used was:

```
tests/script.sh bc add.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: add.bc

real 2.06
user 1.09
sys 0.96
```

For this `bc`:

```
Running bc script: add.bc

real 0.95
user 0.90
sys 0.05
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: subtract.bc

real 2.08
user 1.13
sys 0.94
```

For this `bc`:

```
Running bc script: subtract.bc

real 0.92
user 0.88
sys 0.04
```

### Multiplication

The command used was:

```
tests/script.sh bc multiply.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: multiply.bc

real 5.54
user 3.72
sys 1.81
```

For this `bc`:

```
Running bc script: multiply.bc

real 2.06
user 2.01
sys 0.05
```

### Division

The command used was:

```
tests/script.sh bc divide.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: divide.bc

real 2.80
user 1.68
sys 1.11
```

For this `bc`:

```
Running bc script: divide.bc

real 1.45
user 1.42
sys 0.02
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 11.46
user 11.45
sys 0.00
```

For this `bc`:

```
real 0.75
user 0.75
sys 0.00
```

### Scripts

[This file][1] was downloaded, saved at `../timeconst.bc` and the following
patch was applied:

```
--- tests/bc/scripts/timeconst.bc	2018-09-28 11:32:22.808669000 -0600
+++ ../timeconst.bc	2019-06-07 07:26:36.359913078 -0600
@@ -108,8 +108,10 @@
 
 		print "#endif /* KERNEL_TIMECONST_H */\n"
 	}
-	halt
 }
 
-hz = read();
-timeconst(hz)
+for (i = 0; i <= 50000; ++i) {
+	timeconst(i)
+}
+
+halt
```

The command used was:

```
time -p [bc] ../timeconst.bc > /dev/null
```

For GNU `bc`:

```
real 15.16
user 14.59
sys 0.56
```

For this `bc`:

```
real 11.63
user 11.63
sys 0.00
```

Because this `bc` is faster when doing math, it might be a better comparison to
run a script that is not running any math. As such, I put the following into
`../test.bc`:

```
for (i = 0; i < 100000000; ++i) {
	y = i
}

i
y

halt
```

The command used was:

```
time -p [bc] ../test.bc > /dev/null
```

For GNU `bc`:

```
real 12.84
user 12.84
sys 0.00
```

For this `bc`:

```
real 21.20
user 21.20
sys 0.00
```

However, when I put the following into `../test2.bc`:

```
i = 0

while (i < 100000000) {
	++i
}

i

halt
```

the results were surprising.

The command used was:

```
time -p [bc] ../test2.bc > /dev/null
```

For GNU `bc`:

```
real 13.80
user 13.80
sys 0.00
```

For this `bc`:

```
real 14.90
user 14.90
sys 0.00
```

It seems that my `bc` runs `while` loops faster than `for` loops. I don't know
why it does that because both loops are using the same code underneath the hood.

Note that, when running the benchmarks, the optimization used is not the one I
recommend, which is `-O3 -flto -march=native`. This `bc` separates its code into
modules that, when optimized at link time, removes a lot of the inefficiency
that comes from function overhead. This is most keenly felt with one function:
`bc_vec_item()`, which should turn into just one instruction (on `x86_64`) when
optimized at link time and inlined. There are other functions that matter as
well.

When compiling this `bc` with the recommended optimizations, the results are as
follows.

For the first script:

```
real 9.85
user 9.85
sys 0.00
```

For the second script:

```
real 18.04
user 18.04
sys 0.00
```

For the third script:

```
real 12.66
user 12.66
sys 0.00
```

This is more competitive.

In addition, when compiling with the above recommendation, this `bc` gets even
faster when doing math.

### Recommended Compiler

When I ran these benchmarks with my `bc` compiled under `clang`, it performed
much better. I recommend compiling this `bc` with `clang`.

[1]: https://github.com/torvalds/linux/blob/master/kernel/time/timeconst.bc
