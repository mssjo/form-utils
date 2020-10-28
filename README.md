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
underlying text editor. This won't automatically make the editor
recognise .frm and .prc files as FORM, but you can do that yourself
if you wish. (FORM doesn't even specify the extension used for header
files, but I follow the convention .hf to avoid confusion with regular
C/C++ header files.)

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
expressions manually, or bite the bullet and read them as-is.

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
which is vastly more readable. This works in general: separate symbols on the
same level of indentation by commas, and those on different levels by spaces.
All symbols that are supplied to the extermal multibracket command must also
be supplied as arguments to the`` `multibracket'`` macro. All FORM output that
is not affected by the macro is printed without being changed. All `print` options
(`+s` etc) are supported, but not options and abbreviations of `bracket`.
