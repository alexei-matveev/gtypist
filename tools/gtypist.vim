" Vim syntax file
" Language: GNU Typist script
" Maintainer: Felix Natter <fnatter@gmx.net> (bug-gtypist@gnu.org)
" URL: http://www.gnu.org/software/gtypist/ (tools/gtypist.vim in the
" source-package)
" Filenames: *.typ
" Last Change: (see ChangeLog below)

" README:
" Installation:
" 'vimrc' is ~/.vimrc on UNIX, and $HOME/_vimrc on DOS/Windows.
" This command should be at the top of your vimrc:
"  autocmd!  " Remove ALL autocommands for the current group
" (which avoids problems if vimrc is sourced more than once)
" and this should be in there too:
"  syntax on " turn on syntax highlighting
"
" vim 5.x:
" copy this file to where the syntax-files go and use this BEFORE "syntax on":
"  autocmd BufNewFile,BufRead *.typ set ft=gtypist
" (this is also the recommended way to install with vim 6.x on DOS/Windows)
"
" vim 6.x:
" create ~/.vim/syntax:
" $ mkdir -p ~/.vim/syntax
" and put this file in there:
" $ mv gtypist.vim ~/.vim/syntax
" put this BEFORE "syntax on" in vimrc:
"  autocmd BufNewFile,BufRead *.typ setf gtypist

" ChangeLog:
" Sat Oct 20 14:35:44 CEST 2001: initial version
" Sun Oct 21 02:48:24 CEST 2001: use "Error" for gtypistExitCmd

" TODO:
" - E: color number in Float

" For version 5.x: Clear all syntax-items
" For version 6.x: Quit when a syntax-file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn case match

syn keyword gtypistKeyword default Default NULL
" this is necessary so that ':' is not colored
syn match gtypistCmdSeparator ":" contained
syn match gtypistCmd "^[A-Za-z ]:" contains=gtypistCmdSeparator
syn match gtypistExitCmd "^X:" contains=gtypistCmdSeparator
syn match gtypistSetLabelCmd "^\*:" contains=gtypistCmdSeparator
syn keyword gtypistTODO TODO FIXME NOTE XXX contained
syn match gtypistComment "^[#!].*" contains=gtypistTODO
syn region gtypistDrillContent start="^[DdSs]:" skip="^ :" end="^" contains=gtypistCmd
syn match gtypistLabel "^\*:.*" contains=gtypistSetLabelCmd
syn match gtypistLabelRef "^[GYNF]:.*" contains=gtypistCmd
syn match gtypistKeybind "^K:[0-9]\+:" contains=gtypistCmd,gtypistCmdSeparator
syn match gtypistKeybindLabelRef "^K:[0-9]\+:.*" contains=gtypistKeybind
syn match gtypistBanner "^B:.*" contains=gtypistCmd

if !exists("did_gtypist_syntax_inits")
  let did_gtypist_syntax_inits = 1
  " note: the links are not very logical (i.e. gtypistLabel != Label),
  " because i.e. Label,Keyword look the same (and aren't readable in the GUI)
  " write to the maintainer if you know a way to make this more logical
  highlight link gtypistKeyword Keyword
  highlight link gtypistCmd SpecialChar
  highlight link gtypistExitCmd Error
  highlight link gtypistComment Comment
  highlight link gtypistDrillContent String
  highlight link gtypistTODO Todo
" Delimiter looks the same as SpecialChar, so this is commented out
"  highlight link gtypistCmdSeparator Delimiter
  highlight link gtypistLabel Type
  highlight link gtypistLabelRef Include
  highlight link gtypistKeybindLabelRef gtypistLabelRef
  highlight gtypistBanner term=bold cterm=bold gui=bold
endif

let b:current_syntax = "gtypist"

