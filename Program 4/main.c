/*
 * ================================================================
 *  Joint PMF Analysis — main.c
 *
 *  Analyzes the joint probability mass function P(X=i, Y=j) where
 *    X = number of video streaming packets  (0, 1, or 2)
 *    Y = number of web browsing packets     (0, 1, or 2)
 *  in a single router buffer observation interval.
 *
 *  Answers Questions 1–7:
 *    Q1 — PMF validity check          (sum == 1, all entries >= 0)
 *    Q2 — Marginal PMFs of X and Y
 *    Q3 — P(X + Y >= 3)
 *    Q4 — E[X] and E[Y]
 *    Q5 — Independence check
 *    Q6 — Conditional probability P(Y=2 | X=2)
 *    Q7 — Engineering interpretation
 *
 *  Compile: gcc -o q4 main.c -lm -Wall
 * ================================================================
 */

#include <stdio.h>
#include <math.h>

/* ── Floating-point comparison tolerance ── */
#define TOLERANCE 1e-9

/*
 * Joint PMF P(X=i, Y=j) stored as pmf[i][j].
 * X = video streaming packets (0, 1, 2)
 * Y = web browsing packets    (0, 1, 2)
 *
 *        Y=0    Y=1    Y=2
 *  X=0   0.10   0.05   0.00
 *  X=1   0.15   0.20   0.05
 *  X=2   0.10   0.20   0.15
 */
double pmf[3][3] = {
    {0.10, 0.05, 0.00},
    {0.15, 0.20, 0.05},
    {0.10, 0.20, 0.15}
};

/*
 * Marginal PMFs — computed once in question2() and reused by
 * question4(), question5(), and question6().
 *   pX[i] = P(X = i)   for i = 0, 1, 2
 *   pY[j] = P(Y = j)   for j = 0, 1, 2
 */
double pX[3] = {0.0, 0.0, 0.0};
double pY[3] = {0.0, 0.0, 0.0};


/* ================================================================
 * question1 — PMF Validity Check
 *
 * Verifies that the joint PMF is a valid probability distribution:
 *   (a) Every entry pmf[i][j] >= 0 (non-negativity).
 *   (b) The sum of all 9 entries equals 1 (within TOLERANCE).
 *
 * Prints each entry with its running total, then a final verdict.
 * ================================================================ */
void question1(void)
{
    int i, j;
    int all_nonneg = 1;      /* flag: 1 = all entries >= 0 so far */
    double running = 0.0;

    printf("  Checking all 9 entries (row-major order):\n\n");
    printf("  %-15s  %-12s  %s\n", "Entry", "Value", "Running Total");
    printf("  %s\n", "-----------------------------------------------");

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            running += pmf[i][j];
            if (pmf[i][j] < 0.0)
                all_nonneg = 0;
            printf("  P(X=%d, Y=%d)      %10.6f    %10.6f\n",
                   i, j, pmf[i][j], running);
        }
    }

    printf("\n");
    printf("  Non-negativity of all entries : %s\n",
           all_nonneg ? "YES" : "NO — negative value found");

    int sum_ok = (fabs(running - 1.0) < TOLERANCE);
    printf("  Valid PMF: %s — sum = %.6f\n",
           (all_nonneg && sum_ok) ? "YES" : "NO", running);
}


/* ================================================================
 * question2 — Marginal PMFs of X and Y
 *
 * Computes and stores (in globals pX[], pY[]) the marginal PMFs:
 *   pX[i] = sum over j of pmf[i][j]
 *   pY[j] = sum over i of pmf[i][j]
 *
 * Prints both distributions as formatted tables with row sums.
 *
 * NOTE: This function MUST be called before question4(), question5(),
 *       or question6() because those functions read pX[] and pY[].
 * ================================================================ */
