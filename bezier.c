
/* dep: essentials.c memory.c */

struct rbspline2d_elem {
    float knot;
    float weight;
    float point[2];
};

struct rbspline2d {
    int degree;
    int knot_count;
    struct rbspline2d_elem *_Stack elems;
};

struct bspline2d_elem {
    float knot;
    float point[2];
};

struct bspline2d {
    int degree;
    int knot_count;
    struct rbspline2d_elem *_Stack elems;
};

struct nurbs_net {
    float wpoint[4];
};

struct nurbs_knot {
    float u;
    float v;
};

struct nurbs {
    int width;
    int height;
    struct nurbs_net *_Stack net;
    struct nurbs_knot *_Stack knots;
};

struct rbezier3d {
    float wpoint[4];
};

struct nurbs_oneknot {
    union   {
        float u;
        float v;
        float value;
    };
    float _;
};

#define NURBS_Degree (3) /* bicubic */
#define NURBS_Knots_Dim(dim) ((dim) + 2 * NURBS_Degree - 1)
#define NURBS_Knots_Total(udim, vdim) Max (NURBS_Knots_Dim (udim), NURBS_Knots_Dim (vdim))
#define NURBS_Net_Dim(dim) ((dim) + NURBS_Degree)
#define NURBS_Net_Total(udim, vdim) (NURBS_Net_Dim (udim) * NURBS_Net_Dim (vdim))
#define NURBS_CP_From_KnotI(knotI) (knotI - (NURBS_Degree - 1))
#define NURBS_Knot_Delta (0.0001f)
#define NURBS_Local_Knots_Dim NURBS_Knots_Dim (3)
#define NURBS_Local_Knots_Total NURBS_Local_Knots_Dim
#define NURBS_Local_Net_Dim (NURBS_Net_Dim (3) + 1)
#define NURBS_Local_Net_Total (NURBS_Local_Net_Dim * NURBS_Local_Net_Dim)

static void deboor_algorithm (const int degree, int column, int index, float u, const float interval [], const float controls [] [2], _Out_writes_ (2) float *out) {
    if (column > 0) {
        float left [2], right [2];
        float alpha;

        deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
        deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
        alpha = (u - interval [index - degree + column]) / (interval [index + 1] - interval [index - degree + column]);
        Assert (alpha >= 0 && alpha <= 1);
        out[0] = (1 - alpha) * left[0] + alpha * right[0];
        out[1] = (1 - alpha) * left[1] + alpha * right[1];
    } else {
        out[0] = controls[index][0];
        out[1] = controls[index][1];
    }
}

static void evaluate_rbspline2d_degree2 (struct rbspline2d *spline, float u, _Out_writes_ (2) float *out) {
    float left [2], right [2];
    float weights[2], weight;
    float alpha, palpha[2];
    int knotI;
    struct rbspline2d_elem *pleft, *pmid, *pright;

    Assert (spline->degree == 2);
    Assert (get_stack_size (spline->elems) / sizeof *spline->elems > 2);
    knotI = 1;
    while (u >= spline->elems [knotI].knot) {
        knotI += 1;
    }
    knotI -= 1;
    Assert (u <= spline->elems[spline->knot_count - 2].knot);
    pleft = spline->elems + (knotI - 1);
    pmid = spline->elems + knotI;
    pright = spline->elems + (knotI + 1);
    alpha = (u - pleft->knot) / (pright->knot - pleft->knot);
    Assert (alpha >= 0 && alpha <= 1);
    palpha[0] = (1 - alpha) * pleft->weight;
    palpha[1] = alpha * pmid->weight;
    weights[0] = palpha[0] + palpha[1];
    left[0] = (palpha[0] * pleft->point[0] + palpha[1] * pmid->point[0]) / weights[0];
    left[1] = (palpha[0] * pleft->point[1] + palpha[1] * pmid->point[1]) / weights[0];
    alpha = (u - pmid->knot) / (spline->elems [knotI + 2].knot - pmid->knot);
    Assert (alpha >= 0 && alpha <= 1);
    palpha[0] = (1 - alpha) * pmid->weight;
    palpha[1] = alpha * pright->weight;
    weights[1] = palpha[0] + palpha[1];
    right[0] = (palpha[0] * pmid->point[0] + palpha[1] * pright->point[0]) / weights[1];
    right[1] = (palpha[0] * pmid->point[1] + palpha[1] * pright->point[1]) / weights[1];
    alpha = (u - pmid->knot) / (pright->knot - pmid->knot);
    Assert (alpha >= 0 && alpha <= 1);
    palpha[0] = (1 - alpha) * weights[0];
    palpha[1] = alpha * weights[1];
    weight = palpha[0] + palpha[1];
    out[0] = (palpha[0] * left[0] + palpha[1] * right[0]) / weight;
    out[1] = (palpha[0] * left[1] + palpha[1] * right[1]) / weight;
}

