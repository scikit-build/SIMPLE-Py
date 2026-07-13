---
authors: [Cristian Le]
---

# Shipping to distros

{button}`Slides <https://scikit-build.org/SIMPLE-Py/slides/5_04_distros>`

Is packaging to distros still valuable these days?
Yes, both to the ecosystem in general and your own project.[^1]

[^1]: <https://packaging.lecris.dev/>

## Why get involved?

- You find what is currently used across the ecosystem
- You get reverse-dependency tests
- You get notified early of dependencies breakage
- You get help from the packagers

## Participating as upstream

You do not have to be a packager to help with distro packaging.
Just running the downstream package build in upstream is a huge step towards it.
If your project is not yet packaged, that's fine, you can do it yourself.

The only files you need to manage are a `.spec` file and a `packit` configuration[^2].
The spec file can be quite minimal and low/no maintenance:

```rpmspec
Name:           foo
Version:        0.0.0
Release:        %autorelease
Summary:        Example package
License:        Unlicense
URL:            https://example.com
Source:         %{pypi_source foo}
BuildRequires:  python3-devel

%description
Lorem ipsum

%prep
%autosetup -n foo-%{version}

%generate_buildrequires
%pyproject_buildrequires -x test

%build
%pyproject_wheel

%install
%pyproject_install
%pyproject_save_files foo

%check
%pytest

%files -f %{pyproject_files}
%license LICENSE
%doc README.md

%changelog
%autochangelog
```

:::{dropdown} Or you can start from a no-op spec file and build up

```{literalinclude} ../../examples/5_04_distros/foo.spec
:language: rpmspec
```

:::

Packit is a service available as a GitHub app and on GitLab, but can be extended upon request.
There are some [onboarding steps] to follow, after which you get distro builds in your upstream project.

```{image} 04_distros-packit_example.png
:alt: Packit jobs run in upstream
```

[onboarding steps]: https://packit.dev/docs/guide
[^2]: You can handle the building yourself, but why would you?
There are a bunch of edge-case issues you should not be subjected to.

## Shipping to distros

```{image} 04_distros-shipping.jpg
:alt: Packaging is dangerous business
```

Doing the whole packaging process yourself is daunting.
It is best to contact your local neighborhood distro packager.
You can find some in [#scitech] or [#python] channels in Fedora matrix who also share your interest.
[#devel] channel is where you find everyone else if you're feeling social.

If you want to do it all yourself, drop a line in [#join] or [#devel] and there will be people guiding you.
Note being a packager is as much a social problem as it is a technical one.

[#scitech]: https://matrix.to/#/#scitech:fedoraproject.org
[#python]: https://matrix.to/#/#python:fedoraproject.org
[#devel]: https://matrix.to/#/#devel:fedoraproject.org
[#join]: https://matrix.to/#/#join:fedoraproject.org

## What about non-Fedora?

Unfortunately equivalent CI tools do not seem to be available for other distros.

```{image} 04_distros-debian_files.png
:alt: Lots more files to manage
```

```{image} 04_distros-debian_control_file.png
:alt: Looks like a lot to maintain
```

But still the same advice applies, get in touch with a local packager and express your interest.