void question2(void)
{
    int i, j;
    double sumX = 0.0, sumY = 0.0;

    /* ── Compute marginals ── */
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            pX[i] += pmf[i][j];
            pY[j] += pmf[i][j];
        }
    }

    /* ── Print marginal PMF of X ── */
    printf("  Marginal PMF of X  (video streaming packets):\n\n");
    for (i = 0; i < 3; i++) {
        sumX += pX[i];
        printf("    P(X=%d) = %.6f\n", i, pX[i]);
    }
    printf("    Sum    = %.6f  (should be 1.0)\n", sumX);

    printf("\n");

    /* ── Print marginal PMF of Y ── */
    printf("  Marginal PMF of Y  (web browsing packets):\n\n");
    for (j = 0; j < 3; j++) {
        sumY += pY[j];
        printf("    P(Y=%d) = %.6f\n", j, pY[j]);
    }
    printf("    Sum    = %.6f  (should be 1.0)\n", sumY);
}


/* ================================================================
 * question3 — P(X + Y >= 3)
 *
 * Sums the probability of all (i, j) pairs where i + j >= 3.
 * With X, Y in {0,1,2} the qualifying pairs are:
 *   (1,2) — i+j = 3
 *   (2,1) — i+j = 3
 *   (2,2) — i+j = 4
 *
 * Prints each contributing term individually, then the total.
 * ================================================================ */
void question3(void)
{
    int i, j;
    double prob = 0.0;

    printf("  Pairs (i,j) with i + j >= 3:\n\n");

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            if (i + j >= 3) {
                prob += pmf[i][j];
                printf("    P(X=%d, Y=%d) = %.6f   [i+j = %d]\n",
                       i, j, pmf[i][j], i + j);
            }
        }
    }

    printf("\n");
    printf("  P(X + Y >= 3) = %.6f\n", prob);
}


/* ================================================================
 * question4 — Expected Values E[X] and E[Y]
 *
 * Uses the marginals computed by question2():
 *   E[X] = sum_{i=0}^{2}  i * pX[i]
 *   E[Y] = sum_{j=0}^{2}  j * pY[j]
 *
 * Prints each term of the weighted sum, then the final result.
 * ================================================================ */
void question4(void)
{
    int i;
    double EX = 0.0, EY = 0.0;

    /* ── E[X] ── */
    printf("  Computing E[X] = sum_i  i * P(X=i):\n\n");
    for (i = 0; i < 3; i++) {
        double term = (double)i * pX[i];
        EX += term;
        printf("    %d * P(X=%d) = %d * %.6f = %.6f\n",
               i, i, i, pX[i], term);
    }
    printf("\n  E[X] = %.6f\n", EX);

    printf("\n");

    /* ── E[Y] ── */
    printf("  Computing E[Y] = sum_j  j * P(Y=j):\n\n");
    for (i = 0; i < 3; i++) {
        double term = (double)i * pY[i];
        EY += term;
        printf("    %d * P(Y=%d) = %d * %.6f = %.6f\n",
               i, i, i, pY[i], term);
    }
    printf("\n  E[Y] = %.6f\n", EY);
}


/* ================================================================
 * question5 — Independence Check
 *
 * X and Y are independent if and only if
 *   P(X=i, Y=j) == P(X=i) * P(Y=j)   for ALL (i,j) pairs.
 *
 * Checks all 9 pairs using TOLERANCE for floating-point comparison.
 * Prints each comparison and concludes INDEPENDENT / NOT INDEPENDENT.
 * ================================================================ */
void question5(void)
{
    int i, j;
    int independent = 1;   /* assume independent until a mismatch is found */

    printf("  Checking P(X=i,Y=j) vs P(X=i)*P(Y=j) for all 9 pairs:\n\n");
    printf("  %-22s  %-10s  %-22s  %s\n",
           "P(X=i,Y=j)", "Value", "P(X=i)*P(Y=j)", "Match");
    printf("  %s\n",
           "--------------------------------------------------------------------");

    for (i = 0; i < 3; i++) {
        for (j = 0; j < 3; j++) {
            double joint   = pmf[i][j];
            double product = pX[i] * pY[j];
            int match      = (fabs(joint - product) < TOLERANCE);
            if (!match)
                independent = 0;

            printf("  P(X=%d,Y=%d) = %.6f     %.6f = P(X=%d)*P(Y=%d)   %s\n",
                   i, j, joint,
                   product, i, j,
                   match ? "YES" : "NO");
        }
    }

    printf("\n  Conclusion: X and Y are %s.\n",
           independent ? "INDEPENDENT" : "NOT INDEPENDENT");
}


