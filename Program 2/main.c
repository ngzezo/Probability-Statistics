/*
 * ================================================================
 *  Statistical Analysis of Service Time Dataset — main.c
 *  Performs a complete statistical analysis of 50 service time
 *  observations, covering Tasks 1–6 of the assignment.
 * ================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* ── Dataset (hard-coded exactly as given; never modified) ── */
double data[] = {
     5,  7,  8,  6, 10, 12,  9, 11, 13, 15,
    14, 10,  8,  7,  6,  9, 13, 16, 18, 20,
    22, 19, 17, 15, 14, 12, 11,  9,  8,  7,
     6,  5,  4,  6,  7,  9, 10, 12, 14, 16,
    18, 21, 23, 25, 24, 22, 20, 18, 15, 13
};
int n = 50;

/* ================================================================
 * compare_double — qsort comparator for ascending order.
 * Casts the opaque pointers to double* and returns the sign of
 * the difference, avoiding the precision loss of (int)(a - b).
 * ================================================================ */
int compare_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return  1;
    return 0;
}

/* ================================================================
 * mean — return the arithmetic mean of arr[0..n-1].
 * ================================================================ */
double mean(double arr[], int n)
{
    double sum = 0.0;
    int i;
    for (i = 0; i < n; i++) sum += arr[i];
    return sum / n;
}

/* ================================================================
 * median_of — return the median of a PRE-SORTED array of length len.
 * If len is even: average the two central elements.
 * If len is odd:  return the single central element.
 * The caller must sort the array before passing it here.
 * ================================================================ */
double median_of(double sorted[], int len)
{
    if (len % 2 == 0)
        return (sorted[len/2 - 1] + sorted[len/2]) / 2.0;
    else
        return sorted[len/2];
}

/* ================================================================
 * find_mode — print every value that appears most frequently.
 * Handles multi-modal datasets: if several values share the same
 * peak frequency, all of them are printed.
 * Sorts a local copy so the original array is not modified.
 * ================================================================ */
void find_mode(double arr[], int n)
{
    double sorted[50];          /* local sorted copy                */
    int i, max_freq, cur_freq, first;

    /* copy and sort */
    for (i = 0; i < n; i++) sorted[i] = arr[i];
    qsort(sorted, n, sizeof(double), compare_double);

    /* --- pass 1: find the maximum run length (peak frequency) --- */
    max_freq = 1;
    cur_freq = 1;
    for (i = 1; i < n; i++) {
        if (sorted[i] == sorted[i - 1]) {
            cur_freq++;                          /* extend current run */
            if (cur_freq > max_freq) max_freq = cur_freq;
        } else {
            cur_freq = 1;                        /* start a new run    */
        }
    }

    /* --- pass 2: print every value whose run equals max_freq --- */
    printf("Mode(s)  : ");
    first    = 1;               /* controls comma separation          */
    cur_freq = 1;
    for (i = 1; i < n; i++) {
        if (sorted[i] == sorted[i - 1]) {
            cur_freq++;
        } else {
            if (cur_freq == max_freq) {          /* this value qualifies */
                if (!first) printf(", ");
                printf("%.0f", sorted[i - 1]);
                first = 0;
            }
            cur_freq = 1;
        }
    }
    /* handle the last run (the loop ends before printing it) */
    if (cur_freq == max_freq) {
        if (!first) printf(", ");
        printf("%.0f", sorted[n - 1]);
    }
    printf(" (each appears %d times)\n", max_freq);
}

/* ================================================================
 * variance — compute the sample variance of arr[0..n-1].
 * Uses (n-1) in the denominator (Bessel's correction for samples).
 * ================================================================ */
double variance(double arr[], int n, double mean_val)
{
    double sum_sq = 0.0;
    int i;
    for (i = 0; i < n; i++) {
        double diff = arr[i] - mean_val;   /* deviation from mean */
        sum_sq += diff * diff;
    }
    return sum_sq / (n - 1);              /* divide by n-1 for sample variance */
}

/* ================================================================
 * phi — standard normal CDF Phi(z) via the Abramowitz & Stegun
 * polynomial approximation (accuracy ~1e-7).
 * Handles negative z by symmetry: Phi(-z) = 1 - Phi(z).
 * ================================================================ */
