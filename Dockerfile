# Basic stuff
FROM ghcr.io/ifmif/lipac-debian:master AS build

# Prepare the working environment
WORKDIR /work/epics-7.0
COPY ./ .

# Clone/update the submodules
RUN git submodule update --init --recursive
RUN git submodule update --remote --merge

# Compile EPICS
COPY CONFIG_SITE.local /work/epics-7.0/
RUN mkdir -p /opt/epics-7.0
RUN make EPICS_TARGET=/opt/epics-7.0 -j8

# Delete the temporary work files
WORKDIR /opt/epics-7.0/
RUN rm -rf /work
RUN find -name '*.a' -delete

# Final step, create the clean image
FROM ghcr.io/ifmif/lipac-debian:master
COPY --from=build /opt/epics-7.0/ /opt/epics-7.0/