/* ================================================================
 * question6 — Conditional Probability P(Y=2 | X=2)
 *
 * By the definition of conditional probability:
 *   P(Y=2 | X=2) = P(X=2, Y=2) / P(X=2)
 *                = pmf[2][2]   / pX[2]
 *
 * Prints the numerator (joint probability), the denominator
 * (marginal of X), and the resulting conditional probability.
 * ================================================================ */
void question6(void)
{
    double numerator   = pmf[2][2];
    double denominator = pX[2];
    double result      = numerator / denominator;

    printf("  Formula:  P(Y=2 | X=2) = P(X=2, Y=2) / P(X=2)\n\n");
    printf("    Numerator   — P(X=2, Y=2) = pmf[2][2] = %.6f\n", numerator);
    printf("    Denominator — P(X=2)      = pX[2]     = %.6f\n", denominator);
    printf("\n");
    printf("  P(Y=2 | X=2) = %.6f / %.6f = %.6f\n",
           numerator, denominator, result);
}


/* ================================================================
 * question7 — Engineering Interpretation
 *
 * Discusses the practical significance of statistical dependence
 * between X and Y in the context of network router buffer design.
 * ================================================================ */
void question7(void)
{
    printf(
        "  The result from Question 5 shows that X (video streaming) and\n"
        "  Y (web browsing) are NOT independent — knowing the video load\n"
        "  changes the probability distribution of the web-browsing load,\n"
        "  and vice versa.  In a router, this means the two traffic flows\n"
        "  are statistically correlated: both tend to peak (or stay low)\n"
        "  at the same time rather than varying freely on their own.\n"
        "\n"
        "  This correlation directly raises the risk of buffer overflow.\n"
        "  If the flows were independent, large values of X and large\n"
        "  values of Y would coincide only rarely (probability =\n"
        "  P(X=2)*P(Y=2) = 0.45*0.20 = 0.09).  Because they are\n"
        "  dependent, the actual joint probability P(X=2,Y=2) = 0.15 is\n"
        "  67%% higher — meaning simultaneous traffic bursts, and therefore\n"
        "  buffer overflow events, occur far more often than an\n"
        "  independence model would predict.\n"
        "\n"
        "  If X and Y were independent, a network engineer could dimension\n"
        "  each buffer independently using only the individual marginal\n"
        "  distributions, greatly simplifying capacity planning.  Because\n"
        "  they are dependent, the engineer must use the full joint PMF\n"
        "  (joint buffer dimensioning): the buffer must be sized to handle\n"
        "  the combined worst-case load implied by the joint distribution,\n"
        "  not merely the product of the individual worst cases.  Ignoring\n"
        "  this dependence leads to systematically under-provisioned\n"
        "  buffers, elevated packet-drop rates, and degraded quality of\n"
        "  service for both video and web traffic.\n"
    );
}


/* ================================================================
 * main — Entry Point
 *
 * Calls each question function in order, preceded by a clear
 * section header.  question2() is called first (Q2) so that the
 * marginals pX[] and pY[] are available to Q4, Q5, and Q6.
 * ================================================================ */
int main(void)
{
    printf("================================================================\n");
    printf("  Joint PMF Analysis — Video Streaming (X) & Web Browsing (Y)\n");
    printf("================================================================\n");

    printf("\n=== Question 1 — PMF Validity Check ===\n\n");
    question1();

    printf("\n=== Question 2 — Marginal PMFs of X and Y ===\n\n");
    question2();

    printf("\n=== Question 3 — P(X + Y >= 3) ===\n\n");
    question3();

    printf("\n=== Question 4 — Expected Values E[X] and E[Y] ===\n\n");
    question4();

    printf("\n=== Question 5 — Independence Check ===\n\n");
    question5();

    printf("\n=== Question 6 — Conditional Probability P(Y=2 | X=2) ===\n\n");
    question6();

    printf("\n=== Question 7 — Engineering Interpretation ===\n\n");
    question7();

    printf("\n================================================================\n");
    return 0;
}
