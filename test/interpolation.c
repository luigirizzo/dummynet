#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* gcc interpolation.c -o interpolation */

void    
err(int eval, const char *fmt, ...) 
{
}           
void    
errx(int eval, const char *fmt, ...) 
{
}           
        

#define ED_MAX_SAMPLES_NO 1000
#define ED_MAX_LINE_LEN 128
#define EX_DATAERR 1
#define EX_UNAVAILABLE  3
#define ED_TOK_DELAY    "delay"
#define ED_TOK_PROB     "prob"
#define ED_SEPARATORS   " \t\n"
#define ED_TOK_PROFILE_NO "profile_no"


struct point {
	double prob;		/* y */
	double delay;		/* x */
};

struct profile {
        char    filename[128];                   /* profile filename */
        int     samples[ED_MAX_SAMPLES_NO+1];    /* may be shorter */
        int     samples_no;                     /* actual len of samples[] */
};

/*
 * returns 1 if s is a non-negative number, with at least one '.'
 */
static int
is_valid_number(const char *s)
{
#if 0
        int i, dots_found = 0;
        int len = strlen(s);

        for (i = 0; i<len; ++i)
                if (!isdigit(s[i]) && (s[i] !='.' || ++dots_found > 1))
                        return 0;
#endif
        return 1;
}

static int
compare_points(const void *vp1, const void *vp2)
{
	const struct point *p1 = vp1;
	const struct point *p2 = vp2;
	double res = 0;

	res = p1->prob - p2->prob;
	if (res == 0)
		res = p1->delay - p2->delay;
	if (res < 0)
		return -1;
	else if (res > 0)
		return 1;
	else
		return 0;
}

#define ED_EFMT(s) 1,"error in %s at line %d: "#s,filename,lineno

/*
 * The points defined by the user are stored in the ponts structure.
 * The number of user defined points is stored in points_no.
 *       We assume that The last point for the '1' value of the
 *       probability should be defined. (XXX add checks for this)
 * The user defined sampling value is stored in samples_no.
 * The resulting samples are in the "samples" pointer.
 */
static void
interpolate_samples(struct point *p, int points_no, 
		int *samples, int samples_no, const char *filename)
{
	double dy;		/* delta on the y axis */
	double y;		/* current value of y */
	double x;		/* current value of x */
	double m;		/* the y slope */
	int i;			/* samples index */
	int curr;		/* points current index */

	dy = 1.0/samples_no;
	y = 0;

	for (i=0, curr = 0; i < samples_no; i++, y+=dy) {
		/* This statment move the curr pointer to the next point
		 * skipping the points with the same x value. We are
		 * guaranteed to exit from the loop because the
		 * last possible value of y is stricly less than 1
		 * and the last possible value of the y points is 1 */
		while ( y >= p[curr+1].prob ) curr++;

		/* compute the slope of the curve */
		m = (p[curr+1].delay - p[curr].delay) / (p[curr+1].prob - p[curr].prob);
		/* compute the x value starting from the current point */
		x = p[curr].delay + (y - p[curr].prob) * m;
		samples[i] = x;
	}

	/* add the last sample */
	samples[i] = p[curr+1].delay;
}

#if 0
static void
interpolate_samples_old(struct point *points, int points_no, 
		int *samples, int samples_no, const char *filename)
{
	int i;		/* pointer to the sampled array */
	int j = 0;	/* pointer to user defined samples */
	double dy;	/* delta y */
	double y;	/* current value of y */
	int x;		/* computed value of x */
	double m;	/* slope of the line */
	double y1, x1, y2, x2;	/* two points of the current line */

	/* make sure that there are enough points. */
	/* XXX Duplicated shoule be removed */
	if (points_no < 3)
	    errx(EX_DATAERR, "%s too few samples, need at least %d",
		filename, 3);

	qsort(points, points_no, sizeof(struct point), compare_points);

	samples_no--;
	dy = 1.0/samples_no;
	printf("\nsamples no is %d dy is %f ", samples_no, dy);

	/* start with the first two points */
	y1 = points[j].prob * samples_no;
	x1 = points[j].delay;
	j++;
	y2 = points[j].prob * samples_no;
	x2 = points[j].delay;

	m = (y2-y1)/(x2-x1);
	printf("\nStart");
	printf("\n\tCurrent points x1 y1 %f %f next point x2y2 %f %f m %f\n",
		 x1, y1, x2, y2, m);

	y = 0;
	x = x1;

	for(i=0; i < samples_no+1; i++, y+=dy) {
		printf("\ni:%d j:%d y:%f real y:%f", i, j, y, y*samples_no);
		if ( (y*samples_no) >= y2 ) { /* move to the next point */
			j++;
			if ( j >= points_no ) {
				printf("\n\tNo more points, exit with j: %d i: %d and y:%f %f\n",
					 j, i, y, (y*samples_no));
				break;	/* no more user defined points */
			}
			/* load a new point */
			y1 = y2;
			x1 = x2;
			y2 = points[j].prob * samples_no;
			x2 = points[j].delay;
			m = (y2-y1)/(x2-x1);
			if (x1==x2) { /* m = infinito */
				m = -1;
				x = x2;
			}
			/* very small m problem */
			printf ("\ndelta %f\n", (y1 - y2));
			if (abs(y1 - y2) < 0.00001) { /* m = 0 XXX Should this magic number depend on samples_no ? */
				m = 0;
				x = x2;
			}
			printf("\n\tCurrent points x1 y1 %f %f next point x2y2 %f %f (%f/%f)=m \n",
				 x1, y1, x2, y2, (y2-y1), (x2-x1), m);
		}
		printf("\n\tcompute step y %f x[%d]=%d ",
			y, i, x);
		if ((m != -1) && ( m != 0 )) {
			x = x + (dy * samples_no)/m;
		}
		samples[i] = x;
		printf(" dy %f x new %d\n", dy*samples_no, x);
		printf(" m %f (dy * samples_no)/m %f \n", m, (dy * samples_no)/m);
	}

	x = samples[i-1];
	printf("Finish i is %d samples_no is %d\n", i, samples_no);
	/* The last point has a probability less than 1 */
	for (; i <= samples_no; i++)
		samples[i] = x;
}
#endif

