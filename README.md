Fragments of code that relate to the Logitech SWIFT API used to support
their *Cyberman* controller, along with others. The SWIFT API
("SenseWare InterFace Technology") was an extension to the mouse driver.
Several well-known DOS games supported this API.

Contents:

* `allegro-ww.c`: Allegro library code to support the Wingman Warrior,
  though this is actually the SWIFT API.
* `cyberman.h`: From the "SPHINX C-- Compiler" source code.
* `d2_mouse.c`: SWIFT code from the Descent 2 source code.
* `i_cyber.c`: From the Heretic/Hexen source code. This file was not in
  the released linuxdoom source code but it's pretty safe to say that
  this was compiled into the DOS Doom binaries too.
* `rott`: Directory containing Rise of the Triad files for supporting
  the SWIFT API.

The file `api.txt` contains some notes about the API inferred from these
files.

Since many DOS games support this interface, an interesting project
would be to implement support for the SWIFT API in DOSbox, hooking it up
to a more recent 6DoF controller, like the Wii remote or Sony SIXAXIS
controller.

