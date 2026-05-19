/*
 * ================================================================
 *  Constrained Lattice Path Counter — main.c
 *
 *  Counts monotonic lattice paths from S=(0,0) to T=(5,5) that
 *  visit EXACTLY TWO of the special points {A, B, C} and never
 *  pass through the forbidden point F=(2,3).
 *
 *  Grid  : 6x6 lattice, coordinates 0..5 in both dimensions
 *  S     = (0,0)   start
 *  T     = (5,5)   finish
 *  A     = (1,2)   special point
 *  B     = (3,2)   special point
 *  C     = (4,4)   special point
 *  F     = (2,3)   forbidden — path must NOT pass through here
 *
 *  Method: inclusion-exclusion over three mutually exclusive cases.
 *  For each pair, the path is decomposed into ordered segments; each
 *  segment is counted with complementary counting to exclude F.
 *
 *    exactly(A,B) = paths(A and B)   - paths(A and B and C)
 *    exactly(A,C) = paths(A and C)   - paths(A and B and C)
 *    exactly(B,C) = paths(B and C)   - paths(A and B and C)
 *    total        = exactly(A,B) + exactly(A,C) + exactly(B,C)
 *
 *  Compile: gcc -o q3 main.c -Wall -std=c99
 * ================================================================
 */

#include <stdio.h>

/* ── Problem constants (hard-coded per assignment) ── */
#define SX 0   /* Start  x */
#define SY 0   /* Start  y */
#define TX 5   /* Finish x */
#define TY 5   /* Finish y */
#define AX 1   /* Special point A x */
#define AY 2   /* Special point A y */
#define BX 3   /* Special point B x */
#define BY 2   /* Special point B y */
#define CX 4   /* Special point C x */
#define CY 4   /* Special point C y */
#define FX 2   /* Forbidden point F x */
#define FY 3   /* Forbidden point F y */

/* ----------------------------------------------------------------
 * C(n, k) — binomial coefficient "n choose k".
 *
 * Iterative formula (avoids factorial overflow on small inputs):
 *   C(n,k) = (n * (n-1) * ... * (n-k+1)) / (1 * 2 * ... * k)
 *
 * Dividing after each multiplication keeps the running product an
 * integer (C(n,i) is always an integer for 0 <= i <= n).
 * We use min(k, n-k) to minimise the number of loop iterations.
 * ---------------------------------------------------------------- */