static void
load_profile(struct profile *p)
{
	FILE    *f;			/* file handler */
	char    line[ED_MAX_LINE_LEN];
	int     lineno = 0;
	int     do_points = 0;
	int     delay_first = -1;
	int i;

	struct	point points[1000]; /* MAX_POINTS_NO */
	int     points_no = 0;

	char *filename = p->filename;
	f = fopen(filename, "r");
	if (f == NULL) {
	    err(EX_UNAVAILABLE, "fopen: %s", filename);
	}


	while (fgets(line, ED_MAX_LINE_LEN, f)) {         /* read commands */
		char *s, *cur = line, *name = NULL, *arg = NULL;

		++lineno;

		/* parse the line */
		while (cur) {
			s = strsep(&cur, ED_SEPARATORS);
			if (s == NULL || *s == '#')
				break;
			if (*s == '\0')
				continue;
			if (arg)
				errx(ED_EFMT("too many arguments"));
			if (name == NULL)
				name = s;
			else
				arg = s;
		}

		if (name == NULL)
			continue;

		if (!strcasecmp(name, ED_TOK_DELAY)) {
		    if (do_points)
			errx(ED_EFMT("duplicated token: %s"), name);
		    delay_first = 1;
		    do_points = 1;
		    continue;
		} else if (!strcasecmp(name, ED_TOK_PROB)) {
		    if (do_points)
			errx(ED_EFMT("duplicated token: %s"), name);
		    delay_first = 0;
		    do_points = 1;
		    continue;
		}
		if (!strcasecmp(name, ED_TOK_PROFILE_NO)) {
			int p_no = atof(arg);
			if (p_no <= 0) {
				p_no = 100;
				printf("invalid interpolation samples, using %d\n",
					 p_no);
			}
			if (p_no > ED_MAX_SAMPLES_NO) {
				p_no = ED_MAX_SAMPLES_NO;
				printf("invalid interpolation samples, using %d\n",
					 p_no);
			}

			p->samples_no = p_no;
		    continue;

		} else if (do_points) {
		    if (!is_valid_number(name) || !is_valid_number(arg))
			errx(ED_EFMT("invalid point found"));
		    if (delay_first) {
			points[points_no].delay = atof(name);
			points[points_no].prob = atof(arg);
		    } else {
			points[points_no].delay = atof(arg);
			points[points_no].prob = atof(name);
		    }
		    if (points[points_no].prob > 1.0)
			errx(ED_EFMT("probability greater than 1.0"));
		    ++points_no;
	/* XXX no more that 1000 */
		    continue;
		} else {
		    errx(ED_EFMT("unrecognised command '%s'"), name);
		}
	}

	for(i=0; i < p->samples_no; i++) {
		p->samples[i] = 666;
	}

	/* This code assume the user define a value of X for the sampling value,
	 * and that:
	 * - the value stored in the emulator structure is X;
	 * - the allocated structure for the samples is X+1;
	 */
	interpolate_samples(points, points_no, p->samples, p->samples_no, filename);

	// User defined samples
	printf("\nLoaded %d points:\n", points_no);
	for(i=0; i < points_no; i++) {
		printf("%f %f\n", points[i].prob, points[i].delay);
	}
	printf("\n");
	printf("The sample value is %d \n", p->samples_no);

}

int main(int argc, char **argv)
{
	if (argc < 2) {
		printf("Usage: ./interpolation <filename>\n");
		return -1;
	}

	char *filename;
	filename = argv[1];

	struct profile p;
	int i;

	strncpy(p.filename, filename, 128);
	load_profile(&p);
	printf("-----------\n");
	for (i=0; i<=p.samples_no; i++)
		printf("%d %d\n", i, p.samples[i]);
	printf("-----------\n");
	return 0;
}
