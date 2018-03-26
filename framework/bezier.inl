// https://www.scratchapixel.com/lessons/advanced-rendering/bezier-curve-rendering-utah-teapot
// Compute the position of a point along a Bezier curve at t [0:1]
inline rapid::vector3 evalBezierCurve(const rapid::vector3 *P, const float &t)
{
    float b0 = (1 - t) * (1 - t) * (1 - t);
    float b1 = 3 * t * (1 - t) * (1 - t);
    float b2 = 3 * t * t * (1 - t);
    float b3 = t * t * t;

    return P[0] * b0 + P[1] * b1 + P[2] * b2 + P[3] * b3;
}

inline rapid::vector3 evalBezierPatch(const rapid::vector3 *controlPoints, const float &u, const float &v)
{
    rapid::vector3 uCurve[4];
    for (int i = 0; i < 4; ++i)
        uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);

    return evalBezierCurve(uCurve, v);
}

inline rapid::vector3 derivBezier(const rapid::vector3 *P, const float &t)
{
    return -3 * (1 - t) * (1 - t) * P[0] +
        (3 * (1 - t) * (1 - t) - 6 * t * (1 - t)) * P[1] +
        (6 * t * (1 - t) - 3 * t * t) * P[2] +
        3 * t * t * P[3];
}

// Compute the derivative of a point on Bezier patch along the u parametric direction
inline rapid::vector3 dUBezier(const rapid::vector3 *controlPoints, const float &u, const float &v)
{
    rapid::vector3 P[4];
    rapid::vector3 vCurve[4];
    for (int i = 0; i < 4; ++i) {
        P[0] = controlPoints[i];
        P[1] = controlPoints[4 + i];
        P[2] = controlPoints[8 + i];
        P[3] = controlPoints[12 + i];
        vCurve[i] = evalBezierCurve(P, v);
    }

    return derivBezier(vCurve, u);
}

// Compute the derivative of a point on Bezier patch along the v parametric direction
inline rapid::vector3 dVBezier(const rapid::vector3 *controlPoints, const float &u, const float &v)
{
    rapid::vector3 uCurve[4];
    for (int i = 0; i < 4; ++i) {
        uCurve[i] = evalBezierCurve(controlPoints + 4 * i, u);
    }

    return derivBezier(uCurve, v);
}