static void evaluate_bspline2d_degree2 (struct rbspline2d *spline, float u, _Out_writes_ (2) float *out) {
    float left [2], right [2];
    float alpha;
    int knotI;

    Assert (spline->degree == 2);
    Assert (get_stack_size (spline->elems) / sizeof *spline->elems > 2);
    knotI = 1;
    while (u >= spline->elems [knotI].knot) {
        knotI += 1;
    }
    knotI -= 1;
    Assert (u <= spline->elems [spline->knot_count - 2].knot);
    alpha = (u - spline->elems [knotI - 1].knot) / (spline->elems [knotI + 1].knot - spline->elems [knotI - 1].knot);
    Assert (alpha >= 0 && alpha <= 1);
    left[0] = (1 - alpha) * spline->elems [knotI - 1].point[0] + alpha * spline->elems [knotI].point[0];
    left[1] = (1 - alpha) * spline->elems [knotI - 1].point[1] + alpha * spline->elems [knotI].point[1];
    alpha = (u - spline->elems [knotI].knot) / (spline->elems [knotI + 2].knot - spline->elems [knotI].knot);
    Assert (alpha >= 0 && alpha <= 1);
    right[0] = (1 - alpha) * spline->elems [knotI].point[0] + alpha * spline->elems [knotI + 1].point[0];
    right[1] = (1 - alpha) * spline->elems [knotI].point[1] + alpha * spline->elems [knotI + 1].point[1];
    alpha = (u - spline->elems [knotI].knot) / (spline->elems [knotI + 1].knot - spline->elems [knotI].knot);
    Assert (alpha >= 0 && alpha <= 1);
    out[0] = (1 - alpha) * left[0] + alpha * right[0];
    out[1] = (1 - alpha) * left[1] + alpha * right[1];
}

