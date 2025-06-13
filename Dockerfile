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

# Git submodules are a bit tricky and sometimes become desynchronized.
# To prevent issues, we forcefully clone/update the submodules.
RUN ./update_submodules.sh

# Compile EPICS.
RUN mkdir -p /opt/epics-7.0
RUN make EPICS_TARGET=/opt/epics-7.0
#RUN make prepare EPICS_TARGET=/opt/epics-7.0
#RUN make base EPICS_TARGET=/opt/epics-7.0
#RUN make support EPICS_TARGET=/opt/epics-7.0
#RUN make extensions EPICS_TARGET=/opt/epics-7.0

# Delete the static libraries to reduce the final size.
# We will only use dynamic linking.
WORKDIR /opt/epics-7.0/
RUN find -name '*.a' -delete

# -----------------------------------------------------------------------------
# Final step, create the clean image.
# -----------------------------------------------------------------------------

FROM ghcr.io/ifmif/lipac-debian
COPY --from=build /opt/epics-7.0/ /opt/epics-7.0/