double phi(double z)
{
    double t, poly, result;
    int negative = (z < 0);               /* remember sign before folding */
    if (negative) z = -z;                 /* work with |z| */
    t    = 1.0 / (1.0 + 0.2316419 * z);
    poly = t * (0.319381530
              + t * (-0.356563782
              + t * (1.781477937
              + t * (-1.821255978
              + t *  1.330274429))));
    result = 1.0 - (1.0 / sqrt(2.0 * 3.14159265358979)) * exp(-0.5 * z * z) * poly;
    return negative ? 1.0 - result : result;  /* apply symmetry if z was negative */
}

/* ================================================================
 * task1 — Measures of Central Tendency.
 * Computes and interprets the mean, median, and mode.
 * ================================================================ */
void task1(double arr[], int n)
{
    double sorted[50];     /* sorted copy for median */
    double mn, med;
    int i;

    printf("\n====== TASK 1: Measures of Central Tendency ======\n");

    /* sort a local copy — original arr is untouched */
    for (i = 0; i < n; i++) sorted[i] = arr[i];
    qsort(sorted, n, sizeof(double), compare_double);

    mn  = mean(arr, n);
    med = median_of(sorted, n);        /* n=50 is even: average sorted[24] & sorted[25] */

    printf("Mean     : %.2f minutes\n", mn);
    printf("Median   : %.2f minutes\n", med);
    find_mode(arr, n);

    printf("\nInterpretation:\n");
    printf("  The mean service time is %.2f minutes, suggesting the typical\n", mn);
    printf("  customer waits approximately %.2f minutes.\n", mn);
    printf("  The median of %.2f minutes indicates that half of all customers\n", med);
    printf("  are served within that time, regardless of extreme values.\n");
}

/* ================================================================
 * task2 — Measures of Dispersion.
 * Computes range, sample variance, standard deviation, and CV.
 * ================================================================ */
void task2(double arr[], int n)
{
    double sorted[50];
    double mn, var, sd, cv, range;
    int i;

    printf("\n====== TASK 2: Measures of Dispersion ======\n");

    /* sort a copy to find min (sorted[0]) and max (sorted[n-1]) */
    for (i = 0; i < n; i++) sorted[i] = arr[i];
    qsort(sorted, n, sizeof(double), compare_double);

    mn    = mean(arr, n);
    range = sorted[n - 1] - sorted[0];   /* max minus min                    */
    var   = variance(arr, n, mn);
    sd    = sqrt(var);
    cv    = (sd / mn) * 100.0;           /* coefficient of variation in %    */

    printf("Min / Max               : %.0f / %.0f\n", sorted[0], sorted[n - 1]);
    printf("Range                   : %.2f\n", range);
    printf("Sample Variance         : %.4f\n", var);
    printf("Standard Deviation      : %.4f minutes\n", sd);
    printf("Coefficient of Variation: %.2f%%\n", cv);

    printf("\nInterpretation:\n");
    if (cv > 30.0) {
        printf("  CV = %.2f%% > 30%% — HIGH VARIABILITY.\n", cv);
        printf("  Service times are widely spread; demand is unpredictable\n");
        printf("  and staffing plans must account for significant fluctuations.\n");
    } else {
        printf("  CV = %.2f%% <= 30%% — MODERATE/LOW VARIABILITY.\n", cv);
        printf("  Service times are relatively consistent and predictable.\n");
    }
}

/* ================================================================
 * task3 — Frequency Distribution and Shape Assessment.
 * Counts values in five fixed bins and assesses distribution shape
 * by comparing mean vs. median (skewness proxy).
 * ================================================================ */
