# A BeOS "/bin/poke" replacement for Haiku.

Any resemblance, and difference, with the original is absolutely intentional.

(Originally written and tested in 2005 under BeOS)

Note:

    The version for modern Haiku systems (2022) is a work in progress.
    Most likely to work better on 32 bits, at least for now.

Building requires:

 - haiku_devel
 - libedit_devel

Build with:

    > make

Basic (and really old and unfinished) docs: [poke.html](https://htmlpreview.github.io/?https://github.com/OscarL/poke/blob/master/poke.html)

Thanks to:

 - Eric Huss for his "The C Library Reference Guide".
 - Snippets.org
 - readline/editline's "fileman" sample program.
 - Rudolf Cornelissen for the testing and suggestions.

ToDo(s):

 - Better scripting than "cat commands.txt | poke - > output.txt" ?
 - Better command completion (complete arguments from history, for example).
 - New commands to force reads/writes to protected areas?
 - Only one command for virtual/physical addresses?
 - Fix the Broken English in the help and comments.
