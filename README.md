RT Topology Library
===================

## Build status

[![Gitlab-CI](https://gitlab.com/rttopo/rttopo/badges/master/build.svg)]
(https://gitlab.com/rttopo/rttopo/commits/master)

## About

This is a fork of PostGIS' liblwgeom, moved to its own repository to
allow for faster release cycles independent from PostGIS itself.

It is a work in progress, still not ready for use.

Official code repository is https://git.osgeo.org/gogs/rttopo/librttopo.

A mirror exists at https://gitlab.com/rttopo/rttopo, automatically
updated on every push to the official repository.

## Building, testing, installing

### Unix

Using Autotools:

    ./autogen.sh # in ${srcdir}, if obtained from GIT
    ${srcdir}/configure # in build dir
    make # build
    make check # test
    make install # install

### Microsoft Windows

TODO

## Using

The rttopo library exposes an API to create and manage "standard"
topologies that use provided callbacks to take care of actual
data storage.

The topology standard is based on what was provided by PostGIS at its
version 2.0.0, which in turn is based on ISO SQL/MM (ISO 13249) with
the addition of the "TopoGeometry" concept.

The public header for topology support is `librttopo.h`.
The caller has to setup a backend interface (`RTT_BE_IFACE`) implementing
all the required callbacks and will then be able to use the provided
editing functions.

The contract for each callback is fully specified in the header.
The callbacks are as simple as possible while still allowing for
backend-specific optimizations.

The backend interface is an opaque object and callabcks are registered
into it using free functions. This is to allow for modifying the required
set of callbacks between versions of the library without breaking backward
compatibility.
