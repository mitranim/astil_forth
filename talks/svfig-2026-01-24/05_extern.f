import' ../../forth/lang_s.f

\ Interfacing with external libraries is a breeze.



\ ## Easy IO

3 0 extern: write
1 0 extern: putchar

: type { str len } 1 str len write [ redefine ] ;
: cr 10 putchar ;

" hello world! (using libc I/O)" type cr



\ ## Easy memory

1 1            extern: calloc
1 cells calloc let:    ptr

ptr @ .   \ 0
123 ptr !
ptr @ .   \ 123
