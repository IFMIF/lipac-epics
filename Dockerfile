# -----------------------------------------------------------------------------
# EPICS 7.0 distribution for LIPAc
#
# This is a multi-stage build.
# First, compile everything to a temporary workspace.
# -----------------------------------------------------------------------------

FROM registry.ifmif.org/tools/lipac-builder AS build

# Prepare the working environment.
COPY . /work
WORKDIR /work

# Unpack the external dependencies
RUN mkdir -p /opt/
RUN tar -xJf ./deps/open62541-v1.3.15.tar.xz -C /opt/
RUN ln -s /usr/include/libxml2/libxml /usr/include/

# Git submodules are a bit tricky and sometimes become desynchronized.
# To prevent issues, we forcefully clone/update the submodules.
RUN ./update_submodules.sh

# Compile EPICS.
RUN mkdir -p /opt/epics-7.0
RUN make EPICS_TARGET=/opt/epics-7.0 -j8

# -----------------------------------------------------------------------------
# Final step, create the clean image.
# -----------------------------------------------------------------------------

FROM registry.ifmif.org/tools/lipac-builder
COPY --from=build /opt/ /opt/
