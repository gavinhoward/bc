# Benchmarks

These are the results of benchmarks comparing this `bc` (at version `2.7.0`) and
GNU `bc` (at version `1.07.1`), both compiled with `clang 9` at `-O2`.

Note: all benchmarks were run four times, and the fastest run is the one shown.
Also, `[bc]` means whichever `bc` was being run, and the assumed working
directory is the root directory of this repository. Also, this `bc` was built at
`-O2`.

### Addition

The command used was:

```
tests/script.sh bc add.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.06
user 1.11
sys 0.95
```

For this `bc`:

```
real 0.98
user 0.95
sys 0.02
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.04
user 1.04
sys 0.99
```

For this `bc`:

```
real 1.02
user 1.00
sys 0.01
```

### Multiplication

The command used was:

```
tests/script.sh bc multiply.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 5.96
user 4.27
sys 1.68
```

For this `bc`:

```
real 2.15
user 2.11
sys 0.04
```

### Division

The command used was:

```
tests/script.sh bc divide.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.74
user 1.84
sys 0.89
```

For this `bc`:

```
real 1.49
user 1.48
sys 0.00
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 9.60
user 9.58
sys 0.01
```

For this `bc`:

```
real 0.67
user 0.66
sys 0.00
```

### Scripts

[This file][1] was downloaded, saved at `../timeconst.bc` and the following
patch was applied:

```
--- ../timeconst.bc	2018-09-28 11:32:22.808669000 -0600
+++ ../timeconst.bc	2019-06-07 07:26:36.359913078 -0600
@@ -110,8 +110,10 @@
 
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
real 15.26
user 14.60
sys 0.64
```

For this `bc`:

```
real 11.24
user 11.23
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
real 14.89
user 14.88
sys 0.00
```

For this `bc`:

```
real 22.19
user 22.18
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
real 42.92
user 32.70
sys 10.19
```

For this `bc`:

```
real 28.50
user 28.44
sys 0.02
```

I have no idea why the performance of both `bc`'s fell off a cliff, especially
the dismal showing by the GNU `bc`.

Note that, when running the benchmarks, the optimization used is not the one I
recommend, which is `-O3 -flto -march=native`. This `bc` separates its code into
modules that, when optimized at link time, removes a lot of the inefficiency
that comes from function overhead. This is most keenly felt with one function:
`bc_vec_item()`, which should turn into just one instruction (on `x86_64`) when
optimized at link time and inlined. There are other functions that matter as
well.

When compiling both `bc`'s with the recommended optimizations, the results are
as follows.

For the first script, the command was:

```
time -p [bc] ../timeconst.bc > /dev/null
```

For GNU `bc`:

```
real 14.01
user 13.41
sys 0.59
```

For this `bc`:

```
real 9.40
user 9.39
sys 0.00
```

For the second script, the command was:

```
time -p [bc] ../test.bc > /dev/null
```

For GNU `bc`:

```
real 12.58
user 12.58
sys 0.00
```

For this `bc`:

```
real 17.99
user 17.98
sys 0.00
```

For the third script, the command was:

```
time -p [bc] ../test2.bc > /dev/null
```

For GNU `bc`:

```
real 39.74
user 27.28
sys 12.44
```

For this `bc`:

```
real 23.31
user 23.27
sys 0.02
```

This is more competitive.

In addition, when compiling with the above recommendation, this `bc` gets even
faster when doing math.

### Recommended Compiler

When I ran these benchmarks with my `bc` compiled under `clang` vs. `gcc`, it
performed much better under `clang`. I recommend compiling this `bc` with
`clang`.

[1]: https://github.com/torvalds/linux/blob/master/kernel/time/timeconst.bc
