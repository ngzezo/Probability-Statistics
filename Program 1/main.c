/*
 * ================================================================
 *  M/M/c/K Bank Queue Simulation — main.c
 *  Simulates an M/M/c/K queueing system via discrete-event
 *  simulation and answers Tasks 1-5 from the assignment.
 * ================================================================
 *
 * TASK 3 — ALGORITHM DESCRIPTION
 *
 * Event Scheduling:
 *   Events (ARRIVAL, DEPARTURE) are kept in a globally shared
 *   sorted array — event_list[] — ordered by ascending simulation
 *   time.  insert_event() locates the correct position with a
 *   linear right-scan and shifts existing entries to make room,
 *   maintaining sorted order at all times.  pop_event() removes
 *   and returns the front (smallest-time) entry and left-shifts
 *   the rest.  At any instant the list holds exactly 1 pending
 *   ARRIVAL and at most c pending DEPARTURES (one per busy server),
 *   so it never exceeds ~c+1 entries — well within MAX_EVENTS=200.
 *
 * Queue Management:
 *   Waiting customers are stored as a FIFO array of arrival
 *   timestamps (queue_arrival[]).  Two monotonically growing
 *   integer indices — q_head (front) and q_tail (back) — are used
 *   with modulo to implement a circular buffer.  q_length tracks
 *   current depth and is compared against K before enqueueing to
 *   enforce the capacity limit.  Customers who find all servers
 *   busy AND q_length == K are dropped (counted in dropped_count).
 *
 * Statistics Collection:
 *   Running accumulators are updated at every event before
 *   dispatching on event type:
 *     - queue_length_area  +=  q_length * (event.time - last_event_time)
 *       so that E[Nq] = queue_length_area / sim_end_time (time average).
 *     - server_busy_time[s] is incremented at each DEPARTURE by the
 *       interval since that server last became busy.
 *     - total_wait_time sums the queue waiting time of every dequeued
 *       customer; total_waiting_customers counts how many queued.
 *     - dropped_count and total_arrived are plain integer counters.
 * ================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/* ── System parameters (hard-coded per assignment) ── */
#define NUM_SERVERS     5       /* c: number of customer service reps      */
#define LAMBDA          20.0   /* arrival rate  (customers / hour)         */
#define MU              5.0    /* service rate per server (customers / hr) */
#define QUEUE_CAPACITY  10     /* K: max customers waiting (not in service)*/
#define NUM_ARRIVALS    50000  /* simulation length (number of arrivals)   */

/* ── Internal sizing limits ── */
#define MAX_EVENTS  200   /* ceiling for the sorted event list array        */
#define MAX_C        50   /* ceiling for per-server arrays inside simulate()*/
#define MAX_QUEUE  1000   /* ceiling for the FIFO queue circular buffer      */

/* ── Event type identifiers ── */
#define EVENT_ARRIVAL   0
#define EVENT_DEPARTURE 1

/* ── Event: one record in the sorted event list ── */
typedef struct {
    double time;      /* simulation time when this event fires  */
    int    type;      /* EVENT_ARRIVAL or EVENT_DEPARTURE        */
    int    server_id; /* for DEPARTURE: index of the server     */
} Event;

/* ── SimResult: all performance metrics returned by simulate() ── */
typedef struct {
    double wait;        /* expected waiting time in queue (hours)     */
    double queue_len;   /* expected number of customers in the queue  */
    double utilization; /* fraction of total server capacity in use   */
    double drop_prob;   /* probability an arriving customer is dropped*/
} SimResult;

/* ── Global event list — reset at the start of every simulate() call ── */
Event event_list[MAX_EVENTS];
int   event_count = 0;   /* number of events currently in the list */

/* ================================================================
 * insert_event — insert event e into event_list[], keeping the
 * array sorted in ascending order of time (one-element insertion).
 * ================================================================ */
void insert_event(Event e)
{
    int i = event_count - 1;

    /* shift events with a later time one slot to the right */
    while (i >= 0 && event_list[i].time > e.time) {
        event_list[i + 1] = event_list[i];
        i--;
    }
    event_list[i + 1] = e; /* place e in the gap just created */
    event_count++;
}

/* ================================================================
 * pop_event — remove and return the earliest event (index 0),
 * then left-shift the remaining events to close the gap.
 * ================================================================ */
Event pop_event(void)
{
    Event e = event_list[0]; /* earliest event is always at index 0 */
    int i;

    /* left-shift all remaining events by one position */
    for (i = 0; i < event_count - 1; i++) {
        event_list[i] = event_list[i + 1];
    }
    event_count--;
    return e;
}

/* ================================================================
 * exponential — generate an Exponential(rate) random variate
 * using the inverse-transform method: X = -ln(1-U) / rate,
 * where U ~ Uniform(0,1).
 * ================================================================ */
double exponential(double rate)
{
    double u = (double)rand() / RAND_MAX; /* uniform sample in [0, 1] */
    return -log(1.0 - u) / rate;         /* inverse CDF formula      */
}

