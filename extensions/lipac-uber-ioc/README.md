# Das LIPAc über IOC

## Introduction

This is a prebuilt IOC will all the support modules included in the [LIPAc EPICS distribution](https://github.com/IFMIF/lipac-epics).

It is intended to be used as a base to build a deploy your own IOCs easily: just add the DB files needed under `db` and adjust the startup script `./boot/st.cmd` to load them, and there you go.

## Trivia

The name is a reference to ubershaders, a concept used in graphics programming. Shaders are small programs that calculate pixel colors according to the material properties. Different materials can have different properties, and when the number of materials is high enough, it can result in a combinatorial explosion, so instead of writting a shader for each possible material, graphics developers write a huge shader able to handle every possible combination.

We apply the same logic to EPICS: instead of dealing with thousands or millions of possible combinations, we just create a big IOC that can handle everything, and everybody is happy.

Reference: https://dolphin-emu.org/blog/2017/07/30/ubershaders/