void task3(double arr[], int n)
{
    /* five bins: [lo, hi] inclusive */
    int lo[] = { 0,  6, 11, 16, 21};
    int hi[] = { 5, 10, 15, 20, 25};
    int bins[5];             /* frequency count per bin             */
    double sorted[50];
    double mn, med;
    char   stars[60];        /* buffer for the '*' histogram bar    */
    int i, b, len;

    printf("\n====== TASK 3: Frequency Distribution and Shape ======\n");

    /* initialise bin counts to zero */
    for (b = 0; b < 5; b++) bins[b] = 0;

    /* classify each observation into its bin */
    for (i = 0; i < n; i++) {
        for (b = 0; b < 5; b++) {
            if ((int)arr[i] >= lo[b] && (int)arr[i] <= hi[b]) {
                bins[b]++;
                break;           /* each value belongs to exactly one bin */
            }
        }
    }

    /* print text histogram */
    for (b = 0; b < 5; b++) {
        /* build the star bar (one '*' per customer) */
        len = (bins[b] < 59) ? bins[b] : 59;   /* guard against buffer overflow */
        for (i = 0; i < len; i++) stars[i] = '*';
        stars[len] = '\0';                       /* null-terminate the string    */

        printf("[%2d-%2d]: %3d  | %s\n", lo[b], hi[b], bins[b], stars);
    }

    /* ----- skewness: compare mean vs. median ----- */
    for (i = 0; i < n; i++) sorted[i] = arr[i];
    qsort(sorted, n, sizeof(double), compare_double);

    mn  = mean(arr, n);
    med = median_of(sorted, n);

    printf("\nMean = %.2f,  Median = %.2f\n", mn, med);
    printf("Shape Assessment:\n");

    if (mn > med + 0.5) {
        printf("  Mean (%.2f) > Median (%.2f): distribution is RIGHT-SKEWED.\n",
               mn, med);
        printf("  Most customers experience shorter service times, but a minority\n");
        printf("  face very long waits that pull the mean upward. Peak service load\n");
        printf("  is concentrated at lower values, with a long right tail.\n");
    } else if (mn < med - 0.5) {
        printf("  Mean (%.2f) < Median (%.2f): distribution is LEFT-SKEWED.\n",
               mn, med);
        printf("  Occasional very fast service times pull the mean below the median.\n");
        printf("  Peak load appears at higher service times.\n");
    } else {
        printf("  Mean (%.2f) ≈ Median (%.2f): distribution is approximately SYMMETRIC.\n",
               mn, med);
        printf("  Service times are evenly distributed around the centre.\n");
        printf("  Staffing can be planned around the average with confidence.\n");
    }
}

/* ================================================================
 * task4 — Quantile Analysis and Outlier Detection.
 * Computes Q1, Q3, IQR, Tukey fences, and flags any outliers.
 * Method: Q1 = median of the lower 25 values (sorted[0..24]);
 *         Q3 = median of the upper 25 values (sorted[25..49]).
 * ================================================================ */
void task4(double arr[], int n)
{
    double sorted[50];
    double q1, q3, iqr, lower_fence, upper_fence;
    int i, outlier_count;

    printf("\n====== TASK 4: Quantile Analysis ======\n");

    /* sort a full copy */
    for (i = 0; i < n; i++) sorted[i] = arr[i];
    qsort(sorted, n, sizeof(double), compare_double);

    /* Q1: median of the first n/2 = 25 elements (indices 0–24)
     * For 25 elements (odd), the median is element at index 12. */
    q1 = median_of(sorted, n / 2);

    /* Q3: median of the last n/2 = 25 elements (indices 25–49)
     * Pass a pointer to sorted[25] so median_of sees 25 elements. */
    q3 = median_of(sorted + n / 2, n / 2);

    iqr         = q3 - q1;
    lower_fence = q1 - 1.5 * iqr;    /* Tukey lower fence */
    upper_fence = q3 + 1.5 * iqr;    /* Tukey upper fence */

    printf("Q1 (25th percentile)       : %.2f\n", q1);
    printf("Q3 (75th percentile)       : %.2f\n", q3);
    printf("IQR (Q3 - Q1)              : %.2f\n", iqr);
    printf("Lower fence (Q1 - 1.5*IQR): %.2f\n", lower_fence);
    printf("Upper fence (Q3 + 1.5*IQR): %.2f\n", upper_fence);

    /* scan the original data for values outside the fences */
    printf("\nOutliers (values outside fences):\n");
    outlier_count = 0;
    for (i = 0; i < n; i++) {
        if (arr[i] < lower_fence || arr[i] > upper_fence) {
            printf("  %.0f\n", arr[i]);
            outlier_count++;
        }
    }
    if (outlier_count == 0) {
        printf("  None detected — all values lie within the Tukey fences.\n");
    }

    printf("\nOperational Interpretation:\n");
    printf("  Values below %.2f would indicate unusually fast service\n",
           lower_fence);
    printf("  (possible data errors or exceptional efficiency).\n");
    printf("  Values above %.2f represent extreme delays — customers who\n",
           upper_fence);
    printf("  faced severe congestion or unusually complex requests.\n");
    printf("  Such cases should be investigated to prevent recurring bottlenecks.\n");
}

