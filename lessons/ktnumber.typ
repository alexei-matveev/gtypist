# created by ktouch2typ.pl from /mnt/src/ktouch-1.0/ktouch/number.ktouch
# on Sun Jul 22 21:42:20 CEST 2001

G:MENU

# Level 1
*:S_LESSON1
B:                           Lesson 1: 45
I: (1)
O:455454545445454545454
 :5454555445454554454
 :544445454454454545545
 :544545445544545454545
I: (2)
O:55445555445545545455
G:E_LESSON1

*:S_LESSON2
B:                           Lesson 2: 06
I: (1)
O:6006540604605650650546
 :6056050565464065060654
 :60650446506060606465406
 :0465406060646054060406
I: (2)
O:06546006406405065065460
 :6054406540650456600460
G:E_LESSON2

*:S_LESSON3
B:                           Lesson 3: 19
I: (1)
O:190190194609199019094
 :590490491095094019049
 :56040916460910956491
 :910919049096014540916
I: (2)
O:50455901609564019
G:E_LESSON3

*:S_LESSON4
B:                           Lesson 4: 37
I: (1)
O:731373164353197
 :1316406031379413164973
 :4316090373737033703037973
 :197303197379134679037
I: (2)
O:51305497133464377
G:E_LESSON4

*:S_LESSON5
B:                           Lesson 5: 82
I: (1)
O:268292164372156428283802
 :828314619872378031873218
 :7321783642083274138
 :5284208083549398328
G:E_LESSON5

*:S_LESSON6
B:                            Lesson 6: +
I: (1)
O:1+424+414+5+8+5+685+54
 :54+454+87+85+4547+548+54
G:E_LESSON6

*:S_LESSON7
B:                            Lesson 7: .
I: (1)
O:5.445+5.5+5665.66+6564.64
 :465.4+0.4+0.005+465.56+54
 :46.4+4654.4
G:E_LESSON7

*:S_LESSON8
B:                            Lesson 8: /
I: (1)
O:4/5+6.0/7
 :5/6
 :6/8.6+5
G:E_LESSON8

*:S_LESSON9
B:                            Lesson 9: *
I: (1)
O:4*3*3
 :-4*5
 :-7+5*5*4*0.6
G:E_LESSON9

*:S_LESSON10
B:                           Lesson 10: -
I: (1)
O:45-65.54-45.2+8-9-0.01
 :-9+5-+65*0.5
 :45+65
 :6.5+5.6
I: (2)
O:9.5-6.4
 :-
G:E_LESSON10


# jump-table
*:E_LESSON1
Q: Do you want to continue to lesson 2 [Y/N] ?
N:MENU
G:S_LESSON2
*:E_LESSON2
Q: Do you want to continue to lesson 3 [Y/N] ?
N:MENU
G:S_LESSON3
*:E_LESSON3
Q: Do you want to continue to lesson 4 [Y/N] ?
N:MENU
G:S_LESSON4
*:E_LESSON4
Q: Do you want to continue to lesson 5 [Y/N] ?
N:MENU
G:S_LESSON5
*:E_LESSON5
Q: Do you want to continue to lesson 6 [Y/N] ?
N:MENU
G:S_LESSON6
*:E_LESSON6
Q: Do you want to continue to lesson 7 [Y/N] ?
N:MENU
G:S_LESSON7
*:E_LESSON7
Q: Do you want to continue to lesson 8 [Y/N] ?
N:MENU
G:S_LESSON8
*:E_LESSON8
Q: Do you want to continue to lesson 9 [Y/N] ?
N:MENU
G:S_LESSON9
*:E_LESSON9
Q: Do you want to continue to lesson 10 [Y/N] ?
N:MENU
G:S_LESSON10
*:E_LESSON10
G:MENU


*:MENU
*:MENU_P1
K:1:S_LESSON1
K:2:S_LESSON2
K:3:S_LESSON3
K:4:S_LESSON4
K:5:S_LESSON5
K:6:S_LESSON6
K:7:S_LESSON7
K:8:S_LESSON8
K:9:S_LESSON9
K:10:S_LESSON10
K:11:NULL
K:12:EXIT

B:               Lesson selection menu - [page 1 of 1]
T:this file contains the following 10 lessons:
 :
 :        Fkey 1 - 45
 :        Fkey 2 - 06
 :        Fkey 3 - 19
 :        Fkey 4 - 37
 :        Fkey 5 - 82
 :        Fkey 6 - +
 :        Fkey 7 - .
 :        Fkey 8 - /
 :        Fkey 9 - *
 :        Fkey 10 - -
 :        Fkey 12 - Quit program

*:EXIT
X:
