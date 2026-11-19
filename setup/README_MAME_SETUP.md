# MAME setup notes (per machine, not part of the app build)

The app launches MAME 0.289 (`bin\mame_executable.txt`) with `-skip_gameinfo`, which
removes MAME's system-information screen.

The Street Fighter EX games run on Capcom ZN-1/ZN-2 hardware, which MAME marks as
*imperfect graphics*, so MAME also shows an "imperfect emulation" warning screen.
That screen cannot be turned off from the command line; it is controlled by MAME's
own `ui.ini`. In the MAME folder, `ui.ini` contains:

    skip_warnings             1

("display fewer repeated warnings about imperfect emulation"). MAME then skips the
warning for games whose warnings were already shown before.