/* ================================================================
 * task5 — Probability Estimation using the Normal Approximation.
 * Standardises threshold values with the sample mean and std dev,
 * then evaluates probabilities via the phi() CDF approximation.
 * ================================================================ */
void task5(double arr[], int n)
{
    double mn, var, sd;
    double z1, z2, prob;

    printf("\n====== TASK 5: Probability Estimation (Normal Approximation) ======\n");

    mn  = mean(arr, n);
    var = variance(arr, n, mn);
    sd  = sqrt(var);

    printf("Parameters: mean = %.4f,  std dev = %.4f\n\n", mn, sd);

    /* ----- P(X > 18) ----- */
    z1   = (18.0 - mn) / sd;                /* standardise X = 18            */
    prob = 1.0 - phi(z1);                   /* P(X > 18) = 1 - Phi(z)        */
    printf("P(X > 18):\n");
    printf("  z = (18 - %.4f) / %.4f = %.4f\n", mn, sd, z1);
    printf("  P(X > 18) = 1 - phi(%.4f) = %.4f\n\n", z1, prob);

    /* ----- P(8 <= X <= 15) ----- */
    z1   = (8.0  - mn) / sd;               /* standardise lower bound X = 8  */
    z2   = (15.0 - mn) / sd;               /* standardise upper bound X = 15 */
    prob = phi(z2) - phi(z1);              /* area between the two z-scores  */
    printf("P(8 <= X <= 15):\n");
    printf("  z1 = (8  - %.4f) / %.4f = %.4f\n", mn, sd, z1);
    printf("  z2 = (15 - %.4f) / %.4f = %.4f\n", mn, sd, z2);
    printf("  P(8 <= X <= 15) = phi(%.4f) - phi(%.4f) = %.4f\n", z2, z1, prob);
}

/* ================================================================
 * task6 — Operational Decision Making.
 * Checks what fraction of customers violate the < 12-minute SLA,
 * then issues a staffing recommendation based on that proportion.
 * ================================================================ */
void task6(double arr[], int n)
{
    int i, count = 0;
    double proportion;

    printf("\n====== TASK 6: Operational Decision Making ======\n");

    /* count observations that meet or exceed the 12-minute threshold */
    for (i = 0; i < n; i++) {
        if (arr[i] >= 12.0) count++;
    }
    proportion = (double)count / n;

    printf("%d out of %d customers (%.1f%%) violate the 12-minute standard.\n",
           count, n, proportion * 100.0);

    printf("\nRecommendation:\n");
    if (proportion > 0.30) {
        printf("  %.1f%% > 30%% — INCREASE STAFFING.\n", proportion * 100.0);
        printf("  More than a third of customers wait beyond the 12-minute\n");
        printf("  target. Additional service representatives are required to\n");
        printf("  bring waiting times within the agreed standard.\n");
    } else if (proportion > 0.10) {
        printf("  %.1f%% > 10%% — MONITOR CLOSELY.\n", proportion * 100.0);
        printf("  A notable minority of customers exceed the 12-minute target.\n");
        printf("  Track trends carefully and prepare contingency staffing plans.\n");
    } else {
        printf("  %.1f%% <= 10%% — STAFFING APPEARS ADEQUATE.\n",
               proportion * 100.0);
        printf("  Fewer than 1 in 10 customers exceed the standard.\n");
        printf("  Continue monitoring to ensure performance is maintained.\n");
    }
}

/* ================================================================
 * main — run all six tasks in order, each with a clear header.
 * ================================================================ */
int main(void)
{
    printf("================================================\n");
    printf("  Statistical Analysis — Service Time Dataset\n");
    printf("  n = %d observations\n", n);
    printf("================================================\n");

    task1(data, n);
    task2(data, n);
    task3(data, n);
    task4(data, n);
    task5(data, n);
    task6(data, n);

    printf("\n================================================\n");
    printf("  Analysis complete.\n");
    printf("================================================\n");

    return 0;
}
