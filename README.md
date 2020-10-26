## form-utils
Useful accessories to Jos Vermaseren's computer algebra system FORM.
(See https://www.nikhef.nl/~form/ for information about FORM.)

FORM is wonderful and powerful, but sometimes a bit inconvenient to
use. As an avid FORM user, I have started to make some custom things
to make its use a bit smoother.

# Syntax highlighting
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