static void evaluate_bspline2d_degree3 (struct rbspline2d *spline, float u, _Out_writes_ (2) float *out) {
    float left [3] [2], right [3] [2];
    float alpha;
    int knotI;
    int ctrlI;
    int column;
    int index;
    const int degree = 3;

    /* unwrapped deboor algorithm for degree 3. only for reference */
    Assert (spline->degree == 3);
    Assert (get_stack_size (spline->elems) / sizeof *spline->elems > 3);
    knotI = (spline->degree - 1);
    while (u >= spline->elems [knotI].knot) {
        knotI += 1;
    }
    knotI -= 1;
    Assert (u <= spline->elems [spline->knot_count - spline->degree].knot);
    ctrlI = knotI - (spline->degree - 1);

    column = 3;
    index = 0;

    // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
    {
        column = 2;
        index = 0;
        // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
        {
            column = 1;
            index = 0;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 0;
                left[column][0] = spline->elems [ctrlI + index].point[0];
                left[column][1] = spline->elems [ctrlI + index].point[1];
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 1;
                right[column][0] = spline->elems [ctrlI + index].point[0];
                right[column][1] = spline->elems [ctrlI + index].point[1];
            }
            column = 1;
            index = 0;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            left[column][0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
            left[column][1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
        }
        // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
        {
            column = 1;
            index = 1;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 1;
                left[column][0] = spline->elems [ctrlI + index].point[0];
                left[column][1] = spline->elems [ctrlI + index].point[1];
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 2;
                right[column][0] = spline->elems [ctrlI + index].point[0];
                right[column][1] = spline->elems [ctrlI + index].point[1];
            }
            column = 1;
            index = 1;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            right[column][0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
            right[column][1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
        }
        column = 2;
        index = 0;
        alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
        Assert (alpha >= 0 && alpha <= 1);
        left[column][0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
        left[column][1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
    }
    // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
    {
        column = 2;
        index = 1;
        // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
        {
            column = 1;
            index = 1;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 1;
                left[column][0] = spline->elems [ctrlI + index].point[0];
                left[column][1] = spline->elems [ctrlI + index].point[1];
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 2;
                right[column][0] = spline->elems [ctrlI + index].point[0];
                right[column][1] = spline->elems [ctrlI + index].point[1];
            }
            column = 1;
            index = 1;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            left[column][0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
            left[column][1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
        }
        // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
        {
            column = 1;
            index = 2;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 2;
                left[column][0] = spline->elems [ctrlI + index].point[0];
                left[column][1] = spline->elems [ctrlI + index].point[1];
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 3;
                right[column][0] = spline->elems [ctrlI + index].point[0];
                right[column][1] = spline->elems [ctrlI + index].point[1];
            }
            column = 1;
            index = 2;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            right[column][0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
            right[column][1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
        }
        column = 2;
        index = 1;
        alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
        Assert (alpha >= 0 && alpha <= 1);
        right[column][0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
        right[column][1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
    }
    column = 3;
    index = 0;

    alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
    Assert (alpha >= 0 && alpha <= 1);
    out[0] = (1 - alpha) * left[column - 1][0] + alpha * right[column - 1][0];
    out[1] = (1 - alpha) * left[column - 1][1] + alpha * right[column - 1][1];
}

static void evaluate_rbspline2d_degree3 (struct rbspline2d *spline, float u, _Out_writes_ (2) float *out) {
    float left [3] [2], right [3] [2];
    float weights [3] [2], weight;
    float alpha;
    int knotI;
    int ctrlI;
    int column;
    int index;
    const int degree = 3;

    /* unwrapped deboor algorithm for degree 3. only for reference */
    Assert (spline->degree == 3);
    Assert (get_stack_size (spline->elems) / sizeof *spline->elems > 3);
    knotI = (spline->degree - 1);
    while (u >= spline->elems [knotI].knot) {
        knotI += 1;
    }
    knotI -= 1;
    Assert (u <= spline->elems [spline->knot_count - spline->degree].knot);
    ctrlI = knotI - (spline->degree - 1);

    column = 3;
    index = 0;

    // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
    {
        column = 2;
        index = 0;
        // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
        {
            column = 1;
            index = 0;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 0;
                left [column] [0] = spline->elems [ctrlI + index].point[0];
                left [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [0] = spline->elems [ctrlI + index].weight;
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 1;
                right [column] [0] = spline->elems [ctrlI + index].point[0];
                right [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [1] = spline->elems [ctrlI + index].weight;
            }
            column = 1;
            index = 0;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            weights [column] [0] = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
            left [column] [0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weights [column] [0];
            left [column] [1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weights [column] [0];
        }
        // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
        {
            column = 1;
            index = 1;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 1;
                left [column] [0] = spline->elems [ctrlI + index].point[0];
                left [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [0] = spline->elems [ctrlI + index].weight;
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 2;
                right [column] [0] = spline->elems [ctrlI + index].point[0];
                right [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [1] = spline->elems [ctrlI + index].weight;
            }
            column = 1;
            index = 1;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            weights [column] [1] = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
            right [column] [0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weights [column] [1];
            right [column] [1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weights [column] [1];
        }
        column = 2;
        index = 0;
        alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
        Assert (alpha >= 0 && alpha <= 1);
        weights [column] [0] = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
        left [column] [0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weights [column] [0];
        left [column] [1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weights [column] [0];
    }
    // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
    {
        column = 2;
        index = 1;
        // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
        {
            column = 1;
            index = 1;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 1;
                left [column] [0] = spline->elems [ctrlI + index].point[0];
                left [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [0] = spline->elems [ctrlI + index].weight;
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 2;
                right [column] [0] = spline->elems [ctrlI + index].point[0];
                right [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [1] = spline->elems [ctrlI + index].weight;
            }
            column = 1;
            index = 1;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            weights [column] [0] = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
            left [column] [0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weights [column] [0];
            left [column] [1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weights [column] [0];
        }
        // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
        {
            column = 1;
            index = 2;
            // deboor_algorithm (degree, column - 1, index, u, interval, controls, left);
            {
                column = 0;
                index = 2;
                left [column] [0] = spline->elems [ctrlI + index].point[0];
                left [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [0] = spline->elems [ctrlI + index].weight;
            }
            // deboor_algorithm (degree, column - 1, index + 1, u, interval, controls, right);
            {
                column = 0;
                index = 3;
                right [column] [0] = spline->elems [ctrlI + index].point[0];
                right [column] [1] = spline->elems [ctrlI + index].point[1];
                weights [column] [1] = spline->elems [ctrlI + index].weight;
            }
            column = 1;
            index = 2;
            alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
            Assert (alpha >= 0 && alpha <= 1);
            weights [column] [1] = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
            right [column] [0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weights [column] [1];
            right [column] [1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weights [column] [1];
        }
        column = 2;
        index = 1;
        alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
        Assert (alpha >= 0 && alpha <= 1);
        weights [column] [1] = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
        right [column] [0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weights [column] [1];
        right [column] [1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weights [column] [1];
    }
    column = 3;
    index = 0;

    alpha = (u - spline->elems [knotI + index - degree + column].knot) / (spline->elems [knotI + index + 1].knot - spline->elems [knotI + index - degree + column].knot);
    Assert (alpha >= 0 && alpha <= 1);
    weight = (1 - alpha) * weights [column - 1] [0] + alpha * weights [column - 1] [1];
    out[0] = ((1 - alpha) * left [column - 1] [0] * weights [column - 1] [0] + alpha * right [column - 1] [0] * weights [column - 1] [1]) / weight;
    out[1] = ((1 - alpha) * left [column - 1] [1] * weights [column - 1] [0] + alpha * right [column - 1] [1] * weights [column - 1] [1]) / weight;
}

static void insert_knot_rbspline2d_degree3 (struct rbspline2d *spline, float u, int knot_ind) {
    int result;
    int ctrl_ind;
    float points [4] [2];
    float new_points [3] [3];
    float alpha;
    struct rbspline2d_elem new_elem, *left, *right;

    Assert (spline->degree == 3);
    Assert (knot_ind >= spline->degree && knot_ind < spline->knot_count - (spline->degree - 1));
    Assert (u >= spline->elems [knot_ind].knot && u < spline->elems [knot_ind + 1].knot);
    ctrl_ind = knot_ind - (spline->degree - 1);
    points [0] [0] = spline->elems [ctrl_ind + 0].point [0] * spline->elems [ctrl_ind + 0].weight;
    points [0] [1] = spline->elems [ctrl_ind + 0].point [1] * spline->elems [ctrl_ind + 0].weight;
    points [1] [0] = spline->elems [ctrl_ind + 1].point [0] * spline->elems [ctrl_ind + 1].weight;
    points [1] [1] = spline->elems [ctrl_ind + 1].point [1] * spline->elems [ctrl_ind + 1].weight;
    points [2] [0] = spline->elems [ctrl_ind + 2].point [0] * spline->elems [ctrl_ind + 2].weight;
    points [2] [1] = spline->elems [ctrl_ind + 2].point [1] * spline->elems [ctrl_ind + 2].weight;
    points [3] [0] = spline->elems [ctrl_ind + 3].point [0] * spline->elems [ctrl_ind + 3].weight;
    points [3] [1] = spline->elems [ctrl_ind + 3].point [1] * spline->elems [ctrl_ind + 3].weight;
    alpha = (u - spline->elems [knot_ind - 2].knot) / (spline->elems [knot_ind + 1].knot - spline->elems [knot_ind - 2].knot);
    new_points [0] [2] = (1 - alpha) * spline->elems [ctrl_ind + 0].weight + alpha * spline->elems [ctrl_ind + 1].weight;
    new_points [0] [0] = ((1 - alpha) * points [0] [0] + alpha * points [1] [0]) / new_points [0] [2];
    new_points [0] [1] = ((1 - alpha) * points [0] [1] + alpha * points [1] [1]) / new_points [0] [2];
    alpha = (u - spline->elems [knot_ind - 1].knot) / (spline->elems [knot_ind + 2].knot - spline->elems [knot_ind - 1].knot);
    new_points [1] [2] = (1 - alpha) * spline->elems [ctrl_ind + 1].weight + alpha * spline->elems [ctrl_ind + 2].weight;
    new_points [1] [0] = ((1 - alpha) * points [1] [0] + alpha * points [2] [0]) / new_points [1] [2];
    new_points [1] [1] = ((1 - alpha) * points [1] [1] + alpha * points [2] [1]) / new_points [1] [2];
    alpha = (u - spline->elems [knot_ind].knot) / (spline->elems [knot_ind + 3].knot - spline->elems [knot_ind].knot);
    new_points [2] [2] = (1 - alpha) * spline->elems [ctrl_ind + 2].weight + alpha * spline->elems [ctrl_ind + 3].weight;
    new_points [2] [0] = ((1 - alpha) * points [2] [0] + alpha * points [3] [0]) / new_points [2] [2];
    new_points [2] [1] = ((1 - alpha) * points [2] [1] + alpha * points [3] [1]) / new_points [2] [2];
    new_elem.knot = u;
    memcpy (insert_stack (spline->elems, spline->elems + knot_ind + 1, sizeof new_elem), &new_elem, sizeof new_elem);
    memcpy (spline->elems [ctrl_ind + 1].point, new_points [0], sizeof spline->elems [ctrl_ind + 1].point);
    memcpy (spline->elems [ctrl_ind + 2].point, new_points [1], sizeof spline->elems [ctrl_ind + 2].point);
    memcpy (spline->elems [ctrl_ind + 3].point, new_points [2], sizeof spline->elems [ctrl_ind + 3].point);
    spline->elems [ctrl_ind + 1].weight = new_points [0] [2];
    spline->elems [ctrl_ind + 2].weight = new_points [1] [2];
    spline->elems [ctrl_ind + 3].weight = new_points [2] [2];
    spline->knot_count += 1;
}


static void init_debug_rbspline2d_configuration (struct root *root, struct rbspline2d *spline) {
    int index, knot_ind;
    float knots [32];
    int knots_count;

    spline->degree = 3;
    // spline->degree = 2;
    spline->knot_count = 8;
    knots_count = spline->knot_count;
    push_stack (spline->elems, spline->knot_count * sizeof *spline->elems);
    index = 2;
    while (index < spline->knot_count - 2) {
        knots [index] = spline->elems [index].knot = (float) (index - 2) / (spline->knot_count - 5);
        index += 1;
    }
    spline->elems [0].knot = knots [0] = spline->elems [2].knot;
    spline->elems [1].knot = knots [1] = spline->elems [2].knot;
    spline->elems [spline->knot_count - 1].knot = knots [spline->knot_count - 1] = spline->elems [spline->knot_count - 3].knot;
    spline->elems [spline->knot_count - 2].knot = knots [spline->knot_count - 2] = spline->elems [spline->knot_count - 3].knot;
    index = 0;
    while (index < spline->knot_count - (spline->degree - 1)) {
        spline->elems [index].point[0] = 50 + 100 * index;
        spline->elems [index].point[1] = 50 + 200 * (index & 1);
        spline->elems [index].weight = 1;
        index += 1;
    }
    spline->elems [3].point [1] = 500;
    spline->elems [3].weight = 0.1f;
    index = 0;
    knot_ind = 3;
    while (index < knots_count - 6) {
        insert_knot_rbspline2d_degree3 (spline, knots [index + 3], knot_ind);
        knot_ind += 1;
        insert_knot_rbspline2d_degree3 (spline, knots [index + 3], knot_ind);
        knot_ind += 2;
        index += 1;
    }
}

static void push_nurbs (struct nurbs *nurbs, int udelta, int vdelta) {
    Assert (udelta >= 0 && vdelta >= 0);
    if (udelta > 0 || vdelta > 0) {
        int new_width, new_height;

        if (0 == nurbs->net) {
            nurbs->net = allocate_stack (0);
        }
        if (0 == nurbs->knots) {
            nurbs->knots = allocate_stack (0);
        }
        new_width = nurbs->width + udelta;
        new_height = nurbs->height + vdelta;
        if (nurbs->width == 0 || nurbs->height == 0) {
            int size;

            Assert (nurbs->width == 0 && nurbs->height == 0);
            Assert (get_stack_size (nurbs->net) == 0 && get_stack_size (nurbs->knots) == 0);
            Assert (nurbs->net != 0 && nurbs->knots != 0);
            size = NURBS_Net_Total (new_width, new_height) * sizeof *nurbs->net;
            memset (push_stack (nurbs->net, size), 0, size);
            size = NURBS_Knots_Total (new_width, new_height) * sizeof *nurbs->knots;
            memset (push_stack (nurbs->knots, size), 0, size);
        } else {
            int net_delta, knots_delta;

            net_delta = NURBS_Net_Total (new_width, new_height) - NURBS_Net_Total (nurbs->width, nurbs->height);
            knots_delta = NURBS_Knots_Total (new_width, new_height) - NURBS_Knots_Total (nurbs->width, nurbs->height);
            memset (push_stack (nurbs->net, net_delta * sizeof *nurbs->net), 0, net_delta * sizeof *nurbs->net);
            memset (push_stack (nurbs->knots, knots_delta * sizeof *nurbs->knots), 0, knots_delta * sizeof *nurbs->knots);
            if (udelta > 0) {
                int index;

                index = 0;
                while (index < nurbs->height) {
                    memmove (nurbs->net + new_width * (index + 1), nurbs->net + new_width * index + nurbs->width, ((nurbs->height - (index + 1)) * nurbs->width) * sizeof *nurbs->net);
                    memset (nurbs->net + new_width * index + nurbs->width, 0, udelta * sizeof *nurbs->net);
                    index += 1;
                }
            }
        }
        nurbs->width = new_width;
        nurbs->height = new_height;
    }
}

static inline struct nurbs_oneknot *get_nurbs_u_oneknot (struct nurbs *nurbs) {
    return ((struct nurbs_oneknot *) &nurbs->knots->u);
}

static inline struct nurbs_oneknot *get_nurbs_v_oneknot (struct nurbs *nurbs) {
    return ((struct nurbs_oneknot *) &nurbs->knots->v);
}

static void init_nurbs_clamped_uniform_knots_by_oneknot (struct nurbs_oneknot *knots, int count) {
    int index;

    knots [0].value = 0;
    knots [1].value = 0;
    index = 2;
    while (index < count - 2) {
        knots [index].value = (float) (index - 2) / (count - 5);
        index += 1;
    }
    knots [count - 2].value = 1;
    knots [count - 1].value = 1;
}

static void init_nurbs_clamped_uniform_knots (struct nurbs *nurbs) {
    init_nurbs_clamped_uniform_knots_by_oneknot (get_nurbs_u_oneknot (nurbs), NURBS_Knots_Dim (nurbs->width));
    init_nurbs_clamped_uniform_knots_by_oneknot (get_nurbs_v_oneknot (nurbs), NURBS_Knots_Dim (nurbs->height));
}

static void init_nurbs_uniform_knots_by_oneknot (struct nurbs_oneknot *knots, int count) {
    int     index;

    index = 0;
    while (index < count) {
        knots[index].value = (float) (index) / (count - 1);
        index += 1;
    }
}

static void init_nurbs_uniform_knots (struct nurbs *nurbs) {
    init_nurbs_uniform_knots_by_oneknot (get_nurbs_u_oneknot (nurbs), NURBS_Knots_Dim (nurbs->width));
    init_nurbs_uniform_knots_by_oneknot (get_nurbs_v_oneknot (nurbs), NURBS_Knots_Dim (nurbs->height));
}

static void init_debug_nurbs_configuration (struct nurbs *nurbs, int is_clamped) {
    int result;
    int u, v;

    Zero (nurbs);
    push_nurbs (nurbs, 3, 3);
    if (is_clamped) {
        init_nurbs_clamped_uniform_knots (nurbs);
    } else {
        init_nurbs_uniform_knots (nurbs);
    }
    v = 0;
    while (v < NURBS_Net_Dim (nurbs->height)) {
        u = 0;
        while (u < NURBS_Net_Dim (nurbs->width)) {
            struct nurbs_net *net;

            /* wave-like configuration of control points */
            net = nurbs->net + v * NURBS_Net_Dim (nurbs->width) + u;
            net->wpoint [0] = u * 1;
            net->wpoint [1] = ((u & 1) == (v & 1)) * 4;
            net->wpoint [2] = -v * 1;
            net->wpoint [3] = 1;
            u += 1;
        }
        v += 1;
    }
    /* make one of the points have weight */
    nurbs->net [2 * NURBS_Net_Dim (nurbs->width) + 3].wpoint [0] *= 10;
    nurbs->net [2 * NURBS_Net_Dim (nurbs->width) + 3].wpoint [1] *= 10;
    nurbs->net [2 * NURBS_Net_Dim (nurbs->width) + 3].wpoint [2] *= 10;
    nurbs->net [2 * NURBS_Net_Dim (nurbs->width) + 3].wpoint [3] = 10;
}

static int count_oneknot_patches (struct nurbs_oneknot *knots, int count) {
    struct nurbs_oneknot *end;

    end = knots + count - NURBS_Degree + 1;
    count = 0;
    knots += NURBS_Degree;
    while (knots < end) {
        if (knots->value - knots[-1].value > NURBS_Knot_Delta) {
            count += 1;
        }
        knots += 1;
    }
    return (count);
}

static void count_uv_patches_in_nurbs_knots (struct nurbs *nurbs, int *udim, int *vdim) {
    *udim = count_oneknot_patches (get_nurbs_u_oneknot (nurbs), NURBS_Knots_Dim (nurbs->width));
    *vdim = count_oneknot_patches (get_nurbs_v_oneknot (nurbs), NURBS_Knots_Dim (nurbs->height));
}

static void copy_nurbs_1x1patch_to_local (struct nurbs *nurbs, int uknot, int vknot, struct nurbs *local) {
    int u, v;

    Assert (uknot >= NURBS_Degree - 1 && uknot < NURBS_Knots_Dim (nurbs->width) - NURBS_Degree);
    Assert (vknot >= NURBS_Degree - 1 && vknot < NURBS_Knots_Dim (nurbs->height) - NURBS_Degree);
    if (uknot > NURBS_Degree - 1) {
        local->knots [0].u = nurbs->knots [uknot - NURBS_Degree].u;
    }
    if (vknot > NURBS_Degree - 1) {
        local->knots [0].v = nurbs->knots [vknot - NURBS_Degree].v;
    }
    u = 1;
    while (u < NURBS_Local_Knots_Dim) {
        local->knots [u].u = nurbs->knots [uknot - (NURBS_Degree - 1) + (u - 1)].u;
        local->knots [u].v = nurbs->knots [vknot - (NURBS_Degree - 1) + (u - 1)].v;
        u += 1;
    }
    v = vknot == NURBS_Degree - 1;
    while (v < NURBS_Local_Net_Dim - (vknot + 1 == NURBS_Knots_Dim (nurbs->height) - NURBS_Degree)) {
        u = uknot == NURBS_Degree - 1;
        memcpy (local->net + v * NURBS_Local_Net_Dim + u, nurbs->net + (NURBS_CP_From_KnotI (vknot) + (v - 1)) * NURBS_Net_Dim (nurbs->width) + NURBS_CP_From_KnotI (uknot) + (u - 1), (NURBS_Local_Net_Dim - u - (uknot + 1 == NURBS_Knots_Dim (nurbs->height) - NURBS_Degree)) * sizeof *nurbs->net);
        v += 1;
    }
}

static void multiply_u_knot_in_local_nurbs (struct nurbs *local, int uknot, int vnet_start, int vnet_pad, int *out_unet_start, int *out_unet_pad) {
    int vnet;
    int unet;

    unet = NURBS_CP_From_KnotI (uknot);
    vnet = vnet_start;
    while (vnet < NURBS_Local_Net_Dim - vnet_pad) {
        struct nurbs_net new_points [2];
        float alpha, ralpha;
        struct nurbs_net *row;
        struct nurbs_knot *knotrow;
        int copy_count;

        row = local->net + vnet * NURBS_Local_Net_Dim + unet;
        knotrow = local->knots + uknot;
        alpha = (knotrow[0].u - knotrow[-2].u) / (knotrow[1].u - knotrow[-2].u);
        ralpha = 1 - alpha;
        new_points [0].wpoint[0] = ralpha * row[0].wpoint[0] + alpha * row[1].wpoint[0];
        new_points [0].wpoint[1] = ralpha * row[0].wpoint[1] + alpha * row[1].wpoint[1];
        new_points [0].wpoint[2] = ralpha * row[0].wpoint[2] + alpha * row[1].wpoint[2];
        new_points [0].wpoint[3] = ralpha * row[0].wpoint[3] + alpha * row[1].wpoint[3];
        if (knotrow[0].u - knotrow[-1].u > NURBS_Knot_Delta) {
            alpha = (knotrow[0].u - knotrow[-1].u) / (knotrow[2].u - knotrow[-1].u);
            ralpha = 1 - alpha;
            new_points [1].wpoint[0] = ralpha * row[1].wpoint[0] + alpha * row[2].wpoint[0];
            new_points [1].wpoint[1] = ralpha * row[1].wpoint[1] + alpha * row[2].wpoint[1];
            new_points [1].wpoint[2] = ralpha * row[1].wpoint[2] + alpha * row[2].wpoint[2];
            new_points [1].wpoint[3] = ralpha * row[1].wpoint[3] + alpha * row[2].wpoint[3];
            copy_count = 2;
        } else {
            copy_count = 1;
        }
        if (uknot < NURBS_Local_Knots_Dim / 2) {
            if (unet) {
                memmove (row - unet, row - unet + 1, unet * sizeof *row);
            }
            memcpy (row, new_points, copy_count * sizeof *new_points);
        } else {
            Assert (unet);
            memmove (row + 2, row + 1, (NURBS_Local_Net_Dim - (unet + 2)) * sizeof *row);
            memcpy (row + 1, new_points, copy_count * sizeof *new_points);
        }
        vnet += 1;
    }
    if (uknot < NURBS_Local_Knots_Dim / 2) {
        vnet = 0;
        while (vnet < uknot) {
            local->knots [vnet].u = local->knots [vnet + 1].u;
            vnet += 1;
        }
        *out_unet_start = 0;
    } else {
        vnet = NURBS_Local_Knots_Dim - 1;
        while (vnet > uknot) {
            local->knots [vnet].u = local->knots [vnet - 1].u;
            vnet -= 1;
        }
        *out_unet_pad = 0;
    }
}

void multiply_v_knot_in_local_nurbs (struct nurbs *local, int vknot, int unet_start, int unet_pad) {
    int unet;
    int vnet;
    int stride;

    vnet = NURBS_CP_From_KnotI (vknot);
    stride = NURBS_Local_Net_Dim;
    unet = unet_start;
    while (unet < NURBS_Local_Net_Dim - unet_pad) {
        struct nurbs_net new_points [2];
        float alpha, ralpha;
        struct nurbs_net *row [4];
        struct nurbs_knot *knotrow;
        int copy_second;

        knotrow = local->knots + vknot;
        row [0] = local->net + vnet * NURBS_Local_Net_Dim + unet;
        row [1] = row [0] + stride;
        row [2] = row [1] + stride;
        row [3] = row [2] + stride;
        alpha = (knotrow [0].v - knotrow [-2].v) / (knotrow [1].v - knotrow [-2].v);
        ralpha = 1 - alpha;
        new_points [0].wpoint [0] = ralpha * row [0]->wpoint [0] + alpha * row [1]->wpoint [0];
        new_points [0].wpoint [1] = ralpha * row [0]->wpoint [1] + alpha * row [1]->wpoint [1];
        new_points [0].wpoint [2] = ralpha * row [0]->wpoint [2] + alpha * row [1]->wpoint [2];
        new_points [0].wpoint [3] = ralpha * row [0]->wpoint [3] + alpha * row [1]->wpoint [3];
        if (knotrow [0].v - knotrow [-1].v > NURBS_Knot_Delta) {
            alpha = (knotrow [0].v - knotrow [-1].v) / (knotrow [2].v - knotrow [-1].v);
            ralpha = 1 - alpha;
            new_points [1].wpoint [0] = ralpha * row [1]->wpoint [0] + alpha * row [2]->wpoint [0];
            new_points [1].wpoint [1] = ralpha * row [1]->wpoint [1] + alpha * row [2]->wpoint [1];
            new_points [1].wpoint [2] = ralpha * row [1]->wpoint [2] + alpha * row [2]->wpoint [2];
            new_points [1].wpoint [3] = ralpha * row [1]->wpoint [3] + alpha * row [2]->wpoint [3];
            copy_second = 1;
        } else {
            copy_second = 0;
        }
        if (vknot < NURBS_Local_Knots_Dim / 2) {
            int index;

            index = 0;
            while (index < vnet) {
                (local->net + unet) [index * stride] = (local->net + unet) [(index + 1) * stride];
                index += 1;
            }
            *row [0] = new_points [0];
            if (copy_second) {
                *row [1] = new_points [1];
            }
        } else {
            int index;

            index = NURBS_Local_Net_Dim - 1;
            while (index > vnet) {
                (local->net + unet) [index * stride] = (local->net + unet) [(index - 1) * stride];
                index -= 1;
            }
            *row [1] = new_points [0];
            if (copy_second) {
                *row [2] = new_points [1];
            }
        }
        unet += 1;
    }
    if (vknot < NURBS_Local_Knots_Dim / 2) {
        unet = 0;
        while (unet < vknot) {
            local->knots [unet].v = local->knots [unet + 1].v;
            unet += 1;
        }
    } else {
        unet = NURBS_Local_Knots_Dim - 1;
        while (unet > vknot) {
            local->knots [unet].v = local->knots [unet - 1].v;
            unet -= 1;
        }
    }
}

static void convert_local_nurbs_to_rbezier_points (struct nurbs *local, int unet_start, int unet_pad, int vnet_start, int vnet_pad, struct rbezier3d *patch) {
    int v;

    if (local->knots [2 + 1].u - local->knots [1 + 1].u > NURBS_Knot_Delta) {
        multiply_u_knot_in_local_nurbs (local, 2 + 1, vnet_start, vnet_pad, &unet_start, &unet_pad);
    }
    if (local->knots [1 + 1].u - local->knots [0 + 1].u > NURBS_Knot_Delta) {
        multiply_u_knot_in_local_nurbs (local, 1 + 1, vnet_start, vnet_pad, &unet_start, &unet_pad);
    }
    if (local->knots [4 + 1].u - local->knots [3 + 1].u > NURBS_Knot_Delta) {
        multiply_u_knot_in_local_nurbs (local, 3 + 1, vnet_start, vnet_pad, &unet_start, &unet_pad);
    }
    if (local->knots [5 + 1].u - local->knots [4 + 1].u > NURBS_Knot_Delta) {
        multiply_u_knot_in_local_nurbs (local, 4 + 1, vnet_start, vnet_pad, &unet_start, &unet_pad);
    }
    if (local->knots [2 + 1].v - local->knots [1 + 1].v > NURBS_Knot_Delta) {
        multiply_v_knot_in_local_nurbs (local, 2 + 1, unet_start, unet_pad);
    }
    if (local->knots [1 + 1].v - local->knots [0 + 1].v > NURBS_Knot_Delta) {
        multiply_v_knot_in_local_nurbs (local, 1 + 1, unet_start, unet_pad);
    }
    if (local->knots [4 + 1].v - local->knots [3 + 1].v > NURBS_Knot_Delta) {
        multiply_v_knot_in_local_nurbs (local, 3 + 1, unet_start, unet_pad);
    }
    if (local->knots [5 + 1].v - local->knots [4 + 1].v > NURBS_Knot_Delta) {
        multiply_v_knot_in_local_nurbs (local, 4 + 1, unet_start, unet_pad);
    }
    v = 0;
    while (v < NURBS_Net_Dim (1)) {
        Static_Assert (sizeof (struct rbezier3d) == sizeof (struct nurbs_net));
        memcpy (patch + v * NURBS_Net_Dim (1), local->net + (v + 1) * NURBS_Local_Net_Dim + 1, NURBS_Net_Dim (1) * sizeof *patch);
        v += 1;
    }
}

static void convert_nurbs_to_rbezier_patches (struct nurbs *nurbs, struct rbezier3d *_Stack out) {
    int result;
    int udim, vdim;
    int u, v;

    count_uv_patches_in_nurbs_knots (nurbs, &udim, &vdim);
    prepare_stack (out, 16 * udim * vdim * sizeof *out);
    v = NURBS_Degree;
    while (v < NURBS_Knots_Dim (nurbs->height) - (NURBS_Degree - 1)) {
        if (nurbs->knots[v].v - nurbs->knots[v - 1].v > NURBS_Knot_Delta) {
            u = NURBS_Degree;
            while (u < NURBS_Knots_Dim (nurbs->width) - (NURBS_Degree - 1)) {
                if (nurbs->knots[u].u - nurbs->knots[u - 1].u > NURBS_Knot_Delta) {
                    struct rbezier3d *patch;
                    struct nurbs local;
                    char local_knots_data [sizeof (struct stack_prefix) + NURBS_Local_Knots_Total * sizeof (struct nurbs_knot)];
                    char local_net_data [sizeof (struct stack_prefix) + NURBS_Local_Net_Total * sizeof (struct nurbs_net)];

                    patch = push_stack_ (out, 16 * sizeof *patch);
                    Zero (&local);
                    local.width = local.height = 1;
                    local.net = init_static_stack (local_net_data, sizeof local_net_data, 0);
                    local.knots = init_static_stack (local_knots_data, sizeof local_knots_data, 0);
                    copy_nurbs_1x1patch_to_local (nurbs, u - 1, v - 1, &local);
                    convert_local_nurbs_to_rbezier_points (&local, u == NURBS_Degree, u == NURBS_Knots_Dim (nurbs->width) - NURBS_Degree, v == NURBS_Degree, v == NURBS_Knots_Dim (nurbs->height) - NURBS_Degree, patch);
                }
                u += 1;
            }
        }
        v += 1;
    }
}