long long C(int n, int k)
{
    if (k < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;   /* use smaller half — same value */
    long long result = 1;
    for (int i = 0; i < k; i++) {
        result *= (n - i);
        result /= (i + 1);
    }
    return result;
}

/* ----------------------------------------------------------------
 * paths(x1,y1, x2,y2) — unrestricted monotonic path count.
 *
 * Moving only right (+x) or up (+y), the number of distinct paths
 * from (x1,y1) to (x2,y2) equals the number of ways to arrange
 * dx = x2-x1 right-steps and dy = y2-y1 up-steps in sequence:
 *
 *   paths = C(dx + dy, dx)
 *
 * Returns 0 if (x2,y2) is unreachable (i.e. dx < 0 or dy < 0).
 * ---------------------------------------------------------------- */
long long paths(int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;
    if (dx < 0 || dy < 0) return 0;   /* would need to go left or down */
    return C(dx + dy, dx);
}

/* ----------------------------------------------------------------
 * paths_through_F(x1,y1, x2,y2, fx,fy) — paths that pass through F.
 *
 * Every path through F splits uniquely into two sub-paths at F:
 *   (x1,y1) -> (fx,fy)   then   (fx,fy) -> (x2,y2)
 *
 *   count = paths(x1,y1, fx,fy) * paths(fx,fy, x2,y2)
 *
 * If F lies outside the bounding box of the two endpoints, one
 * factor will be 0 (returned by paths()), giving a product of 0.
 * ---------------------------------------------------------------- */
long long paths_through_F(int x1, int y1, int x2, int y2, int fx, int fy)
{
    return paths(x1, y1, fx, fy) * paths(fx, fy, x2, y2);
}

/* ----------------------------------------------------------------
 * paths_avoid_F(x1,y1, x2,y2, fx,fy) — paths that avoid F.
 *
 * By complementary counting:
 *   paths avoiding F = total paths - paths through F
 * ---------------------------------------------------------------- */
long long paths_avoid_F(int x1, int y1, int x2, int y2, int fx, int fy)
{
    return paths(x1, y1, x2, y2)
         - paths_through_F(x1, y1, x2, y2, fx, fy);
}

/* ----------------------------------------------------------------
 * paths_through_waypoints(wp, n, fx, fy):
 *
 * Given n ordered intermediate waypoints, count paths from S to T
 * that visit every waypoint in the given order, with each segment
 * avoiding the forbidden point (fx,fy).
 *
 * Decomposition:  S -> wp[0] -> wp[1] -> ... -> wp[n-1] -> T
 *
 * Result = product of paths_avoid_F() for each consecutive segment.
 * If any segment is unreachable, paths_avoid_F() returns 0 and the
 * whole product becomes 0, correctly representing impossibility.
 * ---------------------------------------------------------------- */
long long paths_through_waypoints(int wp[][2], int n, int fx, int fy)
{
    long long product = 1;
    int x1 = SX, y1 = SY;
    for (int i = 0; i < n; i++) {
        product *= paths_avoid_F(x1, y1, wp[i][0], wp[i][1], fx, fy);
        x1 = wp[i][0];
        y1 = wp[i][1];
    }
    product *= paths_avoid_F(x1, y1, TX, TY, fx, fy);   /* last -> T */
    return product;
}

/* ================================================================
 * main — compute and print the full step-by-step solution.
 * ================================================================ */
int main(void)
{
    /* ── Pre-compute all segment values used in the three cases ── */

    /* Unrestricted path counts for every segment that appears */
    long long p_SA  = paths(SX,SY, AX,AY);   /* S -> A */
    long long p_SB  = paths(SX,SY, BX,BY);   /* S -> B */
    long long p_SC  = paths(SX,SY, CX,CY);   /* S -> C (helper section) */
    long long p_AB  = paths(AX,AY, BX,BY);   /* A -> B */
    long long p_AC  = paths(AX,AY, CX,CY);   /* A -> C */
    long long p_BC  = paths(BX,BY, CX,CY);   /* B -> C */
    long long p_BT  = paths(BX,BY, TX,TY);   /* B -> T */
    long long p_CT  = paths(CX,CY, TX,TY);   /* C -> T */

    /* Paths-through-F counts (via inclusion-exclusion split at F) */
    long long tf_SA = paths_through_F(SX,SY, AX,AY, FX,FY);
    long long tf_SB = paths_through_F(SX,SY, BX,BY, FX,FY);
    long long tf_SC = paths_through_F(SX,SY, CX,CY, FX,FY);
    long long tf_AB = paths_through_F(AX,AY, BX,BY, FX,FY);
    long long tf_AC = paths_through_F(AX,AY, CX,CY, FX,FY);
    long long tf_BC = paths_through_F(BX,BY, CX,CY, FX,FY);
    long long tf_BT = paths_through_F(BX,BY, TX,TY, FX,FY);
    long long tf_CT = paths_through_F(CX,CY, TX,TY, FX,FY);

    /* Avoid-F counts = unrestricted - through-F */
    long long af_SA = p_SA - tf_SA;
    long long af_SB = p_SB - tf_SB;
    long long af_AB = p_AB - tf_AB;
    long long af_AC = p_AC - tf_AC;
    long long af_BC = p_BC - tf_BC;
    long long af_BT = p_BT - tf_BT;
    long long af_CT = p_CT - tf_CT;

    /*
     * Paths through all three waypoints A, B, C in order S->A->B->C->T.
     * The ordering is unique: A.x=1 < B.x=3 < C.x=4 fixes A before B
     * before C on every monotone path, so no other arrangement is valid.
     */
    int wp_ABC[3][2] = {{AX,AY},{BX,BY},{CX,CY}};
    long long all_ABC = paths_through_waypoints(wp_ABC, 3, FX, FY);

    /* ════════════════════════════════════════════════════════════ */
    printf("======================================================\n");
    printf("  CONSTRAINED LATTICE PATHS: S=(%d,%d) to T=(%d,%d)\n",
           SX,SY,TX,TY);
    printf("  Special points: A=(%d,%d), B=(%d,%d), C=(%d,%d)\n",
           AX,AY,BX,BY,CX,CY);
    printf("  Forbidden point: F=(%d,%d)\n", FX,FY);
    printf("  Constraint: visit EXACTLY 2 of {A, B, C}, avoid F\n");
    printf("======================================================\n\n");

    /* ── HELPER VALUES ─────────────────────────────────────────── */
    printf("--- Helper values (segment path counts) ---\n\n");

    /* S -> A */
    printf("  Segment S->A  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           SX,SY,AX,AY, AX-SX, AY-SY);
    printf("    paths(S->A)             = C(%d,%d)  = %lld\n",
           (AX-SX)+(AY-SY), AX-SX, p_SA);
    printf("    paths(S->F)*paths(F->A) = %lld * %lld  = %lld"
           "  [F.x=%d > A.x=%d => F->A needs leftward move]\n",
           paths(SX,SY,FX,FY), paths(FX,FY,AX,AY), tf_SA, FX,AX);
    printf("    paths_avoid_F(S->A)     = %lld - %lld    = %lld\n\n",
           p_SA, tf_SA, af_SA);

    /* S -> B */
    printf("  Segment S->B  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           SX,SY,BX,BY, BX-SX, BY-SY);
    printf("    paths(S->B)             = C(%d,%d)  = %lld\n",
           (BX-SX)+(BY-SY), BX-SX, p_SB);
    printf("    paths(S->F)*paths(F->B) = %lld * %lld  = %lld"
           "  [F.y=%d > B.y=%d => F->B needs downward move]\n",
           paths(SX,SY,FX,FY), paths(FX,FY,BX,BY), tf_SB, FY,BY);
    printf("    paths_avoid_F(S->B)     = %lld - %lld   = %lld\n\n",
           p_SB, tf_SB, af_SB);

    /* S -> C  (shown for completeness; F lies on this segment) */
    printf("  Segment S->C  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           SX,SY,CX,CY, CX-SX, CY-SY);
    printf("    paths(S->C)             = C(%d,%d)  = %lld\n",
           (CX-SX)+(CY-SY), CX-SX, p_SC);
    printf("    paths(S->F)*paths(F->C) = %lld * %lld = %lld"
           "  [F=(%d,%d) lies inside bounding box S->C]\n",
           paths(SX,SY,FX,FY), paths(FX,FY,CX,CY), tf_SC, FX,FY);
    printf("    paths_avoid_F(S->C)     = %lld - %lld   = %lld\n\n",
           p_SC, tf_SC, p_SC - tf_SC);

    /* A -> B */
    printf("  Segment A->B  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           AX,AY,BX,BY, BX-AX, BY-AY);
    printf("    paths(A->B)             = C(%d,%d)  = %lld\n",
           (BX-AX)+(BY-AY), BX-AX, p_AB);
    printf("    paths(A->F)*paths(F->B) = %lld * %lld  = %lld"
           "  [F.y=%d > B.y=%d => F->B needs downward move]\n",
           paths(AX,AY,FX,FY), paths(FX,FY,BX,BY), tf_AB, FY,BY);
    printf("    paths_avoid_F(A->B)     = %lld - %lld    = %lld\n\n",
           p_AB, tf_AB, af_AB);

    /* A -> C  (F lies on this segment) */
    printf("  Segment A->C  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           AX,AY,CX,CY, CX-AX, CY-AY);
    printf("    paths(A->C)             = C(%d,%d)  = %lld\n",
           (CX-AX)+(CY-AY), CX-AX, p_AC);
    printf("    paths(A->F)*paths(F->C) = %lld * %lld  = %lld"
           "  [F=(%d,%d) lies inside bounding box A->C]\n",
           paths(AX,AY,FX,FY), paths(FX,FY,CX,CY), tf_AC, FX,FY);
    printf("    paths_avoid_F(A->C)     = %lld - %lld    = %lld\n\n",
           p_AC, tf_AC, af_AC);

    /* B -> C */
    printf("  Segment B->C  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           BX,BY,CX,CY, CX-BX, CY-BY);
    printf("    paths(B->C)             = C(%d,%d)  = %lld\n",
           (CX-BX)+(CY-BY), CX-BX, p_BC);
    printf("    paths(B->F)*paths(F->C) = %lld * %lld  = %lld"
           "  [F.x=%d < B.x=%d => B->F needs leftward move]\n",
           paths(BX,BY,FX,FY), paths(FX,FY,CX,CY), tf_BC, FX,BX);
    printf("    paths_avoid_F(B->C)     = %lld - %lld    = %lld\n\n",
           p_BC, tf_BC, af_BC);

    /* B -> T */
    printf("  Segment B->T  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           BX,BY,TX,TY, TX-BX, TY-BY);
    printf("    paths(B->T)             = C(%d,%d)  = %lld\n",
           (TX-BX)+(TY-BY), TX-BX, p_BT);
    printf("    paths(B->F)*paths(F->T) = %lld * %lld  = %lld"
           "  [F.x=%d < B.x=%d => B->F needs leftward move]\n",
           paths(BX,BY,FX,FY), paths(FX,FY,TX,TY), tf_BT, FX,BX);
    printf("    paths_avoid_F(B->T)     = %lld - %lld   = %lld\n\n",
           p_BT, tf_BT, af_BT);

    /* C -> T */
    printf("  Segment C->T  (%d,%d)->(%d,%d)  [%dR + %dU]\n",
           CX,CY,TX,TY, TX-CX, TY-CY);
    printf("    paths(C->T)             = C(%d,%d)  = %lld\n",
           (TX-CX)+(TY-CY), TX-CX, p_CT);
    printf("    paths(C->F)*paths(F->T) = %lld * %lld  = %lld"
           "  [F.x=%d < C.x=%d => C->F needs leftward move]\n",
           paths(CX,CY,FX,FY), paths(FX,FY,TX,TY), tf_CT, FX,CX);
    printf("    paths_avoid_F(C->T)     = %lld - %lld    = %lld\n\n",
           p_CT, tf_CT, af_CT);

    /* ════════════════════════════════════════════════════════════
     * CASE 1: Through A and B (not C)
     *
     * exactly(A,B) = paths(A and B)   -  paths(A and B and C)
     *
     * Monotone ordering: A.x=1 < B.x=3, so A always comes before B.
     * Only one valid ordering of waypoints exists: S->A->B->T.
     * ════════════════════════════════════════════════════════════ */
    printf("--- Case 1: Through A and B (not C) ---\n\n");

    printf("  Order S->A->B->T:\n");
    printf("    A=(%d,%d), B=(%d,%d) — A.x=%d < B.x=%d,"
           " so A always precedes B on any monotone path. Valid.\n",
           AX,AY,BX,BY,AX,BX);
    printf("    seg S->A  avoid F = %lld\n", af_SA);
    printf("    seg A->B  avoid F = %lld\n", af_AB);
    printf("    seg B->T  avoid F = %lld\n", af_BT);
    long long paths_AB = af_SA * af_AB * af_BT;
    printf("    product           = %lld * %lld * %lld = %lld\n\n",
           af_SA, af_AB, af_BT, paths_AB);

    printf("  Order S->B->A->T:\n");
    printf("    B=(%d,%d), A=(%d,%d) — B.x=%d > A.x=%d,"
           " would require moving left. Impossible. Contribution = 0.\n\n",
           BX,BY,AX,AY,BX,AX);

    printf("  paths(through A and B, any order)   = %lld\n", paths_AB);
    printf("  paths(through A and B and C):\n");
    printf("    S->A->B->C->T = %lld * %lld * %lld * %lld = %lld\n",
           af_SA, af_AB, af_BC, af_CT, all_ABC);
    long long exactly_AB = paths_AB - all_ABC;
    printf("  paths(exactly A and B, not C)       = %lld - %lld = %lld\n\n",
           paths_AB, all_ABC, exactly_AB);

    /* ════════════════════════════════════════════════════════════
     * CASE 2: Through A and C (not B)
     *
     * exactly(A,C) = paths(A and C)   -  paths(A and B and C)
     *
     * Monotone ordering: A.x=1 < C.x=4 AND A.y=2 < C.y=4,
     * so A always comes before C. Only ordering: S->A->C->T.
     * Note: some S->A->C->T paths also pass through B=(3,2),
     * which is why we subtract all_ABC to enforce "not B".
     * ════════════════════════════════════════════════════════════ */
    printf("--- Case 2: Through A and C (not B) ---\n\n");

    printf("  Order S->A->C->T:\n");
    printf("    A=(%d,%d), C=(%d,%d) — A.x=%d < C.x=%d and A.y=%d < C.y=%d,"
           " so A precedes C. Valid.\n",
           AX,AY,CX,CY,AX,CX,AY,CY);
    printf("    seg S->A  avoid F = %lld\n", af_SA);
    printf("    seg A->C  avoid F = %lld\n", af_AC);
    printf("    seg C->T  avoid F = %lld\n", af_CT);
    long long paths_AC = af_SA * af_AC * af_CT;
    printf("    product           = %lld * %lld * %lld = %lld\n\n",
           af_SA, af_AC, af_CT, paths_AC);

    printf("  Order S->C->A->T:\n");
    printf("    C=(%d,%d), A=(%d,%d) — C.x=%d > A.x=%d,"
           " would require moving left. Impossible. Contribution = 0.\n\n",
           CX,CY,AX,AY,CX,AX);

    printf("  paths(through A and C, any order)   = %lld\n", paths_AC);
    printf("  paths(through A and B and C):\n");
    printf("    S->A->B->C->T = %lld (same computation as Case 1)\n", all_ABC);
    long long exactly_AC = paths_AC - all_ABC;
    printf("  paths(exactly A and C, not B)       = %lld - %lld = %lld\n\n",
           paths_AC, all_ABC, exactly_AC);

    /* ════════════════════════════════════════════════════════════
     * CASE 3: Through B and C (not A)
     *
     * exactly(B,C) = paths(B and C)   -  paths(A and B and C)
     *
     * Monotone ordering: B.x=3 < C.x=4 AND B.y=2 < C.y=4,
     * so B always comes before C. Only ordering: S->B->C->T.
     * Note: some S->B->C->T paths also pass through A=(1,2),
     * which is why we subtract all_ABC to enforce "not A".
     * ════════════════════════════════════════════════════════════ */
    printf("--- Case 3: Through B and C (not A) ---\n\n");

    printf("  Order S->B->C->T:\n");
    printf("    B=(%d,%d), C=(%d,%d) — B.x=%d < C.x=%d and B.y=%d < C.y=%d,"
           " so B precedes C. Valid.\n",
           BX,BY,CX,CY,BX,CX,BY,CY);
    printf("    seg S->B  avoid F = %lld\n", af_SB);
    printf("    seg B->C  avoid F = %lld\n", af_BC);
    printf("    seg C->T  avoid F = %lld\n", af_CT);
    long long paths_BC = af_SB * af_BC * af_CT;
    printf("    product           = %lld * %lld * %lld = %lld\n\n",
           af_SB, af_BC, af_CT, paths_BC);

    printf("  Order S->C->B->T:\n");
    printf("    C=(%d,%d), B=(%d,%d) — C.y=%d > B.y=%d,"
           " would require moving down. Impossible. Contribution = 0.\n\n",
           CX,CY,BX,BY,CY,BY);

    printf("  paths(through B and C, any order)   = %lld\n", paths_BC);
    printf("  paths(through A and B and C):\n");
    printf("    S->A->B->C->T = %lld (same computation as Case 1)\n", all_ABC);
    long long exactly_BC = paths_BC - all_ABC;
    printf("  paths(exactly B and C, not A)       = %lld - %lld = %lld\n\n",
           paths_BC, all_ABC, exactly_BC);

    /* ── FINAL TOTAL ─────────────────────────────────────────── */
    long long total = exactly_AB + exactly_AC + exactly_BC;
    printf("======================================================\n");
    printf("TOTAL VALID PATHS = %lld + %lld + %lld = %lld\n",
           exactly_AB, exactly_AC, exactly_BC, total);
    printf("======================================================\n");

    return 0;
}
