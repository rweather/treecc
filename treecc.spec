Name: treecc
Version: 0.0.1
Release: 1mdk
Source: http://www.southern-storm.com.au/download/%{name}-%{version}.tar.gz
License: GPL
BuildRoot: %{_tmppath}/%{name}-%{version}-root
Group: Development/Tools
Summary: Tree Compiler Compiler
%description
The treecc program is designed to assist in the development of compilers
and other language-based tools.  It manages the generation of code to handle
abstract syntax trees and operations upon the trees.
%prep
%setup -q
%build
%configure
make
%install
make DESTDIR=${RPM_BUILD_ROOT} install
%clean
if test "/" != "${RPM_BUILD_ROOT}"; then rm -rf ${RPM_BUILD_ROOT}; fi
%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README examples
%{_bindir}
%{_mandir}
%{_datadir}/treecc
