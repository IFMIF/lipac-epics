# -----------------------------------------------------------------------------
# EPICS 7.0 distribution for LIPAc
#
# This is a multi-stage build.
# First, compile everything to a temporary workspace.
# -----------------------------------------------------------------------------

FROM ghcr.io/ifmif/lipac-debian AS build

# Prepare the working environment.
WORKDIR /work
COPY ./ .

# Unpack the external dependencies
RUN mkdir -p /opt/
RUN tar -xJf ./deps/open62541-v1.3.15.tar.xz -C /opt/

# Git submodules are a bit tricky and sometimes become desynchronized.
# To prevent issues, we forcefully clone/update the submodules.
RUN ./update_submodules.sh

# Compile EPICS.
RUN mkdir -p /opt/epics-7.0
RUN make EPICS_TARGET=/opt/epics-7.0 -j8

# Delete the static libraries to reduce the final size.
# We will only use dynamic linking.
WORKDIR /opt/epics-7.0/
RUN find -name '*.a' -delete

# -----------------------------------------------------------------------------
# Final step, create the clean image.
# -----------------------------------------------------------------------------

FROM ghcr.io/ifmif/lipac-debian
COPY --from=build /opt/ /opt/
