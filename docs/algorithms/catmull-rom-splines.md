# Catmull-Rom spline implementation

The Checkpoint 1 spline demonstration is implemented in
`checkpoint1/splines/main.cpp`. It uses the uniform Catmull-Rom basis directly
in Pure C++20 and rasterises sampled points into a PPM canvas.

For each group of four points, the segment begins at the second point and ends
at the third. Open paths duplicate their endpoints, while closed paths use
modular indexing. The derivative of the cubic polynomial supplies the tangent
used for vehicle orientation.

See `checkpoint1/splines/README.md` for the equation, build commands,
verification cases, visual legend and limitations.
