# Homogeneous 2D transformations

The Checkpoint 2 transformation engine is implemented in
`checkpoint2/transformations/include/transform2d.hpp`.

It stores row-major 3 x 3 matrices and multiplies them by homogeneous column
vectors. The rightmost matrix in a product is therefore applied first.
Translation, scaling, rotation, pivot composition and polygon transformation
are implemented without a graphics or mathematics library.

The accompanying test executable validates individual and combined operations.
The demonstration renderer transforms every polygon component of the Apex3D
Formula 1 car and exports a PPM/PNG comparison image.

See `checkpoint2/transformations/README.md` for equations, compilation commands,
panel interpretation, tests and limitations.
