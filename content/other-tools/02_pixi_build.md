# Pixi-build

In this section we'll see how we can use [Pixi Build](https://pixi.prefix.dev/latest/build/getting_started/) to extend all of the packaging concepts that we've learned about so far into conda packages with as little additional effort as possible.
Pixi Build allows for defining metadata in the Pixi manifest that define [package](https://pixi.prefix.dev/latest/reference/pixi_manifest/#the-package-section), [build](https://pixi.prefix.dev/latest/reference/pixi_manifest/#build-table), [build-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#build-dependencies), [host-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#host-dependencies), and [run-dependencies](https://pixi.prefix.dev/latest/reference/pixi_manifest/#host-dependencies) TOML tables.
These tables can be used together with the information in the rest of the Pixi manifest to build and install conda packages without having to have the full formal specification of a `rattler-build` recipe, like those shown in the [Shipping to conda-forge section](https://scikit-build.org/SIMPLE-Py/conda-forge/).
To learn the sections and functionalities by example rather than by inspection of the spec, we'll take the examples from previous packaging sections and adapt them to Pixi Build.

## Basic Packaging
