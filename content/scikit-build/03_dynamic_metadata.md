---
authors: [Cristian Le]
---

# Dynamic Metadata

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/4_03_dynamic_metadata>`

A method to handle `project.dynamic` fields generically.
The [specification] is generic, but currently, it is a feature used only in scikit-build-core.
I.e. for now you must use

```{code} toml
:filename: pyproject.toml

[build-system]
requires = ["scikit-build-core"]
build-backend = "scikit_build_core.build"
```

[specification]: https://github.com/scikit-build/dynamic-metadata

## Example

::::{tab-set}

:::{tab-item} Generic (recommended)

```{code} toml
:filename: pyproject.toml

[project]
name = "mypackage"
dynamic = ["version"]

[tool.scikit-build]
sdist.include = ["src/package/_version.py"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.setuptools_scm"

[tool.setuptools_scm]  # Section required
write_to = "src/package/_version.py"
```

:::

:::{tab-item} scikit-build

```{code} toml
:filename: pyproject.toml

[project]
name = "mypackage"
dynamic = ["version"]

[tool.scikit-build]
sdist.include = ["src/package/_version.py"]
metadata.version.provider = "scikit_build_core.metadata.setuptools_scm"

[tool.setuptools_scm]  # Section required
write_to = "src/package/_version.py"
```

:::

::::

## From the user side

- Add a field that you want to be resolved dynamically in `project.dynamic` [^1]
- Add a `[[tool.dynamic-metadata]]` section with a `provider` that can provide that field
- Configure all other (arbitrary) keys that the provider consumes in `tool.dynamic-metadata` or other `tool.*`
- Profit?

[^1]: With PEP808 you can have mixed static and dynamic fields for table and array fields like `dependencies`, `scripts`

## Pre-defined plugins

There are some pre-defined plugins to get you started both in `scikit-build-core` or `dynamic-metadata`.
To use the latter, make sure `dynamic-metadata` is in the `build-system.requires`.
For an updated documentation check the relevant documentation section in [scikit-build-core] and [dynamic-metadata].

[scikit-build-core]: https://scikit-build-core.readthedocs.io/en/latest/configuration/dynamic.html#built-in-plugins
[dynamic-metadata]: https://dynamic-metadata.readthedocs.io/en/latest/plugins.html#bundled-plugins

::::{tab-set}

:::{tab-item} `scikit_build_core.metadata.setuptools_scm`

Get version from git metadata

```toml
[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.setuptools_scm"

[tool.setuptools_scm]
```

- dynamic field: `version`
- inputs: `tool.setuptools_scm`

See [setuptools-scm]

[setuptools-scm]: https://setuptools-scm.readthedocs.io/en/latest/

:::

:::{tab-item} `(scikit_build_core.metadata|dynamic_metadata.plugins).regex`

Format a field from a regex search of a file

```toml
[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.regex"
field = "version"
input = "src/package/_version.py"
```

- dynamic field: any, specify in `field`
- inputs:
  - `field`: The metadata field to set
  - `input`: The file to read
  - `regex`: The pattern to search for (default: match for `__version__`, `VERSION`)
  - `result`: `str.format` template to generate with the named regex groups (default: `{value}`)
  - `remove`: A regex to be stripped after `result` is rendered

:::

:::{tab-item} `(scikit_build_core.metadata|dynamic_metadata.plugins).template`

Format a field from other values in `project` table

```toml
[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.template"
field = "dependencies"
result = ["mypackage-core == {project[version]}"]
```

- dynamic field: any, specify in `field`
- inputs:
  - `field`: The metadata field to set
  - `result`: `str.format` template to generate with the `project` table

:::

:::{tab-item} `scikit_build_core.metadata.fancy_pypi_readme`

Generate a comprehensive readme from snippets or with substitution

```toml
[project]
name = "mypackage"
dynamic = ["readme"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.fancy_pypi_readme"

[tool.hatch.metadata.hooks.fancy-pypi-readme]
```

- dynamic field: `readme`
- inputs: `[tool.hatch.metadata.hooks.fancy-pypi-readme]`

See [hatch-fancy-pypi-readme]

[hatch-fancy-pypi-readme]: https://github.com/hynek/hatch-fancy-pypi-readme

:::

:::{tab-item} `dynamic_metadata.plugins.ast`

Parse a Python file and use a variable

```toml
[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.ast"
field = "version"
input = "src/my_package/__init__.py"
name = "__version__"
```

- dynamic field: any, specify in `field`
- inputs:
  - `field`: The metadata field to set
  - `input` : The python file to parse
  - `name`: The global variable to read

:::

:::{tab-item} `dynamic_metadata.plugins.from_file`

Read a file and pass it to the field

```toml
[project]
name = "mypackage"
dynamic = ["dependencies"]

[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.from_file"
field = "dependencies"
path = "requirements.txt"
```

- dynamic field: any, specify in `field`
- inputs:
  - `field`: The metadata field to set
  - `path` : The file to read

:::

:::{tab-item} `dynamic_metadata.plugins.static`

Set the field statically

```toml
[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.static"
version = "1.2.3"
```

- dynamic field: any
- inputs: any `project` fields

:::

:::{tab-item} `dynamic_metadata.plugins.readme_fragment`

Build a readme from multiple fragments

```toml
[project]
name = "mypackage"
dynamic = ["readme"]

[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.readme_fragment"
path = "README.md"
```

- dynamic field: `readme`
- inputs:
  - `text`: A literal text
  - `path`: A file to read
  - `content-type`: The readme content (default: `text/markdown`)
  - `start-after`, `end-before`: Markers indicating the content (exclusive)
  - `start-at`, `end-at`: Markers indicating the content (inclusive)
  - `pattern`: A regex pattern to capture the fragment

:::

:::{tab-item} `dynamic_metadata.plugins.pin_installed`

Pin the runtime dependencies to the versions in `build-system.requires`

```toml
[project]
name = "mypackage"
dynamic = ["dependencies"]

[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.pin_installed"
packages = ["torch==x.x.*"]
```

- dynamic field: `dependencies`
- inputs:
  - `packages`: A list of requirement templates

:::

:::{tab-item} `dynamic_metadata.plugins.substitute`

Apply a regex substitution on the field previously generated

```toml
[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.substitute"
field = "readme"
pattern = "#(\\d+)"
replacement = "[#\\1](https://github.com/org/repo/issues/\\1)"
```

- dynamic field: any, specify in `field`
- inputs:
  - `field`: The metadata field to transform
  - `pattern`: The regex pattern to search
  - `replacement`: The regex replacement
  - `ignore-case`: Whether to match case-insensitively
  - `format`: Whether to resolve `project`

:::

::::

## Order matters

The `tool.dynamic-metadata` are processed in the order they appear in `pyproject.toml`.
If a provider expects a dynamic field to have been resolved, make sure you order them appropriately.

``` toml
:filename: pyproject.toml
:emphasize-lines: 5-11
:linenos:

[project]
name = "mypackage"
dynamic = ["version", "dependencies"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.setuptools_scm"

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.template"
field = "dependencies"
result = ["mypackage-core == {project[version]}"]

[tool.setuptools_scm]
```

## Want more? Custom plugins

You can write your own plugin and use it locally or distribute it via `dynamic_metadata.provider` entry-point.

```toml
[[tool.dynamic-metadata]]
provider = {path = "helpers/plugins", module = "my_plugin"}
```

The referenced module/class must have at least the function

```python
def dynamic_metadata(
    settings: Mapping[str, Any],
    project: Mapping[str, Any],
) -> dict[str, Any]:
    """
    :param settings: all the additional fields defined in the ``[[tool.dynamic-metadata]]``
    :param project: current view of the ``[project]``
    :return: a ``[project]`` snippet to be merged
    """
```
