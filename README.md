# form-utils
Useful accessories to Jos Vermaseren's computer algebra system FORM.
(See https://www.nikhef.nl/~form/ for information about FORM.)

FORM is wonderful and powerful, but sometimes a bit inconvenient to
use. As an avid FORM user, I have started to make some custom things
to make its use a bit smoother.

## Syntax highlighting
As far as I know, the only editor in widespread use that supports
FORM syntax highlighing is Vim. GitHub doesn't recognise it, but
someday I dream of adding it to Linguist. For Gnome-based editors, 
some wonderful person (https://github.com/vsht/form.lang) has written
a syntax highlighting specification. Here, I do the same for KDE.

To use, put the file form.xml provided here into
```
$HOME/.local/share/org.kde.syntax-highlighting/syntax/
```
and you're good to go: FORM will show up under the "scientific"
language category in any editor/IDE that uses Katepart as the
underlying text editor. This will also make the editor automatically
recognise `.frm` and `.prc` files as FORM files, as well as `.hf`,
which is one possible convention for FORM header files (FORM doesn't
specify the extension for these, but `.hf` makes for easier distinction
from C header files).

## Multibrackets
FORM's bracket command is great for increasing the readability of the
output, but it is unfortunately limited to one level. Therefore, if
```
bracket a,b,(...),z, F; 
print +s;
.end
```
results in the output
```
expression =  
   + a*b*(...)*z*F(aaa) * ( 
     long expression 
   )
   + a*b*(...)*z*F(bbb) * ( 
      another long expression 
   )
   ...
   + a*b*(...)*z*F(zzz) * (
      yet another long expression 
   );
```
the long `a*b*(...)*z` makes all lines unnecessarily crowded, but removing
either `a,b,(...),z` or `F` from the bracket would make them appear on each 
line in the long expressions. The only other solution is to restructure your
expressions manually, do some clumsy and limited tricks with the `dum_` 
function, or bite the bullet and read the expressions as-is.

This problem is resolved in a rather hacky way by including multibrackets.
After making `multibracket` and putting things in the relevant include paths,
simlply put
```
#include multibracket.hf
```
somewhere in your FORM program where declarations are allowed, and then replace
the `bracket` keyword with the macro`` `multibracket'``. Leave the other arguments
as they are. Then pipe your FORM output as follows:
```
> form your_program.frm | multibracket a,b,(...),z F
```
and the output will instead look like
```
expression = 

   + a*b*(...)*z * (
      + F(aaa) * (
         long expression
      )
      + F(bbb) * (
         another long expression
      )
      ...
      + F(zzz) * (
         yet another long expression
      )
   );
```
which is vastly more readable. This works in general: each level of bracketing
is given as a separate argument to `multibracket`, and symbols on the same level
are separated by commas (and/or spaces, if the argument is quoted). FORM's '...'
syntax can be used (e.g. `multibracket "a1,...,a55" "<f1x>,...,<f7x>"`), although 
quotes may be necessary to avoid confusing the shell. Set notation can not be used,
although it may be supported in the future. If a symbol occurs multiple times,
only its first appearance counts.

All symbols that are supplied to the extermal multibracket command must also
be supplied as arguments to the`` `multibracket'`` macro. All FORM output that
is not affected by the macro is printed without being changed. All `print` options
(`+s` etc) are supported, but not options and abbreviations of `bracket`, nor
`antibracket`.

The tags needed for multibracket formatting are not automatically removed.
You need to use the macro `` `nmultibracket'``, which works as an executable 
satement, to remove them so that expressions can subsequently be printed
without multibracketing, if you so wish.

KNOWN BUG: If the outside-the-bracket part is so long that FORM line-wraps it (i.e. if the " * (" ends up on a different line than the " + ", multibracket will not work (or rather, it will appear to work but will fail to balance parentheses). To avoid this, never multibracket FORM log filed (which are hard-wrapped) but instead feed FORM output directly into it, or via a file written on your conditions.
