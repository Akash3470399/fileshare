gcc -c -o obj/helper.o impl/helper.c -I./
gcc -c -o obj/main.o impl/main.c -I./
gcc -c -o obj/part.o impl/part.c -I./
gcc -c -o obj/receiver.o impl/receiver.c -I./
gcc -c -o obj/sender.o impl/sender.c -I./
gcc -c -o obj/timer.o impl/timer.c -I./
gcc obj/helper.o obj/main.o obj/part.o obj/receiver.o obj/sender.o obj/timer.o -o run
