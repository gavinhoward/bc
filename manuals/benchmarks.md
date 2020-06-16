# Benchmarks

The results of these benchmarks suggest that building this `bc` with
optimization at `-O3` with link-time optimization (`-flto`) will result in the
best performance.

*Note*: all benchmarks were run four times, and the fastest run is the one
shown. Also, `[bc]` means whichever `bc` was being run, and the assumed working
directory is the root directory of this repository. Also, this `bc` was at
version `3.0.0` while GNU `bc` was at version `1.07.1`, and all tests were
conducted on an `x86_64` machine running Gentoo Linux with `clang` `9.0.1` as
the compiler.

## Typical Optimization Level

These benchmarks were run with both `bc`'s compiled with the typical `-O2`
optimizations and no link-time optimization.

### Addition

The command used was:

```
tests/script.sh bc add.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.59
user 1.12
sys 1.47
```

For this `bc`:

```
real 0.90
user 0.87
sys 0.03
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.64
user 1.15
sys 1.48
```

For this `bc`:

```
real 0.96
user 0.92
sys 0.03
```

### Multiplication

The command used was:

```
tests/script.sh bc multiply.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 7.51
user 5.01
sys 2.48
```

For this `bc`:

```
real 2.30
user 2.22
sys 0.08
```

### Division

The command used was:

```
tests/script.sh bc divide.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 3.53
user 2.01
sys 1.51
```

For this `bc`:

```
real 1.70
user 1.67
sys 0.02
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 11.81
user 11.76
sys 0.02
```

For this `bc`:

```
real 0.75
user 0.74
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
real 17.66
user 16.89
sys 0.72
```

For this `bc`:

```
real 14.09
user 14.06
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
real 17.25
user 17.22
sys 0.00
```

For this `bc`:

```
real 24.94
user 24.88
sys 0.01
```

I also put the following into `../test2.bc`:

```
i = 0

while (i < 100000000) {
	i += 1
}

i

halt
```

The command used was:

```
time -p [bc] ../test2.bc > /dev/null
```

For GNU `bc`:

```
real 18.18
user 18.15
sys 0.00
```

For this `bc`:

```
real 17.91
user 17.87
sys 0.00
```

It seems that the improvements to the interpreter helped a lot in certain cases.

## Recommended Optimizations from `2.7.0`

Note that, when running the benchmarks, the optimizations used are not the ones
I recommended for version `2.7.0`, which are `-O3 -flto -march=native`. This
`bc` separates its code into modules that, when optimized at link time, removes
a lot of the inefficiency that comes from function overhead. This is most keenly
felt with one function: `bc_vec_item()`, which should turn into just one
instruction (on `x86_64`) when optimized at link time and inlined. There are
other functions that matter as well.

When compiling both `bc`'s with the optimizations I recommend for this `bc`, the
results are as follows.

### Addition

The command used was:

```
tests/script.sh bc add.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.55
user 1.13
sys 1.40
```

For this `bc`:

```
real 0.92
user 0.85
sys 0.06
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.55
user 1.05
sys 1.48
```

For this `bc`:

```
real 0.96
user 0.89
sys 0.06
```

### Multiplication

The command used was:

```
tests/script.sh bc multiply.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 7.34
user 4.80
sys 2.51
```

For this `bc`:

```
real 2.30
user 2.21
sys 0.08
```

### Division

The command used was:

```
tests/script.sh bc divide.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 3.43
user 1.91
sys 1.51
```

For this `bc`:

```
real 1.68
user 1.65
sys 0.02
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 11.53
user 11.49
sys 0.01
```

For this `bc`:

```
real 0.76
user 0.76
sys 0.00
```

### Scripts

The command for the `../timeconst.bc` script was:

```
time -p [bc] ../timeconst.bc > /dev/null
```

For GNU `bc`:

```
real 16.35
user 15.66
sys 0.65
```

For this `bc`:

```
real 13.98
user 13.93
sys 0.02
```

The command for the next script, the `for` loop script, was:

```
time -p [bc] ../test.bc > /dev/null
```

For GNU `bc`:

```
real 15.37
user 15.34
sys 0.00
```

For this `bc`:

```
real 25.06
user 24.99
sys 0.01
```

The command for the next script, the `while` loop script, was:

```
time -p [bc] ../test2.bc > /dev/null
```

For GNU `bc`:

```
real 15.72
user 15.65
sys 0.01
```

For this `bc`:

```
real 17.88
user 17.83
sys 0.00
```

These results surprised me a little. I thought that the performance of my `bc`
would get better with link-time optimization.

Let's see if `-march=native` is the problem.

## Link-Time Optimization Only

The optimizations I used for both `bc`'s were `-O3 -flto`.

### Addition

The command used was:

```
tests/script.sh bc add.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.46
user 1.08
sys 1.36
```

For this `bc`:

```
real 0.60
user 0.53
sys 0.06
```

### Subtraction

The command used was:

```
tests/script.sh bc subtract.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 2.44
user 1.05
sys 1.39
```

For this `bc`:

```
real 0.64
user 0.59
sys 0.04
```

### Multiplication

The command used was:

```
tests/script.sh bc multiply.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 6.94
user 4.35
sys 2.58
```

For this `bc`:

```
real 1.60
user 1.51
sys 0.09
```

### Division

The command used was:

```
tests/script.sh bc divide.bc 1 0 1 1 [bc]
```

For GNU `bc`:

```
real 3.30
user 1.87
sys 1.43
```

For this `bc`:

```
real 1.29
user 1.25
sys 0.04
```

### Power

The command used was:

```
printf '1234567890^100000; halt\n' | time -p [bc] -lq > /dev/null
```

For GNU `bc`:

```
real 10.71
user 10.70
sys 0.00
```

For this `bc`:

```
real 0.72
user 0.72
sys 0.00
```

### Scripts

The command for the `../timeconst.bc` script was:

```
time -p [bc] ../timeconst.bc > /dev/null
```

For GNU `bc`:

```
real 15.83
user 15.08
sys 0.72
```

For this `bc`:

```
real 10.57
user 10.54
sys 0.00
```

The command for the next script, the `for` loop script, was:

```
time -p [bc] ../test.bc > /dev/null
```

For GNU `bc`:

```
real 15.32
user 15.30
sys 0.00
```

For this `bc`:

```
real 18.39
user 18.33
sys 0.01
```

The command for the next script, the `while` loop script, was:

```
time -p [bc] ../test2.bc > /dev/null
```

For GNU `bc`:

```
real 15.30
user 15.27
sys 0.00
```

For this `bc`:

```
real 13.09
user 13.07
sys 0.00
```

It turns out that `-march=native` *is* the problem. As such, I have removed the
recommendation to build with `-march=native`.

## Recommended Compiler

When I ran these benchmarks with my `bc` compiled under `clang` vs. `gcc`, it
performed much better under `clang`. I recommend compiling this `bc` with
`clang`.

[1]: https://github.com/torvalds/linux/blob/master/kernel/time/timeconst.bc
