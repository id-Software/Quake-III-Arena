typedef struct point { int x,y; } point;
typedef struct rect { point pt1, pt2; } rect;

point addpoint(point p1, point p2) {	/* add two points */
	p1.x += p2.x;
	p1.y += p2.y;
	return p1;
}

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

rect canonrect(rect r) {		/* canonicalize rectangle coordinates */
	rect temp;

	temp.pt1.x = min(r.pt1.x, r.pt2.x);
	temp.pt1.y = min(r.pt1.y, r.pt2.y);
	temp.pt2.x = max(r.pt1.x, r.pt2.x);
	temp.pt2.y = max(r.pt1.y, r.pt2.y);
	return temp;
}

point makepoint(int x, int y) {		/* make a point from x and y components */
	point p;

	p.x = x;
	p.y = y;
	return p;
}

rect makerect(point p1, point p2) {	/* make a rectangle from two points */
	rect r;

	r.pt1 = p1;
	r.pt2 = p2;
	return canonrect(r);
}

int ptinrect(point p, rect r) {		/* is p in r? */
	return p.x >= r.pt1.x && p.x < r.pt2.x
		&& p.y >= r.pt1.y && p.y < r.pt2.y;
}

struct odd {char a[3]; } y = {'a', 'b', 0};

odd(struct odd y) {
	struct odd x = y;
	printf("%s\n", x.a);
}

main() {
	int i;
	point x, origin = { 0, 0 }, maxpt = { 320, 320 };
	point pts[] = { -1, -1, 1, 1, 20, 300, 500, 400 };
	rect screen = makerect(addpoint(maxpt, makepoint(-10, -10)),
		addpoint(origin, makepoint(10, 10)));

	for (i = 0; i < sizeof pts/sizeof pts[0]; i++) {
		printf("(%d,%d) is ", pts[i].x,
			(x = makepoint(pts[i].x, pts[i].y)).y);
		if (ptinrect(x, screen) == 0)
			printf("not ");
		printf("within [%d,%d; %d,%d]\n", screen.pt1.x, screen.pt1.y,
			screen.pt2.x, screen.pt2.y);
	}
	odd(y);
	exit(0);
}

