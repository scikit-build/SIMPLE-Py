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
