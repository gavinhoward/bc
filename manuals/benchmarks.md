# Benchmarks

These are the results of benchmarks comparing this `bc` (at version `2.0.3`) and
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

real 2.24
user 1.13
sys 1.11
```

For this `bc`:

```
Running bc script: add.bc

real 1.32
user 1.30
sys 0.02
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 0 1 1 [bc]
```

For GNU `bc`:

```
Running bc script: subtract.bc

real 2.28
user 1.33
sys 0.94
```

For this `bc`:

```
Running bc script: subtract.bc

real 1.37
user 1.34
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

real 6.02
user 4.06
sys 1.92
```

For this `bc`:

```
Running bc script: multiply.bc

real 2.59
user 2.53
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

real 2.94
user 1.85
sys 1.09
```

For this `bc`:

```
Running bc script: divide.bc

real 1.91
user 1.90
sys 0.00
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 11.81
user 11.80
sys 0.00
```

For this `bc`:

```
real 0.76
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
+for (i = 0; i <= 10000; ++i) {
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
real 3.03
user 2.90
sys 0.13
```

For this `bc`:

```
real 4.41
user 4.41
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
real 13.46
user 13.46
sys 0.00
```

For this `bc`:

```
real 24.72
user 24.72
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
real 53.85
user 39.50
sys 14.34
```

For this `bc`:

```
real 26.61
user 26.58
sys 0.03
```

It seems that both `bc`'s run `for` loops faster than `while` loops. I don't
know why my `bc` does that because both loops are using the same code underneath
the hood.

Note that, when running the benchmarks, the optimization used is not the one I
recommend, which is `-O3 -flto -march=native`. This `bc` separates its code into
modules that, when optimized at link time, removes a lot of the inefficiency
that comes from function overhead. This is most keenly felt with one function:
`bc_vec_item()`, which should just turn into one instruction (on `x86_64`) when
optimized at link time and inlined. There are other functions that matter as
well.

When compiling this `bc` with the recommended optimizations, the results are as
follows.

For the first script:

```
real 1.88
user 1.88
sys 0.00
```

For the second script:

```
real 19.43
user 19.43
sys 0.00
```

For the third script:

```
real 20.58
user 20.53
sys 0.03
```

This is more competitive.

In addition, when compiling with the above recommendation, this `bc` gets even
faster when doing math.

[1]: https://github.com/torvalds/linux/blob/master/kernel/time/timeconst.bc
