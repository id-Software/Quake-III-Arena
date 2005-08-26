int in[] = {10, 32, -1, 567, 3, 18, 1, -51, 789, 0};

main() {
	int i;

	sort(in, (sizeof in)/(sizeof in[0]));
	for (i = 0; i < (sizeof in)/(sizeof in[0]); i++) {
		putd(in[i]);
		putchar('\n');
	}
	return 0;
}

/* putd - output decimal number */
putd(n) {
	if (n < 0) {
		putchar('-');
		n = -n;
	}
	if (n/10)
		putd(n/10);
	putchar(n%10 + '0');
}

int *xx;

/* sort - sort a[0..n-1] into increasing order */
sort(a, n) int a[]; {
	quick(xx = a, 0, --n);
}

/* quick - quicksort a[lb..ub] */
quick(a, lb, ub) int a[]; {
	int k, partition();

	if (lb >= ub)
		return;
	k = partition(a, lb, ub);
	quick(a, lb, k - 1);
	quick(a, k + 1, ub);
}

/* partition - partition a[i..j] */
int partition(a, i, j) int a[]; {
	int v, k;

	j++;
	k = i;
	v = a[k];
	while (i < j) {
		i++; while (a[i] < v) i++;
		j--; while (a[j] > v) j--;
		if (i < j) exchange(&a[i], &a[j]);
	}
	exchange(&a[k], &a[j]);
	return j;
}

/* exchange - exchange *x and *y */
exchange(x, y) int *x, *y; {
	int t;

	printf("exchange(%d,%d)\n", x - xx, y - xx);
	t = *x; *x = *y; *y = t;
}
