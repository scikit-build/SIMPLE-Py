---
marp: true
theme: simplepy
paginate: true
_paginate: skip
_class: lead
---

# SIMPLE-Py

## Scikit-build-core: Dynamic metadata

---

- `setuptools-scm` is cool
* Wanted even more control?
* Wanted to inject your own helpers?

---

# Example: `pyproject.toml`
```toml
[build-system]
requires = ["scikit-build-core"]
build-backend = "scikit_build_core.build"

[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = "scikit_build_core.metadata.setuptools_scm"

[tool.setuptools_scm]
```

---

# You can chain them

```toml
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

---

# There are many built-ins

```toml
[project]
name = "mypackage"
dynamic = ["dependencies"]

[[tool.dynamic-metadata]]
provider = "dynamic_metadata.plugins.pin_installed"
packages = ["torch==x.x.*"]
```

---

# And you can write your own


<div class="columns">
<div>

```toml
[project]
name = "mypackage"
dynamic = ["version"]

[[tool.dynamic-metadata]]
provider = {path = "helpers/plugins", module = "my_plugin"}
```

</div>
<div>


```python
def dynamic_metadata(
    settings: Mapping[str, Any],
    project: Mapping[str, Any],
) -> dict[str, Any]:
    return {"version": "1.2.3"}
```

</div>
</div>

---

# Caveats

- It is only implemented in `scikit-build-core` right now
- Not even as scikit-build-core plugin