/* ================================================================
 * simulate — run one complete M/M/c/K simulation.
 *   c          : number of servers
 *   lambda     : customer arrival rate
 *   mu         : service completion rate per server
 *   K          : maximum number of customers allowed to wait
 *   n_arrivals : stop after this many arrival events are processed
 * Returns a SimResult struct with all four performance metrics.
 * ================================================================ */
SimResult simulate(int c, double lambda, double mu, int K, int n_arrivals)
{
    /* ── reseed RNG so every simulate() call is reproducible ── */
    srand(42);

    /* ── reset the global event list for this run ── */
    event_count = 0;

    /* ── per-server state ── */
    int    server_free[MAX_C];        /* 1 = idle, 0 = busy            */
    double server_busy_since[MAX_C];  /* time this busy stint began     */
    double server_busy_time[MAX_C];   /* total accumulated busy time    */
    int s;
    for (s = 0; s < c; s++) {
        server_free[s]       = 1;     /* all servers start idle         */
        server_busy_since[s] = 0.0;
        server_busy_time[s]  = 0.0;
    }

    /* ── FIFO queue: circular buffer of arrival timestamps ── */
    double queue_arrival[MAX_QUEUE];  /* arrival times of waiting customers */
    int    q_head   = 0;              /* index of front customer (dequeue)  */
    int    q_tail   = 0;              /* index where next customer goes      */
    int    q_length = 0;              /* current number of waiting customers */

    /* ── statistics accumulators ── */
    double total_wait_time        = 0.0; /* sum of all queue waiting times     */
    int    total_waiting_customers = 0;  /* customers who actually entered queue*/
    int    dropped_count          = 0;   /* customers dropped (queue full)      */
    int    total_arrived          = 0;   /* total arrivals processed            */
    double queue_length_area      = 0.0; /* integral of q_length over time      */
    double last_event_time        = 0.0; /* time of the previous event          */
    double sim_end_time           = 0.0; /* time of the last processed event    */

    /* ── schedule the very first arrival ── */
    Event first;
    first.time      = exponential(lambda);
    first.type      = EVENT_ARRIVAL;
    first.server_id = -1;               /* arrivals have no server_id         */
    insert_event(first);

    /* ================================================================
     * Main event loop — runs until n_arrivals have been processed.
     * ================================================================ */
    while (total_arrived < n_arrivals) {

        Event e = pop_event();          /* fetch the next (earliest) event    */

        /* ── update the time-integral of queue length before dispatching ── */
        double dt = e.time - last_event_time;
        queue_length_area += (double)q_length * dt;
        last_event_time    = e.time;
        sim_end_time       = e.time;

        /* ============================================================
         * ARRIVAL EVENT
         * ============================================================ */
        if (e.type == EVENT_ARRIVAL) {

            total_arrived++;             /* count this arrival                 */

            /* schedule the next customer's arrival immediately */
            Event next_arr;
            next_arr.time      = e.time + exponential(lambda);
            next_arr.type      = EVENT_ARRIVAL;
            next_arr.server_id = -1;
            insert_event(next_arr);

            /* find the first idle server (linear scan) */
            int free_srv = -1;
            for (s = 0; s < c; s++) {
                if (server_free[s]) {
                    free_srv = s;
                    break;
                }
            }

            if (free_srv != -1) {
                /* ── a server is free: begin service immediately ── */
                server_free[free_srv]       = 0;       /* mark busy           */
                server_busy_since[free_srv] = e.time;  /* record start time   */

                /* schedule this customer's departure */
                Event dep;
                dep.time      = e.time + exponential(mu);
                dep.type      = EVENT_DEPARTURE;
                dep.server_id = free_srv;
                insert_event(dep);

            } else if (q_length < K) {
                /* ── all servers busy but queue has room: enqueue ── */
                queue_arrival[q_tail % MAX_QUEUE] = e.time; /* store arrival time */
                q_tail++;
                q_length++;
                total_waiting_customers++;   /* count customers who queued    */

            } else {
                /* ── all servers busy AND queue is full: drop customer ── */
                dropped_count++;
            }

        /* ============================================================
         * DEPARTURE EVENT
         * ============================================================ */
        } else {

            s = e.server_id;   /* which server just finished              */

            /* accumulate how long this server was busy this stint */
            server_busy_time[s] += e.time - server_busy_since[s];
            server_free[s]       = 1;          /* server is now idle       */

            if (q_length > 0) {
                /* ── someone is waiting: dequeue and start their service ── */
                double arr_time = queue_arrival[q_head % MAX_QUEUE];
                q_head++;
                q_length--;

                /* record this customer's waiting time */
                total_wait_time += e.time - arr_time;

                /* assign the waiting customer to the now-free server */
                server_busy_since[s] = e.time;
                server_free[s]       = 0;       /* mark busy again          */

                /* schedule their departure */
                Event dep;
                dep.time      = e.time + exponential(mu);
                dep.type      = EVENT_DEPARTURE;
                dep.server_id = s;
                insert_event(dep);
            }
        }
    } /* end main event loop */

    /* ================================================================
     * Compute and return the four performance metrics.
     * ================================================================ */
    SimResult r;

    /* average wait only over customers who actually queued */
    r.wait = (total_waiting_customers > 0)
             ? (total_wait_time / total_waiting_customers)
             : 0.0;

    /* time-averaged queue length via the running integral */
    r.queue_len = (sim_end_time > 0.0)
                  ? (queue_length_area / sim_end_time)
                  : 0.0;

    /* fraction of total server capacity that was busy */
    double total_busy = 0.0;
    for (s = 0; s < c; s++) {
        total_busy += server_busy_time[s];
    }
    r.utilization = total_busy / ((double)c * sim_end_time);

    /* fraction of arrivals that were dropped */
    r.drop_prob = (total_arrived > 0)
                  ? ((double)dropped_count / total_arrived)
                  : 0.0;

    return r;
}

