LINK99 SBC - FUTURE OVERLAYS
============================

This directory is intentionally empty in Step 0.

The linker is designed so that large functional passes can later become
mapped code overlays without moving persistent linker state.  Candidate
future overlays are:

  OVL_REL     canonical REL parsing / module ingest
  OVL_RES     external-symbol resolution
  OVL_LIB     library scanning
  OVL_EMIT    executable / overlay image emission
  OVL_MAP     diagnostics and map output

Step 0 and the first functional milestones remain resident on purpose.
We will introduce an overlay only when measurements say it is needed.