/* ================================================================
 * print_results — print the Task 2 formatted results block.
 * ================================================================ */
void print_results(SimResult r, int c, double lambda)
{
    printf("=== SIMULATION RESULTS (c=%d, lambda=%.0f, mu=%.0f, K=%d) ===\n",
           c, lambda, MU, QUEUE_CAPACITY);
    printf("Expected Waiting Time in Queue  : %.4f hours\n",    r.wait);
    printf("Expected Number in Queue        : %.4f customers\n", r.queue_len);
    printf("System Utilization              : %.4f (fraction)\n", r.utilization);
    printf("Customer Dropping Probability   : %.4f\n",           r.drop_prob);
    printf("\n");
}

/* ================================================================
 * main — orchestrates Tasks 2/3, 4, and 5.
 * ================================================================ */
int main(void)
{
    /* ── Tasks 2 & 3: run the base simulation and print results ── */
    SimResult base = simulate(NUM_SERVERS, LAMBDA, MU, QUEUE_CAPACITY, NUM_ARRIVALS);
    print_results(base, NUM_SERVERS, LAMBDA);

    /* ================================================================
     * Task 4 — find the minimum number of servers that halves the
     * expected waiting time.  Incremental search starting at c+1.
     * ================================================================ */
    printf("=== TASK 4: Halving Expected Wait Time ===\n");
    double base_wait = base.wait;
    double target    = base_wait / 2.0;  /* the halved threshold */
    printf("Base expected wait time       : %.4f hours\n", base_wait);
    printf("Target (half)                 : %.4f hours\n", target);

    int    min_c      = -1;
    double min_c_wait = 0.0;
    int    c;

    /* try c = NUM_SERVERS+1, NUM_SERVERS+2, ... until target is met */
    for (c = NUM_SERVERS + 1; ; c++) {
        SimResult r = simulate(c, LAMBDA, MU, QUEUE_CAPACITY, NUM_ARRIVALS);
        if (r.wait <= target) {          /* first c that meets the goal    */
            min_c      = c;
            min_c_wait = r.wait;
            break;
        }
    }
    printf("Minimum servers required      : %d  (wait time = %.4f hours)\n\n",
           min_c, min_c_wait);

    /* ================================================================
     * Task 5 — sensitivity analysis: vary lambda, keep c and mu fixed.
     * ================================================================ */
    printf("=== TASK 5: Sensitivity Analysis — Varying Arrival Rate Lambda ===\n");
    printf("Lambda   | E[Wait]   | E[Queue]  | Utilization | Drop Prob\n");
    printf("---------|-----------|-----------|-------------|----------\n");

    double lambdas[] = {10.0, 20.0, 30.0};
    int    n_lambdas = (int)(sizeof(lambdas) / sizeof(lambdas[0]));
    int    i;

    for (i = 0; i < n_lambdas; i++) {
        double lam = lambdas[i];
        SimResult r = simulate(NUM_SERVERS, lam, MU, QUEUE_CAPACITY, NUM_ARRIVALS);

        /* mark the base case (lambda = 20) with an arrow */
        const char *marker = (i == 1) ? "   <- base case" : "";

        printf("  %5.2f  |  %.4f   |  %.4f   |   %.4f    |  %.4f%s\n",
               lam, r.wait, r.queue_len, r.utilization, r.drop_prob, marker);
    }

    /* ── narrative explanation of the sensitivity results ── */
    printf("\n");
    printf("Effect of increasing Lambda on each metric:\n");
    printf("  E[Wait]      : INCREASES — more arrivals per hour intensify\n");
    printf("                 competition for servers; each customer waits longer.\n");
    printf("  E[Queue]     : INCREASES — higher arrival rate fills the waiting\n");
    printf("                 line faster, raising the time-averaged queue length.\n");
    printf("  Utilization  : INCREASES — servers spend a larger fraction of time\n");
    printf("                 busy as offered load (lambda / (c * mu)) grows.\n");
    printf("  Drop Prob    : INCREASES sharply — once utilization is high the\n");
    printf("                 queue fills regularly and a rising share of arriving\n");
    printf("                 customers are turned away because all slots are taken.\n");

    return 0;
}
